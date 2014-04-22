/////////
// NEG //
/////////

// SIMULATE
template<UINT32 lengthInBits> 
void BINARY::sNEG_M(THREADID tid, ADDRINT writeAddress, ADDRINT insAddress) 
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    
    if ( !pTmgrGlobal->isMemoryTainted<lengthInBits>(writeAddress))  pTmgrTls->unTaintAllFlags(); 
    else 
    {
        _LOGTAINT(tid, insAddress, "negM" + decstr(lengthInBits));
        ObjectSource objSrc(pTmgrGlobal->getMemoryTaint<lengthInBits>(writeAddress));

        // cr�ation de l'objet r�sultat
        TAINT_OBJECT_PTR resultPtr = MK_TAINT_OBJECT_PTR(X_NEG, objSrc);

        // marquage flags et destination
        fTaintNEG(pTmgrTls, objSrc, resultPtr);
        pTmgrGlobal->updateMemoryTaint<lengthInBits>(writeAddress, resultPtr);
    }
} // sNEG_M

template<UINT32 lengthInBits> 
void BINARY::sNEG_R(THREADID tid, REG reg, ADDRINT regValue, ADDRINT insAddress) 
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    
    if ( !pTmgrTls->isRegisterTainted<lengthInBits>(reg)) pTmgrTls->unTaintAllFlags();
    else 
    {
        _LOGTAINT(tid, insAddress, "negR" + decstr(lengthInBits));
        ObjectSource objSrc(pTmgrTls->getRegisterTaint<lengthInBits>(reg, regValue));

        // cr�ation de l'objet r�sultat
        TAINT_OBJECT_PTR resultPtr = MK_TAINT_OBJECT_PTR(X_NEG, objSrc);

        // marquage flags et destination
        fTaintNEG(pTmgrTls, objSrc, resultPtr);
        pTmgrTls->updateTaintRegister<lengthInBits>(reg, resultPtr);
    }
} // sNEG_R

/////////
// INC //
/////////

// SIMULATE
template<UINT32 lengthInBits> 
void BINARY::sINC_M(THREADID tid, ADDRINT writeAddress, ADDRINT insAddress) 
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    
    if ( !pTmgrGlobal->isMemoryTainted<lengthInBits>(writeAddress)) fUnTaintINCDEC(pTmgrTls);
    else 
    {
        _LOGTAINT(tid, insAddress, "incM" + decstr(lengthInBits));
        ObjectSource objSrc(pTmgrGlobal->getMemoryTaint<lengthInBits>(writeAddress));
        
        // cr�ation de l'objet r�sultat
        TAINT_OBJECT_PTR resultPtr = MK_TAINT_OBJECT_PTR(X_INC, objSrc);

        // marquage flags et destination
        fTaintINC(pTmgrTls, objSrc, resultPtr);
        pTmgrGlobal->updateMemoryTaint<lengthInBits>(writeAddress, resultPtr);
    }
} // sINC_M

template<UINT32 lengthInBits> 
void BINARY::sINC_R(THREADID tid, REG reg, ADDRINT regValue, ADDRINT insAddress) 
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    
    if ( !pTmgrTls->isRegisterTainted<lengthInBits>(reg)) fUnTaintINCDEC(pTmgrTls);
    else 
    {
        _LOGTAINT(tid, insAddress, "incR" + decstr(lengthInBits));
        ObjectSource objSrc(pTmgrTls->getRegisterTaint<lengthInBits>(reg, regValue));
        
        // cr�ation de l'objet r�sultat
        TAINT_OBJECT_PTR resultPtr = MK_TAINT_OBJECT_PTR(X_INC, objSrc);

        // marquage flags et destination
        fTaintINC(pTmgrTls, objSrc, resultPtr);
        pTmgrTls->updateTaintRegister<lengthInBits>(reg, resultPtr);
    }
} // sINC_R

/////////
// DEC //
/////////

// SIMULATE
template<UINT32 lengthInBits> 
void BINARY::sDEC_M(THREADID tid, ADDRINT writeAddress, ADDRINT insAddress)
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    
    if ( !pTmgrGlobal->isMemoryTainted<lengthInBits>(writeAddress))  fUnTaintINCDEC(pTmgrTls);
    else 
    {
        _LOGTAINT(tid, insAddress, "decM" + decstr(lengthInBits));
        ObjectSource objSrc(pTmgrGlobal->getMemoryTaint<lengthInBits>(writeAddress));

        // cr�ation de l'objet r�sultat
        TAINT_OBJECT_PTR resultPtr = MK_TAINT_OBJECT_PTR(X_DEC, objSrc);

        // marquage flags et destination
        fTaintDEC(pTmgrTls, objSrc, resultPtr);
        pTmgrGlobal->updateMemoryTaint<lengthInBits>(writeAddress, resultPtr);
    }
} // sDEC_M

template<UINT32 lengthInBits> 
void BINARY::sDEC_R(THREADID tid, REG reg, ADDRINT regValue, ADDRINT insAddress) 
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    
    if ( !pTmgrTls->isRegisterTainted<lengthInBits>(reg))  fUnTaintINCDEC(pTmgrTls);
    else 
    {
        _LOGTAINT(tid, insAddress, "decR" + decstr(lengthInBits));
        ObjectSource objSrc(pTmgrTls->getRegisterTaint<lengthInBits>(reg, regValue));
        
        // cr�ation de l'objet r�sultat
        TAINT_OBJECT_PTR resultPtr = MK_TAINT_OBJECT_PTR(X_DEC, objSrc);

        // marquage flags et destination
        fTaintDEC(pTmgrTls, objSrc, resultPtr);
        pTmgrTls->updateTaintRegister<lengthInBits>(reg, resultPtr);
    }
} // sDEC_R

/////////
// ADC //
/////////

// SIMULATE
template<UINT32 lengthInBits> 
void BINARY::sADC_IR(THREADID tid, ADDRINT value, REG reg, ADDRINT regValue, 
                     ADDRINT flagsValue, ADDRINT insAddress) 
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);

    bool isCFTainted  = pTmgrTls->isCarryFlagTainted();
    bool isRegTainted = pTmgrTls->isRegisterTainted<lengthInBits>(reg);

    // rien de marqu� : juste un d�marquage des flags
    if (! (isCFTainted || isRegTainted))    pTmgrTls->unTaintAllFlags();  
    else
    {
        _LOGTAINT(tid, insAddress, "ADC_IR " + decstr(lengthInBits));

        // On va calculer deux sources : 'objSrcPlusDest' et 'objCarryFlag'
        // puis les additionner et marquer flags et destination

        // objets sources � calculer
        ObjectSource objSrcPlusDest, objCarryFlag;
        
        /*** traitement de 'objSrcPlusDest' : 2 cas possibles selon marquage reg ***/
        if (isRegTainted)  
        {  
            objSrcPlusDest = ObjectSource(MK_TAINT_OBJECT_PTR(X_ADD, 
                ObjectSource(pTmgrTls->getRegisterTaint<lengthInBits>(reg, regValue)),
                ObjectSource(lengthInBits, value)));
        }
        else objSrcPlusDest = ObjectSource(lengthInBits, value + regValue);

        /*** traitement de 'objCarryFlag' : 2 cas possibles (marqu� ou valeur) ***/
        if (isCFTainted)	
        {
            // CF doit �tre zero-extended � la longueur des sources
            TAINT_OBJECT_PTR zeroExtCFPtr = MK_TAINT_OBJECT_PTR(X_ZEROEXTEND,
                ObjectSource(pTmgrTls->getTaintCarryFlag()));
            objCarryFlag = ObjectSource(zeroExtCFPtr);
        }
        else objCarryFlag = ObjectSource(lengthInBits, flagsValue & 1); // CF est le LSB des flags

        /*** resultat = addition des deux sources ***/
        TAINT_OBJECT_PTR resultPtr = MK_TAINT_OBJECT_PTR(X_ADD, objSrcPlusDest, objCarryFlag);

        /*** marquage flags et destination ***/
        fTaintADD(pTmgrTls, objSrcPlusDest, objCarryFlag, resultPtr);	
        pTmgrTls->updateTaintRegister<lengthInBits>(reg, resultPtr);			
    }  
} // sADC_IR

template<UINT32 lengthInBits> 
void BINARY::sADC_IM(THREADID tid, ADDRINT value, ADDRINT writeAddress, 
                     ADDRINT flagsValue, ADDRINT insAddress)
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);

    bool isCFTainted  = pTmgrTls->isCarryFlagTainted();
    bool isMemTainted = pTmgrGlobal->isMemoryTainted<lengthInBits>(writeAddress);

    // rien de marqu� : juste un d�marquage des flags
    if (! (isCFTainted || isMemTainted))    pTmgrTls->unTaintAllFlags();  
    else
    {
        _LOGTAINT(tid, insAddress, "ADC_IM " + decstr(lengthInBits));

        // On va calculer deux sources : 'objSrcPlusDest' et 'objCarryFlag'
        // puis les additionner et marquer flags et destination

        // objets sources � calculer
        ObjectSource objSrcPlusDest, objCarryFlag;
        
        /*** traitement de 'objSrcPlusDest' : 2 cas possibles selon marquage mem ***/
        if (isMemTainted)  
        {  
            objSrcPlusDest = ObjectSource(MK_TAINT_OBJECT_PTR(X_ADD, 
                ObjectSource(pTmgrGlobal->getMemoryTaint<lengthInBits>(writeAddress)),
                ObjectSource(lengthInBits, value)));
        }
        else 
        {
            ADDRINT memValue = getMemoryValue<lengthInBits>(writeAddress);
            objSrcPlusDest = ObjectSource(lengthInBits, value + memValue);
        }

        /*** traitement de 'objCarryFlag' : 2 cas possibles (marqu� ou valeur) ***/
        if (isCFTainted)	
        {
            // CF doit �tre zero-extended � la longueur des sources
            TAINT_OBJECT_PTR zeroExtCFPtr = MK_TAINT_OBJECT_PTR(X_ZEROEXTEND,
                ObjectSource(pTmgrTls->getTaintCarryFlag()));
            objCarryFlag = ObjectSource(zeroExtCFPtr);
        }
        else objCarryFlag = ObjectSource(lengthInBits, flagsValue & 1); // CF est le LSB des flags

        /*** resultat = addition des deux sources ***/
        TAINT_OBJECT_PTR resultPtr = MK_TAINT_OBJECT_PTR(X_ADD, objSrcPlusDest, objCarryFlag);

        /*** marquage flags et destination ***/
        fTaintADD(pTmgrTls, objSrcPlusDest, objCarryFlag, resultPtr);	
        pTmgrGlobal->updateMemoryTaint<lengthInBits>(writeAddress, resultPtr);				
    }  
} // sADC_IM

template<UINT32 lengthInBits> 
void BINARY::sADC_MR(THREADID tid, ADDRINT readAddress, REG regSrcDest, 
                     ADDRINT regSrcDestValue, ADDRINT flagsValue, ADDRINT insAddress) 
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);

    bool isCFTainted  = pTmgrTls->isCarryFlagTainted();
    bool isRegTainted = pTmgrTls->isRegisterTainted<lengthInBits>(regSrcDest);
    bool isMemTainted = pTmgrGlobal->isMemoryTainted<lengthInBits>(readAddress);
    
    // rien de marqu� : juste un d�marquage des flags
    if (! (isCFTainted || isRegTainted || isMemTainted))
    {
        pTmgrTls->unTaintAllFlags();  
    }
    else
    {
        _LOGTAINT(tid, insAddress, "ADC_MR " + decstr(lengthInBits));

        // On va calculer deux sources : 'objSrcPlusDest' et 'objCarryFlag'
        // puis les additionner et marquer flags et destination

        // objets sources � calculer
        ObjectSource objSrcPlusDest, objCarryFlag;
        
        /*** traitement de 'objSrcPlusDest' : 4 cas possibles (M/M, M/V, V/M, V/V) ***/
        if (isRegTainted)  
        {  
            TAINT_OBJECT_PTR tMemRegPtr = MK_TAINT_OBJECT_PTR(X_ADD, 
                ObjectSource(pTmgrTls->getRegisterTaint<lengthInBits>(regSrcDest, regSrcDestValue)));
            
            if (isMemTainted) // Cas M/M
            {  
                tMemRegPtr->addSource(pTmgrGlobal->getMemoryTaint<lengthInBits>(readAddress));
            }
            else // cas M/V
            {
                ADDRINT memValue = getMemoryValue<lengthInBits>(readAddress);
                tMemRegPtr->addConstantAsASource<lengthInBits>(memValue);
            }
            // r�sultat reg + mem traduit sous forme de source
            objSrcPlusDest = ObjectSource(tMemRegPtr);
        }
        else if (isMemTainted) // cas V/M
        {
            objSrcPlusDest = ObjectSource(MK_TAINT_OBJECT_PTR(X_ADD, 
                ObjectSource(pTmgrGlobal->getMemoryTaint<lengthInBits>(readAddress)),
                ObjectSource(lengthInBits, regSrcDestValue)));
        }
        else // cas V/V
        {
            ADDRINT memValue = getMemoryValue<lengthInBits>(readAddress);
            objSrcPlusDest = ObjectSource(lengthInBits, memValue + regSrcDestValue);
        }

        /*** traitement de 'objCarryFlag' : 2 cas possibles (marqu� ou valeur) ***/
        if (isCFTainted)	
        {
            // CF doit �tre zero-extended � la longueur des sources
            TAINT_OBJECT_PTR zeroExtCFPtr = MK_TAINT_OBJECT_PTR(X_ZEROEXTEND,
                ObjectSource(pTmgrTls->getTaintCarryFlag()));
            objCarryFlag = ObjectSource(zeroExtCFPtr);
        }
        else objCarryFlag = ObjectSource(lengthInBits, flagsValue & 1); // CF est le LSB des flags

        /*** resultat = addition des deux sources ***/
        TAINT_OBJECT_PTR resultPtr = MK_TAINT_OBJECT_PTR(X_ADD, objSrcPlusDest, objCarryFlag);

        /*** marquage flags et destination ***/
        fTaintADD(pTmgrTls, objSrcPlusDest, objCarryFlag, resultPtr);	
        pTmgrTls->updateTaintRegister<lengthInBits>(regSrcDest, resultPtr);			
    }  
} // sADC_MR

template<UINT32 lengthInBits> 
void BINARY::sADC_RM(THREADID tid, REG regSrc, ADDRINT regSrcValue, ADDRINT writeAddress, 
                     ADDRINT flagsValue, ADDRINT insAddress)
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);

    bool isCFTainted  = pTmgrTls->isCarryFlagTainted();
    bool isRegTainted = pTmgrTls->isRegisterTainted<lengthInBits>(regSrc);
    bool isMemTainted = pTmgrGlobal->isMemoryTainted<lengthInBits>(writeAddress);  

    // rien de marqu� : juste un d�marquage des flags
    if (! (isCFTainted || isRegTainted || isMemTainted))
    {
        pTmgrTls->unTaintAllFlags();  
    }
    else
    {
        _LOGTAINT(tid, insAddress, "ADC_RM " + decstr(lengthInBits));

        // On va calculer deux sources : 'objSrcPlusDest' et 'objCarryFlag'
        // puis les additionner et marquer flags et destination

        // objets sources � calculer
        ObjectSource objSrcPlusDest, objCarryFlag;
        
        /*** traitement de 'objSrcPlusDest' : 4 cas possibles (M/M, M/V, V/M, V/V) ***/
        if (isRegTainted)  
        {  
            TAINT_OBJECT_PTR tMemRegPtr = MK_TAINT_OBJECT_PTR(X_ADD, 
                ObjectSource(pTmgrTls->getRegisterTaint<lengthInBits>(regSrc, regSrcValue)));
            
            if (isMemTainted) // Cas M/M
            {  
                tMemRegPtr->addSource(pTmgrGlobal->getMemoryTaint<lengthInBits>(writeAddress));
            }
            else // cas M/V
            {
                ADDRINT memValue = getMemoryValue<lengthInBits>(writeAddress);
                tMemRegPtr->addConstantAsASource<lengthInBits>(memValue);
            }
            // r�sultat reg + mem traduit sous forme de source
            objSrcPlusDest = ObjectSource(tMemRegPtr);
        }
        else if (isMemTainted) // cas V/M
        {
            objSrcPlusDest = ObjectSource(MK_TAINT_OBJECT_PTR(X_ADD, 
                ObjectSource(pTmgrGlobal->getMemoryTaint<lengthInBits>(writeAddress)),
                ObjectSource(lengthInBits, regSrcValue)));
        }
        else // cas V/V
        {
            ADDRINT memValue = getMemoryValue<lengthInBits>(writeAddress);
            objSrcPlusDest = ObjectSource(lengthInBits, memValue + regSrcValue);
        }

        /*** traitement de 'objCarryFlag' : 2 cas possibles (marqu� ou valeur) ***/
        if (isCFTainted)	
        {
            // CF doit �tre zero-extended � la longueur des sources
            TAINT_OBJECT_PTR zeroExtCFPtr = MK_TAINT_OBJECT_PTR(X_ZEROEXTEND,
                ObjectSource(pTmgrTls->getTaintCarryFlag()));
            objCarryFlag = ObjectSource(zeroExtCFPtr);
        }
        else objCarryFlag = ObjectSource(lengthInBits, flagsValue & 1); // CF est le LSB des flags

        /*** resultat = addition des deux sources ***/
        TAINT_OBJECT_PTR resultPtr = MK_TAINT_OBJECT_PTR(X_ADD, objSrcPlusDest, objCarryFlag);

        /*** marquage flags et destination ***/
        fTaintADD(pTmgrTls, objSrcPlusDest, objCarryFlag, resultPtr);	
        pTmgrGlobal->updateMemoryTaint<lengthInBits>(writeAddress, resultPtr);			
    }  
} // sADC_RM

template<UINT32 lengthInBits> 
void BINARY::sADC_RR(THREADID tid, REG regSrc, ADDRINT regSrcValue, REG regSrcDest, 
                     ADDRINT regSrcDestValue, ADDRINT flagsValue, ADDRINT insAddress) 
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);

    bool isCFTainted          = pTmgrTls->isCarryFlagTainted();
    bool isRegSrcTainted      = pTmgrTls->isRegisterTainted<lengthInBits>(regSrc);
    bool isRegSrcDestTainted  = pTmgrTls->isRegisterTainted<lengthInBits>(regSrcDest);
    
    // rien de marqu� : juste un d�marquage des flags
    if (! (isCFTainted || isRegSrcTainted || isRegSrcDestTainted))
    {
        pTmgrTls->unTaintAllFlags();  
    }
    else
    {
        _LOGTAINT(tid, insAddress, "ADC_RR " + decstr(lengthInBits));

        // On va calculer deux sources : 'objSrcPlusDest' et 'objCarryFlag'
        // puis les additionner et marquer flags et destination

        // objets sources � calculer
        ObjectSource objSrcPlusDest, objCarryFlag;
        
        /*** traitement de 'objSrcPlusDest' : 4 cas possibles (M/M, M/V, V/M, V/V) ***/
        if (isRegSrcTainted)  
        {  
            TAINT_OBJECT_PTR tRegRegPtr = MK_TAINT_OBJECT_PTR(X_ADD, 
                ObjectSource(pTmgrTls->getRegisterTaint<lengthInBits>(regSrc, regSrcValue)));
            
            if (isRegSrcDestTainted) // Cas M/M
            {  
                tRegRegPtr->addSource(
                    pTmgrTls->getRegisterTaint<lengthInBits>(regSrcDest, regSrcDestValue));
            }
            // cas M/V
            else  tRegRegPtr->addConstantAsASource<lengthInBits>(regSrcDestValue);
            // r�sultat reg + mem traduit sous forme de source
            objSrcPlusDest = ObjectSource(tRegRegPtr);
        }
        else if (isRegSrcDestTainted) // cas V/M
        {
            objSrcPlusDest = ObjectSource(MK_TAINT_OBJECT_PTR(X_ADD, 
                ObjectSource(pTmgrTls->getRegisterTaint<lengthInBits>(regSrcDest, regSrcDestValue)),
                ObjectSource(lengthInBits, regSrcValue)));
        }
        // cas V/V
        else objSrcPlusDest = ObjectSource(lengthInBits, regSrcDestValue + regSrcValue);

        /*** traitement de 'objCarryFlag' : 2 cas possibles (marqu� ou valeur) ***/
        if (isCFTainted)	
        {
            // CF doit �tre zero-extended � la longueur des sources
            TAINT_OBJECT_PTR zeroExtCFPtr = MK_TAINT_OBJECT_PTR(X_ZEROEXTEND,
                ObjectSource(pTmgrTls->getTaintCarryFlag()));
            objCarryFlag = ObjectSource(zeroExtCFPtr);
        }
        else objCarryFlag = ObjectSource(lengthInBits, flagsValue & 1); // CF est le LSB des flags

        /*** resultat = addition des deux sources ***/
        TAINT_OBJECT_PTR resultPtr = MK_TAINT_OBJECT_PTR(X_ADD, objSrcPlusDest, objCarryFlag);

        /*** marquage flags et destination ***/
        fTaintADD(pTmgrTls, objSrcPlusDest, objCarryFlag, resultPtr);	
        pTmgrTls->updateTaintRegister<lengthInBits>(regSrcDest, resultPtr);			
    }  
} // sADC_RR

/////////
// ADD //
/////////

// SIMULATE
template<UINT32 lengthInBits> 
void BINARY::sADD_IR(THREADID tid, ADDRINT value, REG reg, ADDRINT regValue, ADDRINT insAddress) 
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    
    if ( !pTmgrTls->isRegisterTainted<lengthInBits>(reg) )	pTmgrTls->unTaintAllFlags();
    else 
    {
        _LOGTAINT(tid, insAddress, "addIR" + decstr(lengthInBits));

        ObjectSource objSrcDest(pTmgrTls->getRegisterTaint<lengthInBits>(reg, regValue));
        ObjectSource objSrc(lengthInBits, value);		
    
        TAINT_OBJECT_PTR resultPtr = MK_TAINT_OBJECT_PTR(X_ADD, objSrcDest, objSrc);

        // marquage flags et destination
        fTaintADD(pTmgrTls, objSrcDest, objSrc, resultPtr);	
        pTmgrTls->updateTaintRegister<lengthInBits>(reg, resultPtr);			
    }
} // sADD_IR

template<UINT32 lengthInBits> 
void BINARY::sADD_IM(THREADID tid, ADDRINT value, ADDRINT writeAddress, ADDRINT insAddress)
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    
    if ( !pTmgrGlobal->isMemoryTainted<lengthInBits>(writeAddress)) pTmgrTls->unTaintAllFlags();
    else 
    {
        _LOGTAINT(tid, insAddress, "addIM" + decstr(lengthInBits));

        ObjectSource objSrcDest(pTmgrGlobal->getMemoryTaint<lengthInBits>(writeAddress));
        ObjectSource objSrc(lengthInBits, value);		
    
        TAINT_OBJECT_PTR resultPtr = MK_TAINT_OBJECT_PTR(X_ADD, objSrcDest, objSrc);

        // marquage flags et destination
        fTaintADD(pTmgrTls, objSrcDest, objSrc, resultPtr);	
        pTmgrGlobal->updateMemoryTaint<lengthInBits>(writeAddress, resultPtr);			
    }
} // sADD_IM

template<UINT32 lengthInBits> 
void BINARY::sADD_MR(THREADID tid, ADDRINT readAddress, REG regSrcDest, ADDRINT regSrcDestValue, ADDRINT insAddress) 
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    
    bool isSrcDestTainted = pTmgrTls->isRegisterTainted<lengthInBits>(regSrcDest);
    bool isSrcTainted =		pTmgrGlobal->isMemoryTainted<lengthInBits>(readAddress);

    if ( !(isSrcDestTainted || isSrcTainted))  pTmgrTls->unTaintAllFlags();
    else 
    {
        _LOGTAINT(tid, insAddress, "addMR" + decstr(lengthInBits));

        ObjectSource objSrcDest = (isSrcDestTainted) 
            ? ObjectSource(pTmgrTls->getRegisterTaint<lengthInBits>(regSrcDest, regSrcDestValue))
            : ObjectSource(lengthInBits, regSrcDestValue);

        ObjectSource objSrc = (isSrcTainted) 
            ? ObjectSource(pTmgrGlobal->getMemoryTaint<lengthInBits>(readAddress))
            : ObjectSource(lengthInBits, getMemoryValue<lengthInBits>(readAddress));

        TAINT_OBJECT_PTR resultPtr = MK_TAINT_OBJECT_PTR(X_ADD, objSrcDest, objSrc);

        // marquage flags et dest
        fTaintADD(pTmgrTls, objSrcDest, objSrc, resultPtr);	
        pTmgrTls->updateTaintRegister<lengthInBits>(regSrcDest, resultPtr);			
    }
} // sADD_MR

template<UINT32 lengthInBits> 
void BINARY::sADD_RM(THREADID tid, REG regSrc, ADDRINT regSrcValue, ADDRINT writeAddress, ADDRINT insAddress)
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    
    bool isSrcDestTainted = pTmgrGlobal->isMemoryTainted<lengthInBits>(writeAddress); 
    bool isSrcTainted =		pTmgrTls->isRegisterTainted<lengthInBits>(regSrc);

    if ( !(isSrcDestTainted || isSrcTainted))  pTmgrTls->unTaintAllFlags();
    else 
    {
        _LOGTAINT(tid, insAddress, "addRM" + decstr(lengthInBits));
           	
        ObjectSource objSrcDest = (isSrcDestTainted)
            ? ObjectSource(pTmgrGlobal->getMemoryTaint<lengthInBits>(writeAddress))
            : ObjectSource(lengthInBits, getMemoryValue<lengthInBits>(writeAddress));

        ObjectSource objSrc = (isSrcTainted) 
            ? ObjectSource(pTmgrTls->getRegisterTaint<lengthInBits>(regSrc, regSrcValue))
            : ObjectSource(lengthInBits, regSrcValue);

        TAINT_OBJECT_PTR resultPtr = MK_TAINT_OBJECT_PTR(X_ADD, objSrcDest, objSrc);

        // marquage flags et destination
        fTaintADD(pTmgrTls, objSrcDest, objSrc, resultPtr);
        pTmgrGlobal->updateMemoryTaint<lengthInBits>(writeAddress, resultPtr);			
    }
} // sADD_RM

template<UINT32 lengthInBits> 
void BINARY::sADD_RR(THREADID tid, REG regSrc, ADDRINT regSrcValue, REG regSrcDest, ADDRINT regSrcDestValue, ADDRINT insAddress) 
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    
    bool isSrcDestTainted = pTmgrTls->isRegisterTainted<lengthInBits>(regSrcDest);
    bool isSrcTainted =		pTmgrTls->isRegisterTainted<lengthInBits>(regSrc);

    if ( !(isSrcDestTainted || isSrcTainted))  pTmgrTls->unTaintAllFlags();
    else 
    {
        _LOGTAINT(tid, insAddress, "addRR" + decstr(lengthInBits));

        ObjectSource objSrcDest = (isSrcDestTainted) 
                ? ObjectSource(pTmgrTls->getRegisterTaint<lengthInBits>(regSrcDest, regSrcDestValue))
                : ObjectSource(lengthInBits, regSrcDestValue);

        ObjectSource objSrc = (isSrcTainted)
                ? ObjectSource(pTmgrTls->getRegisterTaint<lengthInBits>(regSrc, regSrcValue))
                : ObjectSource(lengthInBits, regSrcValue);
    
        TAINT_OBJECT_PTR resultPtr = MK_TAINT_OBJECT_PTR(X_ADD, objSrcDest, objSrc);

        // marquage flags et destination
        fTaintADD(pTmgrTls, objSrcDest, objSrc, resultPtr);   
        pTmgrTls->updateTaintRegister<lengthInBits>(regSrcDest, resultPtr);			
    }
} // sADD_RR

/////////
// SBB //
/////////

// SIMULATE
template<UINT32 lengthInBits> 
void BINARY::sSBB_IR(THREADID tid, ADDRINT value, REG reg, ADDRINT regValue, 
                     ADDRINT flagsValue, ADDRINT insAddress) 
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);

    bool isCFTainted  = pTmgrTls->isCarryFlagTainted();
    bool isRegTainted = pTmgrTls->isRegisterTainted<lengthInBits>(reg);

    // rien de marqu� : juste un d�marquage des flags
    if (! (isCFTainted || isRegTainted))    pTmgrTls->unTaintAllFlags();  
    else
    {
        _LOGTAINT(tid, insAddress, "SBB_IR " + decstr(lengthInBits));

        // On va calculer deux sources : 'objSrcCF' et 'objDest'
        // puis calculer (objDest - ObjSrcCF) et marquer flags et destination

        /*** traitement de 'objSrcCarryFlag' : 2 cas possibles selon marquage CF ***/
        ObjectSource objSrcPlusCarryFlag;
        if (isCFTainted)  
        {  
            // CF doit �tre zero-extended � la longueur des sources
            TAINT_OBJECT_PTR zeroExtCFPtr = MK_TAINT_OBJECT_PTR(X_ZEROEXTEND,
                ObjectSource(pTmgrTls->getTaintCarryFlag()));

            objSrcPlusCarryFlag = ObjectSource(MK_TAINT_OBJECT_PTR(X_ADD, 
                ObjectSource(zeroExtCFPtr),
                ObjectSource(lengthInBits, value)));
        }
        else objSrcPlusCarryFlag = ObjectSource(lengthInBits, value + (flagsValue & 1));

        /*** traitement de 'objDest' : 2 cas possibles (marqu� ou valeur) ***/
        ObjectSource objDest = (isRegTainted)
            ? ObjectSource(pTmgrTls->getRegisterTaint<lengthInBits>(reg, regValue))
            : ObjectSource(lengthInBits, regValue);

        /*** resultat = soustraction des deux sources ***/
        TAINT_OBJECT_PTR resultPtr = MK_TAINT_OBJECT_PTR(X_SUB, objDest, objSrcPlusCarryFlag);

        /*** marquage flags et destination ***/
        fTaintSUB(pTmgrTls, objDest, objSrcPlusCarryFlag, resultPtr);	
        pTmgrTls->updateTaintRegister<lengthInBits>(reg, resultPtr);			
    }  
} // sSBB_IR

template<UINT32 lengthInBits> 
void BINARY::sSBB_IM(THREADID tid, ADDRINT value, ADDRINT writeAddress, 
                     ADDRINT flagsValue, ADDRINT insAddress)
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);

    bool isCFTainted  = pTmgrTls->isCarryFlagTainted();
    bool isMemTainted = pTmgrGlobal->isMemoryTainted<lengthInBits>(writeAddress);

    // rien de marqu� : juste un d�marquage des flags
    if (! (isCFTainted || isMemTainted))    pTmgrTls->unTaintAllFlags();  
    else
    {
        _LOGTAINT(tid, insAddress, "SBB_IM " + decstr(lengthInBits));

        // On va calculer deux sources : 'objSrcCF' et 'objDest'
        // puis calculer (objDest - ObjSrcCF) et marquer flags et destination

        /*** traitement de 'objSrcCarryFlag' : 2 cas possibles selon marquage CF ***/
        ObjectSource objSrcPlusCarryFlag;
        if (isCFTainted)  
        {  
            // CF doit �tre zero-extended � la longueur des sources
            TAINT_OBJECT_PTR zeroExtCFPtr = MK_TAINT_OBJECT_PTR(X_ZEROEXTEND,
                ObjectSource(pTmgrTls->getTaintCarryFlag()));

            objSrcPlusCarryFlag = ObjectSource(MK_TAINT_OBJECT_PTR(X_ADD, 
                ObjectSource(zeroExtCFPtr),
                ObjectSource(lengthInBits, value)));
        }
        else objSrcPlusCarryFlag = ObjectSource(lengthInBits, value + (flagsValue & 1));

        /*** traitement de 'objDest' : 2 cas possibles (marqu� ou valeur) ***/
        ObjectSource objDest = (isMemTainted)
            ? ObjectSource(pTmgrGlobal->getMemoryTaint<lengthInBits>(writeAddress))
            : ObjectSource(lengthInBits, getMemoryValue<lengthInBits>(writeAddress));

        /*** resultat = soustraction des deux sources ***/
        TAINT_OBJECT_PTR resultPtr = MK_TAINT_OBJECT_PTR(X_SUB, objDest, objSrcPlusCarryFlag);

        /*** marquage flags et destination ***/
        fTaintSUB(pTmgrTls, objDest, objSrcPlusCarryFlag, resultPtr);	
        pTmgrGlobal->updateMemoryTaint<lengthInBits>(writeAddress, resultPtr);			
    }    
} // sSBB_IM

template<UINT32 lengthInBits> 
void BINARY::sSBB_MR(THREADID tid, ADDRINT readAddress, REG regSrcDest, 
                     ADDRINT regSrcDestValue, ADDRINT flagsValue, ADDRINT insAddress) 
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);

    bool isCFTainted  = pTmgrTls->isCarryFlagTainted();
    bool isRegTainted = pTmgrTls->isRegisterTainted<lengthInBits>(regSrcDest);
    bool isMemTainted = pTmgrGlobal->isMemoryTainted<lengthInBits>(readAddress);
    
    // rien de marqu� : juste un d�marquage des flags
    if (! (isCFTainted || isRegTainted || isMemTainted))
    {
        pTmgrTls->unTaintAllFlags();  
    }
    else
    {
        _LOGTAINT(tid, insAddress, "SBB_MR " + decstr(lengthInBits));

        // On va calculer deux sources : 'objSrcCF' et 'objDest'
        // puis calculer (objDest - ObjSrcCF) et marquer flags et destination
        
        /*** traitement de 'objSrcCF' : 4 cas possibles (M/M, M/V, V/M, V/V) ***/
        ObjectSource objSrcPlusCarryFlag;
        if (isCFTainted)  
        {  
            // CF doit �tre zero-extended � la longueur des sources
            TAINT_OBJECT_PTR zeroExtCFPtr = MK_TAINT_OBJECT_PTR(X_ZEROEXTEND,
                ObjectSource(pTmgrTls->getTaintCarryFlag()));

            TAINT_OBJECT_PTR tCFMemPtr = MK_TAINT_OBJECT_PTR(X_ADD, ObjectSource(zeroExtCFPtr));
            
            if (isMemTainted) // Cas M/M
            {  
                tCFMemPtr->addSource(pTmgrGlobal->getMemoryTaint<lengthInBits>(readAddress));
            }
            else // cas M/V
            {
                ADDRINT memValue = getMemoryValue<lengthInBits>(readAddress);
                tCFMemPtr->addConstantAsASource<lengthInBits>(memValue);
            }
            // r�sultat CF + mem traduit sous forme de source
            objSrcPlusCarryFlag = ObjectSource(tCFMemPtr);
        }
        else if (isMemTainted) // cas V/M
        {
            objSrcPlusCarryFlag = ObjectSource(MK_TAINT_OBJECT_PTR(X_ADD, 
                ObjectSource(pTmgrGlobal->getMemoryTaint<lengthInBits>(readAddress)),
                ObjectSource(lengthInBits, flagsValue & 1)));
        }
        else // cas V/V
        {
            ADDRINT memValue = getMemoryValue<lengthInBits>(readAddress);
            objSrcPlusCarryFlag = ObjectSource(lengthInBits, memValue + (flagsValue & 1));
        }

        /*** traitement de 'objDest' : 2 cas possibles (marqu� ou valeur) ***/
        ObjectSource objDest = (isRegTainted) 
            ? ObjectSource(pTmgrTls->getRegisterTaint<lengthInBits>(regSrcDest, regSrcDestValue))
            : ObjectSource(lengthInBits, regSrcDestValue);
        
        /*** resultat = soustraction des deux sources ***/
        TAINT_OBJECT_PTR resultPtr = MK_TAINT_OBJECT_PTR(X_SUB, objDest, objSrcPlusCarryFlag);

        /*** marquage flags et destination ***/
        fTaintSUB(pTmgrTls, objDest, objSrcPlusCarryFlag, resultPtr);	
        pTmgrTls->updateTaintRegister<lengthInBits>(regSrcDest, resultPtr);			
    }  
} // sSBB_MR

template<UINT32 lengthInBits> 
void BINARY::sSBB_RM(THREADID tid, REG regSrc, ADDRINT regSrcValue, ADDRINT writeAddress, 
                     ADDRINT flagsValue, ADDRINT insAddress)
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);

    bool isCFTainted  = pTmgrTls->isCarryFlagTainted();
    bool isRegTainted = pTmgrTls->isRegisterTainted<lengthInBits>(regSrc);
    bool isMemTainted = pTmgrGlobal->isMemoryTainted<lengthInBits>(writeAddress);  

    // rien de marqu� : juste un d�marquage des flags
    if (! (isCFTainted || isRegTainted || isMemTainted))
    {
        pTmgrTls->unTaintAllFlags();  
    }
    else
    {
        _LOGTAINT(tid, insAddress, "SBB_RM " + decstr(lengthInBits));

        // On va calculer deux sources : 'objSrcCF' et 'objDest'
        // puis calculer (objDest - ObjSrcCF) et marquer flags et destination
        
        /*** traitement de 'objSrcCF' : 4 cas possibles (M/M, M/V, V/M, V/V) ***/
        ObjectSource objSrcPlusCarryFlag;
        if (isCFTainted)  
        {  
            // CF doit �tre zero-extended � la longueur des sources
            TAINT_OBJECT_PTR zeroExtCFPtr = MK_TAINT_OBJECT_PTR(X_ZEROEXTEND,
                ObjectSource(pTmgrTls->getTaintCarryFlag()));

            TAINT_OBJECT_PTR tCFRegPtr = MK_TAINT_OBJECT_PTR(X_ADD, ObjectSource(zeroExtCFPtr));
            
            if (isRegTainted) // Cas M/M
            {  
                tCFRegPtr->addSource(pTmgrTls->getRegisterTaint<lengthInBits>(regSrc, regSrcValue));
            }
            // cas M/V
            else tCFRegPtr->addConstantAsASource<lengthInBits>(regSrcValue);

            // r�sultat CF + reg traduit sous forme de source
            objSrcPlusCarryFlag = ObjectSource(tCFRegPtr);
        }
        else if (isRegTainted) // cas V/M
        {
            objSrcPlusCarryFlag = ObjectSource(MK_TAINT_OBJECT_PTR(X_ADD, 
                ObjectSource(pTmgrTls->getRegisterTaint<lengthInBits>(regSrc, regSrcValue)),
                ObjectSource(lengthInBits, flagsValue & 1)));
        }
        // cas V/V
        else objSrcPlusCarryFlag = ObjectSource(lengthInBits, regSrcValue + (flagsValue & 1));

        /*** traitement de 'objDest' : 2 cas possibles (marqu� ou valeur) ***/
        ObjectSource objDest = (isMemTainted) 
            ? ObjectSource(pTmgrGlobal->getMemoryTaint<lengthInBits>(writeAddress))
            : ObjectSource(lengthInBits, getMemoryValue<lengthInBits>(writeAddress));
        
        /*** resultat = soustraction des deux sources ***/
        TAINT_OBJECT_PTR resultPtr = MK_TAINT_OBJECT_PTR(X_SUB, objDest, objSrcPlusCarryFlag);

        /*** marquage flags et destination ***/
        fTaintSUB(pTmgrTls, objDest, objSrcPlusCarryFlag, resultPtr);	
        pTmgrGlobal->updateMemoryTaint<lengthInBits>(writeAddress, resultPtr);				
    }  
} // sSBB_RM

template<UINT32 lengthInBits> 
void BINARY::sSBB_RR(THREADID tid, REG regSrc, ADDRINT regSrcValue, REG regSrcDest, 
                     ADDRINT regSrcDestValue, ADDRINT flagsValue, ADDRINT insAddress) 
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);

    bool isCFTainted          = pTmgrTls->isCarryFlagTainted();
    bool isRegSrcTainted      = pTmgrTls->isRegisterTainted<lengthInBits>(regSrc);
    bool isRegSrcDestTainted  = pTmgrTls->isRegisterTainted<lengthInBits>(regSrcDest);
    
    // rien de marqu� : juste un d�marquage des flags
    if (! (isCFTainted || isRegSrcTainted || isRegSrcDestTainted))
    {
        pTmgrTls->unTaintAllFlags();  
    }
    else
    {
        _LOGTAINT(tid, insAddress, "SBB_RR " + decstr(lengthInBits));

        // On va calculer deux sources : 'objSrcCF' et 'objDest'
        // puis calculer (objDest - ObjSrcCF) et marquer flags et destination
        
        /*** traitement de 'objSrcCF' : 4 cas possibles (M/M, M/V, V/M, V/V) ***/
        ObjectSource objSrcPlusCarryFlag;
        if (isCFTainted)  
        {  
            // CF doit �tre zero-extended � la longueur des sources
            TAINT_OBJECT_PTR zeroExtCFPtr = MK_TAINT_OBJECT_PTR(X_ZEROEXTEND,
                ObjectSource(pTmgrTls->getTaintCarryFlag()));

            TAINT_OBJECT_PTR tCFRegSrcPtr = MK_TAINT_OBJECT_PTR(X_ADD, ObjectSource(zeroExtCFPtr));
            
            if (isRegSrcTainted) // Cas M/M
            {  
                tCFRegSrcPtr->addSource(pTmgrTls->getRegisterTaint<lengthInBits>(regSrc, regSrcValue));
            }
            // cas M/V
            else tCFRegSrcPtr->addConstantAsASource<lengthInBits>(regSrcValue);

            // r�sultat CF + reg traduit sous forme de source
            objSrcPlusCarryFlag = ObjectSource(tCFRegSrcPtr);
        }
        else if (isRegSrcTainted) // cas V/M
        {
            objSrcPlusCarryFlag = ObjectSource(MK_TAINT_OBJECT_PTR(X_ADD, 
                ObjectSource(pTmgrTls->getRegisterTaint<lengthInBits>(regSrc, regSrcValue)),
                ObjectSource(lengthInBits, flagsValue & 1)));
        }
        // cas V/V
        else objSrcPlusCarryFlag = ObjectSource(lengthInBits, regSrcValue + (flagsValue & 1));

        /*** traitement de 'objDest' : 2 cas possibles (marqu� ou valeur) ***/
        ObjectSource objDest = (isRegSrcDestTainted) 
            ? ObjectSource(pTmgrTls->getRegisterTaint<lengthInBits>(regSrcDest, regSrcDestValue))
            : ObjectSource(lengthInBits, regSrcDestValue);
        
        /*** resultat = soustraction des deux sources ***/
        TAINT_OBJECT_PTR resultPtr = MK_TAINT_OBJECT_PTR(X_SUB, objDest, objSrcPlusCarryFlag);

        /*** marquage flags et destination ***/
        fTaintSUB(pTmgrTls, objDest, objSrcPlusCarryFlag, resultPtr);	
        pTmgrTls->updateTaintRegister<lengthInBits>(regSrcDest, resultPtr);				
    }  
} // sSBB_RR

/////////
// SUB //
/////////

// SIMULATE
template<UINT32 lengthInBits> 
void BINARY::sSUB_IR(THREADID tid, ADDRINT value, REG reg, ADDRINT regValue, ADDRINT insAddress) 
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    
    if ( !pTmgrTls->isRegisterTainted<lengthInBits>(reg) )	pTmgrTls->unTaintAllFlags();
    else 
    {
        _LOGTAINT(tid, insAddress, "subIR" + decstr(lengthInBits));

        ObjectSource objSrcDest(pTmgrTls->getRegisterTaint<lengthInBits>(reg, regValue));
        ObjectSource objSrc(lengthInBits, value);		
    
        TAINT_OBJECT_PTR resultPtr = MK_TAINT_OBJECT_PTR(
            X_SUB,
            objSrcDest,
            objSrc);

        // marquage flags et destination
        fTaintSUB(pTmgrTls, objSrcDest, objSrc, resultPtr);	
        pTmgrTls->updateTaintRegister<lengthInBits>(reg, resultPtr);			
    }
} // sSUB_IR

template<UINT32 lengthInBits> 
void BINARY::sSUB_IM(THREADID tid, ADDRINT value, ADDRINT writeAddress, ADDRINT insAddress)
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    
    if ( !pTmgrGlobal->isMemoryTainted<lengthInBits>(writeAddress)) pTmgrTls->unTaintAllFlags();
    else 
    {
        _LOGTAINT(tid, insAddress, "subIM" + decstr(lengthInBits));

        ObjectSource objSrcDest(pTmgrGlobal->getMemoryTaint<lengthInBits>(writeAddress));
        ObjectSource objSrc(lengthInBits, value);		
    
        TAINT_OBJECT_PTR resultPtr = MK_TAINT_OBJECT_PTR(
            X_SUB,
            objSrcDest,
            objSrc);

        // marquage flags et destination
        fTaintSUB(pTmgrTls, objSrcDest, objSrc, resultPtr);	
        pTmgrGlobal->updateMemoryTaint<lengthInBits>(writeAddress, resultPtr);			
    }
} // sSUB_IM

template<UINT32 lengthInBits> 
void BINARY::sSUB_MR(THREADID tid, ADDRINT readAddress, REG regSrcDest, 
                     ADDRINT regSrcDestValue, ADDRINT insAddress) 
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    
    bool isSrcDestTainted = pTmgrTls->isRegisterTainted<lengthInBits>(regSrcDest);
    bool isSrcTainted =		pTmgrGlobal->isMemoryTainted<lengthInBits>(readAddress);

    if ( !(isSrcDestTainted || isSrcTainted))  pTmgrTls->unTaintAllFlags();
    else 
    {
        _LOGTAINT(tid, insAddress, "subMR" + decstr(lengthInBits));

        ObjectSource objSrcDest = (isSrcDestTainted) 
            ? ObjectSource(pTmgrTls->getRegisterTaint<lengthInBits>(regSrcDest, regSrcDestValue))
            : ObjectSource(lengthInBits, regSrcDestValue);

        ObjectSource objSrc = (isSrcTainted) 
            ? ObjectSource(pTmgrGlobal->getMemoryTaint<lengthInBits>(readAddress))
            : ObjectSource(lengthInBits, getMemoryValue<lengthInBits>(readAddress));

        TAINT_OBJECT_PTR resultPtr = MK_TAINT_OBJECT_PTR(
            X_SUB,
            objSrcDest,
            objSrc);

        // marquage flags et dest
        fTaintSUB(pTmgrTls, objSrcDest, objSrc, resultPtr);	
        pTmgrTls->updateTaintRegister<lengthInBits>(regSrcDest, resultPtr);			
    }
} // sSUB_MR

template<UINT32 lengthInBits> 
void BINARY::sSUB_RM(THREADID tid, REG regSrc, ADDRINT regSrcValue, 
                     ADDRINT writeAddress, ADDRINT insAddress)
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    
    bool isSrcDestTainted = pTmgrGlobal->isMemoryTainted<lengthInBits>(writeAddress); 
    bool isSrcTainted =		pTmgrTls->isRegisterTainted<lengthInBits>(regSrc);

    if ( !(isSrcDestTainted || isSrcTainted))  pTmgrTls->unTaintAllFlags();
    else 
    {
        _LOGTAINT(tid, insAddress, "subRM" + decstr(lengthInBits));
           	
        ObjectSource objSrcDest = (isSrcDestTainted)
            ? ObjectSource(pTmgrGlobal->getMemoryTaint<lengthInBits>(writeAddress))
            : ObjectSource(lengthInBits, getMemoryValue<lengthInBits>(writeAddress));

        ObjectSource objSrc = (isSrcTainted) 
            ? ObjectSource(pTmgrTls->getRegisterTaint<lengthInBits>(regSrc, regSrcValue))
            : ObjectSource(lengthInBits, regSrcValue);

        TAINT_OBJECT_PTR resultPtr = MK_TAINT_OBJECT_PTR(
            X_SUB,
            objSrcDest,
            objSrc);

        // marquage flags et destination
        fTaintSUB(pTmgrTls, objSrcDest, objSrc, resultPtr);
        pTmgrGlobal->updateMemoryTaint<lengthInBits>(writeAddress, resultPtr);			
    }
} // sSUB_RM

template<UINT32 lengthInBits> 
void BINARY::sSUB_RR(THREADID tid, REG regSrc, ADDRINT regSrcValue, REG regSrcDest, 
                     ADDRINT regSrcDestValue, ADDRINT insAddress) 
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    
    bool isSrcDestTainted = pTmgrTls->isRegisterTainted<lengthInBits>(regSrcDest);
    bool isSrcTainted =		pTmgrTls->isRegisterTainted<lengthInBits>(regSrc);

    if ( !(isSrcDestTainted || isSrcTainted))  pTmgrTls->unTaintAllFlags();
    else 
    {
        _LOGTAINT(tid, insAddress, "subRR" + decstr(lengthInBits));

        ObjectSource objSrcDest = (isSrcDestTainted) 
                ? ObjectSource(pTmgrTls->getRegisterTaint<lengthInBits>(regSrcDest, regSrcDestValue))
                : ObjectSource(lengthInBits, regSrcDestValue);

        ObjectSource objSrc = (isSrcTainted)
                ? ObjectSource(pTmgrTls->getRegisterTaint<lengthInBits>(regSrc, regSrcValue))
                : ObjectSource(lengthInBits, regSrcValue);
    
        TAINT_OBJECT_PTR resultPtr = MK_TAINT_OBJECT_PTR(
            X_SUB,
            objSrcDest,
            objSrc);

        // marquage flags et destination
        fTaintSUB(pTmgrTls, objSrcDest, objSrc, resultPtr);   
        pTmgrTls->updateTaintRegister<lengthInBits>(regSrcDest, resultPtr);			
    }
} // sSUB_RR

/////////
// CMP //
/////////

// SIMULATE
template<UINT32 lengthInBits> 
void BINARY::sCMP_IR(THREADID tid, ADDRINT value, REG reg, ADDRINT regValue, ADDRINT insAddress) 
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    
    if ( !pTmgrTls->isRegisterTainted<lengthInBits>(reg) )	pTmgrTls->unTaintAllFlags();
    else 
    {
        _LOGTAINT(tid, insAddress, "cmpIR" + decstr(lengthInBits));

        // marquage flags
        ObjectSource objReg(pTmgrTls->getRegisterTaint<lengthInBits>(reg, regValue));
        ObjectSource objVal(lengthInBits, value);
        fTaintCMP<lengthInBits>(pTmgrTls, objReg, objVal);			
    }
} // sCMP_IR

template<UINT32 lengthInBits> 
void BINARY::sCMP_IM(THREADID tid, ADDRINT value, ADDRINT readAddress, ADDRINT insAddress)
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    
    if ( !pTmgrGlobal->isMemoryTainted<lengthInBits>(readAddress)) pTmgrTls->unTaintAllFlags();
    else 
    {
        _LOGTAINT(tid, insAddress, "cmpIM" + decstr(lengthInBits));

        // marquage flags
        ObjectSource objMem(pTmgrGlobal->getMemoryTaint<lengthInBits>(readAddress));
        ObjectSource objVal(lengthInBits, value);
        fTaintCMP<lengthInBits>(pTmgrTls, objMem , objVal);					
    }
} // sCMP_IM

template<UINT32 lengthInBits> 
void BINARY::sCMP_MR(THREADID tid, ADDRINT readAddress, REG regSrcDest, 
                     ADDRINT regSrcDestValue, ADDRINT insAddress) 
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    
    bool isSrcDestTainted = pTmgrTls->isRegisterTainted<lengthInBits>(regSrcDest);
    bool isSrcTainted =		pTmgrGlobal->isMemoryTainted<lengthInBits>(readAddress);

    if ( !(isSrcDestTainted || isSrcTainted))  pTmgrTls->unTaintAllFlags();
    else 
    {
        _LOGTAINT(tid, insAddress, "cmpMR" + decstr(lengthInBits));

        ObjectSource objSrcDest = (isSrcDestTainted) 
            ? ObjectSource(pTmgrTls->getRegisterTaint<lengthInBits>(regSrcDest, regSrcDestValue))
            : ObjectSource(lengthInBits, regSrcDestValue);

        ObjectSource objSrc = (isSrcTainted) 
            ? ObjectSource(pTmgrGlobal->getMemoryTaint<lengthInBits>(readAddress))
            : ObjectSource(lengthInBits, getMemoryValue<lengthInBits>(readAddress));

        // marquage flags
        fTaintCMP<lengthInBits>(pTmgrTls, objSrcDest, objSrc);	
    }
} // sCMP_MR

template<UINT32 lengthInBits> 
void BINARY::sCMP_RM(THREADID tid, REG regSrc, ADDRINT regSrcValue, 
                     ADDRINT readAddress, ADDRINT insAddress)
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    
    bool isSrcDestTainted = pTmgrGlobal->isMemoryTainted<lengthInBits>(readAddress); 
    bool isSrcTainted =		pTmgrTls->isRegisterTainted<lengthInBits>(regSrc);

    if ( !(isSrcDestTainted || isSrcTainted))  pTmgrTls->unTaintAllFlags();
    else 
    {
        _LOGTAINT(tid, insAddress, "cmpRM" + decstr(lengthInBits));
           	
        ObjectSource objSrcDest = (isSrcDestTainted)
            ? ObjectSource(pTmgrGlobal->getMemoryTaint<lengthInBits>(readAddress))
            : ObjectSource(lengthInBits, getMemoryValue<lengthInBits>(readAddress));

        ObjectSource objSrc = (isSrcTainted) 
            ? ObjectSource(pTmgrTls->getRegisterTaint<lengthInBits>(regSrc, regSrcValue))
            : ObjectSource(lengthInBits, regSrcValue);

        // marquage flags
        fTaintCMP<lengthInBits>(pTmgrTls, objSrcDest, objSrc);	
    }
} // sCMP_RM

template<UINT32 lengthInBits> 
void BINARY::sCMP_RR(THREADID tid, REG regSrc, ADDRINT regSrcValue, REG regSrcDest, 
                     ADDRINT regSrcDestValue, ADDRINT insAddress) 
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    
    bool isSrcDestTainted = pTmgrTls->isRegisterTainted<lengthInBits>(regSrcDest);
    bool isSrcTainted =		pTmgrTls->isRegisterTainted<lengthInBits>(regSrc);

    if ( !(isSrcDestTainted || isSrcTainted))  pTmgrTls->unTaintAllFlags();
    else 
    {
        _LOGTAINT(tid, insAddress, "cmpRR" + decstr(lengthInBits));

        ObjectSource objSrcDest = (isSrcDestTainted) 
                ? ObjectSource(pTmgrTls->getRegisterTaint<lengthInBits>(regSrcDest, regSrcDestValue))
                : ObjectSource(lengthInBits, regSrcDestValue);

        ObjectSource objSrc = (isSrcTainted)
                ? ObjectSource(pTmgrTls->getRegisterTaint<lengthInBits>(regSrc, regSrcValue))
                : ObjectSource(lengthInBits, regSrcValue);
    
        // marquage flags
        fTaintCMP<lengthInBits>(pTmgrTls, objSrcDest, objSrc);   	
    }
} // sCMP_RR

// FLAGS
template<UINT32 lengthInBits> 
void BINARY::fTaintCMP(TaintManager_Thread *pTmgrTls, const ObjectSource &objSrcDest, 
                       const ObjectSource &objSrc) 
{
    ObjectSource objResult(MK_TAINT_OBJECT_PTR(X_SUB, objSrcDest, objSrc));
    
    // CMP : O/S/Z/A/P/C
    pTmgrTls->updateTaintCarryFlag (std::make_shared<TaintBit>(F_CARRY_SUB, objSrcDest, objSrc));
    pTmgrTls->updateTaintParityFlag(std::make_shared<TaintBit>(F_PARITY, objResult));
    pTmgrTls->updateTaintAuxiliaryFlag(std::make_shared<TaintBit>(
        F_AUXILIARY_SUB, objSrcDest, objSrc));
    pTmgrTls->updateTaintZeroFlag(std::make_shared<TaintBit>(F_ARE_EQUAL, objSrcDest, objSrc));
    pTmgrTls->updateTaintSignFlag(std::make_shared<TaintBit>(F_MSB, objResult));
    pTmgrTls->updateTaintOverflowFlag(std::make_shared<TaintBit>(
        F_OVERFLOW_SUB, objSrcDest, objSrc, objResult));
} // fTaintCMP

//////////
// IMUL //
//////////

// SIMULATE
template<UINT32 lengthInBits> 
void BINARY::sIMUL_1M(THREADID tid, ADDRINT readAddress, ADDRINT implicitRegValue, ADDRINT insAddress) 
{ 
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    
    // valeurs fixes calcul�es � la compilation (m�taprogrammation)
    // 1ere op�rande et registre destination partie basse (AL/AX/EAX/RAX)
    REG regACC = RegisterACC<lengthInBits>::getReg(); 
    // registre de destination, partie haute (AH, DX, EDX, RDX)
    REG regIO  = RegisterIO<lengthInBits>::getReg();  
   
    bool isSrcDestTainted = pTmgrTls->isRegisterTainted<lengthInBits>(regACC);
    bool isSrcTainted =	pTmgrGlobal->isMemoryTainted<lengthInBits>(readAddress);

    if ( !(isSrcDestTainted || isSrcTainted)) 
    {
        // d�marquage flags et partie haute dest (partie basse d�j� non marqu�e)                       
        pTmgrTls->unTaintCarryFlag();
        pTmgrTls->unTaintOverflowFlag();	  
        pTmgrTls->unTaintRegister<lengthInBits>(regIO);
    }	
    else 
    {
        _LOGTAINT(tid, insAddress, "imul1M" + decstr(lengthInBits));
        	
        ObjectSource objSrcDest = (isSrcDestTainted)
            ? ObjectSource(pTmgrTls->getRegisterTaint<lengthInBits>(regACC, implicitRegValue))
            : ObjectSource(lengthInBits, implicitRegValue);

        ObjectSource objSrc = (isSrcTainted) 
            ? ObjectSource(pTmgrGlobal->getMemoryTaint<lengthInBits>(readAddress))
            : ObjectSource(lengthInBits, getMemoryValue<lengthInBits>(readAddress));
        
        // longueur r�sultat = double des sources	
        std::shared_ptr<TaintObject<(2*lengthInBits)>> resultPtr = 
            std::make_shared<TaintObject<(2*lengthInBits)>>(X_IMUL, objSrcDest, objSrc);

        // marquage des flags
        fTaintIMUL(pTmgrTls, resultPtr);	

        // marquage destination : � partir du resultat, il faut extraire
        // --les (lengthInBits>>3) octets faibles -> registre d'accumulation (AL/AX/EAX/RAX)
        // --les (lengthInBits>>3) octets forts   -> registre d'I/O (AH/DX/EDX/RDX)
        REGINDEX regIndexes[2] = { getRegIndex(regACC), getRegIndex(regIO) };
        UINT32 indexOfExtraction = 0;
        ObjectSource objResult(resultPtr);
        
        for (UINT32 index = 0 ; index < 2 ; ++index)
        {
            REGINDEX regIndex = regIndexes[index]; // partie basse puis partie haute
            for (UINT32 regPart = 0 ; regPart < (lengthInBits >> 3) ; ++regPart, ++indexOfExtraction)
            {
                pTmgrTls->updateTaintRegisterPart(regIndex, regPart, std::make_shared<TaintByte>(
                    EXTRACT,
                    objResult,
                    ObjectSource(8, indexOfExtraction)));
            }
        }
    }
} // sIMUL_1M

template<UINT32 lengthInBits> 
void BINARY::sIMUL_1R(THREADID tid, REG regSrc, ADDRINT regSrcValue,
                      ADDRINT implicitRegValue, ADDRINT insAddress) 
{ 
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    
    // valeurs fixes calcul�es � la compilation (m�taprogrammation)
    // 1ere op�rande et registre destination partie basse (AL/AX/EAX/RAX)
    REG regACC = RegisterACC<lengthInBits>::getReg(); 
    // registre de destination, partie haute (AH, DX, EDX, RDX)
    REG regIO  = RegisterIO<lengthInBits>::getReg();  
    
    bool isSrcDestTainted = pTmgrTls->isRegisterTainted<lengthInBits>(regACC);
    bool isSrcTainted =	pTmgrTls->isRegisterTainted<lengthInBits>(regSrc);
    

    if ( !(isSrcDestTainted || isSrcTainted)) 
    {
        // d�marquage flags et partie haute dest (partie basse non marqu�e)
        pTmgrTls->unTaintCarryFlag();
        pTmgrTls->unTaintOverflowFlag();	  
        pTmgrTls->unTaintRegister<lengthInBits>(regIO);
    }	
    else 
    {
        _LOGTAINT(tid, insAddress, "imul1R" + decstr(lengthInBits));

        ObjectSource objSrcDest = (isSrcDestTainted)
            ? ObjectSource(pTmgrTls->getRegisterTaint<lengthInBits>(regACC, implicitRegValue))
            : ObjectSource(lengthInBits, implicitRegValue);

        ObjectSource objSrc = (isSrcTainted) 
            ? ObjectSource(pTmgrTls->getRegisterTaint<lengthInBits>(regSrc, regSrcValue))
            : ObjectSource(lengthInBits, regSrcValue);
        
        // longueur r�sultat = double des sources	
        std::shared_ptr<TaintObject<(2*lengthInBits)>> resultPtr = 
            std::make_shared<TaintObject<(2*lengthInBits)>>(X_IMUL, objSrcDest, objSrc);

        // marquage des flags
        fTaintIMUL(pTmgrTls, resultPtr);	

        // marquage destination : � partir du resultat, il faut extraire
        // --les (lengthInBits>>3) octets faibles -> registre d'accumulation (AL/AX/EAX/RAX)
        // --les (lengthInBits>>3) octets forts   -> registre d'I/O (AH/DX/EDX/RDX)
        REGINDEX regIndexes[2] = { getRegIndex(regACC), getRegIndex(regIO) };
        UINT32 indexOfExtraction = 0;
        ObjectSource objResult(resultPtr);
        
        for (UINT32 index = 0 ; index < 2 ; ++index)
        {
            REGINDEX regIndex = regIndexes[index]; // partie basse puis partie haute
            for (UINT32 regPart = 0 ; regPart < (lengthInBits >> 3) ; ++regPart, ++indexOfExtraction)
            {
                pTmgrTls->updateTaintRegisterPart(regIndex, regPart, std::make_shared<TaintByte>(
                    EXTRACT,
                    objResult,
                    ObjectSource(8, indexOfExtraction)));
            }
        }
    }
} // sIMUL_1R

template<UINT32 lengthInBits> 
void BINARY::sIMUL_2MR(THREADID tid, ADDRINT readAddress, REG regSrcDest, 
                       ADDRINT regSrcDestValue, ADDRINT insAddress) 
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    
    // IMUL2MR <=> regSrcDest = mem * regSrcDest
    bool isSrcDestTainted = pTmgrTls->isRegisterTainted<lengthInBits>(regSrcDest);
    bool isSrcTainted =		pTmgrGlobal->isMemoryTainted<lengthInBits>(readAddress);

    if ( !(isSrcDestTainted || isSrcTainted))
    {
        // d�marquage flags (destination d�ja d�marqu�e)
        pTmgrTls->unTaintCarryFlag();  	
        pTmgrTls->unTaintOverflowFlag();	 
    }
    else 
    {
        _LOGTAINT(tid, insAddress, "imul2MR" + decstr(lengthInBits));
        
        ObjectSource objSrcDest = (isSrcDestTainted)
            ? ObjectSource(pTmgrTls->getRegisterTaint<lengthInBits>(regSrcDest, regSrcDestValue))
            : ObjectSource(lengthInBits, regSrcDestValue);

        ObjectSource objSrc = (isSrcTainted) 
            ? ObjectSource(pTmgrGlobal->getMemoryTaint<lengthInBits>(readAddress))
            : ObjectSource(lengthInBits, getMemoryValue<lengthInBits>(readAddress));

        // longueur r�sultat = double des sources	
        std::shared_ptr<TaintObject<(2*lengthInBits)>> resultPtr = 
            std::make_shared<TaintObject<(2*lengthInBits)>>(X_IMUL, objSrcDest, objSrc);

        // marquage des flags
        fTaintIMUL(pTmgrTls, resultPtr);	
        
        // marquage de la destination avec partie basse du r�sultat (partie haute ignor�e)
        // => marquage de "lengthInBits" objects de taille 8bits
        // � partir du r�sultat de longueur lengthInBits*2
        REGINDEX regIndex = getRegIndex(regSrcDest);
        ObjectSource objResult(resultPtr);

        for (UINT32 regPart = 0 ; regPart < (lengthInBits >> 3) ; ++regPart) 
        {
            pTmgrTls->updateTaintRegisterPart(regIndex, regPart, std::make_shared<TaintByte>(
                EXTRACT,
                objResult,
                ObjectSource(8, regPart)));
        }
    }
} // sIMUL_2MR

template<UINT32 lengthInBits> 
void BINARY::sIMUL_2RR(THREADID tid, REG regSrc, ADDRINT regSrcValue, REG regSrcDest, 
                       ADDRINT regSrcDestValue, ADDRINT insAddress)
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    
    // IMUL2RR <=> regSrcDest = regSrc * regSrcDest
    bool isSrcDestTainted = pTmgrTls->isRegisterTainted<lengthInBits>(regSrcDest);
    bool isSrcTainted =		pTmgrTls->isRegisterTainted<lengthInBits>(regSrc);

    if ( !(isSrcDestTainted || isSrcTainted)) 
    {
        // d�marquage flags (destination d�ja d�marqu�e)
        pTmgrTls->unTaintCarryFlag();
        pTmgrTls->unTaintOverflowFlag();		
    }
    else 
    {
        _LOGTAINT(tid, insAddress, "imul2RR" + decstr(lengthInBits));
 
        ObjectSource objSrcDest = (isSrcDestTainted)
            ? ObjectSource(pTmgrTls->getRegisterTaint<lengthInBits>(regSrcDest, regSrcDestValue))
            : ObjectSource(lengthInBits, regSrcDestValue);

        ObjectSource objSrc = (isSrcTainted) 
            ? ObjectSource(pTmgrTls->getRegisterTaint<lengthInBits>(regSrc, regSrcValue))
            : ObjectSource(lengthInBits, regSrcValue);
        
        // longueur r�sultat = double des sources	
        std::shared_ptr<TaintObject<(2*lengthInBits)>> resultPtr = 
            std::make_shared<TaintObject<(2*lengthInBits)>>(X_IMUL, objSrcDest, objSrc);

        // marquage des flags
        fTaintIMUL(pTmgrTls, resultPtr);	
        
        // marquage de la destination avec partie basse du r�sultat (partie haute ignor�e)
        // => marquage de "lengthInBits" objects de taille 8bits 
        // � partir du r�sultat de longueur lengthInBits*2
        REGINDEX regIndex = getRegIndex(regSrcDest);
        ObjectSource objResult(resultPtr);

        for (UINT32 regPart = 0 ; regPart < (lengthInBits >> 3) ; ++regPart) 
        {
            pTmgrTls->updateTaintRegisterPart(regIndex, regPart, std::make_shared<TaintByte>(
                EXTRACT,
                objResult,
                ObjectSource(8, regPart)));
        }
    }
} // sIMUL_2RR

template<UINT32 lengthInBits> 
void BINARY::sIMUL_3M(THREADID tid, ADDRINT value, ADDRINT readAddress, REG regDest, ADDRINT insAddress)
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    
    if (!pTmgrGlobal->isMemoryTainted<lengthInBits>(readAddress)) 
    {
        pTmgrTls->unTaintCarryFlag();
        pTmgrTls->unTaintOverflowFlag();    
        pTmgrTls->unTaintRegister<lengthInBits>(regDest);
    }			
    else 
    {
        _LOGTAINT(tid, insAddress, "imul3IM" + decstr(lengthInBits));

        // longueur r�sultat = double des sources	
        std::shared_ptr<TaintObject<(2*lengthInBits)>> resultPtr = 
            std::make_shared<TaintObject<(2*lengthInBits)>>(X_IMUL,
                ObjectSource(pTmgrGlobal->getMemoryTaint<lengthInBits>(readAddress)),
                ObjectSource(lengthInBits, value));

        // marquage des flags
        fTaintIMUL(pTmgrTls, resultPtr);	
        
        // marquage de la destination avec partie basse du r�sultat (partie haute ignor�e)
        // => marquage de "lengthInBits" objects de taille 8bits 
        // � partir du r�sultat de longueur lengthInBits*2
        REGINDEX regIndex = getRegIndex(regDest);
        ObjectSource objResult(resultPtr);

        for (UINT32 regPart = 0 ; regPart < (lengthInBits >> 3) ; ++regPart) 
        {
            pTmgrTls->updateTaintRegisterPart(regIndex, regPart, std::make_shared<TaintByte>(
                EXTRACT,
                objResult,
                ObjectSource(8, regPart)));
        }
    }
} // sIMUL_3M

template<UINT32 lengthInBits> 
void BINARY::sIMUL_3R(THREADID tid, ADDRINT value, REG regSrc, 
                      ADDRINT regSrcValue, REG regDest, ADDRINT insAddress)
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    
    if (!pTmgrTls->isRegisterTainted<lengthInBits>(regSrc)) 
    {
        pTmgrTls->unTaintCarryFlag();
        pTmgrTls->unTaintOverflowFlag();
        pTmgrTls->unTaintRegister<lengthInBits>(regDest);
    }			
    else 
    {
        _LOGTAINT(tid, insAddress, "imul3IR" + decstr(lengthInBits));

        // longueur r�sultat = double des sources	
        std::shared_ptr<TaintObject<(2*lengthInBits)>> resultPtr = 
            std::make_shared<TaintObject<(2*lengthInBits)>>(X_IMUL,
                ObjectSource(pTmgrTls->getRegisterTaint<lengthInBits>(regSrc, regSrcValue)),
                ObjectSource(lengthInBits, value));

        // marquage des flags (uniquement besoin du r�sultat) 
        fTaintIMUL(pTmgrTls, resultPtr);	
        
        // marquage de la destination avec partie basse du r�sultat (partie haute ignor�e)
        // => marquage de "lengthInBits" objects de taille 8bits
        // � partir du r�sultat de longueur lengthInBits*2
        REGINDEX regIndex = getRegIndex(regDest);
        ObjectSource objResult(resultPtr);

        for (UINT32 regPart = 0 ; regPart < (lengthInBits >> 3) ; ++regPart) 
        {
            pTmgrTls->updateTaintRegisterPart(regIndex, regPart, std::make_shared<TaintByte>(
                EXTRACT,
                objResult,
                ObjectSource(8, regPart)));
        }
    }
} // sIMUL_3R

////////////////////////////
// DIVISION (DIV et IDIV) //
////////////////////////////

// SIMULATE
 template<UINT32 lengthInBits> 
 void BINARY::sDIVISION_M(THREADID tid, ADDRINT readAddress,
    bool isSignedDivision, ADDRINT lowDividendValue, ADDRINT highDividendValue, ADDRINT insAddress)
{
    // dividende = registre (lengthInBits*2 bits, compos�), diviseur = m�moire (lengthInBits bits)
    // partie faible du dividende = registre d'accumulation (AL/AX/EAX/RAX)
    // partie forte du dividende = registre d'I/O (AH/DX/EDX/RDX)
    // valeurs fixes calcul�es � la compilation (m�taprogrammation)
    REG regACC = RegisterACC<lengthInBits>::getReg(); 
    REG regIO  = RegisterIO<lengthInBits>::getReg();  
    
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    
    bool isDivisorTainted =      pTmgrGlobal->isMemoryTainted<lengthInBits>(readAddress);
    bool isLowDividendTainted  = pTmgrTls->isRegisterTainted<lengthInBits>(regACC);
    bool isHighDividendTainted = pTmgrTls->isRegisterTainted<lengthInBits>(regIO);
    
    if (isLowDividendTainted || isHighDividendTainted || isDivisorTainted) 
    {
        _LOGTAINT(tid, insAddress, (isSignedDivision ? "IDIVM" : "DIVM") + decstr(lengthInBits));

        ObjectSource objSrcLowDividend = (isLowDividendTainted)
            ? ObjectSource(pTmgrTls->getRegisterTaint<lengthInBits>(regACC, lowDividendValue))
            : ObjectSource(lengthInBits, lowDividendValue);

        ObjectSource objSrcHighDividend = (isHighDividendTainted)
            ? ObjectSource(pTmgrTls->getRegisterTaint<lengthInBits>(regIO, highDividendValue))
            : ObjectSource(lengthInBits, highDividendValue);
  
        ObjectSource objSrcDivisor = (isDivisorTainted) 
            ? ObjectSource(pTmgrGlobal->getMemoryTaint<lengthInBits>(readAddress))
            : ObjectSource(lengthInBits, getMemoryValue<lengthInBits>(readAddress));

        // cr�ation de l'objet correspondant au quotient
        TaintObject<lengthInBits> quotient(
            (isSignedDivision) ? X_IDIV_QUO : X_DIV_QUO, /* division sign�e ou non */
            objSrcLowDividend,
            objSrcHighDividend,
            objSrcDivisor);
        
        // cr�ation de l'objet correspondant au reste
        TaintObject<lengthInBits> remainder(
            (isSignedDivision) ? X_IDIV_REM : X_DIV_REM, /* division sign�e ou non */
            objSrcLowDividend,
            objSrcHighDividend,
            objSrcDivisor);

        // Affectation quotient et reste aux registres ad�quats (idem dividende, cf manuel Intel)
        pTmgrTls->updateTaintRegister<lengthInBits>(regACC, MK_TAINT_OBJECT_PTR(quotient));
        pTmgrTls->updateTaintRegister<lengthInBits>(regIO,  MK_TAINT_OBJECT_PTR(remainder));	
    }
} // sDIVISION_M

template<UINT32 lengthInBits> 
void BINARY::sDIVISION_R(THREADID tid, REG regSrc, ADDRINT regSrcValue, 
    bool isSignedDivision, ADDRINT lowDividendValue, ADDRINT highDividendValue, ADDRINT insAddress)
{
    // dividende = registre (lengthInBits*2 bits, compos�), diviseur = m�moire (lengthInBits bits)
    // partie faible du dividende = registre d'accumulation (AL/AX/EAX/RAX)
    // partie forte du dividende = registre d'I/O (AH/DX/EDX/RDX)
    // valeurs fixes calcul�es � la compilation (m�taprogrammation)
    REG regACC = RegisterACC<lengthInBits>::getReg(); 
    REG regIO  = RegisterIO<lengthInBits> ::getReg();  
    
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    
    bool isDivisorTainted      = pTmgrTls->isRegisterTainted<lengthInBits>(regSrc, regSrcValue);
    bool isLowDividendTainted  = pTmgrTls->isRegisterTainted<lengthInBits>(regACC);
    bool isHighDividendTainted = pTmgrTls->isRegisterTainted<lengthInBits>(regIO);
    
    if (isLowDividendTainted || isHighDividendTainted || isDivisorTainted) 
    {
        _LOGTAINT(tid, insAddress, (isSignedDivision ? "IDIVR" : "DIVR ") + decstr(lengthInBits));

        ObjectSource objSrcLowDividend = (isLowDividendTainted)
            ? ObjectSource(pTmgrTls->getRegisterTaint<lengthInBits>(regACC, lowDividendValue))
            : ObjectSource(lengthInBits, lowDividendValue);

        ObjectSource objSrcHighDividend = (isHighDividendTainted)
            ? ObjectSource(pTmgrTls->getRegisterTaint<lengthInBits>(regIO, highDividendValue))
            : ObjectSource(lengthInBits, highDividendValue);

        ObjectSource objSrcDivisor = (isDivisorTainted)
            ? ObjectSource(pTmgrTls->getRegisterTaint<lengthInBits>(regSrc, regSrcValue))
            : ObjectSource(lengthInBits, regSrcValue);
         
        // cr�ation de l'objet correspondant au quotient
        TaintObject<lengthInBits> quotient(
            (isSignedDivision) ? X_IDIV_QUO : X_DIV_QUO, /* division sign�e ou non */
            objSrcLowDividend,
            objSrcHighDividend,
            objSrcDivisor);
        
        // cr�ation de l'objet correspondant au reste
        TaintObject<lengthInBits> remainder(
            (isSignedDivision) ? X_IDIV_REM : X_DIV_REM, /* division sign�e ou non */
            objSrcLowDividend,
            objSrcHighDividend,
            objSrcDivisor);

        // Affectation quotient et reste aux registres ad�quats (idem dividende, cf manuel Intel)
        pTmgrTls->updateTaintRegister<lengthInBits>(regACC, MK_TAINT_OBJECT_PTR(quotient));
        pTmgrTls->updateTaintRegister<lengthInBits>(regIO,  MK_TAINT_OBJECT_PTR(remainder));	
    }
} //sDIVISION_R