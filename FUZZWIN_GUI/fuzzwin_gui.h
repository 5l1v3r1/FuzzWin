#pragma once

#include <QtCore/QVariant>
#include <QtCore/QProcess>
#include <QtCore/QThread>

#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QTimeEdit>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QScrollBar>

#include <QtGui/QStandardItemModel>

#include "globals.h"
#include "fuzzwin_algorithm.h"  // classe fuzzwinThread
#include "fuzzwin_widgets.h"
#include "fuzzwin_modelview.h"
#include "qhexview.h"

class FUZZWIN_GUI : public QMainWindow
{
    Q_OBJECT

public:
    FUZZWIN_GUI();
    ~FUZZWIN_GUI();

    // test de la version de l'OS et de la pr�sence du pintool
    // d�termine �galement le marquage des boutons au d�marrage
    int testConfig();  

private:
    /***** PARTIE NON_GUI *****/
    
    QProcessEnvironment _env;         // variables d'environnement du processus  
    QThread *_pFuzzwinThread;         // thread de l'algo SAGE
    FuzzwinAlgorithm *_pFuzzwinAlgo;  // algorithme sage (lanc� dans le thread)

    QString _pinPath_X86, _pinPath_X64; // chemin des ex�cutables PIN  (32/64bits)
    QString _pintool_X86, _pintool_X64; // chemin des DLL des pintools (32/64bits)
    QString _z3Path;                    // chemin vers Z3

    OSTYPE _osType;     // type d'OS Natif (version de Windows + archi 32/64)

    /***** PARTIE GUI *****/
    // taille des widgets
    QSizePolicy _fixedSizePolicy;

    // disposition globale
    QWidget     *_centralWidget;
    QGridLayout *_gCentralLayout;    
    QFrame      *_Vline;
    // Configuration : PIN et Z3
    QGroupBox     *_groupConfiguration;
    QHBoxLayout   *_hLayout1;
        FuzzwinButton *_selectPin;
        FuzzwinButton *_selectZ3;
    // Donn�es concernant la session
    QGroupBox     *_groupSession;
    QVBoxLayout   *_vLayout1;
        FuzzwinButton *_selectInitialInput;
        FuzzwinButton *_selectTarget;
        FuzzwinButton *_selectResultsDir;
    // Options pour la session
    QGroupBox   *_groupOptions;
    QGridLayout *_gLayout1;
        QCheckBox   *_maxConstraintsEnabled;
        QSpinBox    *_maxConstraints;
        QCheckBox   *_maxTimeEnabled;
        QTimeEdit   *_maxTime;
        QCheckBox   *_bytesToTaintEnabled;
        QLineEdit   *_listOfBytesToTaint;
        QCheckBox   *_scoreEnabled;
        QCheckBox   *_verboseEnabled;
        QCheckBox   *_keepfilesEnabled;
        QCheckBox   *_traceOnlyEnabled;
    // boutons de commande
    QPushButton *_startButton;
    QPushButton *_stopButton;
    QPushButton *_quitButton;
    QPushButton *_aboutButton;
    // partie r�sultats
    QGroupBox   *_groupResultats;
    QGridLayout *_gLayout2;
        QLabel      *_labelWorklistSize;
        QSpinBox    *_worklistSize;
        
        QLabel      *_labelElapsedTime;
        QLabel      *_labelInitialInput;
        InitialInputLineEdit   *_initialInput;    
        QLabel      *_labelTargetPath;
        TargetPathLineEdit   *_targetPath;
        QLabel      *_labelResultsDir;
        ResultsDirLineEdit   *_resultsDir;
        QTimeEdit   *_elapsedTime;
    // partie tabs
    QTabWidget  *_Tabs;
        QWidget     *_logTab;
        QHBoxLayout *_hLayout2;
        QTextEdit   *_logWindow;
        QWidget     *_entriesTab;
        QVBoxLayout *_vLayout2;
        QTreeView   *_inputsView;
        TreeModel   *_inputsModel;
    // boutons de sauvegarde
    QPushButton *_saveLogButton;
    QPushButton *_saveConfigButton;

    // initialisation des diff�rents groupes d'objets et connection signal/slots
    // Fonctions appel�es par le constructeur

    void initGroupConfiguration();
    void initGroupSession();
    void initGroupOptions();
    void initGroupResultats();
    void initButtons();
    void initLines();
    void connectSignalsToSlots();   

    // r�impl�mentation de l'�v�nement "Close" pour demander confirmation
    void closeEvent(QCloseEvent *e);    
 
signals:
    void launchAlgorithm();

public slots:
    void selectPin_clicked(); // selection du r�pertoire de PIN
    void selectZ3_clicked(); // s�lection du r�pertoire de Z3
    void selectInitialInput_clicked(); // s�lection de l'entr�e initiale
    void selectTarget_clicked();       // s�lection du programme cible
    void selectResultsDir_clicked();   // s�lection du r�pertoire de r�sultats
    void go_clicked();
    void stop_clicked();
    void saveLog_clicked(); 
    void saveConfig_clicked(); 
    void quitFuzzwin();
    
    void checkPinPath(QString path);  // v�rification de l'int�grit� du dossier racine de PIN
    void checkZ3Path(QString path);
    void checkKindOfExe(const QString &path); // v�rification du type d'ex�cutable s�lectionn�
    void checkDir(const QString &path); // v�rification du dossier de r�sultats (effacement si besoin)
        
    void logWithTime(const QString &msg);

    void algoFinished(quint32 nbFautes);
    void autoScrollLogWindow();

    void updateInputView(CInput input);
    void testButtons();
};
