/////////
// MOV //
/////////

// SIMULATE
template<UINT32 lengthInBits> void DATAXFER::sMOV_RR(THREADID tid, REG regSrc, REG regDest, ADDRINT insAddress) 
{
    // pas besoin de tester le marquage de la source : perte de temps
    // car isRegisterTainted va faire une boucle pour tester le marquage, 
    // et le marquage est �galement test� octet par octet dans la boucle for ci dessous
    
    // on pourrait faire directement un updateRegisterTaint mais
    // on traite octet par octet pour �viter le surmarquage, et en plus
    // cela ne n�cessite pas les valeurs des registres (gain en performances)

    REGINDEX regIndexDest = getRegIndex(regDest);	
    REGINDEX regIndexSrc  = getRegIndex(regSrc);
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    
    // si le registre est enti�rement marqu�, r�cup�rer l'objet et l'affecter
    // au registre de destination
    TAINT_OBJECT_PTR tPtr = pTmgrTls->getFullRegisterTaint<lengthInBits>(regSrc);
    if ( (bool) tPtr)  pTmgrTls->updateTaintRegister<lengthInBits>(regDest, tPtr);
    // sinon traitement octet par octet
    else
    {
        for (UINT32 regPart = 0 ; regPart < (lengthInBits >> 3) ; ++regPart) 
        {	
            if (pTmgrTls->isRegisterPartTainted(regIndexSrc, regPart))  // octet marqu� ? 
            {	
                _LOGTAINT(tid, insAddress, "movRR" + decstr(lengthInBits) + " octet " + decstr(regPart));
                pTmgrTls->updateTaintRegisterPart(regIndexDest, regPart, std::make_shared<TaintByte>(
                    X_ASSIGN, 
                    ObjectSource(pTmgrTls->getRegisterPartTaint(regIndexSrc, regPart))));
            }
            else pTmgrTls->unTaintRegisterPart(regIndexDest, regPart);
        }
    }
} // sMOV_RR

template<UINT32 lengthInBits> void DATAXFER::sMOV_RM(THREADID tid, REG regSrc, ADDRINT writeAddress, ADDRINT insAddress) 
{
    // pas besoin de tester le marquage de la source : perte de temps
    // car isRegisterTainted va faire une boucle pour tester le marquage, 
    // et le marquage est �galement test� octet par octet dans la boucle for ci dessous
    REGINDEX regIndex = getRegIndex(regSrc);
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    
    // si le registre est enti�rement marqu�, r�cup�rer l'objet et l'affecter
    // au registre de destination
    TAINT_OBJECT_PTR tPtr = pTmgrTls->getFullRegisterTaint<lengthInBits>(regSrc);
    if ( (bool) tPtr)  pTmgrGlobal->updateMemoryTaint<lengthInBits>(writeAddress, tPtr);
    // sinon traitement octet par octet
    else
    {
        for (UINT32 regPart = 0 ; regPart < (lengthInBits >> 3) ; ++regPart, ++writeAddress) 
        {	
            if (pTmgrTls->isRegisterPartTainted(regIndex, regPart))  // octet marqu� ? 
            {  
                _LOGTAINT(tid, insAddress, "movRM" + decstr(lengthInBits) + " octet " + decstr(regPart));	
                pTmgrGlobal->updateMemoryTaint<8>(writeAddress, std::make_shared<TaintByte>(
                    X_ASSIGN, 
                    ObjectSource(pTmgrTls->getRegisterPartTaint(regIndex, regPart))));
            }
            else pTmgrGlobal->unTaintMemory<8>(writeAddress);   
        }
    }
} // sMOV_RM

template<UINT32 lengthInBits> void DATAXFER::sMOV_MR(THREADID tid, ADDRINT readAddress, REG regDest, ADDRINT insAddress) 
{
    REGINDEX regIndex = getRegIndex(regDest);	
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    
    for (UINT32 regPart = 0 ; regPart < (lengthInBits >> 3) ; ++regPart, ++readAddress)
    {
        if (pTmgrGlobal->isMemoryTainted<8>(readAddress))  // octet marqu� ? 
        {	
            _LOGTAINT(tid, insAddress, "movMR" + decstr(lengthInBits) + " octet " + decstr(regPart));	
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
template<UINT32 lengthInBits> void DATAXFER::sXCHG_M(THREADID tid, REG reg, ADDRINT address, ADDRINT insAddress) 
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    
    TaintBytePtr tbTempPtr; // variable de mise en cache
    REGINDEX regIndex = getRegIndex(reg);

    for (UINT32 regPart = 0 ; regPart < (lengthInBits >> 3) ; ++regPart, ++address) 
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

template<UINT32 lengthInBits> void DATAXFER::sXCHG_R(THREADID tid, REG regSrc, REG regDest, ADDRINT insAddress) 
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    
    TaintBytePtr tbTempPtr; // variable de mise en cache
    REGINDEX regIndexSrc = getRegIndex(regSrc);
    REGINDEX regIndexDest = getRegIndex(regDest);

    for (UINT32 regPart = 0 ; regPart < (lengthInBits >> 3) ; ++regPart) 
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

template<UINT32 lengthInBitsSrc, UINT32 lengthInBitsDest> 
void DATAXFER::sMOVSX_RR(THREADID tid, REG regSrc, ADDRINT regSrcValue, REG regDest, ADDRINT insAddress)
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    
    if ( !pTmgrTls->isRegisterTainted<lengthInBitsSrc>(regSrc)) pTmgrTls->unTaintRegister<lengthInBitsDest>(regDest);
    else 
    {
        _LOGTAINT(tid, insAddress, "movsxRR" + decstr(lengthInBitsSrc) + "->" + decstr(lengthInBitsDest));
        // cr�ation d'un nouvel objet de taille = � la destination
        // avec comme source l'objet de taille = � la source
        // et affectation au registre de destination
        pTmgrTls->updateTaintRegister<lengthInBitsDest>(regDest, std::make_shared<TaintObject<lengthInBitsDest>>
            (X_SIGNEXTEND, 
            ObjectSource(pTmgrTls->getRegisterTaint<lengthInBitsSrc>(regSrc, regSrcValue))));
    }
} // sMOVSX_RR

template<UINT32 lengthInBitsSrc, UINT32 lengthInBitsDest>
void DATAXFER::sMOVSX_MR(THREADID tid, ADDRINT readAddress, REG regDest, ADDRINT insAddress) 
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    
    if ( !pTmgrGlobal->isMemoryTainted<lengthInBitsSrc>(readAddress)) pTmgrTls->unTaintRegister<lengthInBitsDest>(regDest);
    else 
    {
        _LOGTAINT(tid, insAddress, "movsxMR" + decstr(lengthInBitsSrc) + "->" + decstr(lengthInBitsDest));
        // cr�ation d'un nouvel objet de taille = � la destination
        // avec comme source l'objet de taille = � la source
        // et affectation au registre de destination
        pTmgrTls->updateTaintRegister<lengthInBitsDest>(regDest, std::make_shared<TaintObject<lengthInBitsDest>>
            (X_SIGNEXTEND, 
            ObjectSource(pTmgrGlobal->getMemoryTaint<lengthInBitsSrc>(readAddress))));
    }
} // sMOVSX_MR

///////////
// MOVZX //
///////////

// SIMULATE
template<UINT32 lengthInBitsSrc, UINT32 lengthInBitsDest>
void DATAXFER::sMOVZX_RR(THREADID tid, REG regSrc, REG regDest, ADDRINT insAddress) 
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    
    // quoi qu'il arrive, effacement du marquage de la destination
    // en effet partie basse sera marqu�e (ou non) par le registre source 
    // et la partie haute sera d�marqu�e (mise � z�ro)
    pTmgrTls->unTaintRegister<lengthInBitsDest>(regDest);

    // marquage partie basse
    if (pTmgrTls->isRegisterTainted<lengthInBitsSrc>(regSrc)) 
    { 
        _LOGTAINT(tid, insAddress, "movzxRR" + decstr(lengthInBitsSrc) + "->" + decstr(lengthInBitsDest));
        REGINDEX regIndexSrc  = getRegIndex(regSrc);
        REGINDEX regIndexDest = getRegIndex(regDest);

        // d�placement des lengthInBits>>3 octets de la source vers la destination
        for (UINT32 regPart = 0 ; regPart < (lengthInBitsSrc >> 3) ; ++regPart) 
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

template<UINT32 lengthInBitsDest> void DATAXFER::sMOVZX_8R(THREADID tid, REG regSrc, REG regDest, ADDRINT insAddress) 
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    
    // quoi qu'il arrive, effacement du marquage de la destination
    // en effet octet faible sera marqu� (ou non) et octets forts mis � z�ro
    pTmgrTls->unTaintRegister<lengthInBitsDest>(regDest);
  
    if (pTmgrTls->isRegisterTainted<8>(regSrc))
    {
        _LOGTAINT(tid, insAddress, "moxzxRR8->" + decstr(lengthInBitsDest));
        REGINDEX regIndex = getRegIndex(regDest);
        pTmgrTls->updateTaintRegisterPart(regIndex, 0, std::make_shared<TaintByte>(
            X_ASSIGN, 
            ObjectSource(pTmgrTls->getRegisterTaint(regSrc))));
    }
} // sMOVZX_8R

template<UINT32 lengthInBitsSrc, UINT32 lengthInBitsDest> 
void DATAXFER::sMOVZX_MR(THREADID tid, ADDRINT readAddress, REG regDest, ADDRINT insAddress) 
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    
    // quoi qu'il arrive, effacement du marquage de la destination
    // en effet partie basse sera marqu�e (ou non) par la m�moire source
    // et la partie haute sera d�marqu�e (mise � z�ro)
    pTmgrTls->unTaintRegister<lengthInBitsDest>(regDest);
    
    if ( pTmgrGlobal->isMemoryTainted<lengthInBitsSrc>(readAddress)) 
    { 
        _LOGTAINT(tid, insAddress, "movzxMR" + decstr(lengthInBitsSrc) + "->" + decstr(lengthInBitsDest));
    
        REGINDEX regIndex = getRegIndex(regDest);	
        for (UINT32 regPart = 0 ; regPart < (lengthInBitsSrc >> 3) ; ++regPart, ++readAddress) 
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

template<UINT32 lengthInBits> void DATAXFER::sBSWAP(THREADID tid, REG reg, ADDRINT insAddress) 
{  
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    
    REGINDEX regIndex = getRegIndex(reg);
    // variable tampon
    TaintBytePtr tbTempPtr;
    // position de l'octet fort du registre (octet 3 ou 7)
    UINT32 highestByte = (lengthInBits >> 3) - 1;

    // echange little/big endian d'un registre 
    // en 32bits �change octet 0<->3 et 1<->2 => 2 tours de boucle (lengthInBits / 16)
    // en 64 bits �change 0/7, 1/6, 2/5, 3/4  => 4 tours de boucle (lengthInBits / 16)
    for (UINT32 regPart = 0; regPart < (lengthInBits >> 4); ++regPart)
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
