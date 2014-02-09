/////////
// MOV //
/////////

// SIMULATE
template<UINT32 len> void DATAXFER::sMOV_RR(THREADID tid, REG regSrc, REG regDest ADDRESS_DEBUG) 
{
    // pas besoin de tester le marquage de la source : perte de temps
    // car isRegisterTainted va faire une boucle pour tester le marquage, 
    // et le marquage est �galement test� octet par octet dans la boucle for ci dessous
    REGINDEX regIndexDest = getRegIndex(regDest);	
    REGINDEX regIndexSrc  = getRegIndex(regSrc);
    TaintManager_Thread *pTmgrTls = static_cast<TaintManager_Thread*>(PIN_GetThreadData(g_tlsKeyTaint, tid));
    
    for (UINT32 regPart = 0 ; regPart < (len >> 3) ; ++regPart) 
    {	
        if (pTmgrTls->isRegisterPartTainted(regIndexSrc, regPart))  // octet marqu� ? 
        {	
            _LOGTAINT("movRR" << len << " octet " << regPart);
            pTmgrTls->updateTaintRegisterPart(regIndexDest, regPart, std::make_shared<TaintByte>(
                X_ASSIGN, 
                ObjectSource(pTmgrTls->getRegisterPartTaint(regIndexSrc, regPart))));
        }
        else pTmgrTls->unTaintRegisterPart(regIndexDest, regPart);
    }
} // sMOV_RR

template<UINT32 len> void DATAXFER::sMOV_RM(THREADID tid, REG regSrc, ADDRINT writeAddress ADDRESS_DEBUG) 
{
    // pas besoin de tester le marquage de la source : perte de temps
    // car isRegisterTainted va faire une boucle pour tester le marquage, 
    // et le marquage est �galement test� octet par octet dans la boucle for ci dessous
    REGINDEX regIndex = getRegIndex(regSrc);
    TaintManager_Thread *pTmgrTls = static_cast<TaintManager_Thread*>(PIN_GetThreadData(g_tlsKeyTaint, tid));
    
    for (UINT32 regPart = 0 ; regPart < (len >> 3) ; ++regPart, ++writeAddress) 
    {	
        if (pTmgrTls->isRegisterPartTainted(regIndex, regPart))  // octet marqu� ? 
        {  
            _LOGTAINT("movRM" << len << " octet " << regPart);	
            pTmgrGlobal->updateMemoryTaint<8>(writeAddress, std::make_shared<TaintByte>(
                X_ASSIGN, 
                ObjectSource(pTmgrTls->getRegisterPartTaint(regIndex, regPart))));
        }
        else pTmgrGlobal->unTaintMemory<8>(writeAddress);   
    }
} // sMOV_RM

template<UINT32 len> void DATAXFER::sMOV_MR(THREADID tid, ADDRINT readAddress, REG regDest ADDRESS_DEBUG) 
{
    REGINDEX regIndex = getRegIndex(regDest);	
    TaintManager_Thread *pTmgrTls = static_cast<TaintManager_Thread*>(PIN_GetThreadData(g_tlsKeyTaint, tid));
    
    for (UINT32 regPart = 0 ; regPart < (len >> 3) ; ++regPart, ++readAddress)
    {
        if (pTmgrGlobal->isMemoryTainted<8>(readAddress))  // octet marqu� ? 
        {	
            _LOGTAINT("movMR" << len << " octet " << regPart);	
            pTmgrTls->updateTaintRegisterPart(regIndex, regPart, std::make_shared<TaintByte>(
                X_ASSIGN, 
                ObjectSource(pTmgrGlobal->getMemoryTaint<8>(readAddress))));
        }
        else pTmgrTls->unTaintRegisterPart(regIndex, regPart); 
    }
} // sMOV_MR

//////////
// XCHG //
//////////

// SIMULATE
template<UINT32 len> void DATAXFER::sXCHG_M(THREADID tid, REG reg, ADDRINT address ADDRESS_DEBUG) 
{
    TaintManager_Thread *pTmgrTls = static_cast<TaintManager_Thread*>(PIN_GetThreadData(g_tlsKeyTaint, tid));
    
    TaintBytePtr tbTempPtr; // variable de mise en cache
    REGINDEX regIndex = getRegIndex(reg);

    for (UINT32 regPart = 0 ; regPart < (len >> 3) ; ++regPart, ++address) 
    {				
        // r�cup�ration du marquage de la m�moire (nullptr si absent)
        tbTempPtr = pTmgrGlobal->getMemoryTaint<8>(address);

        // r�cup�ration du marquage du registre et affectation directe � la m�moire
        if (pTmgrTls->isRegisterPartTainted(regIndex, regPart))
        {
            pTmgrGlobal->updateMemoryTaint<8>(address, pTmgrTls->getRegisterPartTaint(regIndex, regPart));
        }
        else pTmgrGlobal->unTaintMemory<8>(address);

        // affectation de la variable temporaire au registre
        if (tbTempPtr) pTmgrTls->updateTaintRegisterPart(regIndex, regPart, tbTempPtr);
        else pTmgrTls->unTaintRegisterPart(regIndex, regPart);
    }
} // sXCHG_M

template<UINT32 len> void DATAXFER::sXCHG_R(THREADID tid, REG regSrc, REG regDest ADDRESS_DEBUG) 
{
    TaintManager_Thread *pTmgrTls = static_cast<TaintManager_Thread*>(PIN_GetThreadData(g_tlsKeyTaint, tid));
    
    TaintBytePtr tbTempPtr; // variable de mise en cache
    REGINDEX regIndexSrc = getRegIndex(regSrc);
    REGINDEX regIndexDest = getRegIndex(regDest);

    for (UINT32 regPart = 0 ; regPart < (len >> 3) ; ++regPart) 
    {				
        // r�cup�ration du marquage du registre destination (nullptr si absent)
        tbTempPtr = pTmgrTls->getRegisterPartTaint(regIndexDest, regPart); 

        // r�cup�ration du marquage du registre src et affectation directe � la destination
        if (pTmgrTls->isRegisterPartTainted(regIndexSrc, regPart))
        {
            pTmgrTls->updateTaintRegisterPart(regIndexDest, regPart, pTmgrTls->getRegisterPartTaint(regIndexSrc, regPart));
        }
        else pTmgrTls->unTaintRegisterPart(regIndexDest, regPart);

        // affectation de la variable temporaire au registre
        if (tbTempPtr) pTmgrTls->updateTaintRegisterPart(regIndexSrc, regPart, tbTempPtr);
        else pTmgrTls->unTaintRegisterPart(regIndexSrc, regPart);
    }
} // sXCHG_R

///////////
// MOVSX //
///////////

// SIMULATE

template<UINT32 lenSrc, UINT32 lenDest> 
void DATAXFER::sMOVSX_RR(THREADID tid, REG regSrc, ADDRINT regSrcValue, REG regDest ADDRESS_DEBUG)
{
    TaintManager_Thread *pTmgrTls = static_cast<TaintManager_Thread*>(PIN_GetThreadData(g_tlsKeyTaint, tid));
    
    if ( !pTmgrTls->isRegisterTainted<lenSrc>(regSrc)) pTmgrTls->unTaintRegister<lenDest>(regDest);
    else 
    {
        _LOGTAINT("movsxRR" << lenSrc << "->" << lenDest << " ");
        // cr�ation d'un nouvel objet de taille = � la destination
        // avec comme source l'objet de taille = � la source
        // et affectation au registre de destination
        pTmgrTls->updateTaintRegister<lenDest>(regDest, std::make_shared<TaintObject<lenDest>>
            (X_SIGNEXTEND, 
            ObjectSource(pTmgrTls->getRegisterTaint<lenSrc>(regSrc, regSrcValue))));
    }
} // sMOVSX_RR

template<UINT32 lenSrc, UINT32 lenDest>
void DATAXFER::sMOVSX_MR(THREADID tid, ADDRINT readAddress, REG regDest ADDRESS_DEBUG) 
{
    TaintManager_Thread *pTmgrTls = static_cast<TaintManager_Thread*>(PIN_GetThreadData(g_tlsKeyTaint, tid));
    
    if ( !pTmgrGlobal->isMemoryTainted<lenSrc>(readAddress)) pTmgrTls->unTaintRegister<lenDest>(regDest);
    else 
    {
        _LOGTAINT("movsxMR" << lenSrc << "->" << lenDest << " ");
        // cr�ation d'un nouvel objet de taille = � la destination
        // avec comme source l'objet de taille = � la source
        // et affectation au registre de destination
        pTmgrTls->updateTaintRegister<lenDest>(regDest, std::make_shared<TaintObject<lenDest>>
            (X_SIGNEXTEND, 
            ObjectSource(pTmgrGlobal->getMemoryTaint<lenSrc>(readAddress))));
    }
} // sMOVSX_MR

///////////
// MOVZX //
///////////

// SIMULATE
template<UINT32 lenSrc, UINT32 lenDest>
void DATAXFER::sMOVZX_RR(THREADID tid, REG regSrc, REG regDest ADDRESS_DEBUG) 
{
    TaintManager_Thread *pTmgrTls = static_cast<TaintManager_Thread*>(PIN_GetThreadData(g_tlsKeyTaint, tid));
    
    // quoi qu'il arrive, effacement du marquage de la destination
    // en effet partie basse sera marqu�e (ou non) par le registre source 
    // et la partie haute sera d�marqu�e (mise � z�ro)
    pTmgrTls->unTaintRegister<lenDest>(regDest);

    // marquage partie basse
    if (pTmgrTls->isRegisterTainted<lenSrc>(regSrc)) 
    { 
        _LOGTAINT("movzxRR" << lenSrc << "->" << lenDest << " ");
        REGINDEX regIndexSrc  = getRegIndex(regSrc);
        REGINDEX regIndexDest = getRegIndex(regDest);

        // d�placement des len>>3 octets de la source vers la destination
        for (UINT32 regPart = 0 ; regPart < (lenSrc >> 3) ; ++regPart) 
        { 
            if (pTmgrTls->isRegisterPartTainted(regIndexSrc, regPart)) // marqu�?
            {	
                pTmgrTls->updateTaintRegisterPart(regIndexDest, regPart, std::make_shared<TaintByte>(
                    X_ASSIGN, 
                    ObjectSource(pTmgrTls->getRegisterPartTaint(regIndexSrc, regPart))));
            }
        }
    }
} // sMOVZX_RR

template<UINT32 lenDest> void DATAXFER::sMOVZX_8R(THREADID tid, REG regSrc, REG regDest ADDRESS_DEBUG) 
{
    TaintManager_Thread *pTmgrTls = static_cast<TaintManager_Thread*>(PIN_GetThreadData(g_tlsKeyTaint, tid));
    
    // quoi qu'il arrive, effacement du marquage de la destination
    // en effet octet faible sera marqu� (ou non) et octets forts mis � z�ro
    pTmgrTls->unTaintRegister<lenDest>(regDest);
  
    if (pTmgrTls->isRegisterTainted<8>(regSrc))
    {
        _LOGTAINT("moxzxRR8->" << lenDest << " ");
        REGINDEX regIndex = getRegIndex(regDest);
        pTmgrTls->updateTaintRegisterPart(regIndex, 0, std::make_shared<TaintByte>(
            X_ASSIGN, 
            ObjectSource(pTmgrTls->getRegisterTaint(regSrc))));
    }
} // sMOVZX_8R

template<UINT32 lenSrc, UINT32 lenDest> 
void DATAXFER::sMOVZX_MR(THREADID tid, ADDRINT readAddress, REG regDest ADDRESS_DEBUG) 
{
    TaintManager_Thread *pTmgrTls = static_cast<TaintManager_Thread*>(PIN_GetThreadData(g_tlsKeyTaint, tid));
    
    // quoi qu'il arrive, effacement du marquage de la destination
    // en effet partie basse sera marqu�e (ou non) par la m�moire source
    // et la partie haute sera d�marqu�e (mise � z�ro)
    pTmgrTls->unTaintRegister<lenDest>(regDest);
    
    if ( pTmgrGlobal->isMemoryTainted<lenSrc>(readAddress)) 
    { 
        _LOGTAINT("movzxMR");
    
        REGINDEX regIndex = getRegIndex(regDest);	
        for (UINT32 regPart = 0 ; regPart < (lenSrc >> 3) ; ++regPart, ++readAddress) 
        {
            if (pTmgrGlobal->isMemoryTainted<8>(readAddress)) 
            {	
                pTmgrTls->updateTaintRegisterPart(regIndex, regPart, std::make_shared<TaintByte>(
                    X_ASSIGN, 
                    ObjectSource(pTmgrGlobal->getMemoryTaint<8>(readAddress))));
            }
        }
    }
} // sMOVZX_MR

///////////
// BSWAP //
///////////

template<UINT32 len> void DATAXFER::sBSWAP(THREADID tid, REG reg ADDRESS_DEBUG) 
{  
    TaintManager_Thread *pTmgrTls = static_cast<TaintManager_Thread*>(PIN_GetThreadData(g_tlsKeyTaint, tid));
    
    REGINDEX regIndex = getRegIndex(reg);
    // variable tampon
    TaintBytePtr tbTempPtr;
    // position de l'octet fort du registre (octet 3 ou 7)
    UINT32 highestByte = (len >> 3) - 1;

    // echange little/big endian d'un registre 
    // en 32bits �change octet 0<->3 et 1<->2 => 2 tours de boucle (len / 16)
    // en 64 bits �change 0/7, 1/6, 2/5, 3/4  => 4 tours de boucle (len / 16)
    for (UINT32 regPart = 0; regPart < (len >> 4); ++regPart)
    {				
        // r�cup�ration du marquage de l'octet faible (nullptr si absent)
        tbTempPtr = pTmgrTls->getRegisterPartTaint(regIndex, regPart); 

        // r�cup�ration du marquage du registre de l'octet fort et affectation directe � l'octet faible
        if (pTmgrTls->isRegisterPartTainted(regIndex, highestByte - regPart))
        {
            pTmgrTls->updateTaintRegisterPart(regIndex, regPart, 
                pTmgrTls->getRegisterPartTaint(regIndex, highestByte - regPart));
        }
        else pTmgrTls->unTaintRegisterPart(regIndex, regPart);

        // affectation de la variable temporaire � l'octet fort
        if (tbTempPtr) 
        {
            pTmgrTls->updateTaintRegisterPart(regIndex, highestByte - regPart, tbTempPtr);
        }
        else pTmgrTls->unTaintRegisterPart(regIndex, 3 - regPart);
    }
} // sBSWAP
