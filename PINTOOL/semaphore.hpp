template<UINT32 lengthInBits> void SEMAPHORE::sCMPXCHG_RM
    (THREADID tid, REG regSrc, ADDRINT address, REG cmpReg, ADDRINT cmpRegValue ADDRESS_DEBUG)
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    
    // 1ere partie de CMPXCHG : il s'agit en fait d'un CMP_RM
    // donc appeler la fonction correspondante impl�ment�e dans BINARY
    // qui marquera les flags en cons�quence
    BINARY::sCMP_RM<lengthInBits>(tid, cmpReg, cmpRegValue, address INSADDRESS);

    // 2�me partie de CMPXCHG
    // si op�randes sont �gales : MOV regSrc->Memoire
    // sinon                      MOV M�moire -> cmpReg (AL..RAX)
    ADDRINT addressValue = getMemoryValue<lengthInBits>(address);
    if (addressValue == cmpRegValue)   DATAXFER::sMOV_RM<lengthInBits>(tid, regSrc, address INSADDRESS);
    else                               DATAXFER::sMOV_MR<lengthInBits>(tid, address, cmpReg INSADDRESS);
} // sCMPXCHG_RM

template<UINT32 lengthInBits> void SEMAPHORE::sCMPXCHG_RR
    (THREADID tid, REG regSrc, REG regDest, ADDRINT regDestValue, REG cmpReg, ADDRINT cmpRegValue ADDRESS_DEBUG)
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    
    // 1ere partie de CMPXCHG : il s'agit en fait d'un CMP_RR
    // donc appeler la fonction correspondante impl�ment�e dans BINARY
    // qui marquera les flags en cons�quence
    BINARY::sCMP_RR<lengthInBits>(tid, cmpReg, cmpRegValue, regDest, regDestValue INSADDRESS);

    // 2�me partie de CMPXCHG
    // si op�randes sont �gales : MOV regSrc->Memoire
    // sinon                      MOV RegDest -> cmpReg (AL..RAX)
    if (regDestValue == cmpRegValue)   DATAXFER::sMOV_RR<lengthInBits>(tid, regSrc, regDest INSADDRESS);
    else                               DATAXFER::sMOV_RR<lengthInBits>(tid, regDest, cmpReg INSADDRESS);
} // sCMPXCHG_RR

