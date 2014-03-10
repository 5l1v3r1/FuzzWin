#ifndef FUZZWIN_GUI_H
#define FUZZWIN_GUI_H

#include <QtWidgets/QMainWindow>
#include <QtCore/QProcess>
#include "ui_fuzzwin_gui.h"

class FUZZWIN_GUI : public QMainWindow, private Ui::FUZZWIN_GUIClass
{
    Q_OBJECT

public:
    enum ButtonStatus
    {
        button_OK,
        button_NON_OK
    };

    FUZZWIN_GUI(QWidget *parent = 0);
    ~FUZZWIN_GUI();

    void initialize();  // �tat des boutons au d�marrage
    void sendToLogWindow(const QString &msg);

private:
    QProcessEnvironment env;    // variables d'environnement du processus   

    bool isPinPresent;
    bool isZ3Present;
    bool isZ3Launched;

    
    void setButtonOk(QPushButton *b);
    void setButtonError(QPushButton *b);

    bool checkPinPath(QString &path);
    bool checkZ3Path (QString &path);
    

public slots:
    void on_GUI_selectPinButton_clicked(); // selection du r�pertoire de PIN
    void on_GUI_selectZ3Button_clicked(); // s�lection du r�pertoire de Z3
    void on_GUI_selectInitialInputButton_clicked(); // s�lection de l'entr�e initiale
    void on_GUI_selectTargetButton_clicked();       // s�lection du programme cible
    void on_GUI_selectResultsDirButton_clicked();   // s�lection du r�pertoire de r�sultats
};

extern FUZZWIN_GUI *window;

#endif // FUZZWIN_GUI_H
