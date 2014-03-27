#pragma once

#include <string>   
#include <regex>
#include <list>
#include <set>
#include <fstream>

#include <windows.h>

#include "md5.h" // hash des fichiers, r�sultat en b64
#include "osType.h"

/* solutions fournies par le solveur sont du type
define OFF?? (_BitVec 8) 
#x??    */ 
#ifndef parseZ3ResultRegex
#define parseZ3ResultRegex "OFF(\\d+).*\r\n.*([0-9a-f]{2})"
#endif

class   CInput;
class   FuzzwinAlgorithm;

typedef std::list<CInput*>    ListOfInputs;
typedef unsigned char         UINT8;
typedef unsigned int          UINT32;
typedef unsigned long long    UINT64;
typedef std::set<std::string> HashTable; // stockage des hashes (MD5 base64) des fichiers d�j� g�n�r�s

static const std::string solverConfig
    (
    "(set-option :auto-config false)"   \
    "(set-option :produce-models true)" \
    "(set-option :print-success false)" \
    "(set-option :relevancy 0)"         \
    "(set-option :smtlib2_compliant true)"  \
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

class CInput
{
private:
    UINT32 _refCount;      // impl�mentation "basique" d'un sharedPtr
    UINT32 _bound;         // num�ro de contrainte invers�e qui a men� � la cr�ation du fichier
    UINT32 _exceptionCode; // code d'erreur engendr� par le fichier. 0 si aucun
    UINT32 _score;         // score de l'entr�e (couverture de code)
    std::string  _filePath;   // chemin vers le fichier
    std::string  _fileName;   // nom de fichier uniquement (sans le chemin)
    CInput*      _pFather;    // entr�e de laquelle cette entr�e est d�riv�e
    
public:
    // constructeur sp�cifique 1ere entr�e
    CInput(const std::string &firstInputPath, bool keepfiles);
    // cr�ation du 'nb' nouvel objet d�riv� de l'objet 'pFather' � la contrainte 'b'
    CInput(CInput* pFather, UINT64 nb, UINT32 b); 

    ~CInput() ;

    const bool   _keepFiles;  // si VRAI, en pas effacer physiquement le fichier � la destruction 

    CInput* getFather() const;

    // Accesseurs renvoyant les membres priv�s de la classe
    UINT32 getBound() const;
    UINT32 getScore() const;
    UINT32 getExceptionCode() const;
    const std::string& getFilePath() const;
    const std::string& getFileName() const;

    // num�ro de contrainte invers�e qui a donn� naissance � cette entr�e
    void setBound(const UINT32 b);
    // Affectation d'un score � cette entr�e
    void setScore(const UINT32 s);
    // num�ro d'Exception g�n�r� par cette entr�e
    void setExceptionCode(const UINT32 e);

    // gestion de la vie des objets "CInput" : refCount basique
    void   incRefCount();
    UINT32 decRefCount();
    UINT32 getRefCount() const;

    // renvoie la taille de l'entr�e en octets
    UINT32 getFileSize() const;

    // renvoie le chemin vers le fichier qui contiendra la formule SMT2
    // associ�e � l'execution de cette entr�e (option --keepfiles mise � TRUE)
    std::string getLogFile() const;

    // renvoie le contenu du fichier sous la forme de string
    std::string getFileContent() const;
}; // class CInput_common

// fonction de tri de la liste d'entr�es de tests
bool sortCInputs(CInput* pA, CInput* pB);

class FuzzwinAlgorithm
{
protected:
    OSTYPE   _osType;   // type d'OS natif

    const std::regex   _regexModel;     // pour parser les resultats du solveur

    std::string  _resultsDir;     // dossier de r�sultats
    std::string  _targetPath;     // ex�cutable fuzz�
    std::string  _bytesToTaint;   // intervalles d'octets � surveiller 
    std::string  _z3Path;         // chemin vers le solveur Z3

    std::string  _cmdLinePin;   // ligne de commande du pintool, pr�-r�dig�e
    std::string  _faultFile;    // fichier texte retracant les fautes trouv�es
    std::string  _formula;      // formule retourn�e par le pintool

    UINT32 _nbFautes;          // nombre de fautes trouv�es au cours de l'analyse
    UINT32 _maxExecutionTime;  // temps maximal d'execution 
    UINT32 _maxConstraints;    // nombre maximal de contraintes � r�cup�rer

    bool   _keepFiles;         // les fichiers sont gard�s dans le dossier de resultat
    bool   _computeScore;      // option pour calculer le score de chaque entr�e
    bool   _verbose;           // mode verbeux
    bool   _timeStamp;         // ajout de l'heure aux lignes de log
    bool   _hashFiles;         // calcul du hash de chaque entr�e pour �viter les collisions

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

    
    /*** METHODES PRIVEES (PROTECTED) ***/
    
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
    UINT32       sendNonInvertedConstraints(UINT32 bound);
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

public:

    explicit FuzzwinAlgorithm(OSTYPE osType);
    ~FuzzwinAlgorithm();
    void    start();
    UINT32  getNumberOfFaults() const;
};