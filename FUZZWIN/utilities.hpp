#pragma once
#include <windows.h>
#include <string>	

/* solutions fournies par le solveur sont du type
   define OFF?? (_BitVec 8) 
      #x??    */ 
#ifndef parseZ3ResultRegex
#define parseZ3ResultRegex "OFF(\\d+).*\r\n.*([0-9a-f]{2})"
#endif

// codes d�finissant le type d'OS pour la d�termination des num�ros d'appels syst�mes
// Le type d'OS est d�termin� par fuzzwin.exe et pass� en argument au pintool
enum OSTYPE 
{
    HOST_X86_2000,
    HOST_X86_XP,
    HOST_X86_2003,

    // pour cette version, le syscall 'setinformationfile' est diff�rent des autres SP...
    HOST_X86_VISTA_SP0, 
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

static const std::string solverConfig(
    "(set-option :auto-config false)"   \
    "(set-option :produce-models true)" \
    "(set-option :print-success false)" \
    "(set-option :relevancy 0)"         \
    "(set-option :smtlib2_compliant true)"  \
    "(set-logic QF_AUFBV)");

static const std::string infoHeader
(   
"; **************************************************\n" 
"; *  FUZZWIN : FUZZER AUTOMATIQUE SOUS WINDOWS     *\n" 
"; *  v1.2 (c) Sebastien LECOMTE 02/03/2014         *\n"
"; *  PIN Version 2.13 kit 62732 et Z3 version 4.3  *\n"
"; **************************************************\n" 
);

// d�termination de l'OS dynamiquement, inspir� de l'exemple fourni sur MSDN
// le type d'OS sera pass� en argument au pintool
// pour la surveillance des appels syst�mes
// NB : la version 8.1 du Windows Kit a d�sormais des fonctions sp�cifiques
// mais ne sera pas utilis� ici
static inline OSTYPE getNativeArchitecture()
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

// test du type d'ex�cutable (32 ou 64bits). Si non support� => retour n�gatif (-1)
static inline int getKindOfExecutable(const std::string &targetPath)
{
    DWORD kindOfExe = 0;
    BOOL  result = GetBinaryType((LPCSTR) targetPath.c_str(), &kindOfExe);
    
    // erreur si fichier non ex�cutable ou si non trouv�
    if (!result) return (-1);
    else         return (kindOfExe);
} // getKindOfExecutable


// Partie pure GUI : d�finition de pr�processeur QT_DLL pr�sente
#if QT_DLL

#ifndef FUZZWIN_GUI_DEFINES

#define FUZZWIN_GUI_DEFINES
#define TIMESTAMP     QDateTime::currentDateTime().time().toString("HH:mm:ss.zzz ")
#define TEXTRED(x)    QString("<font color=\"Red\">"##x##"</font>")
#define TEXTGREEN(x)  QString("<font color=\"Green\">"##x##"</font>")
#define LINEFEED      QString("<br>")

#endif

#endif