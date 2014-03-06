#ifndef FUZZWIN_GUI_H
#define FUZZWIN_GUI_H

#include <QtWidgets/QMainWindow>
#include "ui_fuzzwin_gui.h"

class FUZZWIN_GUI : public QMainWindow
{
    Q_OBJECT

public:
    FUZZWIN_GUI(QWidget *parent = 0);
    ~FUZZWIN_GUI();

private:
    Ui::FUZZWIN_GUIClass ui;

public slots:
    void on_Q_selectPin_clicked(); // selection du r�pertoire de PIN
    void on_Q_selectZ3_clicked(); // s�lection du r�pertoire de Z3
    void on_Q_selectInitialInput_clicked(); // s�lection de l'entr�e initiale
    void on_Q_selectTarget_clicked();       // s�lection du programme cible
    void on_Q_selectResultsDir_clicked();   // s�lection du r�pertoire de r�sultats
};

#endif // FUZZWIN_GUI_H
