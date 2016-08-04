#include <QtGui>
#include <QApplication>
#include <QFontMetrics>
#include <QInputDialog>
#include <QMessageBox>
#include <QSet>
#include <QSvgGenerator>
#include <QString>
#include <stdio.h>
#include <time.h>
#include "CLineChart.h"
//#include "matrix.h"
//#define _abs(x) (((x)>0?(x):(x)*(-1)))
#define max(a, b)  (((a) > (b)) ? (a) : (b))
#define min(a, b)  (((a) < (b)) ? (a) : (b))
#define NearInt(x) ((int)(x+0.5))


int max3(int i,  int j, int k){
  int ret;
  ret=(i>j?i:j);
  ret=(ret>k?ret:k);
  return ret;
}

static int pari(int i)  {
    if(i==0)
      return 1;
    else{
      if(i/2*2==i)
        return 1;
      else
        return 0;
    }
  }


CLineChart::CLineChart(QWidget * parent):QLabel(parent,0)
{
/* La realizzazione dell'Image si potrebbe forse fare con il ridimensionamenti sulla base della Pixmap, con il ritracciamento del plot solo in momenti particolari, ad esempio quando si lascia il mouse dopo il ridimensionamento.
  Però si hanno varie difficoltà, fra cui quella che non sono riuscito a far sì che si possa ridimensionare la image anche a diminuire delle dimensioni, oltre che ad aumentare.

Pertanto adotto lo schema già adottato per BCB, con l'esecuzione di plot() durante il ridimensionamento.
A questo punto la Image viene definita qui solo per avere la possibilità di fare un delete sicuro dentro il plot.
La reimplementazione della funzione virtual resizeEvent in questo file contiene la chiamata a plot().
*/
    myTimer = new QTimer(this);
    myTimer->setSingleShot(true);
    connect(myTimer, SIGNAL(timeout()), this, SLOT(resizeStopped()));

/* La sequenza delle righe di questa routine è la seguente:
1) definizione variabile r che serve per il prosieguo
2) attribuzione valori default  variabili indipendenti a complessità crescente e, per il medesimo tipo in ordine alfabetico
3) inizializzazioni finali*/

    // 1) definizione variabile plotRect.
    plotRect=rect();
    /* Qui plotRect ha valori di default con largheza di 100 e altezza di 30.
      Per ragioni non note alla chiamata a resizeEvent ha invece i valori corretti.
      Probabilmente questo è frutto della gestione di Qt dei layout destro e basso che lasciano libero giusto lo spazio che poi è attribuito a CLineChart. Comunque la situazione non è chiarita.
*/


//    2) attribuzione valori default variabili a complessità crescente e, a pari complessità, in ordine alfabetico
    autoLabelXY=true;
    autoMark=false;
    blackWhite=false;
    copying=false;
    dataGot=false;
    dataCursDragging=false;
    dataCursVisible=false;
    dataCurs2Dragging=false;
    dataCurs2Visible=false;
    exactMatch=false;
    forceYZero=false;
    FLegend=true;
    FLinearInterpolation=true;
    labelOverride=false;
    makingSVG=false;
    plotDone=false;
    printing=false;
    twinScale=false;
    useBrackets=true;
    xGrid=false;
    yGrid=false;
    variableStep1=false;
    wIsOmega=false;
    writeTitle1=false;
    zoomed=false;
    zoomSelecting=false;
    // dopo le bool le int, in ordine alfabetico
    legendHeight=0;
    maxAutolineWPoints=2000;
    minXTic=4;
    minYTic=2;
    numOfVSFiles=0;
    swarmPointWidth=4;
    dataCursSelecting=0;
    //infine le variabili più complesse e le funzioni (in ordine alfabetico)
    manuMarks.lastMark=-1;
    baseFontName="Times";  // i font usati sono solo baseFontName e symbFontName!
    symbFontName="Symbol";
    plotType=ptLine;
    xAxis.type=atX;
    xAxis.scaleType=stLin;;
    xVarParam.unitS="";
    xAxis.addZeroLine=false;
    yAxis.type=atYL;
    yAxis.scaleType=stLin;;
    yAxis.addZeroLine=false;
    rYAxis.type=atYR;
    rYAxis.scaleType=stLin;
    rYAxis.addZeroLine=false;

    px=NULL;
    py=NULL;
    titleRectF=QRectF(0,0,0,0);
    setCursor(myCursor);
    titleText=QString("Double-Click here to set the title text!");
    numFont=baseFont=expFont=lgdFont=font();
    baseFontFamily=numFont.family();

    //    3) inizializzazioni finali
    myImage= new QImage(rect().width(),rect().height(),QImage::Format_RGB32);
    myPainter=new QPainter(myImage); //L'associazione standard di myPainter, il painter standard è con myImage;
    //associazioni temporanee vengono fatte con altri paint devices in makeSvg() e print().
    hovVarRect=QRect(0,0,0,0);
    /* Prima di selezionare il font di riferimento devo fare designPlot().
    designPlot() richiede che un painter sia già stato creato, e questo è stato fatto qui sopra.
    Tutte le operazioni di scrittura di testo sul widget, qualunque sia il suo paintDevice, vengono effettuate attraverso myPainter.
    Pertanto non serve attribuire il font definito a CLineChart, bensì a myPainter->CLineChart.
*/
    fontSizeType=fsAuto;
    fixedFontPx=0;
    legendFontSizeType=LAuto;
    drawType=dtMC;
    setPlotPenWidth(pwAuto);
    designPlot();
    //il PointPx viene poi aggiornato al generalpointPx in designPlot().
    //In textOut2 viene inoltre modificato temporaneamente il solo pointPx del font del painter di image->


    //Nelle seguenti righe faccio predisposizioni affinché nella routine Plot si possa prima deallocare (rispettivamente con delete, DeleteIMatrix e DeleteFMatrix) gli array e poi riallocarli con le dimensioni giuste.
    cursorXValues=NULL;
    cursorXValBkp=NULL;
    curveParam=NULL;
    startIndex=NULL;
    stopIndex=NULL;
    pixelToIndexDX=CreateIMatrix(1,1);
    cursorYValues=CreateFMatrix(1,1);
    cursorYValBkp=CreateFMatrix(1,1);
    setMouseTracking(true);
}


bool CLineChart::allowsZero(float MinRoundValue, float MaxRoundValue,int ntic){
/* Funzione che valuta se con i dati passati c'è una tacca in corrispondenza dello 0*/
    int i;
    float ticInterval=(MaxRoundValue-MinRoundValue)/(float)(ntic+1);
    for(i=0; i<ntic; i++){
        if( fabs(MinRoundValue+i*ticInterval)<0.001)return true;
    }
    return false;
}

void CLineChart::beginRounding(SDigits &Min, SDigits &Max, float MinVal, float MaxVal, unsigned Include0)
{
/* Questa funzione serve a compilare Min e Max (escluso RoundValue) a partire dal minimo e massimo valore di una certa scala, passati come Minval e Maxval; contiene la gestione della forzatura dello zero mediante la variabile "Include0".
Essa prende in considerazione le prime 5 cifre significative di MinVal e MaxVal; e' richiesto che questi ultimi differiscano di qualcosa nelle prime quattro cifre  significative, pertanto prima di chiamare BeginRounding gestisco a parte i casi speciali.
*/

    int delta_ie;
   float Ratio;
   char buffermin[12]="           ", buffermax[12]="           ";

   /* Gestione della forzatura della inclusione dello 0 nella scala sulla base
      del contenuto di "Include0". Essa ha senso solo con MinVal e MaxVal dello
      stesso segno */
   if( MinVal*MaxVal>0 ){
     if(MaxVal>0) {
         Ratio=MinVal/MaxVal;     //Ratio e' la percentuale di vuoti
         if( Ratio <= (float)Include0/100.f) MinVal=0.f;
     }else {
       Ratio=MaxVal/MinVal;
       if( Ratio <= (float)Include0/100.f) MaxVal=0.f;
     }
   }
   Min.Value=MinVal;
   Max.Value=MaxVal;

   /* Compilazione interi delle cifre e segno di Min e Max: */
   sprintf(buffermax,"%+10.3e",Max.Value); //scrivo Max e Min nel formato:
   sprintf(buffermin,"%+10.3e",Min.Value); //       s#.###es##   s: segno

   /* Analizzo le due stringhe e compilo le corrispondenti grandezze: */
   Max.Sign=*buffermax;
   sscanf( buffermax+1, "%1u", &Max.i1 );
   sscanf( buffermax+3, "%1u", &Max.i2 );
   sscanf( buffermax+4, "%1u", &Max.i3 );
   sscanf( buffermax+5, "%1u", &Max.i4 );
   sscanf( buffermax+7, "%u", &Max.ie );

   Min.Sign=*buffermin;
   sscanf( buffermin+1, "%1u", &Min.i1 );
   sscanf( buffermin+3, "%1u", &Min.i2 );
   sscanf( buffermin+4, "%1u", &Min.i3 );
   sscanf( buffermin+5, "%1u", &Min.i4 );
   sscanf( buffermin+7, "%u", &Min.ie );

   /* Qualora i due estremi abbiano ordine di grandezza differente,
      le approssimazioni vanno fatte con riferimento al numero di modulo
      maggiore. */
   delta_ie=Max.ie-Min.ie;
   switch(delta_ie) {
     case -3:
        if(Min.i1==0)break;
        Max.ie=Min.ie; Max.i4=Max.i1; Max.i3=0;
        Max.i2=0;      Max.i1=0;      break;
     case -2:
        if(Min.i1==0)break;
        Max.ie=Min.ie; Max.i4=Max.i2; Max.i3=Max.i1;
        Max.i2=0;      Max.i1=0;      break;
     case -1:
        if(Min.i1==0)break;
        Max.ie=Min.ie; Max.i4=Max.i3; Max.i3=Max.i2;
        Max.i2=Max.i1; Max.i1=0;      break;
     case 0:
        break;
     case 1:
        if(Max.i1==0)break;
        Min.ie=Max.ie; Min.i4=Min.i3; Min.i3=Min.i2;
        Min.i2=Min.i1; Min.i1=0;      break;
     case 2:
        if(Max.i1==0)break;
        Min.ie=Max.ie; Min.i4=Min.i2; Min.i3=Min.i1;
        Min.i2=0;      Min.i1=0;      break;
     case 3:
        if(Max.i1==0)break;
        Min.ie=Max.ie; Min.i4=Min.i1;
        Min.i3=0;      Min.i2=0;      Min.i1=0;    break;
     default:
     /* Se gli ordini di differenza sono piu' di tre il numero di modulo minimo viene posto pari a 0 (con effetto Max minore  dell'1%. sul riempimento del grafico) */
        float AbsMin=Min.Value, AbsMax=Max.Value;
        if(AbsMin<0)AbsMin=-AbsMin;
        if(AbsMax<0)AbsMax=-AbsMax;
        if(AbsMin<AbsMax){
           Min.ie=Max.ie;
           /* (istruz.necessaria per avere all'esterno in Max.ie=Min.ie
             sempre l'esponente dell'estremo di massimo modulo */
           Min.i4=0;   Min.i3=0;   Min.i2=0;   Min.i1=0;
        }else{
           Max.ie=Min.ie;                   // come sopra
           Max.i4=0;   Max.i3=0;   Max.i2=0;   Max.i1=0;
        }
    }                                         //fine switch delta_ie
}



//---------------------------------------------------------------------------
int CLineChart::computeDecimals(float scaleMin_, float ticInterval, bool halfTicNum_){
    //Calcolo dei decimali da usare per i numeri sulle tacche degli assi.
    //Scrivo le prime due tacche con 4 decimali ed individuo quanti posso ometterne
    //perché nulli. Per far ciò devo trovare, partendo da destra, il primo carattere
    //diverso da '0' e anche la posizione del carattere '.'
    char num[20];
    int i, ret, temp1, temp2;
    sprintf(num,"%+.4f",scaleMin_+ticInterval*(1+halfTicNum_));
    i=strlen(num);
    do{	 i--; } while(num[i]=='0' && i>0);
    if(num[i]=='.') //Se trovo il puntino prima di altri caratteri non nulli ho 0 decimali
        ret=0;
    else{
        temp1=i; //Ora che ho trovato un carattere diverso da '0' cerco il '.'
        do{	 i--; } while(num[i]!='.' && i>0);
        ret=temp1-i;
        //Il numero di cifre significative non deve comunque superare 5:
        if(temp1>6)ret=min(ret,2);
    }
    sprintf(num,"%+.4f",scaleMin_+2*ticInterval*(1+halfTicNum_));
    i=strlen(num);
    do{	 i--; } while(num[i]=='0' && i>0);
    if(num[i]=='.')
        temp2=0;
    else{
        temp1=i;
        do{	 i--; } while(num[i]!='.' && i>0);
        temp2=temp1-i;
        //Il numero di cifre significative non deve comunque superare 5:
        if(temp1>6)temp2=min(temp2,2);
    }
    ret=max(temp2,ret);
    return ret;
}


int CLineChart::computeTic(float minRoundValue_, float maxRoundValue_, int minTic){
/*   Calcolo del numero di tacche da mettere sopra un asse.
  Se MinTic>=0, il risultato è sempre fra MinTic e MinTic+3.
  Se MinTic<0. per convenzione il risultato è 0.
  Esemplifico la logica operativa nel caso più importante, ovvero con MinTic=4:
-> divido "idelta" per 5, 6, 7, 8 e scrivo i corrispondenti 4 numeri risultanti su una
   stringa (DeltaTicStr) di formato: ##.###. Il minimo numero di decimali ottenibile sarà
   memorizzato in MinDecimals
-> trovo i numeri di tacche che mi danno sulle tacche un numero di decimali pari a
   MinDecimals. Nel caso in cui il campo numerico da considerare contiene lo 0, verifico
   se fra tali numeri ne esistono alcuni che contemplino la presenza di una tacca in
   corrispondenza dello 0. SOLO NEL CASO DI RISPOSTA NEGATIVA procedo come segue:
     - trovo i numeri di tacche che mi danno sulle tacche un numero di decimali pari a
       MinDecimals+1 o MinDecimals+2. Verifico se fra tali numeri ne esistono alcuni che
       contemplino la presenza di una tacca in corrispondenza dello 0. Se questa verifica
       ha esito affermativo il numero di tacche sarà prescelto fra quelli che comportano
       sulle tacche un numero di decimali pari al più a MinDecimals+1 e consentono di avere
       una tacca in corrispondenza dello 0; in caso contrario fra quelli che comportano sulle
       tacche un numero di decimali pari a MinDecimals.
->  A questo punto ho individuato un set di numeri di tacche ammissibili (SelectedSet).
    Fra di essi scelgo secondo la seguente lista di priorità:
    - fra i numeri selezionati scelgo quello che ha l'ultimo digit più "semplice".
      Uso il seguente ordine si semplicit‡ decrescente:
      . digit '0'
      . digit'5'
      . cifre pari
      . altro
  - A parità di semplicità scelgo il numero di tacche più elevato.

Se minTic è diverso da 4, ad es. è 3, il numero di tacche consentito andrà da 3 a 6, e quindi la divisione iniziale si farà per 4, 5, 6, 7 anzicché 5, 6, 7, 8.

***** Nel seguente codice il numero costante 4 sta per numero di possibilità per il numero di tacche. Ad es. per grafici standard potrà avere 4, 5, 6, o 7 tacche; per grafici piccoli, ad es. 2, 3, 4, o 5.****/

  int i, //indice generico
    id1, id2,  idelta,
    j,  //contatore di caratteri
    ret=0, //codice di ritorno
    Decimals[4], //numero di cifre decimali non nulle
    MinDecimals; //numero minimo di cifre decimali non nulle
  QSet <int> Empty, work, hasMinDecimals, hasMinDecimals_, SAllowsZero, selectedSet;

  char **deltaTicStr;
  char buffer[12];
  deltaTicStr=CreateCMatrix(4,8); //8 E' il numero di spazi necessari per scrivere i numeri su stringa
  if(minTic<0){
    ret=0;
    goto Return;
  }
  if(deltaTicStr==NULL){
    QMessageBox::critical(this, tr("TestLineChart"),
      tr("Error Allocating Space in \"computeTic\" within \"CLineChart\""), QMessageBox::Ok);
    ret=minTic;
    goto Return;
  }

  sprintf(buffer,"%+10.3e",maxRoundValue_-minRoundValue_);

// static float fff=-1.0;
// sprintf(buffer,"%+10.3e",fff);

  sscanf(buffer+1, "%1u", &id1);
  sscanf(buffer+3, "%1u", &id2);
  idelta=id1*10+id2;

  MinDecimals=5;
  //Calcolo i numeri di decimali e ne individuo il numero minimo:
  for(i=0; i<4; i ++){
    sprintf(deltaTicStr[i],"%7.4f",(float)idelta/(i+minTic+1));
    j=7;
    if(allowsZero(minRoundValue_,maxRoundValue_,i+minTic))
      SAllowsZero<<i;
    else
      SAllowsZero.remove(i);
    do{j--;} while(deltaTicStr[i][j]=='0' && j>0);
    Decimals[i]=j-2;
    MinDecimals=min(MinDecimals,Decimals[i]);
  }
  //Individuo le intertacche con numero minimo e subminimo di decimali:
  for(i=0; i<4; i++){
    if(Decimals[i]==MinDecimals)
      hasMinDecimals<<i;
    else
      hasMinDecimals.remove(i);
    if(Decimals[i]==MinDecimals+1 || Decimals[i]==MinDecimals+2)
      hasMinDecimals_<<i;
    else
      hasMinDecimals_.remove(i);
  }

  //Ore definisco il SelectedSet che contiene i numeri di tacche che rispettano i criteri
  // prescelti, e fra cui devo poi ritrovare un unico numero da scegliere
  if(minRoundValue_*maxRoundValue_>=0)
    selectedSet=hasMinDecimals;
  else{
    work=hasMinDecimals&SAllowsZero;
    if(work!=Empty)
       selectedSet=work;
    else{
      work=hasMinDecimals_&SAllowsZero;
      if(work!=Empty)
        selectedSet=work;
      else
        selectedSet=hasMinDecimals;
    }
  }
  //Verifico se fra le intertacche selezionate ce n'è qualcuna
  //la cui ultima cifra è lo '0':
  for(i=3; i>=0; i--)
    if(selectedSet.contains(i))
      if(deltaTicStr[i][Decimals[i]+1]=='0'){
        ret=i+minTic;
          goto Return;
      }
    //Verifico se fra le intertacche a numero minimo di decimali ce ne è qualcuna la cui ultima cifra è il '5':
  for(i=3; i>=0; i--)
    if(selectedSet.contains(i))
      if(deltaTicStr[i][Decimals[i]+1]=='5'){
        ret=i+minTic;
        goto Return;
      }
  //Verifico se fra le intertacche a numero minimo di decimali ce ne è qualcuna
  //la cui ultima cifra è pari:
  for(i=3; i>=0; i--)
    if(selectedSet.contains(i)){
      char c=deltaTicStr[i][Decimals[i]+1];
      if(c=='2'||c=='4'||c=='6'||c=='8'){
        ret=i+minTic;
        goto Return;
      }
    }
  //A questo punto ritorno il più grande numero di tacche a minimo numero di decimali
  for(i=3; i>=0; i--)
    if(selectedSet.contains(i)){
      ret=i+minTic;
      goto Return;
    }
    //Riga che serve solo per evitare un warning:
Return:
  DeleteCMatrix(deltaTicStr);
  return ret;
}

//---------------------------------------------------------------------------
void CLineChart::designPlot(void){
 /* Routine per il calcolo delle dimensioni principali del grafico.
Oltre che al momento della costruzione di CLineChart, e ad ogni suo ridimensionamento (comando resizeStopped() andrà utilizzato prima e dopo un eventuale comando di print(), il quale fa riferimento al numero di pixel della stampante, di regola molto maggiore di quello del grafico a schermo.
*/

  int numWidth,maxTic;
  ticWidth.setX(max(4,(unsigned)(0.015f*plotRect.height())));
  ticWidth.setY(ticWidth.x());
  svgOffset= max(6,plotRect.width()/100);

  //Stabilisco i tre font del grafico (baseFont, expFont, legendFont) ed attribuisco il baseFont al myPainter
  generalFontPx=max(min((int)(0.014f*(plotRect.height()+plotRect.width())),24),10);
  int fontPxSize=generalFontPx;
  if(fontSizeType==fsFixed)
    fontPxSize=fixedFontPx;
  //Nelle fasi iniziali del programma fontPxSize è 0 e l'assegnazione di questo valore crea un messaggio d'errore nella finestra di uscita del programma. Il seguente if consente di evitarlo:
 if(fontPxSize>0){
   numFont. setPixelSize(fontPxSize);
   baseFont.setPixelSize(fontPxSize);
   expFont.setPixelSize(EXPFRACTION*fontPxSize);
   lgdFont.setPixelSize(fontPxSize);
  }
  QFontMetrics fm(baseFont);
  smallHSpace=0.6*fm.width("a");
  markHalfWidth=smallHSpace;

  numWidth=fm.width("+0.000");
  textHeight=fm.height();
  maxTic=plotRect.height()/(1.2*textHeight)/2-1;
  minYTic=min(4,maxTic-3);
  maxTic=plotRect.width()/(1.2*numWidth)-1;
  minXTic=min(4,maxTic-3);

}
void CLineChart::drawBars(void){
/* Routine per il tracciamento di diagrammi a barre.  Per il momento consente di tracciare diagrammi relativi ad un'unica grandezza
Pertanto l'indice di file è sempre 0 e l'indice della variabile è sempre 0.
*/
  int i,barLeft,barCenter,
          barY, //coordinata in pixel Y del lato orizzontale del rettangolo rappreentativo dell'ordinata da rappreentare con la barra
          barWidth,barRight,
          yZero; //Coordinata pixel Y della retta y=0 (scale lineari) e logy=0 (scale log)
  float minXStep, fAux;
  QBrush redBrush, grayBrush;
  QString XText;


  plotPen.setColor(Qt::black);
  myPainter->setPen(plotPen);
  redBrush.setColor(Qt::red);
  redBrush.setStyle(Qt::SolidPattern);
  grayBrush.setColor(Qt::gray);
  grayBrush.setStyle(Qt::SolidPattern);
  myPainter->setBrush(redBrush);
  if(yAxis.scaleType==stLin)
    yZero=Y1+yAxis.minF*ratio.y;
  else
    yZero=Y1+yAxis.eMin *ratio.y;

  if(xAxis.scaleType==stLin){
     xStartIndex[0]=X0+margin + NearInt((px[0][startIndex[0]]-xAxis.minF)*ratio.x);
     xStopIndex[0] =X0+margin + NearInt((px[0][stopIndex[0]] -xAxis.minF)*ratio.x);
     minXStep=px[0][stopIndex[0]]-px[0][startIndex[0]];
     for(i=startIndex[0]+1; i<=stopIndex[0]; i++)
       if(px[0][i]-px[0][i-1]<minXStep) minXStep=px[0][i]-px[0][i-1];
     barWidth=max(2,0.7*(X1-X0)*minXStep/(px[0][stopIndex[0]]-px[0][startIndex[0]]));
  }else{
    xStartIndex[0]=X0+margin + NearInt(log10(px[0][startIndex[0]]-xAxis.eMin)*ratio.x);
    xStopIndex[0] =X0+margin + NearInt(log10(px[0][stopIndex[0]] -xAxis.eMin)*ratio.x);
    minXStep=log10(px[0][stopIndex[0]]/px[0][startIndex[0]]);
    for(i=startIndex[0]+1; i<=stopIndex[0]; i++){
      fAux=log10(px[0][i]/px[0][i-1]);
      if(fAux<minXStep) minXStep=fAux;
    }
    barWidth=max(2,0.7*(X1-X0)*minXStep/(log10(px[0][stopIndex[0]]/px[0][startIndex[0]])));
  }
  // Nel caso in cui il numero di barre è molto modesto, la loro larghezza può risultare eccessiva rispetto al font usato per tracciare i numeri sugli assi. Faccio la correzione:
  if (barWidth>1.5*generalFontPx)
      barWidth=1.5*generalFontPx;
// Se le barre sono larghe, la larghezza del cursore va aumentata rispetto al valore default:
  dataCurs.setWidth(max(dataCurs.width(),barWidth/2));

  plotPen.setColor(Qt::black);
  //Tracciamento barre:
  for(i=startIndex[0]; i<=stopIndex[0]; i++){
    myPainter->setBrush(redBrush);
    if(yAxis.scaleType==stLin){
       barY=Y1-(py[0][0][i]-yAxis.minF)*ratio.y;
    }else{
       barY=Y1-log10(py[0][0][i]/yAxis.scaleMin)*ratio.y;
    }
    if(xAxis.scaleType==stLin){
       fAux=px[0][i] - xAxis.minF;
    }else{
      fAux=log10(px[0][i]) - xAxis.eMin;
    }
    if(fAux<0)fAux=0;
    barCenter=X0+margin + NearInt(fAux*ratio.x);
    barLeft=barCenter-barWidth/2.+0.5;
    barLeft=max(X0,barLeft);
    barRight=barLeft+barWidth-1;
    //La seguente condizione non dovrebbe mai accadere. Siccome però accade e per ora non ho il tempo di vedere perché, la gestisco in modo semplice:
    if(barLeft>=X1)continue;
    barRight=min(X1,barRight);
    if(zoomed){
      if(barY>Y1){
        myPainter->setBrush(grayBrush);
        if(yZero<Y0)
          myPainter->drawRect(barLeft,Y0,barRight-barLeft,Y1-Y0+1);
        else if (yZero>Y1)
          ;
        else
          myPainter->drawRect(barLeft,yZero,barRight-barLeft,Y1-yZero+1);
        continue;
      }else if (barY<Y0){
        myPainter->setBrush(grayBrush);
         if(yZero<Y0)
           ;
         else if (yZero>Y1)
           myPainter->drawRect(barLeft,Y0,barRight-barLeft,Y1-Y0+1);
         else
           myPainter->drawRect(barLeft,Y0,barRight-barLeft,yZero-Y0+1);
         continue;
      }
    }
    if(yZero<Y0)
      myPainter->drawRect(barLeft,Y0,barRight-barLeft,barY-Y0);
    else if (yZero>Y1)
      myPainter->drawRect(barLeft,Y1,barRight-barLeft,barY-Y1);
    else
      // E' stato visto che se larghezza o altezza del rettangolo sono negativi, il rettangolo viene sì tracciato, ma l'interno deborda di un pixel, con uno sgradevole effetto di imprecisione. Pertanto nel caso di altezze negative devo manualmente rigirare il rettangolo:
      if(barY-yZero>1){ //se la differenza è 0 o 1 non traccio nulla
        myPainter->drawRect(barLeft,yZero,barRight-barLeft,barY-yZero);
      }else if (barY-yZero<-1){  //se la differenza è 0 o 1 non traccio nulla
        myPainter->drawRect(barLeft,barY,barRight-barLeft,yZero-barY);
    }
  }

  //Piazzo il cursore in corrispondenza della barra più vicina alla metà del grafico:
  /*
  i=(startIndex[0]+stopIndex[0])/2.+0.5;
  static SXYValues values;
  int rWidth=dataRect.width(); //va calcolata volta per volta perché nel caso di ptBar la larghezza è calcolata dinamicamente
  int nearX;
  values=giveValues(px[0][i], FLinearInterpolation, nearX, false, false);
  if(nearX<X0)nearX=X0;
  if(nearX>X1) nearX=X1;
  dataRect.moveLeft(nearX-rWidth/2);
  //Mando fuori i valori:
  emit valuesChanged(values,false,false);
*/
}



void CLineChart::drawCurves(bool noCurves){
 /* Funzione per il tracciamento delle curve su grafici di linea.  Contiene al suo interno un algoritmo per l'eliminazione automatica dei punti visualmente ridondanti e del taglio delle curve all'esterno del rettangolo di visualizzazione.
Essendo stata realizzata con grande cura ed essendo intrinsecamente complessa è sconsigliato modificarla se non strettamente necessario.
*/

  int i, iPlot=0, icount, igraf, iTotPlot=-1;
  int pointsDrawn0;
  float sxmin, symin, xf, yf, x1f, y1f, yRatio;
  float x,y,x1,y1; //valori arrotondati di xf, yf, x1f, y1f
  FC.getRect(X0,Y0,X1,Y1);
  if(xAxis.scaleType==stLin)
    sxmin=xAxis.minF;
  else
    sxmin=xAxis.eMin;
  pointsDrawn=0;
  for(i=0; i<nFiles; i++){
    xStartIndex[i]=NearInt(ratio.x * (px[i][startIndex[i]] - sxmin))+X0;
    xStopIndex[i]=NearInt(ratio.x * (px[i][stopIndex[i]] - sxmin))+X0;
  }
  if(noCurves)return;
  bool wasInRect=false;
  for(i=0; i<nFiles; i++){
    for(igraf=0; igraf<nPlots[i]; igraf++)	{
      iTotPlot++;
      QPainterPath path;
      if(blackWhite)
        plotPen.setColor(Qt::black);
      else
        plotPen.setColor(curveParam[iTotPlot].color);
      if(igraf>7)
          plotPen.setStyle(Qt::DashLine);
      else
          plotPen.setStyle(Qt::SolidLine);
      myPainter->setPen(plotPen);
      // Calcolo yRatio e symin, valutando se sono relativi alla scala di sinistra o
      //a quella eventuale di destra:
      if(curveParam[iTotPlot].rightScale){
        yRatio=ratio.yr;
        if(yAxis.scaleType==stLin)
          symin=rYAxis.minF;
        else
          symin=rYAxis.eMin;
      }else{
        yRatio=ratio.y;
        if(yAxis.scaleType==stLin)
        symin=yAxis.minF;
      else
        symin=yAxis.eMin;
      }
      CFilterClip::FloatPoint P1,P2;
      pointsDrawn0=0;
      if(xAxis.scaleType==stLin)
        x1f=ratio.x * (px[i][startIndex[i]]-sxmin)+X0;
      else
        x1f=ratio.x * (log10(px[i][startIndex[i]])-sxmin)+X0;
      if(yAxis.scaleType==stLin)
        y1f=yAxis.width-yRatio * (py[i][igraf][startIndex[i]]-symin)+Y0;
      else
        y1f=yAxis.width-yRatio * (log10(py[i][igraf][startIndex[i]])-symin)+Y0;
      x1=NearInt(x1f);
      y1=NearInt(y1f);
      if(FC.IsInRect(x1,y1)) wasInRect=true;
      path.moveTo(x1,y1);
      if(xAxis.scaleType==stLin)
        xf=ratio.x * (px[i][startIndex[i]]-sxmin)+X0;
      else
        xf=ratio.x * (log10(px[i][startIndex[i]])-sxmin)+X0;
      if(yAxis.scaleType==stLin)
        yf=yAxis.width-yRatio * (py[i][igraf][startIndex[i]]-symin)+Y0;
      else
        yf=yAxis.width-yRatio * (log10(py[i][igraf][startIndex[i]])-symin)+Y0;
      x=NearInt(xf);
      y=NearInt(yf);
      FC.getLine(x1,y1,x,y);
      //Grafico fino al penultimo punto, con filtraggio e "Clippaggio". L'ultimo punto
      //lo traccio fuori del loop per essere certo che venga comunque tracciato, anche
      //se è nel prolungamento della retta congiungente i due punti precedenti.
      for(icount=startIndex[i]+1; icount<stopIndex[i]; icount++)	{
        if(xAxis.scaleType==stLin)
          xf=ratio.x * (px[i][icount] - sxmin) +X0;
        else
          xf=ratio.x * (log10(px[i][icount]) - sxmin) +X0;
        if(yAxis.scaleType==stLin)
          yf=yAxis.width-yRatio*(py[i][igraf][icount] - symin) +Y0;
        else
          yf=yAxis.width-yRatio*(log10(py[i][igraf][icount]) - symin) +Y0;
        x=xf+0.5;
        y=yf+0.5;
        if(wasInRect){
          if(FC.IsInRect(x,y)){ //Vecchio e nuovo punto dentro il rettangolo
            if(!FC.IsRedundant(x,y)){
              FC.getLine(x1,y1,x,y);
              path.lineTo(x1,y1);
            pointsDrawn0++;
         }
         }else{ //Il vecchio punto era nel rettangolo il nuovo no
           wasInRect=false;
           FC.getLine(x1,y1,x,y);
           path.lineTo(x1,y1);
           if(FC.giveRectIntersect(P1,P2)==1)	path.lineTo(P1.X,P1.Y);
           pointsDrawn0++;
         }
         }else{ //Il punto precedente non era dentro il rettangolo
           if(FC.IsInRect(x,y)){ //Il vecchio punto era fuori, il nuovo dentro il rettangolo
             FC.getLine(x1,y1,x,y);
             wasInRect=true;
             FC.giveRectIntersect(P1,P2);
             path.moveTo(P1.X,P1.Y);
             path.lineTo(x,y);
             pointsDrawn0++;
           }else{ //Vecchio e nuovo punto fuori del rettangolo
             FC.getLine(x1,y1,x,y);
             if(FC.giveRectIntersect(P1,P2)){ //anche se i due punti sono fuori la loro congiungente interseca il rettangolo
               path.moveTo(P1.X,P1.Y);
               path.lineTo(P2.X,P2.Y);
               pointsDrawn0+=2;
            } //Fine ricerca prima intersezione
          } //Fine vecchio e nuovo punto fuori dal rettangolo
        } //Fine punto vecchio fuori dal rettangolo
        x1=x;
        y1=y;
      } //Fine ciclo for tracciamento curve
      //Tracciamento ultimo punto della curva:
      if(xAxis.scaleType==stLin)
        xf=ratio.x * (px[i][stopIndex[i]] - sxmin) +X0;
      else
        xf=ratio.x * (log10(px[i][stopIndex[i]]) - sxmin) +X0;
      if(yAxis.scaleType==stLin)
        yf=yAxis.width-yRatio*(py[i][igraf][stopIndex[i]] - symin) +Y0;
      else
        yf=yAxis.width-yRatio*(log10(py[i][igraf][stopIndex[i]]) - symin) +Y0;
      x=xf+0.5; y=yf+0.5;

      /**********************
/Per ragioni sconosciute talvolta l'ultimo punto non è tracciato, MA SOLO IN RELEASE.
Per questa ragione sono state taggiunte le seguenti righe qdebug() per cercare tdi tracciare l problema.
Ma con qDebug() il problema non si presenta nemmano in relase mode!
Per ora pertanto si lascia il cocdice con queste righe, riducendone al minimo le funzioni, in attesa che prima o poi il vero problema venga evidenziato.
*/
      static int iWasInRect=0;
#include <QDebug>
      //qDebug()<<"X0, X1, Y0, Y1:"<<X0<<X1<<Y0<<Y1;
      //qDebug()<<"x,y,x1,y1:"<<x<<y<<x1<<y1;
      //L'ultimo punto lo traccio solo se il penultimo era nel rettangolo:
      if(wasInRect){
        iWasInRect++;
//        qDebug()<<"wasInRect, i:"<<wasInRect<<iWasInRect;
        if(FC.IsInRect(x,y)){
          path.lineTo(x1,y1);
          path.lineTo(x,y);
        }else{
          FC.getLine(x1,y1,x,y);
          path.lineTo(x1,y1);
          if(FC.giveRectIntersect(P1,P2)==1)
            path.lineTo(P1.X,P1.Y);
          else if(FC.giveRectIntersect(P1,P2)==2)
            path.lineTo(P2.X,P2.Y);
        }
        pointsDrawn0++;
      }
      pointsDrawn=max(pointsDrawn,pointsDrawn0);
      QElapsedTimer timer;
      timer.start();

//      QFile file("pathdata.dat");
//      if(!file.open(QIODevice::WriteOnly))
//          return;
//      QDataStream out(&file);
//      out<<path;
//      file.close();
      if(makingSVG){
          myPainter->drawPath(path);
//          qDebug() << "drawpath operation took" << timer.elapsed() << "milliseconds";
      }else{
// Qui uso la sintassi che mi è stata suggerita da Samuel Rodal, ma è superflua l'iterazione fa i poligoni, visto che le mie curve sono composte tutte da un unico poligono. Notare l'uso di foreach(), estensione di Qt al C++ (significato accessibile via help).
          foreach(QPolygonF poly, path.toSubpathPolygons())
              for(int i=0; i<poly.size()-1; i++)
                  myPainter->drawLine(poly.at(i),poly.at(i+1));
//          qDebug() << "foreach operation took" << timer.elapsed() << "milliseconds";
      }
      iPlot++;
    } //Fine tracciamento varie curve relative ad un medesimo file
  } //Fine ciclo for fra i vari files
//  Return:
}

void CLineChart::drawCurvesPoly(bool NoCurves){
/*Funzione per il traccamento di curve su grafici di linea.
A differenza delle altre versioni di "drawCurves" fa uso di QPloigon invece di QPath.
QUesto in via sperimentale in quanto si è visto che tutte le versioni di questa funzione che fanno uso di drawPath hanno un problema: in taluni casi non tracciano uno dei segmenti di curva previsti. Il fatto è ben visibile con la curva vDcloadDcmeno del file "rad2.mat"
Naturalmente rinunciare a drawpath ha i seguenti inconvenienti:
1- quando si esporta il grafico come SVG il grafico non è oggetto unico; invece, ognuno dei segmenti costituenti lo è
2- ci si aspetta una minore efficenza, ma questo è da verificare.
La presente funzioen serve qunidi a verificare i cambiamenti di efficienza che si hanno se si elimina il drawPath; se essi sono trascurabili e scompare l'errore di visualizzazione sopra citato, si potrebbe usare all'uso di drawPath almeno nella visualizzazione a schermo, magari lascandolo invece attivo nella scrittura su svg.
*/

    int i, iPlot=0, icount, igraf, iTotPlot=-1;
    int pointsDrawn0;
    float sxmin, symin, xf, yf, x1f, y1f, yRatio;
    QPolygon poly;
    if(xAxis.scaleType==stLin)
      sxmin=xAxis.minF;
    else
      sxmin=xAxis.eMin;
    pointsDrawn=0;
    for(i=0; i<nFiles; i++){
      xStartIndex[i]=NearInt(ratio.x * (px[i][startIndex[i]] - sxmin))+X0;
      xStopIndex[i]=NearInt(ratio.x * (px[i][stopIndex[i]] - sxmin))+X0;
    }
    if(NoCurves)return;
    for(i=0; i<nFiles; i++){
      for(igraf=0; igraf<nPlots[i]; igraf++)	{
        if(blackWhite)
          plotPen.setColor(Qt::black);
        else
          plotPen.setColor(curveParam[igraf].color);
        myPainter->setPen(plotPen);
        iTotPlot++;
        // Calcolo YRatio e symin, valutando se sono relativi alla scala di sinistra o
        //a quella eventuale di destra:
        if(curveParam[iTotPlot].rightScale){
          yRatio=ratio.yr;
          if(yAxis.scaleType==stLin)
            symin=rYAxis.minF;
          else
            symin=rYAxis.eMin;
        }else{
          yRatio=ratio.y;
          if(yAxis.scaleType==stLin)
          symin=yAxis.minF;
        else
          symin=yAxis.eMin;
        }
        pointsDrawn0=0;
        if(xAxis.scaleType==stLin)
          x1f=ratio.x * (px[i][startIndex[i]]-sxmin)+X0;
        else
          x1f=ratio.x * (log10(px[i][startIndex[i]])-sxmin)+X0;
        if(yAxis.scaleType==stLin)
          y1f=yAxis.width-yRatio * (py[i][igraf][startIndex[i]]-symin)+Y0;
        else
          y1f=yAxis.width-yRatio * (log10(py[i][igraf][startIndex[i]])-symin)+Y0;
        poly << QPoint((int)x1f,(int)y1f);
        //Tracciamento grafico
        for(icount=startIndex[i]+1; icount<=stopIndex[i]; icount++)	{
          if(xAxis.scaleType==stLin)
            xf=ratio.x * (px[i][icount] - sxmin) +X0;
          else
            xf=ratio.x * (log10(px[i][icount]) - sxmin) +X0;
          if(yAxis.scaleType==stLin)
            yf=yAxis.width-yRatio*(py[i][igraf][icount] - symin) +Y0;
          else
            yf=yAxis.width-yRatio*(log10(py[i][igraf][icount]) - symin) +Y0;
          poly << QPoint(xf,yf);
          pointsDrawn0++;
        } //Fine ciclo for tracciamento curve
        pointsDrawn=max(pointsDrawn,pointsDrawn0);
        iPlot++;
      } //Fine tracciamento varie curve relative ad un medesimo file
    } //Fine ciclo for fra i vari files
  //  Return:
    myPainter->drawPolyline(poly);


}

void CLineChart::drawCurvesQtF(bool NoCurves){
/* Funzione per il tracciamento delle curve su grafici di linea.
Nelle versioni QtI e QtF è stato soppresso il fltraggio in quanto si presume che per velocizzare sia sufficiente la separazione fra preparazione del path e successiva generazione della Pixmap a partire da esso.
I test effettuati hanno in effetti mostrato che con QtF si hanno tempi eccessivi, mentre con QtI  i tempi sono paragonabili a quelli ottenibili FC (cioè la mia versione di DrawCurves che fa uso di FIlterClip) mentre con il BCB senza filterClip i tempi erano enormemente superiori.
   Ecco un resoconto in cui si sono mediati i risultati otteniuti in tutti i casi con più prove consecutive effettuate sul medesimo PC:
   Punti      100 000    100 000
   Linea       thin       thick
   FC/ms         8          8
   QtF/ms       15         95
   QtI/ms        8          9
Occorre ricordare che  al momento non è chiaro come si comportano le due funzioni quando viene effettuata l'operazione di Clip.
Si osserva che Qti opera SENZA simplified(), in quanto se metto simplified() i tempi si allungano e il grafico viene richiuso da una linea che invece non dovrebbe esserci.
L'unica differe za fra QtF e QtI sta nella linea "lineTo", la quint'ultima di codice, che nel caso di QtF traccia fra valori float, mentre QtI traccia fra valori preventivamente convertiti da float a int.
   */

  int i, iPlot=0, icount, igraf, iTotPlot=-1;
  int pointsDrawn0;
  float sxmin, symin, xf, yf, x1f, y1f, yRatio;
  QPainterPath path;
  if(xAxis.scaleType==stLin)
    sxmin=xAxis.minF;
  else
    sxmin=xAxis.eMin;
  pointsDrawn=0;
  for(i=0; i<nFiles; i++){
    xStartIndex[i]=NearInt(ratio.x * (px[i][startIndex[i]] - sxmin))+X0;
    xStopIndex[i]=NearInt(ratio.x * (px[i][stopIndex[i]] - sxmin))+X0;
  }
  if(NoCurves)return;
  for(i=0; i<nFiles; i++){
    for(igraf=0; igraf<nPlots[i]; igraf++)	{
      if(blackWhite)
        plotPen.setColor(Qt::black);
      else
        plotPen.setColor(curveParam[igraf].color);
      myPainter->setPen(plotPen);
      iTotPlot++;
      // Calcolo YRatio e symin, valutando se sono relativi alla scala di sinistra o
      //a quella eventuale di destra:
      if(curveParam[iTotPlot].rightScale){
        yRatio=ratio.yr;
        if(yAxis.scaleType==stLin)
          symin=rYAxis.minF;
        else
          symin=rYAxis.eMin;
      }else{
        yRatio=ratio.y;
        if(yAxis.scaleType==stLin)
        symin=yAxis.minF;
      else
        symin=yAxis.eMin;
      }
      pointsDrawn0=0;
      if(xAxis.scaleType==stLin)
        x1f=ratio.x * (px[i][startIndex[i]]-sxmin)+X0;
      else
        x1f=ratio.x * (log10(px[i][startIndex[i]])-sxmin)+X0;
      if(yAxis.scaleType==stLin)
        y1f=yAxis.width-yRatio * (py[i][igraf][startIndex[i]]-symin)+Y0;
      else
        y1f=yAxis.width-yRatio * (log10(py[i][igraf][startIndex[i]])-symin)+Y0;
      path.moveTo(x1f,y1f);
      //Tracciamento grafico
      for(icount=startIndex[i]+1; icount<=stopIndex[i]; icount++)	{
        if(xAxis.scaleType==stLin)
          xf=ratio.x * (px[i][icount] - sxmin) +X0;
        else
          xf=ratio.x * (log10(px[i][icount]) - sxmin) +X0;
        if(yAxis.scaleType==stLin)
          yf=yAxis.width-yRatio*(py[i][igraf][icount] - symin) +Y0;
        else
          yf=yAxis.width-yRatio*(log10(py[i][igraf][icount]) - symin) +Y0;
        path.lineTo(xf,yf);
        pointsDrawn0++;
      } //Fine ciclo for tracciamento curve
      pointsDrawn=max(pointsDrawn,pointsDrawn0);
      iPlot++;
    } //Fine tracciamento varie curve relative ad un medesimo file
  } //Fine ciclo for fra i vari files
//  Return:
  myPainter->drawPath(path);
}

void CLineChart::drawCurvesQtI(bool NoCurves){
 /* Funzione per il tracciamento delle curve su grafici di linea.
Per la spiegazione vedere il commento alla funzione drawCurvesQtF.
*/

  int i, iPlot=0, icount, igraf, iTotPlot=-1;
  int PointsDrawn0;
  float sxmin, symin, xf, yf, x1f, y1f, YRatio;
  QPainterPath path;
  if(xAxis.scaleType==stLin)
    sxmin=xAxis.minF;
  else
    sxmin=xAxis.eMin;
  pointsDrawn=0;
  for(i=0; i<nFiles; i++){
    xStartIndex[i]=NearInt(ratio.x * (px[i][startIndex[i]] - sxmin))+X0;
    xStopIndex[i]=NearInt(ratio.x * (px[i][stopIndex[i]] - sxmin))+X0;
  }
  if(NoCurves)return;
  for(i=0; i<nFiles; i++){
    for(igraf=0; igraf<nPlots[i]; igraf++)	{
      if(blackWhite)
        plotPen.setColor(Qt::black);
      else
        plotPen.setColor(curveParam[igraf].color);
      myPainter->setPen(plotPen);
      iTotPlot++;
      // Calcolo YRatio e symin, valutando se sono relativi alla scala di sinistra o
      //a quella eventuale di destra:
      if(curveParam[iTotPlot].rightScale){
        YRatio=ratio.yr;
        if(yAxis.scaleType==stLin)
          symin=rYAxis.minF;
        else
          symin=rYAxis.eMin;
      }else{
        YRatio=ratio.y;
        if(yAxis.scaleType==stLin)
        symin=yAxis.minF;
      else
        symin=yAxis.eMin;
      }
      PointsDrawn0=0;
      if(xAxis.scaleType==stLin)
        x1f=ratio.x * (px[i][startIndex[i]]-sxmin)+X0;
      else
        x1f=ratio.x * (log10(px[i][startIndex[i]])-sxmin)+X0;
      if(yAxis.scaleType==stLin)
        y1f=yAxis.width-YRatio * (py[i][igraf][startIndex[i]]-symin)+Y0;
      else
        y1f=yAxis.width-YRatio * (log10(py[i][igraf][startIndex[i]])-symin)+Y0;
      path.moveTo(x1f,y1f);
      //Tracciamento grafico
      for(icount=startIndex[i]+1; icount<=stopIndex[i]; icount++)	{
        if(xAxis.scaleType==stLin)
          xf=ratio.x * (px[i][icount] - sxmin) +X0;
        else
          xf=ratio.x * (log10(px[i][icount]) - sxmin) +X0;
        if(yAxis.scaleType==stLin)
          yf=yAxis.width-YRatio*(py[i][igraf][icount] - symin) +Y0;
        else
          yf=yAxis.width-YRatio*(log10(py[i][igraf][icount]) - symin) +Y0;
        path.lineTo(xf,yf);
        PointsDrawn0++;
      } //Fine ciclo for tracciamento curve
      pointsDrawn=max(pointsDrawn,PointsDrawn0);
      iPlot++;
    } //Fine tracciamento varie curve relative ad un medesimo file
  } //Fine ciclo for fra i vari files
//  Return:
  myPainter->drawPath(path);
}

void  CLineChart::drawMark(float X, float Y, int mark, bool markName){
  /* Funzione per il tracciamento dei vari simboli identificativi delle varie curve.
   X e Y sono le coordinate del centro; larghezza e altezza sono "markWidth/2".
    Se markName è true sto marchiando i nomi delle variabili e non devo fare il check per vedere se il punto passato è dentro il rettangolo del grafico o meno
  */
  QPainterPath path;
  QPolygon polygon;
  myPainter->setPen(Qt::black);
  if(!markName && (X<X0 || X>X1 || Y<Y0 || Y>Y1))return;
  switch(mark){
    case 0:
      //Cerchietto:
      myPainter->drawEllipse(X-markHalfWidth,Y-markHalfWidth,2*markHalfWidth,2*markHalfWidth);
      break;
    case 1:
      //Quadratino:
      myPainter->drawRect(X-markHalfWidth,Y-markHalfWidth,2*markHalfWidth,2*markHalfWidth);
      break;
    case 2:
      //Triangolino:
      polygon<<QPoint(X-markHalfWidth,Y+markHalfWidth);
      polygon<<QPoint(X+markHalfWidth,Y+markHalfWidth);
      polygon<<QPoint(X          ,Y-markHalfWidth);
      myPainter->drawPolygon(polygon);
      break;
    case 3:
       //Crocetta
       path.moveTo(X-markHalfWidth,Y-markHalfWidth);
       path.lineTo(X+markHalfWidth,Y+markHalfWidth);
       path.moveTo(X-markHalfWidth,Y+markHalfWidth);
       path.lineTo(X+markHalfWidth,Y-markHalfWidth);
       myPainter->drawPath(path);
       break;
    case 4:
       //Cerchietto pieno:
        myPainter->setBrush(Qt::black);
        myPainter->drawEllipse(X-markHalfWidth,Y-markHalfWidth,2*markHalfWidth,2*markHalfWidth);
        break;
     case 5:
        //Quadratino pieno:
        myPainter->setBrush(Qt::black);
        myPainter->drawRect(X-markHalfWidth,Y-markHalfWidth,markHalfWidth,markHalfWidth);
        break;
      case 6:
        //Triangolino pieno:
        myPainter->setBrush(Qt::black);
        polygon<<QPoint(X-markHalfWidth,Y+markHalfWidth);
        polygon<<QPoint(X+markHalfWidth,Y+markHalfWidth);
        polygon<<QPoint(X          ,Y-markHalfWidth);
        myPainter->drawPolygon(polygon);
        break;
      case 7:
        //Crocetta dentro quadratino
        path.moveTo(X-markHalfWidth,Y-markHalfWidth);
        path.lineTo(X+markHalfWidth,Y+markHalfWidth);
        path.moveTo(X-markHalfWidth,Y+markHalfWidth);
        path.lineTo(X+markHalfWidth,Y-markHalfWidth);
        path.addRect(QRectF(X-markHalfWidth,Y-markHalfWidth,2*markHalfWidth,2*markHalfWidth));
        myPainter->drawPath(path);
        break;
    }
  myPainter->setBrush(Qt::white);
}


void CLineChart::drawSwarm(void){
/* Funzione per il tracciamento di uno sciame di punti, non collegati da alcuna linea. I punti possono essere minimi (pari ad un pixel) o medi (pari ad un quadratino 3x3  il cui centro è la coordinata del punto desiderato).
 */
  int i, iPlot=0, icount, igraf, iTotPlot=-1;
  int pointsDrawn0, ptRadius=swarmPointWidth/2;
  float sxmin, symin, xf, yf, x1f, y1f, yRatio;
  float x,y,x1,y1; //valori arrotondati di xf, yf, x1f, y1f
//  CursorShape->Width=1;
  ticPen.setWidth(1);
  plotPen.setWidth(1);
  framePen.setWidth(1);

  FC.getRect(X0,Y0,X1,Y1);

  if(xAxis.scaleType==stLin)
    sxmin=xAxis.minF;
  else
    sxmin=xAxis.eMin;
  pointsDrawn=0;
  for(i=0; i<nFiles; i++){
    xStartIndex[i]=NearInt(ratio.x * (px[i][startIndex[i]] - sxmin))+X0;
    xStopIndex[i]=NearInt(ratio.x * (px[i][stopIndex[i]] - sxmin))+X0;
  }
  for(i=0; i<nFiles; i++){
    for(igraf=0; igraf<nPlots[i]; igraf++) {
      iTotPlot++;
      if(blackWhite)
        plotPen.setColor(Qt::black);
      else
        plotPen.setColor(curveParam[iTotPlot].color);
      myPainter->setPen(plotPen);
// Calcolo YRatio e symin, valutando se sono relativi alla scala di sinistra o a quella eventuale di destra:
      if(curveParam[iTotPlot].rightScale){
        yRatio=ratio.yr;
        if(yAxis.scaleType==stLin)
          symin=rYAxis.minF;
        else
          symin=rYAxis.eMin;
      }else{
         yRatio=ratio.y;
         if(yAxis.scaleType==stLin)
           symin=yAxis.minF;
         else
           symin=yAxis.eMin;
      }
      pointsDrawn0=0;
      if(xAxis.scaleType==stLin)
        x1f=ratio.x * (px[i][startIndex[i]]-sxmin)+X0;
      else
        x1f=ratio.x * (log10(px[i][startIndex[i]])-sxmin)+X0;
      if(yAxis.scaleType==stLin)
        y1f=yAxis.width-yRatio * (py[i][igraf][startIndex[i]]-symin)+Y0;
      else
        y1f=yAxis.width-yRatio * (log10(py[i][igraf][startIndex[i]])-symin)+Y0;
      x1=NearInt(x1f);
      y1=NearInt(y1f);
      if(xAxis.scaleType==stLin)
        xf=ratio.x * (px[i][startIndex[i]]-sxmin)+X0;
      else
        xf=ratio.x * (log10(px[i][startIndex[i]])-sxmin)+X0;
      if(yAxis.scaleType==stLin)
        yf=yAxis.width-yRatio * (py[i][igraf][startIndex[i]]-symin)+Y0;
      else
        yf=yAxis.width-yRatio * (log10(py[i][igraf][startIndex[i]])-symin)+Y0;
      x=NearInt(xf);
      y=NearInt(yf);
      FC.getLine(x1,y1,x,y);
//Grafico fino al penultimo punto, con filtraggio e "Clippaggio". Il primo e l'ultimo punto li traccio fuori del loop: il primo altrimenti non verrebbe tracciato, l'ultimo per essere certo che venga comunque tracciato, anche se è nel prolungamento della retta congiungente i due punti precedenti.
      if(xAxis.scaleType==stLin)
        xf=ratio.x * (px[i][startIndex[i]] - sxmin) +X0;
      else
        xf=ratio.x * (log10(px[i][startIndex[i]]) - sxmin) +X0;
      if(yAxis.scaleType==stLin)
        yf=yAxis.width-yRatio*(py[i][igraf][startIndex[i]] - symin) +Y0;
       else
        yf=yAxis.width-yRatio*(log10(py[i][igraf][startIndex[i]]) - symin) +Y0;
      x=xf+0.5; y=yf+0.5;
      myPainter->drawRect(x-ptRadius,y-ptRadius,swarmPointWidth,swarmPointWidth);
      pointsDrawn0++;
      for(icount=startIndex[i]+1; icount<stopIndex[i]; icount++)	{
        if(xAxis.scaleType==stLin)
          xf=ratio.x * (px[i][icount] - sxmin) +X0;
        else
          xf=ratio.x * (log10(px[i][icount]) - sxmin) +X0;
        if(yAxis.scaleType==stLin)
          yf=yAxis.width-yRatio*(py[i][igraf][icount] - symin) +Y0;
        else
          yf=yAxis.width-yRatio*(log10(py[i][igraf][icount]) - symin) +Y0;
        x=xf+0.5;
        y=yf+0.5;
        if(FC.IsInRect(x,y)){ //Traccio il punto
          if(swarmPointSize==ssPixel)
            myPainter->drawPoint(QPointF(xf,yf));
          else
            myPainter->drawRect(x-ptRadius,y-ptRadius,swarmPointWidth,swarmPointWidth);
          pointsDrawn0++;
        }
      } //Fine ciclo for tracciamento curve
      //Tracciamento ultimo punto della curva:
      if(xAxis.scaleType==stLin)
        xf=ratio.x * (px[i][stopIndex[i]] - sxmin) +X0;
      else
        xf=ratio.x * (log10(px[i][stopIndex[i]]) - sxmin) +X0;
      if(yAxis.scaleType==stLin)
        yf=yAxis.width-yRatio*(py[i][igraf][stopIndex[i]] - symin) +Y0;
      else
        yf=yAxis.width-yRatio*(log10(py[i][igraf][stopIndex[i]]) - symin) +Y0;
      x=xf+0.5; y=yf+0.5;
      //Traccio l'ultimo punto :
      if(FC.IsInRect(x,y)){ //Traccio il punto
        if(swarmPointSize==ssPixel)
          myPainter->drawPoint(QPointF(xf,yf));
        else
          myPainter->drawRect(x-ptRadius,y-ptRadius,swarmPointWidth,swarmPointWidth);
        pointsDrawn0++;
      }
      pointsDrawn=max(pointsDrawn,pointsDrawn0);
      if(swarmPointSize!=ssPixel)
      iPlot++;
    } //Fine tracciamento varie curve relative ad un medesimo file
  } //Fine ciclo for fra ivari files
}


int CLineChart::drawText2(int X, int Y, EadjustType hAdjust, EadjustType vAdjust, QString msg1, QString msg2, bool _virtual, bool addBrackets){
/* Function che serve per mettere su una canvas il testo msg1, seguito dal testo  msg2, come esponente (più piccolo e allineato in alto). Gestisce gli allineamenti (orizzontale e verticale) specificati, ritorna la larghezza in pixel della stringa completa.
Notare che:
- se  _virtual=true, la stringa non è scritta, e questa funzione ha il solo scopo del calcolo della larghezza (come valore di ritorno) MA AL MOMENTO (SETT 2015) NON E' MAI RICHIAMATA CON L'ULTIMO PARAMETRO TRUE!.
-  se addBracket=true a sinistra si ms1 bviene aggmesso '(' a destra di msg2 ì)'
-  se CLineChart->OIsOmega==true  la lettera "O" viene scritta con il carattere greco Omega maiuscolo.
Note Qt:
- All'ingresso in questa funzione "painter" ha il valore default del widget CLineChart. Quindi painter>fontMetrics() coincide con fontMetrics() e con this->fontMetrics().
- Il font di painter è temporaneamente modificato sia per fare l'esponente che per l'eventuale omega in stile symbol, e poi rimesso al valore del widget in uscita.

VALORE DI RITORNO: la lunghezza in pixel della stringa composita, considerando sia msg1 che msg2 che le eventuali parentesi.
*/

  int len=0; //Lunghezza senza hOffset;
  int ret; //valore di ritorno: lunghezza compreso hOffset
  int wPosition, xPosition=X,
          width1, width2, wBracket, //larghezza testo msg1, msg2 e di una parentesi
          H1, H2; //Altezza testo msg1 e msg2.
  int hOffset, vOffset; //un certo numero di pixel da lasciare fra il testo e le coordinate passate. Non li metto da fuori perché se li gestisco qui dentro li posso esprimere in funzione del fontSize. Questo è particolarmente importante in quanto sulla stampante ho un effetto completamente differente, a parità di pixel, che sullo schermo per via della grande differenza sulla dimensione del pixel stesso.

/* La Y passata a questa routine è il pixel superiore in cui scrivere.
In Qt la posizione verticale da passare per il tracciamento non è il margine superiore del testo come in BCB ma il margine inferiore
Questo ha comportato nel seguente if la sostituzione di "atLeft" con "atRight" e lo 0.55 con 0.45 (poi trasformato in 0.35)  ***/
// int aaa=smallHSpace;
  myPainter->setFont(baseFont);
  width1=myPainter->fontMetrics().width(msg1);
  H1=myPainter->fontMetrics().height();
  myPainter->setFont(expFont);
  width2=myPainter->fontMetrics().width(msg2);
  H2=myPainter->fontMetrics().height();
  if(addBrackets)
    wBracket=myPainter->fontMetrics().width("(");
  else
    wBracket=0;

//  qDebug()<<"msg1: "+msg1<<"msg2: "<<msg2<<"width1 : "<<width1<<"width2 : "<<width2;

  if(msg1=="")return 0;
  if(vAdjust==atRight)
    vOffset=0.1*H1;
  else if(vAdjust==atCenter)
    vOffset=0.35*H1;
  else{ //atLeft
    vOffset=1.0*H1;
}
//Determinazione del pixel più a sinistra del testo da scrivere XPosition ritoccando il valore di partenza pari all'"X" ricevuto dalla funzione
  myPainter->setFont(baseFont);
  if(hAdjust==atLeft){
    hOffset=2*smallHSpace;
    xPosition+=hOffset;
  }else if(hAdjust==atCenter){
      hOffset=0;
      xPosition-=(width1+width2+2*wBracket*(int)addBrackets)/2 +hOffset;
   }else{ //hAdjust=atRight
      hOffset=1.5*smallHSpace;
      xPosition-= width1+width2+2*wBracket*(int)addBrackets+hOffset;
  }

  //Scrittura del testo msg1 contemplando l'eventuale carattere Omega
  wPosition=msg1.indexOf('W');

  //Scrittura parentesi sinistra:
  if(!_virtual && addBrackets && msg1!="")
    myPainter->drawText(xPosition,Y+vOffset,"(");
  if(addBrackets && msg1!="")
      len+=wBracket;

  if(!wIsOmega||wPosition==-1){
    // *** Scrittura di msg1 quando non ci sono Omega: ***
    if(!_virtual)
        myPainter->drawText(xPosition+len,Y+vOffset,msg1);
    len+=width1;
  }else{
    // *** Scrittura di msg1 quando è presente un "W" che va interpretato come Omega: ***
    QString part1=msg1, part2=msg1;
    //preservo la parte del testo rispettivamente a sinistra e destra di "W" per poi fare la ricomposizione dopo che "W" è scritto come simbolo:
    part1.truncate(wPosition);
    part2.remove(0,wPosition+1);
    myPainter->drawText(xPosition+len,Y+vOffset,part1);
    len+=myPainter->fontMetrics().width(part1);
    baseFont.setFamily(symbFontName);
    myPainter->setFont(baseFont);
    if(!_virtual)
        myPainter->drawText(xPosition+len,Y+vOffset,"W");
    len+=myPainter->fontMetrics().width("W");
    baseFont.setFamily(baseFontFamily);
    myPainter->setFont(baseFont);
    if(!_virtual)
      myPainter->drawText(xPosition+len,Y+vOffset, part2);
    len+=myPainter->fontMetrics().width(part2);
  }
  //Scrittura del testo msg2 :
  myPainter->setFont(expFont);
  if(!_virtual && width2>0)
      myPainter->drawText(xPosition+len,Y+vOffset-0.4*H2,msg2);
  len+=width2;
  myPainter->setFont(baseFont);
  //Scrittura parentesi destra:
  if(!_virtual && addBrackets && msg1!="")
    myPainter->drawText(xPosition+len,Y+vOffset,")");
  if(addBrackets && msg1!="")
      len+=wBracket;
  //STRANAMENTE la seguente aggiunta di abs(5*hOffset) non ha nessun effetto!!
  return ret=len+abs(hOffset);
}

void CLineChart::disableTitle(){
  writeTitle1=false;
  myPainter->drawRect(0,0,geometry().width(),titleHeight);
  resizeStopped();
}

void CLineChart::enableTitle(){
  writeTitle1=true;
  resizeStopped();
}

bool  CLineChart::fillPixelToIndex(int **pixelToIndexDX){
  //Funzione che, nel caso di file a passo variabile d‡ gli indici corrispondenti ai
  //vari pixel. Ritorna true se il lavoro è stato eseguito, false se ci sono stati errori.
  //Per ogni pixel dà l'indice del punto più vicino il cui valore sull'asse x SUPERA il
  //valore corrispondente al pixel considerato.
  if(numOfVSFiles==0)return false;
  int VSFile=0, iFile, iPixel, index;
  float t, tStart, tStep;

  tStart=xAxis.scaleMin * xAxis.scaleFactor;
  tStep=(xAxis.scaleMax*xAxis.scaleFactor - tStart)/(X1-X0);
  for(iFile=0; iFile<nFiles; iFile++)
    if(filesInfo[iFile].variableStep){
      // t=tStart;
      //Il punto corrispondente a X0 è ovviamente il primo punto:
      index=startIndex[iFile];
      pixelToIndexDX[VSFile][0]=index;
      for(iPixel=1; iPixel<=X1-X0; iPixel++){
        t=tStart+iPixel*tStep;
        while (px[iFile][index]<t && index<filesInfo[iFile].numOfPoints-1 )
            index++;
        pixelToIndexDX[VSFile][iPixel]=index;
      }
      VSFile++;
    }
  return true;
}


bool  CLineChart::fillPixelToIndexLog(int **PixelToIndexDX){
  //Versione di FilPixelToIndex per scale stLog e stDB.
  int iPix, Index, iFile;
  float Val0;
  for(iFile=0; iFile<nFiles; iFile++){
    Index=startIndex[iFile];
    PixelToIndexDX[iFile][0]=Index;
    Val0=log10(px[iFile][Index]);
    for(iPix=1; iPix<=X1-X0; iPix++){
      while (log10(px[iFile][Index])<iPix/ratio.x+Val0 && Index<filesInfo[iFile].numOfPoints-1) Index++;
        PixelToIndexDX[iFile][iPix]=Index;
    }
  }
 return true;
}


struct CLineChart::SMinMax CLineChart::findMinMax(float *vect, unsigned dimens)
{
   unsigned icount;
   struct SMinMax vmM;

   vmM.Min=*vect;
   vmM.Max=*vect;

//   float v[5];
//   for (unsigned i=1; i<min(5,dimens); i++) v[i] =*(vect+i);

   for (icount=1; icount<dimens; icount++)
   {
      vmM.Min=min(vmM.Min, *(vect+icount));
      vmM.Max=max(vmM.Max, *(vect+icount));
   }
   return(vmM);
}

void CLineChart::getAllLabels(SAllLabels all){
  xUserLabel=all.xLbl;
  yUserLabel=all.yLbl;
  ryUserLabel=all.ryLbl;
}

void CLineChart::getData(float *px_,float **py_, int nPoints_, int nPlots_){
   /*funzione  per l'utilizzo elementare di lineChart: associabile naturalmente a plot() (senza parametri passati).*/
   int i;
   SFileInfo FI;
   delete[] curveParam;
   curveParam=new SCurveParam[nPlots_];
   //Attribuisco valori default standard alle variabili necessarie al getData per singole variabili asse x,
   FI.name="fileName";
   FI.numOfPoints=nPoints_;
   FI.variableStep=false;
   xVarParam.name="x";
   xVarParam.isMonotonic=true;
   xVarParam.isVariableStep=false;
   for(i=0; i<nPlots_; i++){
     if(i==0)curveParam[i].color=Qt::red;
     if(i==1)curveParam[i].color=Qt::green;
     if(i==2)curveParam[i].color=Qt::blue;
     if(i==3)curveParam[i].color=Qt::gray;
     if(i>3)curveParam[i].color=Qt::black;
     curveParam[i].name="var";
     curveParam[i].rightScale=false;
     curveParam[i].unitS="";
    }

   getData(FI,  nPlots_, xVarParam, curveParam, px_, py_);

}
void CLineChart::getData(SFileInfo FI, int nPlots_,  SXVarParam xVarParam_, SCurveParam *curveParam_,float *px_,float **py_){
    /*getData per il caso di singole variabili asse x*/
    filesInfo.clear();
    filesInfo.append(FI);

    //L'uso delle seguenti variabili con "1" in fondo evita di dover fare delle chiamate a new per un unico elemento dei vettori, però non è una tecnica sicura. Funziona con molti compilatori, ma con il MingW (e Qt 5.0.2) dà segmetation fault. La abbandonerò progressivamente.
    //Per ora ho eliminato variableStep, avendola sostituita con filesInfo[iFile].variableStep.
    variableStep1=FI.variableStep;    //variableStep=&variableStep1;
    nPlots1=nPlots_;      nPlots=&nPlots1;

    xVarParam=xVarParam_;
    curveParam=curveParam_;

    nFiles=1;
    delete[] px;
    delete[] py;
    px= new float*[1];
    py= new float **[1];
    px[0]=px_;
    py[0]=py_;
    numOfTotPlots=nPlots[0];
    if(variableStep1)numOfVSFiles=1;
    dataGot=true;
}

void CLineChart::getData(QList <SFileInfo> FI, int *nPlots_,  SXVarParam xVarParam_, SCurveParam *curveParam_,float **px_,float ***py_){
    /*getData per il caso di multiple variabili asse x*/

    filesInfo=FI;
    nFiles=FI.count();
    nPlots=nPlots_;
    xVarParam=xVarParam_;
    curveParam=curveParam_;
    px=px_;
    py=py_;
    numOfTotPlots=0;
    numOfVSFiles=0;
    for(int i=0; i<nFiles; i++){
        numOfTotPlots+=nPlots[i];
        if(FI[i].variableStep)numOfVSFiles++;
    }
    dataGot=true;
}


int CLineChart::giveActiveDataCurs(){
    if(dataCurs2Visible) return 2;
    if(dataCursVisible) return 1;
    return 0;
}

SFloatRect2 CLineChart::giveDispRect(){
    return dispRect;
}

EPlotPenWidth CLineChart::givePlotPenWidth() const {
    return PPlotPenWidth;
}

SAllLabels CLineChart::giveAllLabels(void){
  SAllLabels all;
  all.xLbl=xUserLabel;
  all.yLbl=yUserLabel;
  all.ryLbl=ryUserLabel;
  return all;
}


 SXYValues CLineChart::giveValues(int cursorX, bool interpolation,  bool Xdiff, bool Ydiff){
    int nearX;
    return giveValues(cursorX, interpolation, nearX, Xdiff, Ydiff);
}

SXYValues CLineChart::giveValues(int CursorX, bool Interpolation, int &NearX, bool Xdiff, bool Ydiff){
/*  Function che dà i valori delle variabili visualizzate in corrispondenza della posizione, passata come parametro, della retta di cursore.
 Nel vettore di uscita si troverà prima il valore della variabile sull'asse x,  poi quello delle variabili sull'asse y.
 NearX ha un significato pregnante soltanto se non sono in multifile o se in multifile le varie variabili X coincidono. In questo caso dà la posizione in pixel del punto rispetto a cui vengono dati i valori. Essa verrà sfruttata per muovere "a scatto" il CursorShape nel punto giusto.
Nel caso di multifile con files aventi tempi di campionamento diversi, i vari valori si riferiscono in generale ad ascisse differenti, e quindi questa logica del movimento "a scatto" ha poco senso. In questo caso NearX contiene l'ascissa del valore calcolato rispetto all'ultimo file considerato.
*/

    /******************************************************************
    Questa funzione dà per scontato che sia già stato effettuato un plot, e non fa alcun
    check a riguardo. E' responsabilità del chiamante richiamare questa funzione soltanto
    se esiste un plot effettuato in LineChart.
    ****************************************************************/

    int iFile, iVar, NetCursorX,
       VSFile=0, //indice del file VariableStep
       IndexSX, //Indice del punto a SX del cursore
       Index;	//l'indice del punto più vicino al cursore
    float fCursorX, DeltaX, Slope;
    SXYValues Ret;
    NetCursorX=min(max(CursorX-X0-margin,0),X1-X0-2*margin);
    if(xAxis.scaleType==stLin)
        fCursorX=xAxis.scaleMin*xAxis.scaleFactor+NetCursorX/ratio.x;
    else
        //In questo caso devo tener conto del logaritmo per la costruzione del grafico,
        // e posso omettere ScaleFactor che è sempre unitario:
        fCursorX=pow(10,xAxis.eMin+NetCursorX/ratio.x);
    //Può capitare che il cursore sia fuori della zona in cui ci sono grafici.
    //In tal caso riporto dentro il valore di fCusorX:
    fCursorX=min(fCursorX,xmM.Max);
    fCursorX=max(fCursorX,xmM.Min);
    if(Xdiff)
        lastXCurs[1]=fCursorX;
    else if(Ydiff)
        lastXCurs[2]=fCursorX;
    else
        lastXCurs[0]=fCursorX;


    if(!xVarParam.isMonotonic){
        QMessageBox::critical(this,tr("MC's PlotXY"),tr("X variable here must be monotonic"),QMessageBox::Ok);
//        qApp->closeAllWindows();

        //Le seguenti due righe solo per evitare un Warning:
        Ret.X=cursorXValues;
        Ret.Y=cursorYValues;
        return Ret;
    }
    for(iFile=0; iFile<nFiles; iFile++){
        int nPoints=filesInfo[iFile].numOfPoints;
/***Per ogni grafico scelgo la coppia di valori più vicina alla riga-cursore visualizzata:     ***/
        if(filesInfo[iFile].variableStep){
            IndexSX=max(0,pixelToIndexDX[VSFile][NetCursorX]-1);
            DeltaX=px[iFile][IndexSX+1]-px[iFile][IndexSX];
            if(fCursorX-px[iFile][IndexSX]<px[iFile][IndexSX+1]-fCursorX)
                Index=IndexSX;
            else
                Index=IndexSX+1;
        }else{
            DeltaX=px[iFile][1]-px[iFile][0];
            IndexSX=(fCursorX-px[iFile][0])/DeltaX;
            Index=NearInt((fCursorX-px[iFile][0])/DeltaX);
            //Può accadere che fra l'ultimo e il penultimo punto non si abbia la stessa distanza
            //che fra gli altri. Questo perché se Tmax supera TOld+DeltaT, vengono registrati
            //i valori a Tmax e non a TOld+DeltaT. La seguente riga considera questa eventualità:
            if(  fabs(px[iFile][nPoints-1]-fCursorX)   <   fabs(px[iFile][nPoints-2]-fCursorX)
             &&  Index<nPoints-1    ) Index++;
        }
        if(Interpolation){
            for(iVar=0; iVar<nPlots[iFile]; iVar++){
                Slope=(py[iFile][iVar][IndexSX+1]-py[iFile][iVar][IndexSX])/DeltaX;
                cursorXValues[iFile]=fCursorX-(int)Xdiff*cursorXValBkp[iFile];
                cursorYValues[iFile][iVar]=py[iFile][iVar][IndexSX]+
                    /*Slope=DeltaY/DeltaX:*/ Slope*(fCursorX-px[iFile][IndexSX]) -(int)Ydiff* cursorYValBkp[iFile][iVar];
                if(!Ydiff)cursorYValBkp[iFile][iVar]=cursorYValues[iFile][iVar];
            }
        }else{
            /*Occorre tener conto della possibilità che non tutti i grafici hanno piena
            lunghezza. Dove ho un valore non inizializzato, per convenzione (simile a
            quella di MODELS) gli dò il valore 8888.8
            */
            for(iVar=0; iVar<nPlots[iFile]; iVar++){
                if(fCursorX<=px[iFile][nPoints-1]){
                    cursorXValues[iFile]=px[iFile][Index]-(int)Xdiff*cursorXValBkp[iFile];
                    cursorYValues[iFile][iVar]=py[iFile][iVar][Index] -(int)Ydiff* cursorYValBkp[iFile][iVar];
                    if(!Ydiff)cursorYValBkp[iFile][iVar]=cursorYValues[iFile][iVar];
                }else{
                    cursorXValues[iFile]=8888.8f;
                    cursorYValues[iFile][iVar]=8888.8f;
                }
            }
        }
        if(filesInfo[iFile].variableStep)  VSFile++;
        if(Interpolation||nFiles>1)
            NearX=CursorX;
        else{
            if(cursorXValues[iFile]==8888.8f)
                NearX=X0;
            else{
                if(xAxis.scaleType==stLin)
                    NearX=X0+margin+ratio.x*(cursorXValues[iFile]+(int)Xdiff*cursorXValBkp[iFile]  -xAxis.scaleMin* xAxis.scaleFactor);
                else
                    NearX=X0+margin+ratio.x*log10(cursorXValues[iFile]/xAxis.scaleMin);
            }
        }
        if(!Xdiff)cursorXValBkp[iFile]=cursorXValues[iFile];
    }
    Ret.X=cursorXValues;
    Ret.Y=cursorYValues;
    return Ret;
}


SFloatRect2 CLineChart::giveZoomRect(int StartSelX, int StartSelY, int X, int Y){
    SFloatRect2 R;

    if(xAxis.scaleType==stLin){
        R.Left=xAxis.scaleMin*xAxis.scaleFactor+max(StartSelX-X0,0)/ratio.x;
        R.Right=xAxis.scaleMin*xAxis.scaleFactor+min(X-X0,xAxis.width)/ratio.x;
    }else{
        //In questo caso devo tener conto del logaritmo per la costruzione del grafico,
        // e posso omettere ScaleFactor che è sempre unitario:
        R.Left= pow(10,xAxis.eMin+max(StartSelX-X0,0)/ratio.x);
        R.Right=pow(10,xAxis.eMin+min(X-X0,xAxis.width)/ratio.x);
    }
    if(yAxis.scaleType==stLin){
        R.LBottom=yAxis.scaleMin*yAxis.scaleFactor+max(Y1-Y,0)/ratio.y;
        R.LTop=   yAxis.scaleMin*yAxis.scaleFactor+min(Y1-StartSelY,yAxis.width) /ratio.y;
  }else{
        //In questo caso devo tener conto del logaritmo per la costruzione del grafico,
        // e posso omettere ScaleFactor che è sempre unitario:
        R.LBottom=pow(10,yAxis.eMin+max(Y1-Y,0)/ratio.y);
        R.LTop=   pow(10,yAxis.eMin+min(Y1-StartSelY,yAxis.width)/ratio.y);
  }
    if(twinScale){
        R.RBottom=rYAxis.scaleMin*rYAxis.scaleFactor+max(Y0+yAxis.width-Y,0)/ratio.yr;
        R.RTop=rYAxis.scaleMin*rYAxis.scaleFactor+min(Y0+yAxis.width-StartSelY,yAxis.width) /ratio.yr;
    }
    return R;
}




void CLineChart::leaveEvent(QEvent *){
    setCursor(Qt::ArrowCursor);
}


void CLineChart::mouseMoveEvent(QMouseEvent *event)
{
/* Questa routine ha tre sezioni:
1) nella prima gestisce l'eventuale vizualizzazione di informazioni aggiuntive su una variabile quando si fa l'hovering sul nome. Al momento è implemtnata solo la visualizzazione del nome di funzione completo quando si fa l'hovering sul nome conciso normalmente visualizzato
2) nella seconda gestisce la selezione dell'area di zoom
3) nella terza gestisce sia la definizione della posizione finale del rettangolo di selezione, che la transizione del cursore mouse in freccette orizzontali quando siamo in prossimità del rettangolo dei dati.
Il cursore del mouse deve rimanere a freccette orizzontali in taluni casi anche lontano dal cursore dati: quando mi sto spostando e il cursore dati resta indietro perché attratto dal punto più vicino.
*/
    int nearX;
    static SXYValues values;
    int x=event->pos().x();
    hovVarRect=QRect(0,0,0,0);

    setCursor(Qt::ArrowCursor);
    //1) verifico se sono all'interno di un'area contenente il nome di variabile.
    setToolTip("");
    foreach(SHoveringData hovData, hovDataLst){
      if(hovData.rect.contains(event->pos())){
        if(!curveParam[hovData.iTotPlot].isFunction)break;
        hovVarRect= hovData.rect;
        QString str=curveParam[hovData.iTotPlot].fullName;
        setToolTip(curveParam[hovData.iTotPlot].fullName);
      }
    }

    if(zoomSelecting){  //sono in fase di selezione del rettangolo di zoom
      //endPos è la posizione finale per il rettangolo delle zoomate
      endPos=event->pos();
      update();
      return;
    }
   //A questo punto non sto selezionando il rettangolo di zoom. Adesso se sono in un'azione di dataRectDragging aggiorno i valori numerici e la posizione del cursore dati; altrimenti valuto se il mouse è nel raggio di azione di uno dei tre cursori dati. La cosa viene ripetuta tre volte, una per ogni cursore dati.
    dataCursSelecting=0;
    if(dataCursVisible){
      if(dataCursDragging){
        int rWidth=dataCurs.width(); //va calcolata volta per volta perché nel caso di ptBar la larghezza è calcolata dinamicamente
        setCursor(Qt::SizeHorCursor);
        //la seguente chiamata atribuisce un valore a nearX:
        values=giveValues(x-1, FLinearInterpolation, nearX, false, false);
        if(nearX<X0)nearX=X0;
        if(nearX>X1) nearX=X1;
        dataCurs.moveLeft(nearX-rWidth/2);
        //Mando fuori i valori:
        emit valuesChanged(values,false,false);
      }else{
  //Se sono a meno di 5 pixel dall'eventuale cursore dati, cambio poi il cursore mouse per indicare la trascinabilità del cursore dati:
        if(x>dataCurs.x()-5 && x<dataCurs.x()+dataCurs.width()+5){
          setCursor(Qt::SizeHorCursor);
          dataCursSelecting=1;
        }
      }
    }

    if(dataCurs2Visible){
      if(dataCurs2Dragging){
        setCursor(Qt::SizeHorCursor);
        values=giveValues(x-1, FLinearInterpolation, nearX, true, true);
        dataCurs2.moveLeft(nearX-1);
        //Mando fuori i valori:
        emit valuesChanged(values,true,true);
      }else{
  //Se sono a meno di 5 pixel dall'eventuale cursore dati, setto il corrispodente flag affinché in fondo alla routine si cambi poi il cursore mouse:
        if(x>dataCurs2.x()-5 && x<dataCurs2.x()+dataCurs2.width()+5){
          setCursor(Qt::SizeHorCursor);
          dataCursSelecting=2;
        }
      }
    }

    update();

}


void CLineChart::mouseDoubleClickEvent(QMouseEvent *event){
  /* Il double click serve solo per introdurre il testo del titolo.
   *Esso andrà effettuato quando il titolo è visibile e all'interno dell'area titolo
  */
  bool ok;
  //Se il doppioclick non è nel rettangolo del titolo esco:
  if(!titleRectF.contains(event->pos()))return;
  //Se il titolo non è visibile esco
  if(!writeTitle1)return;

  QString text = QInputDialog::getText(this, "plot Title",
                       "enter title text:", QLineEdit::Normal,  "", &ok);
  titleText=text;
  zoomSelecting=false;
  resizeStopped();
}


void CLineChart::mousePressEvent(QMouseEvent *event){
/* Quando mi sposto con il mouse, se entro nel raggio d'azione di un dataCursor il puntatore del mouse diviene la doppia freccetta orizontale ed un eventuale click aggancia il dataCursor attivo alla posizione del mouse.
Questo aggancio viene comandato nella funzione mouseMoveEvent(), attraverso la selezione del valore opportuno per la variabile dataCursSelecting, la quale vale 0 se quando non siamo nel raggio d'azione di alcun cursore dati, altrimenti vale il numero del cursore dati: 1 per quello base, 2 per quello con le differenze.

  Pertanto la routine ha un comportamento che va distinto in funzione del valore della variabile dataCursSelecting.

*/
    int Ret=0, x1=event->pos().x()-1;

    if(dataCursSelecting>0){  //sono nel raggio d'azione di un qualche cursore dati
      if(event->buttons() & Qt::RightButton)return;
      switch(dataCursSelecting){
        case 1:
          dataCursDragging=true;
          lastDataCurs=1;
          dataCurs.moveLeft(x1);
          /* In conseguenza di un baco non identificato nell'evento paint arrivano spessissimo valori di dataRect errati.
In attesa di comprendere la causa del problema copio il rettangolo in una copia di riserva sperando di tamponare così il problema*/
          debugCurs=dataCurs;

          break;
        case 2:
          dataCurs2Dragging=true;
          lastDataCurs=2;
          dataCurs2.moveLeft(x1);
          break;
        default:
          ;
      }
      mouseMoveEvent(event);
      return;
    }

    //Se arrivo a questo punto il mouse non è nel raggio d'azione di alcun cursore dati e la pressione viene associata alle funzioni di zoom.
    if((event->buttons() & Qt::RightButton) && zoomed){ //tasto destro: menù contestuale per dezoom
      QMenu myMenu;
      QAction * zoombackAct, *unzoomAct, *myAct;
      zoombackAct=myMenu.addAction("Zoom Back");
      unzoomAct=myMenu.addAction("Unzoom");
      myAct=myMenu.exec(event->globalPos());
      if(myAct==zoombackAct)
        dispRect=plStack.pop();
      if(myAct==unzoomAct){
        exactMatch=false;
        while(!plStack.isEmpty())  dispRect=plStack.pop();
      }
      if(plStack.isEmpty())zoomed=false;
      Ret=scaleXY(dispRect,false);
      if(Ret) return;
      plot(false,false);
      return;
  }
  if(event->buttons() & Qt::LeftButton){  //tasto sinistro: inizio zoomata
    //Qui è stato premuto il bottone sinistro:
    zoomSelecting=true;
    stPos=event->pos();
  }
}



void CLineChart::mouseReleaseEvent(QMouseEvent *ev)
{

    setCursor(Qt::ArrowCursor);
    if(dataCursDragging){
        dataCursDragging=false;
        return;
    }
    if(dataCurs2Dragging){
        dataCurs2Dragging=false;
        return;
    }

    if(zoomSelecting){
        SFloatRect2 oldDispRect=dispRect;
        if(ev->x()<=stPos.x() || ev->y()<=stPos.y()) goto Return;
        zoomed=true;
        forceYZero=false;
        dispRect=giveZoomRect(stPos.x(), stPos.y(), ev->x(), ev->y());
        //Qui devo comandare il grafico con i nuovi estremi
        exactMatch=false;
        scaleXY(dispRect,false);
        dispRect.Left =xAxis.scaleMin*xAxis.scaleFactor;
        dispRect.Right=xAxis.scaleMax*xAxis.scaleFactor;
/*
Il seguente if sarebbe pensato per evitare di fare un diagramma a barre contenente meno di 3 barre,il quale infatti ha poco senso.
Viononostante così com'è on funziona, in quanto può benissimo accadere che dispRect.Right-dispRect.Left<3 e che fra tali due estermi il numero di barre sia superiore a 3.
pertanto il codice per ora è commentato Eventualmente potrà essere reintrodotto in modo corretto in un secondo momento.
*/
/*        if(plotType==ptBar && dispRect.Right-dispRect.Left<3){
            dispRect=oldDispRect;
            scaleXY(dispRect,false);
            QMessageBox::warning(this,tr("MC's PlotXWin"),tr("Zoom Rectangle too small!"),QMessageBox::Ok);
            goto Return;
        }
*/
        myCursor=Qt::BusyCursor;
        if(xAxis.done==0 || yAxis.done==0){
            QMessageBox::warning(this,tr("MC's PlotXWin"),tr("Unable to zoom so deeply."),QMessageBox::Ok);
            dispRect=oldDispRect;
            scaleXY(dispRect,false);
        }else{
            plStack.push(oldDispRect);
            plot(false,false);
//          ResetMarkData();
        }
/*La seguente riga per notificare all'esterno il cambiamento di stato.
Potrà essere sostituita con l'emissione di un Signal()*/
//        if(OnZoomStateChange) OnZoomStateChange(Owner,FZoomed);
    }
    Return:
      endPos=stPos;
      zoomSelecting=false;
//      update();

}

void  CLineChart::drawAllLabelsAndGrid(SAxis axis){
  /* Calcola:
   - tacche
   - etichette numeriche ("numLabels")
   - etichette di asse ("axisLabel")
   - griglia
 */
  int x, y, ticCount, yy;
  float xf, yf, ticInterval;
  double auxd;
  //char num[10], * format="%.1f";
  QString numStr;
  QPainterPath ticPath, gridPath;

  if(axis.scaleType!=stLin){
    if(axis.scaleType==stDB)
      drawAllLabelsAndGridDB(axis);
    else
      drawAllLabelsAndGridLog(axis);
    return;
  }
  /* Ora che ho rimandato le scale logaritmiche ad altri files, qui di seguito tratto
     solo le scale lineari.
  */

  if(axis.addZeroLine){
    int zeroPosition, initialPix, finalPix;
    if(axis.type==atX){
      initialPix=Y0;
      finalPix=Y1;
      if(axis.scaleMin*axis.scaleMax>0)
        zeroPosition=-1;
      else{
        zeroPosition=NearInt(-axis.scaleMin/(axis.scaleMax-axis.scaleMin)*(X1-X0)+X0);
      }
    }else{
      initialPix=X0;
      finalPix=X1;
    }
    if(axis.scaleMin*axis.scaleMax>=0)
      zeroPosition=-1;
    else{
      zeroPosition=NearInt(axis.scaleMax/(axis.scaleMax-axis.scaleMin)*(Y1-Y0)+Y0);
    }
    ticPath.moveTo(initialPix,zeroPosition);
    ticPath.lineTo(finalPix,zeroPosition);
  }

  if(axis.type==atX){
    // Tacche e numeri:
    ticCount=0;
    for(x=X0+margin, auxd=X0+margin, xf=xAxis.scaleMin; x<=X1-margin;
            auxd+=ticInterv.x, x=NearInt(auxd), xf+=xAxis.ticInterval) {
      ticPath.moveTo(x,yAxis.width+Y0+1);
      ticPath.lineTo(x,yAxis.width+Y0+ticWidth.x()+1);
      numStr=QString::number(xf,'g',4);
//      qDebug()<<"numStr: "+numStr;
      if(ticCount/2*2==ticCount){
        drawText2(x,Y1+ticWidth.x(),atCenter,atLeft,numStr,"",false);
      }
      if(xAxis.halfTicNum)ticCount++;
    }
    //La seguente riga mette la label di asse al centro fra due tic, se non sono in halfTicNum, nel
    //qual caso viene spostata un poco a sinistra ma non tanto da sembrare posta proprio in
    // corrispondenza della "halftic".
    writeAxisLabel(X1-(0.5+0.2*xAxis.halfTicNum)*ticInterv.x, Y1+ticWidth.x(),xAxis,false);
    //Eventuale griglia:
    for(auxd=ticInterv.x, x=NearInt(auxd); x<xAxis.width-1;	auxd+=ticInterv.x, x=NearInt(auxd)) {
      if(xGrid) {
        gridPath.moveTo(x+X0,Y0);
        gridPath.lineTo(x+X0,Y0+yAxis.width);
      }
    }
  } else {   // *******  Casi assi atYL e atYR
    // Tacche e label numeriche:
    if(axis.type==atYR){
      ticInterval=ticInterv.yr;
    }else{
      ticInterval=ticInterv.y;
    }
    ticCount=0;
     for(y=yAxis.width+Y0, auxd=(double)(yAxis.width+Y0),  yf=axis.scaleMin; y>=Y0-2;
               auxd-=ticInterval, y=NearInt(auxd), yf+=axis.ticInterval)   	  {
      if(axis.type==atYR){ //caso di scala destra
        ticPath.moveTo(X1+1,y);
        ticPath.lineTo(X1+1+ticWidth.y(),y);
      }else{
         ticPath.moveTo(X0-1,y);
         ticPath.lineTo(X0-1-ticWidth.y(),y);
      }
      if(fontSizeType==fsFixed) myPainter->setFont(QFont(baseFontName,fixedFontPx));
      numStr=QString::number(yf,'f',axis.ticDecimals);
      if(ticCount/2*2==ticCount){
        if(axis.type==atYR){
          drawText2(X1+ticWidth.y(),y,atLeft,atCenter,numStr,"",false);
        }else{
          drawText2(X0-ticWidth.y()-1,y,atRight,atCenter,numStr,"",false);
        }
      }
      if(axis.halfTicNum)ticCount++;
    }
    // Labels di asse:
    if(axis.halfTicNum)
      yy=Y0+0.9*ticInterval;
    else
      yy=Y0+0.5*ticInterval;
    if(axis.type==atYR)
       writeAxisLabel(X1+0.5*(1+axis.halfTicNum)*ticWidth.y() ,yy, axis,false);
    else{
      writeAxisLabel(X0-0.5*(1+axis.halfTicNum)*ticWidth.y() , yy, axis,false);
    }
    //eventuale griglia (solo per la scala di sinistra)
    if(yGrid && axis.type!=atYR){
      for(y=yAxis.width+Y0, auxd=(double)(yAxis.width+Y0),  yf=axis.scaleMin; y>=Y0;
      auxd-=ticInterv.y, y=NearInt(auxd), yf+=axis.ticInterval) {
        if(y!=yAxis.width+Y0 && y!=Y0) {
          gridPath.moveTo(X0,y);
//          gridPath.lineTo(X0+xAxis.width-2,y);
          gridPath.lineTo(X1,y);
        }
      }
    }  //fine if(Axis.Grid)
  } //fine casi assi atYL e atYR
  myPainter->setPen(ticPen);
  myPainter->drawPath(ticPath);
  myPainter->setPen(gridPen);
  myPainter->drawPath(gridPath);
}

//---------------------------------------------------------------------------
void  CLineChart::drawAllLabelsAndGridDB(SAxis Axis){
/* Versione della funzione LabelsAndGrid relativa a scale logaritmiche tarate in dB
 */
  int i,
    Pos[MAXLOGTICS], //Posizioni lungo l'asse considerato delle varie tacche
    Pos1, Pos0, //sono X1-X0 ovvero Y1-Y0
    Pos10, //=Pos1-Pos0
    NumTics,
    NumTicType; //Se è 1 metto solo tacche agli estremi della scala; se 2 ogni 20 db, se 3 ogni 10 db
  char num[10];
  float  DBStep,Value0,Value;     //distanza in stDB fra due tacche consecutive

  QPainterPath path;

  myPainter->setPen(ticPen);
  myPainter->setFont(QFont(baseFontName,generalFontPx));
  if(Axis.type==atX){
    Pos0=X0;
    Pos1=X1;
  }else{
    Pos0=Y0;
    Pos1=Y1;
  }
  Pos10=Pos1-Pos0;
  // Inizialmente ipotizzo di mettere tacche ogni 20 db:
  NumTicType=2;
  //Se c'è poco spazio metto solo tacche agli estremi:
  if( Pos10/(Axis.eMax-Axis.eMin) < 2*fontMetrics().height() ) NumTicType=1;
  //Se c'è spazio metto tacche ogni 10 db, purché non siano in numero eccessivo:
  if(Axis.eMax-Axis.eMin <= MAXLOGTICS/3        &&
       Pos10/(Axis.eMax-Axis.eMin) > 5.5*fontMetrics().height() ) NumTicType=3;

  //Ora devo compilare il vettore con le posizioni delle tacche
  // (dall'alto verso il basso ovvero da sinistra a destra)
  Pos[0]=Pos0;
  switch(NumTicType){
    case 1:
      DBStep=20.f*(Axis.eMax-Axis.eMin);
      Axis.ticInterval=Axis.width;
      Pos[1]=Pos1;
      NumTics=2;
      break;
    case 2:
      DBStep=20.f;
      Axis.ticInterval= (float)(Pos10+1)/(Axis.eMax-Axis.eMin);
      for(i=1; i<=Axis.eMax-Axis.eMin; i++){
        Pos[i]=Pos[i-1]+Axis.ticInterval;
      }
      NumTics=(Axis.eMax-Axis.eMin)+1;
      break;
    case 3:
      DBStep=10.f;
      Axis.ticInterval= (float)(Pos10+1)/(Axis.eMax-Axis.eMin)/2.;
      for(i=1; i<=2*(Axis.eMax-Axis.eMin); i+=2){
        Pos[i]=Pos[0]+i*Axis.ticInterval;
        Pos[i+1]=Pos[0]+(i+1)*Axis.ticInterval;
      }
      //Per gli arrotondamenti ci può essere un errore massimo di 1 pixel sull'ultima
      //tacca e lo correggo:
      if(abs(Pos[i-1]-Pos1)==1)  Pos[i-1]=Pos1;
      NumTics=2*(Axis.eMax-Axis.eMin)+1;
      break;
  }

  //Scrittura tacche e label numeriche:
  Value0=20.*Axis.eMin;
  for(i=0; i<NumTics; i++){
    // Tacche:
    if(Axis.type==atX){
      path.moveTo(Pos[i],Y1+1);
      path.lineTo(Pos[i],Y1+ticWidth.x());
    }else if(Axis.type==atYL){
      path.moveTo(X0,Pos[i]);
      path.lineTo(X0-ticWidth.y(),Pos[i]);
    }else{
      path.moveTo(X1,Pos[i]);
      path.lineTo(X1-ticWidth.y(),Pos[i]);
    }
    Value=Value0+i*DBStep;
    sprintf(num,"%.0f",Value);
    //Valore numerico:
    if(Axis.type==atX){
        drawText2(Pos[i],Y1,atCenter,atLeft,num,"",false);
    }else if(Axis.type==atYL){
        drawText2(X0-ticWidth.y()-1,Pos[NumTics-i-1],atRight,atCenter,num,"",false);
    }else{
        drawText2(X1+smallHSpace,  Pos[NumTics-i-1],atLeft,atCenter,num,"",false);
    }
  }
  //eventuale griglia (asse y solo per la scala di sinistra)
  myPainter->setPen(gridPen);
  if(Axis.type==atX){
    for(i=1; i<NumTics-1; i++){
      path.moveTo(Pos[i],Y0);
      path.lineTo(Pos[i],Y1);
    }
  }else if(Axis.type==atYL){
    if(yGrid){
       for(i=1; i<NumTics-1; i++){
        path.moveTo(X0,Pos[i]);
        path.lineTo(X1,Pos[i]);
      }
    }
  }else
    goto Return;
  //Label "dB":
  if(Axis.type==atX){
      drawText2(X1-Axis.ticInterval/2,Y1+ticWidth.x(), atCenter,atLeft,"dB","",false);
    }else{
      writeAxisLabel(X0-ticWidth.y(),Y0+Axis.ticInterval/2., Axis,false);
    }
  Return:
    myPainter->drawPath(path);
}

//---------------------------------------------------------------------------
void CLineChart::drawAllLabelsAndGridLog(SAxis axis){
/* Versione della funzione LabelsAndGrid relativa a scale logaritmiche tarate in valori effettivi della variabile prima dell'effettuazione del logaritmo (scale "stLog")
*/
  int i, dec, //indice di decade
    tic, //indice di tacca all'interno della decade
    Pos[MAXLOGTICS],
    Pos1, Pos0, //sono X1-X0 ovvero Y1-Y0
    Pos10, //=Pos1-Pos0
    xx, yy,
    NumTicType; //Se è 1 metto solo tacche agli estremi della scala; se 2 ogni decade,
           //se 3 ogni due unita' interne alla decade
  float decInterval=0; //spaziatura in pixel fra le tacche di due decadi consecutive
  char num[10];
  QPainterPath ticPath, gridPath;

  myPainter->setPen(ticPen);
  myPainter->setFont(QFont(baseFontName,generalFontPx));

  if(axis.type==atX){
    Pos0=X0;
    Pos1=X1;
  }else{
    Pos0=Y0;
    Pos1=Y1;
  }
  Pos10=Pos1-Pos0;
  // Inizialmente ipotizzo di mettere tacche ogni decade:
  NumTicType=2;
  //Se c'è poco spazio metto solo tacche agli estremi:
  if( Pos10/(axis.eMax-axis.eMin) < 2*fontMetrics().height() ) NumTicType=1;
  //Se c'è spazio metto tacche ogni due unità interne alla decade,
  //purché non si superino le 5 decadi:
  if(Pos10/(axis.eMax-axis.eMin) > 5*fontMetrics().height() && axis.eMax-axis.eMin<6)
    NumTicType=3;
  //Ora devo compilare il vettore con le posizioni delle tacche
  // (dal basso verso il basso ovvero da sinistra a destra)
  Pos[0]=Pos1;
  if(axis.type==atX) Pos[0]=Pos0;
  switch(NumTicType){
   case 1:
      Pos[1]=Pos0;
      break;
    case 2:
      decInterval= (float)(Pos10+1)/(axis.eMax-axis.eMin);
      for(dec=1; dec<axis.eMax-axis.eMin; dec++){
         Pos[dec]=Pos[0]+(2*(axis.type==atX)-1)*dec*decInterval;
      }
      Pos[axis.eMax-axis.eMin]=Pos1;
      break;
    case 3:
      decInterval= (float)(Pos10+1)/(axis.eMax-axis.eMin);
      for(dec=0; dec<axis.eMax-axis.eMin; dec++){
        Pos[5*dec]=Pos[0]+(2*(axis.type==atX)-1)*dec*decInterval;
        for(tic=1; tic<5; tic++){
          Pos[5*dec+tic]=Pos[5*dec]+(2*(axis.type==atX)-1)*log10((float)2*tic)*decInterval;
        }
      }
      Pos[5*(axis.eMax-axis.eMin)]=Pos0+(axis.type==atX)*(Pos1-Pos0);
      break;
  }

  //predisposizione path  tacche e relativi numeri, ed eventuale griglia
  switch(NumTicType){
  case 1:
    for(i=0; i<2; i++){
      if(axis.type==atX){
        ticPath.moveTo(Pos[i],Y1);
        ticPath.lineTo(Pos[i],Y1+ticWidth.x());
        drawText2(Pos[i],Y0-ticWidth.x(),atCenter,atLeft,"10",num,false);
      }else if(axis.type==atYL){
        ticPath.moveTo(X0-ticWidth.y(),Pos[i]);
        ticPath.lineTo(X0+1,   Pos[i]);
        sprintf(num,"%d",axis.eMin+i*(axis.eMax-axis.eMin));
        drawText2(X0-ticWidth.y(),Pos[i],atRight,atCenter,"10",num,false);
      }else{ //caso atYR
        ticPath.moveTo(X1-ticWidth.y(),Pos[i]);
        ticPath.lineTo(X1,   Pos[i]);
        sprintf(num,"%d",axis.eMin+i*(axis.eMax-axis.eMin));
        drawText2(X1+smallHSpace,Pos[i],atLeft,atCenter,"10",num,false);
      }
    }
    break;
  case 2:
    for(dec=0; dec<=axis.eMax-axis.eMin; dec++){
      if(axis.type==atX){
        ticPath.moveTo(Pos[dec],Y1);
        ticPath.lineTo(Pos[dec],Y1+ticWidth.x());
        if(xGrid && dec>0 && dec<axis.eMax-axis.eMin){
          gridPath.moveTo(Pos[dec],Y0);
          gridPath.lineTo(Pos[dec],Y1);
        }
        ticPath.moveTo(Pos[dec],Y1);
        ticPath.lineTo(Pos[dec],Y1+ticWidth.x());
        sprintf(num,"%d",axis.eMin+dec);
        drawText2(Pos[dec],Y1+ticWidth.x(),atCenter,atLeft,"10",num,false);
      }else if(axis.type==atYL){
        myPainter->setPen(ticPen);
        ticPath.moveTo(X0-ticWidth.y(),Pos[dec]);
        ticPath.lineTo(X0+1,   Pos[dec]);
        sprintf(num,"%d",axis.eMin+dec);
        drawText2(X0-ticWidth.y(),Pos[dec],atRight,atCenter,"10",num,false);
        if(yGrid && dec>0 && dec<axis.eMax-axis.eMin){
            //Non faccio la riga in corrispodnenza della posizione 10^0 di diagrammi
            //a barre con scala verticale logaritmica per evitare che per errori di
            //arrotondamento restino visibili sia la riga continua di base delle
            //barre che la linea tratteggiata della griglia:
          if(!(plotType==ptBar && axis.scaleType!=stLin && axis.eMin+dec==0)){
            gridPath.moveTo(X0,Pos[dec]);
            gridPath.lineTo(X1,Pos[dec]);
          }
        }
      }else{ //caso atYR
        myPainter->setPen(ticPen);
        ticPath.moveTo(X0-ticWidth.y(),Pos[dec]);
        ticPath.lineTo(X0+1,   Pos[dec]);
        sprintf(num,"%d",axis.eMin+dec);
        drawText2(X1+smallHSpace,Pos[dec],atLeft,atCenter,"10",num,false);
      }
   }
   break;
  case 3:
   for(dec=0; dec<=axis.eMax-axis.eMin; dec++){
     myPainter->setPen(ticPen);
     sprintf(num,"%d",axis.eMin+dec);
     if(axis.type==atX){
       ticPath.moveTo(Pos[5*dec],Y1);
       ticPath.lineTo(Pos[5*dec],Y1+ticWidth.x());
       drawText2(Pos[5*dec],Y1+ticWidth.x(),atCenter,atLeft,"10",num,false);
     }else if(axis.type==atYL){
       ticPath.moveTo(X0-ticWidth.y(),Pos[5*dec]);
       ticPath.lineTo(X0+1,   Pos[5*dec]);
       drawText2(X0-ticWidth.y(),Pos[5*dec],atRight,atCenter,"10",num,false);
     } else{ //case atYR
       ticPath.moveTo(X1-ticWidth.y(),Pos[5*dec]);
       ticPath.lineTo(X1,   Pos[5*dec]);
       drawText2(X1+smallHSpace,Pos[5*dec],atLeft,atCenter,"10",num,false);
     }
     for(tic=0; tic<5; tic++){
       if(dec==axis.eMax-axis.eMin)break;
       //Scrittura delle tacche intermedie fra le decadi intere ed eventuale griglia:
        if(axis.type==atX){
          if(xGrid){
             gridPath.moveTo(Pos[5*dec+tic],Y0);
             gridPath.lineTo(Pos[5*dec+tic],Y1);
           }else{
             ticPath.moveTo(Pos[5*dec+tic],Y0);
             ticPath.lineTo(Pos[5*dec+tic],Y0+ticWidth.x());
             ticPath.moveTo(Pos[5*dec+tic],Y1);
             ticPath.lineTo(Pos[5*dec+tic],Y1-ticWidth.x());
           }
         }else if(axis.type==atYL){
           //a barre con scala verticale logaritmica per evitare che per errori di
           //arrotondamento restino visibili sia la riga continua di base delle
           //barre che la linea tratteggiata della griglia:
           if(tic!=0 || !(plotType==ptBar && axis.eMin+dec==0)){
             if(yGrid){
               gridPath.moveTo(X0,Pos[5*dec+tic]);
               gridPath.lineTo(X1,Pos[5*dec+tic]);
             }else{
               ticPath.moveTo(X0,Pos[5*dec+tic]);
               ticPath.lineTo(X0+ticWidth.y(),Pos[5*dec+tic]);
               if(!twinScale){
                 ticPath.moveTo(X1,Pos[5*dec+tic]);
                 ticPath.lineTo(X1-ticWidth.y(),Pos[5*dec+tic]);
               }
             }
          }
        } else { //caso atYR
          if(tic!=0 || !(plotType==ptBar && axis.eMin+dec==0)){
            ticPath.moveTo(X1,Pos[5*dec+tic]);
            ticPath.lineTo(X1-ticWidth.y(),Pos[5*dec+tic]);
          }
        } //fine if(Axis.Type==atX)
      } //fine for(tic=0 ...
    } //fine for(dec=0 ...
    break;
  } //fine switch

  myPainter->setPen(gridPen);
  myPainter->drawPath(gridPath);
  myPainter->setPen(ticPen);
  myPainter->drawPath(ticPath);

  //Label
  if(axis.type==atX){
    xx=X1-axis.ticInterval/2;
    yy=Y1+ticWidth.x();
  }else{
    //Si ricordi che la label nel caso di assi sinistri (come questo) è automaticamente
    //accostata a destra e quindi l'X da passare è l'ascissa dell'estremo destro della label
    xx=X0-ticWidth.y();
    yy=Pos0+decInterval/2;
  }
  writeAxisLabel(xx,yy, axis,false);
}


void CLineChart::paintEvent(QPaintEvent *ev)
{
    QPainter painter(this);
    QRect dirtyRect = ev->rect();
    painter.drawImage(dirtyRect, *myImage);

    QColor selCol(255,0,0,80), dataCol(100,100,100,80), dataCol2(10,200,10,80);
    if(plotType==ptBar) //Il cursore del bar è più chiaro e non trasparente:
        dataCol=QColor(200,200,200);
    QBrush selBrush(selCol), dataBrush(dataCol), dataBrush2(dataCol2);
    painter.setBrush(selBrush);
    if(zoomSelecting)painter.drawRect(stPos.x(), stPos.y(),endPos.x()-stPos.x(), endPos.y()-stPos.y());
    if(dataCursVisible){
        painter.setBrush(dataBrush);
        painter.drawRect(dataCurs);
//        painter.drawRect(debugRect);
    }
    if(dataCurs2Visible){
        painter.setBrush(dataBrush2);
        painter.drawRect(dataCurs2);
    }
    //    QColor varBkgnd(200,200,200,80);
    //    QBrush varBkBrush(varBkgnd);
        painter.setBrush(QBrush(QColor(200,200,200,80)));
        painter.drawRect(hovVarRect);

}

void CLineChart::copy(){
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setImage(*myImage);
    QMessageBox::information(this,"CLineChart","plot copied as an image into the system clipboard.");
}

QImage *  CLineChart::giveImage(){
    //Questa funzione è utile per creare, ad esempio nella realizzazione di spettri di fourier, immaggini contenenti più grafici.
    return myImage;
}


QString CLineChart::makePng(QString fullName, bool issueMsg){
    bool ok=false;
    QString ret="Error";
    QPixmap pixmap;
    pixmap=pixmap.fromImage(*myImage);
    ok=pixmap.save(fullName);
    if(ok)ret="";
    if(issueMsg&&ok)
     QMessageBox::information(this,"CLineChart","PNG image successfully created and saved into file\n"+fullName);
    return ret;
}

QString CLineChart::makeSvg(QString fullName, bool issueMsg){
    QString ret="";
    QSvgGenerator generator;

    generator.setFileName(fullName);
    //I seguenti fattori 0,85 non ci dovrebbero essere. Non è chiaro perché con essi il file SVG viene più centrato.
    generator.setSize(QSize(0.85*geometry().width(), 0.85*geometry().height()));

      myPainter->end();
      myPainter->begin(&generator);

/* La seguente riga è sperimentale, e potrebbe anche essere inutile.
L'idea è di ritoccare dispRect in quanto il font SVG può essere diverso da quello a schermo
Ad es. a schermo uso un 25 point ma il mio paint device ne consente 24; oppure i points sono differenti ma le misure di larghezza di testo sono differenti a causa di diversa grafica del font.
*/
//    scaleXY(dispRect,false);

    plot(false,false);
    markAll();
    myPainter->begin(myImage);
    //Non so perché il grafico a questo punto è scomparso dallo schermo. Lo ritraccio:
    resizeStopped();
    if(issueMsg)
       QMessageBox::information(this,"CLineChart","SVG drawing successfully created and saved into file\n"+fullName);
    update();
    return ret;
}


void CLineChart::mark(void){
    /*Function per il tracciamento di marcatori in corrispondenza dei nomi delle
    variabili e sui grafici in corrispondenza, come ascissa, dell'ascissa di CursorShape.
    */
  mark(true);
}


//---------------------------------------------------------------------------
void CLineChart::mark(bool store){
  /*Function per il tracciamento di marcatori in corrispondenza dei nomi delle variabili e sui grafici in corrispondenza, come ascissa, dell'ascissa di dataCurs[0].
  */
  bool someMark=false;
  int X=dataCurs.x(),iFile,iPlot,iTotPlot=0,iVSFile=0;
  //Traccio i marcatori in corrispondenza dei nomi delle variabili:
  drawMark(markPts[iTotPlot].x(), markPts[iTotPlot].y(), iTotPlot,true);
  if(store)
    if(++manuMarks.lastMark==MAXMANUMARKS){
      manuMarks.lastMark--;
      QApplication::beep();
      return;
    }
    //Traccio i marcatori sulle curve:
    for(iFile=0; iFile<nFiles; iFile++){
      for(iPlot=0; iPlot<nPlots[iFile]; iPlot++)	{
        if(X>=xStartIndex[iFile] && X<xStopIndex[iFile]){
        someMark=true;
        markSingle(iFile, iVSFile, iPlot, iTotPlot, store);
      }else if (store){
        manuMarks.indexSX[iFile][manuMarks.lastMark]=-1;
      }
      iTotPlot++;
    }
    if(filesInfo[iFile].variableStep) iVSFile++;
  }
  if(!someMark)  QApplication::beep();
  update();

}


//---------------------------------------------------------------------------
void CLineChart::markSingle(int iFile, int iVSFile, int iPlot, int iTotPlot, bool store){
    /*Function per il tracciamento di un unico marcatore in corrispondenza, come ascissa,  dell'ascissa di dataCursor.
  E' in sostanza una versione con funzioni più limitate di Mark(bool), ma che consente un accesso più diretto alle operazioni; è più scomoda da usare di Mark(bool) per via dei numerosi parametri passati.
  Parametri passati:
  - iFile è l'indice del file correntemente considerato
  - iVSFile è l'indice del file, se a spaziatura variabile, all'interno dell'elenco dei files a spaziatura variabile
  - iPlot è l'indice del grafico all'interno dell'elenco dei grafici del file iFile
  - iTotPlot è l'indice generale del grafico e serve per scegliere l'aspetto del marcatore
  - Se store è true la posizione del marcatore tracciato è memorizzata in modo da facilitare il ritracciamento in caso, as es. di resize della finestra.
  La memorizzazione della posizione viene effettuata memorizzando l'indice del punto  immediatamente a sinistra del punto del cursore, e poi la frazione dell'intervallo  fra esso e l'indice immediatamente a destra. In tal modo la memorizzazione è indipendente dal Pait del supporto di visualizzazione, e quindi può essere utilizzata senza conversioni per tracciare a stampa i marcatori.
    */
    int X=dataCurs.x(), Y, indexSX, netCursorX;
    float yMinF, yValue, yRatio, fCursorX, slope;
    netCursorX=min(max(X-X0,0),X1-X0);

    fCursorX=xAxis.scaleMin*xAxis.scaleFactor+netCursorX/ratio.x;
  if(filesInfo[iVSFile].variableStep )
    indexSX=max(0,pixelToIndexDX[iVSFile][netCursorX]-1);
  else{
    indexSX=(fCursorX-px[iFile][0])/(px[iFile][1]-px[iFile][0]);
  }

    if(X<xStartIndex[iFile] || X>=xStopIndex[iFile]){
    return;
  }
  if(curveParam[iTotPlot].rightScale){
    yMinF=rYAxis.minF;
    yRatio=ratio.yr;
  }else{
    yMinF=yAxis.minF;
    yRatio=ratio.y;
  }
  slope=(py[iFile][iPlot][indexSX+1]-py[iFile][iPlot][indexSX])/
                  (px[iFile][indexSX+1]-px[iFile][indexSX]); /*Slope=DeltaY/DeltaX:*/
  yValue=py[iFile][iPlot][indexSX]+ slope*(fCursorX-px[iFile][indexSX]);
  Y=Y0+yAxis.width-NearInt(yRatio*(yValue - yMinF));
  if(store){
    manuMarks.indexSX[iFile][manuMarks.lastMark]=indexSX;
    manuMarks.fraction[iFile][manuMarks.lastMark]=
           (fCursorX-px[iFile][indexSX])/(px[iFile][indexSX+1]-px[iFile][indexSX]);
  }
  //Traccio il marcatore sul grafico:
  drawMark(X+1,Y,iTotPlot,false);//il "+1" perché il dataCurs ha larghezza 3 e il valore è nel pixel centrale
}


void CLineChart::markAll(){
  /* Funzione utilizzata per Resize, Print e Copy, per mettere i marcatori sul grafico
     in tutte le posizioni in cui essi sono presenti a schermo.
  */
  int i, iPlot, iMark, iTotPlot=0, X, Y;
  float yMinF, yRatio;

  //La seguente riga serve per evitare che vengano messi i marcatori vicino ai nomi
  //delle variabili se non è da farsi marcatura né manuale né automatica
  if(!autoMark && manuMarks.lastMark==-1)return;
  if(autoMark){
    markAuto();
  }else{
    //Traccio i marcatori vicino ai nomi delle variabili:
    for(i=0; i<nFiles; i++)
      for(iPlot=0; iPlot<nPlots[i]; iPlot++){
        drawMark(markPts[iTotPlot].x(), markPts[iTotPlot].y(), iTotPlot,true);
        iTotPlot++;
      }
    iTotPlot=0;
  }
  yRatio=ratio.y;
  for(i=0; i<nFiles; i++)
  for(iPlot=0; iPlot<nPlots[i]; iPlot++) {
    /* Ora traccio eventuali punti a posizione determinata manualmente.
  La memorizzazione della posizione dei punti è stata effettuata memorizzando l'indice
  del punto immediatamente a sinistra del punto del cursore, e poi la frazione
  dell'intervallo fra esso e l'indice immediatamente a destra. In tal modo la
  memorizzazione è indipendente dalla Canvas del supporto di visualizzazione, e
  quindi può essere utilizzata senza conversioni per tracciare a stampa i marcatori.
    */
    if(curveParam[iTotPlot].rightScale){
      yMinF=rYAxis.minF;
      yRatio=ratio.yr;
    }else{
      yMinF=yAxis.minF;
    }
    if(manuMarks.lastMark>-1)
      for(iMark=0; iMark<=manuMarks.lastMark; iMark++){
        if(manuMarks.indexSX[i][iMark]<startIndex[i] || manuMarks.indexSX[i][iMark]>=stopIndex[i])  continue;
        if(manuMarks.indexSX[i][iMark]<0)  continue;
        int indexSX=manuMarks.indexSX[i][iMark];
        float Slope=(py[i][iPlot][indexSX+1]-py[i][iPlot][indexSX])/
                    (px[i][indexSX+1]-px[i][indexSX]); /*Slope=DeltaY/DeltaX:*/
        float yValue=py[i][iPlot][indexSX] + manuMarks.fraction[i][iMark]*Slope*(px[i][indexSX+1]-px[i][indexSX]);
        float xValue=px[i][indexSX] +
              manuMarks.fraction[i][iMark]*(px[i][indexSX+1]-px[i][indexSX]);
        X=X0+NearInt(ratio.x * (xValue - xAxis.minF));
        Y=Y0+yAxis.width-NearInt(yRatio*(yValue - yMinF));
        drawMark(X,Y,iTotPlot,false);
      }
        iTotPlot++;
    }
}

//---------------------------------------------------------------------------
void CLineChart::markAuto(){
  /* Funzione per mettere i marcatori in posizioni automaticamente fissate:
  Per una data ascissa media, i marcatori	non li metto esattamente in corrispondenza
  della stessa ascissa, per evitare	sovrapposizioni, ma ad ascisse separate
  orizzontalmente di circa 1.5*MarkWidth pixel.
  */
  int i, iPlot, iTotPlot=0, iVSFile=0, indexRange, index, deltaIndex, X, Y;
  float yMinF, yRatio;

  //La seguente riga serve per memorizzare che ci sono i marcatori nelle posizioni
  //automaticamente calcolate, per l'eventuale ritracciamento a seguito di comando
  //di Copy o Print.
  autoMark=true;

  float aux=1./(MAXAUTOMARKS);
  for(i=0; i<nFiles; i++){
    for(iPlot=0; iPlot<nPlots[i]; iPlot++)	{
      //Traccio i marcatori vicino ai nomi delle variabili:
      drawMark(markPts[iTotPlot].x(), markPts[iTotPlot].y(), iTotPlot,true);
      indexRange=stopIndex[i]-startIndex[i];
      if(curveParam[iTotPlot].rightScale){
        yMinF=rYAxis.minF;
        yRatio=ratio.yr;
      }else{
        yMinF=yAxis.minF;
        yRatio=ratio.y;
      }
      //Traccio eventuali marcatori equispaziati in posizioni determinate automaticamente:
      if(filesInfo[i].variableStep  && xVarParam.isMonotonic){
        int xInitial, xFinal, stepX;
        xInitial=X0+NearInt(ratio.x * (px[i][startIndex[i]] - xAxis.minF));
        xFinal  =X0+NearInt(ratio.x * (px[i][stopIndex[i]]  - xAxis.minF));
        stepX=(xFinal-xInitial)/MAXAUTOMARKS;
        X=X0+stepX/2;
        for(int ii=1; ii<=MAXAUTOMARKS; ii++){
          dataCurs.moveLeft(X);
          markSingle(i,iVSFile,iPlot,iTotPlot,false);
          X+=stepX;
        }
      }else{
        for(int ii=1; ii<=MAXAUTOMARKS; ii++){
          if(xVarParam.isMonotonic)
             deltaIndex=max(1,1.8*markHalfWidth/ratio.x/(px[i][1]-px[i][0]));
          else
            deltaIndex=1;
          index=startIndex[i]+aux*(ii-0.5)*indexRange+deltaIndex*(iTotPlot-nFiles/2);
          X=X0+NearInt(ratio.x * (px[i][index] - xAxis.minF));
          Y=Y0+yAxis.width-NearInt(yRatio*(py[i][iPlot][index] - yMinF));
          drawMark(X+1,Y,iTotPlot,false); //il "+1" perché il dataCurs ha larghezza 3 e il valore è nel pixel centrale
        }
      }
      iTotPlot++;
    }
    if(filesInfo[i].variableStep) iVSFile++;
  }
  update();
}


float CLineChart::minus(struct SDigits c, unsigned icifra, unsigned ifrac){
/* Per la spiegazione v. inizio funzione "plus"*/
   float xret;
   switch(icifra) {
      case 3:           //in questo caso ifrac è sempre 1
        break;          // nessuna operazione per questo caso
      case 2:
        switch(ifrac) {
          case 1:
            c.i3=0;
            break;
          case 2:
            if(c.i3<5) c.i3=0;  else     c.i3=5;
            break;
          case 5:
            if(!pari(c.i3)) c.i3--;
            break;
          default:
            puts("Errore 1 in minus");
        }
        break;
      case 1:                   // arrotondamento con una cifra significativa
        c.i3=0;
        switch(ifrac) {
          case 1:
            c.i2=0;
            break;
          case 2:
            if(c.i2<5) c.i2=0;     else     c.i2=5;
          break;
          case 5:
            if(!pari(c.i2)) c.i2--;
            break;
          default:
            puts("Errore 3 in minus");
        }	                              // fine switch di ifrac
        break;
      default:
        puts("Errore 2 in minus");
   }                                    // fine switch di ii
   /* A differenza di plus qui non è necessaria alcuna gestione dei riporti */
   xret=( (float)c.i1+(float)c.i2/10.f+(float)c.i3/100.f)*pow(10.f,(float)c.ie);
   if(c.Sign == '-') xret=-xret;
   return(xret);
}


//---------------------------------------------------------------------------
QString CLineChart::plot(bool autoScale){
  int i,icount,iTotPlot, iRet;
  designPlot();
  if(autoScale){
    bool someLeftScale=false, someRightScale=false;
    /*Gestione eventuale scala destra. La metto se sono verificate due condizioni:
       1. E' richiesto almeno un grafico sulla scala sinistra
       2. E' richiesto almeno un grafico sulla scala destra     */
    twinScale=false;
    iTotPlot=0;
    for(i=0; i<nFiles; i++)
    for(icount=0; icount<nPlots[i]; icount++){
        if(curveParam[iTotPlot].rightScale)
            someRightScale=true;
        else
            someLeftScale=true;
        iTotPlot++;
    }

    if(someRightScale){
      if(someLeftScale)
        twinScale=true;
      else{
     //Se sono state richieste solo scale destre, si forzano tutte a scale sinistre:
        iTotPlot=0;
        for(i=0; i<nFiles; i++)
        for(icount=0; icount<nPlots[i]; icount++){
          curveParam[iTotPlot].rightScale=false;
            iTotPlot++;
        }
      }
    }
    setFullDispRect(); //Mette il risultato in "DispRect"
    exactMatch=false;
    iRet=scaleXY(dispRect,false); //Ritocca il valore di DispRect
  }else
      iRet=scaleXY(dispRect,false);
  if(iRet)return "";
  plotDone=true;
  return plot(false,false);
}
//---------------------------------------------------------------------------
QString CLineChart::plot(bool Virtual, bool /*IncludeFO*/){
/* Se il parametro Virtual=true si fanno tutti i calcoli, curve escluse, ma non si scrive niente con myPainter.
Questo serve quando si fa una stampa: dopo di essa vanno ripristinati tutti i valori corretti di X0, Y0, axisW, markPts, ratio, xStartIndex e xStopIndex, e forse anche qualcos'altro. Tutte queste cose servono per poter fare poi le operazioni di lettura valori tramite cursore, e di marcatura ("Mark").
Allora dopo la stampa è opportuno ripetere il Plot, ma per evitare un inutile ritracciamento delle curve in quell'unico caso pongo noCurves a true. Ovviamente se Virtual è true non andrà neanche effettuata la cancellazione del grafico preesistente per evitare di cancellare le curve preesistenti.
Il parametro includeFO viene passato a writeLegend, la quale viene così informata se deve includere o meno i Factors e Offsets nella leggenda. */

  int i, MaxPlots;
  QString str;
  SFloatRect2 R=dispRect;

  delete startIndex;
  delete stopIndex;
  startIndex=new int[nFiles];
  stopIndex=new int[nFiles];

  if(stopIndex==NULL)return "Unable to allocate variables in TLineChart!";
  myImage->fill(Qt::white);
  if(copying)
    FC.strongFilter=strongFilter;
  else
    FC.strongFilter=false;
  //Valori validi nel caso in cui non si stia facendo una zoomata
  //o si fa una zoomata ma la variabile X non è il tempo:
  for(i=0; i<nFiles; i++){
    startIndex[i]=0;
    stopIndex[i]=filesInfo[i].numOfPoints-1;
  }

  if(PPlotPenWidth!=pwAuto){
    ticPen.setWidth(PPlotPenWidth+1); //se pwThin uso 1 pixel, se pwThick 2
    plotPen.setWidth(PPlotPenWidth+1);
    framePen.setWidth(PPlotPenWidth+1);
  }else{  //Ora sono in spessore penna automatico
    //Si ricordi che il risultato della seguente divisione è troncato, non arrotondato.
    ticPen.setWidth((plotRect.width()+plotRect.height())/600);
    plotPen.setWidth(ticPen.width());
    framePen.setWidth(plotPen.width());
  }

  gridPen.setWidth(1);
  gridPen.setStyle(Qt::DotLine);
  //E' stato verificato che la seguente riga è indispensabile per la scrittura su SVG
  if(makingSVG) myPainter->setFont(QFont(baseFontName,generalFontPx));
  if(writeTitle1){
    titleHeight=fontMetrics().height();
    Y0=titleHeight+textHeight/2;
    QTextOption txtOpt;
    txtOpt.setAlignment(Qt::AlignCenter);
    int addSpace=0;
    if(printing) addSpace=5;
//    QRectF testR1=plotRect, testR2=rect();
    titleRectF=QRectF(0,2,plotRect.width(),titleHeight+addSpace);
//    titleRectF=QRectF(0,2,myImage->width(),titleHeight);
    myPainter->setPen(Qt::black);
    myPainter->drawText(titleRectF, titleText, txtOpt);
  }else{
    Y0=textHeight+1;
  }

  if(FLegend)
    writeLegend(false);
  else
    legendHeight=0;

  myPainter->setPen(framePen);
  //Vertici rettangolo di visualizzazione (X0-X1-Y0-Y1):
  X0=yAxis.maxTextWidth+1.5*ticWidth.y()+smallHSpace;
  if(makingSVG)X0+=svgOffset;
  if(printing)X0+=plotRect.x();
  //note:
  //- con questo X0 si lascia un margine bianco fisso di smallHSpace.
  //- Y0 è già stato calcolato più sopra!

  //  X1=plotRect.width()-max(rYAxis.maxTextWidth+2.0*ticWidth.y()+1.2*smallHSpace, 0.65*xAxis.maxTextWidth);
  X1=plotRect.width()-max(rYAxis.maxTextWidth+ticWidth.y()+3*smallHSpace, xAxis.maxTextWidth);
  if(X1<=X0){
    QMessageBox::critical(this,"","Critical error \"X1<X0\"LineChart\n Program will be stopped");
    qApp->closeAllWindows();
  }
  Y1=plotRect.height()-legendHeight-textHeight-ticWidth.x()-2; //2 pixel fra la tacca e il numero
  Y1-=smallHSpace*1.5; //un piccolo ulteriore spazio per allontanarmi dal bordo
  if(Y1<=Y0){
    QMessageBox::critical(this,"","Critical error \"Y1<Y0\"LineChart\n Program will be stopped");
    qApp->closeAllWindows();
  }

  dataCurs.setRect((X0+X1)/2,Y0,3,Y1-Y0);
  dataCurs2.setRect((X0+X1)*0.6,Y0,3,Y1-Y0);
  debugCurs=dataCurs;

  /*
Tracciamento rettangolo del grafico. Lo faccio trasparente e non a fondo bianco,  perché in tal modo rendo più agevole l'eventuale editazione di un grafico esportato, senza particolari inconvenienti. Nel grafico esportato avrò quindi pieno solo il rettangolo complessivo del grafico, tracciato altrove, ma non quello "netto" contenente gli assi, cioè questo:
*/
  if(!Virtual){
    myPainter->setPen(framePen);
    myPainter->setBrush(Qt::NoBrush);
    myPainter->drawRect(X0,Y0,X1-X0+1,Y1-Y0+1);
  }



  /* Nel caso di zoomata, la generazione del grafico può essere grandemente velocizzata (soprattutto nel caso di molti punti, evidentemente) se, invece di tracciare tutto il grafico e lasciare l'effettuazione della zoomata al taglio che la ClipRgn (o la mia ClipRgn, molto più efficiente di quella di Windows, implementata in CFilterClip) fa delle parti di grafico fuori zoomata, si procede al tracciamento della sola parte di grafico interessata dalla zoomata.
Questo, che si ottiene calcolando i valori di StartIndex e StopIndex relativi all'effettiva finestra del grafico da tracciare, risulta possibile soltanto se la variabile X è monotona crescente rispetto al suo indice, e questo è senz'altro verificato se si tratta della variabile tempo. Questa caratteristica è specificata in CLineChart tramite la variabile booleana xVarParam.isMonotonic.
Se X, oltre che monotona crescente è anche costituita da campioni equispaziati (come nel caso di ATP) il calcolo di startIndex e stopIndex risulta particolarmente veloce.
*/
  if(xVarParam.isMonotonic){
    for(i=0; i<nFiles; i++){
        int nPoints=filesInfo[i].numOfPoints;
        if(filesInfo[i].variableStep){
        int iPoint=0;
        do
          iPoint++;
        while(px[i][iPoint]<R.Left && iPoint<nPoints-1);
        startIndex[i]=iPoint-1;
        do
          iPoint++;
        while(px[i][iPoint]<R.Right && iPoint<nPoints-1);
        stopIndex[i]=iPoint;
      }else{
        startIndex[i]=(xAxis.scaleFactor*xAxis.scaleMin-px[i][0])/(px[i][1]-px[i][0]);
        startIndex[i]=max(0,startIndex[i]);
        startIndex[i]=min(nPoints,startIndex[i]);
        stopIndex[i]= NearInt((xAxis.scaleFactor*xAxis.scaleMax-px[i][0])/ (px[i][1]-px[i][0]))+1;
        stopIndex[i]=max(0,stopIndex[i]);
        stopIndex[i]=min(nPoints-1,stopIndex[i]);
      }
    }
  }

/*****   Preparazione degli assi  *****/
  if(plotType==ptBar)
    margin=max(1,0.42*(X1-X0)/(stopIndex[0]-startIndex[0]+0.84));
  else
    margin=0;
  // ampiezza assi e intertacche
  xAxis.width=X1-X0-2*margin;
  yAxis.width=Y1-Y0;
  if(xAxis.scaleType==stLin)
    ratio.x= (float)xAxis.width / (xAxis.scaleMax-xAxis.scaleMin);
  else
    ratio.x= (float)xAxis.width / (xAxis.eMax-xAxis.eMin);
  if(yAxis.scaleType==stLin)
    ratio.y= (float)yAxis.width / (yAxis.scaleMax-yAxis.scaleMin);
  else
    ratio.y= (float)yAxis.width / (yAxis.eMax-yAxis.eMin);
  ticInterv.x=   xAxis.ticInterval * ratio.x;
  ticInterv.y=   yAxis.ticInterval * ratio.y;
  ratio.x /= xAxis.scaleFactor;
  ratio.y /= yAxis.scaleFactor;
  if(twinScale){
    if(rYAxis.scaleType==stLin)
      ratio.yr= (float)yAxis.width / (rYAxis.scaleMax-rYAxis.scaleMin);
    else
      ratio.yr= (float)yAxis.width / (rYAxis.eMax-rYAxis.eMin);
    ticInterv.yr=  rYAxis.ticInterval * ratio.yr;
    ratio.yr /= rYAxis.scaleFactor;
  }
  if(numOfVSFiles>0){
    DeleteIMatrix(pixelToIndexDX);
    pixelToIndexDX=CreateIMatrix(numOfVSFiles,X1-X0+1);
    if(xAxis.scaleType==stLin)
      fillPixelToIndex(pixelToIndexDX);
    else
      fillPixelToIndexLog(pixelToIndexDX);
  }
  // Tacche, numeri ed eventuale griglia Asse(i) y:
  // iii=myPainter->fontMetrics().width("-0.0008");
  if(!Virtual){
    drawAllLabelsAndGrid(xAxis);
    drawAllLabelsAndGrid(yAxis);
    if(twinScale)  drawAllLabelsAndGrid(rYAxis);
  }
/***** Grafico  *****/
  QElapsedTimer timer;
  timer.start();
  switch (plotType){
    case ptLine:
      if(drawType==dtMC)
        drawCurves(Virtual);
      else if(drawType==dtQtF)
        drawCurvesQtF(Virtual);
      else if(drawType==dtQtI)
        drawCurvesQtI(Virtual);
      else
        drawCurvesPoly(Virtual);
    break;
    case ptBar:
      drawBars();
    break;
    case ptSwarm:
      drawSwarm();
    break;
  }
  drawTimeMs=timer.elapsed();
//  qDebug() << "Drawing time: " << timer.elapsed() << "/ms";
  //Qui alloco lo spazio per i valori numerici da leggere in corrispondenza del cursore dati, anche se l'effettivo assegnamento dei valori avverrà nella routine "giveValues".
  delete cursorXValues;
  delete cursorXValBkp;
  DeleteFMatrix(cursorYValues);
  DeleteFMatrix(cursorYValBkp);
  cursorXValues=new float[nFiles];
  cursorXValBkp=new float[nFiles];

  MaxPlots=0;
  for(i=0; i<nFiles; i++)
    MaxPlots=max(MaxPlots,nPlots[i]);
  cursorYValues=CreateFMatrix(nFiles,MaxPlots);
  cursorYValBkp=CreateFMatrix(nFiles,MaxPlots);
  //sPlotTime=str.number((clock()-t1)/(float)CLK_TCK,'g',3);
  update();
  return NULL;
}


float CLineChart::plus(struct SDigits c, unsigned icifra, unsigned ifrac){
  /* Le funzioni plus e minus servono per trovare un numero "tondo" rispettivamente un po' più grande
  e un po' più piccolo del numero memorizzato dentro la struct cifre passata come primo
  parametro.
  "icifra" è il numero di cifre significative utilizzabili per rappresentare il numero. Più
           grande è icifra, più il risultato "tondo" si avvicinerà al numero di partenza.
  "ifrac" è una specie di indicazione frazionaria dei numero di cifre significative.
          Essa può valere solo 1 2 o 5.
          - Nel primo caso dopo "icifre" cifre significative il numero tondo avrà solo zeri.
          - Con ifrac=2 dopo icifre cifre significative si potrà avere un'unica cifra pari a 5,
            poi solo zeri
          - Con ifrac=5 dopo icifre cifre significative si potrà avere un'unica cifra
            che potrà assumere il valore di 2, 4, 6, o 8.
    */
  int pari(int);
  float xret;
  switch(icifra) {
    case 3:                           // in questo caso ifrac è sempre 1
      if(c.i4==0) goto exit;
      c.i3++;
      break;
    case 2:
      switch(ifrac) {
        case 1:
          if(c.i3+c.i4==0) goto exit;
          c.i3=0; c.i2++;
          break;
        case 2:
          if(c.i4==0 && (c.i3==5||c.i3==0)) goto exit;
          if(c.i3>=5) {
            c.i3=0; c.i2++;
          } else
              c.i3=5;
          break;
        case 5:
          if( c.i4==0 && pari(c.i3) ) goto exit;
          if(pari(c.i3)) c.i3+=2;
            else c.i3++;
          break;
        default:
          puts("errore 1 in plus");
      }                      // fine switch di ifrac
            break;
    case 1:     // arrotondamento con una cifra significativa
                // c.i3 verra' posto a 0 alla fine di questo caso
      switch(ifrac) {
        case 1:
          if(c.i2+c.i3+c.i4==0) goto exit;
          c.i2=0; c.i1++;
          break;
        case 2:
          if(c.i3+c.i4==0 && (c.i2==5||c.i2==0) ) goto exit;
          if(c.i2>=5) {
            c.i2=0; c.i1++;
          } else c.i2=5;
          break;
       case 5:
         if(  c.i3+c.i4==0 && pari(c.i2)  ) goto exit;
         if(pari(c.i2)) c.i2+=2;
           else c.i2++;
         break;
       default:
         puts("Errore 2 in plus");
     }                                  // fine switch di ifrac
     c.i3=0;
     break;
   default:
      puts("Errore 3 in plus");
  }                                     // fine switch di icifra

  /* Ora si sistemano eventuali riporti: */
  if(c.i3==10) {
    c.i3=0; c.i2++;
    if(c.i2==10){
      c.i2=0; c.i1++;
      if(c.i1==10){
        c.i1=1; c.ie++;
      }
    }
  }
  exit:
  xret=( (float)c.i1+(float)c.i2/10.f+(float)c.i3/100.f)*pow(10,(float)c.ie);
  if(c.Sign=='-') xret=-xret;
  return(xret);
}

QString CLineChart::print(QPrinter * printer, bool thinLines){
  /* Funzione per la stampa del plot, tipicamente su supporto cartaceo.
 A differenza di BCB non ho una stampante generale di sistema. viene quindi passato un puntatore a printer
*/
  //Per la stampa userò il normale comando plot(), passando la stampante come output device.
  QString ret="";
  myPainter->end();
  if(myPainter->begin(printer)==false){
    ret="Unable to open the selected printer";
    QMessageBox::critical(this,"",ret);
    return ret;
  }
  QRect prnRect=printer->pageRect();
  if(prnRect.width()<20 || prnRect.height()<20 ){
    ret="Unable to get valid information from the selected printer";
    QMessageBox::critical(this,"",ret);
    return ret;
  }
  //Riduco un poco per avere un certo margine:
  prnRect.setWidth(0.9*prnRect.width());
  prnRect.setHeight(0.9*prnRect.height());
  int xmargin=0.05*prnRect.width(), ymargin=0.05*prnRect.height();
  //Devo modificare printRect per mantenere l'aspect ratio di plotRect:
  if(prnRect.height()/prnRect.width() > aspectRatio){
    plotRect=QRect(xmargin,ymargin, prnRect.width(), aspectRatio*prnRect.width());
  }else{
    if(prnRect.height()/aspectRatio>prnRect.width())
      plotRect=QRect(xmargin,ymargin, prnRect.width(), aspectRatio*prnRect.width());
    else
      plotRect=QRect(xmargin,ymargin, prnRect.height()/aspectRatio, prnRect.height());
  }
  printing=true;
  designPlot();
  EPlotPenWidth oldPenWidth=PPlotPenWidth;
  if(thinLines)
    PPlotPenWidth=pwThin;
  else
    PPlotPenWidth=pwThick;
  plot(false,false);
  markAll();
  myPainter->end();
  PPlotPenWidth=oldPenWidth;
  plotRect=geometry();
  printing=false;
  blackWhite=false;
  //Non so perché il grafico a questo punto è scomparso dallo schermo. Lo ritraccio:
  resizeStopped();
  update();
//  ret="aaa";
  return ret;
}

void CLineChart::resetMarkData(){
    autoMark=false;
    manuMarks.lastMark=-1;
}

void CLineChart::resizeEvent(QResizeEvent * ){
/*La prima chiamata di resizeEvent avviene subito prima dello showEvent, e devo effettuare le operazioni usuali di PlotXY, ora trasferite in resizeStopped.
Nelle chiamate successive, durante un vero resize, occorre evitare che per grafici lunghi la reattività del programma sia insufficiente. Allora se i grafici sono lunghi l'aggiornamento del plot verrà fatto solo alla fine del resize, quando il timeout di myTimerattiverà resizeStopped.
Nella versione finale, pertanto, il timeout di myTimer sarà 0 per i grafici veloci (come verrà misurato alla prima esecuzione del comando plot()); sarà invece 100 ms per i grafici lenti, che verranno quindi tracciati solo alla fine del resize. */
  int timeout=100;
    if(plotType==ptBar)
      timeout=10;
    if(isVisible())
        myTimer->start(timeout);
    else
        resizeStopped();
}


void CLineChart::resizeStopped(){
  static QRect oldRect;
  plotRect=geometry();
  QRect r=plotRect;
  if(r.width()<50||r.height()<50)    return;
  delete myPainter;
  delete myImage;
  aspectRatio=(float)r.height()/r.width();

  myImage= new QImage(r.width(),r.height(),QImage::Format_RGB32);

  myPainter=new QPainter(myImage);
  if(dataGot){
    designPlot();
    scaleXY(dispRect,false); //Ritocca il valore di DispRect
    plot(false,false);
  }
  markAll();
  oldRect=r;
}


int CLineChart::scaleAxis(SAxis &myAxis, float minVal, float maxVal, int minTic,
  unsigned include0, bool exactMatch)  {
/* Funzione per determinare la scala di un dato asse.
1) Nel caso di scala logaritmica la funzione determina soltanto i valori minimo e massimo
   delle potenze di 10 da includere nella scala.
2) Nel caso di scala lineare la funzione determina, in particolare:
   - il valori minimo e massimo dell'asse, scelti in modo che siano dei numeri "rotondi"
   - il numero di tacche da mettere e i valori numerici da mettere sulle tacche.
NOTE:
  . Se il grafico è costante e non nullo su tutto il range (minVal=maxVal!=0),
    lo 0 è forzato nella scala;
  . se minVal=maxVal!=0, la scala è convenzionalmente fissata fra -1 e 1.
  . per scale logaritmiche minVal e maxVal devono essere positivi

Significato delle variabili passate (solo fino a maxVal sono usate per scale logaritmiche):
- myAxis è l'asse i cui attributi vanno determinati
- minVal e MaxVal sono il valore minimo e massimo dei punti che devono essere raffigurati
  nella finestra individuata dalla scala che si vuole determinare.
  Il valore minimo della scala, scaleMin, sarà un numero "tondo" inferiore o uguale a
  minVal; analogamente scaleMax sarà un numero tondo superiore o uguale a MaxVal.
- minTic è il numero minimo di tacche sull'asse. Il corrispondente massimo è
  minTic+3. Ad es. all'avviamento il numero di tacche può essere compreso fra 4 e 7.
    Se il grafico diventa molto piccolo minTic si riduce di conseguenza.
- "include0" (intero senza segno) indica la percentuale di Include0zione
    richiesta della presenza dello 0 nella scala:
    - con include0 = 0 non ho alcuna forzatura;
    - con include0 = 100 lo zero è sempre in scala;
    - con include0 = p   lo zero è incluso in scala purché la percentuale di vuoti che ciò comporta non superi p
- exactMatch se è true disabilita la richiesta di estremi di scala "rotondi".
  Pertanto, in tal caso la funzione calcola solo le tacche e il fattore di scala senza
  preventivamente arrotondare minVal e maxVal.
****** CODICI DI RITORNO Ret
- ret=0 se è tutto OK
- ret=1 se è richiesta una scala log con valori della grandezza sul grafico non tutti positivi.
*/
  int i, aux, icifra,ifrac,ntic,
          iermx,   //digit di esponente del valore arrotondato di Max
          iermn,   //digit di esponente del valore arrotondato di Min
          irmn2;   //secondo digit del valore arrotondato Min
  float Ratio, scaleFactor, roundRange, minVal1, maxVal1, fAux;
  char buffer[12];
  QString msg;
  SDigits Min, Max;
  float (*pmax) (struct SDigits, unsigned, unsigned);
  float	(*pmin)(struct SDigits, unsigned,unsigned);

        myAxis.done=0;
  /**** Parte relativa al caso di scale logaritmiche ****/
  if(myAxis.scaleType!=stLin){
    if(maxVal<=0 || minVal<=0){
      if(myAxis.type==atX)
        msg=tr("Cannot create a log scale on the x-axis\n"
               "because the range contains null or negative values\n\n"
            "Please change the variable to be plot, or the x-axis range or choose a linear scale");
      else
        msg=tr("Cannot create a log scale on the y-axis\n"
                "because the range contains null or negative values\n\n"
            "Please change the variable to be plot, or the y-axis range or choose a linear scale");
        QMessageBox::critical(this, tr("MC's PlotXWin"),msg,QMessageBox::Ok);
        return 1;
    }
    sprintf(buffer,"%+10.3e",minVal);
    sscanf(buffer+7, "%u", &myAxis.eMin);
    sprintf(buffer,"%+10.3e",maxVal);
    sscanf(buffer+7, "%u", &aux);
    /* Se il numero MaxVal non è una potenza di 10 eMax=Aux+1, altrimenti eMax=Aux*/
    buffer[6]=0;
    if(strcmp(buffer,"+1.000")==0)
      myAxis.eMax=aux;
    else
      myAxis.eMax=aux+1;
    myAxis.scaleFactor=1.;
    myAxis.scaleMin=pow(10.,myAxis.eMin);
    myAxis.scaleMax=pow(10.,myAxis.eMax);
    if(myAxis.scaleType==stDB){
      sprintf(buffer,"%d",20*myAxis.eMin);
      myAxis.maxTextWidth=myPainter->fontMetrics().width(buffer);
      sprintf(buffer,"%d",20*myAxis.eMax);
      myAxis.maxTextWidth=max(myAxis.maxTextWidth, myPainter->fontMetrics().width(buffer));
    }else{// myAxis.ScaleType=stLog
        aux=myPainter->fontMetrics().width("10");
      sprintf(buffer,"%d",myAxis.eMin);
      myPainter->setFont(expFont);
      myAxis.maxTextWidth=aux+myPainter->fontMetrics().width(buffer);
      sprintf(buffer,"%d",myAxis.eMax);
      aux+=myPainter->fontMetrics().width(buffer);
      myAxis.maxTextWidth=max(myAxis.maxTextWidth,aux);
      myPainter->setFont(numFont);
    }
    myAxis.done=1;
    myAxis.minF=myAxis.scaleMin;
    myAxis.maxF=myAxis.scaleMax;
    return 0;
  }

  /**** Ora la parte, ben più complessa, relativa al caso di scale lineari ***/
  myAxis.halfTicNum=false;
  //Elimino da minVal e maxVal quello che c'è oltre le prime 4 cifre significative:
  sprintf(buffer,"%+10.3e",maxVal);	sscanf(buffer,"%f", &maxVal1);
  sprintf(buffer,"%+10.3e",minVal);	sscanf(buffer,"%f", &minVal1);
  //gestisco il caso particolare di estremi, arrotondati alle prime quattro cifre significative, uguali:
  if(minVal1==maxVal1){
    if(minVal1==0){
      myAxis.scaleFactor=1;
      myAxis.ticInterval=1;
      myAxis.ticDecimals=0;
      myAxis.scaleMax=1;
      myAxis.scaleMin=-1;
    }
    if(minVal1>0){
      myAxis.scaleMin=0;
      myAxis.scaleMax=2*minVal1;
      myAxis.ticInterval=minVal1;
    }
    if(minVal1<0){
      myAxis.scaleMin=2*minVal1;
      myAxis.scaleMax=0;
      myAxis.ticInterval=-minVal1;
    }
    sprintf(buffer,"%+10.3e",minVal1);
    sscanf(buffer+7, "%3u", &iermx);
    myAxis.done=0;
    Max.roundValue=Min.roundValue=minVal;
    sprintf(buffer,"%+10.3e",Max.roundValue);
    sscanf(buffer+7, "%3u", &iermx);
    iermn=iermx;
    goto ComputeScaleFactor;
  }

  beginRounding(Min, Max, minVal, maxVal, include0);
  if(exactMatch){
    Min.roundValue=minVal;
    Max.roundValue=maxVal;
    myAxis.scaleMin=minVal;
    myAxis.scaleMax=maxVal;
    if(minVal==maxVal){
      myAxis.done=0;
    }else if(minVal==0 || maxVal==0){
      myAxis.done=2;
    }else{
      fAux=fabs((minVal-maxVal)/maxVal);
      if(fAux<0.0001)
        myAxis.done=1;
      else
        myAxis.done=2;
    }
  }else{
    /* Se Min.Value e Max.Value hanno segni opposti vanno entrambi arrotondati con la funzione "plus"; se hanno lo stesso segno quello minore in valore assoluto verrà arrotondato usando la funzione "minus" l'altro con "plus": */
    if(Max.Value*Min.Value<=0) {
      pmax=plus;
      pmin=plus;
    }else if(fabs(Max.Value) >= fabs(Min.Value)){
       pmax=plus;
       pmin=minus;
    }else{
      pmax=minus;
      pmin=plus;
    }
    myAxis.done=1;
    for(icifra=1; icifra<=3; icifra++){
      float f1, f2;
//      float f3;
      f1=Max.Value*pow(10,-(float)Max.ie);
      f2=Min.Value*pow(10,-(float)Min.ie);
//      f3=fabs(f1-f2-1.2);
      //caso particolarissimo da trattare separatamente da tutti gli altri:
      if( fabs(f1-f2-1.2)<1e-5  &&  Max.i3==0 && Max.i4==0){
        Max.roundValue=Max.Value;
        Min.roundValue=Min.Value;
        myAxis.done = 2;
        break;
      }
      ifrac=1;
      Max.roundValue=pmax(Max,icifra,ifrac);
      Min.roundValue=pmin(Min,icifra,ifrac);
      roundRange=Max.roundValue-Min.roundValue;
      if(roundRange!=0) {
        Ratio=(Max.Value-Min.Value) / roundRange;
        if(Ratio >= RATIOLIM) {myAxis.done = 2; break;}
      } else break;
      ifrac=2;
      Max.roundValue=pmax(Max,icifra,ifrac);
      Min.roundValue=pmin(Min,icifra,ifrac);
      roundRange=Max.roundValue-Min.roundValue;
      if(roundRange!=0) {
        Ratio=(Max.Value-Min.Value) / roundRange;
        if(Ratio >= RATIOLIM) {myAxis.done = 2; break;}
      } else break;
      ifrac=5;
      Max.roundValue=pmax(Max,icifra,ifrac);
      Min.roundValue=pmin(Min,icifra,ifrac);
      roundRange=Max.roundValue-Min.roundValue;
      if(roundRange!=0) {
        Ratio=(Max.Value-Min.Value) / roundRange;
        if(Ratio >= RATIOLIM) {myAxis.done = 2; break;}
      } else
        break;
    }
    if(myAxis.done != 2) {
      Max.roundValue=Max.Value;
      Min.roundValue=Min.Value;
    }
    myAxis.scaleMin = Min.roundValue;
    myAxis.scaleMax = Max.roundValue;
  }
  /* Calcolo del numero di tacche (sempre fra MinTic e MinTic+3)*/
  sprintf(buffer,"%+10.3e",Max.roundValue);
  sscanf(buffer+7, "%u", &iermx);
  sprintf(buffer,"%+10.3e",Min.roundValue);
  sscanf(buffer+7, "%u", &iermn);
  if(myAxis.done != 2){
    ntic=minTic+1;
  }else{
    //Il seguente calcolo di irmn2 verrà sfruttato per il calcolo del numero di decimali sulle tacche
    sprintf(buffer,"%+10.3e",Min.roundValue);
    sscanf(buffer+3, "%1u", &irmn2);
    ntic=computeTic(Min.roundValue,Max.roundValue,minTic);
    if(ntic==minTic)myAxis.halfTicNum=true;
  }
  /* Distanza tra le tacche: */
  myAxis.ticInterval= (myAxis.scaleMax-myAxis.scaleMin) / ((float)ntic+1);
  if(myAxis.halfTicNum) myAxis.ticInterval/=2;

  /* L'algoritmo fin qui attivato non si comporta male. Il suo principale difetto è che spesso allarga gli estremi della scala, sempre con il vincolo di un riempimento  non inferiore all'80% quando non sarebbe necessario. Ad es. se gli estremi di una variabile sono 0-35, l'algoritmo calcola 0-40 con ntic=7.
 Pertanto qui effettuo un correttivo:  se il grafico ci sta tra la prima tacca e l'estremo destro, ovvero fra l'estremo sinistro e l'ultima tacca, allora il numero di tacche è ridotto di uno e la tacca rispettivamente sinistra o destra diventa l'estremo sinistro o destro.
 */
  if(include0==0 && myAxis.done==2 && !myAxis.halfTicNum){
    if(minVal>0.9999*(myAxis.scaleMin+myAxis.ticInterval)){
      myAxis.scaleMin=myAxis.scaleMin+myAxis.ticInterval;
      ntic--;
      myAxis.ticInterval = (myAxis.scaleMax-myAxis.scaleMin) / ((float)ntic+1);
    }else if(maxVal<1.0001*(myAxis.scaleMax-myAxis.ticInterval)){
      myAxis.scaleMax=myAxis.scaleMax-myAxis.ticInterval;
      ntic--;
      myAxis.ticInterval = (myAxis.scaleMax-myAxis.scaleMin) / ((float)ntic+1);
    }
  }
ComputeScaleFactor:
  /* Calcolo di eventuale fattore di scala:  */
  aux=iermx;
  if(Max.roundValue==0)
    aux=iermn;
  else if (iermn>iermx && Min.roundValue!=0)
    aux=iermn;
  if(aux>-2 && aux<4)
     myAxis.scaleExponent=0;
  else
    myAxis.scaleExponent=(aux-(aux<0))/3*3;
  scaleFactor=pow(10.0f,(float)myAxis.scaleExponent);
  myAxis.scaleFactor=scaleFactor;
  myAxis.scaleMax/=scaleFactor;
  myAxis.scaleMin/=scaleFactor;
  myAxis.ticInterval/=scaleFactor;

  // Calcolo decimali da scrivere sulle etichette numeriche:
  myAxis.ticDecimals=computeDecimals(myAxis.scaleMin,myAxis.ticInterval,myAxis.halfTicNum);


  /* Calcolo  della larghezza massima delle etichette dell'asse, se di tipo LY o RY  */
//  painter->setFont(QFont(fontName, exponentFontSize));
  float yf, yy, YY;
  char num[10], format[5]="%.1f";
  yf=myAxis.scaleMin;
  sprintf(format+2,"%1df",max(myAxis.ticDecimals,0));
  sprintf(num,format,myAxis.scaleMax);
  sscanf(num,"%f",&YY);
  if(myAxis.done==0){
    aux=myPainter->fontMetrics().width(num);
    sprintf(num,format,myAxis.scaleMin);
    aux=max(aux,myPainter->fontMetrics().width(num));
    goto Return;
  }
  aux=0;
  i=0;
  myPainter->setFont(numFont);
  do{
/*Il seguente numero 10 è dovuto al fatto che al massimo ci possono essere 7 ticmarks con label numeriche, il che implica 8 spazi fra una ticmark con label  e l'altra.
  Peraltro si hanno casi particolari in cui si raggiunge 10. Un esempio è quando i due estremi degli assi sono estremamente vicini, come nel file "Motori3" var. u2:TQGEN e scelta manuale degli assi con   Ymin=-17235  Ymax=-17234
*/
    if(++i==10){
//      QMessageBox::information(this, tr("TestLineChart"),  tr("Sorry, cannot find better scales for these axes!"), QMessageBox::Ok,QMessageBox::Ok);
        break;
    }
    sprintf(format+2,"%1df",max(myAxis.ticDecimals,0));
    sprintf(num,format,yf);
    aux=max(aux,myPainter->fontMetrics().width(num));
    yf+=myAxis.ticInterval*(1+myAxis.halfTicNum);
    sprintf(num,format,yf);
    sscanf(num,"%f",&yy);
  }while(yy<=YY);
 Return:
/*A questo punto ho messo in Aux la massima ampiezza delle etichette numeriche.
  Va ora calcolata maxTextWidth tenendo conto dell'ampiezza della eventuale label di asse.
  Si ricorda che writeAxisLabel con l'ultimo parametro true anziché scrivere calcola solo la lunghezza della label; il termine dipendente da ticWidth.y è conseguenza del fatto che la label, non essendo in corrispondenza dei ticWidth, viene accostata un po' più vicino al rettangolo del grafico dei valori numerici.
   */
//  int iii=myPainter->fontMetrics().width("-0.0008");
  if(myAxis.type==atX)
    myAxis.maxTextWidth=aux;
  else if(myAxis.type==atYL)
    myAxis.maxTextWidth=max(aux,writeAxisLabel(0,0,myAxis,true)-ticWidth.y()* !myAxis.halfTicNum);
  else if(myAxis.type==atYR)
    myAxis.maxTextWidth=max(aux,writeAxisLabel(0,0,myAxis,true));
  else{
    QMessageBox::critical(this, tr("TestLineChart"),
         tr("Unexpected Error \"AxisType\"\n"
                "Please, contact program maintenance"),
                 QMessageBox::Ok,QMessageBox::Ok);
    qApp->closeAllWindows();
  }
  myAxis.minF=myAxis.scaleMin*myAxis.scaleFactor;
  myAxis.maxF=myAxis.scaleMax*myAxis.scaleFactor;
  return 0;
}



//---------------------------------------------------------------------------
int CLineChart::scaleXY(SFloatRect2 R, bool JustTic){
/* Questa funzione serve per fare i calcoli delle scale sui due o tre assi.
Riceve in ingresso una versione "grezza" di DispRect, con numeri non ancora tondi, e ne calcola una versione rifinita.

DA FARE A BREVE :
Il JustTic deve essere definito come const.

  Durante l'esecuzione di un resize viene ugualmente cambiata, solo per ricalcolare il numero delle tacche (JustTic=true).  Per ragioni di semplicità implementativa non propago la gestione di JustTic anche a
  Scala Asse, anche se faccio fare un po' di calcoli inutili al programma.
  ***** CODICI DI RITORNO Ret
  - Ret =0 se è tutto OK
  - Ret =1 se scaleAxis ha rilevato un problema (attualmente solo scala log assieme a valori non tutti positivi della grandezza)
*/
  unsigned Include0=100*forceYZero;
  int ret;

  if(!JustTic){
    ret =scaleAxis(xAxis, R.Left, R.Right, minXTic, Include0, exactMatch);
    ret+=scaleAxis(yAxis, R.LBottom, R.LTop, minYTic, Include0, exactMatch);
    dispRect.Left=xAxis.scaleMin*xAxis.scaleFactor;
    dispRect.Right=xAxis.scaleMax*xAxis.scaleFactor;
    dispRect.LBottom=yAxis.scaleMin*yAxis.scaleFactor;
    dispRect.LTop=yAxis.scaleMax*yAxis.scaleFactor;
  }else{
    ret =scaleAxis(xAxis, R.Left, R.Right, minXTic, Include0, true);
    ret+=scaleAxis(yAxis, R.LBottom, R.LTop, minYTic, Include0, true);
  }
  if(ret)return ret;
    if(twinScale){
    if(!JustTic){
      scaleAxis(rYAxis, R.RBottom, R.RTop, minYTic, Include0, exactMatch);
        dispRect.RBottom=rYAxis.scaleMin*rYAxis.scaleFactor;
        dispRect.RTop=rYAxis.scaleMax*rYAxis.scaleFactor;
    }else{
      scaleAxis(rYAxis, R.RBottom, R.RTop, minYTic, Include0, true);
    }
  }else
    rYAxis.maxTextWidth=0;
    return ret;
}

void CLineChart::setActiveDataCurs(int setCurs){
  SXYValues values;
  switch(setCurs){
    case 0:
      dataCursVisible=false;
      dataCurs2Visible=false;
      break;
    case 1:
      dataCursVisible=true;
      dataCurs2Visible=false;
      if(dataGot)
        giveValues(dataCurs.x()+1, FLinearInterpolation, false, false);
      break;
    case 2:
      if(!dataCursVisible)return;
      dataCurs2Visible=true;
      if(!dataGot)break;
      values=giveValues(dataCurs.x()+1, FLinearInterpolation, false, false);
      //Prendo i valori con differenze orizzontali e verticali rispetto alla posizione del cursore
      //primario:
      values=giveValues(dataCurs2.x()+1, FLinearInterpolation, true, true);
      emit valuesChanged(values,true,true);
      break;
    }
    update();
}
void CLineChart::setDispRect(SFloatRect2 rect){
     dispRect=rect;
}


QString CLineChart::setFullDispRect(){
  bool RmMInitialised=false, LmMInitialised=false;
    int i;
    struct SMinMax AuxmM;
//    char *Msg="Unexpected Error \"Not Got Data\" in TLineChart";
    static char Msg[]="Unexpected Error \"Not Got Data\" in TLineChart";
    if(!dataGot){
        QMessageBox::critical(this, tr("TestLineChart"), tr("Unexpected Error"), QMessageBox::Ok);
        qApp->closeAllWindows();
        //Riga che serve solo per superare il check sintattico:
        return Msg;
    }

    xmM=findMinMax(px[0],filesInfo[0].numOfPoints);
    for(i=1; i<nFiles; i++){
        AuxmM=findMinMax(px[i],filesInfo[i].numOfPoints);
        xmM.Min=min(xmM.Min,AuxmM.Min);
        xmM.Max=max(xmM.Max,AuxmM.Max);
    }

    int iTotPlot=0;
    for(i=0; i<nFiles; i++){
      for(int iPlot=0; iPlot<nPlots[i]; iPlot++){
        AuxmM=findMinMax(py[i][iPlot],filesInfo[i].numOfPoints);
        if(curveParam[iTotPlot].rightScale){
            if(RmMInitialised){
                RymM.Min=min(RymM.Min,AuxmM.Min);
                RymM.Max=max(RymM.Max,AuxmM.Max);
            }else{
                RymM=AuxmM;
                RmMInitialised=true;
            }
        }else{
            if(LmMInitialised){
                LymM.Min=min(LymM.Min,AuxmM.Min);
                LymM.Max=max(LymM.Max,AuxmM.Max);
            }else{
                LymM=AuxmM;
                LmMInitialised=true;
            }
        }
        iTotPlot++;
      }
    }

  dispRect.Left=xmM.Min;
  dispRect.Right=xmM.Max;
  dispRect.LTop=LymM.Max;
  dispRect.LBottom=LymM.Min;
  dispRect.RTop=RymM.Max;
  dispRect.RBottom=RymM.Min;
  return "";
}


void CLineChart::setPlotPenWidth(EPlotPenWidth myWidth){
  PPlotPenWidth=myWidth;
  /* In linea di massima si potrebbe pensare di attribuire qui PPlotPenWidth a plotPen, ticPen, framePen. Il problema è che il valore passato non è sempre un numero fisso: nel caso sia pwAuto, dipenderà  dalle dimensioni della finestra. Di conseguenza sarebbe abbastanza inutile.
Pertanto l'attribuzione dei valori effettivi degli spessori delle penne sarà  fatta all'inizio della funzione plot(bool, bool). */
}

void CLineChart::setXZeroLine(bool zeroLine_){
    xAxis.addZeroLine=zeroLine_;
}

void CLineChart::setYZeroLine(bool yZeroLine_){
    yAxis.addZeroLine=yZeroLine_;
    rYAxis.addZeroLine=yZeroLine_;
}


void CLineChart::setXScaleType(enum EScaleType xScaleType_){
    xAxis.scaleType=xScaleType_;
}

void CLineChart::setYScaleType(enum EScaleType yScaleType_){
    yAxis.scaleType=yScaleType_;
}

//---------------------------------------------------------------------------
int CLineChart::writeAxisLabel(int X, int Y, SAxis &axis, bool _virtual ){
/* Funzione che scrive l' "etichetta di asse" su un asse.
  Si tratta di un'etichetta che può contenere una potenza di 10 o, fra parentesi, l'unità di misura.
  Se LabelOverride==true, la label scritta è quella richiesta dall'utente dell'oggetto CLineChart, e preesistente nella variabile corrispondente all'asse considerato, fra le xUserLabel, yUserLabel,  ryUserLabel.
  Se labelOverride==false, allora:
  - se autoLabelXY==false vengono messe nel grafico solo potenze di 10 quando necessario.
  - se autoLabelXY==true  vengono automaticamente riconosciute le unità di misura a partire dalla prima lettera del nome,  ad es. v->V, c->A, p->W, ecc. In tal caso si ha una scrittura letterale anche se sono necessarie potenze di 10, ad es. t(ms), (kV) ecc.
 Se è Virtual=true, l'effettiva scrittura non viene fatta ma viene solo calcolata l'ampiezza della label, che viene ritornata dalla funzione.
 Se Virtual=false il valore di ritorno è indeterminato.
 */
  char prefix[]={'p','n','u','m','0','k','M','G','T'};
  SUserLabel userLbl;
  int iTotPlot;
  EadjustType hAdjust, vAdjust;
  QString txt;
  userLbl.B="";
  userLbl.E="";

  QString unit=""; //E' l'unità di misura dell'asse corrente.
  if(axis.type==atX)
    unit=xVarParam.unitS;
  else
    for(iTotPlot=0; iTotPlot<numOfTotPlots; iTotPlot++){
      //Se una curva non va graficata sull'asse corrente non la considero:
      if(curveParam[iTotPlot].rightScale  && axis.type!=atYR) continue;
      if(!curveParam[iTotPlot].rightScale && axis.type==atYR) continue;
      //Se la variabile corrente ha unità indeterminata l'asse non potrà avere label testuale:
      if(curveParam[iTotPlot].unitS==""){
        unit="";
        break;
      }
      // Tutte le variabili sul medesimo asse devono avere la stessa unità. Pertanto una volta definita la unit per una variabile dell'asse corrente anche le altre devono avere la stessa oppure non metto niente sull'asse:
      if(unit==""){
        unit=curveParam[iTotPlot].unitS;
      }else if(unit!=curveParam[iTotPlot].unitS){
        unit="";
        break;
      }
    }

  switch(axis.type){
    case atX:
      userLbl=xUserLabel;
      hAdjust=atCenter;
      vAdjust=atLeft;
      break;
    case atYL:
      userLbl=yUserLabel;
      hAdjust=atRight;
      vAdjust=atCenter;
      break;
    case atYR:
      userLbl=ryUserLabel;
      hAdjust=atLeft;
      vAdjust=atCenter;
      break;
    default:  //Serve solo per evitare un warnining sull'inizializzazione di hAdjust e vAdjust
      userLbl=xUserLabel;
      hAdjust=atCenter;
      vAdjust=atLeft;
      break;
  }

  // Il caso di label manualmente specificata dall'utente di CLineChart è di gran lunga il più semplice e lo analizzo per primo:
  if(labelOverride)
      goto Write;

  //Ora la label determinata da questa routine, in maniera dipendente da autoLabelXY
  if(axis.scaleType==stDB){
    userLbl.B="dB";
    userLbl.E="";
    goto Write;
  }
  if(axis.scaleType==stLog){
    userLbl.B="";
    userLbl.E="";
    goto Write;
  }

/* Calcolo nel caso di scala lineare.
    Il tracciamento avviene come potenza di 10 se l'Autolabel generale del grafico è false
    ovvero si verifica una delle condizioni:
    - esistono variabili la cui unità  di misura è '\0' (=intederminata)
    - ho variabili di tipi diversi da plottare sul medesimo asse verticale
    - ho variabili di tipi diversi da plottare sul medesimo asse orizzontale (ad es. tempo e frequenza)
    - esistono variabili che sono talmente grandi o piccole da non poter essere tracciate
      con autolabel per mancanza di prefix adeguato
    - label di asse x e plot del tipo X-Y
  */
  if(!autoLabelXY || abs(axis.scaleExponent)>12 || (axis.type==atX && !xVarParam.isMonotonic) ){
    if(axis.scaleFactor==1){
      userLbl.B="";
      userLbl.E="";
    } else {
      userLbl.B="*10";
      userLbl.E=QString::number(axis.scaleExponent);
    }
    goto Write;
  }

  // A questo punto è determinato se unit è "" (label in potenza di 10 e solo quando necessario). ovvero !"" (label testuale).
  // Posso fissare base ed esponente della label nei due casi.
  if(unit==""){
    if(axis.scaleFactor==1){
      userLbl.B="";
      userLbl.E="";
    } else {
      userLbl.B="*10";
      userLbl.E=QString::number(axis.scaleExponent);
    }
  }else{
    txt="";
    if(prefix[axis.scaleExponent/3+4]!='0')
      txt.append(prefix[axis.scaleExponent/3+4]);
    txt.append(unit);
    userLbl.B=txt;
    userLbl.E="";
  }

Write:
  // La gestione delle parentesi la faccio qui, in modo che agisce sia nel caso di labels automatiche che specificate manualmente dall'utente
  if(axis.type==atX) Y+=1; //Ago 2012
  if(axis.type==atYL) X-=1; //May 2014
  if(axis.type==atYR) X+=3; //May 2014
  return drawText2(X,Y,hAdjust,vAdjust,userLbl.B,userLbl.E,_virtual, useBrackets)+1;
}

void  CLineChart::writeLegend(bool _virtual){

/* Quando _virtual=true la routine fa solo il calcolo di legendHeight e blocksPerRow. In caso contrario fa la scrittura.
_virtual=Questo artificio è molto utile per evitare di creare una routine indipendente per il calcolo delle altezze, con la possibilità sempre presente di un disallineamento con la routine di effettiva scrittura.
Esso è usato solo quando il file è unico, in quanto nella modalità a più files viene comunque fatto in due passate perché altrimenti il codice risultava di difficile scrittura.
Però mantenere le due passate all'interno di questo unico file aiuta molto.

La presente routine viene chiamata con _virtual=true solo da sé stessa, all'inizio delle attività, per l'appunto per calcolare le altezze, e quindi la posizione in cui scrivere.
Poi effettua la scrittura.
Il software esterno chiama prima writeLegend delle routine di esecuzione del grafico in modo che la legenda è già scritta in basso quando si fa il grafico, e lo spazio che occupa è quindi già noto.

La leggenda ha aspetto differente a seconda che si tratti di un grafico
le cui variabili provengono da diversi files, o da un unico file.

1) variabili provenienti da diversi files.
   In questo caso creo dei blocchi cosituiti dal nome del file e dalle relative variabili da plottare. Se in una riga ci stanno più blocchi ce li metto. In tal modo se ho molte variabili per file verrà un file per riga, altrimenti anche più files su una medesima riga.
Questo consente di sfruttare bene le tre righe disponibili per la leggenda sia nel caso in cui ho molti files e poche variabili per file che nel caso opposto di pochi files, ciascuno conmolte variabili.
I blocchi vengono calcolati assieme a legendHeight alla prima passata.
la variabile x è a comune ed è la variabile tempo e non è necessario riportarne il nome in leggenda. La leggenda contiene un paragrafo per ogni file riportante prima i nome del file, poi il carattere ':', poi l'elencazione delle variabili, eventualmente su più righe.

2) variabili provenienti tutte da unico file. In tal caso si potrebbe anche trattare di grafico di tipo XY. La leggenda contiene all'inizio, fra parentesi, informazione sul nome del file e della variabile x poi, dopo il carattere ':'. l'elenco delle variabili.

PER ENTRAMBI I CASI limito comunque la leggenda ad un massimo di 3 righe, rinunciando a riportare alcune variabili se questo spazio è insufficiente. L'utente avrà comunque la possibilità di avere informazione completa sui grafici visualizzati attivando la visione dei valori numerici e quindi la relativa leggenda.
 */
  int iTotPlot=-1;
  int numRows, xPosition=plotRect.left(), yPosition;
  int markerXPos, markerYPos;

  QString msg;

  hovDataLst.clear();
   if(!_virtual){
    writeLegend(true); //mette il valore in legendHeight;
  }
  lgdFont.setUnderline(false);
  myPainter->setFont(lgdFont);
  if(_virtual)
    yPosition=0;
  else
    yPosition=plotRect.y()+plotRect.height()-legendHeight+textHeight;
  if(legendHeight==-1)return;
  numRows=1;
  //Nel seguente caso di nFiles=1 il medesimo codice serve per il calcolo delle dimensioni della leggenda e l'effettiva scrittura:
  if(nFiles==1){
    myPainter->setPen(Qt::black);
    msg="(file "+filesInfo[0].name + "; x-var "+xVarParam.name+")  ";
    if(!_virtual)myPainter->drawText(xPosition,yPosition,msg);
    xPosition+=myPainter->fontMetrics().width(msg);
    while(1){
      iTotPlot++;
      if(iTotPlot==numOfTotPlots)    break;
      msg=curveParam[iTotPlot].name+"   ";
      if(curveParam[iTotPlot].rightScale)
       lgdFont.setUnderline(true);
      else
        lgdFont.setUnderline(false);
      myPainter->setFont(lgdFont);
      if(!blackWhite)
        myPainter->setPen(curveParam[iTotPlot].color);
      //Decido se devo andare a capo sulla base della stringa senza "   ", ma se poi scrivo metto la stringa con "   ":
      if(xPosition+fontMetrics().width(curveParam[iTotPlot].name)<plotRect.width()){
        if(!_virtual){
           myPainter->drawText(xPosition,yPosition,msg);
           struct SHoveringData hovData;
           hovData.rect=myPainter->boundingRect(xPosition,yPosition-textHeight,400,50,Qt::AlignLeft, msg);
           hovData.iTotPlot=iTotPlot;
           hovDataLst.append(hovData);
           markerXPos=xPosition+myPainter->fontMetrics().width(curveParam[iTotPlot].name)+markHalfWidth;
           markerYPos=yPosition-0.8*myPainter->fontMetrics().height()+markHalfWidth;
           //Posizione dei marcatori vicino ai rispettivi nomi sulla leggenda:
           markPts[iTotPlot].setX(markerXPos);
           markPts[iTotPlot].setY(markerYPos);
        }
        xPosition+=myPainter->fontMetrics().width(msg);
      }else{
        iTotPlot--;
        numRows++;
        xPosition=0;
        yPosition+=textHeight;
        xPosition=0;
        if(numRows==4){
          numRows--;
          break;
        }
      }
    }
    legendHeight=(numRows+0.5)*textHeight;
    return;
  }
  //Nel seguente caso di nFiles>1 il codice è organizzato su due passate e si fanno tutte con Virtual = false:
  if(nFiles>1 && !_virtual){
    bool endBlock;
    int count=0, //generico contatore
    iRow, //indice della riga corrente
    nRows, //numero di righe scritte
    blocksPerRow[3],  //quanti bloccki per riga
    iTotPlStartBlock; //indice globale di variabile di inizio blocco
    iRow=0;
    blocksPerRow[0]=blocksPerRow[1]=blocksPerRow[2]=0;
    for(int iFile=0; iFile<nFiles; iFile++){
      endBlock=false;
      iTotPlStartBlock=iTotPlot;
      blocksPerRow[iRow]++;
      if(xPosition==0)
        msg=msg+filesInfo[iFile].name+":  ";
      else
        msg=msg+"  "+filesInfo[iFile].name+":  ";
      for(int iVar=0; iVar<nPlots[iFile]; iVar++){
        iTotPlot++;
        msg=msg+curveParam[iTotPlot].name+"   ";
 //Se sforo il fine riga tolgo il blocco corrente dalla riga corrente a meno che non sia l'unico blocco:
        if(myPainter->fontMetrics().width(msg)>plotRect.width()){
          if(blocksPerRow[iRow]>1){
            blocksPerRow[iRow]--;
            iTotPlot=iTotPlStartBlock;
            msg="";
            iFile--;
            count++;
            if(iVar<0||count>3){
              QMessageBox::critical(this,"CLineChart","error in writeLegend");
              qApp->quit();
            }
          }
          iRow++;
          endBlock=true;
          break;
        }
     }
     if(endBlock){
       if(iRow<3)
         continue;
       else
         break;
      }
    }
    nRows=3;
    if(blocksPerRow[2]==0)nRows=2;
    if(blocksPerRow[1]==0)nRows=1;
    legendHeight=(nRows+1)*textHeight;
    //Ora le dimensioni sono calcolate, faccio l'effettiva scrittura:
    yPosition=plotRect.y()+plotRect.height()-legendHeight+textHeight;
    int curBlock=0;
    iTotPlot=-1;
    xPosition=0;
    iRow=0;
    for(int iFile=0; iFile<nFiles; iFile++){
      curBlock++;
      if(curBlock>blocksPerRow[iRow]){
        curBlock=1;
        iRow++;
        if(iRow>2)break;
        xPosition=0;
        yPosition+=textHeight;
      }
      myPainter->setPen(Qt::black);
      if(xPosition==0)
        msg=filesInfo[iFile].name+":  ";
      else
        msg="  "+filesInfo[iFile].name+":  ";
      myPainter->drawText(xPosition,yPosition,msg);
      //Ora che ho scritto ad inizio riga il nome del file scrivo l'elenco delle corrispondenti variabili:
      xPosition+=myPainter->fontMetrics().width(msg);
      for(int iVar=0; iVar<nPlots[iFile]; iVar++){
        iTotPlot++;
        if(!blackWhite)
          myPainter->setPen(curveParam[iTotPlot].color);
        msg=curveParam[iTotPlot].name+"   ";
        lgdFont.setUnderline(curveParam[iTotPlot].rightScale);
        myPainter->setFont(lgdFont);
        myPainter->drawText(xPosition,yPosition,msg);
        struct SHoveringData hovData;
        hovData.rect=myPainter->boundingRect(xPosition,yPosition-textHeight,400,50,Qt::AlignLeft, msg);
        hovData.iTotPlot=iTotPlot;
        hovDataLst.append(hovData);
        markerXPos=xPosition+myPainter->fontMetrics().width(curveParam[iTotPlot].name);
        xPosition+=myPainter->fontMetrics().width(msg);
       //Posizione dei marcatori vicino ai rispettivi nomi sulla leggenda:
       markPts[iTotPlot].setX(markerXPos);
       markPts[iTotPlot].setY(yPosition+markHalfWidth);
     }
      lgdFont.setUnderline(false);
      myPainter->setFont(lgdFont);
   }
 }
}

/*$$$*******************************************************************/
/**************Righe da FilterClip.cpp**********************************/
/***********************************************************************/

#include <math.h>
#define abs(x) (((x)>0?(x):(x)*(-1)))

CLineChart::CFilterClip::CFilterClip(){
    MaxErr=0.5;
    LineDefined=false;
}

//---------------------------------------------------------------------------
bool CLineChart::CFilterClip::getLine(float X1_, float Y1_, float X2_, float Y2_){
    /* Descrivo l'equazione della retta tramite Ax+By+C=0, nella quale pongo B=1.
    */
    X1=X1_;	Y1=Y1_; X2=X2_;	Y2=Y2_;
    LineDefined=true;
    if(X2==X1)
        if(Y1==Y2){
            LineDefined=false;
            /* Le seguenti due righe servono in quanto, se i due punti passati sono coincidenti
            omettero' di tracciare un eventuale successivo punto solo se coincidente con
            il precedente.*/
            LastX=X1;
            LastY=Y1;
            return LineDefined;
        }else{
            A=1.f;
            B=0.f;
            C=-X1;
            Aux=1.f;
        }
    else{
        A=(Y1-Y2)/(X2-X1);
        B=1.f;
        C=-A*X2-Y2;
        Aux=sqrt(A*A+1.f);
    }
    //Aux=sqrt(A*A+B*B);
    Vector.X=X2-X1;
    Vector.Y=Y2-Y1;
    return LineDefined;
};

void CLineChart::CFilterClip::getRect(int X0, int Y0, int X1, int Y1){
    R.Left=X0-0.5f;	R.Top=Y0-0.5f;
    R.Right=X1+0.5f; R.Bottom=Y1+0.5f;
}
//---------------------------------------------------------------------------
bool  CLineChart::CFilterClip::IsInRect(float X, float Y){
    if(X>=R.Left && X<=R.Right &&	Y>=R.Top && Y<=R.Bottom)
        return true;
    else
        return false;
}
//---------------------------------------------------------------------------
bool  CLineChart::CFilterClip::IsRedundant(float X, float Y){
    //ritorna true se il punto passato Ë all'interno della striscia
    float Dist;
    /* Se non sono riuscito a definire in precedenza una retta era perché avevo
    utilizzato due punti coincidenti,
    */
    if(!LineDefined) {
        if(X==LastX && Y==LastY)
            return true;
        else
            return false;
    }
    Dist=abs(A*X+B*Y+C)/Aux;
    if (Dist<MaxErr){
        if( Vector.X*(X-X2) + Vector.Y*(Y-Y2) >=0  || strongFilter ){
            X2=X;
            Y2=Y;
            return true;
        }
    }
    return false;
}
//---------------------------------------------------------------------------
int CLineChart::CFilterClip::giveRectIntersect(FloatPoint & I1, FloatPoint &I2){
    /* Questa funzione copia nei parametri passati i valori delle eventuali intersezioni
    del segmento congiungente i due punti interni (X1,Y1) e (X2,Y2), passati con la
    presente GetLine, con il rettangolo R passato con il recente GetRect.
    La funzione ritorna il numero di intersezioni trovate.
    */
    float X;
    FloatPoint P[2];
    int iPoint=-1;

    if(GiveX(R.Top,X))
        if(X>=R.Left && X<=R.Right){
            P[++iPoint].X=X;
            P[iPoint].Y=R.Top;
        }
    if(GiveX(R.Bottom,X))
        if(X>=R.Left && X<=R.Right){
            P[++iPoint].X=X;
            P[iPoint].Y=R.Bottom;
        }
    if(GiveY(R.Left,X))
        if(X>=R.Top && X<=R.Bottom){
            P[++iPoint].X=R.Left;
            P[iPoint].Y=X;
        }
    if(GiveY(R.Right,X))
        if(X>=R.Top && X<=R.Bottom){
            P[++iPoint].X=R.Right;
            P[iPoint].Y=X;
        }
    switch (iPoint){
        case -1:
            return 0;
        case 0:
            I1=P[0];
            return 1;
        case 1:
            //Se sono stati trovati due punti devo individuare quale Ë il primo che si incontra
            //andando da (X1,Y1) a (X2,Y2).
            if( (P[0].X-X1)*(P[0].X-X1) + (P[0].Y-Y1)*(P[0].Y-Y1) <
                    (P[1].X-X1)*(P[1].X-X1) + (P[1].Y-Y1)*(P[0].Y-Y1)  ) {
                I1=P[0];
                I2=P[1];
            }else{
                I1=P[1];
                I2=P[0];
            }
    }
    return 2;
}


float inline CLineChart::CFilterClip::GiveX(float Y){
    /* Funzione che mi dà l'ascissa X dell'intersezione con una data Y
    della retta passante per i punti interni (X1,Y1), (X2,Y2).
    */
    return -(B*Y+C)/A;
}

bool CLineChart::CFilterClip::GiveX(float Y, float &X){
    /* Funzione che cerca l'intersezione con una data Y	del segmento che congiunge i punti
    interni (X1,Y1), (X2,Y2), e ne mette in X il valore.
    Gli estremi sono esclusi.
    */
    float Factor;
    if(Y2==Y1) return false;
    Factor=(Y-Y1)/(Y2-Y1);
    if(Factor>0 && Factor<=1){
        X=X1 + Factor*(X2-X1);
        return true;
    }else
        return false;
}

float inline CLineChart::CFilterClip::GiveY(float X){
    /* Funzione che mi d‡ l'ordinata Y dell'intersezione con una data X
    della retta passante per i punti interni (X1,Y1), (X2,Y2).
    */
    return -(A*X+C)/B;
}

bool CLineChart::CFilterClip::GiveY(float X, float & Y){
    /* Funzione che cerca l'intersezione con una data X	del segmento che congiunge i punti
    interni (X1,Y1), (X2,Y2), e ne mette in Y il valore.
    Gli estremi sono esclusi.
    */
    float Factor;
    if(X2==X1) return false;
    Factor=(X-X1)/(X2-X1);
    if(Factor>0 && Factor<1){
        Y=Y1 + Factor*(Y2-Y1);
        return true;
    }else
        return false;
}
