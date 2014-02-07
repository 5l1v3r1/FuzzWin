#pragma once 
#include "TaintManager.h"

namespace UTILS 
{
// Callbacks pour les instructions non g�r�es => d�marquage destination(s)
void cUNHANDLED(INS &ins);

// d�marquage rapide registre par callback 
template<UINT32 len> void PIN_FAST_ANALYSIS_CALL uREG(THREADID tid, REG reg)
{  
    TaintManager_Thread *pTmgrTls = static_cast<TaintManager_Thread*>(PIN_GetThreadData(tlsKeyTaint, tid));
    pTmgrTls->unTaintRegister<len>(reg); 
}

// d�marquage rapide m�moire par callback. Pas de template car taille ind�finie 
inline void PIN_FAST_ANALYSIS_CALL uMEM(ADDRINT address, UINT32 sizeInBytes)
{  
    ADDRINT lastAddress = address + sizeInBytes;
    while (address++ < lastAddress)    pTmgrGlobal->unTaintMemory<8>(address);
}

inline void PIN_FAST_ANALYSIS_CALL uFLAGS(THREADID tid)  
{  
    TaintManager_Thread *pTmgrTls = static_cast<TaintManager_Thread*>(PIN_GetThreadData(tlsKeyTaint, tid));
    pTmgrTls->unTaintAllFlags(); 
}

// calcule et stocke un objet correspondant au marquage de l'adresse indirecte
// l'adresse est calcul�e sur 32bits (x86) ou 64bits (x64) et stock�e dans TaintManager
// a charge de l'appelant de l'adapter � la longueur r�elle de l'adresse effective (OperandSize)
// 3 surcharges : 
// 1) Base et Index Pr�sents : cas BISD
// 2) Index Pr�sent (d�placement nul ou non): cas ISD
// 3) Base Pr�sent et d�placement (nul ou non) : cas BD.
// pour plus de d�tails voir la d�finition dans utils.cpp

#if TARGET_IA32
void computeTaintEffectiveAddress(THREADID tid, REG baseReg,  ADDRINT baseRegValue,  REG indexReg, ADDRINT indexRegValue, INT32 displ, UINT32 scale);
void computeTaintEffectiveAddress(THREADID tid, REG indexReg, ADDRINT indexRegValue, INT32 displ,  UINT32 scale);
void computeTaintEffectiveAddress(THREADID tid, REG baseReg,  ADDRINT baseRegValue,  INT32 displ);
#else
void computeTaintEffectiveAddress(THREADID tid, REG baseReg,  ADDRINT baseRegValue,  REG indexReg, ADDRINT indexRegValue, INT32 displ, UINT32 scale);
void computeTaintEffectiveAddress(THREADID tid, REG indexReg, ADDRINT indexRegValue, INT32 displ,  UINT32 scale);
void computeTaintEffectiveAddress(THREADID tid, REG baseReg,  ADDRINT baseRegValue,  INT32 displ);
#endif
} // namespace UTILS
