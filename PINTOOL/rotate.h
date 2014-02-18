#pragma once
#include "pintool.h"

#include "TaintManager.h"

namespace ROTATE 
{
// CALLBACKS 
void cROL(INS &ins);
void cROR(INS &ins);
void cRCL(INS &ins);
void cRCR(INS &ins);

// SIMULATE 
template<UINT32 lengthInBits> 
void sROL_IM(THREADID tid, UINT32 maskedDepl, ADDRINT writeAddress ADDRESS_DEBUG);        
template<UINT32 lengthInBits> 
void sROL_IR(THREADID tid, UINT32 maskedDepl, REG reg, ADDRINT regValue ADDRESS_DEBUG);
template<UINT32 lengthInBits> 
void sROL_RM(THREADID tid, ADDRINT regCLValue, ADDRINT writeAddressess ADDRESS_DEBUG);
template<UINT32 lengthInBits> 
void sROL_RR(THREADID tid, ADDRINT regCLValue, REG reg, ADDRINT regValue ADDRESS_DEBUG);
        
template<UINT32 lengthInBits> 
void sROR_IM(THREADID tid, UINT32 maskedDepl, ADDRINT writeAddress ADDRESS_DEBUG);        
template<UINT32 lengthInBits> 
void sROR_IR(THREADID tid, UINT32 maskedDepl, REG reg, ADDRINT regValue ADDRESS_DEBUG);
template<UINT32 lengthInBits> 
void sROR_RM(THREADID tid, ADDRINT regCLValue, ADDRINT writeAddressess ADDRESS_DEBUG);
template<UINT32 lengthInBits> 
void sROR_RR(THREADID tid, ADDRINT regCLValue, REG reg, ADDRINT regValue ADDRESS_DEBUG);

template<UINT32 lengthInBits> 
void sRCL_IM(THREADID tid, UINT32 maskedDepl, ADDRINT writeAddress, ADDRINT regGflagsValue ADDRESS_DEBUG);        
template<UINT32 lengthInBits> 
void sRCL_IR(THREADID tid, UINT32 maskedDepl, REG reg, ADDRINT regValue, ADDRINT regGflagsValue ADDRESS_DEBUG);
template<UINT32 lengthInBits> 
void sRCL_RM(THREADID tid, ADDRINT regCLValue, ADDRINT writeAddressess, ADDRINT regGflagsValue ADDRESS_DEBUG);
template<UINT32 lengthInBits> 
void sRCL_RR(THREADID tid, ADDRINT regCLValue, REG reg, ADDRINT regValue, ADDRINT regGflagsValue  ADDRESS_DEBUG);
     
template<UINT32 lengthInBits> 
void sRCR_IM(THREADID tid, UINT32 maskedDepl, ADDRINT writeAddress, ADDRINT regGflagsValue ADDRESS_DEBUG);        
template<UINT32 lengthInBits> 
void sRCR_IR(THREADID tid, UINT32 maskedDepl, REG reg, ADDRINT regValue, ADDRINT regGflagsValue ADDRESS_DEBUG);
template<UINT32 lengthInBits> 
void sRCR_RM(THREADID tid, ADDRINT regCLValue, ADDRINT writeAddressess, ADDRINT regGflagsValue ADDRESS_DEBUG);
template<UINT32 lengthInBits> 
void sRCR_RR(THREADID tid, ADDRINT regCLValue, REG reg, ADDRINT regValue, ADDRINT regGflagsValue  ADDRESS_DEBUG);
        
//  FLAGS 
// marquage flags sp�cifique pour ROL, d�placement non marqu� 
void fROL(TaintManager_Thread *pTmgr, const TaintPtr &resultPtr, UINT32 maskedDepl);
// marquage flags sp�cifique pour ROL, d�placement marqu� 
void fROL(TaintManager_Thread *pTmgr, const TaintPtr &resultPtr);

// marquage flags sp�cifique pour ROR, d�placement non marqu� 
void fROR(TaintManager_Thread *pTmgr, const TaintPtr &resultPtr, UINT32 maskedDepl);
// marquage flags sp�cifique pour ROR, d�placement marqu� 
void fROR(TaintManager_Thread *pTmgr, const TaintPtr &resultPtr);

// marquage flags sp�cifique pour RCL, d�placement non marqu�
void fRCL(TaintManager_Thread *pTmgr, const TaintPtr &resultPtr, const ObjectSource &objSrc, UINT32 maskedDepl);
// marquage flags sp�cifique pour RCL, d�placement marqu�
void fRCL(TaintManager_Thread *pTmgr, const ObjectSource &objSrc, const TaintBytePtr &tbCountPtr);

// marquage flags sp�cifique pour RCR, d�placement non marqu�
void fRCR(TaintManager_Thread *pTmgrTls, const ObjectSource &objSrc, const ObjectSource &objCarryFlagBeforeRcr, UINT32 maskedDepl);
// marquage flags sp�cifique pour RCR, d�placement marqu�
void fRCR(TaintManager_Thread *pTmgrTls, const ObjectSource &objSrc, const TaintBytePtr &tbCountPtr);

// d�marquage flags ROTATE : uniquement OF et CF
void fUnTaintROTATE(TaintManager_Thread *pTmgrTls);
} // namespace ROTATE

#include "rotate.hpp"