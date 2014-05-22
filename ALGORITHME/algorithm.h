#pragma once

#include <mutex>
#include <regex>
#include <set>
#include <windows.h>

#include "md5.h"     // hash des fichiers, r�sultat en b64
#include "osType.h"  // types d'OS natif
#include "CInput.h"  // calsse de descrpition d'une entr�e de test

/* solutions fournies par le solveur sont du type
define OFF?? (_BitVec 8) 
#x??    */ 
#ifndef parseZ3ResultRegex
#define parseZ3ResultRegex "OFF(\\d+).*\r\n.*([0-9a-f]{2})"
#endif

typedef std::set<std::string> HashTable; // stockage des hashes (MD5 base64) des fichiers d�j� g�n�r�s

static const std::string solverConfig
    (
    "(set-option :auto-config true)\n"  \
    "(set-logic QF_AUFBV)"
    );

static const std::string infoHeader
    (   
    "; **************************************************\n" 
    "; *  FUZZWIN : FUZZER AUTOMATIQUE SOUS WINDOWS     *\n" 
    "; *  v1.3 (c) Sebastien LECOMTE 26/03/2014         *\n"
    "; *  PIN Version 2.13 kit 62732 et Z3 version 4.3  *\n"
    "; **************************************************\n" 
    );

// test du type d'ex�cutable (32 ou 64bits). Si non support� => retour n�gatif (-1)
int getKindOfExecutable(const std::string &targetPath);

// chemin complet d'un fichier (sans r�f�rence au dossier de travail)
std::string getAbsoluteFilePath(const std::string &s);

// d�termination du type l'OS dynamiquement, inspir� de l'exemple fourni sur MSDN
OSTYPE getNativeArchitecture();

/*********************************************/
/* ALGORITHME (DERIVEES DANS CMDLINE ET GUI) */
/*********************************************/

class FuzzwinAlgorithm
{
protected:
    enum ALGORITHM_STATUS
    {
        ALGORITHM_RUNNING = 0,  // situation nominale
        ALGORITHM_CREATED,      // classe cr��e, mais algo non encore ex�cut�
        ALGORITHM_PAUSED, 
        ALGORITHM_STOPPED,
        ALGORITHM_TRACEONLY_FINISHED
    } _status;

    OSTYPE   _osType;   // type d'OS natif

    const std::regex   _regexModel;     // pour parser les resultats du solveur

    std::string  _resultsDir;     // dossier de r�sultats
    std::string  _targetPath;     // ex�cutable fuzz�
    std::string  _firstInputPath; // premi�re entr�e test�e
 
    std::string  _z3Path;         // chemin vers le solveur Z3

    std::string  _cmdLinePin;   // ligne de commande du pintool, pr�-r�dig�e
    std::string  _cmdLineCheckScore;    // ligne de commande pour checkscore, pr�-r�dig�e
    std::string  _faultFile;    // fichier texte retracant les fautes trouv�es
    std::string  _formula;      // formule retourn�e par le pintool

    UINT32 _nbFautes;          // nombre de fautes trouv�es au cours de l'analyse

    bool   _keepFiles;         // les fichiers sont gard�s dans le dossier de resultat
    bool   _computeScore;      // option pour calculer le score de chaque entr�e
    bool   _verbose;           // mode verbeux
    bool   _timeStamp;         // ajout de l'heure aux lignes de log
    bool   _hashFiles;         // calcul du hash de chaque entr�e pour �viter les collisions
    bool   _traceOnly;         // mode 'traceOnly' : une seule ex�cution avec l'entr�e s�lectionn�e (pas de solveur)
    UINT32 _maxExecutionTime;  // temps maximal d'ex�cution (pour tuer le processus de la cible lanc� en d�bug)

    HANDLE  _hZ3_process;  // handle du process Z3
    HANDLE  _hZ3_thread;   // handle du thread Z3
    HANDLE  _hZ3_stdout;   // handle du pipe correspondant � la sortie de Z3(= son stdout)
    HANDLE  _hZ3_stdin;    // handle du pipe correspondant � l'entr�e de Z3 (= son stdin)
    HANDLE  _hReadFromZ3;  // handle pour lire les r�sultats de Z3
    HANDLE  _hWriteToZ3;   // handle pour envoyer des donn�es vers Z3
    HANDLE  _hPintoolPipe; // handle du named pipe avec le pintool Fuzzwin
    HANDLE  _hTimer;       // handle du timer (temps maximal d'ex�cution)

    ListOfInputs _workList;       // liste des fichiers � traiter 
    UINT32       _numberOfChilds; // num�rotation des fichiers d�riv�s
    CInput*      _pCurrentInput;  // entr�e en cours de test
    
    HashTable    _hashTable;      // table de hachage des fichiers d�j� trait�s

    std::mutex   _algoMutex;

    /*** METHODES PROTECTED ***/
    
    /* redefinition de la sortie des resultats dans les classes d�riv�es */

    virtual void log(const std::string &msg)        const = 0;
    virtual void logEndOfLine()                     const = 0;
    virtual void logTimeStamp()                     const = 0;

    virtual void logVerbose(const std::string &msg) const = 0;
    virtual void logVerboseEndOfLine()              const = 0;
    virtual void logVerboseTimeStamp()              const = 0;

    /* partie algorithme pur (fuzzwin_algo.cpp) */
    void         algorithmSearch();  
    ListOfInputs expandExecution(); 
    size_t       sendNonInvertedConstraints(UINT32 bound);
    std::string  invertConstraint(const std::string &constraint);
    void         updateRefCounts(CInput *pInput) const;   

    /* PARTIE Communication avec pintool (pintool_interface.cpp)*/

    int          sendArgumentToPintool(const std::string &argument) const;
    void         callPintool();
    int          createNamedPipePintool();
    std::string  getCmdLinePintool() const;

    /* PARTIE Communication avec solveur (solver_interface.cpp) */

    bool         createSolverProcess(const std::string &solverPath);    
    bool         sendToSolver(const std::string &data) const;
    bool         checkSatFromSolver() const;
    std::string  getModelFromSolver() const;

    /* PARTIE deboggage du programme (debug_interface.cpp) */
    
    DWORD        debugTarget(CInput *pNewInput);        
    std::string  getCmdLineDebug(const CInput *pNewInput) const;
    static void  CALLBACK timerExpired(LPVOID arg, DWORD, DWORD);

    /* PARTIE calcul du score (checkscore.cpp) */
    void         checkScore(CInput *pNewChild);
    std::string  getCmdLineCheckScore(const CInput *pNewChild) const;

public:

    explicit FuzzwinAlgorithm(OSTYPE osType);
    ~FuzzwinAlgorithm();

    // initialisation commune � toutes les classes d�riv�es : 
    // 1) cr�ation de la ligne de commande pr�-remplie pour le pintool
    // 2) initialisation de la liste de travail et du fichier de fautes
    // 3) cr�ation du pipe pintool et du processus du solveur
    // renvoie EXIT_FAILURE en cas de souci
    int finalizeInitialization(
        const std::string &pin_X86,     const std::string &pin_X64,
        const std::string &pintool_X86, const std::string &pintool_X64,
        const std::string &cmdLineOptions);

    // proc�dures de contr�le. 
    // Les m�thodes "finish" et "notification de mise en pause" sont sp�cifique cmdline ou GUI
    void run();
    void pause();                 
    void stop();   
    virtual void faultFound() = 0;
    virtual void algorithmFinished()  = 0;
    virtual void notifyAlgoIsPaused() = 0; 
    virtual void algorithmTraceOnlyFinished() = 0;
    
};