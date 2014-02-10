#include "instrumentation.h"
#include "buildFormula.h"
#include "syscalls.h"

#include "binary.h"
#include "bitbyte.h"
#include "cmov.h"
#include "CALL.h"
#include "conditional_branch.h"
#include "convert.h"
#include "unconditional_branch.h"
#include "dataxfer.h"
#include "logical.h"
#include "misc.h"
#include "pop.h"
#include "push.h"
#include "shift.h"
#include "ret.h"
#include "rotate.h"
#include "utils.h"
#include "stringop.h"
#include "semaphore.h"

/*********************/
/*** PINTOOL TAINT ***/
/*********************/

// Fonction d'instrumentation des instructions
// premier tri en fonction du nom de l'instruction, qui orientera vers les
// proc�dures sp�cifiques � chaque instruction
// les instructions sont regroup�es en CATEGORIES (fichiers .cpp, .hpp et .h)
// ces cat�gories sont celles d�finies par PIN
void INSTRUMENTATION::Instruction(INS ins, void* )
{
    if (!g_beginInstrumentationOfInstructions) return;

    // mode DEBUG : d�sassemblage 
    _LOGINS(ins); 

    switch (INS_Opcode(ins)) // BIG BIG SWITCH :)
    {
    case XED_ICLASS_NOP:    break;    // la plus facile :p

    // BINARY: ADD, SUB, INC, DEC, NEG, CMP
    case XED_ICLASS_SUB:  BINARY::cSUB(ins);  break;
    case XED_ICLASS_ADD:  BINARY::cADD(ins);  break;
    case XED_ICLASS_INC:  BINARY::cINC(ins);  break;
    case XED_ICLASS_DEC:  BINARY::cDEC(ins);  break;
    case XED_ICLASS_CMP:  BINARY::cCMP(ins);  break;
    case XED_ICLASS_NEG:  BINARY::cNEG(ins);  break;
    case XED_ICLASS_MUL:  BINARY::cIMUL(ins); break; // Traitement identique � MUL
    case XED_ICLASS_IMUL: BINARY::cIMUL(ins); break;
    case XED_ICLASS_IDIV: BINARY::cDIVISION(ins, true); break;  // true = signed
    case XED_ICLASS_DIV:  BINARY::cDIVISION(ins, false); break; // false = unsigned

    // CMOV
    case XED_ICLASS_CMOVB:   CMOV::cCMOVB(ins);    break;
    case XED_ICLASS_CMOVNB:  CMOV::cCMOVNB(ins);   break;
    case XED_ICLASS_CMOVO:   CMOV::cCMOVO(ins);    break;
    case XED_ICLASS_CMOVNO:  CMOV::cCMOVNO(ins);   break;
    case XED_ICLASS_CMOVP:   CMOV::cCMOVP(ins);    break;
    case XED_ICLASS_CMOVNP:  CMOV::cCMOVNP(ins);   break;
    case XED_ICLASS_CMOVS:   CMOV::cCMOVS(ins);    break;
    case XED_ICLASS_CMOVNS:  CMOV::cCMOVNS(ins);   break;
    case XED_ICLASS_CMOVZ:   CMOV::cCMOVZ(ins);    break;
    case XED_ICLASS_CMOVNZ:  CMOV::cCMOVNZ(ins);   break;
    case XED_ICLASS_CMOVBE:  CMOV::cCMOVBE(ins);   break;
    case XED_ICLASS_CMOVNBE: CMOV::cCMOVNBE(ins);  break;
    case XED_ICLASS_CMOVL:   CMOV::cCMOVL(ins);    break;
    case XED_ICLASS_CMOVNL:  CMOV::cCMOVNL(ins);   break;
    case XED_ICLASS_CMOVLE:  CMOV::cCMOVLE(ins);   break;
    case XED_ICLASS_CMOVNLE: CMOV::cCMOVNLE(ins);  break;

    // CONDITIONAL_BRANCH   
    case XED_ICLASS_JB:     CONDITIONAL_BR::cJB(ins);   break;
    case XED_ICLASS_JNB:    CONDITIONAL_BR::cJNB(ins);  break;
    case XED_ICLASS_JO:     CONDITIONAL_BR::cJO(ins);   break;
    case XED_ICLASS_JNO:    CONDITIONAL_BR::cJNO(ins);  break;
    case XED_ICLASS_JP:     CONDITIONAL_BR::cJP(ins);   break;
    case XED_ICLASS_JNP:    CONDITIONAL_BR::cJNP(ins);  break;
    case XED_ICLASS_JS:     CONDITIONAL_BR::cJS(ins);   break;
    case XED_ICLASS_JNS:    CONDITIONAL_BR::cJNS(ins);  break;
    case XED_ICLASS_JZ:     CONDITIONAL_BR::cJZ(ins);   break;
    case XED_ICLASS_JNZ:    CONDITIONAL_BR::cJNZ(ins);  break;
    case XED_ICLASS_JBE:    CONDITIONAL_BR::cJBE(ins);  break;
    case XED_ICLASS_JNBE:   CONDITIONAL_BR::cJNBE(ins); break;
    case XED_ICLASS_JL:     CONDITIONAL_BR::cJL(ins);   break;
    case XED_ICLASS_JNL:    CONDITIONAL_BR::cJNL(ins);  break;
    case XED_ICLASS_JLE:    CONDITIONAL_BR::cJLE(ins);  break;
    case XED_ICLASS_JNLE:   CONDITIONAL_BR::cJNLE(ins); break;
   
    // DATAXFER: MOV, MOVSX, MOVZX, XCHG, BSWAP
    case XED_ICLASS_MOV:   DATAXFER::cMOV(ins);  break;
    case XED_ICLASS_MOVSX: DATAXFER::cMOVSX(ins);  break;
    case XED_ICLASS_MOVZX: DATAXFER::cMOVZX(ins);  break;
    case XED_ICLASS_XCHG:  DATAXFER::cXCHG(ins);  break;
    case XED_ICLASS_BSWAP: DATAXFER::cBSWAP(ins);  break;
    #if TARGET_IA32E
    case XED_ICLASS_MOVSXD: DATAXFER::cMOVSXD(ins);  break;
    #endif 

    // SHIFT: SHL, SHR, SAR (manque SHRD et SHLD)
    case XED_ICLASS_SHL:  SHIFT::cSHL(ins);  break;
    case XED_ICLASS_SHR:  SHIFT::cSHR(ins);  break;
    case XED_ICLASS_SAR:  SHIFT::cSAR(ins);  break;
    case XED_ICLASS_SHRD: SHIFT::cSHRD(ins); break;
    case XED_ICLASS_SHLD: SHIFT::cSHLD(ins); break;

    // LOGICAL: AND, NOT, OR, TEST, XOR
    case XED_ICLASS_NOT:  LOGICAL::cNOT(ins);  break;
    case XED_ICLASS_AND:  LOGICAL::cAND(ins);  break;
    case XED_ICLASS_OR:   LOGICAL::cOR(ins);   break;
    case XED_ICLASS_XOR:  LOGICAL::cXOR(ins);  break;
    case XED_ICLASS_TEST: LOGICAL::cTEST(ins); break;

    // PUSH:
    case XED_ICLASS_PUSH:   PUSH::cPUSH(ins);  break;
    #if TARGET_IA32
    case XED_ICLASS_PUSHA:  PUSH::cPUSHA(ins); break;
    case XED_ICLASS_PUSHAD: PUSH::cPUSHAD(ins); break;
    #endif
    //case XED_ICLASS_PUSHF:    PUSH::PUSHF(ins);  break;
    //case XED_ICLASS_PUSHFD:   PUSH::PUSHFD(ins);  break;
    //case XED_ICLASS_PUSHFQ:   PUSH::PUSHFQ(ins);  break;
       
    // POP:  
    case XED_ICLASS_POP:   POP::cPOP(ins);   break;
    #if TARGET_IA32
    case XED_ICLASS_POPA:  POP::cPOPA(ins);  break;
    case XED_ICLASS_POPAD: POP::cPOPAD(ins); break;
    #endif
    //case XED_ICLASS_POPF:    POP::POPF(ins);  break;
    //case XED_ICLASS_POPFD:   POP::POPFD(ins); break;
    //case XED_ICLASS_POPFQ:   POP::POPFQ(ins); break;
        
    // MISC: LEA, LEAVE (RET Far consid�r� niveau marquage comme ret)
    case XED_ICLASS_LEA:    MISC::cLEA(ins);      break;
    case XED_ICLASS_LEAVE:  MISC::cLEAVE(ins);    break;
    case XED_ICLASS_RET_FAR: // identique en traitement � RET_NEAR
    case XED_ICLASS_RET_NEAR:    RET::cRET(ins);   break;

    // STRINGOP: le second argument est la taille des op�randes, en octets
    case XED_ICLASS_CMPSB:  STRINGOP::cCMPS(ins, 1);  break;
    case XED_ICLASS_CMPSW:  STRINGOP::cCMPS(ins, 2);  break;
    case XED_ICLASS_CMPSD:  STRINGOP::cCMPS(ins, 4);  break;
        
    case XED_ICLASS_LODSB:  STRINGOP::cLODS(ins, 1);  break;
    case XED_ICLASS_LODSW:  STRINGOP::cLODS(ins, 2);  break;
    case XED_ICLASS_LODSD:  STRINGOP::cLODS(ins, 4);  break;

    case XED_ICLASS_MOVSB:  STRINGOP::cMOVS(ins, 1);  break;
    case XED_ICLASS_MOVSW:  STRINGOP::cMOVS(ins, 2);  break;
    case XED_ICLASS_MOVSD:  STRINGOP::cMOVS(ins, 4);  break;

    case XED_ICLASS_SCASB:  STRINGOP::cSCAS(ins, 1);  break;
    case XED_ICLASS_SCASW:  STRINGOP::cSCAS(ins, 2);  break;
    case XED_ICLASS_SCASD:  STRINGOP::cSCAS(ins, 4);  break;

    case XED_ICLASS_STOSB:  STRINGOP::cSTOS(ins, 1);  break;
    case XED_ICLASS_STOSW:  STRINGOP::cSTOS(ins, 2);  break;
    case XED_ICLASS_STOSD:  STRINGOP::cSTOS(ins, 4);  break;

    #if TARGET_IA32E
    case XED_ICLASS_CMPSQ:  STRINGOP::cCMPS(ins, 8);  break;
    case XED_ICLASS_LODSQ:  STRINGOP::cLODS(ins, 8);  break;
    case XED_ICLASS_MOVSQ:  STRINGOP::cMOVS(ins, 8);  break;
    case XED_ICLASS_SCASQ:  STRINGOP::cSCAS(ins, 8);  break;
    case XED_ICLASS_STOSQ:  STRINGOP::cSTOS(ins, 8);  break;
    #endif

    // CALL (call FAR trait� niveau marquage comme un call Near)
    case XED_ICLASS_CALL_FAR: 
    case XED_ICLASS_CALL_NEAR: CALL::cCALL(ins); break;

    // UNCONDITIONAL_BR
    case XED_ICLASS_JMP: UNCONDITIONAL_BR::cJMP(ins); break;
    // les JMP_FAR ne sont pas trait�s pour l'instant
    case XED_ICLASS_JMP_FAR: _LOGDEBUG(" *** JMP_FAR ***"); break;

    // CONVERT
    case XED_ICLASS_CBW:  CONVERT::cCBW(ins);  break;
    case XED_ICLASS_CWDE: CONVERT::cCWDE(ins); break;
    case XED_ICLASS_CWD:  CONVERT::cCWD(ins);  break;
    case XED_ICLASS_CDQ:  CONVERT::cCDQ(ins);  break;
    #if TARGET_IA32E
    case XED_ICLASS_CDQE: CONVERT::cCDQE(ins); break;
    case XED_ICLASS_CQO:  CONVERT::cCQO(ins);  break;
    #endif

    // BIBYTE : SETcc
    case XED_ICLASS_SETB:   BITBYTE::cSETB(ins);    break;
    case XED_ICLASS_SETNB:  BITBYTE::cSETNB(ins);   break;
    case XED_ICLASS_SETO:   BITBYTE::cSETO(ins);    break;
    case XED_ICLASS_SETNO:  BITBYTE::cSETNO(ins);   break;
    case XED_ICLASS_SETP:   BITBYTE::cSETP(ins);    break;
    case XED_ICLASS_SETNP:  BITBYTE::cSETNP(ins);   break;
    case XED_ICLASS_SETS:   BITBYTE::cSETS(ins);    break;
    case XED_ICLASS_SETNS:  BITBYTE::cSETNS(ins);   break;
    case XED_ICLASS_SETZ:   BITBYTE::cSETZ(ins);    break;
    case XED_ICLASS_SETNZ:  BITBYTE::cSETNZ(ins);   break;
    case XED_ICLASS_SETBE:  BITBYTE::cSETBE(ins);   break;
    case XED_ICLASS_SETNBE: BITBYTE::cSETNBE(ins);  break;
    case XED_ICLASS_SETL:   BITBYTE::cSETL(ins);    break;
    case XED_ICLASS_SETNL:  BITBYTE::cSETNL(ins);   break;
    case XED_ICLASS_SETLE:  BITBYTE::cSETLE(ins);   break;
    case XED_ICLASS_SETNLE: BITBYTE::cSETNLE(ins);  break;
      
    // BIBYTE : BTxx, BSR, BSF
    case XED_ICLASS_BT:  BITBYTE::cBT(ins);  break;
    case XED_ICLASS_BTC: BITBYTE::cBTC(ins); break;
    case XED_ICLASS_BTR: BITBYTE::cBTR(ins); break;
    case XED_ICLASS_BTS: BITBYTE::cBTS(ins); break;
    case XED_ICLASS_BSR: BITBYTE::cBSR(ins); break;
    case XED_ICLASS_BSF: BITBYTE::cBSF(ins); break;
   
    // SEMAPHORE
    case XED_ICLASS_CMPXCHG : SEMAPHORE::cCMPXCHG(ins); break;
    case XED_ICLASS_CMPXCHG8B : SEMAPHORE::cCMPXCHG8B(ins); break;
    #if TARGET_IA32E
    case XED_ICLASS_CMPXCHG16B : SEMAPHORE::cCMPXCHG16B(ins); break;
    #endif

    // ROTATE
    case XED_ICLASS_ROL: ROTATE::cROL(ins); break;
    case XED_ICLASS_ROR: ROTATE::cROR(ins); break;
    case XED_ICLASS_RCL: ROTATE::cRCL(ins); break;
    case XED_ICLASS_RCR: ROTATE::cRCR(ins); break;
    
   
    default: // fonction non trait�e : d�marquage de(s) destination(s)
        UTILS::cUNHANDLED(ins);     
        _LOGUNHANDLED(ins);
        break;
    }
    _LOGDEBUG(""); // juste un retour chariot
}

// Fonction de notification lors de la fin de l'ex�cution
void INSTRUMENTATION::FiniTaint(INT32 code, void* ) 
{
    PIN_GetLock(&g_lock, 0);
    
    // envoi des dernieres donn�es r�colt�es
    g_pFormula->final();

#if DEBUG
    // fermeture des fichiers de log
    clock_t totalTime = clock() - g_timeBegin;

    g_debug << "Temps ecoule (ticks) : " << decstr(totalTime);
    g_debug << " soit " << ((float) totalTime)/CLOCKS_PER_SEC << " secondes" << std::endl;
    g_debug << "#eof\n";    
    g_debug.close();

    // r�cup�ration du nombre d'objets encore marqu�s en m�moire
    auto taintedMem = pTmgrGlobal->getSnapshotOfTaintedLocations();
    g_taint << "nombre d'octets encore marqu�s : " << taintedMem.size() << std::endl;
    g_taint << "#eof\n";    
    g_taint.close();
#else   
    // flush puis d�connexion du pipe, en mode release
    WINDOWS::FlushFileBuffers(g_hPipe);
    WINDOWS::CloseHandle(g_hPipe);     
#endif    

    // lib�ration des classes globales
    delete (pTmgrGlobal);
    delete (g_pFormula);

    // lib�ration des slots de la TLS
    PIN_DeleteThreadDataKey(g_tlsKeyTaint);
    PIN_DeleteThreadDataKey(g_tlsKeySyscallData);

    // Lib�ration du verrou
    PIN_ReleaseLock(&g_lock);

    // fin forc�e (sans attendre le thread "maxtime" s'il s'ex�cute encore)
    PIN_ExitProcess(code);
}

// Fonction de notification lors du lancement d'un thread
void INSTRUMENTATION::threadStart(THREADID tid, CONTEXT *, INT32 , VOID *)
{
    _LOGDEBUG("Cr�ation du thread n� " << tid << " et TLS associ�e");

    // classe de gestion du marquage des registres
    TaintManager_Thread *pTmgrTls = new TaintManager_Thread;
    Syscall_Data *pSyscallData    = new Syscall_Data;

    PIN_SetThreadData(g_tlsKeyTaint, pTmgrTls, tid);
    PIN_SetThreadData(g_tlsKeySyscallData, pSyscallData, tid);
}

// Fonction de notification lors de la fin d'un thread
void INSTRUMENTATION::threadFini(THREADID tid, const CONTEXT *, INT32 , VOID *)
{
    _LOGDEBUG("destruction du thread n� " << tid << " et TLS associ�e");

    TaintManager_Thread *pTmgrTls = 
        static_cast<TaintManager_Thread*>(PIN_GetThreadData(g_tlsKeyTaint, tid));
    delete (pTmgrTls);

    Syscall_Data *pSysData = 
        static_cast<Syscall_Data*>(PIN_GetThreadData(g_tlsKeySyscallData, tid));
    delete (pSysData);
}

/**************************/
/*** PINTOOL CHECKSCORE ***/
/**************************/

// Fonction de notification lors du changement de contexte
void INSTRUMENTATION::changeCtx
    (THREADID, CONTEXT_CHANGE_REASON reason, const CONTEXT* , CONTEXT* , INT32 sig, VOID*) 
{
    if (reason == CONTEXT_CHANGE_REASON_EXCEPTION) 
    {
        // remise � z�ro du nombre d'instructions => erreur
        PIN_GetLock(&g_lock, PIN_GetTid());
        g_nbIns = 0;
        PIN_ReleaseLock(&g_lock);

        // appel de la fonction "FiniCheckscore" avant de quitter le programme
        PIN_ExitApplication(EXIT_EXCEPTION); 
    }
}

// Fonction de notification lors de la fin de l'ex�cution
void INSTRUMENTATION::FiniCheckScore(INT32 code, void* ) 
{
    WINDOWS::DWORD cbWritten;
    std::string message;

    // si exception, le message sera '0'
    // sinon ce sera le nombre d'instructions ex�cut�es
    message = decstr(g_nbIns);

    // envoi du resultat en entier dans le pipe  (= stdout en mode debug)
    WINDOWS::WriteFile(g_hPipe, 
        message.c_str(), 
        static_cast<WINDOWS::DWORD>(message.size()), // cast pour eviter C4267 en x64 
        &cbWritten, 
        NULL);
    WINDOWS::FlushFileBuffers(g_hPipe);

    // fin forc�e (sans attendre le thread "maxtime" s'il s'ex�cute encore)
    PIN_ExitProcess(code);
}

// Fonction d'instrumentation des traces
// ajout du nombre d'instructions de la trace au total
void INSTRUMENTATION::insCount(TRACE trace, VOID *) 
{
    PIN_GetLock(&g_lock, PIN_GetTid());
    for (BBL bbl = TRACE_BblHead(trace) ; BBL_Valid(bbl) ; bbl = BBL_Next(bbl))
    {
        g_nbIns += BBL_NumIns(bbl);
    }
    PIN_ReleaseLock(&g_lock);
}