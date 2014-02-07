#include "pintool.h"
#include "syscalls.h"
#include "TaintManager.h"

#include <algorithm> // std::find

// num�ros des syscalls instrument�s par le pintool
// ceux ci sont d�termines dynamiquement
static WINDOWS::DWORD PIN_NtCreateFile; 
static WINDOWS::DWORD PIN_NtClose;  
static WINDOWS::DWORD PIN_NtOpenFile;   
static WINDOWS::DWORD PIN_NtReadFile;   
static WINDOWS::DWORD PIN_NtSetInformationFile; 
static WINDOWS::DWORD PIN_NtCreateSection; 
static WINDOWS::DWORD PIN_NtMapViewOfSection; 

// convertit un texte unicode en ascii
static std::string SYSCALLS::unicodeToAscii(const std::wstring &input)
{  
    char *ascii = new char[input.length() + 1];
    ascii[input.length() + 1] = 0;

    WINDOWS::WideCharToMultiByte(CP_ACP, 0, input.c_str(), 
        -1, ascii, static_cast<int>(input.length()), nullptr, nullptr);

    std::string result(ascii);
    delete[] (ascii);
    return (result);
}

void SYSCALLS::defineSyscallsNumbers() 
{
    OSTYPE osVersion = HOST_UNSUPPORTED; // par d�faut 'OS non support�'
    
    // d�termination des num�ros des appels syst�mes selon l'OS 
    // uniquement n�cessaire en 32bits (en 64 bits ils sont les memes qq soit la version de l'OS)
#if TARGET_IA32 

    typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, BOOL*);  
    BOOL bIsWow64 = false;	// par d�faut, pas d'�mulation 32 bits  

    LPFN_ISWOW64PROCESS fnIsWow64Process = reinterpret_cast<LPFN_ISWOW64PROCESS>
        (WINDOWS::GetProcAddress(WINDOWS::GetModuleHandle(TEXT("kernel32")),"IsWow64Process"));

    if (nullptr != fnIsWow64Process)  fnIsWow64Process(WINDOWS::GetCurrentProcess(),&bIsWow64);

    // si mode 32bits sur OS 64bits (WOW64) => syscalls 64bits, fin des tests
    if (bIsWow64)	osVersion = HOST_WOW64_OR_64BITS;
    // sinon poursuite des tests
    else
    {
        // la fonction GetSystemInfo retourne une structure avec notamment wProcessorArchitecture. 
        // Si AMD64 => syscalls 64bits
        // on ne fait pas appel � GetNativeSystemInfo, car le cas WOW64 a d�ja �t� test�
        WINDOWS::SYSTEM_INFO si = {0};
        WINDOWS::GetSystemInfo(&si);

        if ( PROCESSOR_ARCHITECTURE_AMD64 == si.wProcessorArchitecture)	
        {
            // ce cas arrive si IsWow64Process n'est pas dispo
            osVersion = HOST_WOW64_OR_64BITS;	
        }
        else if (PROCESSOR_ARCHITECTURE_INTEL == si.wProcessorArchitecture)	
        {
            // OS 32bits, oblig� d'appeler GetVersionEx pour determiner 
            // la version de l'OS 32bits pour fixer le num�ro des syscalls
            // la structure OSVERSIONINFOEX contient ce que l'on cherche � savoir
            WINDOWS::OSVERSIONINFOEX osvi;
            ZeroMemory(&osvi, sizeof(osvi));
            osvi.dwOSVersionInfoSize = sizeof(WINDOWS::OSVERSIONINFOEX);
            WINDOWS::GetVersionEx(reinterpret_cast<WINDOWS::OSVERSIONINFO*>(&osvi));

            // Test for the specific product.
            switch (osvi.dwMajorVersion)	
            {
            case 6:
                switch (osvi.dwMinorVersion) 
                {
                case 0:	// Version 6.0 = Vista ou Server 2008
                    if( osvi.wProductType == VER_NT_WORKSTATION )
                    {
                        osVersion = HOST_VISTA;
                        // TESTER ICI LE CAS SP0
                    }
                    else osVersion = HOST_2008;
                    break;
                case 1:	// Version 6.1 = Seven ou Server 2008 R2
                    if  (osvi.wProductType == VER_NT_WORKSTATION )	osVersion = HOST_SEVEN;
                    else osVersion = HOST_2008_R2;
                    break;
                case 2:
                    if (osvi.wProductType == VER_NT_WORKSTATION )	osVersion = HOST_8_0;
                    else osVersion = HOST_2012; 
                    break;
                case 3:
                    if (osvi.wProductType == VER_NT_WORKSTATION )	osVersion = HOST_8_1;
                    else osVersion = HOST_2012_R2; 
                    break;
                default: break;
                }
                break;
            case 5:
                switch (osvi.dwMinorVersion) 
                {
                    case 0:	// Version 5.0 = Windows 2000
                        osVersion = HOST_2000;
                        break;
                    case 1:	// Version 5.1 = Windows XP
                        osVersion = HOST_XP;
                        break;
                    case 2: // Version 5.2 = Server 2003
                        osVersion = HOST_2003; 
                        break;
                    default: break;
                }
                break;
            default: break; 
            }
        }
    }

#else   
    osVersion = HOST_WOW64_OR_64BITS; // pintool 64bits A MODIFIER AVEC WIN8/8.1
#endif
    
    PIN_NtClose         = syscallTable[osVersion][INDEX_NTCLOSE];
    PIN_NtCreateFile    = syscallTable[osVersion][INDEX_NTCREATEFILE];
    PIN_NtOpenFile      = syscallTable[osVersion][INDEX_NTOPENFILE];
    PIN_NtReadFile      = syscallTable[osVersion][INDEX_NTREADFILE];
    PIN_NtCreateSection = syscallTable[osVersion][INDEX_NTCREATESECTION];
    PIN_NtSetInformationFile = syscallTable[osVersion][INDEX_NTSETINFORMATIONFILE];
    PIN_NtMapViewOfSection   = syscallTable[osVersion][INDEX_NTMAPVIEWOFSECTION];
}

void SYSCALLS::syscallEntry(THREADID tid, CONTEXT *ctxt, SYSCALL_STANDARD std, void *)
{
    // r�cup�ration de la structure de gestion des syscalls dans la TLS
    Syscall_Data *pSysData = static_cast<Syscall_Data*>(PIN_GetThreadData(tlsKeySyscallData, tid));
    
    // par d�faut, le syscall ne sera pas �tudi� apres son appel
    // on l'indique en stockant un num�ro de syscall nul dans la TLS 
    pSysData->syscallNumber = 0;
    
    // stockage du num�ro du syscall concern�
    ADDRINT syscallNumber = PIN_GetSyscallNumber(ctxt, std);
    
    // traitement selon le num�ro du syscall ; un switch aurait �t�
    // l'ideal mais les num�ros de syscalls sont d�termin�s dynamiquement...

    /******** READFILE ********/
    if (syscallNumber == PIN_NtReadFile)  
    {
        _LOGSYSCALLS("] *** ReadFile Before");
        
        // (ARG 0:IN) handle du device
        HANDLE hFile = reinterpret_cast<HANDLE>(PIN_GetSyscallArgument(ctxt, std, 0)); 
        
        // it�rateurs sur la liste des descripteurs suivis,
        auto itFind = pSysData->listOfFileHandles.find(hFile);
        auto itEnd  = pSysData->listOfFileHandles.end();
        // si ce handle a �t� trouv�, c'est qu'il correspond 
        // au fichier cible pr�alablement ouvert
        if (itFind != itEnd) 
        { 
            // stockage du num�ro du syscall dans la TLS pour traitement au retour du syscall
            pSysData->syscallNumber = PIN_NtReadFile;

            // (ARG N�4:OUT) structure d'information remplie suite � la lecture
            pSysData->pInfos = reinterpret_cast<PIO_STATUS_BLOCK>(PIN_GetSyscallArgument(ctxt, std, 4)); 
            
            // (ARG N�5:OUT) adresse du buffer qui contiendra les donn�es
            pSysData->pBuffer = reinterpret_cast<void*>(PIN_GetSyscallArgument(ctxt, std, 5)); 
            
            // (7:IN_OPT) (PLARGE_INTEGER) position de l'offset de lecture
            // argument optionnel : si non fourni (nullptr), utiliser offset actuel
            // on utilise aussi l'offset actuel quand la structure PLARGE_INTEGER
            // vaut HighPart = -1 et LowPart = FILE_USE_FILE_POINTER_POSITION
            // cf. http://msdn.microsoft.com/EN-US/library/windows/hardware/ff567072(v=vs.85).aspx
            WINDOWS::PLARGE_INTEGER pByteOffset = 
                reinterpret_cast<WINDOWS::PLARGE_INTEGER>(PIN_GetSyscallArgument(ctxt, std, 7));
            
            if (nullptr == pByteOffset)  pSysData->offset = itFind->second;
            else if ((-1L == pByteOffset->HighPart) && (FILE_USE_FILE_POINTER_POSITION == pByteOffset->LowPart))
            {
                pSysData->offset = itFind->second;
            }
            // dans les autres cas, l'argument n�7 donne l'offset de d�but de lecture
            else pSysData->offset = static_cast<UINT32>(pByteOffset->QuadPart);

            _LOGSYSCALLS("] *** ReadFile Before FICHIER CIBLE");
        }   
    }
    /******** OPENFILE & CREATEFILE ********/
    else if ( (syscallNumber == PIN_NtCreateFile) || (syscallNumber == PIN_NtOpenFile))
    {
        // NB: les arguments sont les m�mes pour les deux syscalls

        // (2:IN) pointeur vers structure d'information fournie en entr�e
        // cette structure contient notamment le nom du fichier contenant le nom du device
        POBJECT_ATTRIBUTES pObjAttr = 
            reinterpret_cast<POBJECT_ATTRIBUTES>(PIN_GetSyscallArgument(ctxt, std, 2));   

        // transformation du nom de fichier en ASCII
        std::string filename = unicodeToAscii(pObjAttr->ObjectName->Name);
        _LOGSYSCALLS("]  Nom du fichier :" << filename);

        // si le nom du fichier est celui de la cible : sauvegarde des arguments
        if (filename.find(inputFile) != std::string::npos) 
        { 
            // stockage du num�ro du syscall dans la TLS pour traitement au retour du syscall
            pSysData->syscallNumber = PIN_NtCreateFile;
            
            // (0:OUT) (PHANDLE) pointeur vers handle apres ouverture device 
            pSysData->pHandle = reinterpret_cast<WINDOWS::PHANDLE>(PIN_GetSyscallArgument(ctxt, std, 0));  

            _LOGSYSCALLS("] *** Open//CreateFile Before FICHIER CIBLE");
        }
        else  _LOGSYSCALLS("] *** Open//CreateFile Before Device non suivi");
        
    }
    /******** CLOSE ********/
    else if (syscallNumber == PIN_NtClose)
    {
        // (0:IN) handle du device � fermer
        HANDLE handle = reinterpret_cast<HANDLE>(PIN_GetSyscallArgument(ctxt, std, 0)); 
        
        // recherche dans les handles de fichier et de section
        auto itFileFind = pSysData->listOfFileHandles.find(handle);
        auto itFileEnd  = pSysData->listOfFileHandles.end();

        auto itSectionBegin = pSysData->listOfSectionHandles.begin();
        auto itSectionEnd   = pSysData->listOfSectionHandles.end();
        auto itSectionFind  = std::find(itSectionBegin, itSectionEnd, handle); 
        
        // correspondance trouv�e au niveau des fichiers
        if (itFileFind != itFileEnd)
        { 
            // stockage du num�ro du syscall et du handle FICHIER � fermer
            // dans la TLS pour traitement au retour du syscall
            pSysData->syscallNumber = PIN_NtClose;
            pSysData->hFile =    handle;
            pSysData->hSection = NULL;

            _LOGSYSCALLS("] *** CloseFile Before CIBLE hFILE: " << reinterpret_cast<UINT32>(handle));
        }
        // correspondance trouv�e au niveau des sections
        else if (itSectionFind != itSectionEnd)
        {
             // stockage du num�ro du syscall et du handle FICHIER � fermer
            // dans la TLS pour traitement au retour du syscall
            pSysData->syscallNumber = PIN_NtClose;
            pSysData->hFile =    NULL;
            pSysData->hSection = handle;

            _LOGSYSCALLS("] *** CloseFile Before CIBLE hSECTION: " << reinterpret_cast<UINT32>(handle));
        }
        else _LOGSYSCALLS("] *** CloseFile Before autre device" );
    }
    /******** SETINFORMATIONFILE ********/
    else if (syscallNumber == PIN_NtSetInformationFile) 
    { 
        // (4:IN) code de l'op�ration � effectuer
        // s'il vaut 0xe (FilePositionInformation) => c'est un "seek"
        if (FilePositionInformation == static_cast<UINT32>(PIN_GetSyscallArgument(ctxt, std, 4))) 
        {  
            // (0:IN) handle du device 
            HANDLE hFile = reinterpret_cast<HANDLE>(PIN_GetSyscallArgument(ctxt, std, 0)); 

            // si le handle fait partie des descripteurs suivis, alors traiter apres syscalls
            if (pSysData->listOfFileHandles.find(hFile) != pSysData->listOfFileHandles.end()) 
            { 
                // stockage du num�ro du syscall dans la TLS pour traitement au retour du syscall
                pSysData->syscallNumber = PIN_NtSetInformationFile;

                // (2:IN) structure contenant le nouvel offset
                PFILE_POSITION_INFORMATION pFileInfo = 
                    reinterpret_cast<PFILE_POSITION_INFORMATION>(PIN_GetSyscallArgument(ctxt, std, 2)); 

                // cast du nouvel offset � 32bits (fichiers de 64bits non g�r�s) et stockage dans TLS
                pSysData->offset = static_cast<UINT32>(pFileInfo->CurrentByteOffset.QuadPart);

                _LOGSYSCALLS("] *** SetInformationFile Before nouvel offset : " << pSysData->offset);
            }
            else _LOGSYSCALLS( "] SetInformationFile");
        }
    }
    /******** CREATESECTION ********/
    else if (syscallNumber == PIN_NtCreateSection)
    {
        // La cr�ation d'un mapping de fichier s'effectue apr�s l'ouverture du fichier
        // la section est adoss�e en passant le handle du fichier en parametre 6 (optionnel)
        // il faut d�j� tester si le handle pass� est celui du fichier suivi
        
        // ARG N�6 (IN_OPT) : fileHandle de fichier
        HANDLE hFile = reinterpret_cast<HANDLE>(PIN_GetSyscallArgument(ctxt, std, 6));

        // si le handle fait partie des descripteurs suivis, alors traiter apres syscalls
        if (pSysData->listOfFileHandles.find(hFile) != pSysData->listOfFileHandles.end()) 
        {
            // ARG N�0 (OUT) : pointeur vers handle de la section
            PHANDLE pHandleSection = reinterpret_cast<PHANDLE>(PIN_GetSyscallArgument(ctxt, std, 0)); 

            // stockage du num�ro du syscall dans la TLS pour traitement au retour du syscall
            pSysData->syscallNumber = PIN_NtCreateSection;
            // stockage de la valeur dans la TLS pour traitement apr�s syscall
            pSysData->pHandle = pHandleSection;

            _LOGSYSCALLS("] *** CreateSection Before FICHIER CIBLE");
        }
        else  _LOGSYSCALLS("] *** CreateSection");

#if 0
        // ARG N�1 (IN) : ACCESS MASK (inutilis� ici)
        WINDOWS::ACCESS_MASK mask = static_cast<WINDOWS::ACCESS_MASK>(PIN_GetSyscallArgument(ctxt, std, 1));
        _LOGDEBUG("\t\t" << "ARG N�1 (IN) : ACCESS MASK :" << mask);
        // ARG N�2 (IN_OPT) : nom de l'objet et attributs
        POBJECT_ATTRIBUTES pObjectAttributes = reinterpret_cast<POBJECT_ATTRIBUTES>(PIN_GetSyscallArgument(ctxt, std, 2));
        _LOGDEBUG("\t\t" << "ARG N�2 (IN_OPT) : nom de l'objet et attributs :" << (ADDRINT) pObjectAttributes);
        // ARG N�3 (IN_OPT) : taille maximale de la section
        WINDOWS::PLARGE_INTEGER maxSize = reinterpret_cast<WINDOWS::PLARGE_INTEGER>(PIN_GetSyscallArgument(ctxt, std, 3));
#endif
    }
    /******** MAPVIEWOFSECTION ********/
    else if (syscallNumber == PIN_NtMapViewOfSection)
    {
        // si la liste des handles de section est vide : cela ne sera � rien de continuer
        if ( ! pSysData->listOfSectionHandles.empty())
        {
            // Le mapping de fichier s'effectue via le handle de la section (argument IN)
            // ARG N�0 (IN) : handle de la section
            HANDLE hSection = reinterpret_cast<HANDLE>(PIN_GetSyscallArgument(ctxt, std, 0));
 
            auto itFirst = pSysData->listOfSectionHandles.begin();
            auto itLast  = pSysData->listOfSectionHandles.end();

            // si le handle fait partie des descripteurs suivis, alors traiter apres syscalls
            if (std::find(itFirst, itLast, hSection) != itLast)
            {
                // stockage du num�ro du syscall dans la TLS pour traitement au retour du syscall
                pSysData->syscallNumber = PIN_NtMapViewOfSection;

                // ARG N�2 (IN & OUT) : pointeur vers pointeur de l'adresse de base ! PVOID *BaseAddress
                pSysData->ppBaseAddress = reinterpret_cast<WINDOWS::PVOID*>(PIN_GetSyscallArgument(ctxt, std, 2));

                // ARG N�5 (IN_OUT) : pointeur vers l'offset de la vue par rapport au d�but de la section
                // Attention : peut �tre NULL
                pSysData->pOffsetFromStart = reinterpret_cast<WINDOWS::PLARGE_INTEGER>(PIN_GetSyscallArgument(ctxt, std, 5));
        
                // ARG N�6 (IN_OUT) : pointeur vers taille de la vue
                pSysData->pViewSize = reinterpret_cast<WINDOWS::PSIZE_T>(PIN_GetSyscallArgument(ctxt, std, 6));
            }
        }
    }
    /******** SYSCALL NON GERE ********/
    else
    {    
        _LOGSYSCALLS("] *** Syscall non g�r� ");
    }
} // syscallEntry

void SYSCALLS::syscallExit(THREADID tid, CONTEXT *ctxt, SYSCALL_STANDARD std, void *) 
{
    // r�cup�ration de la structure de gestion des syscalls dans la TLS
    Syscall_Data *pSysData = static_cast<Syscall_Data*>(PIN_GetThreadData(tlsKeySyscallData, tid));

    // r�cup�ration du num�ro de syscall stock� avant l'appel
    ADDRINT syscallNumber = pSysData->syscallNumber;

    // fin si le syscall ne doit pas �tre �tudi� (valeur nulle), ou si erreur du syscall
    if ((syscallNumber == 0) || (PIN_GetSyscallErrno(ctxt, std) != 0))  return;
    else if (syscallNumber == PIN_NtReadFile)
    {
        // r�cup�ration du nombre d'octets r�ellement lus dans le fichier
        UINT32 nbBytesRead = static_cast<UINT32>(pSysData->pInfos->Information); 
                
        // r�cup�ration du couple (handle, offset) du handle concern�
        auto it = pSysData->listOfFileHandles.find(pSysData->hFile);

        // r�cup�ration de l'offset du premier octet lu
        UINT32 startOffset = pSysData->offset;
             
        // marquage de la m�moire : 'nbBytesRead' octets, adresse de d�part d�termin�e par pBuffer
        ADDRINT startAddress = reinterpret_cast<ADDRINT>(pSysData->pBuffer);
        pTmgrGlobal->createNewSourceTaintBytes(startAddress, nbBytesRead, startOffset);

        // mise � jour de l'offset de lecture
        pSysData->listOfFileHandles[pSysData->hFile] = startOffset + nbBytesRead;

        _LOGSYSCALLS("] *** ReadFile AFTER offset " << startOffset << " nb octets lus " << nbBytesRead);

        // des donn�es ont �t� lues dans le fichier, on peut commencer l'instrumentation
        // obligation d'obtenir le lock, la variable �tant globale
        PIN_GetLock(&lock, tid + 1);
        beginInstrumentation = true;
        PIN_ReleaseLock(&lock);
    }
    /******** OPENFILE & CREATEFILE ********/
    else if ( (syscallNumber == PIN_NtCreateFile) || (syscallNumber == PIN_NtOpenFile))
    {
        // Le syscall nous fournit le handle du fichier �tudi� tout juste ouvert
        // on le sauvegarde dans la liste des handles, avec un offset nul (= d�but du fichier)
        HANDLE hFile = *pSysData->pHandle;
        pSysData->listOfFileHandles[hFile] = 0;

        _LOGSYSCALLS("] *** Fichier cible OUVERT, handle = " << reinterpret_cast<UINT32>(hFile));
    }
    /******** SETINFORMATIONFILE ********/
    else if (syscallNumber == PIN_NtSetInformationFile) 
    {
        // modification de l'offset de lecture dans le fichier d�crit par le handle
        pSysData->listOfFileHandles[pSysData->hFile] = pSysData->offset;
    }
    /******** CREATESECTION ********/

    else if (syscallNumber == PIN_NtCreateSection)
    {
        // r�cup�ration du handle de la section nouvellement cr��e
        HANDLE hSection = *(pSysData->pHandle);

        // stockage dans le vecteur adhoc dans la TLS
        pSysData->listOfSectionHandles.push_back(hSection);

        _LOGSYSCALLS("] *** Section cr��e, handle = " << hSection);
    }      
    /******** MAPVIEWOFSECTION ********/
    else if (syscallNumber == PIN_NtMapViewOfSection)
    {
        // R�cup�ration de l'offset par rapport au d�but de la vue, par d�faut nul
        // si l'arguemnt est fourni (le pointeur non nul) r�cup�rer la valeur indiqu�e
        UINT32 offset = 0;
        if (nullptr != pSysData->pOffsetFromStart)
        {
            WINDOWS::LARGE_INTEGER largeOffset = *(pSysData->pOffsetFromStart);
            offset = static_cast<UINT32>(largeOffset.QuadPart);
        }

        // r�cup�ration de la taille de la vue
        UINT32 viewSize = static_cast<UINT32>(*(pSysData->pViewSize));

        // marquage de la m�moire � partir de (baseAddress + offset) sur 'viewSize' octets
        // provoque un surmarquage, car la taille de la vue est arrondie et ne
        // correspond pas � la taille du fichier
        ADDRINT startAddress = reinterpret_cast<ADDRINT>(*pSysData->ppBaseAddress) + offset;
        pTmgrGlobal->createNewSourceTaintBytes(startAddress, viewSize, 0);

        // Les donn�es ud fichier ont �t� mapp�es, on peut commencer l'instrumentation
        // obligation d'obtenir le lock, la variable �tant globale
        PIN_GetLock(&lock, tid + 1);
        beginInstrumentation = true;
        PIN_ReleaseLock(&lock);

        _LOGSYSCALLS("] *** MapViewOfSection After adresse d�part " << startAddress << " taille " << viewSize);
 } 
    else if (syscallNumber == PIN_NtClose)
    {
        // suppression du handle dans la liste (fichiers ou section)
        if (nullptr != pSysData->hFile)
        {
            pSysData->listOfFileHandles.erase(pSysData->hFile);
            _LOGSYSCALLS("]*** Close After fileHandle: " << reinterpret_cast<UINT32>(pSysData->hFile));
        }
        else if (nullptr != pSysData->hSection)
        {
            auto itSectionBegin = pSysData->listOfSectionHandles.begin();
            auto itSectionEnd   = pSysData->listOfSectionHandles.end();

            pSysData->listOfSectionHandles.erase(
                std::remove(itSectionBegin, itSectionEnd, pSysData->hSection), itSectionEnd);
            _LOGSYSCALLS("]*** Close After sectionHandle: " << reinterpret_cast<UINT32>(pSysData->hSection));
        }
    }
} // syscallExit

