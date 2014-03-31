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

// besoin de std::cout pour erreurs d'initialisation principalement
#include <iostream> 

// Namespace obligatoire pour eviter les confilts WINAPI / PIN
namespace WINDOWS 
{
#include <windows.h>
}

// d�finitions des types de registres et des macros
// selon l'architecture 32/64 bits
#include "architecture.h"

/* les types DWORD et HANDLE ne sont pas d�finis par PIN */
typedef unsigned long DWORD; 
typedef WINDOWS::HANDLE HANDLE;

/*********************/
/* Constantes utiles */
/*********************/

#define EXIT_EXCEPTION      -1 // exception trouv�e (option checkscore)
#define EXIT_MAX_CONSTRAINTS 2 // fin du pintool pour cause de contrainte max
#define EXIT_TIMEOUT         3 // fin du pintool pour temps max d�pass�

#define OPTION_TAINT         1 // pintool "fuzzwin" : extraction contraintes marqu�es
#define OPTION_CHECKSCORE    2 // pintool checkscore : test et score de l'entr�e

/*********************************************************/
/* Variables globales du programme (d�clarations extern) */
/*********************************************************/

// handle du pipe de communication des resultats (STDOUT en DEBUG, pipe avec SAGE en RELEASE)
extern HANDLE       g_hPipe;

// fichier d'entr�e du programme fuzz�
extern std::string  g_inputFile; 

// nombre maximal de contraintes
extern UINT32       g_maxConstraints;
// temps maximal d'execution
extern UINT32       g_maxTime;

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

/*******************************************************************/
/* procedure d'initialisation des variables globales et param�tres */
/*******************************************************************/
int pintoolInit();

/****************************************************/
/* variables et fonctions sp�cifiques au mode DEBUG */
/****************************************************/

#if DEBUG // mode DEBUG

#include <fstream>
#include <ctime>    // pour log

extern clock_t g_timeBegin;

extern KNOB<std::string> KnobInputFile;     
extern KNOB<UINT32>      KnobMaxExecutionTime;
extern KNOB<std::string> KnobBytesToTaint;
extern KNOB<UINT32>      KnobMaxConstraints;
extern KNOB<UINT32>      KnobOsType;
extern KNOB<std::string> KnobOption;
extern KNOB<UINT32>      KnobOsType;

extern std::ofstream g_debug; // fichier de log du desassemblage
extern std::ofstream g_taint; // fichier de log du marquage

// log de marquage pour une instruction
#define _LOGTAINT(t)     { \
    PIN_GetLock(&g_lock, PIN_ThreadId()); \
    g_taint << hexstr(insAddress) << " " << t << " MARQUE !! " << std::endl; \
    PIN_ReleaseLock(&g_lock); }

// log de dessassemblage standard (partie instrumentation)
#define _LOGINS(ins)  { \
    PIN_GetLock(&g_lock, PIN_ThreadId()); \
    g_debug << "[T:" << PIN_ThreadId() << "] " << hexstr(INS_Address(ins)); \
    g_debug << " " << INS_Disassemble(ins).c_str(); \
    PIN_ReleaseLock(&g_lock); }

// log d�sassemblage, partie analyse
#define _LOGDEBUG(s)  { \
    PIN_GetLock(&g_lock, PIN_ThreadId()); \
    g_debug << s << std::endl; \
    PIN_ReleaseLock(&g_lock); }

// log d�sassemblage, partie Syscalls
#define _LOGSYSCALLS(s)  { \
    PIN_GetLock(&g_lock, PIN_ThreadId());      \
    g_debug << "[T:"  << decstr(tid);          \
    g_debug << "][P:" << hexstr(PIN_GetPid()); \
    g_debug << "][S:" << hexstr(syscallNumber);\
    g_debug << s << std::endl; \
    PIN_ReleaseLock(&g_lock); }

// log de dessasemblage suite � non prise en charge d'une instruction
#define _LOGUNHANDLED(ins)  { \
    PIN_GetLock(&g_lock, PIN_ThreadId()); \
    g_debug << " *** non instrument� ***"; \
    PIN_ReleaseLock(&g_lock); }

// argument pour l'enregistrement d'un callback : adresse de l'instruction
#define CALLBACK_DEBUG  IARG_INST_PTR,  

// argument correspondant � l'adresse de l'instruction
#define ADDRESS_DEBUG   ,ADDRINT insAddress 

// argument ajout� lors de l'appel � une fonction hors du cas callback
#define INSADDRESS      ,insAddress

#else // Mode RELEASE : aucune fonction

#define ADDRESS_DEBUG   
#define CALLBACK_DEBUG 
#define INSADDRESS
#define _LOGTAINT(t)
#define _LOGINS(ins)
#define _LOGUNHANDLED(ins)
#define _LOGDEBUG(s)
#define _LOGSYSCALLS(s)
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
