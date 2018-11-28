#include "CUnitsDlg.h"
#include "ui_CUnitsDlg.h"

CUnitsDlg::CUnitsDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CLabelsDlg){
    ui->setupUi(this);
    useSmartUnits=true;
}

CUnitsDlg::~CUnitsDlg()
{
    delete ui;
}

void CUnitsDlg::on_bracketsChkBox_clicked(bool checked){
  useBrackets=checked;
}

void CUnitsDlg::getTwinScales(bool isTwinScale_){
    ui->ryUnitEdit->setEnabled(isTwinScale_);
}

void CUnitsDlg::setUseBrackets(bool useBrackets_){
    useBrackets=useBrackets_;
    ui->bracketsChkBox->setChecked(useBrackets);
}

void CUnitsDlg::setXUnit(QString xUnit_){
    xUnit=xUnit_;
    ui->xUnitEdit->setText(xUnit_);
}


void CUnitsDlg::on_xUnitEdit_editingFinished(){
    xUnit=ui->xUnitEdit->text();
}
void CUnitsDlg::on_ryUnitEdit_editingFinished(){
    ryUnit=ui->ryUnitEdit->text();
}
void CUnitsDlg::on_yUnitEdit_editingFinished(){
    yUnit=ui->yUnitEdit->text();
}



void CUnitsDlg::on_buttonBox_clicked(QAbstractButton *button){
  if(button->text()=="Reset"){
    doReset=true;
    ui->xUnitEdit->setText("");
    ui->yUnitEdit->setText("");
    ui->ryUnitEdit->setText("");
  }
}


void CUnitsDlg::showEvent(QShowEvent *){
    doReset=false;
}


void CUnitsDlg::on_buttonBox_accepted()
{
    useSmartUnits=ui->smartUnitsCB->isChecked();
}
