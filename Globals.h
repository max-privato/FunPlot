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
