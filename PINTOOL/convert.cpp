#include "convert.h"

void CONVERT::cCBW(INS &ins)
{
    // pas besoin de transmettre valeur de AL
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) sCBW, 
        IARG_THREAD_ID,
        CALLBACK_DEBUG IARG_END); 
} // cCBW

void CONVERT::cCWDE(INS &ins)
{
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) sCWDE, 
        IARG_THREAD_ID,
        IARG_REG_VALUE, REG_AX, 
        CALLBACK_DEBUG IARG_END); 
} // cCWDE

void CONVERT::cCWD(INS &ins)
{
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) sCWD, 
        IARG_THREAD_ID,
        IARG_REG_VALUE, REG_AX, 
        CALLBACK_DEBUG IARG_END); 
} // cCWD

void CONVERT::cCDQ(INS &ins)
{
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) sCDQ, 
        IARG_THREAD_ID,
        IARG_REG_VALUE, REG_EAX, 
        CALLBACK_DEBUG IARG_END); 
} // cCDQ

#if TARGET_IA32E
void CONVERT::cCDQE(INS &ins)
{
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) sCDQE, 
        IARG_THREAD_ID,
        IARG_REG_VALUE, REG_EAX, 
        CALLBACK_DEBUG IARG_END); 
} // cCDQE

void CONVERT::cCQO(INS &ins)
{
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) sCQO, 
        IARG_THREAD_ID,
        IARG_REG_VALUE, REG_RAX, 
        CALLBACK_DEBUG IARG_END); 
} // cCQO
#endif

// SIMULATE

// CBW : AX <- signExtend(AL)

void CONVERT::sCBW(THREADID tid ADDRESS_DEBUG)
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    REGINDEX regIndex = getRegIndex(REG_AL);

    if (pTmgrTls->isRegisterPartTainted(regIndex, 0))
    {
        // affectation � AX (enregistrement du TaintWord)
        pTmgrTls->updateTaintRegister<16>(REG_AX, std::make_shared<TaintWord>(
            X_SIGNEXTEND,
            ObjectSource(pTmgrTls->getRegisterPartTaint(regIndex, 0))));
    }
    else pTmgrTls->unTaintRegisterPart(regIndex, 1); // simple d�marquage AH (AL l'est d�j�)
} // cCBW

// CWDE : EAX <- signExtend(AX)
void CONVERT::sCWDE(THREADID tid, ADDRINT regAXValue ADDRESS_DEBUG)
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    if (pTmgrTls->isRegisterTainted<16>(REG_AX))
    {
        // affectation � EAX (enregistrement du TaintDword)
        pTmgrTls->updateTaintRegister<32>(REG_EAX, std::make_shared<TaintDword>(
            X_SIGNEXTEND,
            ObjectSource(pTmgrTls->getRegisterTaint<16>(REG_AX, regAXValue))));
    }
    else pTmgrTls->unTaintRegister<32>(REG_EAX); // d�marquage EAX
} // cCWDE

// CWD : DX:AX <- signExtend(AX)
void CONVERT::sCWD(THREADID tid, ADDRINT regAXValue ADDRESS_DEBUG)
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    if (pTmgrTls->isRegisterTainted<16>(REG_AX))
    {
        // construction du r�sultat
        TaintDwordPtr tdwPtr = std::make_shared<TaintDword>(
            X_SIGNEXTEND,
            ObjectSource(pTmgrTls->getRegisterTaint<16>(REG_AX, regAXValue)));

        // marquage de la destination (DX:AX)
        // les 2 octets faibles du r�sultat => inchang�s, aucun traitement
        // les 2 octets forts du r�sultat	=> marquage sur DX
        // extraction depuis le r�sultat du TaintWord d'index 1
        pTmgrTls->updateTaintRegister<16>(REG_DX, std::make_shared<TaintWord>(
            EXTRACT,
            ObjectSource(tdwPtr),
            ObjectSource(8, 1)));
    }
    else pTmgrTls->unTaintRegister<16>(REG_DX); // d�marquage DX (AX l'est d�j�)
} // cCWD

// CDQ : EDX:EAX <- signExtend(EAX)
void CONVERT::sCDQ(THREADID tid, ADDRINT regEAXValue ADDRESS_DEBUG)
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    if (pTmgrTls->isRegisterTainted<32>(REG_EAX))
    {
        // construction du r�sultat
        TaintQwordPtr tqwPtr = std::make_shared<TaintQword>(
            X_SIGNEXTEND,
            ObjectSource(pTmgrTls->getRegisterTaint<32>(REG_EAX, regEAXValue)));

        // marquage de la destination (EDX:EAX)
        // les 4 octets faibles du r�sultat => inchang�s, aucun traitement
        // les 4 octets forts du r�sultat	=> EDX
        // extraction depuis le r�sultat du TaintDword d'index 1
        pTmgrTls->updateTaintRegister<32>(REG_EDX, std::make_shared<TaintDword>(
            EXTRACT,
            ObjectSource(tqwPtr),
            ObjectSource(8, 1)));
    }
    else pTmgrTls->unTaintRegister<32>(REG_EDX); // d�marquage EDX (EAX l'est d�j�)
} // cCDQ

#if TARGET_IA32E
// CDQE : RAX <- signExtend(EAX)
void CONVERT::sCDQE(THREADID tid, ADDRINT regEAXValue ADDRESS_DEBUG)
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    if (pTmgrTls->isRegisterTainted<32>(REG_EAX))
    {
        // affectation � RAX (enregistrement du TaintQword)
        pTmgrTls->updateTaintRegister<64>(REG_RAX, std::make_shared<TaintQword>(
            X_SIGNEXTEND,
            ObjectSource(pTmgrTls->getRegisterTaint<32>(REG_EAX, regEAXValue))));
    }
    else pTmgrTls->unTaintRegister<64>(REG_RAX); // d�marquage RAX
} // cCDQE

// CQO : RDX:RAX <- signExtend(RAX)
void CONVERT::sCQO(THREADID tid, ADDRINT regRAXValue ADDRESS_DEBUG)
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    if (pTmgrTls->isRegisterTainted<64>(REG_RAX))
    {
        // construction du r�sultat
        TaintDoubleQwordPtr tdqwPtr = std::make_shared<TaintDoubleQword>(
            X_SIGNEXTEND,
            ObjectSource(pTmgrTls->getRegisterTaint<64>(REG_RAX, regRAXValue)));

        // marquage de la destination (RDX:RAX)
        // les 8 octets faibles du r�sultat => inchang�s, aucun traitement
        // les 8 octets forts du r�sultat	=> RDX
        // extraction depuis le r�sultat du TaintQword d'index 1
        pTmgrTls->updateTaintRegister<64>(REG_RDX, std::make_shared<TaintQword>(
            EXTRACT,
            ObjectSource(tdqwPtr),
            ObjectSource(8, 1)));
    }
    else pTmgrTls->unTaintRegister<64>(REG_RDX); // d�marquage RDX (RAX l'est d�j�)
} // cCQO
#endif
