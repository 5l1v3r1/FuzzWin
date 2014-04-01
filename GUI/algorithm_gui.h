#pragma once
#include <QtCore\QObject>
#include <QDateTime>

#include "../ALGORITHME/algorithm.h"

#define TIMESTAMP     QDateTime::currentDateTime().time().toString("[HH:mm:ss:zzz] ")
#define LINEFEED      QString("<br>")
#define TEXTRED(x)    QString("<font color=\"Red\">"##x##"</font>")
#define TEXTGREEN(x)  QString("<font color=\"Green\">"##x##"</font>")

class ArgumentsForAlgorithm // structure pour initialiser l'algo en mode GUI
{
public:
    /* donnn�es de session */
    std::string _resultsDir;     // dossier de r�sultats
    std::string _firstInputPath; // premi�re entr�e test�e
    std::string _targetPath;     // ex�cutable fuzz�
    
    /* outils externes */
    std::string _z3Path;     // chemin vers le solveur Z3
    std::string _cmdLinePin; // ligne de commande pr�-remplie pour PIN

    /* Options pour le fuzzing */
    UINT32  _maxExecutionTime;  // temps maximal d'execution 
    UINT32  _maxConstraints;    // nombre maximal de contraintes � r�cup�rer
    bool    _keepFiles;         // les fichiers sont gard�s dans le dossier de resultat
    bool    _computeScore;      // option pour calculer le score de chaque entr�e
    bool    _verbose;           // mode verbeux
    bool    _timeStamp;         // ajout de l'heure aux lignes de log
    bool    _hashFiles;         // calcul du hash de chaque entr�e pour �viter les collisions
    bool    _traceOnly;         // simple trace d'ex�cution de l'entr�e initiale
    std::string _bytesToTaint;      // intervalles d'octets � surveiller 
};

// cr�ation de la classe algorithme adapt� � la ligne de commande

class FuzzwinAlgorithm_GUI : public QObject, public FuzzwinAlgorithm 
{    
    Q_OBJECT
public:
    FuzzwinAlgorithm_GUI(OSTYPE osType, ArgumentsForAlgorithm *pArgs);

private:
    /**  impl�mentation des m�thodes virtuelles pures de log **/

    void log(const std::string &msg) const;        // envoi en console
    void logVerbose(const std::string &msg) const; // idem en mode verbose
    void logVerboseEndOfLine() const;
    void logEndOfLine() const;
    void logTimeStamp() const;
    void logVerboseTimeStamp() const;

signals: // signaux �mis par l'algorithme vers la GUI
    void sendToGui(const QString&) const;
    void sendAlgorithmFinished();
    void sendTraceOnlyFinished();
    void pauseIsEffective();
    void setFaultsFound(int);

public slots:
    
    /*** impl�mentation des m�thodes virtuelles pures de contr�le de l'algorithme ***/
    void algorithmFinished(); 
    void notifyAlgoIsPaused();
    void faultFound();
    void algorithmTraceOnlyFinished();

    // wrappers permettant d'appeler les m�thodes de l'algorithme parent �ponymes
    void wrapStartFromGui();
    void wrapStopFromGui();
    void wrapPauseFromGui();
};