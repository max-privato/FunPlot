#-------------------------------------------------
#
# Project created by QtCreator 2013-06-05T20:22:54
#
#-------------------------------------------------

QT       += core gui svg printsupport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = funPlot
TEMPLATE = app


SOURCES += main.cpp\
        MainWindow.cpp \
    CLineChart.cpp \
    matrix.cpp \
    CLineCalc.cpp \
    CPlotOptions.cpp  \
    CScaleDlg.cpp  \
    info.cpp \
    CUnitsDlg.cpp

HEADERS  += MainWindow.h\
    CLineChart.h \
    matrix.h \
    CLineCalc.h \
    CPlotOptions.h \
    CScaleDlg.h \
    info.h \
    CUnitsDlg.h

FORMS    += MainWindow.ui \
    CPlotOptions.ui \
    CScaleDlg.ui \
    info.ui \
    CUnitsDlg.ui

RESOURCES =  Images.qrc
# ICON = funPlot.icns
RC_ICONS = funPlot.ico

