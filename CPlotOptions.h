#ifndef CPLOTOPTIONS_H
#define CPLOTOPTIONS_H

#include <QDialog>
#include "CLineChart.h"

namespace Ui {
class CPlotOptions;
}

struct SPlotOptions{
    EScaleType xScaleType, yScaleType;
    bool grids, autoAxisFontSize, autoLegendFontSize;
    int axisFontSize;
    EPlotType plotType;
    EPlotPenWidth penWidth;
};

class CPlotOptions : public QDialog
{
    Q_OBJECT
    
public:
    bool accepted,
         customPoints; //true se l'utente ha scelto custo points per le scale
    bool swSizeIsPixel; //se true ed è richiesto plot di tipo swarm, per esso la dimensione del puntino è un semplice plixel
    explicit CPlotOptions(QWidget *parent = 0);
    SPlotOptions giveData();
    void prepare(bool grids, EPlotType plotType, EScaleType xScaleType, EScaleType yScaleType, EPlotPenWidth penWidth);
    ~CPlotOptions();


private slots:
    void on_linesTypeBtn_clicked();
    void on_barsTypeBtn_clicked();
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();
    void on_dotsTypeBtn_clicked(bool checked);
    void on_linesTypeBtn_clicked(bool checked);
    void on_pointPixelBtn_clicked(bool checked);
    void on_pointSquareBtn_clicked(bool checked);
//    void on_fixedAxisFontRBtn_clicked(bool checked);
    void on_fontPointCombo_currentIndexChanged(const QString);
    void on_autoAxisFontBtn_clicked();

private:
    Ui::CPlotOptions *ui;
};

#endif // CPLOTOPTIONS_H
