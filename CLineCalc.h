#ifndef CLINECALC_H
#define CLINECALC_H
#include <QString>
#include <QList>
#include <QRegExp>
#include <QVector>
#include <math.h>
#define MAXBINARYOPS 5 //massimo numero di operatori binari
#define MAXFUNCTIONS 13 //massimo numero di funzioni matematiche

/* Questa classe realizza un calcolatore di linea che interpreta la sintassi ed effettua calcoli.
  La stringa da interpretare può contenere i segni delle quattro operazioni, le parentesi, il segno unario davanti ad un numero (ad inizio stringa o inizio contenuto di coppia di parentesi).
  Ovviamente è mantenuta la priorità standard:
  - prima i contenuti all'interno delle parentesi; all'interno di esse:
    . prima * e / da sinistra a destra
    . poi + e - da sinistra a destra
  La stringa può contenere anche nomi di variabili (iniziano con una lettera). Se vengono incontrate, viene emesso un signal che riceve un puntatore alla cella contenente il valore della variabile.


*****************
 UTILIZZO:
 la funzione effettua una combinazione algebrica e funzionale dei valori di vettori contenenti valori memorizzati in array esterni. Gli array sono le righe della matrice **y_. Sia size il numero di colonne. Per usare la funzione si procede così:
 1) si passa la stringa (QString) da analizzare attraverso "getLine". Se la linea contiene delle variabili va usata la versione overload di getline che consente di passare anche nomi e puntatore ai valori delle variabili
 2) con i che va da 0 a size-1 si fa result[i]=compute(i); (
 3) eventuali messaggi di errore si trovano nella QString, interna ma pubblica, di nome "err"


 *********************
 FUNZIONAMENTO INTERNO
 E' descritto nel file developer.docx
*/

struct SVarNums{
    int fileNum, //contiene il numero del file per la variabile considerata; vale -1 per varaiabili di tipo 'n#'
        varNum; // numero della variabile; è il valore dell'ultimo '#' in f#v# o n#.
    bool operator== (const SVarNums & x);
};


struct SXYNameData{
    bool  isIntegral, //se true la stringa sta richiedendo il calcolo sdi un integrale e quindi vengono tornati in fileNums i dati si un unica variabile, che è quella di cui si fa l'integrale.
    allLegalNames; //true se tutte le variabili contengono un nome legale Se allLegalNames=false il contenuto delle altre variabili della struttura è indeterminato in quanto alla prima variabile non valida l'analisi della stringa di input è interrotta
    QList <int> fileNums; //contiene la lista dei numeri dei files da cui è necessario prelevare le variabili della stringa (in item in lista per ogni file differente)
    QList <SVarNums> varNumsLst; //Numeri delle variabili. Un item in lista per ogni variabile differente
};


class CLineCalc{
  public:
    bool noVariables; //se true la stringa non contiene variabili
    QString ret;
    CLineCalc();
    float compute(int iVal);
    QString getLine(QString line_);
    QString getLine(QString line_, QList<QString> nameList, float ** y_);
    SXYNameData giveXYNameData();
  private:
    bool varListsReceived; //è true se è sono state inviate le liste dei nomi e valori di variabili per la riga corrente
    bool yReceived; //è true se è sono state inviate le liste dei nomi di variabili per la riga corrente e relativa matrice dei valori y
    bool constantsConverted;
    int defaultFileNum;
//    float result;
    QString line0, prepLine, line, stringOperators;
    QVector <int> varNumVect; //contiene l'elenco del numero di variabili per i MAXFILES files; serve per il check sintattico per le funzioni di variabile.
    QRegExp
       rxAlphabet, //l'intero alfabeto, unione delle seguenti due stringhe
       rxBinOper, //operator
       rxDatumFunPtr, //caratteri '#', '@', '&'
       rxDatumPtr, //il carattere '#' o '@'. Entrambi puntatno ad un dato il primo di costante, il secondo di variabile.
       rxLetter, //a letter: a variable name must begin with a letter
       rxLetterDigit, //a letter or a digit: a non-first variable name character must be a letter or a digit
       rxNotDigit, //not a digit
       rxNotLetterDigit, //the first character after a variable name must be such
       rxNotNum, //not a character allowable in a number
       rxNum, //number
       rxNumSepar; //carattere ammissibile fra un numero e il successivo

    float * pConst; //vettore di puntatori alle costanti in line
    float ** pVar; //vettore di puntatori ai primi valori di ogni variabile
    double (*(*pFun))(double x); //vettore di puntatori alle funzioni
    int * pOper; //vettore di puntatori agli operatori binari

    //Nomi delle funzioni unarie:
    QString funStr[MAXFUNCTIONS];
    double (*fun1[MAXFUNCTIONS])(double x1); // Il nome fun1 fa riferimento al fatto che sono funzioni a un argomento (come sin, cos, ecc.)
    QString (*fun2[MAXBINARYOPS])(float x1, float x2,  float  &y); // Il nome fun2 fa riferimento al fatto che sono funzioni a due argomenti (come somma, prodotto, ecc.)
    static QString  sum(float x1, float x2, float & y), subtr(float x1, float x2, float & y),
               prod(float x1, float x2, float & y),   div(float x1, float x2, float & y),
               power(float x1, float x2, float & y);
    QString computeFun1(int start, int end, int iVal);
    QString computeOperator(int start, int end, int startOp, int numOp, int iVal);
    QString computeUnaryMinus(int start, int end, int iVal);
    QString getVarPointers(QList<QString> nameList, float **y_);
    QString substPointersToConsts();
    QString substPointersToOpers();
    QString substPointersToFuns();
    void simplify();
    SVarNums readVarXYNums(QString varStr);

};

#endif // CLINECALC_H
