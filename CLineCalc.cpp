#include "CLineCalc.h"
#include <QDebug>
#include <math.h>
#define max(a, b)  (((a) > (b)) ? (a) : (b))

CLineCalc::CLineCalc(){
    varListsReceived=false;
    funStr[0]="sin";   fun1[0]=sin;
    funStr[1]="cos";   fun1[1]=cos;
    funStr[2]="tan";   fun1[2]=tan;
    funStr[3]="sinh";  fun1[3]=sinh;
    funStr[4]="cosh";  fun1[4]=cosh;
    funStr[5]="tanh";  fun1[5]=tanh;
    funStr[6]="exp";   fun1[6]=exp;
    funStr[7]="sqrt";  fun1[7]=sqrt;
    funStr[8]="asin";  fun1[8]=asin;
    funStr[9]="acos";  fun1[9]=acos;
    funStr[10]="atan"; fun1[10]=atan;
    funStr[11]="log";  fun1[11]=log;
    funStr[12]="abs";  fun1[12]=fabs;
    fun2[0]=power;
    fun2[1]=prod;
    fun2[2]=div;
    fun2[3]=sum;
    fun2[4]=subtr;
    stringOperators="^*/+-"; //l'indice del simbolo è pari all'indice della fun2 qui sopra definito
//    rxAlphabet=QRegExp("[0-9a-zA-Z .-+*/()]");
    rxAlphabet=QRegExp("[\\^0-9a-zA-Z .-+*/()]");
    rxDatumFunPtr=QRegExp("[#@&]");
    rxDatumPtr=QRegExp("[#@]");
    rxLetter=QRegExp("[a-zA-Z]"); //initial character of a variable
    rxLetterDigit=QRegExp("[a-zA-Z0-9]");    rxNotLetterDigit=QRegExp("[^a-zA-Z0-9]");
    rxNotDigit=QRegExp("[^0-9]");  //not a digit
    rxNum=QRegExp("[0-9.]");   rxNotNum=QRegExp("[^0-9.]"); //not a character allowable in a number. 'E' and 'e' are considered not-allowable since they are evaluated separtely to manage exponent sign.
    rxNumSepar=QRegExp("[-+*/() ]"); //carattere ammissibile fra un numero ed il successivo
    rxBinOper=QRegExp("[-+*/^]"); //operator
    pConst=0;
    pFun=0;
    pVar=0;
    pOper=0;
}

QString CLineCalc::getLine(QByteArray line_){
  /*Questa funzione overloaded va usata quando non ci sono variabili in Line.
*/
    QString err;
    QList <QByteArray> nameList;
    float **y=0;
    //Mando in esecuzione la getLine con tutti gli argomenti con una lista di variabili vuota:
    err=getLine (line_, nameList, y);
    return err;
}

QString CLineCalc::getLine(QByteArray line_, QList <QByteArray> nameList_, float ** y_){
    /* In questa funzione si trasferisce a CLineCalc la linea da analizzare e si fanno
     * le seguenti semplici operazioni:
     * - allocazione dei vettori pConst, pFun, pUnaryMinus, pVar
     * - sostituzione del carattere virgola col carattere punto come separatore della
     *   parte decimale
     * - una semplice diagnostica
     * - sostituzione di costanti, funzioni, variabili, operatori binari e operatore
     *   unario negativo con i rispettivi caratteri di puntamento (#, @, &, + e -)
     * In realtà nel caso di '-', indicatore di unario negativo, non serve un puntatore
     * perché esiste un unico tipo di unario negativo, e quindi questo segno diviene
     * direttamente una richiesta di unario negativo.
     * Alla fine in line, oltre le parentesi resteranno solo i caratteri (#, @, &, + e -).
     * Il '+' rappresenta le 4 operazioni matematiche (quella che è realmente
     * rappresentata è indicata dal corrispondente *pOper[]), mentre il '-' rimane solo
     * per l'unario negativo.
     *
     * PARAMETRI PASSATI:
     * line_ contiene la linea da analizzare (e successivamente valutare)
     * nameList_ contiene la lista dei nomi delle variabili presenti in line_
     * y_ contiene per ogni riga il vettore dei valori delle variabili presenti in nameList_
*/
   int i, j, k;

   constantsConverted=false;
   noVariables=false;
   varListsReceived=false;
   yReceived=false;
   line=line_.simplified();
   line0=line;
   delete pConst;
   delete pFun;
   delete pVar;
   delete pOper;

   if(line.length()<1)return "null strings are not accepted";

   //per semplicità alloco spazio per costanti, variabili, funzioni e operatori  uno per ogni carattere della stringa.
   pConst=new float[line.length()];
   typedef double (*FuncPtr)(double);
   pFun = new FuncPtr[line.length()];
   pVar=new float*[line.length()];
   pOper=new int [line.length()];
   //Il carattere di separazione decimale usato internamente è '.'. Pertanto se trovo ',' faccio la conversione. Però può accadere che io debba usare la stringa originale per l'emissione di messaggi d'errore. Pertanto mantengo la stringa con il separatore originale in line0.
   line.replace(',','.');

   // **** ora una semplice diagnostica.
   //1) verifica che tutti i caratteri appartengono all'alfabeto previsto.
   for(i=0; i<line.length(); i++){
     if(rxAlphabet.indexIn(line.mid(i,1))<0)
        return "The string cannot contain character "+line.mid(i,1)+"\'";
   }
   //2) dopo un operatore non deve essere presente un altro operatore:
   j=-1;
   while(true){
     j++;
     j=rxBinOper.indexIn(line,j);
     if(j>=0){
     //ora in j è l'indice di un operatore. Il carattere successivo non deve essere un operatore; se è ' ' quello ancora dopo non dev'essere un operatore.
         k=rxBinOper.indexIn(line,j+1);
         if(k==j+1 || (k==j+2 && line[j+1]==' ') )
          return "The string contains consecutive operators without numbers or brackets in between.";
     } else break;
   }
   //3) bilanciamento parentesi: lungo la stringa non ci devono essere più chiuse che aperte e alla fine devono essere bilanciate
   int par=0;
   for(i=0; i<line.length(); i++){
     if(line[i]=='(') par++;
     if(line[i]==')') par--;
     if(par<0)break;
   }
   if(par!=0) return "The string contains unbalanced brackets"; 

  //4) se è presente un nome di funzione deve essere seguito dal carattere '(' (in tal modo si intercettano nomi di variabili coincidenti con nomi di funzione,  inammissibili)
  for(i=0; i<MAXFUNCTIONS; i++){
      int index=-1;
      while (1){
        index=line.indexOf(funStr[i],++index);
        if(index>-1){
          //Ho trovato una stringa funzione, deve essere seguita da '('
          //Trovo il carattere dopo la stringa:
          j=rxNotLetterDigit.indexIn(line,index);
          if(j<1 || line[j]!='('){
            QString str;
            if(j<1)
              str=line.mid(index,line.count()-index);
            else
              str=line.mid(index,j-index+1);
            return "string "+ str +
            "appears to be the name of a built-in function\n"
            "but it is not followed by the character '(',\n"
             "that is against rules.";
          }
        }else
          break;
      }
   }
   ret=substPointersToConsts();
   if(ret!="") return ret;
   ret=substPointersToFuns();
   if(ret!="") return ret;
   if(nameList_.count()>0)
     ret=getVarPointers(nameList_, y_);
   simplify();  //toglie le parentesi inutili.
   substPointersToOpers();
   return ret;
}


float CLineCalc::compute(int iVal){
  /* Quando questa funzione è chiamata line contiene già puntatori a costanti, variabili, funzioni e operatori.
  Ne fa l'interpretazione. iVal è l'indice di variabile delle variabili-funzione. Se ad esempio devo sommare due sinusoidi di 100 elementi, questa funzione verrà richiamata con iVal che va da 0 a 99.
*/
   bool blanked=false;
   static bool recursiveCall=false;
   int d1, //indici del dato  1
         j;  //indice generico
   float result;
   QString err;

/* Questa funzione altera "line" durante il calcolo. Può essere chiamata più volte dall'esterno con diversi valori di iVal, e può essere anche chiamata ricorsivamente da se stessa durante l'elaborazione delle espressioni fra parentesi.
La variabile contenente la stringa da analizzare, che rimane stabile da una chiamata dall'esterno all'altra di compute() è prepLine.
Ogni volta che compute() è chiamata dall'esterno ricopio prepLine in line, mentre durante le chiamate di compute() a compute() non effettuo questa ricopiatura*/
   if(!recursiveCall)line=prepLine;
   recursiveCall=false;

   // se vi è una parentesi aperta richiamo ricorsivamente la funzione sostituendo '(' con '['
   if((j=line.indexOf('('))>=0){
       line[j]='[';
       recursiveCall=true;
       compute(iVal);
   }
   //adesso effettuo l'analisi fra l'ultima '[' e la prima ') che la segue
   int start=line.lastIndexOf('['),
        end=line.indexOf(')',start);
   if(start==-1){
       start=0;
       end=line.length()-1;
   }else{
       //elimino le parentesi in quanto nel resto della funzione ne tratto il contenuto riconducendolo ad un unico '#' o '@' all'interno di spazi
       line[start]=' ';
       line[end]  =' ';
       blanked=true;
   }
   //nel caso di (x) non occorre fare i calcoli
   if(end-start==2 && blanked){
     d1=start+1;
     if(line[d1]=='#')result=pConst[d1];
     if(line[d1]=='@')result=pVar[d1][iVal];
     return result;
   }

   /* Nelle seguenti righe è implementato l'ordine di priorità fra le operazioni, che è quella del C standard:
     - prima di tutto si fanno le chiamate a funzione (da SX a DX)
     - poi si fa la potenza
     - poi si applicano gli operatori unari negativi esistenti
     - poi si fanno moltiplicazioni e divisioni (da sinistra verso destra)
     - poi si fanno addizioni e sottrazioni (da sinistra verso destra)
   */

   computeFun1(start, end,  iVal); //funzioni con ()
   ret=computeOperator(start, end, 0, 1, iVal); // operatore '^'
   if(ret!="") return 0;
   computeUnaryMinus(start, end,  iVal); //operatore unario negativo
   ret=computeOperator(start, end, 1, 2, iVal);  // * e /: vedi ordine delle fun2[]
   if(ret!="") return 0;
   ret=computeOperator(start, end, 3, 2, iVal); // + e -: vedi ordine delle fun2[]
   if(ret!="") return 0;


   //a questo punto la stringa contiene fra start ed end un unico carattere '#' o '@' e il relativo valore è il risultato, a meno dell'eventuale '-' unario a sinistra della parentesi considerata:
   d1=line.indexOf('#',start);
   if(d1>-1)
     result=pConst[d1];
   else{
     d1=line.indexOf('@',start);
     if(d1<0) err="ERROR d1";
     result=pVar[d1][iVal];
   }
   return result;
}


QString CLineCalc::computeFun1(int start, int end, int iVal){
 /* Percorre sequenzialmente la stringa line, fra gli indici start ed end, ed esegue da sinistra a destra tutte le chiamate a funzioni specificate dai puntatori pFun.
*/
  int i=0,j;
  double y;
  while(true){
    i=line.indexOf('&',start);
    if(i==-1)return "";
    j=i+1;
    while(line[j]==' ')j++;
    //A questo punto l'argomento della funzione può essere una costante (carattere '#') o una variabile (carattere '@')
    if(line[j]=='#')
      y=pFun[i](pConst[j]);
    if(line[j]=='@')
      y=pFun[i](pVar[j][iVal]);
    line[i]='#';
    line[j]=' ';
    pConst[i]=(float)y;
  }
}

QString CLineCalc::computeOperator(int start, int end, int startOp, int numOp, int iVal){
    /* Percorre sequenzialmente la stringa line, fra gli indici start ed end, ed esegue da sinistra a destra tutte le operazioni binarie specificate da startOp e numOp.
     * Cioè, più precisamente:
     * - gli operatori binari hanno un ordine, definito della posizione dei relativi simboli nella stringa stringOperators, ovvero, equivalentemente, dall'ordine delel funzioni nel vettore fun2[]
     * - se per esempio è passato startOp=3 e numOp=2  due operatori da considerare in numero di due, a partire dall'indice 3, quindi '+' e '-'.
     * Il valore di numOp sarà pari a 1 quando si fa il calcolo con l'operatore prioritario '^', a 2 negli altri due casi ('*' '/' e '+' '-').
Dopo il calcolo metto '#' dove c'era l'operatore e in pConst del suo indice il relativo valore.

Significato dei parametri passati:
   - start e end sono il primo e l'ultimo indice di line da considerare
   - startOp e numOp sono stati già descritti
   - iVal è l'indice delle variabili-array eventualmente presenti (argomento di compute(i).
    */
    int d1, d2, //indici dei dati  1 e 2
            o, // indice dell'operatore
            j;  //indice generico
    float x1, x2, y;
    for(o=start; o<end; o++){
      //Proseguo nel loop finché non trovo un operatore fra quelli che devo trattare nella presente chiamata (in oper)
      if(line[o]!='+')
        continue;//Necessario perchè non devo considerare gli operatori se in line non c'è '+'
      if(numOp==1){
        if(pOper[o]!=startOp)
          continue;
      }else{ //numOp==2
        if(pOper[o]!=startOp && pOper[o]!=startOp+1)
             continue;
      }
      //cerco il dato a sinistra
      d1=qMax(line.lastIndexOf('#',o-1),line.lastIndexOf('@',o-1));
      if(d1<start)
        return "";
  //       {ret="Internal error \"firstOperPosition\"";return 0;}
      if(line[d1]=='#')
        x1=pConst[d1];
      else{ //a questo punto line[d1]='@'
           x1=pVar[d1][iVal];
      }//cerco il dato a destra:
      d2=rxDatumPtr.indexIn(line,o+1);
      if(d2<0||d2>end)
          return "";
    //{ret="Internal error \"secondOperPosition\"";return 0;}
      if(line[d2]=='#')
        x2=pConst[d2];
      else{  //a questo punto line[d2]='@'
        x2=pVar[d2][iVal];
      }
      //effettuo il calcolo:
      ret=fun2[pOper[o]](x1,x2,y);
      if(ret.length()>0)return ret;
      //sostituisco il risultato: metto '#' dove c'era l'operatore e in pConst del suo indice il relativo valore.
      for(j=d1; j<=d2; j++)line[j]=' ';
      line[o]='#';
      pConst[o]=y;
    }
    return "";
}


QString CLineCalc::computeUnaryMinus(int start, int end, int iVal){
    /* Percorre sequenzialmente la stringa line, fra gli indici start ed end, ed esegue la valutazione del segno meno unario se presente.
     * Dopo il calcolo metto un '#' dove c'era il dato ('#' o '@') e il corrispondente valore è cambiato di segno

Significato dei parametri passati:
   - start e end sono il primo e l'ultimo indice di line da considerare
   - iVal è l'indice delle variabili-array eventualmente presenti (argomento di compute(i).
    */
  int i,j;
  i=end-line.count();
  while(true){
    i=rxDatumFunPtr.lastIndexIn(line,i);
    if(i<start)break;
    j=i;
    while(j>0){
      j--;
      if(line[j]==' ')
        continue;
      else
        break;
    }
    if(j<start)
        return"";
    if(line[j]=='-'){
      line[j]=' ';
      if(line[i]=='#')
        pConst[i]=-pConst[i];
      if(line[i]=='@'){
        pConst[i]=-pVar[i][iVal];
        line[i]='#';
      }
    }
    if (j-1<start)return "";
    i=j-1-line.count();
  }
  return "";
}


QString CLineCalc::getVarPointers(QList <QByteArray> nameList, float ** y_){
/* In questa funzione si ricevono i nomi delle variabili presenti in "line"
 * ed puntatori ai rispettivi valori.
 * Il secondo una matrice, realizzata con la mia funzione "CreateFmatrix", quindi
 * attraverso puntatore ad un vettore di puntatori alle righe.
 * Ogni riga contiene i dati numerici di una delle funzioni, con corrispondenza
 * ordinata ad i nomi riportati in nameList.
 *
 * I valori di y devono essere allocati e disponibili esternamente all'oggetto CLineCalc.
 *
 * I nomi delle variabili vengono qui sostituiti con il carattere '@', ed in corrispondenza
 * della sua posizione, il relativo valore viene messo nel corrispondente puntatore a float.
 *
 * la funzione viene chiamata all'interno di getLine, dopo che essa ha chiamato:
 * - substPointersToConsts, quindi si sa che non sono presenti numeri espliciti;
 * - substPointersToFuns, quindi si sa che non sono presenti nomi di funzioni;
*
 * Ricordiamo che y_ punta già ad un vettore di puntatori alle variabili-funzione di cui
 * si vuole fare l'elaborazione. Questi puntatori vanno però rimappati in pVar in quanto
 * pVar è indicizzato sulla posizione dei rispettivi caratteri in "line". Il numero di
 * elementi di pVar è inferiore a quello di variabili funzione.

*/
   int i,k1;

   yReceived=true;

   //Verifico che i nomi di variabili passati comincino per lettera:
   foreach (QByteArray ba,  nameList){
     if(rxLetter.indexIn(ba,0)!=0)
       return "err0";
   }
   //Verifico che nessuna variabile passata abbia un nome uguale a quello di una funzione
   foreach (QByteArray ba,  nameList){
     for( i=0; i<MAXFUNCTIONS; i++)
       if (ba==funStr[i])
           return"you cannot use variable names of built-in mathematical functions";
   }
   //Ora verifico che i nomi di variabile presenti in line corrispondano tutti a variabili passate.
   //Occorre ricordarsi che questa funzione è chiamata a valle di substPointersToConst e substPointersToFuns(), ma a monte di substPointersToOpers()
   //Creo una stringa temporanea in cui trasformo in ' ' tutto quanto non interessa (caratteri #@&^*/+-  e le parentesi); poi riduco gli spazi interposti fra i token a 1 con simplified(), così faccio più agevolmente il check:
   QByteArray auxStr=line;
   auxStr.replace('#',' ');
   auxStr.replace('@',' ');
   auxStr.replace('&',' ');
   auxStr.replace('^',' ');
   auxStr.replace('*',' ');
   auxStr.replace('/',' ');
   auxStr.replace('+',' ');
   auxStr.replace('-',' ');
   auxStr.replace('(',' ');
   auxStr.replace(')',' ');
   auxStr=auxStr.simplified();
   // Se ora auxStr=="", in line non ho alcuna variabile (solo costanti)
   if(auxStr==""){
       noVariables=true;
       return"";
   }
   i=0;
   while(true){
     QByteArray token;
       k1=auxStr.indexOf(' ',i);
     if(k1<1)
       token=auxStr.mid(i,auxStr.count()-i);
     else
       token=auxStr.mid(i,k1-i);
     bool found=false;
     foreach(QByteArray ba, nameList){
       if (ba==token) {
         found=true;
         break;
       }
     }
     i=k1+1;
     if(!found)
       return "Error: the substring \""+token+"\" is not a variable name.";
     if(k1<1)break;
   }

   foreach(QByteArray varStr, nameList){
     //Cerco tutte le occorrenze di varStr in line
     i=-1;
     while(true){
       i=line.indexOf(varStr,++i);
       if(i==-1)
         break;
       //Devo filtrare il caso in cui in line vi sia una stringa di cui varStr è una sottostringa. Ad es. se varStr è "s" non devo intercettare la sottostringa di line "s2"
       QChar c=QChar(line[i+varStr.count()]);
       if(c.isLetterOrNumber())
          continue;
       line[i]='@';
       for(int j=1; j<varStr.count()-1; j++)
         line[i+j]=' ';

       //Ora associo alla variabile trovata il relativo puntatore ai dati:
       int index=nameList.lastIndexOf(varStr);
       if(index<0) return "variable name not found in memory: \""+varStr+"\"";
 //il seguente pVar[i] è un puntatore alla cella contenente il primo elemento della variabile funzione puntata dal carattere in i-esima posizione di line. I valori numerici saranno pVar[i][j].
       pVar[i]=y_[index];
     }
   }
   prepLine=line; //la linea preparata deve ricevere spazio proprio rispetto a line. Infatti durante l'interpretazione line viene modificata fino a contenere l'unico carattere '#'. Quando compute() viene chiamata iterativamente, si partirà sempre dalla prepLine che è la linea preparata per il calcolo.
   return "";
}

SVarNums CLineCalc::readVarXYNums(QString varStr){
    /* semplice routine privata che interpreta il nome di una variabile contenuta in varStr secono lo standard XY. Il nome può quindi essere f#v# o v#.
Se il nome è incorretto ritorna varNum=-1 e fileNum indefinito.
Se il nome è di tipo v# il filenum è pNum*/
    bool  ok=false;
    int  j, k, fileNum=defaultFileNum, varNNum;
    SVarNums varNums;  //E' il valore di ritorno. Se varNNum<0 vi è stato un errore
    switch(varStr.at(0).toLatin1()){
    case 'f':
        j=rxNotDigit.indexIn(varStr,1); //j dovrebbe contenere il primo carattere dopo il numero dopo f
        if(j<0) goto errorReturn;
        fileNum=varStr.mid(1,j-1).toInt(&ok);
        if(ok==false) goto errorReturn;
        if(varStr[j]!='v') goto errorReturn;
    case 'v':
        if(varStr.at(0).toLatin1()=='v') j=0;
        k=rxNotDigit.indexIn(varStr,j+2); //k dovrebbe contenere il primo carattere dopo il numero dopo v. Siccome tale carattere non deve esistere, mi attendo k<0
        if(k>=0) goto errorReturn;
        varNNum=varStr.mid(j+1,k-j).toInt(&ok);
        if(ok==false) goto errorReturn;
        if (varNNum>varNumVect[fileNum-1])
            goto errorReturn;
        varNums.fileNum=fileNum;
        varNums.varNum=varNNum;
        break;
    default:
        varNums.varNum=-1;
    }
    if(varStr.at(0).toLatin1()=='v') varNums.fileNum=defaultFileNum;
    return varNums;

errorReturn:
    varNums.varNum=-1;
    return varNums;
}


void CLineCalc::simplify(){
  /* elimino le parentesi inutili per velocizzare l'esecuzione dei successivi compute()
   * Per semplicità opero su un solo livello di parentesi anche se con la ricorsione sarebbe facile da generalizzare.
   * (Magari più in là...)
*/
  int i=0, j;
  while(true){
    i=line.indexOf("(");
    if (i<0) break;
    j=i+1;
    while(line[j]==' ') j++;
    if(line[j]!='#' && line[j]!='@') break;
    while(line[++j]==' ') j++;
    if(line[j]!=')') break;
    //A questo punto sono sicuro che fra le parentesi vi è solo un dato e spazi, quindi posso togliere le parantesi
    line[i]=' ';
    line[j]=' ';
    i++;
  }
  prepLine=line;
  return;
}



QString CLineCalc::substPointersToConsts(){
    /* In questa funzione si sostituiscono le costanti con il carattere '#', ed in corrispondenza della sua posizione, il relativo valore viene messo nel corrispondente puntatore a float.
Per prima cosa si tratta l'eventuale unario che si trova a inizio stringa, e poi si procede con numeri tutti positivi */
   bool eol=false, unary=false, unaryMinus=false, ok;
   int i=0,j, k1, k2;
   QByteArray numStr;

   i=-1;
   while(!eol){
     i++;
     i=rxNum.indexIn(line,i);
     if(i<0){
       eol=true;
       break;
     }
     if(i>0){
        // se immediatamente prima di i vi è un digit o una lettera il digit che ho trovato è all'interno di una variabile e non mi interessa
        if(rxLetterDigit.indexIn(line,i-1)==i-1) continue;
        //devo verificare se il numero è preceduto da un operatore unario ('+' o '-'). Vado verso monte saltando gli spazi e vedo cosa c'è::
        k1=i;
        while(line[--k1]==' ');
        if(line[k1]=='+' ||line[k1]=='-'){
          //alla posizione k1 vi è un unario se prima di esso, escluso al più un ' ', non vi è nulla o una parentesi aperta
          if(k1==0 && line[1]!='(')
              unary=true;
          else if(k1>0){
              k2=line.lastIndexOf('(',k1-1);
              if(k2==k1-1 || (k2==k1-2 && line[k1-1]==' ')) unary=true;
          }
          if(unary && line[k1]=='-') unaryMinus=true;
          if(unary)line[k1]=' ';
        }
      }
      j=rxNotNum.indexIn(line,i+1);
      //Se il numero era in formato esponenziale, line[j] contiene la lettera "E" o "e".
      if(j>0){
        if(line[j]=='E'||line[j]=='e'){
          if(line[j+1]=='+'||line[j+1]=='-')
            j=rxNumSepar.indexIn(line,j+2);
          else{
            j=rxNumSepar.indexIn(line,j+1);
          }
        }else{ //a questo punto ho un non numero in j oltre i, e devo verificare se è un separatore valido
          if(rxNumSepar.indexIn(line,j)!=j)
            return "invalid substring \""+line.mid(i,j-i+1)+"\"";
        }
      }
    if(j<0)j=line.length();
      numStr=line.mid(i,j-i);
      if(unaryMinus)
         pConst[i]=-numStr.toFloat(&ok);
      else
        pConst[i]=numStr.toFloat(&ok);
      if(!ok) return "Erroneous number substring: \""+numStr+"\"";
      unary=false;
      unaryMinus=false;
      line[i]='#';
      if(j<0)j=line.length();
      for(int k=i+1;k<j;k++)
          line[k]=' ';
   }
   constantsConverted=true;
   return"";
}

QString CLineCalc::substPointersToFuns(){
    /* In questa funzione si sostituiscono le chiamate a funzione con il carattere
     *  '&', ed in corrispondenza della sua posizione il relativo valore viene messo
     * nel corrispondente puntatore a float.
     * QUESTA FUNZIONE  dà per scontato che sia stata già eseguita
     * substConstsToPointers().
     * Possono invece essere presenti variabili, ma tutte con nomi diversi dai nomi
     * di funzione ammessi.
    */

    int  i,j; //indici generici

   //check che questa routine sia stata chiamata a valle di substPointersToConsts():
   if (!constantsConverted)
     return "functionsToPointers() called ahead of constantsToPointers()";

   //Percorro line, individuo i nomi di funzioni e sostituisco con i rispettivi puntatori.

   for(i=0; i<MAXFUNCTIONS; i++){
       int index=-1;
       while (1){
         index=line.indexOf(funStr[i],++index);
         if(index>-1){
           //Qui non sono ancora certo di aver trovato una stringa funzione, in quanto potrebbe essere la prima parte di un nome di variabile lungo, ad es. sinModified (contiene sin)
           if(line[index+funStr[i].count()]!='(')
               continue;
/* Ora devo verificare che non ho trovato una stringa funzione illecita per avere a SX del testo, ad es. "xsin"  invece di "sin". Devo quindi escludere lettere e digits
*/
            if(index>0){
                QChar c=QChar(line[index-1]);
                if(c.isDigit() || c.isLetter()){
                  QString str;
                  str=line.mid(index-1,1)+funStr[i];
                  return "string \""+ str +
                  "\" appears to be the name of a non-allowed function.\n";
              }
            }
           //Ho trovato una stringa funzione, deve essere seguita da '('
           //Trovo il carattere dopo la stringa:
           j=rxNotLetterDigit.indexIn(line,index);
           if(j<1 || line[j]!='('){
             QString str;
             if(j<1)
               str=line.mid(index,line.count()-index);
             else
               str=line.mid(index,j-index+1);
             return "string "+ str +
             "appears to be the name of a built-in function\n"
             "but it is not followed by the character '(',\n"
              "that is against rules.";
           }else{
             line[index]='&';
             pFun[index]=fun1[i];
             for (j=1; j<funStr[i].count(); j++)
               line[index+j]=' ';
             break;
           }
         }
         goto nextFunction;
       }
       nextFunction:
       continue;
    }
   return "";
}

QString CLineCalc::substPointersToOpers(){
    /* In questa funzione si sostituiscono i segni di operatore binario con il segno unificato '+'.
     * L'informazione sul tipo di operatore originariamente presente è memorizzata nel corrispondente elemento del vettore pOper.
     * Questo rende più efficiente la successiva esecuzione di compute().
     * (cfr. developer.docx)
     * Per quanto riguarda gli operatori unari trasforma l'unario '+' in ' ' e lascia l'unario '-' invariato.
     * QUESTA FUNZIONE dà per scontato che tutte le variabili e funzioni siano già state sostituite con i rispettivi puntatori.
    */
    int i,j;
    //Prima gestisco gli operatori che hanno la versione unaria (per evitare che il '+' dovuto alla conversione degli operatori complichi l'analisi degli unari):
    i=-1;
    while(true){
      //Con questo while cerco i caratteri degli operatori unari; poi vado all'indietro per vedere cosa c'è per appurare se un '+' o un '-' sono unari o binari
      i=rxBinOper.lastIndexIn(line,i);
      if(i<0)break;
      if(line[i]=='^' || line[i]=='*' || line[i]=='/'){
        i--;
        if(i<0) break;
        continue;
      }
      //se sono arrivato qui in i c'è '+' o '-'0' (possono essere unari)
      //Ora cerco l'ultimo carattere significativo a SX di i: sarà in j
      if(i<1)break;
      j=i-1;
      while(line[j]==' '){
        j--;
        if(j<0) break;
      }
      //se qui ho un dato o una parentesi chiusa, in i ho un binario, altrimenti un unario
      if(rxDatumPtr.indexIn(line,j)==j || line[j]==')'){
          if(line[i]=='+') pOper[i]=3;
          if(line[i]=='-') pOper[i]=4;
          line[i]='+';
      }else{
          if(line[i]=='+') line[i]=' ';
      }
      i=j-1;
      if(i<0)break;
    }
    // ora gli altri operatori:
    for (i=0; i<line.count(); i++){
      switch(line[i]){
        case '^':   pOper[i]=0; break;
        case '*':   pOper[i]=1; break;
        case '/':   pOper[i]=2; break;
        default:    continue;
      }
      line[i]='+';
    }
    prepLine=line;
    return "";
}

QString CLineCalc::sum(float x1, float x2, float & y){
    y=x1+x2;
   return "";
}

QString CLineCalc::subtr(float x1, float x2, float & y){
   y=x1-x2;
   return "";
}

QString CLineCalc::power(float x1, float x2, float & y){
   y=pow(x1,x2);
   return "";
}

QString CLineCalc::prod(float x1, float x2, float & y){
   y=x1*x2;
   return "";
}

QString CLineCalc::div(float x1, float x2, float & y){
   if(x2==0)
     return "division by zero";
   y=x1/x2;
   return "";
}

bool SVarNums::operator== (const SVarNums & x){
    return this->fileNum == x.fileNum && this->varNum == x.varNum;
}

