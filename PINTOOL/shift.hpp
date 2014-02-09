/*********/
/** SHL **/
/*********/

template<UINT32 len> 
void SHIFT::sSHL_IM(THREADID tid, UINT32 maskedDepl, ADDRINT writeAddr ADDRESS_DEBUG) 
{  
    TaintManager_Thread *pTmgrTls = static_cast<TaintManager_Thread*>(PIN_GetThreadData(g_tlsKeyTaint, tid));
    
    // op�rande non marqu�e => d�marquage flags
    if (!pTmgrGlobal->isMemoryTainted<len>(writeAddr)) pTmgrTls->unTaintAllFlags();
    // d�placement >= taille destination => d�marquage flags et dest
    else if (maskedDepl >= len) 
    {  
        fUnTaintOZSAP(pTmgrTls); // d�marquage OZASP
            
        // marquage CF si dplt = � la taille de la destination
        // dans ce cas, le carryFlag contiendra le bit 0 de la source (si octet faible marqu�) 
        if ((maskedDepl == len) && pTmgrGlobal->isMemoryTainted<8>(writeAddr))
        {
            pTmgrTls->updateTaintCarryFlag(std::make_shared<TaintBit>(
                F_LSB,
                ObjectSource(pTmgrGlobal->getMemoryTaint<8>(writeAddr))));
        }
        else pTmgrTls->unTaintCarryFlag();

        pTmgrGlobal->unTaintMemory<len>(writeAddr);  // d�marquage destination
    }
    // dans les autres cas : marquage par SHL
    else 
    {
        _LOGTAINT("SHLIM " << len << " ");
        ObjectSource objSrcMem(pTmgrGlobal->getMemoryTaint<len>(writeAddr));

        // construction du r�sultat
        std::shared_ptr<TaintObject<len>> resultPtr = std::make_shared<TaintObject<len>>(
            X_SHL,
            objSrcMem,
            ObjectSource(8, maskedDepl));

        // marquage flags
        fSHL(pTmgrTls, resultPtr, objSrcMem, maskedDepl);

        // MARQUAGE DESTINATION traitement par intervalle de d�placement
        // 1) d�termination du nombre d'octets entiers d�plac�s
        UINT32 deplBytes = maskedDepl >> 3;
        // 2) cas des d�placements multiples de 8 bits (modulo 8 ou AND 7)
        if (!(maskedDepl & 0x7)) 
        {
            // 1ERE BOUCLE : octet fort marqu� avec oct (fort - deplBytes) et ainsi de suite en d�croissant les addresses
            // jusqu'� ce que adresse source = adresse de base (writeAddr)
            // pas de cr�ation d'objet, juste un d�placement
            ADDRINT toAddr   = writeAddr + (len >> 3) ; // octet fort +1 (1er octet de destination)
            ADDRINT fromAddr = toAddr - deplBytes;      // 1er octet de d�part + 1
            while (fromAddr > writeAddr) 
            {
                // ajustement des adresses de d�part et d'arriv�e
                --fromAddr; --toAddr;
                // d�placement de l'octet "faible" (from) vers l'octet "fort" (to)
                if (pTmgrGlobal->isMemoryTainted<8>(fromAddr))
                {
                    pTmgrGlobal->updateMemoryTaint<8>(toAddr, pTmgrGlobal->getMemoryTaint<8>(fromAddr));
                }
                else pTmgrGlobal->unTaintMemory<8>(toAddr); 
            }
            
            // 2EME BOUCLE : demarquage des 'deplBytes' octets faibles
            for (ADDRINT lastAddr = writeAddr + deplBytes ; writeAddr < lastAddr ; ++writeAddr)
            {
                pTmgrGlobal->unTaintMemory<8>(writeAddr);
            }
        }
        // 3) cas g�n�ral : marquage destination, puis demarquage octets faibles en fonction de l'intervalle de d�placement
        else 
        {  
            pTmgrGlobal->updateMemoryTaint<len>(writeAddr, resultPtr);
            for (ADDRINT lastAddr = writeAddr + deplBytes ; writeAddr < lastAddr ; ++writeAddr)
            {
                pTmgrGlobal->unTaintMemory<8>(writeAddr); 
            }
        }
    }
} // sSHL_IM

template<UINT32 len> 
void SHIFT::sSHL_IR(THREADID tid, UINT32 maskedDepl, REG reg, ADDRINT regValue ADDRESS_DEBUG) 
{  
    TaintManager_Thread *pTmgrTls = static_cast<TaintManager_Thread*>(PIN_GetThreadData(g_tlsKeyTaint, tid));
    
    // op�rande non marqu�e => d�marquage flags
    if (!pTmgrTls->isRegisterTainted<len>(reg)) pTmgrTls->unTaintAllFlags();
    // d�placement >= taille destination => d�marquage flags et dest 
    else if (maskedDepl >= len) 
    {  
        fUnTaintOZSAP(pTmgrTls); // d�marquage OZASP
            
        // marquage CF si dplt = � la taille de la destination
        // dans ce cas, le carryFlag contiendra le bit 0 de la source (si octet faible marqu�)
        REGINDEX regIndex = getRegIndex(reg);
        if ((maskedDepl == len) && pTmgrTls->isRegisterPartTainted(regIndex, 0))
        {
            pTmgrTls->updateTaintCarryFlag(std::make_shared<TaintBit>(
                F_LSB,
                ObjectSource(pTmgrTls->getRegisterPartTaint(regIndex, 0))));
        }
        else pTmgrTls->unTaintCarryFlag();

        pTmgrTls->unTaintRegister<len>(reg);  // d�marquage destination
    }
    // dans les autres cas : marquage par SHL
    else 
    {
        _LOGTAINT("SHLIR " << len << " ");
        ObjectSource objSrcReg(pTmgrTls->getRegisterTaint<len>(reg, regValue));
        REGINDEX regIndex = getRegIndex(reg);

        // construction du r�sultat
        std::shared_ptr<TaintObject<len>> resultPtr = std::make_shared<TaintObject<len>>(
            X_SHL,
            objSrcReg,
            ObjectSource(8, maskedDepl));

        // marquage flags
        fSHL(pTmgrTls, resultPtr, objSrcReg, maskedDepl);

        // MARQUAGE DESTINATION traitement par intervalle de d�placement
        // 1) d�termination du nombre d'octets entiers d�plac�s
        UINT32 deplBytes = maskedDepl >> 3;
        // 2) cas des d�placements multiples de 8 bits
        if (!(maskedDepl & 0x7)) 
        {
            // traitement par deux boucles while, avec comme index
            // de d�part l'index haut, pour �viter les recoupements

            // 1ERE BOUCLE : octet fort marqu� avec oct (fort - deplBytes) et ainsi de suite en d�croissant les indexs
            // jusqu'� ce que index source = 0 ; pas de cr�ation d'objet, juste un d�placement
            for (UINT32 regPartFrom = (len >> 3) - deplBytes, regPartTo = (len >> 3) ; regPartFrom >= 0 ; --regPartFrom, --regPartTo)
            {
                // d�placement de l'octet "faible" (from) vers l'octet "fort" (to)
                if (pTmgrTls->isRegisterPartTainted(regIndex, regPartFrom))
                {
                    pTmgrTls->updateTaintRegisterPart(regIndex, regPartTo, 
                        pTmgrTls->getRegisterPartTaint(regIndex, regPartFrom));
                }
                else pTmgrTls->unTaintRegisterPart(regIndex, regPartTo);
            }

            // 2EME BOUCLE : demarquage indexs [0 ; deplBytes - 1]
            for (UINT32 regPart = 0 ; regPart < deplBytes ; ++regPart)
            {
                pTmgrTls->unTaintRegisterPart(regIndex, regPart);
            }
        }
        // 3) cas g�n�ral : marquage destination, puis demarquage octets faibles en fonction de l'intervalle de d�placement
        else 
        {  
            pTmgrTls->updateTaintRegister<len>(reg, resultPtr);  
            for (UINT32 regPart = 0 ; regPart < deplBytes ; ++regPart)
            {
                pTmgrTls->unTaintRegisterPart(regIndex, regPart);
            }
        }
    }
} // sSHL_IR

template<UINT32 len> 
void SHIFT::sSHL_RM(THREADID tid, ADDRINT regCLValue, ADDRINT writeAddress ADDRESS_DEBUG) 
{
    TaintManager_Thread *pTmgrTls = static_cast<TaintManager_Thread*>(PIN_GetThreadData(g_tlsKeyTaint, tid));
    
    bool isCountTainted = pTmgrTls->isRegisterTainted<8>(REG_CL);
    bool isDestTainted  = pTmgrGlobal->isMemoryTainted<len>(writeAddress);
    
    if ( !(isDestTainted || isCountTainted)) pTmgrTls->unTaintAllFlags();
    // d�placement non marqu� (mais m�moire oui) => cas SHL_IM
    else if (!isCountTainted) 
    {
        // masquage du d�placement avant appel de SHL_IM
        UINT32 maskDepl = (len == 64) ? (regCLValue & 0x3f) : (regCLValue & 0x1f);
        sSHL_IM<len>(tid, maskDepl, writeAddress INSADDRESS); 
    }
    // forc�ment d�placement marqu�
    else
    {
        _LOGTAINT("SHL_RM, d�placement marque, source " << ((isDestTainted) ? "marquee" : "non marquee"));
        
        // r�cup�ration du d�placement marqu�
        ObjectSource objTbCount(pTmgrTls->getRegisterTaint(REG_CL));
        // cr�ation de l'objet correspondant � la m�moire shift�e
        ObjectSource objSrcMem = (isDestTainted)
            ? ObjectSource(pTmgrGlobal->getMemoryTaint<len>(writeAddress))
            : ObjectSource(len, getMemoryValue<len>(writeAddress));
       
        // cr�ation de l'objet resultat de l'op�ration
        std::shared_ptr<TaintObject<len>> resultPtr = std::make_shared<TaintObject<len>>(
            X_SHL,
            objSrcMem,
            objTbCount);    
            
        // marquage flags
        fSHL(pTmgrTls, resultPtr, objSrcMem, objTbCount);
        // marquage destination
        pTmgrGlobal->updateMemoryTaint<len>(writeAddress, resultPtr);
    }
} // sSHL_RM

template<UINT32 len> 
void SHIFT::sSHL_RR(THREADID tid, ADDRINT regCLValue, REG reg, ADDRINT regValue ADDRESS_DEBUG) 
{
    TaintManager_Thread *pTmgrTls = static_cast<TaintManager_Thread*>(PIN_GetThreadData(g_tlsKeyTaint, tid));
    
    bool isCountTainted = pTmgrTls->isRegisterTainted<8>(REG_CL);
    bool isDestTainted  = pTmgrTls->isRegisterTainted<len>(reg);
    
    if ( !(isDestTainted || isCountTainted) ) pTmgrTls->unTaintAllFlags();
    // d�placement non marqu� (mais registre oui) => cas SHL_IR
    else if (!isCountTainted)
    {
        // masquage du d�placement avant appel de SHL_IR
        UINT32 maskDepl = (len == 64) ? (regCLValue & 0x3f) : (regCLValue & 0x1f);
        sSHL_IR<len>(tid, maskDepl, reg, regValue INSADDRESS); 
    }
    // forc�ment d�placement marqu�
    else 
    {
        _LOGTAINT("SHL_RR, d�placement marque, source " << ((isDestTainted) ? "marquee" : "non marquee"));
        
        // r�cup�ration du d�placement marqu�
        ObjectSource objTbCount(pTmgrTls->getRegisterTaint(REG_CL));
        // cr�ation de l'objet Source correspondant au registre shift�
        ObjectSource objSrcReg = (isDestTainted)
            ? ObjectSource(pTmgrTls->getRegisterTaint<len>(reg, regValue))
            : ObjectSource(len, regValue);
        
        // cr�ation de l'objet resultat de l'op�ration
        std::shared_ptr<TaintObject<len>> resultPtr = std::make_shared<TaintObject<len>>(
            X_SHL,
            objSrcReg,
            objTbCount);    
            
        // marquage flags
        fSHL(pTmgrTls, resultPtr, objSrcReg, objTbCount);
        // marquage destination
        pTmgrTls->updateTaintRegister<len>(reg, resultPtr);
    }
} // sSHL_RR

/*********/
/** SHR **/
/*********/

template<UINT32 len> 
void SHIFT::sSHR_IM(THREADID tid, UINT32 maskedDepl, ADDRINT writeAddr ADDRESS_DEBUG) 
{  
    TaintManager_Thread *pTmgrTls = static_cast<TaintManager_Thread*>(PIN_GetThreadData(g_tlsKeyTaint, tid));
    
    // op�rande non marqu�e => d�marquage flags
    if (!pTmgrGlobal->isMemoryTainted<len>(writeAddr)) pTmgrTls->unTaintAllFlags();
    // d�placement >= taille destination => d�marquage flags et dest
    else if (maskedDepl >= len) 
    {  
        fUnTaintOZSAP(pTmgrTls); // d�marquage OZASP
            
        // marquage CF si dplt = � la taille de la destination
        // dans ce cas, le carryFlag contiendra le MSB de la source (si octet fort marqu�)
        ADDRINT highAddress = writeAddr + (len >> 3) - 1;
        if ((maskedDepl == len) && pTmgrGlobal->isMemoryTainted<8>(highAddress))
        {
            pTmgrTls->updateTaintCarryFlag(std::make_shared<TaintBit>(
                F_MSB,
                ObjectSource(pTmgrGlobal->getMemoryTaint<8>(highAddress))));
        }
        else pTmgrTls->unTaintCarryFlag();

        pTmgrGlobal->unTaintMemory<len>(writeAddr);  // d�marquage destination 
    }
    else // dans les autres cas : marquage par SHR
    {
        _LOGTAINT("SHRIM " << len << " ");
        ObjectSource objSrcMem(pTmgrGlobal->getMemoryTaint<len>(writeAddr));

        // construction du r�sultat
        std::shared_ptr<TaintObject<len>> resultPtr = std::make_shared<TaintObject<len>>(
            X_SHR,
            objSrcMem,
            ObjectSource(8, maskedDepl));

        // marquage flags
        fSHR(pTmgrTls, resultPtr, objSrcMem, maskedDepl);

        // MARQUAGE DESTINATION traitement par intervalle de d�placement
        // 1) d�termination du nombre d'octets entiers d�plac�s
        UINT32 deplBytes = maskedDepl >> 3;
        // 2) cas des d�placements multiples de 8 bits (modulo 8 ou AND 7)
        if (!(maskedDepl & 0x7))
        {
            // 1ERE BOUCLE : octet faible marqu� avec oct (faible + deplBytes) et ainsi de suite en accroissant les addresses
            // jusqu'� ce que adresse source = adresse haute (writeAddr + (len>>3) - 1
            ADDRINT fromAddr = writeAddr + deplBytes - 1;   // 1er octet source (-1)
            ADDRINT toAddr   = writeAddr - 1;               // 1er octet de destination (-1)
            ADDRINT highAddress = writeAddr + (len >> 3);   // derni�re adresse trait�e = adresse haute exclue

            while (fromAddr < highAddress)
            {
                // ajustement des adresses de d�part et d'arriv�e
                ++fromAddr; ++toAddr;
                // d�placement de l'octet "fort" (from) vers l'octet "faible" (to)
                if (pTmgrGlobal->isMemoryTainted<8>(fromAddr))
                {
                    pTmgrGlobal->updateMemoryTaint<8>(toAddr, pTmgrGlobal->getMemoryTaint<8>(fromAddr));
                }
                else pTmgrGlobal->unTaintMemory<8>(toAddr);
            }

            // 2EME BOUCLE : demarquage des 'deplBytes' octets forts: [wA + (len >> 3) - deplBytes; wA + (len >> 3) [
            for (ADDRINT unTaintAddr = highAddress - deplBytes ; unTaintAddr < highAddress ; ++unTaintAddr)
            {
                pTmgrGlobal->unTaintMemory<8>(unTaintAddr);
            }
        }
        // 3) cas g�n�ral : marquage destination, puis demarquage octets forts en fonction de l'intervalle de d�placement
        else 
        {           
            pTmgrGlobal->updateMemoryTaint<len>(writeAddr, resultPtr);
            ADDRINT highAddress = writeAddr + (len >> 3);   // derni�re adresse trait�e = adresse haute exclue
            for (ADDRINT unTaintAddr = highAddress - deplBytes ; unTaintAddr < highAddress ; ++unTaintAddr)
            {
                pTmgrGlobal->unTaintMemory<8>(unTaintAddr);
            }
        }
    }
} // sSHR_IM

template<UINT32 len> 
void SHIFT::sSHR_IR(THREADID tid, UINT32 maskedDepl, REG reg, ADDRINT regValue ADDRESS_DEBUG) 
{  
    TaintManager_Thread *pTmgrTls = static_cast<TaintManager_Thread*>(PIN_GetThreadData(g_tlsKeyTaint, tid));
    
    // op�rande non marqu�e => d�marquage flags
    if (!pTmgrTls->isRegisterTainted<len>(reg)) pTmgrTls->unTaintAllFlags();
    // d�placement >= taille destination => d�marquage flags et dest 
    else if (maskedDepl >= len) 
    {  
        fUnTaintOZSAP(pTmgrTls); // d�marquage OZASP
  
        // marquage CF si dplt = � la taille de la destination
        // dans ce cas, le carryFlag contiendra le MSB de la source (si octet fort marqu�)
        UINT32 highPart = (len >> 3) - 1;
        REGINDEX regIndex = getRegIndex(reg);
        if ((maskedDepl == len) && pTmgrTls->isRegisterPartTainted(regIndex, highPart))
        {
            pTmgrTls->updateTaintCarryFlag(std::make_shared<TaintBit>(
                F_MSB,
                ObjectSource(pTmgrTls->getRegisterPartTaint(regIndex, highPart))));
        }
        else pTmgrTls->unTaintCarryFlag();
        
        pTmgrTls->unTaintRegister<len>(reg);  // d�marquage destination
    }
    else // dans les autres cas : marquage par SHR
    {
        _LOGTAINT("SHRIR " << len << " ");
        ObjectSource objSrcReg(pTmgrTls->getRegisterTaint<len>(reg, regValue));
        REGINDEX regIndex = getRegIndex(reg);

        // construction du r�sultat
        std::shared_ptr<TaintObject<len>> resultPtr = std::make_shared<TaintObject<len>>(
            X_SHR,
            objSrcReg,
            ObjectSource(8, maskedDepl));

        // marquage flags
        fSHR(pTmgrTls, resultPtr, objSrcReg, maskedDepl);

        // MARQUAGE DESTINATION traitement par intervalle de d�placement
        // 1) d�termination du nombre d'octets entiers d�plac�s
        UINT32 deplBytes = maskedDepl >> 3;
        // 2) cas des d�placements multiples de 8 bits
        if (!(maskedDepl & 0x7)) 
        {
            // 1ERE BOUCLE : octet faible marqu� avec oct (faible + deplBytes) et ainsi de suite en accroissant les indexes
            // jusqu'� ce que index source = index fort (len>>3 - 1) ; pas de cr�ation d'objet, juste un d�placement
            UINT32 fromIndex = deplBytes;  // index source
            UINT32 toIndex = 0; // index de destination
                
            for (UINT32 regPartFrom = deplBytes, regPartTo = 0 ; regPartFrom < (len >> 3) ; ++regPartFrom, ++regPartTo) 
            {
                if (pTmgrTls->isRegisterPartTainted(regIndex, regPartFrom))
                {
                    pTmgrTls->updateTaintRegisterPart
                        (regIndex, regPartTo, pTmgrTls->getRegisterPartTaint(regIndex, regPartFrom));
                }
                else pTmgrTls->unTaintRegisterPart(regIndex, regPartTo);
            }

            // 2EME BOUCLE : demarquage des 'deplBytes' octets forts: [(len >> 3) - deplBytes; (len >> 3) [
            for (UINT32 regPart = (len >> 3) - deplBytes ; regPart < (len >> 3) ; ++regPart)
            {
                pTmgrTls->unTaintRegisterPart(regIndex, regPart);
            }
        }
        // 3) cas g�n�ral : marquage destination, puis demarquage octets forts en fonction de l'intervalle de d�placement
        else 
        {
            pTmgrTls->updateTaintRegister<len>(reg, resultPtr);
            for (UINT32 regPart = (len >> 3) - deplBytes ; regPart < (len >> 3) ; ++regPart)
            {
                pTmgrTls->unTaintRegisterPart(regIndex, regPart);
            }
        }
    }
} // sSHR_IR

template<UINT32 len> 
void SHIFT::sSHR_RM(THREADID tid, ADDRINT regCLValue, ADDRINT writeAddress ADDRESS_DEBUG) 
{
    TaintManager_Thread *pTmgrTls = static_cast<TaintManager_Thread*>(PIN_GetThreadData(g_tlsKeyTaint, tid));
    
    bool isCountTainted = pTmgrTls->isRegisterTainted<8>(REG_CL);
    bool isDestTainted  = pTmgrGlobal->isMemoryTainted<len>(writeAddress);
    
    if ( !(isDestTainted || isCountTainted)) pTmgrTls->unTaintAllFlags();
    // d�placement non marqu� (mais m�moire oui) => cas SHR_IM
    else if (!isCountTainted) 
    {
        // masquage du d�placement avant appel de SHR_IM
        UINT32 maskDepl = (len == 64) ? (regCLValue & 0x3f) : (regCLValue & 0x1f);
        sSHR_IM<len>(tid, maskDepl, writeAddress INSADDRESS); 
    }
    // forc�ment d�placement marqu�
    else
    {
        _LOGTAINT("SHR_RM, d�placement marque, source " << ((isDestTainted) ? "marquee" : "non marquee"));
        
        // r�cup�ration du d�placement marqu�
        ObjectSource objTbCount(pTmgrTls->getRegisterTaint(REG_CL));
        // cr�ation de l'objet correspondant � la m�moire shift�e
        ObjectSource objSrcMem = (isDestTainted)
            ? ObjectSource(pTmgrGlobal->getMemoryTaint<len>(writeAddress))
            : ObjectSource(len, getMemoryValue<len>(writeAddress));

        // cr�ation de l'objet resultat de l'op�ration
        std::shared_ptr<TaintObject<len>> resultPtr = std::make_shared<TaintObject<len>>(
            X_SHR,
            objSrcMem,
            objTbCount);    
            
        // marquage flags
        fSHR(pTmgrTls, resultPtr, objSrcMem, objTbCount);
        // marquage destination
        pTmgrGlobal->updateMemoryTaint<len>(writeAddress, resultPtr);
    }
} // sSHR_RM

template<UINT32 len> 
void SHIFT::sSHR_RR(THREADID tid, ADDRINT regCLValue, REG reg, ADDRINT regValue ADDRESS_DEBUG) 
{
    TaintManager_Thread *pTmgrTls = static_cast<TaintManager_Thread*>(PIN_GetThreadData(g_tlsKeyTaint, tid));
    
    bool isCountTainted = pTmgrTls->isRegisterTainted<8>(REG_CL);
    bool isDestTainted  = pTmgrTls->isRegisterTainted<len>(reg);
    
    if ( !(isDestTainted || isCountTainted) ) pTmgrTls->unTaintAllFlags();
    // d�placement non marqu� (mais registre oui) => cas SHR_IR
    else if (!isCountTainted)
    {
        // masquage du d�placement avant appel de SHR_IR
        UINT32 maskDepl = (len == 64) ? (regCLValue & 0x3f) : (regCLValue & 0x1f);
        sSHR_IR<len>(tid, maskDepl, reg, regValue INSADDRESS); 
    }
    // forc�ment d�placement marqu�
    else
    {
        _LOGTAINT("SHR_RR, d�placement marque, source " << ((isDestTainted) ? "marquee" : "non marquee"));
        
        // r�cup�ration du d�placement marqu�
        ObjectSource objTbCount(pTmgrTls->getRegisterTaint(REG_CL));
        // cr�ation de l'objet Source correspondant au registre shift�
        ObjectSource objSrcReg = (isDestTainted)
            ? ObjectSource(pTmgrTls->getRegisterTaint<len>(reg, regValue))
            : ObjectSource(len, regValue);
        
        // cr�ation de l'objet resultat de l'op�ration
        std::shared_ptr<TaintObject<len>> resultPtr = std::make_shared<TaintObject<len>>(
            X_SHR,
            objSrcReg,
            objTbCount);    
            
        // marquage flags
        fSHR(pTmgrTls, resultPtr, objSrcReg, objTbCount);
        // marquage destination
        pTmgrTls->updateTaintRegister<len>(reg, resultPtr);
    }
} // sSHR_RR

/*********/
/** SAR **/
/*********/

template<UINT32 len> 
void SHIFT::sSAR_IM(THREADID tid, UINT32 maskedDepl, ADDRINT writeAddr ADDRESS_DEBUG) 
{  
    TaintManager_Thread *pTmgrTls = static_cast<TaintManager_Thread*>(PIN_GetThreadData(g_tlsKeyTaint, tid));
    
    // op�rande non marqu�e => d�marquage flags
    if (!pTmgrGlobal->isMemoryTainted<len>(writeAddr)) pTmgrTls->unTaintAllFlags();
    // d�placement >= taille destination => d�marquage flags et dest
    else if (maskedDepl >= len) 
    {  
        fUnTaintOZSAP(pTmgrTls); // d�marquage OZASP
            
        // marquage CF si dplt = � la taille de la destination
        // dans ce cas, le carryFlag contiendra le MSB de la source (si octet fort marqu�)
        ADDRINT highAddress = writeAddr + (len >> 3) - 1;
        if ((maskedDepl == len) && pTmgrGlobal->isMemoryTainted<8>(highAddress))
        {
            pTmgrTls->updateTaintCarryFlag(std::make_shared<TaintBit>(
                F_MSB,
                ObjectSource(pTmgrGlobal->getMemoryTaint<8>(highAddress))));
        }
        else pTmgrTls->unTaintCarryFlag();

        pTmgrGlobal->unTaintMemory<len>(writeAddr);  // d�marquage destination 
    }
    else // dans les autres cas : marquage par SAR
    {
        _LOGTAINT("SARIM " << len << " ");
        ObjectSource objSrcMem(pTmgrGlobal->getMemoryTaint<len>(writeAddr));

        // construction du r�sultat
        std::shared_ptr<TaintObject<len>> resultPtr = std::make_shared<TaintObject<len>>(
            X_SAR,
            objSrcMem,
            ObjectSource(8, maskedDepl));

        // marquage flags
        fSAR(pTmgrTls, resultPtr, objSrcMem, maskedDepl);

        // MARQUAGE DESTINATION 
        pTmgrGlobal->updateMemoryTaint<len>(writeAddr, resultPtr);
    }
} // sSAR_IM

template<UINT32 len> 
void SHIFT::sSAR_IR(THREADID tid, UINT32 maskedDepl, REG reg, ADDRINT regValue ADDRESS_DEBUG) 
{  
    TaintManager_Thread *pTmgrTls = static_cast<TaintManager_Thread*>(PIN_GetThreadData(g_tlsKeyTaint, tid));
    
    // op�rande non marqu�e => d�marquage flags
    if (!pTmgrTls->isRegisterTainted<len>(reg)) pTmgrTls->unTaintAllFlags();
    // d�placement >= taille destination => d�marquage flags et dest 
    else if (maskedDepl >= len) 
    {  
        fUnTaintOZSAP(pTmgrTls); // d�marquage OZASP
  
        // marquage CF si dplt = � la taille de la destination
        // dans ce cas, le carryFlag contiendra le MSB de la source (si octet fort marqu�)
        UINT32 highPart = (len >> 3) - 1;
        REGINDEX regIndex = getRegIndex(reg);
        if ((maskedDepl == len) && pTmgrTls->isRegisterPartTainted(regIndex, highPart))
        {
            pTmgrTls->updateTaintCarryFlag(std::make_shared<TaintBit>(
                F_MSB,
                ObjectSource(pTmgrTls->getRegisterPartTaint(regIndex, highPart))));
        }
        else pTmgrTls->unTaintCarryFlag();
        
        pTmgrTls->unTaintRegister<len>(reg);  // d�marquage destination
    }
    else // dans les autres cas : marquage par SAR
    {
        _LOGTAINT("SARIR " << len << " ");
        ObjectSource objSrcReg(pTmgrTls->getRegisterTaint<len>(reg, regValue));
        REGINDEX regIndex = getRegIndex(reg);

        // construction du r�sultat
        std::shared_ptr<TaintObject<len>> resultPtr = std::make_shared<TaintObject<len>>(
            X_SAR,
            objSrcReg,
            ObjectSource(8, maskedDepl));

        // marquage flags
        fSAR(pTmgrTls, resultPtr, objSrcReg, maskedDepl);

        // MARQUAGE DESTINATION 
        pTmgrTls->updateTaintRegister<len>(reg, resultPtr);
    }
} // sSAR_IR

template<UINT32 len> 
void SHIFT::sSAR_RM(THREADID tid, ADDRINT regCLValue, ADDRINT writeAddress ADDRESS_DEBUG) 
{
    TaintManager_Thread *pTmgrTls = static_cast<TaintManager_Thread*>(PIN_GetThreadData(g_tlsKeyTaint, tid));
    
    bool isCountTainted = pTmgrTls->isRegisterTainted<8>(REG_CL);
    bool isDestTainted  = pTmgrGlobal->isMemoryTainted<len>(writeAddress);
    
    if ( !(isDestTainted || isCountTainted)) pTmgrTls->unTaintAllFlags();
    // d�placement non marqu� (mais m�moire oui) => cas SAR_IM
    else if (!isCountTainted) 
    {
        // masquage du d�placement avant appel de SAR_IM
        UINT32 maskDepl = (len == 64) ? (regCLValue & 0x3f) : (regCLValue & 0x1f);
        sSAR_IM<len>(tid, maskDepl, writeAddress INSADDRESS); 
    }
    // forc�ment d�placement marqu�
    else
    {
        _LOGTAINT("SAR_RM, d�placement marque, source " << ((isDestTainted) ? "marquee" : "non marquee"));
        
        // r�cup�ration du d�placement marqu�
        ObjectSource objTbCount(pTmgrTls->getRegisterTaint(REG_CL));
        // cr�ation de l'objet correspondant � la m�moire shift�e
        ObjectSource objSrcMem = (isDestTainted)
            ? ObjectSource(pTmgrGlobal->getMemoryTaint<len>(writeAddress))
            : ObjectSource(len, getMemoryValue<len>(writeAddress));

        // cr�ation de l'objet resultat de l'op�ration
        std::shared_ptr<TaintObject<len>> resultPtr = std::make_shared<TaintObject<len>>(
            X_SAR,
            objSrcMem,
            objTbCount);    
            
        // marquage flags
        fSAR(pTmgrTls, resultPtr, objSrcMem, objTbCount);
        // marquage destination
        pTmgrGlobal->updateMemoryTaint<len>(writeAddress, resultPtr);
    }
} // sSAR_RM

template<UINT32 len> 
void SHIFT::sSAR_RR(THREADID tid, ADDRINT regCLValue, REG reg, ADDRINT regValue ADDRESS_DEBUG) 
{
    TaintManager_Thread *pTmgrTls = static_cast<TaintManager_Thread*>(PIN_GetThreadData(g_tlsKeyTaint, tid));
    
    bool isCountTainted = pTmgrTls->isRegisterTainted<8>(REG_CL);
    bool isDestTainted  = pTmgrTls->isRegisterTainted<len>(reg);
    
    if ( !(isDestTainted || isCountTainted) ) pTmgrTls->unTaintAllFlags();
    // d�placement non marqu� (mais registre oui) => cas SAR_IR
    else if (!isCountTainted)
    {
        // masquage du d�placement avant appel de SAR_IR
        UINT32 maskDepl = (len == 64) ? (regCLValue & 0x3f) : (regCLValue & 0x1f);
        sSAR_IR<len>(tid, maskDepl, reg, regValue INSADDRESS); 
    }
    // forc�ment d�placement marqu�
    else
    {
        _LOGTAINT("SAR_RR, d�placement marque, source " << ((isDestTainted) ? "marquee" : "non marquee"));
        
        // r�cup�ration du d�placement marqu�
        ObjectSource objTbCount(pTmgrTls->getRegisterTaint(REG_CL));
        // cr�ation de l'objet Source correspondant au registre shift�
        ObjectSource objSrcReg = (isDestTainted)
            ? ObjectSource(pTmgrTls->getRegisterTaint<len>(reg, regValue))
            : ObjectSource(len, regValue);
        
        // cr�ation de l'objet resultat de l'op�ration
        std::shared_ptr<TaintObject<len>> resultPtr = std::make_shared<TaintObject<len>>(
            X_SAR,
            objSrcReg,
            objTbCount);    
            
        // marquage flags
        fSAR(pTmgrTls, resultPtr, objSrcReg, objTbCount);
        // marquage destination
        pTmgrTls->updateTaintRegister<len>(reg, resultPtr);
    }
} // sSAR_RR

/**********/
/** SHLD **/
/**********/

// TODO : tester le marquage octet par octet du 'bit pattern' (seconde op�rande)
// afin d'affiner le d�marquage �ventuel de la destination. Valable surtout pour le marquage bit par bit

template<UINT32 len> void SHIFT::sSHLD_IM
    (THREADID tid, UINT32 maskedDepl, REG regSrc, ADDRINT regSrcValue, ADDRINT writeAddr ADDRESS_DEBUG)
{ 
    TaintManager_Thread *pTmgrTls = static_cast<TaintManager_Thread*>(PIN_GetThreadData(g_tlsKeyTaint, tid));
    
    bool isSrcDestTainted = pTmgrGlobal->isMemoryTainted<len>(writeAddr);
    bool isRegSrcTainted  = pTmgrTls->isRegisterTainted<len>(regSrc);
    // op�randes non marqu�es => d�marquage flags
    if (!(isSrcDestTainted || isRegSrcTainted))  pTmgrTls->unTaintAllFlags();
    // d�placement >= taille destination => d�marquage flags et dest
    else if (maskedDepl >= len) 
    {  
        fUnTaintOZSAP(pTmgrTls); // d�marquage OZASP
            
        // marquage CF si dplt = � la taille de la destination
        // dans ce cas, le carryFlag contiendra le bit 0 de la source (si octet faible marqu�) 
        if ((maskedDepl == len) && pTmgrGlobal->isMemoryTainted<8>(writeAddr))
        {
            pTmgrTls->updateTaintCarryFlag(std::make_shared<TaintBit>(
                F_LSB,
                ObjectSource(pTmgrGlobal->getMemoryTaint<8>(writeAddr))));
        }
        else pTmgrTls->unTaintCarryFlag();

        pTmgrGlobal->unTaintMemory<len>(writeAddr);  // d�marquage destination
    }
    // dans les autres cas : marquage par SHL
    else 
    {
        _LOGTAINT("SHLDIM " << len << " ");

        ObjectSource objMemSrcDest = (isSrcDestTainted) 
            ? ObjectSource(pTmgrGlobal->getMemoryTaint<len>(writeAddr))
            : ObjectSource(len, getMemoryValue<len>(writeAddr));
        ObjectSource objRegSrc = (isRegSrcTainted)
            ? ObjectSource(pTmgrTls->getRegisterTaint<len>(regSrc, regSrcValue))
            : ObjectSource(len, regSrcValue);
        
        // concat�nation de la source et du bit pattern
        TaintObject<(2*len)> concatenatedSrc(CONCAT, objMemSrcDest, objRegSrc);

        // d�placement avec SHL sur (len*2) bits
        TaintObject<(2*len)> shiftOperation(X_SHL,
            ObjectSource(std::make_shared<TaintObject<(2*len)>>(concatenatedSrc)),
            ObjectSource(8, maskedDepl));

        // construction du r�sultat : extraction de la partie forte de shiftOperation
        std::shared_ptr<TaintObject<len>> resultPtr = std::make_shared<TaintObject<len>>(
            EXTRACT,
            ObjectSource(std::make_shared<TaintObject<(2*len)>>(shiftOperation)),
            ObjectSource(8, 1)); // extract de longueur 'len' : 0 = partie faible, 1 = partie forte 

        // marquage flags; SEUL la source originale (ici m�moire) est utilis�e pour le marquage des flags
        // cf impl�mentation de fSHL (le 'bit pattern' est inutile)
        fSHLD(pTmgrTls, resultPtr, objMemSrcDest, maskedDepl);

        // MARQUAGE DESTINATION
        pTmgrGlobal->updateMemoryTaint<len>(writeAddr, resultPtr);
    }
} // sSHLD_IM

template<UINT32 len> void SHIFT::sSHLD_IR
    (THREADID tid, UINT32 maskedDepl, REG regSrc, ADDRINT regSrcValue, REG regSrcDest, ADDRINT regSrcDestValue ADDRESS_DEBUG)
{ 
    TaintManager_Thread *pTmgrTls = static_cast<TaintManager_Thread*>(PIN_GetThreadData(g_tlsKeyTaint, tid));
    
    bool isSrcDestTainted = pTmgrTls->isRegisterTainted<len>(regSrcDest);
    bool isRegSrcTainted  = pTmgrTls->isRegisterTainted<len>(regSrc);
    // op�randes non marqu�es => d�marquage flags
    if (!(isSrcDestTainted || isRegSrcTainted))  pTmgrTls->unTaintAllFlags();
    // d�placement >= taille destination => d�marquage flags et dest 
    else if (maskedDepl >= len) 
    {  
        fUnTaintOZSAP(pTmgrTls); // d�marquage OZASP
            
        // marquage CF si dplt = � la taille de la destination
        // dans ce cas, le carryFlag contiendra le bit 0 de la source (si octet faible marqu�)
        REGINDEX regSrcDestIndex = getRegIndex(regSrcDest);
        if ((maskedDepl == len) && pTmgrTls->isRegisterPartTainted(regSrcDestIndex, 0))
        {
            pTmgrTls->updateTaintCarryFlag(std::make_shared<TaintBit>(
                F_LSB,
                ObjectSource(pTmgrTls->getRegisterPartTaint(regSrcDestIndex, 0))));
        }
        else pTmgrTls->unTaintCarryFlag();

        pTmgrTls->unTaintRegister<len>(regSrcDest);  // d�marquage destination
    }
    // dans les autres cas : marquage par SHL
    else
    {
        _LOGTAINT("SHLD_IR " << len << " ");
               
        ObjectSource objRegSrcDest = (isSrcDestTainted) 
            ? ObjectSource(pTmgrTls->getRegisterTaint<len>(regSrcDest, regSrcDestValue))
            : ObjectSource(len, regSrcDestValue);
        ObjectSource objRegSrc = (isRegSrcTainted)
            ? ObjectSource(pTmgrTls->getRegisterTaint<len>(regSrc, regSrcValue))
            : ObjectSource(len, regSrcValue);
        
        // concat�nation de la source et du bit pattern
        TaintObject<(2*len)> concatenatedSrc(CONCAT, objRegSrcDest, objRegSrc);

        // d�placement avec SHL sur (len*2) bits
        TaintObject<(2*len)> shiftOperation(X_SHL,
            ObjectSource(std::make_shared<TaintObject<(2*len)>>(concatenatedSrc)),
            ObjectSource(8, maskedDepl));

        // construction du r�sultat : extraction de la partie forte de shiftOperation
        std::shared_ptr<TaintObject<len>> resultPtr = std::make_shared<TaintObject<len>>(
            EXTRACT,
            ObjectSource(std::make_shared<TaintObject<(2*len)>>(shiftOperation)),
            ObjectSource(8, 1)); // extract de longueur 'len' : 0 = partie faible, 1 = partie forte 

        // marquage flags; SEUL la source originale (ici registre srcDest) est utilis�e pour le marquage des flags
        // cf impl�mentation de fSHL (le 'bit pattern' est inutile)
        fSHLD(pTmgrTls, resultPtr, objRegSrcDest, maskedDepl);

        // MARQUAGE DESTINATION
        pTmgrTls->updateTaintRegister<len>(regSrcDest, resultPtr);
    }
} // sSHLD_IR

template<UINT32 len> void SHIFT::sSHLD_RM
    (THREADID tid, ADDRINT regCLValue, REG regSrc, ADDRINT regSrcValue, ADDRINT writeAddress ADDRESS_DEBUG)
{ 
    TaintManager_Thread *pTmgrTls = static_cast<TaintManager_Thread*>(PIN_GetThreadData(g_tlsKeyTaint, tid));
    
    bool isCountTainted   = pTmgrTls->isRegisterTainted<8>(REG_CL);
    bool isSrcDestTainted = pTmgrGlobal->isMemoryTainted<len>(writeAddress);
    bool isRegSrcTainted  = pTmgrTls->isRegisterTainted<len>(regSrc);
    
    if ( !(isCountTainted || isSrcDestTainted || isRegSrcTainted)) pTmgrTls->unTaintAllFlags();
    // d�placement non marqu� (mais m�moire oui) => cas SHLD_IM
    else if (!isCountTainted) 
    {
        // masquage du d�placement avant appel de SHLD_IM
        UINT32 maskDepl = (len == 64) ? (regCLValue & 0x3f) : (regCLValue & 0x1f);
        sSHLD_IM<len>(tid, maskDepl, regSrc, regSrcValue, writeAddress INSADDRESS); 
    }
    else // d�placement marqu�. M�moire et Bit Pattern marqu�s ou non  
    {
        _LOGTAINT("SHLD_RM " << len << " ");
        
        ObjectSource objMemSrcDest = (isSrcDestTainted) 
            ? ObjectSource(pTmgrGlobal->getMemoryTaint<len>(writeAddress))
            : ObjectSource(len, getMemoryValue<len>(writeAddress));
        ObjectSource objRegSrc = (isRegSrcTainted)
            ? ObjectSource(pTmgrTls->getRegisterTaint<len>(regSrc, regSrcValue))
            : ObjectSource(len, regSrcValue);

        // concat�nation de la source et du bit pattern
        TaintObject<(2*len)> concatenatedSrc(CONCAT, objMemSrcDest, objRegSrc);
        // r�cup�ration du d�placement marqu�
        ObjectSource objTbCount(pTmgrTls->getRegisterTaint(REG_CL));

        // d�placement avec SHL sur (len*2) bits, d�placement marqu�
        TaintObject<(2*len)> shiftOperation(X_SHL,
            ObjectSource(std::make_shared<TaintObject<(2*len)>>(concatenatedSrc)),
            objTbCount);

        // construction du r�sultat : extraction de la partie forte de shiftOperation
        std::shared_ptr<TaintObject<len>> resultPtr = std::make_shared<TaintObject<len>>(
            EXTRACT,
            ObjectSource(std::make_shared<TaintObject<(2*len)>>(shiftOperation)),
            ObjectSource(8, 1)); // extract de longueur 'len' : 0 = partie faible, 1 = partie forte 

        // marquage flags; SEUL la source originale (ici m�moire) est utilis�e pour le marquage des flags
        // cf impl�mentation de fSHL (le 'bit pattern' est inutile)
        fSHLD(pTmgrTls, resultPtr, objMemSrcDest, objTbCount);

        // MARQUAGE DESTINATION
        pTmgrGlobal->updateMemoryTaint<len>(writeAddress, resultPtr);
    }
} // sSHLD_RM

template<UINT32 len> void SHIFT::sSHLD_RR
    (THREADID tid, ADDRINT regCLValue, REG regSrc, ADDRINT regSrcValue, REG regSrcDest, ADDRINT regSrcDestValue ADDRESS_DEBUG)
{ 
    TaintManager_Thread *pTmgrTls = static_cast<TaintManager_Thread*>(PIN_GetThreadData(g_tlsKeyTaint, tid));
    
    bool isCountTainted   = pTmgrTls->isRegisterTainted<8>(REG_CL);
    bool isSrcDestTainted = pTmgrTls->isRegisterTainted<len>(regSrcDest);
    bool isRegSrcTainted  = pTmgrTls->isRegisterTainted<len>(regSrc);
    
    if ( !(isCountTainted || isSrcDestTainted || isRegSrcTainted)) pTmgrTls->unTaintAllFlags();
    // d�placement non marqu� (mais m�moire oui) => cas SHLD_IR
    else if (!isCountTainted) 
    {
        // masquage du d�placement avant appel de SHLD_IR
        UINT32 maskDepl = (len == 64) ? (regCLValue & 0x3f) : (regCLValue & 0x1f);
        sSHLD_IR<len>(tid, maskDepl, regSrc, regSrcValue, regSrcDest, regSrcDestValue INSADDRESS); 
    }
    else // d�placement marqu� ; Registre et Bit Pattern marqu�s ou non  
    {
        _LOGTAINT("SHLD_RR " << len << " ");
        
        // r�cup�ration du d�placement marqu�
        ObjectSource objTbCount(pTmgrTls->getRegisterTaint(REG_CL));

        ObjectSource objRegSrcDest = (isSrcDestTainted) 
            ? ObjectSource(pTmgrTls->getRegisterTaint<len>(regSrcDest, regSrcDestValue))
            : ObjectSource(len, regSrcDestValue);
        ObjectSource objRegSrc = (isRegSrcTainted)
            ? ObjectSource(pTmgrTls->getRegisterTaint<len>(regSrc, regSrcValue))
            : ObjectSource(len, regSrcValue);
        
        // concat�nation de la source et du bit pattern
        TaintObject<(2*len)> concatenatedSrc(CONCAT, objRegSrcDest, objRegSrc);

        // d�placement avec SHL sur (len*2) bits, d�placement marqu�
        TaintObject<(2*len)> shiftOperation(X_SHL,
            ObjectSource(std::make_shared<TaintObject<(2*len)>>(concatenatedSrc)),
            objTbCount);

        // construction du r�sultat : extraction de la partie forte de shiftOperation
        std::shared_ptr<TaintObject<len>> resultPtr = std::make_shared<TaintObject<len>>(
            EXTRACT,
            ObjectSource(std::make_shared<TaintObject<(2*len)>>(shiftOperation)),
            ObjectSource(8, 1)); // extract de longueur 'len' : 0 = partie faible, 1 = partie forte 

        // marquage flags; SEUL la source originale (ici registre srcDest) est utilis�e pour le marquage des flags
        // cf impl�mentation de fSHL (le 'bit pattern' est inutile)
        fSHLD(pTmgrTls, resultPtr, objRegSrcDest, objTbCount);

        // MARQUAGE DESTINATION
        pTmgrTls->updateTaintRegister<len>(regSrcDest, resultPtr);
    }
} // sSHLD_RR
 
/**********/
/** SHRD **/
/**********/

template<UINT32 len> void SHIFT::sSHRD_IM
    (THREADID tid, UINT32 maskedDepl, REG regSrc, ADDRINT regSrcValue, ADDRINT writeAddr ADDRESS_DEBUG)
{ 
    TaintManager_Thread *pTmgrTls = static_cast<TaintManager_Thread*>(PIN_GetThreadData(g_tlsKeyTaint, tid));
    
    bool isSrcDestTainted = pTmgrGlobal->isMemoryTainted<len>(writeAddr);
    bool isRegSrcTainted  = pTmgrTls->isRegisterTainted<len>(regSrc);
    // op�randes non marqu�es => d�marquage flags
    if (!(isSrcDestTainted || isRegSrcTainted))  pTmgrTls->unTaintAllFlags();
    // d�placement >= taille destination => d�marquage flags et dest
    else if (maskedDepl >= len) 
    {  
        fUnTaintOZSAP(pTmgrTls); // d�marquage OZASP
            
        // marquage CF si dplt = � la taille de la destination
        // dans ce cas, le carryFlag contiendra le MSB de la source (si octet fort marque)
        ADDRINT highAddress = writeAddr + (len >> 3) - 1;
        if ((maskedDepl == len) && pTmgrGlobal->isMemoryTainted<8>(highAddress))
        {
            pTmgrTls->updateTaintCarryFlag(std::make_shared<TaintBit>(
                F_MSB,
                ObjectSource(pTmgrGlobal->getMemoryTaint<8>(highAddress))));
        }
        else pTmgrTls->unTaintCarryFlag();

        pTmgrGlobal->unTaintMemory<len>(writeAddr);  // d�marquage destination 
    }
    else // dans les autres cas : marquage par SHR
    {
        _LOGTAINT("SHRD_IM " << len << " ");
            
        ObjectSource objMemSrcDest = (isSrcDestTainted) 
            ? ObjectSource(pTmgrGlobal->getMemoryTaint<len>(writeAddr))
            : ObjectSource(len, getMemoryValue<len>(writeAddr));
        ObjectSource objRegSrc = (isRegSrcTainted)
            ? ObjectSource(pTmgrTls->getRegisterTaint<len>(regSrc, regSrcValue))
            : ObjectSource(len, regSrcValue);
        
        // concat�nation de la source et du bit pattern
        TaintObject<(2*len)> concatenatedSrc(
            CONCAT, 
            objMemSrcDest, 
            objRegSrc);
        ObjectSource objConcatenatedSrc(std::make_shared<TaintObject<(2*len)>>(concatenatedSrc));

        // d�placement avec SHR sur (len*2) bits
        TaintObject<(2*len)> shiftOperation(X_SHR,
            objConcatenatedSrc,
            ObjectSource(8, maskedDepl));

        // construction du r�sultat : extraction de la partie forte de shiftOperation
        std::shared_ptr<TaintObject<len>> resultPtr = std::make_shared<TaintObject<len>>(
            EXTRACT,
            ObjectSource(std::make_shared<TaintObject<(2*len)>>(shiftOperation)),
            ObjectSource(8, 1)); // extract de longueur 'len' : 0 = partie faible, 1 = partie forte

        // marquage flags
        fSHRD(pTmgrTls, resultPtr, objConcatenatedSrc, maskedDepl);

        // MARQUAGE DESTINATION
        pTmgrGlobal->updateMemoryTaint<len>(writeAddr, resultPtr);
    }
} // sSHRD_IM

template<UINT32 len> void SHIFT::sSHRD_IR
    (THREADID tid, UINT32 maskedDepl, REG regSrc, ADDRINT regSrcValue, REG regSrcDest, ADDRINT regSrcDestValue ADDRESS_DEBUG)
{ 
    TaintManager_Thread *pTmgrTls = static_cast<TaintManager_Thread*>(PIN_GetThreadData(g_tlsKeyTaint, tid));
    
    bool isSrcDestTainted = pTmgrTls->isRegisterTainted<len>(regSrcDest);
    bool isRegSrcTainted  = pTmgrTls->isRegisterTainted<len>(regSrc);

    // op�randes non marqu�es => d�marquage flags
    if (!(isSrcDestTainted || isRegSrcTainted))  pTmgrTls->unTaintAllFlags();
    // d�placement >= taille destination => d�marquage flags et dest 
    else if (maskedDepl >= len) 
    {  
        fUnTaintOZSAP(pTmgrTls); // d�marquage OZASP
            
        // marquage CF si dplt = � la taille de la destination
        // dans ce cas, le carryFlag contiendra le bit 0 de la source (si octet faible marqu�)
        REGINDEX regSrcDestIndex = getRegIndex(regSrcDest);
        if ((maskedDepl == len) && pTmgrTls->isRegisterPartTainted(regSrcDestIndex, 0))
        {
            pTmgrTls->updateTaintCarryFlag(std::make_shared<TaintBit>(
                F_LSB,
                ObjectSource(pTmgrTls->getRegisterPartTaint(regSrcDestIndex, 0))));
        }
        else pTmgrTls->unTaintCarryFlag();

        pTmgrTls->unTaintRegister<len>(regSrcDest);  // d�marquage destination
    }
    else // dans les autres cas : marquage par SHR
    {
        _LOGTAINT("SHRD_IR " << len << " ");
            
        ObjectSource objRegSrcDest = (isSrcDestTainted) 
            ? ObjectSource(pTmgrTls->getRegisterTaint<len>(regSrcDest, regSrcDestValue))
            : ObjectSource(len, regSrcDestValue);
        ObjectSource objRegSrc = (isRegSrcTainted)
            ? ObjectSource(pTmgrTls->getRegisterTaint<len>(regSrc, regSrcValue))
            : ObjectSource(len, regSrcValue);
        
        // concat�nation de la source et du bit pattern
        TaintObject<(2*len)> concatenatedSrc(
            CONCAT, 
            objRegSrcDest, 
            objRegSrc);
        ObjectSource objConcatenatedSrc(std::make_shared<TaintObject<(2*len)>>(concatenatedSrc));

        // d�placement avec SHL sur (len*2) bits
        TaintObject<(2*len)> shiftOperation(X_SHR,
            objConcatenatedSrc,
            ObjectSource(8, maskedDepl));

        // construction du r�sultat : extraction de la partie forte de shiftOperation
        std::shared_ptr<TaintObject<len>> resultPtr = std::make_shared<TaintObject<len>>(
            EXTRACT,
            ObjectSource(std::make_shared<TaintObject<(2*len)>>(shiftOperation)),
            ObjectSource(8, 1)); // extract de longueur 'len' : 0 = partie faible, 1 = partie forte 

        // marquage flags
        fSHRD(pTmgrTls, resultPtr, objConcatenatedSrc, maskedDepl);

        // MARQUAGE DESTINATION
        pTmgrTls->updateTaintRegister<len>(regSrcDest, resultPtr);
    }
} // sSHRD_IR

template<UINT32 len> void SHIFT::sSHRD_RM
    (THREADID tid, ADDRINT regCLValue, REG regSrc, ADDRINT regSrcValue, ADDRINT writeAddress ADDRESS_DEBUG)
{ 
    TaintManager_Thread *pTmgrTls = static_cast<TaintManager_Thread*>(PIN_GetThreadData(g_tlsKeyTaint, tid));
    
    bool isCountTainted   = pTmgrTls->isRegisterTainted<8>(REG_CL);
    bool isSrcDestTainted = pTmgrGlobal->isMemoryTainted<len>(writeAddress);
    bool isRegSrcTainted  = pTmgrTls->isRegisterTainted<len>(regSrc);
    
    if ( !(isCountTainted || isSrcDestTainted || isRegSrcTainted)) pTmgrTls->unTaintAllFlags();
    // d�placement non marqu� (mais m�moire oui) => cas SHRD_IM
    else if (!isCountTainted) 
    {
        // masquage du d�placement avant appel de SHRD_IM
        UINT32 maskDepl = (len == 64) ? (regCLValue & 0x3f) : (regCLValue & 0x1f);
        sSHRD_IM<len>(tid, maskDepl, regSrc, regSrcValue, writeAddress INSADDRESS); 
    }
    else // d�placement marqu�. M�moire et Bit Pattern marqu�s ou non  
    {
        _LOGTAINT("SHRD_RM " << len << " ");
        
        ObjectSource objMemSrcDest = (isSrcDestTainted) 
            ? ObjectSource(pTmgrGlobal->getMemoryTaint<len>(writeAddress))
            : ObjectSource(len, getMemoryValue<len>(writeAddress));
        ObjectSource objRegSrc = (isRegSrcTainted)
            ? ObjectSource(pTmgrTls->getRegisterTaint<len>(regSrc, regSrcValue))
            : ObjectSource(len, regSrcValue);

        // concat�nation de la source et du bit pattern
        TaintObject<(2*len)> concatenatedSrc(
            CONCAT, 
            objMemSrcDest, 
            objRegSrc);
        ObjectSource objConcatenatedSrc(std::make_shared<TaintObject<(2*len)>>(concatenatedSrc));
        // r�cup�ration du d�placement marqu�
        ObjectSource objTbCount(pTmgrTls->getRegisterTaint(REG_CL));

        // d�placement avec SHR sur (len*2) bits, d�placement marqu�
        TaintObject<(2*len)> shiftOperation(X_SHR,
            objConcatenatedSrc,
            objTbCount);

        // construction du r�sultat : extraction de la partie forte de shiftOperation
        std::shared_ptr<TaintObject<len>> resultPtr = std::make_shared<TaintObject<len>>(
            EXTRACT,
            ObjectSource(std::make_shared<TaintObject<(2*len)>>(shiftOperation)),
            ObjectSource(8, 1)); // extract de longueur 'len' : 0 = partie faible, 1 = partie forte 

        // marquage flags
        fSHRD(pTmgrTls, resultPtr, objConcatenatedSrc, objTbCount);

        // MARQUAGE DESTINATION
        pTmgrGlobal->updateMemoryTaint<len>(writeAddress, resultPtr);
    }
} // sSHRD_RM

template<UINT32 len> void SHIFT::sSHRD_RR
    (THREADID tid, ADDRINT regCLValue, REG regSrc, ADDRINT regSrcValue, REG regSrcDest, ADDRINT regSrcDestValue ADDRESS_DEBUG)
{ 
    TaintManager_Thread *pTmgrTls = static_cast<TaintManager_Thread*>(PIN_GetThreadData(g_tlsKeyTaint, tid));
    
    bool isCountTainted   = pTmgrTls->isRegisterTainted<8>(REG_CL);
    bool isSrcDestTainted = pTmgrTls->isRegisterTainted<len>(regSrcDest);
    bool isRegSrcTainted  = pTmgrTls->isRegisterTainted<len>(regSrc);
    
    if ( !(isCountTainted || isSrcDestTainted || isRegSrcTainted)) pTmgrTls->unTaintAllFlags();
    // d�placement non marqu� (mais m�moire oui) => cas SHRD_IR
    else if (!isCountTainted) 
    {
        // masquage du d�placement avant appel de SHRD_IR
        UINT32 maskDepl = (len == 64) ? (regCLValue & 0x3f) : (regCLValue & 0x1f);
        sSHRD_IR<len>(tid, maskDepl, regSrc, regSrcValue, regSrcDest, regSrcDestValue INSADDRESS); 
    }
    else // d�placement marqu� ; Registre et Bit Pattern marqu�s ou non  
    {
        _LOGTAINT("SHRD_RR " << len << " ");

        ObjectSource objRegSrcDest = (isSrcDestTainted) 
            ? ObjectSource(pTmgrTls->getRegisterTaint<len>(regSrcDest, regSrcDestValue))
            : ObjectSource(len, regSrcDestValue);
        ObjectSource objRegSrc = (isRegSrcTainted)
            ? ObjectSource(pTmgrTls->getRegisterTaint<len>(regSrc, regSrcValue))
            : ObjectSource(len, regSrcValue);
        
        // concat�nation de la source et du bit pattern
        TaintObject<(2*len)> concatenatedSrc(
            CONCAT, 
            objRegSrcDest, 
            objRegSrc);
        ObjectSource objConcatenatedSrc(std::make_shared<TaintObject<(2*len)>>(concatenatedSrc));
        // r�cup�ration du d�placement marqu�
        ObjectSource objTbCount(pTmgrTls->getRegisterTaint(REG_CL));
        
        // d�placement avec SHR sur (len*2) bits, d�placement marqu�
        TaintObject<(2*len)> shiftOperation(X_SHR,
            objConcatenatedSrc,
            objTbCount);

        // construction du r�sultat : extraction de la partie forte de shiftOperation
        std::shared_ptr<TaintObject<len>> resultPtr = std::make_shared<TaintObject<len>>(
            EXTRACT,
            ObjectSource(std::make_shared<TaintObject<(2*len)>>(shiftOperation)),
            ObjectSource(8, 1)); // extract de longueur 'len' : 0 = partie faible, 1 = partie forte 

        // marquage flags
        fSHRD(pTmgrTls, resultPtr, objConcatenatedSrc, objTbCount);

        // MARQUAGE DESTINATION
        pTmgrTls->updateTaintRegister<len>(regSrcDest, resultPtr);
    }
} // sSHRD_RR
 