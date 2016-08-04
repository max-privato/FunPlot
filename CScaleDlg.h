#ifndef CSCALEDLG_H
#define CSCALEDLG_H

#include <QDialog>
#include "Globals.h"
#include "CLabelsDlg.h"


namespace Ui {
class CScaleDlg;
}
#ifndef GLOBALS_H
  struct SFloatRect2{float Left,Right,LTop,LBottom,RTop,RBottom;};

  struct SUserLabel {
    QString B,E; //Stanno per base, esponente: l'esponente è scritto più piccolo e in alto
                  // per consentire una buona visualizzazione delle potenze di 10.
  };
#endif

class CScaleDlg : public QDialog
{
    Q_OBJECT
    
public:
    bool exactMatch, labelOverride, wIsOmega;
    explicit CScaleDlg(QWidget *parent = 0);
    void getAllLabels(SAllLabels all);
    void getDispRect(SFloatRect2 dispRect);
    SAllLabels  giveAllLabels(void);
    bool giveWisOmega();
    SFloatRect2 giveDispRect(void);
    QString validDispRect();
    ~CScaleDlg();
    
private slots:
    void on_exaMatchBox_clicked(bool checked);
    void on_labelsBtn_clicked();

private:
    Ui::CScaleDlg *ui;
    SUserLabel labelX, labelYL, labelYR;
    SAllLabels allLabels;
    CLabelsDlg* myLabelsDlg;
};

#endif // CSCALEDLG_H
