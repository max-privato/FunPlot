#ifndef CLABELSDLG_H
#define CLABELSDLG_H

#include <QDialog>
#include "Globals.h"

namespace Ui {
class CLabelsDlg;
}

class CLabelsDlg : public QDialog
{
    Q_OBJECT
    
public:
    explicit CLabelsDlg(QWidget *parent = 0);
    void getAllLabels(SAllLabels all);
    SAllLabels giveAllLabels();
    bool giveWisOmega();
    ~CLabelsDlg();
    
private:
    Ui::CLabelsDlg *ui;
};

#endif // CLABELSDLG_H
