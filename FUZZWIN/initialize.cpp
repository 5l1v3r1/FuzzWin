#include "solver.h" 

// PARSE COMMAND LINE : GETOPT_PP
// documentation : https://code.google.com/p/getoptpp/wiki/Documentation?tm=6
// seule modiifcation apport�e au code : d�sactivation du warning 4290
#include "getopt_pp.h"

// renvoie le r�pertoire ou se trouve l'executable
static std::string getExePath() 
{
    char buffer[MAX_PATH];
    GetModuleFileName(NULL, buffer, MAX_PATH);
    std::string::size_type pos = std::string(buffer).find_last_of("\\/");
    return (std::string(buffer).substr(0, pos + 1)); // antislash inclus
}

// test du type d'ex�cutable (32 ou 64bits). Si non support� => quitter
static int getKindOfExecutable(const std::string &targetPath)
{
    DWORD kindOfExe = 0;
    BOOL  result = GetBinaryType((LPCSTR) targetPath.c_str(), &kindOfExe);
    
    // erreur si fichier non ex�cutable ou si non trouv�
    if (!result) return (-1);
    else         return (kindOfExe);
}

// test de l'existence d'un fichier
static inline bool testFileExists(const std::string &name)
{
    std::ifstream f(name.c_str());
    bool isExists = f.good();
    f.close();
    
    return (isExists);
}

// chemin complet d'un fichier (sans r�f�rence au dossier de travail)
static std::string getAbsoluteFilePath(const std::string &s)
{
    char absolutepath[MAX_PATH];
    LPSTR* a = nullptr;

    // transformation du chemin relatif en chemin absolu
    GetFullPathName((LPCSTR) s.c_str(), MAX_PATH, absolutepath, a);

    return (std::string(absolutepath));
}

// d�termination de l'OS dynamiquement, inspir� de l'exemple fourni sur MSDN
// le type d'OS sera pass� en argument au pintool
// pour la surveillance des appels syst�mes
// NB : la version 8.1 du Windows Kit a d�sormais des fonctions sp�cifiques
// mais ne sera pas utilis� ici
static OSTYPE getNativeArchitecture()
{
    OSTYPE osType = HOST_UNKNOWN;

    // la fonction GetNativeSystemInfo retourne une structure avec notamment
    // 'wProcessorArchitecture' qui d�termine si OS 32 ou 64bits 
    SYSTEM_INFO si;
    ZeroMemory(&si, sizeof(SYSTEM_INFO));
    GetSystemInfo(&si);

    // GetVersionEx r�cup�re la version de l'OS pour fixer le num�ro des syscalls
    // la structure OSVERSIONINFOEX contient ce que l'on cherche � savoir
    OSVERSIONINFOEX osvi;
    ZeroMemory(&osvi, sizeof(osvi));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    GetVersionEx(reinterpret_cast<OSVERSIONINFO*>(&osvi));
    // version = 10*major + minor ; permet de comparer tout de suite les nombres
    int osVersion = (10 * osvi.dwMajorVersion) + osvi.dwMinorVersion;
    
    // isWow64Process d�termine si fuzzwin fonctionne en �mulation 64bits.
    BOOL bIsWow64 = FALSE;
    IsWow64Process(GetCurrentProcess(), &bIsWow64);

    // Architecture de l'OS = 64 bits, ou bien �mulation (Wow64)
    if ((PROCESSOR_ARCHITECTURE_AMD64 == si.wProcessorArchitecture)	|| bIsWow64)
    {
        // Avant Windows 8 (version 6.2), tous les OS 64bits
        // ont les m�mes tables pour les syscalls
        if (osVersion < 62)         osType = HOST_X64_BEFORE_WIN8;

        // Windows 8 : version 6.2
        else if (62 == osVersion)   osType = HOST_X64_WIN80;

        // pour Windows 8.1, getVersionEx est d�pr�ci� => TODO FAIRE AUTREMENT
        else if (63 == osVersion)   osType = HOST_X64_WIN81;
    }
    else if (PROCESSOR_ARCHITECTURE_INTEL == si.wProcessorArchitecture)	
    {
        switch (osVersion)	
        {
        case 50:  // Version 5.0 = Windows 2000
            osType = HOST_X86_2000;
            break;

        case 51:  // Version 5.1 = Windows XP
            osType = HOST_X86_XP;
            break;

        case 52:  // Version 5.2 = Server 2003. XP 64bits n'est pas consid�r� car on est en 32bits
            osType = HOST_X86_2003;
            break;

        case 60:  // Version 6.0 = Vista ou Server 2008
            if (VER_NT_WORKSTATION == osvi.wProductType) // Vista => tester le cas SP0
            {
                // le syscall 'NtSetInformationFile' diff�re entre SP0 et les autres SP
                // on teste donc si un service pack est pr�sent
                osType = (osvi.wServicePackMajor) ? HOST_X86_VISTA : HOST_X86_VISTA_SP0;
            }
            else osType = HOST_X86_2008;
            break;
       
        case 61:  // Version 6.1 = Seven ou Server 2008 R2
            osType = (VER_NT_WORKSTATION == osvi.wProductType) ? HOST_X86_SEVEN : HOST_X86_2008_R2;
            break;
     
        case 62:  // Version 6.2 = Windows 8 ou Server 2012
            osType = (VER_NT_WORKSTATION == osvi.wProductType) ? HOST_X86_WIN80 : HOST_X86_2012;
            break;

        case 63:  // Version 6.3 = Windows 8.1 ou Server 2012R2 (� voir car GetVersionEx d�pr�ci�e)
            osType = (VER_NT_WORKSTATION == osvi.wProductType) ? HOST_X86_WIN81 : HOST_X86_2012_R2;
            break;
      
        default:  // OS non reconnu donc non support� 
            break; 
        }
    }
    return (osType);
} // getNativeArchitecture

// Usage
void showHelp()
{
    std::cout << helpMessage;
}

//Effacement du contenu d'un r�pertoire sans l'effacer lui meme
// source StackOverflow : http://stackoverflow.com/questions/734717/how-to-delete-a-folder-in-c
static int deleteDirectory
    (const std::string &refcstrRootDirectory, bool bDeleteSubdirectories = true)
{
    bool            bSubdirectory = false;  // Flag, indicating whether subdirectories have been found
    HANDLE          hFile;                  // Handle to directory
    std::string     strFilePath;            // Filepath
    std::string     strPattern;             // Pattern
    WIN32_FIND_DATA FileInformation;        // File information

    strPattern = refcstrRootDirectory + "\\*.*";
    hFile = FindFirstFile(strPattern.c_str(), &FileInformation);

    if (hFile != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (FileInformation.cFileName[0] != '.')
            {
                strFilePath.erase();
                strFilePath = refcstrRootDirectory + "\\" + FileInformation.cFileName;

                if (FileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    if (bDeleteSubdirectories)
                    {
                        // Delete subdirectory
                        int iRC = deleteDirectory(strFilePath, bDeleteSubdirectories);
                        
                        if (iRC) return iRC;   
                    }
                    else bSubdirectory = true;
                }
                else
                {
                    // Set file attributes
                    if (FALSE == SetFileAttributes(strFilePath.c_str(), FILE_ATTRIBUTE_NORMAL)) return GetLastError();
                    // Delete file
                    if (FALSE == DeleteFile(strFilePath.c_str())) return GetLastError();
                }
            }
        } while (TRUE == FindNextFile(hFile, &FileInformation));

        // Close handle
        FindClose(hFile);

        DWORD dwError = GetLastError();
        if (dwError != ERROR_NO_MORE_FILES)  return dwError;
    }
    return 0;
}

// Proc�dure d'initialisation de FuzzWin
// parse la ligne de commande, lance le solveur, cr�� le pipe, etc...
std::string initialize(int argc, char** argv) 
{
    /********************************************************/
    /** r�cup�ration des arguments de la ligne de commande **/
    /********************************************************/
    
    // repertoire du launcher
    std::string exePath = getExePath(); 
    
    GetOpt::GetOpt_pp ops(argc, argv);

    // Option -h / --help : affichage de l'usage et retour imm�diat
    if (ops >> GetOpt::OptionPresent('h', "help"))
    {
        showHelp();
        exit(0);
    }

    // argument -t / --target : ex�cutable cible
    ops >> GetOpt::Option('t', "target", pGlobals->targetPath);  

    // argument -i // --initial : premier fichier d'entr�e
    ops >> GetOpt::Option('i', "initial", pGlobals->firstInputPath);
    
    // option -r / --range : liste type impression des octets a tester
    ops >> GetOpt::Option('r', "range", pGlobals->bytesToTaint); 
        
    // option -k / --keepfiles : conservation de tous les fichiers. Option pr�sente = activ�e
    ops >> GetOpt::OptionPresent('k', "keepfiles", pGlobals->keepFiles); 
   
    // option -s / --score : calcul du score de chaque nouveau fichier. Option pr�sente = activ�e
    ops >> GetOpt::OptionPresent('s', "score", pGlobals->computeScore);

    // option -v / --verbose : mode verbeux. Option pr�sente = activ�e
    ops >> GetOpt::OptionPresent('v', "verbose", pGlobals->verbose);

    // option -d // --destination : dossier de destination.
    ops >> GetOpt::Option('d', "dir", pGlobals->resultDir);

    // option -c // --maxconstraints : nombre maximal de contraintes
    ops >> GetOpt::Option('c', "maxconstraints", pGlobals->maxExecutionTime);
    
    // option -m // --maxtime : temps maximal d'execution d'une entree
    ops >> GetOpt::Option('m', "maxtime", pGlobals->maxExecutionTime);

    // affichage du bandeau d'information
    std::cout << infoHeader << std::endl;

    /********************************************************/
    /** exploitation des arguments de la ligne de commande **/
    /********************************************************/
    
    // test de la compatibilit� de l'OS
    pGlobals->osType = getNativeArchitecture();
    if (HOST_UNKNOWN == pGlobals->osType) return ("OS non support�");
    
    // "range" : si option non pr�sente, tous les octets du fichier seront marqu�s
    if (pGlobals->bytesToTaint.empty())  pGlobals->bytesToTaint = "all";

    // "target" : ex�cutable cible
    pGlobals->targetPath = getAbsoluteFilePath(pGlobals->targetPath);
    if (pGlobals->targetPath.empty())  return ("argument -t (ex�cutable cible) absente");

    // test du type d'ex�cutable (32 ou 64 bits)
    int kindOfExe = getKindOfExecutable(pGlobals->targetPath);
    if (kindOfExe < 0)  return (pGlobals->targetPath + " : fichier introuvable ou non ex�cutable");
    // ex�cutable 64bits sur OS 32bits => probl�me
    else if ((SCS_64BIT_BINARY == kindOfExe) && (pGlobals->osType < BEGIN_HOST_64BITS))
    {
        return ("ex�cutable 64bits incompatible avec OS 32bits");
    }

    // "initial" : premier fichier d'entr�e : test de son existence
    if (!testFileExists(pGlobals->firstInputPath))  return ("fichier initial non trouv�");

    // "destination" Si option non pr�sente : choisir par d�faut le dossier "results"
    std::string resultDir;
    bool isDirectoryOptionPresent = (ops >> GetOpt::Option('d', "dir", resultDir));
    if (pGlobals->resultDir.empty()) resultDir = exePath + "results";
    else pGlobals->resultDir = getAbsoluteFilePath(pGlobals->resultDir);

    // cr�ation du dossier de r�sultats. 
    BOOL createDirResult = CreateDirectory(resultDir.c_str(), NULL);
    if (!createDirResult && (ERROR_ALREADY_EXISTS == GetLastError()))
    {
        char c;
        
        std::cout << "Le dossier de r�sultat existe deja\n";
        std::cout << "effacer son contenu et continuer ? (o/n)";

        do { std::cin >> c;	} while ((c != 'o') && (c != 'n'));

        if ('n' == tolower(c))	return ("");

        int eraseDir = deleteDirectory(resultDir);
        if (eraseDir) return ("erreur cr�ation du dossier de r�sultats");
    }  

    // copie du fichier initial dans ce dossier (sans extension, avec le nom 'input0')
    std::string firstFileName(resultDir + "\\input0");
    CopyFile(pGlobals->firstInputPath.c_str(), firstFileName.c_str(), false);
    pGlobals->firstInputPath = firstFileName;

    // chemin pr�rempli pour le fichier de fautes (non cr�� pour l'instant)
    pGlobals->faultFile = pGlobals->resultDir + "\\fault.txt";
    
    /**********************************************************************/
    /** Construction des chemins d'acc�s aux outils externes (PIN et Z3) **/
    /**********************************************************************/
    
    std::string pin_X86, pin_X86_VmDll, pintool_X86;
    std::string pin_X64, pin_X64_VmDll, pintool_X64;
    
    // repertoire 'root' de PIN, soit en variable d'environnement
    // sinon prendre le r�pertoire de fuzzwin
    std::string pinPath;
    char* pinRoot = getenv("PIN_ROOT");
    if (nullptr == pinRoot) pinPath = exePath;
    else                    pinPath = std::string(pinRoot);
    // repertoire 'root' de Z3. Idem
    std::string z3Path;
    char* z3Root = getenv("Z3_ROOT");
    if (nullptr == z3Root) z3Path = exePath;
    else                   z3Path = std::string(z3Root);

    // modules 32 bits 
    pin_X86       = pinPath + "ia32\\bin\\pin.exe";
    pin_X86_VmDll = pinPath + "ia32\\bin\\pinvm.dll";
    pintool_X86   = exePath + "fuzzwinX86.dll";

    // modules 64 bits
    pin_X64       = pinPath + "intel64\\bin\\pin.exe";
    pin_X64_VmDll = pinPath + "intel64\\bin\\pinvm.dll";
    pintool_X64   = exePath + "fuzzwinX64.dll";
    
    // chemin vers Z3
    z3Path += "\\bin\\z3.exe";
    
    // test de la pr�sence des fichiers ad�quats
    if (!testFileExists(z3Path))        return "solveur z3 absent";
    
    if (!testFileExists(pin_X86))       return "executable PIN 32bits absent";
    if (!testFileExists(pin_X86_VmDll)) return "librairie PIN_VM 32bits absente";
    if (!testFileExists(pintool_X86))   return "pintool FuzzWin 32bits absent";
    // OS 32 bits : pas besoin des modules 64bits
    if (pGlobals->osType >= BEGIN_HOST_64BITS) 
    {
        if (!testFileExists(pin_X64))       return "executable PIN 64bits absent";
        if (!testFileExists(pin_X64_VmDll)) return "librairie PIN_VM 64bits absente";
        if (!testFileExists(pintool_X64))   return "pintool FuzzWin 64bits absent";
    }
  
    /**********************************************/
    /** cr�ation des tubes nomm� avec le Pintool **/
    /**********************************************/
    pGlobals->hPintoolPipe = CreateNamedPipe("\\\\.\\pipe\\fuzzwin",	
        PIPE_ACCESS_DUPLEX,	// acc�s en lecture/�criture 
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, // mode message, bloquant
        1,		// une seule instance
        4096,	// buffer de sortie 
        4096,	// buffer d'entr�e
        0,		// time-out du client = defaut
        NULL);	// attributs de securit� par defaut

    if (INVALID_HANDLE_VALUE == pGlobals->hPintoolPipe)
    {
        return ("Erreur de cr�ation du pipe fuzzWin\n");
    }

    /**********************************************************/
    /** cr�ation du process Z3 avec redirection stdin/stdout **/ 
    /**********************************************************/
    if (!createSolverProcess(z3Path)) 	return ("erreur cr�ation processus Z3");

    /***************************************/
    /** Ligne de commande pour le pintool **/ 
    /***************************************/

    // si OS 32 bits, pas la peine de sp�cifier les modules 64bits
    if (pGlobals->osType < BEGIN_HOST_64BITS) 
    {
        /* $(pin_X86) -t $(pintool_X86) -- $(targetFile) %INPUT% (ajout� a chaque fichier test�) */
        pGlobals->cmdLinePin = '"' + pin_X86 \
            + "\" -t \"" + pintool_X86       \
            + "\" -- \"" + pGlobals->targetPath + "\" ";
    }
    else
    {
        /* $(pin_X86) -p64 $(pin_X64) -t64 $(pintool_X64) -t $(pintool_X86) 
        -- $(targetFile) %INPUT% (ajout� a chaque fichier test�) */
        pGlobals->cmdLinePin = '"' + pin_X86  \
            + "\" -p64 \"" + pin_X64      \
            + "\" -t64 \"" + pintool_X64  \
            + "\" -t \""   + pintool_X86  \
            + "\" -- \""   + pGlobals->targetPath + "\" ";
    }
    
    return ("OK");
}
