/*********/
/** ROL **/
/*********/

template<UINT32 len> 
void ROTATE::sROL_IM(THREADID tid, UINT32 maskedDepl, ADDRINT writeAddress ADDRESS_DEBUG) 
{  
    TaintManager_Thread *pTmgrTls = static_cast<TaintManager_Thread*>(PIN_GetThreadData(tlsKeyTaint, tid));

    // op�rande non marqu�e => d�marquage flags
    if (!pTmgrGlobal->isMemoryTainted<len>(writeAddress)) fUnTaintROTATE(pTmgrTls);
    else 
    { 
        _LOGTAINT("ROLIM " << len << " ");
        
        // construction du r�sultat
        std::shared_ptr<TaintObject<len>> resultPtr = std::make_shared<TaintObject<len>>(
            X_ROL,
            ObjectSource(pTmgrGlobal->getMemoryTaint<len>(writeAddress)),  
            ObjectSource(8, maskedDepl));

        // marquage flags
        fROL(pTmgrTls, resultPtr, maskedDepl);

        // MARQUAGE DESTINATION 
        // 1) cas des d�placements multiples de 8 bits : simple d�placement du marquage
        // cela �vite la multiplication des objets marqu�s
        if ((maskedDepl & 0x7) == 0) 
        {
            // sauvegarde du marquage de la source dans un vecteur
            vector<TaintBytePtr> vSavedSrc;
            ADDRINT highestAddress = writeAddress + (len >> 3);
            for (ADDRINT targetAddress = writeAddress ; targetAddress < highestAddress ; ++targetAddress)
            {
                // objet ou nullptr selon le marquage
                vSavedSrc.push_back(pTmgrGlobal->getMemoryTaint<8>(targetAddress));
            }

            // it�rateurs de d�but et de fin du vecteur repr�sentant la source
            auto it = vSavedSrc.begin(), lastIt = vSavedSrc.end();

            // parcours de chaque octet de la source, et affectation � l'octet de destination
            // pour ROL, l'octet 0 va se retrouver � l'octet d'offset 'maskedDepl >> 3'
            // et ainsi de suite. Si la destination d�passe l'adresse haute, la destination est l'offset 0
            for (ADDRINT targetAddress = writeAddress + (maskedDepl >> 3) ; it != lastIt ; ++it, ++targetAddress)
            {
                // Si arriv� � l'octet fort => repartir � l'offset 0
                if (targetAddress == highestAddress) targetAddress = writeAddress;
                
                // r�affectation de l'octet source � la bonne place apres rotation
                if ((bool) *it) pTmgrGlobal->updateMemoryTaint<8>(targetAddress, *it);
                else            pTmgrGlobal->unTaintMemory<8>(targetAddress);
            }        
        }
        // 2) autre cas : marquage destination 'normalement'
        else  pTmgrGlobal->updateMemoryTaint<len>(writeAddress, resultPtr);
    }
} // sROL_IM

template<UINT32 len> 
void ROTATE::sROL_IR(THREADID tid, UINT32 maskedDepl, REG reg, ADDRINT regValue ADDRESS_DEBUG) 
{  
    TaintManager_Thread *pTmgrTls = static_cast<TaintManager_Thread*>(PIN_GetThreadData(tlsKeyTaint, tid));

    // op�rande non marqu�e => d�marquage flags
    if (!pTmgrTls->isRegisterTainted<len>(reg)) fUnTaintROTATE(pTmgrTls);
    else 
    { 
        _LOGTAINT("ROLIR " << len << " ");
        
        // construction du r�sultat
        std::shared_ptr<TaintObject<len>> resultPtr = std::make_shared<TaintObject<len>>(
            X_ROL,
            ObjectSource(pTmgrTls->getRegisterTaint<len>(reg, regValue)),  
            ObjectSource(8, maskedDepl));

        // marquage flags
        fROL(pTmgrTls, resultPtr, maskedDepl);

        // MARQUAGE DESTINATION 
        // 1) cas des d�placements multiples de 8 bits : simple d�placement du marquage
        // cela �vite la multiplication des objets marqu�s
        if ((maskedDepl & 0x7) == 0) 
        {
            // sauvegarde du marquage de la source dans un vecteur
            REGINDEX regIndex = getRegIndex(reg);
            vector<TaintBytePtr> vSavedSrc;
            for (UINT32 regPart = 0 ; regPart < (len >> 3) ; ++regPart)
            {
                // objet ou nullptr selon le marquage
                vSavedSrc.push_back(pTmgrTls->getRegisterPartTaint(regIndex, regPart));
            }

            // it�rateurs de d�but et de fin du vecteur repr�sentant la source
            auto it = vSavedSrc.begin(), lastIt = vSavedSrc.end();
            
            // parcours de chaque octet de la source, et affectation � l'octet de destination
            // pour ROL, l'octet 0 va se retrouver � l'octet 'maskedDepl >> 3'
            // et ainsi de suite. Si la destination d�passe l'index haut, la destination est l'octet 0
            for (UINT32 regPart = maskedDepl >> 3 ; it != lastIt ; ++it, ++regPart)
            {
                // Si arriv� � l'octet fort => repartir � l'offset 0
                if (regPart == len >> 3) regPart = 0;

                if ((bool) *it) pTmgrTls->updateTaintRegisterPart(regIndex, regPart, *it);
                else pTmgrTls->unTaintRegisterPart(regIndex, regPart);
            }
        }
        // 2) autre cas : marquage destination 'normalement'
        else  pTmgrTls->updateTaintRegister<len>(reg, resultPtr);
    }
} // sROL_IR

template<UINT32 len> 
void ROTATE::sROL_RM(THREADID tid, ADDRINT regCLValue, ADDRINT writeAddress ADDRESS_DEBUG) 
{
    TaintManager_Thread *pTmgrTls = static_cast<TaintManager_Thread*>(PIN_GetThreadData(tlsKeyTaint, tid));

    bool isCountTainted = pTmgrTls->isRegisterTainted<8>(REG_CL);
    bool isDestTainted  = pTmgrGlobal->isMemoryTainted<len>(writeAddress);
    
    if ( !(isDestTainted || isCountTainted)) fUnTaintROTATE(pTmgrTls);
    // d�placement non marqu� (mais m�moire oui) => cas ROL_IM
    else if (!isCountTainted) 
    {
        // masquage du d�placement avant appel de ROL_IM
        UINT32 maskedDepl = (len == 64) ? (regCLValue & 0x3f) : (regCLValue & 0x1f);
        sROL_IM<len>(tid, maskedDepl, writeAddress INSADDRESS); 
    }
    else // d�placement marqu�  
    {
        _LOGTAINT("ROL_RM, d�placement marqu�, destination" << ((isDestTainted) ? " marqu�" : " non marqu�"));
            
        // cr�ation de l'objet Source correspondant � la m�moire
        ObjectSource objSrcMem = (isDestTainted)
            ? ObjectSource(pTmgrGlobal->getMemoryTaint<len>(writeAddress)) 
            : ObjectSource(len, getMemoryValue<len>(writeAddress)); 

        // cr�ation de l'objet resultat de l'op�ration
        std::shared_ptr<TaintObject<len>> resultPtr = std::make_shared<TaintObject<len>>(
            X_ROL,
            objSrcMem,
            ObjectSource(pTmgrTls->getRegisterTaint(REG_CL))); // CL contient le nombre de bits de la rotation

        // marquage flags 
        fROL(pTmgrTls, resultPtr);
        // marquage destination
        pTmgrGlobal->updateMemoryTaint<len>(writeAddress, resultPtr);
    }
} // sROL_RM

template<UINT32 len> 
void ROTATE::sROL_RR(THREADID tid, ADDRINT regCLValue, REG reg, ADDRINT regValue ADDRESS_DEBUG) 
{
    TaintManager_Thread *pTmgrTls = static_cast<TaintManager_Thread*>(PIN_GetThreadData(tlsKeyTaint, tid));

    bool isCountTainted = pTmgrTls->isRegisterTainted<8>(REG_CL);
    bool isDestTainted  = pTmgrTls->isRegisterTainted<len>(reg);
    
    if ( !(isDestTainted || isCountTainted) ) fUnTaintROTATE(pTmgrTls);
    // d�placement non marqu� (mais registre oui) => cas ROL_IR
    else if (!isCountTainted)
    {
        // masquage du d�placement avant appel de ROL_IM
        UINT32 maskedDepl = (len == 64) ? (regCLValue & 0x3f) : (regCLValue & 0x1f);
        sROL_IR<len>(tid, maskedDepl, reg, regValue INSADDRESS); 
    }
    else // d�placement marqu� 
    {
        _LOGTAINT("ROL_RR, d�placement marqu�, destination" << ((isDestTainted) ? " marqu�" : " non marqu�"));
        
        // cr�ation de l'objet Source correspondant au registre cible
        ObjectSource objSrcReg = (isDestTainted)
            ? ObjectSource(pTmgrTls->getRegisterTaint<len>(reg, regValue))
            : ObjectSource(len, regValue); 

        // cr�ation de l'objet resultat de l'op�ration
        std::shared_ptr<TaintObject<len>> resultPtr = std::make_shared<TaintObject<len>>(
            X_ROL,
            objSrcReg,
            ObjectSource(pTmgrTls->getRegisterTaint(REG_CL))); // CL contient le nombre de bits de la rotation

        // marquage flags 
        fROL(pTmgrTls, resultPtr);
        // marquage destination
        pTmgrTls->updateTaintRegister<len>(reg, resultPtr);
    }
} // sROL_RR


/*********/
/** ROR **/
/*********/

template<UINT32 len> 
void ROTATE::sROR_IM(THREADID tid, UINT32 maskedDepl, ADDRINT writeAddress ADDRESS_DEBUG)
{
    TaintManager_Thread *pTmgrTls = static_cast<TaintManager_Thread*>(PIN_GetThreadData(tlsKeyTaint, tid));

    // op�rande non marqu�e => d�marquage flags
    if (!pTmgrGlobal->isMemoryTainted<len>(writeAddress)) fUnTaintROTATE(pTmgrTls);
    else 
    { 
        _LOGTAINT("RORIM " << len << " ");
        
        // construction du r�sultat
        std::shared_ptr<TaintObject<len>> resultPtr = std::make_shared<TaintObject<len>>(
            X_ROR,
            ObjectSource(pTmgrGlobal->getMemoryTaint<len>(writeAddress)),  
            ObjectSource(8, maskedDepl));

        // marquage flags
        fROL(pTmgrTls, resultPtr, maskedDepl);

        // MARQUAGE DESTINATION 
        // 1) cas des d�placements multiples de 8 bits : simple d�placement du marquage
        // cela �vite la multiplication des objets marqu�s
        if ((maskedDepl & 0x7) == 0) 
        {
            // sauvegarde du marquage de la source dans un vecteur
            vector<TaintBytePtr> vSavedSrc;
            ADDRINT highestAddress = writeAddress + (len >> 3);
            for (ADDRINT targetAddress = writeAddress ; targetAddress < highestAddress ; ++targetAddress)
            {
                // objet ou nullptr selon le marquage
                vSavedSrc.push_back(pTmgrGlobal->getMemoryTaint<8>(targetAddress));
            }

            // it�rateurs de d�but et de fin du vecteur repr�sentant la source
            auto it = vSavedSrc.begin(), lastIt = vSavedSrc.end();

            // parcours de chaque octet de la source, et affectation � l'octet de destination
            // pour ROR, l'octet 0 va se retrouver � l'octet d'offset '(len >> 3) - (maskedDepl >> 3)'
            // et ainsi de suite. Si la destination d�passe l'adresse haute, la destination est l'offset 0
            for (ADDRINT targetAddress = writeAddress + (len >> 3) - (maskedDepl >> 3) ; it != lastIt ; ++it, ++targetAddress)
            {
                // Si arriv� � l'octet fort => repartir � l'offset 0
                if (targetAddress == highestAddress) targetAddress = writeAddress;
                
                // r�affectation de l'octet source � la bonne place apres rotation
                if ((bool) *it) pTmgrGlobal->updateMemoryTaint<8>(targetAddress, *it);
                else            pTmgrGlobal->unTaintMemory<8>(targetAddress);
            }        
        }
        // 2) autre cas : marquage destination 'normalement'
        else  pTmgrGlobal->updateMemoryTaint<len>(writeAddress, resultPtr);
    }
}   

template<UINT32 len> 
void ROTATE::sROR_IR(THREADID tid, UINT32 maskedDepl, REG reg, ADDRINT regValue ADDRESS_DEBUG)
{
    TaintManager_Thread *pTmgrTls = static_cast<TaintManager_Thread*>(PIN_GetThreadData(tlsKeyTaint, tid));

    // op�rande non marqu�e => d�marquage flags
    if (!pTmgrTls->isRegisterTainted<len>(reg)) fUnTaintROTATE(pTmgrTls);
    else 
    { 
        _LOGTAINT("ROLIR " << len << " ");
        
        // construction du r�sultat
        std::shared_ptr<TaintObject<len>> resultPtr = std::make_shared<TaintObject<len>>(
            X_ROR,
            ObjectSource(pTmgrTls->getRegisterTaint<len>(reg, regValue)),  
            ObjectSource(8, maskedDepl));

        // marquage flags
        fROR(pTmgrTls, resultPtr, maskedDepl);

        // MARQUAGE DESTINATION 
        // 1) cas des d�placements multiples de 8 bits : simple d�placement du marquage
        // cela �vite la multiplication des objets marqu�s
        if ((maskedDepl & 0x7) == 0) 
        {
            // sauvegarde du marquage de la source dans un vecteur
            REGINDEX regIndex = getRegIndex(reg);
            vector<TaintBytePtr> vSavedSrc;
            for (UINT32 regPart = 0 ; regPart < (len >> 3) ; ++regPart)
            {
                // objet ou nullptr selon le marquage
                vSavedSrc.push_back(pTmgrTls->getRegisterPartTaint(regIndex, regPart));
            }

            // it�rateurs de d�but et de fin du vecteur repr�sentant la source
            auto it = vSavedSrc.begin(), lastIt = vSavedSrc.end();

            // parcours de chaque octet de la source, et affectation � l'octet de destination
            // pour ROR, l'octet 0 va se retrouver � l'octet 'len >> 3' - 'maskedDepl >> 3'
            // et ainsi de suite. Si la destination d�passe l'index haut, la destination est l'octet 0
            for (UINT32 regPart = (len >> 3) - (maskedDepl >> 3) ; it != lastIt ; ++it, ++regPart)
            {
                // Si arriv� � l'octet fort => repartir � l'offset 0
                if (regPart == len >> 3) regPart = 0;

                if ((bool) *it) pTmgrTls->updateTaintRegisterPart(regIndex, regPart, *it);
                else pTmgrTls->unTaintRegisterPart(regIndex, regPart);
            }
        }
        // 2) autre cas : marquage destination 'normalement'
        else  pTmgrTls->updateTaintRegister<len>(reg, resultPtr);
    }
} // sROR_IR

template<UINT32 len> 
void ROTATE::sROR_RM(THREADID tid, ADDRINT regCLValue, ADDRINT writeAddress ADDRESS_DEBUG)
{
    TaintManager_Thread *pTmgrTls = static_cast<TaintManager_Thread*>(PIN_GetThreadData(tlsKeyTaint, tid));

    bool isCountTainted = pTmgrTls->isRegisterTainted<8>(REG_CL);
    bool isDestTainted  = pTmgrGlobal->isMemoryTainted<len>(writeAddress);
    
    if ( !(isDestTainted || isCountTainted)) fUnTaintROTATE(pTmgrTls);
    // d�placement non marqu� (mais m�moire oui) => cas ROR_IM
    else if (!isCountTainted) 
    {
        // masquage du d�placement avant appel de ROL_IM
        UINT32 maskedDepl = (len == 64) ? (regCLValue & 0x3f) : (regCLValue & 0x1f);
        sROR_IM<len>(tid, maskedDepl, writeAddress INSADDRESS);
    }
    else // d�placement marqu�  
    {
        _LOGTAINT("ROR_RM, d�placement marqu�, destination" << ((isDestTainted) ? " marqu�" : " non marqu�"));
            
        // cr�ation de l'objet Source correspondant � la m�moire
        ObjectSource objSrcMem = (isDestTainted)
            ? ObjectSource(pTmgrGlobal->getMemoryTaint<len>(writeAddress)) 
            : ObjectSource(len, getMemoryValue<len>(writeAddress)); 

        // cr�ation de l'objet resultat de l'op�ration
        std::shared_ptr<TaintObject<len>> resultPtr = std::make_shared<TaintObject<len>>(
            X_ROR,
            objSrcMem,
            ObjectSource(pTmgrTls->getRegisterTaint(REG_CL))); // CL contient le nombre de bits de la rotation

        // marquage flags 
        fROR(pTmgrTls, resultPtr);
        // marquage destination
        pTmgrGlobal->updateMemoryTaint<len>(writeAddress, resultPtr);
    }
} // sROR_RM

template<UINT32 len>
void ROTATE::sROR_RR(THREADID tid, ADDRINT regCLValue, REG reg, ADDRINT regValue ADDRESS_DEBUG)
{
    TaintManager_Thread *pTmgrTls = static_cast<TaintManager_Thread*>(PIN_GetThreadData(tlsKeyTaint, tid));

    bool isCountTainted = pTmgrTls->isRegisterTainted<8>(REG_CL);
    bool isDestTainted  = pTmgrTls->isRegisterTainted<len>(reg);
    
    if ( !(isDestTainted || isCountTainted) ) fUnTaintROTATE(pTmgrTls);
    // d�placement non marqu� (mais registre oui) => cas ROR_IR
    else if (!isCountTainted)
    {
        // masquage du d�placement avant appel de ROL_IM
        UINT32 maskedDepl = (len == 64) ? (regCLValue & 0x3f) : (regCLValue & 0x1f);
        sROR_IR<len>(tid, maskedDepl, reg, regValue INSADDRESS);  
    }
    else // d�placement marqu� 
    {
        _LOGTAINT("ROR_RR, d�placement marqu�, destination" << ((isDestTainted) ? " marqu�" : " non marqu�"));
        
        // cr�ation de l'objet Source correspondant au registre cible
        ObjectSource objSrcReg = (isDestTainted)
            ? ObjectSource(pTmgrTls->getRegisterTaint<len>(reg, regValue))
            : ObjectSource(len, regValue); 

        // cr�ation de l'objet resultat de l'op�ration
        std::shared_ptr<TaintObject<len>> resultPtr = std::make_shared<TaintObject<len>>(
            X_ROR,
            objSrcReg,
            ObjectSource(pTmgrTls->getRegisterTaint(REG_CL))); // CL contient le nombre de bits de la rotation

        // marquage flags 
        fROR(pTmgrTls, resultPtr);
        // marquage destination
        pTmgrTls->updateTaintRegister<len>(reg, resultPtr);
    }
} // sROR_RR

/*********/
/** RCL **/
/*********/

template<UINT32 len> void ROTATE::sRCL_IM
    (THREADID tid, UINT32 maskedDepl, ADDRINT writeAddress, ADDRINT regGflagsValue ADDRESS_DEBUG)
{  
    TaintManager_Thread *pTmgrTls = static_cast<TaintManager_Thread*>(PIN_GetThreadData(tlsKeyTaint, tid));

    bool isSrcTainted       = pTmgrGlobal->isMemoryTainted<len>(writeAddress);
    bool isCarryFlagTainted = pTmgrTls->isCarryFlagTainted();

    // op�randes non marqu�es => d�marquage flags 
    if (!(isSrcTainted || isCarryFlagTainted)) fUnTaintROTATE(pTmgrTls);
    else 
    { 
        _LOGTAINT("RCL_IM " << len << " ");

        // R�cup�ration du marquage ou valeur de la m�moire
        ObjectSource objSrcMem = (isSrcTainted) 
            ? ObjectSource(pTmgrGlobal->getMemoryTaint<len>(writeAddress))
            : ObjectSource(len, getMemoryValue<len>(writeAddress));

        // R�cup�ration du Carry Flag (marqu� ou valeur);
        ObjectSource objSrcCF = (isCarryFlagTainted) 
            ? ObjectSource(pTmgrTls->getTaintCarryFlag())
            : ObjectSource(1, (regGflagsValue >> CARRY_FLAG) & 1);

        // construction du r�sultat
        std::shared_ptr<TaintObject<len>> resultPtr = std::make_shared<TaintObject<len>>(
            X_RCL,
            objSrcMem,
            objSrcCF,
            ObjectSource(8, maskedDepl));

        // marquage flags
        fRCL(pTmgrTls, resultPtr, objSrcMem, maskedDepl);

        // marquage destination
        pTmgrGlobal->updateMemoryTaint<len>(writeAddress, resultPtr);
    }
} // sRCL_IM

template<UINT32 len> void ROTATE::sRCL_IR
    (THREADID tid, UINT32 maskedDepl, REG reg, ADDRINT regValue, ADDRINT regGflagsValue ADDRESS_DEBUG)
{
    TaintManager_Thread *pTmgrTls = static_cast<TaintManager_Thread*>(PIN_GetThreadData(tlsKeyTaint, tid));

    bool isSrcTainted       = pTmgrTls->isRegisterTainted<len>(reg);
    bool isCarryFlagTainted = pTmgrTls->isCarryFlagTainted();
    
    // op�randes non marqu�es => d�marquage flags 
    if (!(isSrcTainted || isCarryFlagTainted)) fUnTaintROTATE(pTmgrTls);
    else 
    { 
        _LOGTAINT("RCL_IR " << len << " ");
        
        // R�cup�ration du marquage ou valeur du registre
        ObjectSource objSrcReg = (isSrcTainted) 
            ? ObjectSource(pTmgrTls->getRegisterTaint<len>(reg, regValue))
            : ObjectSource(len, regValue);
        
        // R�cup�ration du Carry Flag (marqu� ou valeur);
        ObjectSource objSrcCF = (isCarryFlagTainted) 
            ? ObjectSource(pTmgrTls->getTaintCarryFlag())
            : ObjectSource(1, (regGflagsValue >> CARRY_FLAG) & 1);

        // construction du r�sultat
        std::shared_ptr<TaintObject<len>> resultPtr = std::make_shared<TaintObject<len>>(
            X_RCL,
            objSrcReg,
            objSrcCF,
            ObjectSource(8, maskedDepl));

        // marquage flags
        fRCL(pTmgrTls, resultPtr, objSrcReg, maskedDepl);

        // marquage destination
        pTmgrTls->updateTaintRegister<len>(reg, resultPtr);
    }
} // sRCL

template<UINT32 len> void ROTATE::sRCL_RM
    (THREADID tid, ADDRINT regCLValue, ADDRINT writeAddress, ADDRINT regGflagsValue ADDRESS_DEBUG)
{
    TaintManager_Thread *pTmgrTls = static_cast<TaintManager_Thread*>(PIN_GetThreadData(tlsKeyTaint, tid));

    bool isCountTainted     = pTmgrTls->isRegisterTainted<8>(REG_CL);
    bool isDestTainted      = pTmgrGlobal->isMemoryTainted<len>(writeAddress);
    bool isCarryFlagTainted = pTmgrTls->isCarryFlagTainted();
    
    if ( !(isDestTainted || isCountTainted || isCarryFlagTainted)) fUnTaintROTATE(pTmgrTls);
    // d�placement non marqu� (mais m�moire et/ou carry oui) => cas RCL_IM
    else if (!isCountTainted) 
    {
        // masquage du d�placement avant appel de RCL_IM
        UINT32 maskedDepl = (len == 64) ? (regCLValue & 0x3f) : (regCLValue & 0x1f);
        sRCL_IM<len>(tid, maskedDepl, writeAddress , regGflagsValue INSADDRESS); 
    }
    else // d�placement marqu�  
    {
        _LOGTAINT("RCL_RM, d�placement marqu�, destination" << ((isDestTainted) ? " marqu�" : " non marqu�"));
            
        // cr�ation de l'objet Source correspondant � la m�moire
        ObjectSource objSrcMem = (isDestTainted)
            ? ObjectSource(pTmgrGlobal->getMemoryTaint<len>(writeAddress)) 
            : ObjectSource(len, getMemoryValue<len>(writeAddress));

        // R�cup�ration du Carry Flag (marqu� ou valeur);
        ObjectSource objSrcCF = (isCarryFlagTainted) 
            ? ObjectSource(pTmgrTls->getTaintCarryFlag())
            : ObjectSource(1, (regGflagsValue >> CARRY_FLAG) & 1);

        // r�cup�ration du d�placement (marqu�)
        TaintBytePtr tbCountPtr = pTmgrTls->getRegisterTaint(REG_CL);

        // construction du r�sultat
        std::shared_ptr<TaintObject<len>> resultPtr = std::make_shared<TaintObject<len>>(
            X_RCL,
            objSrcMem,
            objSrcCF,
            ObjectSource(tbCountPtr));

        // marquage flags
        fRCL(pTmgrTls, objSrcMem, tbCountPtr);
        // marquage destination
        pTmgrGlobal->updateMemoryTaint<len>(writeAddress, resultPtr);
    }
} // sRCL_RM

template<UINT32 len> void ROTATE::sRCL_RR
    (THREADID tid, ADDRINT regCLValue, REG reg, ADDRINT regValue, ADDRINT regGflagsValue  ADDRESS_DEBUG)
{
    TaintManager_Thread *pTmgrTls = static_cast<TaintManager_Thread*>(PIN_GetThreadData(tlsKeyTaint, tid));

    bool isCountTainted     = pTmgrTls->isRegisterTainted<8>(REG_CL);
    bool isDestTainted      = pTmgrTls->isRegisterTainted<len>(reg);
    bool isCarryFlagTainted = pTmgrTls->isCarryFlagTainted();
    
    if ( !(isDestTainted || isCountTainted || isCarryFlagTainted)) fUnTaintROTATE(pTmgrTls);
    // d�placement non marqu� (mais registre oui) => cas RCL_IR
    else if (!isCountTainted)
    {
        // masquage du d�placement avant appel de RCL_IR
        UINT32 maskedDepl = (len == 64) ? (regCLValue & 0x3f) : (regCLValue & 0x1f);
        sRCL_IR<len>(tid, maskedDepl, reg, regValue, regGflagsValue INSADDRESS); 
    }
    else // d�placement marqu� 
    {
        _LOGTAINT("RCL_RR, d�placement marqu�, destination" << ((isDestTainted) ? " marqu�" : " non marqu�"));
         
        // cr�ation de l'objet Source correspondant au registre cible
        ObjectSource objSrcReg = (isDestTainted)
            ? ObjectSource(pTmgrTls->getRegisterTaint<len>(reg, regValue))
            : ObjectSource(len, regValue);

        // R�cup�ration du Carry Flag (marqu� ou valeur);
        ObjectSource objSrcCF = (isCarryFlagTainted) 
            ? ObjectSource(pTmgrTls->getTaintCarryFlag())
            : ObjectSource(1, (regGflagsValue >> CARRY_FLAG) & 1);

        // r�cup�ration du d�placement (marqu�)
        TaintBytePtr tbCountPtr = pTmgrTls->getRegisterTaint(REG_CL);

        // cr�ation de l'objet resultat de l'op�ration
        std::shared_ptr<TaintObject<len>> resultPtr = std::make_shared<TaintObject<len>>(
            X_RCL,
            objSrcReg,
            ObjectSource(tbCountPtr)); 
     
        // marquage flags
        fRCL(pTmgrTls, objSrcReg, tbCountPtr);
        // marquage destination
        pTmgrTls->updateTaintRegister<len>(reg, resultPtr);
    }
} // sRCL_RR
     
/*********/
/** RCR **/
/*********/

template<UINT32 len> void ROTATE::sRCR_IM
    (THREADID tid, UINT32 maskedDepl, ADDRINT writeAddress, ADDRINT regGflagsValue ADDRESS_DEBUG)
{  
    TaintManager_Thread *pTmgrTls = static_cast<TaintManager_Thread*>(PIN_GetThreadData(tlsKeyTaint, tid));

    bool isSrcTainted       = pTmgrGlobal->isMemoryTainted<len>(writeAddress);
    bool isCarryFlagTainted = pTmgrTls->isCarryFlagTainted();

    // op�randes non marqu�es => d�marquage flags 
    if (!(isSrcTainted || isCarryFlagTainted)) fUnTaintROTATE(pTmgrTls);
    else 
    { 
        _LOGTAINT("RCR_IM " << len << " ");

        // R�cup�ration du marquage ou valeur de la m�moire
        ObjectSource objSrcMem = (isSrcTainted) 
            ? ObjectSource(pTmgrGlobal->getMemoryTaint<len>(writeAddress))
            : ObjectSource(len, getMemoryValue<len>(writeAddress));

        // R�cup�ration du Carry Flag (marqu� ou valeur);
        ObjectSource objSrcCF = (isCarryFlagTainted) 
            ? ObjectSource(pTmgrTls->getTaintCarryFlag())
            : ObjectSource(1, (regGflagsValue >> CARRY_FLAG) & 1);

        // construction du r�sultat
        std::shared_ptr<TaintObject<len>> resultPtr = std::make_shared<TaintObject<len>>(
            X_RCR,
            objSrcMem,
            objSrcCF,
            ObjectSource(8, maskedDepl));

        // marquage flags
        fRCR(pTmgrTls, objSrcMem, objSrcCF, maskedDepl);

        // marquage destination
        pTmgrGlobal->updateMemoryTaint<len>(writeAddress, resultPtr);
    }
} // sRCR_IM

template<UINT32 len> void ROTATE::sRCR_IR
    (THREADID tid, UINT32 maskedDepl, REG reg, ADDRINT regValue, ADDRINT regGflagsValue ADDRESS_DEBUG)
{
    TaintManager_Thread *pTmgrTls = static_cast<TaintManager_Thread*>(PIN_GetThreadData(tlsKeyTaint, tid));

    bool isSrcTainted       = pTmgrTls->isRegisterTainted<len>(reg);
    bool isCarryFlagTainted = pTmgrTls->isCarryFlagTainted();
    
    // op�randes non marqu�es => d�marquage flags 
    if (!(isSrcTainted || isCarryFlagTainted)) fUnTaintROTATE(pTmgrTls);
    else 
    { 
        _LOGTAINT("RCR_IR " << len << " ");
        
        // R�cup�ration du marquage ou valeur du registre
        ObjectSource objSrcReg = (isSrcTainted) 
            ? ObjectSource(pTmgrTls->getRegisterTaint<len>(reg, regValue))
            : ObjectSource(len, regValue);
        
        // R�cup�ration du Carry Flag (marqu� ou valeur);
        ObjectSource objSrcCF = (isCarryFlagTainted) 
            ? ObjectSource(pTmgrTls->getTaintCarryFlag())
            : ObjectSource(1, (regGflagsValue >> CARRY_FLAG) & 1);

        // construction du r�sultat
        std::shared_ptr<TaintObject<len>> resultPtr = std::make_shared<TaintObject<len>>(
            X_RCR,
            objSrcReg,
            objSrcCF,
            ObjectSource(8, maskedDepl));

        // marquage flags
        fRCR(pTmgrTls, objSrcReg, objSrcCF, maskedDepl);

        // marquage destination
        pTmgrTls->updateTaintRegister<len>(reg, resultPtr);
    }
} // sRCR

template<UINT32 len> void ROTATE::sRCR_RM
    (THREADID tid, ADDRINT regCLValue, ADDRINT writeAddress, ADDRINT regGflagsValue ADDRESS_DEBUG)
{
    TaintManager_Thread *pTmgrTls = static_cast<TaintManager_Thread*>(PIN_GetThreadData(tlsKeyTaint, tid));

    bool isCountTainted     = pTmgrTls->isRegisterTainted<8>(REG_CL);
    bool isDestTainted      = pTmgrGlobal->isMemoryTainted<len>(writeAddress);
    bool isCarryFlagTainted = pTmgrTls->isCarryFlagTainted();
    
    if ( !(isDestTainted || isCountTainted || isCarryFlagTainted)) fUnTaintROTATE(pTmgrTls);
    // d�placement non marqu� (mais m�moire et/ou carry oui) => cas RCR_IM
    else if (!isCountTainted) 
    {
        // masquage du d�placement avant appel de RCR_IM
        UINT32 maskedDepl = (len == 64) ? (regCLValue & 0x3f) : (regCLValue & 0x1f);
        sRCR_IM<len>(tid, maskedDepl, writeAddress , regGflagsValue INSADDRESS); 
    }
    else // d�placement marqu�  
    {
        _LOGTAINT("RCR_RM, d�placement marqu�, destination" << ((isDestTainted) ? " marqu�" : " non marqu�"));
            
        // cr�ation de l'objet Source correspondant � la m�moire
        ObjectSource objSrcMem = (isDestTainted)
            ? ObjectSource(pTmgrGlobal->getMemoryTaint<len>(writeAddress)) 
            : ObjectSource(len, getMemoryValue<len>(writeAddress));

        // R�cup�ration du Carry Flag (marqu� ou valeur);
        ObjectSource objSrcCF = (isCarryFlagTainted) 
            ? ObjectSource(pTmgrTls->getTaintCarryFlag())
            : ObjectSource(1, (regGflagsValue >> CARRY_FLAG) & 1);

        // r�cup�ration du d�placement (marqu�)
        TaintBytePtr tbCountPtr = pTmgrTls->getRegisterTaint(REG_CL);

        // construction du r�sultat
        std::shared_ptr<TaintObject<len>> resultPtr = std::make_shared<TaintObject<len>>(
            X_RCR,
            objSrcMem,
            objSrcCF,
            ObjectSource(tbCountPtr));

        // marquage flags
        fRCR(pTmgrTls, objSrcMem, tbCountPtr);
        // marquage destination
        pTmgrGlobal->updateMemoryTaint<len>(writeAddress, resultPtr);
    }
} // sRCR_RM

template<UINT32 len> void ROTATE::sRCR_RR
    (THREADID tid, ADDRINT regCLValue, REG reg, ADDRINT regValue, ADDRINT regGflagsValue  ADDRESS_DEBUG)
{
    TaintManager_Thread *pTmgrTls = static_cast<TaintManager_Thread*>(PIN_GetThreadData(tlsKeyTaint, tid));

    bool isCountTainted     = pTmgrTls->isRegisterTainted<8>(REG_CL);
    bool isDestTainted      = pTmgrTls->isRegisterTainted<len>(reg);
    bool isCarryFlagTainted = pTmgrTls->isCarryFlagTainted();
    
    if ( !(isDestTainted || isCountTainted || isCarryFlagTainted)) fUnTaintROTATE(pTmgrTls);
    // d�placement non marqu� (mais registre oui) => cas RCL_IR
    else if (!isCountTainted)
    {
        // masquage du d�placement avant appel de RCL_IR
        UINT32 maskedDepl = (len == 64) ? (regCLValue & 0x3f) : (regCLValue & 0x1f);
        sRCR_IR<len>(tid, maskedDepl, reg, regValue, regGflagsValue INSADDRESS); 
    }
    else // d�placement marqu� 
    {
        _LOGTAINT("RCR_RR, d�placement marqu�, destination" << ((isDestTainted) ? " marqu�" : " non marqu�"));
         
        // cr�ation de l'objet Source correspondant au registre cible
        ObjectSource objSrcReg = (isDestTainted)
            ? ObjectSource(pTmgrTls->getRegisterTaint<len>(reg, regValue))
            : ObjectSource(len, regValue);

        // R�cup�ration du Carry Flag (marqu� ou valeur);
        ObjectSource objSrcCF = (isCarryFlagTainted) 
            ? ObjectSource(pTmgrTls->getTaintCarryFlag())
            : ObjectSource(1, (regGflagsValue >> CARRY_FLAG) & 1);

        // r�cup�ration du d�placement (marqu�)
        TaintBytePtr tbCountPtr = pTmgrTls->getRegisterTaint(REG_CL);

        // cr�ation de l'objet resultat de l'op�ration
        std::shared_ptr<TaintObject<len>> resultPtr = std::make_shared<TaintObject<len>>(
            X_RCR,
            objSrcReg,
            ObjectSource(tbCountPtr)); 
     
        // marquage flags
        fRCR(pTmgrTls, objSrcReg, tbCountPtr);
        // marquage destination
        pTmgrTls->updateTaintRegister<len>(reg, resultPtr);
    }
} // sRCR_RR