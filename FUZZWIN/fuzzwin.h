#pragma once
#pragma warning (disable:4129) // s�quence d'�chappement de l'expression rationnelle

#include <process.h>

#include <iostream>	// cin et cout
#include <cstdio>   // remove (effacement fichier)
#include <cstdint>  // uint8_t etc...
#include <fstream>	// ofstream / ifstream
#include <string>	
#include <regex>
#include <set> // table de hachage

#include "utilities.hpp"

typedef uint8_t		     UINT8;
typedef uint32_t	     UINT32;
typedef uint64_t	     UINT64;
typedef std::set<size_t> HashTable; // stockage des hashes des fichiers d�j� g�n�r�s

/* solutions fournies par le solveur sont du type
   define OFF?? (_BitVec 8) 
      #x??    */ 
static std::string parseZ3ResultRegex("OFF(\\d+).*\r\n.*([0-9a-f]{2})");

// log de l'ex�cution (simple et verbeux)
#define LOG(x) { std::cout << x; }
#define VERBOSE(x) { if (pGlobals->verbose) LOG(x) }

class CGlobals 
{
public:   
    std::string resultDir;      // dossier de r�sultats
    std::string faultFile;      // fichier texte retracant les fautes trouv�es
    std::string targetPath;     // ex�cutable fuzz�
    std::string bytesToTaint;   // intervalles d'octets � surveiller
    std::string firstInputPath; // entr�e initiale
    std::string z3Path;         // chemin vers le solveur Z3

    std::string cmdLinePin;	    // ligne de commande du pintool, pr�-r�dig�e 

    OSTYPE osType;
    
    bool keepFiles;         // les fichiers sont gard�s dans le dossier de resultat
    bool computeScore;      // option pour calculer le score de chaque entr�e
    bool verbose;           // mode verbeux
    
    UINT32 maxExecutionTime; // temps maximal d'execution 
    UINT32 maxConstraints;   // nombre maximal de contraintes � r�cup�rer

    HANDLE hZ3_process;	// handle du process Z3
    HANDLE hZ3_thread;	// handle du thread Z3
    HANDLE hZ3_stdout;	// handle du pipe correspondant � la sortie de Z3(= son stdout)
    HANDLE hZ3_stdin;	// handle du pipe correspondant � l'entr�e de Z3 (= son stdin)
    HANDLE hReadFromZ3;	// handle pour lire les r�sultats de Z3
    HANDLE hWriteToZ3;	// handle pour envoyer des donn�es vers Z3

    HANDLE hPintoolPipe; // handle du named pipe avec le pintool Fuzzwin,

    const std::regex regexModel; // pour parser les resultats du solveur

    std::hash<std::string> fileHash ; // hash d'une string : op�rateur () renvoie un size_t    

    CGlobals() : 
        regexModel(parseZ3ResultRegex, std::regex::ECMAScript),
        maxExecutionTime(0), // aucun maximum de temps
        maxConstraints(0),   // pas de limite dans le nombre de contraintes
        // initialisation par d�faut des autres variables
        osType(HOST_UNKNOWN),
        keepFiles   (false),
        computeScore(false),
        verbose     (false),
        hZ3_process (nullptr),
        hZ3_stdin   (nullptr),
        hZ3_stdout  (nullptr),
        hZ3_thread  (nullptr),
        hReadFromZ3 (nullptr),
        hWriteToZ3  (nullptr),
        hPintoolPipe(nullptr)  {}

    ~CGlobals() 
    {
        // fermeture du processus Z3
        CloseHandle(this->hZ3_process); CloseHandle(this->hZ3_thread);
        // fermeture des diff�rents tubes de communication avec Z3
        CloseHandle(this->hZ3_stdout); 	CloseHandle(this->hZ3_stdin);
        CloseHandle(this->hReadFromZ3);	CloseHandle(this->hWriteToZ3);
        // fermeture des tubes nomm�s avec le pintool Fuzzwin
        CloseHandle(this->hPintoolPipe);
    }

    void buildPinCmdLine(const std::string &pin_X86,     const std::string &pin_X64, 
                         const std::string &pintool_X86, const std::string &pintool_X64) 
    {
        // si OS 32 bits, pas la peine de sp�cifier les modules 64bits
        if (this->osType < BEGIN_HOST_64BITS) 
        {
            /* $(pin_X86) -t $(pintool_X86) -- $(targetFile) %INPUT% (ajout� a chaque fichier test�) */
            this->cmdLinePin = 
                '"'          + pin_X86 \
                + "\" -t \"" + pintool_X86       \
                + "\" -- \"" + this->targetPath + "\" ";
        }
        else
        {
            /* $(pin_X86) -p64 $(pin_X64) -t64 $(pintool_X64) -t $(pintool_X86) 
            -- $(targetFile) %INPUT% (ajout� a chaque fichier test�) */
            this->cmdLinePin = 
                '"'            + pin_X86  \
                + "\" -p64 \"" + pin_X64  \
                + "\" -t64 \"" + pintool_X64  \
                + "\" -t \""   + pintool_X86  \
                + "\" -- \""   + this->targetPath + "\" ";
        }
    }
};

extern CGlobals *pGlobals;