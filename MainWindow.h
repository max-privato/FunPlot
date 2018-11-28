#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "CLineCalc.h"
#include "CLineChart.h"
#include "CPlotOptions.h"
#include "CScaleDlg.h"
#include "info.h"


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
protected:
    void showEvent(QPaintEvent *ev_);

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_plotBtn_clicked();
    void on_diffTBtn_clicked();
    void on_infoBtn_clicked();
    void on_interpolateBox_clicked(bool checked);
    void on_lineEdit_returnPressed();
    void on_dataTBtn_clicked(bool checked);
    void chartValuesChanged(SXYValues values, bool hDifference, bool vDifference);
    void on_titleBtn_clicked(bool checked);
    void on_optionsBtn_clicked();

    void on_scaleTBtn_clicked();

private:
    bool exactMatch;
    float *x;
    float **y; //matrice delle funzioni da elaborare
    float **yRes; //matrice (un'unica riga dei risultati
    CLineCalc lineCalc;
    CLineChart *lineChart;
    CPlotOptions *plotOpsWin;
    CScaleDlg *myScaleDlg;
    Ui::MainWindow *ui;


};

#endif // MAINWINDOW_H
