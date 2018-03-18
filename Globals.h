#ifndef GLOBALS_H
#define GLOBALS_H
#include<QColor>
#include <QDateTime>
#include <QString>
#include <QVector>

// *** Valori limite ad allocazione statica:
#define MAXFILES 8  //numero massimo di files (le linee inizialmente visualizzate sono invece 3)
#define MAXSHEETS 4
#define MAXVARS 15  //Numero massimo di variabili nella lista (escluso il tempo)
#define MAXFUNPLOTS MAXVARS  //Numero massimo di funzioni di variabili plottabili simultaneamente
#define VARTABLEPANELS 4


// *** Valori default delle opzioni di programma:
#define AUTOLABELXY true
#define BARCHARTFORFS false
#define BARCHARTFORHFS false
#define COMMASARESEPARATORS false
#define DEFAULTFREQ 50.
#define ENABLEAUTOREFRESH false
#define MAXSPEED false
#define ONLYPOINTSINPLOTS false
#define USETHINLINES false
#define REMEMBERWINPOSANDSIZE false
#define SHOWTIME false
#define USEBRACKETS true
#define USEGRIDS false


// *** Strutture globali:
//struct SReturn{int code; QString msg;}; //valore di ritorno per talune funzioni, quale ad es. loadFromPl4File: contiene un codice di severità: 0 warning, 1 errore che non fa uscire dal programma, 2 errore che fa uscire dal programma

struct SfileRecord{
  QString fullName;
  QDateTime dateTime;
};

struct SOptions{
    bool autoLabelXY, barChartForFS, commasAreSeparators, rememberWinPosSize, showElapsTime, useGrids, onlyPoints, useMatLib, useThinLines, useBrackets;
    int firstFileIndex;
    float defaultFreq; //frequenza di riferimento per il calcolo della DFT
};

struct SCurveParam {
    bool isFunction; //true se ho una variabile-funzione
    bool monotonic; //Nel caso essa sia true, sarà possibile effettuare le zoomate in maniera molto più veloce in quanto si sfrutta il fatto di sapere che X è monotona crescente.
//    int fileIndex; //numero del file in base 0
    int idx; //numero della variabile all'interno del file considerato: indice di mySO[ifile]->y[num];'
    QString name; //il nome della variabile; per le funzioni f1, f2, ecc.
    QString midName;  //per le funzioni la loro stringa, tipo f1v1+2*v3
    QString fullName; // per le funzioni il nome completo, tipo voltage1-voltage2/2.0
    QColor color; // il colore (anche se non definito per la var. x sarà sempre black)
    bool rightScale; //valore non definito per la variabile x
    QString unitS;
};
struct SFileInfo {bool frequencyScan, variableStep; int fileNum, numOfPoints; QString name;};

/* Variabili globali contenenti informazioni generali che si debbano rendere facilmente accessibili ai vari moduli del programma:
*/
struct SFloatRect2{float Left,Right,LTop,LBottom,RTop,RBottom;};

struct SUserLabel {
  QString B,E; //Stanno per base, esponente: l'esponente è scritto più piccolo e in alto
                // per consentire una buona visualizzazione delle potenze di 10.
};
struct SAllLabels{
    struct SUserLabel xLbl, yLbl, ryLbl;
};

struct SGlobalVars {
  bool multiFileMode;
  int shiftWin, //Variabile globale rappresentante lo spostamento verticale, in pixel, delle finestre principali del programma
      instNum, //Variabile globale rappresentante il numero dell'istanza del programma
      firstNameIndex, //indice fra i parametri passati del primo fileName.
      fileNameNum;  //numero di nomi di file passati fra i parametri
  short int WinPosAndSize[3+10*MAXSHEETS];
  QVector <int> varNumsLst;
  struct SOptions PO;
};

//La seguente struttura è obsoleta e verrà soppressa in quanto i dati di SCurveParam sono sufficienti a descrivere sia le variabili di asse x che di asse y
struct SXVarParam{
    bool isMonotonic, isVariableStep;
    QString unitS; //le unità di misura, ad es. "s" "kA", etc.
    QString name;
};
enum EAmplUnit {peak, rms, puOf0, puOf1};
enum EAmplSize {fifty, seventy, hundred};

struct SFourOptions{
    enum EAmplUnit amplUnit;
    enum EAmplSize amplSize;
    int harm1, harm2;
    float initialTime, finalTime;
};

struct SFourData{
    bool variableStep;
    int numOfPoints;
    float *x,*y;
    QString fileName, varName, ret;
    SFourOptions opt;
};

#endif // GLOBALS_H
