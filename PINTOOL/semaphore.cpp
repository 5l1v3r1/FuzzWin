#include "semaphore.h"
#include "TaintManager.h"

// CALLBACKS
void SEMAPHORE::cCMPXCHG(INS &ins)
{
    // CMPXCHG : d'abord une comparaison destination (R ou M) avec 
    // puis 1 MOV selon le resultat de la comparaison
    // Op�rande 0 : ModRM:r/m (si reg : regDest)
    // Op�rande 1 : reg (d�nomm� regSrc)
    // CMPXCHG compare op�rande 0 avec AL/AX/EAX/RAX
    // Si �gal,       ZF = 1 (comme une vraie comparaison) et op�rande 1 -> op�rande 0 
    // Si diff�rent : ZF = 0 (comme une vraie comparaison) et op�rande 0 -> AL/AX/EAX/RAX
    // Dans les arguments des callbacks, il vaut ajouter la valeur de AL/AX/EAX/RAX
    // Et celle du registre dest (cas RR) pour faire la comparaison.
    
    // op�rande 1 : regSrc
    REG regSrc = INS_OperandReg(ins, 1);
    UINT32 regSrcSize = getRegSize(regSrc);
    if (!regSrcSize) return;	// registre non suivi
    
    // registre de comparasion avec la destination
    REG cmpReg = REG_INVALID(); // selon la taille, AL/AX/EAX/RAX

    // pointeur vers fonction � appeler
    void (*callback)() = nullptr;
    if (INS_IsMemoryRead(ins)) // CMPXCHG_RM (source 0 = m�moire)
    {
        switch (regSrcSize) 
        {	
        case 1:	callback = (AFUNPTR) sCMPXCHG_RM<8>;  cmpReg = REG_AL; break;
        case 2:	callback = (AFUNPTR) sCMPXCHG_RM<16>; cmpReg = REG_AX; break;
        case 4:	callback = (AFUNPTR) sCMPXCHG_RM<32>; cmpReg = REG_EAX; break;
        #if TARGET_IA32E
        case 8: callback = (AFUNPTR) sCMPXCHG_RM<64>; cmpReg = REG_RAX; break;
        #endif
        }
        INS_InsertCall (ins, IPOINT_BEFORE, callback,
            IARG_THREAD_ID,
            IARG_UINT32, regSrc, // pas besoin de la valeur du registre, le d�placement se fera octet par octet
            IARG_MEMORYREAD_EA, 
            IARG_UINT32,    cmpReg,
            IARG_REG_VALUE, cmpReg,
            CALLBACK_DEBUG IARG_END);      
    }
    else // CMPXCHG_RR (source 0 = registre)
    {
        REG regDest = INS_OperandReg(ins, 0); // registre de destination (compar� � AL/AX/EAX/RAX)
        switch (getRegSize(regDest)) 
        {	
        case 0: return; // non suivi en instrumentation
        case 1:	callback = (AFUNPTR) sCMPXCHG_RR<8>;  cmpReg = REG_AL; break;
        case 2:	callback = (AFUNPTR) sCMPXCHG_RR<16>; cmpReg = REG_AX; break;
        case 4:	callback = (AFUNPTR) sCMPXCHG_RR<32>; cmpReg = REG_EAX; break;
        #if TARGET_IA32E
        case 8: callback = (AFUNPTR) sCMPXCHG_RR<64>; cmpReg = REG_RAX; break;
        #endif
        }
        INS_InsertCall (ins, IPOINT_BEFORE, callback,
            IARG_THREAD_ID,
            IARG_UINT32, regSrc, 
            IARG_UINT32, regDest, 
            IARG_REG_VALUE, regDest,// valeur indispendable pour comparaison � RAX
            IARG_UINT32,    cmpReg,
            IARG_REG_VALUE, cmpReg,
            CALLBACK_DEBUG IARG_END);     
    }
} // cCMPXCHG
    
void SEMAPHORE::cCMPXCHG8B(INS &ins)
{
    // CMPXCHG8B (mode x86): d'abord une comparaison EDX:EAX avec m64
    // puis 1 MOV ECX:EBX->m64 ou m64->EDX:EAX selon le resultat de la comparaison 
    // Op�rande 0 : M�moire
    // Dans les arguments des callbacks, il vaut ajouter la valeur de EAX et EDX

    INS_InsertCall (ins, IPOINT_BEFORE, (AFUNPTR) sCMPXCHG8B,
        IARG_THREAD_ID,
        IARG_MEMORYREAD_EA, 
        IARG_REG_VALUE, REG_EAX,
        IARG_REG_VALUE, REG_EDX,
        CALLBACK_DEBUG IARG_END); 

} // cCMPXCHG8B

#if TARGET_IA32E
void SEMAPHORE::cCMPXCHG16B(INS &ins)
{
    // CMPXCHG16B (mode x64): d'abord une comparaison RDX:RAX avec m128
    // puis 1 MOV RCX:RBX->m128 ou m128->RDX:RAX selon le resultat de la comparaison 
    // Op�rande 0 : M�moire
    // Dans les arguments des callbacks, il vaut ajouter la valeur de RAX et RDX

    INS_InsertCall (ins, IPOINT_BEFORE, (AFUNPTR) sCMPXCHG16B,
        IARG_THREAD_ID,
        IARG_MEMORYREAD_EA, 
        IARG_REG_VALUE, REG_RAX,
        IARG_REG_VALUE, REG_RDX,
        CALLBACK_DEBUG IARG_END);  
    
} // cCMPXCHG16B
#endif

void SEMAPHORE::cXADD(INS &ins)
{
    // pointeur vers fonction � appeler
    void (*callback)() = nullptr;

    // XADD : op�rande 0 est soit m�moire, soit registre, op�rande 1 tjs registre
    REG regSrc = INS_OperandReg(ins, 1);
    UINT32 regSrcSize = getRegSize(regSrc);

    if (!regSrcSize) return;    // registre non g�r� en marquage
    else if (INS_OperandIsMemory(ins, 0)) // XADD_M (op�rande 0 = m�moire)
    {
        switch (regSrcSize) 
        {	
        case 1:	callback = (AFUNPTR) sXADD_M<8>;  break;
        case 2:	callback = (AFUNPTR) sXADD_M<16>;  break;
        case 4:	callback = (AFUNPTR) sXADD_M<32>;  break;
        #if TARGET_IA32E
        case 8: callback = (AFUNPTR) sXADD_M<64>;  break;
        #endif
        }
        INS_InsertCall (ins, IPOINT_BEFORE, callback,
            IARG_THREAD_ID,
            IARG_UINT32,    regSrc,
            IARG_REG_VALUE, regSrc,
            IARG_MEMORYWRITE_EA, 
            CALLBACK_DEBUG IARG_END); 
    }
    else  // XADD_R (op�rande 0 = registre)
    {
        REG regDest = INS_OperandReg(ins, 0);
        switch (getRegSize(regDest)) 
        {	
        case 1:	callback = (AFUNPTR) sXADD_R<8>;  break;
        case 2:	callback = (AFUNPTR) sXADD_R<16>;  break;
        case 4:	callback = (AFUNPTR) sXADD_R<32>;  break;
        #if TARGET_IA32E
        case 8: callback = (AFUNPTR) sXADD_R<64>;  break;
        #endif
        }
        INS_InsertCall (ins, IPOINT_BEFORE, callback,
            IARG_THREAD_ID,
            IARG_UINT32,    regSrc,
            IARG_REG_VALUE, regSrc,
            IARG_UINT32,    regDest,
            IARG_REG_VALUE, regDest,
            CALLBACK_DEBUG IARG_END); 
    }
} // cXADD

// SIMULATE
void SEMAPHORE::sCMPXCHG8B(THREADID tid, ADDRINT address, ADDRINT regEAXValue, ADDRINT regEDXValue ADDRESS_DEBUG)
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    
    // 1ere partie de CMPXCHG8B :
    // CMP_RM entre EDX:EAX et m64. 
    // Proc�dure sp�cifique (impossible de faire appel � BINARY::sCMP_RM)
    
    bool isMemTainted       = pTmgrGlobal->isMemoryTainted<64>(address); 
    ADDRINT srcMemLowValue  = getMemoryValue<32>(address);
    ADDRINT srcMemHighValue = getMemoryValue<32>(address + 3);
 
    bool isEAXTainted =	pTmgrTls->isRegisterTainted<32>(REG_EAX);
    bool isEDXTainted =	pTmgrTls->isRegisterTainted<32>(REG_EDX);
    
    // marquage flags : seul ZF est affect�, les autres (O/S/A/P/C) sont inchang�s
    if ( !(isMemTainted || isEAXTainted || isEDXTainted))  pTmgrTls->unTaintZeroFlag();
    else 
    {
        _LOGTAINT("CMPXCHG8B - FLAG MARQUE"); 
        // pour repr�senter EDX:EAX et la m�moire il n'est pas possible de faire une concat�nation
        // car cela ne pourra pas traiter le cas ou les deux registres sont d�marqu�s
        // on pourrait envisager de faire un ObjectSource d'1 valeur de 64bits
        // mais afin d'uniformiser la proc�dure avec CMPXCHG16B (128bits)
        // on utilisera donc une relation sp�cifique pour CMPXCHG 8B et 16B
        // on prendra 4 arguments : memLow, memHigh, EAX, EDX. 

        ObjectSource srcMemLow = (pTmgrGlobal->isMemoryTainted<32>(address))
            ? ObjectSource(pTmgrGlobal->getMemoryTaint<32>(address))
            : ObjectSource(32, srcMemLowValue);

        ObjectSource srcMemHigh = (pTmgrGlobal->isMemoryTainted<32>(address + 3))
            ? ObjectSource(pTmgrGlobal->getMemoryTaint<32>(address + 3))
            : ObjectSource(32, srcMemHighValue);

        ObjectSource srcEAX = (isEAXTainted) 
            ? ObjectSource(pTmgrTls->getRegisterTaint<32>(REG_EAX, regEAXValue))
            : ObjectSource(32, regEAXValue);

        ObjectSource srcEDX = (isEDXTainted) 
            ? ObjectSource(pTmgrTls->getRegisterTaint<32>(REG_EDX, regEDXValue))
            : ObjectSource(32, regEDXValue);

        
        pTmgrTls->updateTaintCarryFlag(std::make_shared<TaintBit>(
            F_CMPXCHG_8B16B, 
            srcMemHigh, srcMemLow,
            srcEDX, srcEAX));
    }
     
    /**** 2EME PARTIE : ECHANGE (INDEPENDANT DU MARQUAGE DE ZF) ***/
    // si mem64 == EDX:EAX, alors mem <- ECX:EBX
    // on fera ici un MOVRM de EBX dans addr (traitement partie basse)
    // puis        un MOVRM de ECX dans addr + 3 (tr.    partie haute)
    if ((srcMemLowValue == regEAXValue) && (srcMemHighValue == regEDXValue))
    {
        DATAXFER::sMOV_RM<32>(tid, REG_EBX, address       INSADDRESS);
        DATAXFER::sMOV_RM<32>(tid, REG_ECX, (address + 3) INSADDRESS);
    }
    // sinon EDX:EAX <- mem
    // on fera un MOVMR de addr     dans EAX (partie basse)
    // puis    un MOVMR de addr + 3 dans EDX (partie haute)
    else
    {
        DATAXFER::sMOV_MR<32>(tid, address,       REG_EAX INSADDRESS);
        DATAXFER::sMOV_MR<32>(tid, (address + 3), REG_ECX INSADDRESS);
    }
}// sCMPXCHG8B


#if TARGET_IA32E
void SEMAPHORE::sCMPXCHG16B(THREADID tid, ADDRINT address, ADDRINT regRAXValue, ADDRINT regRDXValue ADDRESS_DEBUG)
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    
    // 1ere partie de CMPXCHG16B :
    // CMP_RM entre RDX:RAX et m128. 
    // Proc�dure sp�cifique (impossible de faire appel � BINARY::sCMP_RM)
    
    bool isMemTainted       = pTmgrGlobal->isMemoryTainted<128>(address); 
    ADDRINT srcMemLowValue  = getMemoryValue<64>(address);
    ADDRINT srcMemHighValue = getMemoryValue<64>(address + 7);
 
    bool isRAXTainted =	pTmgrTls->isRegisterTainted<64>(REG_EAX);
    bool isRDXTainted =	pTmgrTls->isRegisterTainted<64>(REG_EDX);
    
    // marquage flags : seul ZF est affect�, les autres (O/S/A/P/C) sont inchang�s
    if ( !(isMemTainted || isRAXTainted || isRDXTainted))  pTmgrTls->unTaintAllFlags();
    else 
    {
        _LOGTAINT("CMPXCHG16B - FLAG MARQUE"); 
        // pour repr�senter RDX:RAX et la m�moire il n'est pas possible de faire une concat�nation
        // car cela ne pourra pas traiter le cas ou les deux registres sont d�marqu�s
        // cf CMPXCHG16B

        ObjectSource srcMemLow = (pTmgrGlobal->isMemoryTainted<64>(address))
            ? ObjectSource(pTmgrGlobal->getMemoryTaint<64>(address))
            : ObjectSource(64, srcMemLowValue);

        ObjectSource srcMemHigh = (pTmgrGlobal->isMemoryTainted<64>(address + 7))
            ? ObjectSource(pTmgrGlobal->getMemoryTaint<64>(address + 7))
            : ObjectSource(64, srcMemHighValue);

        ObjectSource srcRAX = (isRAXTainted) 
            ? ObjectSource(pTmgrTls->getRegisterTaint<64>(REG_RAX, regRAXValue))
            : ObjectSource(64, regRAXValue);

        ObjectSource srcRDX = (isRDXTainted) 
            ? ObjectSource(pTmgrTls->getRegisterTaint<64>(REG_RDX, regRDXValue))
            : ObjectSource(64, regRDXValue);

        // marquage flags : seul ZF est affect�, les autres (O/S/A/P/C) sont inchang�s
        pTmgrTls->updateTaintCarryFlag(std::make_shared<TaintBit>(
            F_CMPXCHG_8B16B, 
            srcMemHigh, srcMemLow,
            srcRDX, srcRAX));
    }
     
    /**** 2EME PARTIE : ECHANGE (INDEPENDANT DU MARQUAGE DE ZF) ***/
    // si mem128 == RDX:RAX, alors mem <- RCX:RBX
    // on fera ici un MOVRM de RBX dans addr (traitement partie basse)
    // puis        un MOVRM de RCX dans addr + 3 (tr.    partie haute)
    if ((srcMemLowValue == regRAXValue) && (srcMemHighValue == regRDXValue))
    {
        DATAXFER::sMOV_RM<64>(tid, REG_RBX, address       INSADDRESS);
        DATAXFER::sMOV_RM<64>(tid, REG_RCX, (address + 3) INSADDRESS);
    }
    // sinon RDX:RAX <- mem
    // on fera un MOVMR de addr     dans RAX (partie basse)
    // puis    un MOVMR de addr + 3 dans RDX (partie haute)
    else
    {
        DATAXFER::sMOV_MR<64>(tid, address,       REG_RAX INSADDRESS);
        DATAXFER::sMOV_MR<64>(tid, (address + 3), REG_RCX INSADDRESS);
    }
} // sCMPXCHG16B
#endif