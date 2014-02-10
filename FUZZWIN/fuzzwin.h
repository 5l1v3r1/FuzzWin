#pragma once
#pragma warning (disable:4129) // s�quence d'�chappement de l'expression rationnelle

#include <windows.h>
#include <process.h>

#include <iostream>	// cin et cout
#include <cstdio>   // remove (effacement fichier)
#include <cstdint>  // uint8_t etc...
#include <fstream>	// ofstream / ifstream
    
#include <string>	
#include <vector>
#include <regex>
#include <set> // table de hachage
#include <clocale>  // pour passage de la ligne de commande en francais

static const std::string infoHeader
    (   
    "; **************************************************\n" 
    "; *  FUZZWIN : FUZZER AUTOMATIQUE SOUS WINDOWS     *\n" 
    "; *  v1.0 (c) SEBASTIEN LECOMTE 09/01/2014         *\n"
    "; *  PIN Version 2.13 kit 62732 et Z3 version 4.3  *\n"
    "; **************************************************\n" 
    );

static const std::string helpMessage
    (
    "\n"
    "FuzzWin - Fuzzer automatique sous plateforme Windows\n"
    "\n"
    "Usage:  fuzzwin.exe -t <targetExe> - i <firstInput> [options]\n"
    "\n"
    "Options:\n"
    "--help        \t -h : affiche ce message\n"
    "--keepfiles   \t -k : conserve les fichiers interm�diaires\n"
    "--range       \t -r : intervalles d'octets � marquer (ex: 1-10;15;17-51)\n"
    "--dir         \t -d : r�pertoire de sortie (d�faut : './results/')\n"
    "--maxconstraints -c : nombre maximal de contraintes � r�cuperer\n"
    "--maxtime     \t -m : temps limite d'ex�cution de l'ex�cutable �tudie\n"
    "--score       \t -s : calcul du score de chaque fichier\n"
    "--verbose     \t -v : mode verbeux\n"
    );

typedef uint8_t		UINT8;
typedef uint32_t	UINT32;
typedef uint64_t	UINT64;
typedef std::set<size_t> HashTable; // stockage des hashes des fichiers d�j� g�n�r�s

// solutions fournies par le solveur sont du type
/* define OFF?? (_BitVec 8) 
      #x??    */ 
#define parseZ3ResultRegex "OFF(\\d+).*\r\n.*([0-9a-f]{2})"

// codes d�finissant le type d'OS pour la d�termination des num�ros d'appels syst�mes
// Le type d'OS est d�termin� par fuzzwin.exe et pass� en argument au pintool
enum OSTYPE 
{
    HOST_X86_2000,
    HOST_X86_XP,
    HOST_X86_2003,

    HOST_X86_VISTA_SP0, // pour cette version, le syscall 'setinformationfile' n'est pas le meme que pour les autres SP...
    HOST_X86_VISTA,
    HOST_X86_2008 = HOST_X86_VISTA,   // les index des syscalls sont les m�mes
    HOST_X86_2008_R2 = HOST_X86_2008, // les index des syscalls sont les m�mes
  
    HOST_X86_SEVEN,
    
    HOST_X86_WIN80,
    HOST_X86_2012 = HOST_X86_WIN80, 
    
    HOST_X86_WIN81,
    HOST_X86_2012_R2 = HOST_X86_WIN81, // a priori ce sont les memes
    
    BEGIN_HOST_64BITS,
    HOST_X64_BEFORE_WIN8 = BEGIN_HOST_64BITS,
    HOST_X64_WIN80,
    HOST_X64_WIN81,
    HOST_UNKNOWN
};

class CGlobals 
{
public:   
    std::string resultDir;      // dossier de r�sultats
    std::string faultFile;      // fichier texte retracant les fautes trouv�es
    std::string targetPath;     // ex�cutable fuzz�
    std::string bytesToTaint;   // intervelles d'octets � surveiller
    std::string firstInputPath; // entr�e initiale

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
        maxConstraints(0)    // pas de limite dans le nombre de contraintes
        {}

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
};

extern CGlobals *pGlobals;

std::string initialize(int argc, char** argv);