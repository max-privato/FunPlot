#ifndef CLABELSDLG_H
#define CLABELSDLG_H

#include <QAbstractButton>
#include <QDialog>
#include "Globals.h"
#include "CLineChart.h"

namespace Ui {
class CLabelsDlg;
}

class CUnitsDlg : public QDialog
{
    Q_OBJECT
    
public:
    bool useBrackets;
    bool useSmartUnits;
    bool doReset; //commands to reset useUserUnits to false
    QString xUnit, yUnit, ryUnit; //Units of measure of the three axes
    explicit CUnitsDlg(QWidget *parent = 0);
    void getTwinScales(bool isTwinScale_);
    void setUseBrackets(bool useBrackets_);
    void setXUnit(QString xUnit_);
    void showEvent(QShowEvent *);
    ~CUnitsDlg();
    
private slots:
    void on_bracketsChkBox_clicked(bool checked);
    void on_yUnitEdit_editingFinished();
    void on_ryUnitEdit_editingFinished();
    void on_xUnitEdit_editingFinished();
    void on_buttonBox_clicked(QAbstractButton *button);
    void on_buttonBox_accepted();

private:
    Ui::CLabelsDlg *ui;
};

#endif // CLABELSDLG_H
