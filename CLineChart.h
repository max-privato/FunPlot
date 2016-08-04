#ifndef CLINECHART_H
#define CLINECHART_H
#include <QRect>
#include <QWidget>
#include <QLabel>
#include <QPainter>
#include <QPrinter>
#include <QStack>
#include <QTimer>
#include <QMenu>
#include "Globals.h"
#include "matrix.h"
#define RATIOLIM 0.80001
#define MAXAUTOMARKS 4
#define MAXMANUMARKS 8
#define MAXLOGTICS 26  //Cinque tacche per decade, cinque decadi più una
#define EXPFRACTION 0.75  //simensionid ell'esponente in frazione delle dimensionid ella base

class QCursor;

enum EPlotType{ptLine,ptBar, ptSwarm};
enum EPlotPenWidth {pwThin, pwThick, pwAuto};
enum EScaleType {stLin, stDB, stLog};
enum ESwarmPointSize {ssPixel, ssSquare};
enum EDrawType {dtMC, //Filtraggio con gli algoritmi di MC, come sviluppati nell'implementazione storica BCB
                dtQtF, //assenza di filtraggio e uso della funzione painter.lineTo fra numeri float
                dtQtI,//assenza di filtraggio e uso della funzione painter.lineTo fra numeri int.
                dtPoly}; //assenza di filtraggio e uso di Polygon invece di drawPath.
#ifndef GLOBALS_H
  struct SFloatRect2{float Left,Right,LTop,LBottom,RTop,RBottom;};
  struct SUserLabel {
    QString B,E; //Stanno per base, esponente: l'esponente scritto pi piccolo e in alto
               // per consentire una buona visualizzazione delle potenze di 10.
  };
#endif

struct SXYValues{
        float *X, **Y;
};
class CLineChart:public QLabel
{
    /* La classe è molto articolata, e i suoi contenuti sono elencati secondo il seguente ordine:
      0) macro
      1) definizione di enum, strutture e classi (compresa CFilterClip)
      2) variabili implementate come property
      3) variabili private
      4) funzioni private
      5) variabili pubbliche
      6) funzioni pubbliche
      7) funzioni di accesso properties
      8) signals
      9) private slots
*/

// *************  0) MACRO
    Q_OBJECT
    Q_PROPERTY(int activeDataCursor  READ giveActiveDataCurs WRITE setActiveDataCurs)
    Q_PROPERTY(EPlotPenWidth plotPenWidth READ givePlotPenWidth WRITE setPlotPenWidth)
//    Q_PROPERTY(SFloatRect2 dispRect READ giveDispRect WRITE setDispRect)
    Q_ENUMS(EPlotPenWidth)

// *************  1) DEFINIZIONE DI ENUM E STRUTTURE
    enum EadjustType{atLeft, atRight, atCenter};
    enum ELegendFontSizeType {EqualToAxis, LAuto};
public:
    enum EAxisType {atX, atYL, atYR};
private:
    struct SDigits{
        unsigned i1,i2,i3,i4;
      int ie;
        float Value, roundValue;
        char Sign;
    };
    struct FloatPoint{float X,Y;};
    struct SAxis{
      /* Tutti i dati sono relativi a scale lineari eccetto eMin ed eMax*/
      bool
      addZeroLine, //Se attiva aggiungo una riga "del colore degli assi"  che segna la
                   // coordinata "0" Se è true sull'asse y sarà tracciata la retta y=0
                   // parallela all'asse x.
      halfTicNum; //se  attiva, dimezzo i numeri sulle tacche quando metto
                  //un elevato numero di tacche per far comparire anche la tacca sullo 0
      enum EScaleType scaleType; //dice se ho scala lineare, DB o logaritmica pura
      int eMin,   //potenza di 10 relativa all'estremo inferiore di scala logaritmica
          eMax,   //potenza di 10 relativa all'estremo superiore di scala logaritmica
          done,  //se  0 i due estremi, arrotondati a quattro cifre significative, sono uguali,
                //in tal caso di usano due sole tacche, in posizioni convenzionali;
                //se  1, sono riuscito a trovare due estremi distinti ma non "rotondi",
                //2 se il risultato  OK, cio se ho eseguito la scalatura in modo
                //"exactMatch", oppure ho trovato i due numeri rotondi.
          width, //ampiezza dell'asse in pixel: X1-X0, o Y1-Y0
                 //(in realtà sull'asse X c'è una piccola correzione nel solo caso delle bar).
          maxTextWidth, //massima ampiezza in pixel dei testi sugli assi, comprendendo
                    // i numeri sulle tacche e le eventuali label
          ticDecimals, //Numero di decimali sulle tacche
          scaleExponent; //la scala si intende moltiplicata per 10^ScaleExponent
      float scaleFactor, //Fattore di scala per evitare numeri troppo grossi o piccoli
                          //sulle tacche degli assi  = 10^ScaleExponent
            ticInterval;	//distanza numerica fra le tacche
      float scaleMin,  //valore numerico visualizzato sulla label all'estremo SX del grafico
            scaleMax;  //valore numerico visualizzato sulla label all'estremo DX del grafico
            /*NOTA: Nel caso di grafico logaritmico ScaleMin e ScaleMax sono i valori
                   effettivi prima dell'estrazione del logaritmo  */
      float minF,  //=ScaleMin*ScaleFactor
            maxF;  //=ScaleMax*ScaleFactor
         enum EAxisType type;
    };
    struct SMarkPts{
        int indexSX[MAXFILES][MAXMANUMARKS], //indice delle matrici dei punti immediatamente a sinistra del punto su cui si mettono i marcatori
            lastMark; //Indice dell'ultimo marcatore manuale inserito
      float fraction[MAXFILES][MAXMANUMARKS]; /*Questo numero precisa l'indicazione fornita da IndexSX: dà la frazione dell'intervallo fra il punto di indice indexSX e         indexSX+1; in tal modo ho indicazione precisa della posizione del marcatore,        anche quando i punti visualizzati sono pochi. La memorizzazione è anche indipendente        dalla risoluzione del Painter su cui vado a scrivere, e quindi può essere anche        utilizzata per la stampa.
        */
    };
    struct SMinMax {float Min, Max;}; //contiene i valori minimi e massimi ad es. di un array.


/*$$$*********************************************************************************/
/***********Inizio definizione della classe FilterClip*******************************/
/*************************************************************************************/
    class CFilterClip{
  /* Questa classe implementa le funzioni necessarie per
    1. filtrare da grafici punti ridondanti
    2. fare una ClipRegion manuale in quanto wuella standard di Windows non ha effetto
       sulle metafile una volta che queste ultime siano state aperte dal programma di
       destinazione.
    Per quanto riguarda la funzione 1 viene definita una striscia intorno ad una retta:
    se un dato punto dsi trova entro la striscia definita dai due punti precedenti
    può essere filtrato via.
      OCT '98. L'algoritmo della striscia semplice funziona erroneamente in alcuni casi:
        se ad es. ho un overshoot di un solo pixel e poi si torna indietro restando nella
        striscia se non adotto correttivi il punto di picco dell'overshoot viene escluso dal
        grafico. Pertanto devo includere nel grafico oltre che i punti fuori della striscia
        anche quelli che "tornano indietro". Questo risultato lo ottengo considerando il
        segno del prodotto scalare di un vettore rappresentativo, in direzione e verso,
        della retta (LineVector), e un vettore congiungente penultimo e ultimo punto
        considerato.
    Per quanto riguarda la funzione 2 essa  implementata nel metodo "GiveRectIntersect"
    che dà l'intersezione di una retta orientata con un rettangolo.
  */
        public:
          bool strongFilter; //Se  true vuol dire che sto facendo un Copy e il LineChart
                               //di cui FC fa parte ha StrongFilter=true.
          struct FloatPoint{float X,Y;};
            float MaxErr; //SemiLarghezza della striscia
            CFilterClip(void);
            bool getLine(float X1, float Y1, float X2, float Y2); //si passano le coordinate
                //di due punti per cui passa la retta, e si definisce in tal modo la retta di
                //riferimento. Se i due punti passati erano coincidenti, la retta non  definibile
                //e la funzione ritorna false;
            void getRect(int X0, int Y0, int X1, int Y1);
              int giveRectIntersect(FloatPoint & I1, FloatPoint &I2);
                bool IsRedundant(float X, float Y); //ritorna true se il punto passato non  all'interno della striscia
            bool IsInRect(float X, float Y); //ritorna true se il punto passato  all'interno del rettangolo
        private:
          struct FloatRect{	float Top, Bottom, Left, Right;};
            float X1,Y1,X2,Y2;
            FloatRect R; //Rettangolo del grafico su cui effettuare il taglio delle curve
              FloatPoint Vector;  //vettore che mi dà direz. e verso della StraightLine
            bool LineDefined;
            float A, B, C,  //coefficienti dell'equazione della retta e sqrt(A^2+B^2)
                        Aux, LastX, LastY;
            float inline GiveX(float Y), GiveY(float X);
            bool GiveX(float Y, float &X), GiveY(float X, float &Y);
        };
/*$$$***********************************************************************/
/*************Fine definizione della classe FilterClip**********************/
/***************************************************************************/

// *************  2) VARIABILI PRIVATE IMPLEMENTATE COME PROPERTY
  int activeDataCurs; //Il cursore dati attivo di massimo numero (se  2 o 3  attivo anche il cursore 1!)
  EPlotPenWidth PPlotPenWidth;


// *************  3) VARIABILI PRIVATE nell'ordine: bool, int, mie strutture, strutture Qt
  bool
    adjustingTitleSize, //Se è true è in atto un eventuale adattamento del grafico all'altezza del titolo
    autoMark,
    copying, //true se è in atto un'operazione di "Copy"
    dataCursDragging, //se è true sto trascinando il rettangolo per la visualizzazione dati
    dataCurs2Dragging,
    dataCursVisible, dataCurs2Visible,
    dataGot, // se  true i dati sono stati acquisiti
    defaultFO[MAXFILES+1], //L'elemento di indice nFiles fa riferimento alla variabile
      //sull'asse x; gli altri alle variabili dei vari files. Se un valore  true
      //la variabile x, ovvero le variabili provenienti da un certo files hanno tutti
      //Factors unitari e Offsets nulli
    zoomSelecting, //sto facendo il drag per individuare il rettangolo del grafico
    plotDone,  // falso solo all'inizio quando nessun grafico è visualizzato;
    printing, //true se sono in fase di stampa
    strongFilter, //Se è true durante il Copy faccio un filtraggio dei dati "strong"
    variableStep1, // se è true il file  a passo variabile
    makingSVG,
    writeTitle1;  //se true visualizzerò il titolo

  int dataCursSelecting, //indica su quale cursore dati ero quando ho messo il cursore mouse a freccia:
                         // serve quando faccio click in prossimit di un cursore dati
      generalFontPx,
      lastDataCurs, //indice numerico dell'ultimo visualizzatore dati considerato
      legendFontSize,
      legendHeight, titleHeight,titleRows,
      /*La seguente variabile  non nulla soltanto per i diagrammi a barre. E' un margine
      che va aggiunto a SX a X0 e tolto a DX a X1 per fare in modo che la prima e l'ultima
      barra non debbano avere ampiezza dimezzata per non sforare fuori il rettangolo
      X0Y0-X1Y1.
      */
      margin,
      minXTic, minYTic, //Numero minimo di tacche consentite dall'attuale dimensione
                        // del grafico rispettivamente sull'asse X e Y
      nFiles, //Numero di files delle variabili da plottare
      nPlots1, //cella contenente l'informazione "nPlots" per l'uso di funzioni relative al caso di file singoli
      *nPlots, //Vettore dei numeri di grafici per i vari files
      numOfTotPlots, //Numero di grafici visualizzati
      numOfVSFiles, //Numero di files a passo variabile aventi variabili visualizzate
      pointsDrawn, //Numero di punti utilizzati per il tracciamento
      smallHSpace, //circa mezzo carattere di spaziatura con GeneralFontSize (orizzontale)
      svgOffset, //Serve per correggere un errore non chiarito del tracciamento SVG
      swarmPointWidth,
      textHeight, //Altezza testo valori numerici degli assi
      X0,Y0,X1,Y1, //ascisse e ordinate del rettanglolo del grafico;
      xVarIndex, //Indice che nel vettore delle variabili  selectedVarNames indicizza il nome della variabile dell'asse x
      **pixelToIndexDX; //matrice che punta, per ogni pixel, all'indice del punto immediatamente alla sua destra (solo per le variabili variableStep)
  int *startIndex, *stopIndex, //Indici di inizio e fine di grafico in caso di zoom
      xStartIndex[MAXFILES], //ascissa in pixel del primo punto visualizzato (che corrisponde a StartIndex[0])
      xStopIndex[MAXFILES]; //ascissa in pixel dell'ultimo punto visualizzato (che corrisponde a StopIndex[0])
  float aspectRatio;  //altezza/larghezza
  float markHalfWidth; //metà della dimensione orizzontale dei Mark

  float lastXCurs[3]; //serve per riposizionare i cursori durante il dimensionamento
  float **px, ***py; //Matrici dei valori asse x e asse y;
  float *cursorXValues,	//Valori da utilizzare per il cursore numerico
               **cursorYValues; //Valori da utilizzare per il cursore numerico
  float *cursorXValBkp,	//Valori di salvataggio del cursore numerico
               **cursorYValBkp; //Valori di salvataggio del cursore numerico
  ELegendFontSizeType legendFontSizeType;
  // La seguente struttura SCurveParam  definita in globals.h.
  SCurveParam *curveParam;
  SXVarParam xVarParam;
  SAxis xAxis, yAxis, rYAxis;
  SMarkPts manuMarks; //Posizioni dei marcatori manualmente fissati
  SMinMax xmM, LymM, RymM;
  SUserLabel xUserLabel, yUserLabel, ryUserLabel;
  SFloatRect2 dispRect; //Rettangolo di visualizzazione attuale
  QString baseFontFamily;
  CFilterClip FC;
  struct {float x,y,yr;} ticInterv,
         ratio; //rapporto fra ampiezza asse in pixel e ampiezza numerica della grandezza visualizzata nel rettangolo del grafico

  QCursor myCursor;
  QFont numFont, baseFont, expFont, lgdFont; //Loro utilizzo in Developer.odtQuesti tre font vengono definiti solo nel design plot. Essi sono funzione della dimensione del grafico. se é fontSizeType==fsFixed, allora il baseFont assumerà come pixelSize fixedFontPx
  QList <SFileInfo> filesInfo;
  QRect hovVarRect; //contiene la posizione del rettangolo in cui è posizionato il nome della variabile su cui si sta facendo Hovering. Serve per facilitare il debug del tooltip con nme completo per funzioni di variabili. Non è escluso che sia utile anche dopo che questa fuznione di debug si sia esaurita.
  struct SHoveringData {int iTotPlot; QRect rect;};
  QList <SHoveringData> hovDataLst; //informazioni sulle  aree in cui sono posizionati i nomi delle variabili, per consentire informazioni aggiuntive quando si fa l'hovering.
  QImage * myImage;
  QPainter *myPainter; //painter su cui opera il comando plot()
/* Nota: lo stesso myPainter viene usato sia per le scritture su myImage (quindi a schermo) che su Svg e sulla stampante: semplicemente si associa questo painter a tre differenti paint devices! L'unico globale di CLineChart  myImage; l'svg generator e la stampante vengono definiti e usati all'interno di makeSvg() e di print().
*/
  QPen framePen, gridPen, plotPen, ticPen;
  QPointF stPos, endPos;
  QPointF markPts[MAXVARS]; //Posizioni dei marcatori sui nomi delle variabili
  QPoint ticWidth;
/* In debugRect copierò sempre dataCurs in quanto a seguito di un bug non chiarito capita che all'evento paint vengano spesso inviati dei dataCurs con valore x  errato!*/
  QRect debugCurs;
  QRect dataCurs, dataCurs2;
  QRect plotRect; //Normalmente contiene geometry(); Ma nel caso della stampa contiene printer->pageRect();
  QStack <SFloatRect2>  plStack;
  QString baseFontName, symbFontName, titleText;
  QTimer * myTimer;
  //QString sPlotTime;
  QRectF titleRectF;

// *************  4) FUNZIONI PRIVATE in ordine alfabetico

  void drawAllLabelsAndGrid(SAxis axis);
  void drawAllLabelsAndGridDB(SAxis axis);
  void drawAllLabelsAndGridLog(SAxis axis);
  bool allowsZero(float MinRoundValue, float MaxRoundValue,int ntic);
  void beginRounding(SDigits &Min, SDigits &Max, float MinVal, float MaxVal,unsigned Include0);
  int computeTic(float minRoundValue_, float maxRoundValue_, int halfTicNum_),
      computeDecimals(float scaleMin, float ticInterval, bool halfTicNum);
  void drawBars(void);
  void drawCurves(bool NoCurves);
  void drawCurvesQtF(bool NoCurves);
  void drawCurvesQtI(bool NoCurves);
  void drawCurvesPoly(bool NoCurves);
  void  drawMark(float X, float Y, int mark, bool markName);
  void drawSwarm(void);
  bool fillPixelToIndex(int **pixelToIndexDX);
  bool fillPixelToIndexLog(int **PixelToIndexDX);
  //Funzione che d i valori delle X e delle Y in corrispondenza di una data posizione
  //in pixel del cursore. I bool dicono se devo dare solo le differenze rispetto ai
  //valori in corrispondenza del cursore primario:
  SXYValues giveValues(int CursorX, bool Interpolation, int &NearX, bool Xdiff, bool Ydiff);
  SXYValues giveValues(int CursorX, bool Interpolation, bool Xdiff, bool Ydiff);
  SFloatRect2 giveZoomRect(int StartSelX, int StartSelY, int X, int Y);
  struct SMinMax findMinMax(float * vect, unsigned dimens);
  void mark(bool store);
  //Funzione per mettere un singolo marcatore in corrispondenza della posizione
  //orizzontale del cursore e del file e del grafico specificati attraverso i primi due
  //parametri passati:
  void markSingle(int iFile, int iVSFile, int iPlot, int iTotPlot, bool store);
  static float minus(struct SDigits d, unsigned icifra, unsigned ifrac);
  static float plus(struct SDigits d, unsigned icifra, unsigned ifrac);
  void selectUnzoom(QMouseEvent *event);
  int scaleAxis(SAxis &Axis, float MinVal, float MaxVal, int MinTic, unsigned Include0, bool exactMatch);
  int drawText2(int X, int Y, EadjustType hAdjust, EadjustType vAdjust, QString msg1, QString msg2, bool _virtual, bool addBrackets=false);
  void  writeLegend(bool _virtual);

protected:
  void leaveEvent(QEvent *);
  void mouseDoubleClickEvent(QMouseEvent *event);
  void mouseMoveEvent(QMouseEvent *event);
  void paintEvent(QPaintEvent *ev);
  void mousePressEvent(QMouseEvent *event);
  void mouseReleaseEvent(QMouseEvent *event);

public:
// *************  5) VARIABILI PUBBLICHE
/*** Variabili pubbliche implementate in BCB come property: ***/
  bool
    blackWhite, //se  true si tracciano tutte le curve in nero
    forceYZero,//forza lo 0 sull'asse y
    FLegend, //se è true scrivo la leggenda
    FLinearInterpolation, //Se è true con cursore faccio l'interpolazione lineare
    twinScale, //se è true il grafico ha anche la scala verticale destra
    useBrackets, //se è true vengono usate parentesi tonde intorno all'unità di misura
    xGrid, //se è true si mette la griglia sull'asse X
    yGrid, //se è true si mette la griglia sull'asse Y
    wIsOmega, //se è true nelle label manuali la prima "O" è interpretata come
              //Omega (lettera greca)
    zoomed; //Se è true il diagramma è stato zoomato
    EDrawType drawType;
    enum EFontSizeType {fsFixed, fsAuto};
    EFontSizeType fontSizeType;
    EPlotType plotType;
    ESwarmPointSize swarmPointSize;

// Altre variabili pubbliche
bool  exactMatch, //se true DispRect  correntemente visualizzato
                  //in modalità "ExactMatch".
      autoLabelXY, //se  true vengono automaticamente riconosciute le unità di misura
                   //dall'inzio del nome, t(s), (kV), ecc.
      labelOverride; //Se  true vengono messe sul grafico label definite dall'utente
                   //e poste in xUserLabel, yUserLabel, yrUserLabel;
int maxAutolineWPoints; // Numero massimo con cui si fa il grafico con spessore auto. Infatti con Qt, nel caso di MorkSwarm, a spessore aumentato il tempo del grafico  altissimo (4s contro 50ms di BCB)! Questo problema non si può risolvere con path.simplified() che da una parte non migliora la velocità di esecuzione, ma in compenso traccia anche una linea erronea (che rende il path un percorso chiuso).
int drawTimeMs; //drawing time in MS
int fixedFontPx;


// *************  6) FUNZIONI PUBBLICHE (in ordine alfabetico
  CLineChart(QWidget * parent);
  void copy();
  void designPlot(void);
  void disableTitle();
  void enableTitle();
  void getAllLabels(SAllLabels all);
  //funzione per l'utilizzo elementare di lineChart: associabile naturalmente a plot() (senza parametri passati).
  void getData(float *px_,float **py, int nPoints_, int nPlots_=1);
  //Funzione per il passaggio dati all'oggetto relativa al caso di file singoli:
  void getData(SFileInfo FI, int nPlots_,  SXVarParam xVarParam_, SCurveParam *curveParam_,float *px_,float **py_);
  //Funzione per il passaggio dati all'oggetto relativa al caso di file multipli:
  void getData(QList <SFileInfo>, int*nPlots,  SXVarParam xVarParam,SCurveParam *curveParam,float **px, float ***py);
  SFloatRect2 giveDispRect();
  SAllLabels giveAllLabels(void);
  QImage *  giveImage();
  QString makePng(QString fullName, bool issueMsg);
  QString makeSvg(QString fullName, bool issueMsg=true);
  void mark(void), markAuto(void);
  void markAll();
  QString plot(bool autoScale=true);
  QString plot(bool Virtual, bool IncludeFO);
  QString print(QPrinter * printer, bool thinLines);
  void resetMarkData();
  void resizeEvent(QResizeEvent *);
  void setDispRect(SFloatRect2 rect);
  QString setFullDispRect();
  void setXScaleType(enum EScaleType xScaleType_);
  void setYScaleType(enum EScaleType yScaleType_);
  void setXZeroLine(bool zeroLine_);
  void setYZeroLine(bool yZeroLine_);
  int scaleXY(SFloatRect2 r, bool justTic);
  void setRect(QRect r);
  int writeAxisLabel(int X, int Y, SAxis &Axis, bool Virtual);


// *************  7) FUNZIONI DI ACCESSO PROPERTY
  EPlotPenWidth givePlotPenWidth() const;
  void setPlotPenWidth(EPlotPenWidth myWidth);
  int giveActiveDataCurs();
  void setActiveDataCurs(int activeCurs);

  // *************  8)   SIGNALS
  signals:
  void valuesChanged(SXYValues values, bool hDifference, bool vDifference);

  // *************  9)   PRIVATE SLOTS
  private slots:
  void resizeStopped();

};

#endif // CLINECHART_H