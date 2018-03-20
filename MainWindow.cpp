#include <QMessageBox>
#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "matrix.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
//    move(200,200);
    ui->setupUi(this);
    ui->plotBtn->setDefault(true);
    connect(ui->lineChart,SIGNAL(valuesChanged(SXYValues,bool,bool)),this, SLOT(chartValuesChanged(SXYValues,bool,bool)));
    ui->lineChart->FLinearInterpolation=false;
    ui->lineChart->FLegend=false;
    ui->interpolateBox->setVisible(false);
    ui->xValueLbl->setVisible(false);
    ui->yValueLbl->setVisible(false);
    plotOpsWin = new CPlotOptions(this);
    x=NULL;
    y=NULL;
    yRes=NULL;
    //Assegno alla finestra delle opzioni i valori predefiniti.
    plotOpsWin->prepare(false,ptLine,stLin, stLin,pwAuto);
    myScaleDlg =new CScaleDlg(this);
    myScaleDlg->setWindowTitle("Plot scale setup");

}

MainWindow::~MainWindow()
{
    delete ui;
    delete x;
    DeleteFMatrix(y);
    DeleteFMatrix(yRes);
}

void MainWindow::chartValuesChanged(SXYValues values, bool hDifference, bool vDifference){
    QString Msg;
    Msg=Msg.setNum(values.X[0],'g',4);
    if(hDifference)
      Msg= "*"+Msg;
    ui->xValueLbl->setText(Msg);
    Msg=Msg.setNum(values.Y[0][0],'g',4);
    if(vDifference) Msg= "*"+Msg;
    ui->yValueLbl->setText(Msg);
}

void MainWindow::on_diffTBtn_clicked()
{
    if(ui->lineChart->giveActiveDataCurs()==0)return;
    if(ui->lineChart->giveActiveDataCurs()==1){
        ui->lineChart->setActiveDataCurs(2);
        return;
    }
    if(ui->lineChart->giveActiveDataCurs()==2){
        ui->lineChart->setActiveDataCurs(1);
    }
}


void MainWindow::on_interpolateBox_clicked(bool checked)
{
    ui->lineChart->FLinearInterpolation=checked;
}

void MainWindow::on_plotBtn_clicked()
{
   bool ok;
   float xmin, xmax;
   int points=ui->nPointsEdit->text().toInt(&ok);
   if(points<5){
     QMessageBox::information(this,"","number of points too low;"
                              " the minimum value of 5 is set");
     points=5;
   }
   delete x;
   x=new float[points];
   float **xMatr, *xRow0;
   xRow0=x;
   xMatr=&xRow0;
   xmin=ui->xMinEdit->text().toFloat(&ok);
   if(!ok){
     QMessageBox::information(this,"","Invalid number in the x min field.");
     return;
   }
   xmax=ui->xMaxEdit->text().toFloat(&ok);
   if(!ok){
     QMessageBox::information(this,"","Invalid number in the x max field.");
     return;
   }
   QString line;
   QString err;
   QList <QString> names;

   DeleteFMatrix(y);
   DeleteFMatrix(yRes);

   yRes=CreateFMatrix(1,points);
   names.append("x");  //nome della variabile x
   float step=(xmax-xmin)/(points-1);
   for(int i=0; i<points; i++) {
    x[i]=xmin+i*step;
}
   line=ui->lineEdit->text().toLatin1();
   if(line.length()<1)return;
   //Devo passare alla seguente getline come ultimo argomento un puntatore di puntatore a float che definisce la matrice dei valori delle variabili letterali. Però in funPlot l'unica variabile è "x", quindi passo un puntatore al nome di x.
   err=lineCalc.getLine(line,names,xMatr);
   if(lineCalc.noVariables){
       QMessageBox::warning(this,"funPlot","Error: the inputted string does not contain \"x\"");
       return;
   }

   if(err!=""){
       QMessageBox::warning(this,"funPlot",err);
       return;
   }

   for(int i=0; i<points; i++){
     yRes[0][i]=lineCalc.compute(i);
     if(lineCalc.ret.length()>0)break;
   }
   if(lineCalc.ret.length()>0){
       QMessageBox::warning(this, "funPlot", "ERROR\n"+lineCalc.ret);
       return;
   }

   ui->lineChart->getData(x,yRes,points);
   ui->lineChart->plot();
   ui->dataTBtn->setEnabled(true);
   ui->optionsBtn->setEnabled(true);
   ui->titleBtn->setEnabled(true);
   ui->scaleTBtn->setEnabled(true);
}

void MainWindow::on_infoBtn_clicked()
{
    Info info;
    info.exec();
}

void MainWindow::on_lineEdit_returnPressed()
{
   on_plotBtn_clicked();
}

void MainWindow::showEvent(QPaintEvent * )
{
   on_plotBtn_clicked() ;
}

void MainWindow::on_dataTBtn_clicked(bool checked)
{
   int numOfTotPlots=1;
    if(checked){
        ui->lineChart->setActiveDataCurs(1);
        if(numOfTotPlots>1){
            ui->xValueLbl->setVisible(false);
            ui->yValueLbl->setVisible(false);
            ui->interpolateBox->setVisible(false);
        }else{
            ui->xValueLbl->setVisible(true);
            ui->yValueLbl->setVisible(true);
            ui->interpolateBox->setVisible(true);
        }
    }else{
        ui->lineChart->setActiveDataCurs(0);
        ui->xValueLbl->setVisible(false);
        ui->yValueLbl->setVisible(false);
        ui->interpolateBox->setVisible(false);
    }
    ui->diffTBtn->setEnabled(checked);

}

void MainWindow::on_titleBtn_clicked(bool checked)
{
      if(checked)
        ui->lineChart->enableTitle();
      else
        ui->lineChart->disableTitle();
}

void MainWindow::on_optionsBtn_clicked()
{
  SPlotOptions opt;
  plotOpsWin->exec();
  //recupero i dati scelti dall'utente:
  opt=plotOpsWin->giveData();
  ui->lineChart->setXScaleType(opt.xScaleType); //{stLin, stDB, stLog}
  ui->lineChart->setYScaleType(opt.yScaleType); //{stLin, stDB, stLog}
  ui->lineChart->xGrid=opt.grids;
  ui->lineChart->yGrid=opt.grids;
  ui->lineChart->plotType=opt.plotType; //{ptLine,ptBar, ptSwarm}
  ui->lineChart->setPlotPenWidth(opt.penWidth); //{pwThin, pwThick, pwAuto}
  if(plotOpsWin->swSizeIsPixel)
    ui->lineChart->swarmPointSize=ssPixel;
  else
    ui->lineChart->swarmPointSize=ssSquare;
  if(!plotOpsWin->accepted)return;
  if(!opt.autoAxisFontSize){
    ui->lineChart->fontSizeType=CLineChart::fsFixed;
    ui->lineChart->fixedFontPx=opt.axisFontSize;
  } else{
      ui->lineChart->fontSizeType=CLineChart::fsAuto;
  }
  //Operazioni di ritracciamento del grafico
  ui->lineChart->designPlot();
  if(ui->lineChart->zoomed)
    ui->lineChart->plot(false);
  else
    ui->lineChart->plot(true);
}

void MainWindow::on_scaleTBtn_clicked()
{
    //Preparazione prima della visualizzazione della scheda:
    myScaleDlg->getDispRect(ui->lineChart->giveDispRect());
    SAllLabels labels=ui->lineChart->giveAllLabels();
    myScaleDlg->getAllLabels(ui->lineChart->giveAllLabels());

    //Visualizzazione della scheda fino a che non metto dati corretti o faccio Cancel:
    QString ret;
    do{
      int result=myScaleDlg->exec();
      if (result==QDialog::Rejected)return;
      ret=myScaleDlg->validDispRect();
      if(ret!="")  QMessageBox::warning(this," ",ret);
    }while (ret!="");
    ui->lineChart->zoomed=true;
    ui->lineChart->setDispRect(myScaleDlg->giveDispRect());
    if(myScaleDlg->labelOverride){
       ui->lineChart->getAllLabels(myScaleDlg->giveAllLabels());
       ui->lineChart->labelOverride=true;
    }
    ui->lineChart->wIsOmega=myScaleDlg->giveWisOmega();
    ui->lineChart->exactMatch=myScaleDlg->exactMatch;
    ui->lineChart->plot(false);
    ui->lineChart->markAll();
}
