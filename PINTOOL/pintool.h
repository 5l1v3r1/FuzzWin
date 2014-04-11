/*!

 * \file fuzzwin.h
 * \brief header g�n�ral pour le pintool 
 * \author S�bastien LECOMTE
 * \version 05.a
 * \date 30/07/2013

 * contient les d�finitions g�n�rales utilis�es par tous
 * les fichiers source du pintool
*/

#pragma once

// le kit 62376 de PIN provoque un warning de cast (C4244) � la compilation
#pragma warning( push )
#pragma warning( disable : 4244 )
#include "pin.h"
#pragma warning( pop )


#include <iostream> // std::cout pour erreurs d'initialisation principalement
#include <fstream>  // pour fichiers de log
#include <ctime>    // pour calcul du temps dans fichier de log

// Namespace obligatoire pour eviter les confilts WINAPI / PIN
namespace WINDOWS 
{
#include <windows.h>
}

// d�finitions des types de registres et des macros selon l'architecture 32/64 bits
#include <TaintEngine\architecture.h>

/* les types DWORD et HANDLE ne sont pas d�finis par PIN */
typedef unsigned long DWORD; 
typedef WINDOWS::HANDLE HANDLE;

/*********************/
/* Constantes utiles */
/*********************/

#define EXIT_EXCEPTION      -1 // exception trouv�e (option checkscore)
#define EXIT_MAX_CONSTRAINTS 2 // fin du pintool pour cause de contrainte max
#define EXIT_TIMEOUT         3 // fin du pintool pour temps max d�pass�

#define INIT_ERROR           0 // erreur d'initialisation
#define OPTION_TAINT         1 // pintool "fuzzwin"  : extraction contraintes marqu�es
#define OPTION_CHECKSCORE    2 // pintool checkscore : test et score de l'entr�e

/*********************************************************/
/* Variables globales du programme (d�clarations extern) */
/*********************************************************/

// handle du pipe de communication des resultats (tube nomm� ou STDOUT avec l'option -nopipe)
extern HANDLE       g_hPipe;

// fichier d'entr�e du programme fuzz� (pass� via pipe ou ligne de commande)
extern std::string  g_inputFile; 
// option du pintool : taint ou ckeckscore
extern std::string  g_option;
// temps maximal d'execution
extern UINT32       g_maxTime;
// nombre maximal de contraintes
extern UINT32       g_maxConstraints;

// log de dessasemblage du binaire
extern bool         g_logasm;
// log de marquage
extern bool         g_logtaint;
// option pipe ou non pour l'�change du fichier initial  et de la formule finale
extern bool         g_nopipe;

// vrai d�s que les premi�res donn�es seront lues dans la source
extern bool         g_beginInstrumentationOfInstructions;

// clefs de stockage locales pour chaque thread
extern TLS_KEY      g_tlsKeyTaint; 
extern TLS_KEY      g_tlsKeySyscallData;

// structure de blocage inter-thread    
extern PIN_LOCK     g_lock;  

/** OPTION CHECKSCORE **/
// nombre d'instructions ex�cut�es
extern UINT64       g_nbIns;

/** OPTIONS DE LA LIGNE DE COMMANDE **/

extern KNOB<std::string> KInputFile;     
extern KNOB<UINT32>      KMaxTime;
extern KNOB<std::string> KBytes;
extern KNOB<UINT32>      KMaxConstr;
extern KNOB<std::string> KOption;
extern KNOB<UINT32>      KOsType;
extern KNOB<BOOL>        KLogAsm;
extern KNOB<BOOL>        KLogTaint;
extern KNOB<BOOL>        KNoPipe;

extern std::ofstream g_debug;     // fichier de log du desassemblage
extern std::ofstream g_taint;     // fichier de log du marquage
extern clock_t       g_timeBegin; // chrono d�part de l'instrumentation

/*******************************************************************/
/* procedure d'initialisation des variables globales et param�tres */
/*******************************************************************/
int pintoolInit(); // fichier initialize.cpp

/*****************/
/* MACROS DE LOG */
/*****************/

// OPTION -logasm //
//----------------//

// log de dessassemblage standard, partie instrumentation
inline void _LOGINS(INS ins) 
{
    if (g_logasm) 
    { 
        PIN_GetLock(&g_lock, PIN_ThreadId()); 
        g_debug << "[T:" << PIN_ThreadId() << "] " << hexstr(INS_Address(ins)); 
        g_debug << " " << INS_Disassemble(ins).c_str(); 
        PIN_ReleaseLock(&g_lock); 
    }
}

// log d�sassemblage, partie analyse
inline void _LOGDEBUG(const std::string &s)
{
    if (g_logasm) 
    {
        PIN_GetLock(&g_lock, PIN_ThreadId()); 
        g_debug << s << std::endl; 
        PIN_ReleaseLock(&g_lock); 
    }
}

// log d�sassemblage, partie Syscalls, uniquement 
inline void  _LOGSYSCALLS(ADDRINT syscallNumber, const std::string &s)
{
    if (g_logasm) 
    {
        PIN_GetLock(&g_lock, PIN_ThreadId());      
        g_debug << "[T:"  << decstr(PIN_ThreadId());
        g_debug << "][P:" << hexstr(PIN_GetPid()); 
        g_debug << "][S:" << hexstr(syscallNumber);
        g_debug << s << std::endl; 
        PIN_ReleaseLock(&g_lock); 
    }
}

// OPTION -logtaint //
//------------------//

// le log du marquage ins�re en plus l'adresse de l'instruction trait�e en mode DEBUG
// en mode RELEASE, l'argument 'insAddress' n'est pas pass�, ce qui fait gagner un peu
// en performances

#if DEBUG // mode DEBUG

// argument pour l'enregistrement d'un callback : adresse de l'instruction
#define CALLBACK_DEBUG  IARG_INST_PTR,  
// argument correspondant � l'adresse de l'instruction
#define ADDRESS_DEBUG   ,ADDRINT insAddress 
// argument ajout� lors de l'appel � une fonction hors du cas callback
#define INSADDRESS      ,insAddress
// log de marquage pour une instruction, avec adresse de l'instruction en mode DEBUG
#define _LOGTAINT(t)  \
if (g_logtaint) \
{ \
    PIN_GetLock(&g_lock, PIN_ThreadId()); \
    g_taint << "[T:"  << decstr(PIN_ThreadId()) << "] "; \
    g_taint << hexstr(insAddress) << " " << t << " MARQUE !! " << std::endl; \
    PIN_ReleaseLock(&g_lock); \
}
// log de d�marquage pour une instruction, avec adresse de l'instruction en mode DEBUG
#define _LOGUNTAINT(t)  \
if (g_logtaint) \
{ \
    PIN_GetLock(&g_lock, PIN_ThreadId()); \
    g_taint << "[T:"  << decstr(PIN_ThreadId()) << "] "; \
    g_taint << hexstr(insAddress) << " " << t << " DEMARQUAGE !! " << std::endl; \
    PIN_ReleaseLock(&g_lock); \
}

#else // mode RELEASE
#define ADDRESS_DEBUG   
#define CALLBACK_DEBUG 
#define INSADDRESS
// log de marquage pour une instruction
#define _LOGTAINT(t) \
if (g_logtaint) \
{ \
    PIN_GetLock(&g_lock, PIN_ThreadId()); \
    g_taint << "[T:"  << decstr(PIN_ThreadId()) << "] "; \
    g_taint << t << " MARQUE !! " << std::endl; \
    PIN_ReleaseLock(&g_lock);\
}
// log de d�marquage pour une instruction
#define _LOGUNTAINT(t) \
if (g_logtaint) \
{ \
    PIN_GetLock(&g_lock, PIN_ThreadId()); \
    g_taint << "[T:"  << decstr(PIN_ThreadId()) << "] "; \
    g_taint << " " << t << " DEMARQUAGE !! " << std::endl; \
    PIN_ReleaseLock(&g_lock);\
}
#endif

/****************************************/
/* macros globales et metaprogrammation */
/****************************************/

// Renvoie le registre d'accumulation utilis� par certaines instructions
template<UINT32 lengthInBits> class registerACC 
{ public:  static inline REG getReg() { return REG_INVALID_ ; }};
template<> class registerACC<8> 
{ public:  static inline REG getReg() { return REG_AL; }};
template<> class registerACC<16> 
{ public:  static inline REG getReg() { return REG_AX; }};
template<> class registerACC<32>
{ public:  static inline REG getReg() { return REG_EAX; }};
#if TARGET_IA32E
template<> class registerACC<64> 
{ public:  static inline REG getReg() { return REG_RAX; }};
#endif

// Renvoie le registre I/O (AH/DX/EDX/RDX) utilis� par certaines instructions
template<UINT32 lengthInBits> class registerIO 
{ public:  static inline REG getReg() { return REG_INVALID_ ; }};
template<> class registerIO<8> 
{ public:  static inline REG getReg() { return REG_AH; }}; // 8 bits = AH et non DH (cf. instr MUL)
template<> class registerIO<16> 
{ public:  static inline REG getReg() { return REG_DX; }};
template<> class registerIO<32>
{ public:  static inline REG getReg() { return REG_EDX; }};
#if TARGET_IA32E
template<> class registerIO<64> 
{ public:  static inline REG getReg() { return REG_RDX; }};
#endif

// d�termination de la valeur "ff" en fonction de la taille fournie (en bits)
// utilis� par les fonctions d'analyse traitant les instructions LOGICAL::OR et STRINGOP::SCAS
// 8b =  0xff ; 16b = 0xffff ; 32b = 0xffffffff; 64b = (__int64)(-1)
template<UINT32 lengthInBits> class minusOne 
{ public:  static inline const ADDRINT get() { return (0xffffffff >> (32 - lengthInBits)); }};
#if TARGET_IA32E
template<> class minusOne<64> 
{ public:  static inline const ADDRINT get() { return (0xffffffffffffffff); }};
#endif

// d�r�f�rencement de la m�moire pour r�cup�rer la valeur. 
// utilis� dans les fonctions d'analyse pour cr�er 
// des 'ObjectSource' lorsque la m�moire n'est pas marqu�e
template<UINT32 lengthInBits> ADDRINT getMemoryValue(ADDRINT address)
{
    ADDRINT returnValue = 0;
    // d�r�f�rencement de 'lengthInBits>>3' octets � partir de 'address'
    // Equivalent de Memcpy pour PIN
    PIN_SafeCopy(&returnValue, reinterpret_cast<ADDRINT*>(address), lengthInBits >> 3);
    return (returnValue);
}
