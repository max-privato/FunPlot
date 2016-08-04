#include "CLabelsDlg.h"
#include "ui_CLabelsDlg.h"

CLabelsDlg::CLabelsDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CLabelsDlg)
{
    ui->setupUi(this);
}
void CLabelsDlg::getAllLabels(SAllLabels all){
    ui->xBaseEdit->setText(all.xLbl.B);
    ui->xExpEdit->setText(all.xLbl.E);
    ui->yBaseEdit->setText(all.yLbl.B);
    ui->yExpEdit->setText(all.yLbl.E);
}

SAllLabels CLabelsDlg::giveAllLabels(){
    SAllLabels all;
    all.xLbl.B =ui->xBaseEdit->text();
    all.xLbl.E =ui->xExpEdit->text();
    all.yLbl.B =ui->yBaseEdit->text();
    all.yLbl.E =ui->yExpEdit->text();
    return all;
}


bool CLabelsDlg::giveWisOmega(){
    return ui->checkBox->isChecked();
}

CLabelsDlg::~CLabelsDlg()
{
    delete ui;
}
