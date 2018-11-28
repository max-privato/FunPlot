#ifndef CSCALEDLG_H
#define CSCALEDLG_H

#include <QDialog>
#include "Globals.h"
#include "CUnitsDlg.h"


namespace Ui {
class CScaleDlg;
}
#ifndef GLOBALS_H
  struct SFloatRect2{float Left,Right,LTop,LBottom,RTop,RBottom;};

  struct SUserLabel {
    QString B,E; //Stanno per base, esponente: l'esponente è scritto più piccolo e in alto per consentire una buona visualizzazione delle potenze di 10.
  };
#endif

class CScaleDlg : public QDialog
{
    Q_OBJECT
    
public:
    bool exactMatch, //checkbox "exactMatch" is checked
         isTwinScale,
         managefullLimits,
         useSmartUnits,
         useUserUnits;  //Request for CLineChart to use user defined Units of measure
    bool useBrackets;
    QString xUnit, yUnit, ryUnit;  //Units of measure of the three axes
    explicit CScaleDlg(QWidget *parent = 0);
    void getAllUnits(QString xUnit, QString yUnit, QString ryUnit);
    void getInfo(SFloatRect2 dispRect, bool twinScale);
    void getFullLimits(SFloatRect2 fullLimits_, bool manageFullLimits);
    bool giveExactMatch();
    SFloatRect2 giveDispRect(void);
    void setUseBrackets(bool useBrackets_);
    void setTwinScale(bool ts);
    QString validDispRect();
    ~CScaleDlg();
    
private slots:
    void on_exaMatchBox_clicked(bool checked);
    void on_unitsBtn_clicked();
    void on_flRadioBtn_clicked();
    void on_manRadioBtn_clicked();

private:
    Ui::CScaleDlg *ui;
    CUnitsDlg* myUnitsDlg;
    SFloatRect2 dispRect, fullLimits;
    void showEvent(QShowEvent *);

};

#endif // CSCALEDLG_H
