#pragma once 
#include <TaintEngine\TaintManager.h>

namespace CONVERT
{
// Conversion vers AX/EAX/RAX : 1 seul registre destination (CBW, CWDE, CDQE)
// diff�rentiation selon la taille de la source (8, 16 ou 32)
void cCBW(INS &ins);
void cCWDE(INS &ins);
void cCDQE(INS &ins);

// Conversion vers DX:AX/EDX:EAX/RDX:RAX : 2 registres destination (CWD, CDQ, CQO)
// diff�rentiation selon la taille de la source (16, 32 ou 64)
void cCWD(INS &ins);
void cCDQ(INS &ins);
void cCQO(INS &ins);

// fonctions d'analyses associ�es
void sCBW (THREADID tid ADDRESS_DEBUG);
void sCWDE(THREADID tid, ADDRINT regAXValue ADDRESS_DEBUG);
void sCWD (THREADID tid, ADDRINT regAXValue ADDRESS_DEBUG);
void sCDQ (THREADID tid ,ADDRINT regEAXValue ADDRESS_DEBUG);

#if TARGET_IA32E
void sCDQE(THREADID tid, ADDRINT regEAXValue ADDRESS_DEBUG);
void sCQO (THREADID tid, ADDRINT regRAXValue ADDRESS_DEBUG);
#endif
}
