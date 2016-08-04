#include "CScaleDlg.h"
#include "ui_CScaleDlg.h"

CScaleDlg::CScaleDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CScaleDlg)
{
    ui->setupUi(this);
    myLabelsDlg=new CLabelsDlg(this);
    exactMatch=false;
    labelOverride=false;
    wIsOmega=false;
}

void CScaleDlg::getAllLabels(SAllLabels all){
    allLabels=all;
    labelX=all.xLbl;
    labelYL=all.yLbl;
    labelYR=all.ryLbl;
}

void CScaleDlg::getDispRect(SFloatRect2 dispRect){
    ui->xMinEdit->setText(QString::number(dispRect.Left));
    ui->xMaxEdit->setText(QString::number(dispRect.Right));
    ui->yMinEdit->setText(QString::number(dispRect.LBottom));
    ui->yMaxEdit->setText(QString::number(dispRect.LTop));
}

SAllLabels CScaleDlg::giveAllLabels(){
    SAllLabels  all;
    all.xLbl=labelX;
    all.yLbl=labelYL;
    all.ryLbl=labelYR;
    return all;
}

SFloatRect2 CScaleDlg::giveDispRect(void){
    SFloatRect2 rect;
    rect.Left   =ui->xMinEdit->text().toFloat();
    rect.Right  =ui->xMaxEdit->text().toFloat();
    rect.LBottom=ui->yMinEdit->text().toFloat();
    rect.LTop   =ui->yMaxEdit->text().toFloat();
    return rect;
}


bool CScaleDlg::giveWisOmega(){
    return myLabelsDlg->giveWisOmega();
}


void CScaleDlg::on_exaMatchBox_clicked(bool checked)
{
    exactMatch=checked;
}


void CScaleDlg::on_labelsBtn_clicked()
{
    myLabelsDlg->getAllLabels(allLabels);
    int result=myLabelsDlg->exec();
    if(result==Accepted){
        SAllLabels all=myLabelsDlg->giveAllLabels();
        labelX=all.xLbl;
        labelYL=all.yLbl;
        labelYR=all.ryLbl;
        labelOverride=true;
        wIsOmega=myLabelsDlg->giveWisOmega();
    }else{
        labelOverride=false;
    }
}


QString CScaleDlg::validDispRect(){
  SFloatRect2 r=giveDispRect();
  if(r.Left>=r.Right) return "Max value must be larger than Min\non the X-axis";
  if(r.LTop<=r.LBottom) return "Max value must be larger than Min\non the Y-axis";
  return"";
}

CScaleDlg::~CScaleDlg()
{
    delete ui;
}

