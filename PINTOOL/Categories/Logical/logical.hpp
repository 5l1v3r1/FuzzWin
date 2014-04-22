///////////////
///// AND /////
///////////////

// SIMULATE
template<UINT32 lengthInBits> void LOGICAL::sAND_IM(THREADID tid, ADDRINT value, ADDRINT writeAddress, ADDRINT insAddress) 
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);

    if (!pTmgrGlobal->isMemoryTainted<lengthInBits>(writeAddress)) pTmgrTls->unTaintAllFlags();
    else if (!value) // AND x, 0 = 0, donc d�marquage destination et flags
    { 
        pTmgrTls->unTaintAllFlags();
        pTmgrGlobal->unTaintMemory<lengthInBits>(writeAddress);
    }
    else 
    {
        _LOGTAINT(tid, insAddress, "andIM" + decstr(lengthInBits));

        // MARQUAGE DES FLAGS, d�pendant uniquement du r�sultat
        fTaintLOGICAL(pTmgrTls, MK_TAINT_OBJECT_PTR(
            X_AND,
            ObjectSource(pTmgrGlobal->getMemoryTaint<lengthInBits>(writeAddress)),
            ObjectSource(lengthInBits, value)));

        // MARQUAGE DE LA DESTINATION, par octet pour �viter surmarquage
        for (UINT32 byteNbr = 0 ; byteNbr < (lengthInBits >> 3) ; ++byteNbr, ++writeAddress)
        {
            // dest non marqu�e : ne rien faire
            if (!pTmgrGlobal->isMemoryTainted<8>(writeAddress)) continue;
        
            UINT32 value8bits = EXTRACTBYTE(value, byteNbr);

            // 1er cas : valeur = 0xff : aucun changement
            if (value8bits == 0xff) continue;      
            // 2eme cas : valeur nulle  => d�marquage (AND x, 0 = tjs 0)
            else if (!value8bits) pTmgrGlobal->unTaintMemory<8>(writeAddress);
            // sinon, marquage du r�sultat avec op�ration "AND"
            else 
            {
                pTmgrGlobal->updateMemoryTaint<8>(writeAddress, std::make_shared<TaintByte>(
                    X_AND,
                    ObjectSource(pTmgrGlobal->getMemoryTaint<8>(writeAddress)),  
                    ObjectSource(8, value8bits)));
            }
        }   
    }
} // sAND_IM

template<UINT32 lengthInBits> void LOGICAL::sAND_IR(THREADID tid, ADDRINT value, REG reg, ADDRINT regValue, ADDRINT insAddress) 
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);

    if (!pTmgrTls->isRegisterTainted<lengthInBits>(reg)) pTmgrTls->unTaintAllFlags();
    else if (!value)  // AND x, 0 = 0, donc d�marquage destination et flags
    {
        pTmgrTls->unTaintAllFlags();
        pTmgrTls->unTaintRegister<lengthInBits>(reg);
    }
    else 
    {
        _LOGTAINT(tid, insAddress, "andIR" + decstr(lengthInBits));

        // MARQUAGE DES FLAGS, d�pendant uniquement du r�sultat
        fTaintLOGICAL(pTmgrTls, MK_TAINT_OBJECT_PTR(
            X_AND,
            ObjectSource(pTmgrTls->getRegisterTaint<lengthInBits>(reg, regValue)),
            ObjectSource(lengthInBits, value)));

        // MARQUAGE DE LA DESTINATION
        REGINDEX regIndex = getRegIndex(reg);
        for (UINT32 regPart = 0 ; regPart < (lengthInBits >> 3) ; ++regPart) 
        {
            // dest non marqu�e : ne rien faire
            if (!pTmgrTls->isRegisterPartTainted(regIndex, regPart)) continue;
        
            UINT32 value8bits = EXTRACTBYTE(value, regPart);

            // 1er cas : valeur = 0xff :  aucun changement
            if (value8bits == 0xff) continue;  

            // 2eme cas : valeur nulle  => d�marquage (AND x, 0 = tjs 0)
            else if (!value8bits) pTmgrTls->unTaintRegisterPart(regIndex, regPart);

            // sinon, marquage du r�sultat avec op�ration "AND"
            else 
            { 
                pTmgrTls->updateTaintRegisterPart(regIndex, regPart, std::make_shared<TaintByte>(
                    X_AND,
                    ObjectSource(pTmgrTls->getRegisterPartTaint(regIndex, regPart)),  
                    ObjectSource(8, value8bits)));
            }
        }
    }
} // sAND_IR

template<UINT32 lengthInBits> void LOGICAL::sAND_RM
    (THREADID tid, REG regSrc, ADDRINT srcValue, ADDRINT writeAddress, ADDRINT insAddress)
{   
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);

    ADDRINT destValue = getMemoryValue<lengthInBits>(writeAddress);

    // marquage de la totalit� de la destination et de la source
    bool isDestTainted = pTmgrGlobal->isMemoryTainted<lengthInBits>(writeAddress);
    bool isSrcTainted  = pTmgrTls->isRegisterTainted<lengthInBits>(regSrc);
    
    // cas particuliers du AND (cf tableau)
    //if (!isDestTainted && (!isSrcTainted || !destValue)) pTmgr->unTaintAllFlags(); => 5 op�rations
    if (!(isDestTainted || (isSrcTainted && (destValue != 0)))) pTmgrTls->unTaintAllFlags(); // 3 op�rations
    else if ( !(isSrcTainted || (srcValue != 0))) 
    {
        pTmgrTls->unTaintAllFlags();
        if (isDestTainted) pTmgrGlobal->unTaintMemory<lengthInBits>(writeAddress);
    }
    else 
    { 
        // dans tous les autres cas, marquage d'abord des flags puis de la destination octet par octet
        _LOGTAINT(tid, insAddress, "andRM" + decstr(lengthInBits));

        // MARQUAGE DES FLAGS, d�pendant uniquement du r�sultat 

        // source 1 : source
        ObjectSource objSrc = (isSrcTainted)
            ? ObjectSource(pTmgrTls->getRegisterTaint<lengthInBits>(regSrc, srcValue)) 
            : ObjectSource(lengthInBits, srcValue);
        
        // source 2 : source/destination
        ObjectSource objSrcDest = (isDestTainted)
            ? ObjectSource(pTmgrGlobal->getMemoryTaint<lengthInBits>(writeAddress)) 
            : ObjectSource(lengthInBits, destValue);

        fTaintLOGICAL(pTmgrTls, MK_TAINT_OBJECT_PTR(
            X_AND,
            objSrc,
            objSrcDest));

        // MARQUAGE DE LA DESTINATION, octet par octet
        // plusieurs cas selon le marquage des op�randes, cf tableau AND
        
        REGINDEX regSrcIndex = getRegIndex(regSrc);
        for (UINT32 byteNbr = 0 ; byteNbr < (lengthInBits >> 3) ; ++byteNbr, ++writeAddress) 
        {   
            UINT32 src8bitsValue    = EXTRACTBYTE(srcValue, byteNbr);
            UINT32 dest8bitsValue   = EXTRACTBYTE(destValue, byteNbr);
            bool isSrc8bitsTainted  = pTmgrTls->isRegisterPartTainted(regSrcIndex, byteNbr);
            bool isDest8bitsTainted = pTmgrGlobal->isMemoryTainted<8>(writeAddress);

            // CAS 1 : destination et sources non marqu�es => continuer pour prochain octet
            if (!(isDest8bitsTainted || isSrc8bitsTainted)) continue;

            // CAS 2 : destination non marqu�e (et donc source marqu�e, sinon cas 1)
            else if (!isDest8bitsTainted) 
            {
                // cas 2.1 : dest = 0 ; (AND 0, src = 0), donc continuer (dest d�j� d�marqu�e)
                if (!dest8bitsValue) continue;
                // cas 2.2 : destination vaut 0xff ; (AND 0xff, src) �quivaut � MOV dest, src
                else if (dest8bitsValue == 0xff)
                {
                    pTmgrGlobal->updateMemoryTaint<8>(writeAddress, std::make_shared<TaintByte>(
                        X_ASSIGN,
                        ObjectSource(pTmgrTls->getRegisterPartTaint(regSrcIndex, byteNbr))));
                }
                // cas 2.3 : autres valeur de la destination => marquage via AND
                else 
                {
                    pTmgrGlobal->updateMemoryTaint<8>(writeAddress, std::make_shared<TaintByte>(
                        X_AND,
                        ObjectSource(pTmgrTls->getRegisterPartTaint(regSrcIndex, byteNbr)),
                        ObjectSource(8, dest8bitsValue)));            
                }
            }
            // CAS 3 : source non marqu�e (et donc destination marqu�e)
            else if (!isSrc8bitsTainted) 
            {
                // cas 3.1 : src vaut 0 ; (AND dest, 0) fera tjs 0, donc d�marquer destination
                if (!src8bitsValue) pTmgrGlobal->unTaintMemory<8>(writeAddress);
                // cas 3.2 : src vaut 0xff; (AND dest, 0xff) ne modifie pas dest => continuer
                else if (src8bitsValue == 0xff) continue;
                // cas 3.3 : autres valeurs de la source => marquage via AND
                else 
                {
                    pTmgrGlobal->updateMemoryTaint<8>(writeAddress, std::make_shared<TaintByte>(
                        X_AND,
                        ObjectSource(pTmgrGlobal->getMemoryTaint<8>(writeAddress)),
                        ObjectSource(8, src8bitsValue)));    
                }
            }
            // CAS 4 : source et destination marqu�es => marquage via AND
            else 
            {
                pTmgrGlobal->updateMemoryTaint<8>(writeAddress, std::make_shared<TaintByte>(
                    X_AND,
                    ObjectSource(pTmgrGlobal->getMemoryTaint<8>(writeAddress)),  
                    ObjectSource(pTmgrTls->getRegisterPartTaint(regSrcIndex, byteNbr)))); 
            }
        }   
    }
} //sAND_RM

template<UINT32 lengthInBits> void LOGICAL::sAND_MR
    (THREADID tid, ADDRINT readAddress, REG regSrcDest, ADDRINT destValue, ADDRINT insAddress) 
{   
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);

    ADDRINT srcValue = getMemoryValue<lengthInBits>(readAddress);

    // marquage de la totalit� de la destination et de la source
    bool isDestTainted = pTmgrTls->isRegisterTainted<lengthInBits>(regSrcDest);
    bool isSrcTainted  = pTmgrGlobal->isMemoryTainted<lengthInBits>(readAddress);
    
    // cas particuliers du AND (cf tableau)
    // if (!isDestTainted && (!isSrcTainted || !destValue)) pTmgr->unTaintAllFlags(); => 5 op�rations
    if (!(isDestTainted || (isSrcTainted && (destValue != 0)))) pTmgrTls->unTaintAllFlags(); // 3 op�rations
    else if ( !(isSrcTainted || (srcValue != 0))) 
    {
        pTmgrTls->unTaintAllFlags();
        if (isDestTainted) pTmgrTls->unTaintRegister<lengthInBits>(regSrcDest);
    }
    else 
    { 
        // sinon, marquage d'abord des flags puis de la destination octet par octet
        _LOGTAINT(tid, insAddress, "andMR" + decstr(lengthInBits));

        // MARQUAGE DES FLAGS, d�pendant uniquement du r�sultat 

        // source 1 : source
        ObjectSource objSrc = (isSrcTainted)
            ? ObjectSource(pTmgrGlobal->getMemoryTaint<lengthInBits>(readAddress))
            : ObjectSource(lengthInBits, srcValue);
        
        // source 2 : source/destination
        ObjectSource objSrcDest = (isDestTainted)
            ? ObjectSource(pTmgrTls->getRegisterTaint<lengthInBits>(regSrcDest, destValue)) 
            : ObjectSource(lengthInBits, destValue);
        
        fTaintLOGICAL(pTmgrTls, MK_TAINT_OBJECT_PTR(
            X_AND,
            objSrc,
            objSrcDest));

        // MARQUAGE DE LA DESTINATION, octet par octet
        // plusieurs cas selon le marquage des op�randes, cf tableau AND
        
        REGINDEX regSrcDestIndex = getRegIndex(regSrcDest);
        for (UINT32 byteNbr = 0 ; byteNbr < (lengthInBits >> 3) ; ++byteNbr, ++readAddress) 
        {   
            UINT32 src8bitsValue    = EXTRACTBYTE(srcValue, byteNbr);
            UINT32 dest8bitsValue   = EXTRACTBYTE(destValue, byteNbr);
            bool isSrc8bitsTainted  = pTmgrGlobal->isMemoryTainted<8>(readAddress);
            bool isDest8bitsTainted = pTmgrTls->isRegisterPartTainted(regSrcDestIndex, byteNbr);

            // CAS 1 : destination et sources non marqu�es => continuer pour prochain octet
            if (!(isDest8bitsTainted || isSrc8bitsTainted)) continue;  

            // CAS 2 : destination non marqu�e (et donc source marqu�e)
            else if (!isDest8bitsTainted) 
            {
                // cas 2.1 : dest = 0 ; (AND 0, src) fera tjs 0 => continuer (dest d�j� d�marqu�e)
                if (!dest8bitsValue) continue;
                // cas 2.2 : destination vaut 0xff ; (AND 0xff, src) �quivaut � MOV dest, src
                else if (dest8bitsValue == 0xff)  
                {
                    pTmgrTls->updateTaintRegisterPart(regSrcDestIndex, byteNbr, std::make_shared<TaintByte>(
                        X_ASSIGN,
                        ObjectSource(pTmgrGlobal->getMemoryTaint<8>(readAddress))));
                }
                // cas 2.3 : autres valeur de la destination => marquage via AND
                else 
                {
                    pTmgrTls->updateTaintRegisterPart(regSrcDestIndex, byteNbr, std::make_shared<TaintByte>(
                        X_AND,
                        ObjectSource(pTmgrGlobal->getMemoryTaint<8>(readAddress)),
                        ObjectSource(8, dest8bitsValue)));
                }
            }
            // CAS 3 : source non marqu�e (et donc destination marqu�e)
            else if (!isSrc8bitsTainted) 
            {
                // cas 3.1 : src vaut 0 ; (AND dest, 0) fera tjs 0, donc d�marquer destination
                if (!src8bitsValue) pTmgrTls->unTaintRegisterPart(regSrcDestIndex, byteNbr);
                // cas 3.2 : src vaut 0xff ; (AND dest, 0xff) ne modifie pas dest => continuer
                else if (src8bitsValue == 0xff) continue;               
                // cas 3.3 : autres valeur de la source => marquage via AND
                else 
                {
                    pTmgrTls->updateTaintRegisterPart(regSrcDestIndex, byteNbr, std::make_shared<TaintByte>(
                        X_AND,
                        ObjectSource(pTmgrTls->getRegisterPartTaint(regSrcDestIndex, byteNbr)),
                        ObjectSource(8, src8bitsValue)));        
                }                                   
            }
            // CAS 4 : source et destination marqu�es => marquage via AND
            else 
            {
                pTmgrTls->updateTaintRegisterPart(regSrcDestIndex, byteNbr, std::make_shared<TaintByte>(
                    X_AND,
                    ObjectSource(pTmgrTls->getRegisterPartTaint(regSrcDestIndex, byteNbr)),
                    ObjectSource(pTmgrGlobal->getMemoryTaint<8>(readAddress))));        
            }   
        }
    }
} // sAND_MR

template<UINT32 lengthInBits> void LOGICAL::sAND_RR
    (THREADID tid, REG regSrc, ADDRINT srcValue, REG regSrcDest, ADDRINT destValue, ADDRINT insAddress) 
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);

    // marquage de la totalit� de la destination et de la source
    bool isDestTainted = pTmgrTls->isRegisterTainted<lengthInBits>(regSrcDest);
    bool isSrcTainted  = pTmgrTls->isRegisterTainted<lengthInBits>(regSrc);
    
    // cas particuliers du AND (cf tableau)
    //if (!maskDest && (!maskSrc || !destValue)) pTmgr->unTaintAllFlags(); => 5 op�rations
    if (!(isDestTainted || (isSrcTainted && (destValue != 0)))) pTmgrTls->unTaintAllFlags(); // 3 op�rations
    else if ( !(isSrcTainted || (srcValue != 0))) 
    {
        pTmgrTls->unTaintAllFlags();
        if (isDestTainted) pTmgrTls->unTaintRegister<lengthInBits>(regSrcDest);
    }
    else 
    {
        _LOGTAINT(tid, insAddress, "andRR" + decstr(lengthInBits));

        // MARQUAGE DES FLAGS, d�pendant uniquement du r�sultat 

        // source 1 : source
        ObjectSource objSrc = (isSrcTainted)
            ? ObjectSource(pTmgrTls->getRegisterTaint<lengthInBits>(regSrc, srcValue)) 
            : ObjectSource(lengthInBits, srcValue);
        
        // source 2 : source/destination
        ObjectSource objSrcDest = (isDestTainted)
            ? ObjectSource(pTmgrTls->getRegisterTaint<lengthInBits>(regSrcDest, destValue)) 
            : ObjectSource(lengthInBits, destValue);
        
        fTaintLOGICAL(pTmgrTls, MK_TAINT_OBJECT_PTR(
            X_AND,
            objSrc,
            objSrcDest));

        // MARQUAGE DE LA DESTINATION
        // plusieurs cas selon le marquage des op�randes, cf tableau AND
        REGINDEX regSrcDestIndex = getRegIndex(regSrcDest);
        REGINDEX regSrcIndex     = getRegIndex(regSrc);

        for (UINT32 regPart = 0 ; regPart < (lengthInBits >> 3) ; ++regPart) 
        {  
            UINT32 src8bitsValue    = EXTRACTBYTE(srcValue, regPart);
            UINT32 dest8bitsValue   = EXTRACTBYTE(destValue, regPart);
            bool isSrc8bitsTainted  = pTmgrTls->isRegisterPartTainted(regSrcIndex, regPart);
            bool isDest8bitsTainted = pTmgrTls->isRegisterPartTainted(regSrcDestIndex, regPart);

            // CAS 1 : destination et sources non marqu�es => continuer pour prochain octet
            if (!(isDest8bitsTainted || isSrc8bitsTainted)) continue;

            // CAS 2 : destination non marqu�e (et donc source marqu�e)
            else if (!isDest8bitsTainted) 
            {
                // cas 2.1 : dest = 0 ; (AND 0, src) = tjs 0, donc continuer (dest d�j� d�marqu�e)
                if (!dest8bitsValue) continue;
                // cas 2.2 : destination vaut 0xff ; (AND 0xff, src) �quivaut � MOV dest, src
                else if (dest8bitsValue == 0xff) 
                {
                    pTmgrTls->updateTaintRegisterPart(regSrcDestIndex, regPart, std::make_shared<TaintByte>(
                        X_ASSIGN,
                        ObjectSource(pTmgrTls->getRegisterPartTaint(regSrcIndex, regPart))));
                }
                // cas 2.3 : autres valeur de la destination => marquage via AND
                else 
                {
                    pTmgrTls->updateTaintRegisterPart(regSrcDestIndex, regPart, std::make_shared<TaintByte>(
                        X_AND,
                        ObjectSource(pTmgrTls->getRegisterPartTaint(regSrcIndex, regPart)),
                        ObjectSource(8, dest8bitsValue)));
                }
            }
            // CAS 3 : source non marqu�e (et donc destination marqu�e)
            else if (!isSrc8bitsTainted) 
            {
                // cas 3.1 : src vaut 0 ; (AND dest, 0) fera tjs 0, donc d�marquer destination
                if (!src8bitsValue) pTmgrTls->unTaintRegisterPart(regSrcDestIndex, regPart);
                // cas 3.2 : src vaut 0xff ; (AND dest, 0xff) ne modifie pas dest => continuer
                else if (src8bitsValue == 0xff) continue;
                // cas 3.3 : autres valeur de la source => marquage via AND
                else 
                {
                    pTmgrTls->updateTaintRegisterPart(regSrcDestIndex, regPart, std::make_shared<TaintByte>(
                        X_AND,
                        ObjectSource(pTmgrTls->getRegisterPartTaint(regSrcDestIndex, regPart)),
                        ObjectSource(8, src8bitsValue)));  
                }                                   
            }
            // CAS 4 : source et destination marqu�es => marquage via AND
            else 
            {
                pTmgrTls->updateTaintRegisterPart(regSrcDestIndex, regPart, std::make_shared<TaintByte>(
                    X_AND,
                    ObjectSource(pTmgrTls->getRegisterPartTaint(regSrcDestIndex, regPart)),
                    ObjectSource(pTmgrTls->getRegisterPartTaint(regSrcIndex, regPart))));
            }   
        }
    }
} // sAND_RR

//////////////
///// OR /////
//////////////

// SIMULATE
template<UINT32 lengthInBits> void LOGICAL::sOR_IM(THREADID tid, ADDRINT value, ADDRINT writeAddress, ADDRINT insAddress) 
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);

    if (!pTmgrGlobal->isMemoryTainted<lengthInBits>(writeAddress)) pTmgrTls->unTaintAllFlags();
    else if (value == ValueFF<lengthInBits>::get()) 
    { 
        // OR x, 0xff... = 0xff..., donc d�marquage destination et flags
        pTmgrTls->unTaintAllFlags();
        pTmgrGlobal->unTaintMemory<lengthInBits>(writeAddress);
    }
    else 
    {
        _LOGTAINT(tid, insAddress, "orIM" + decstr(lengthInBits));

        // MARQUAGE DES FLAGS, d�pendant uniquement du r�sultat
        fTaintLOGICAL(pTmgrTls, MK_TAINT_OBJECT_PTR(
            X_OR,
            ObjectSource(pTmgrGlobal->getMemoryTaint<lengthInBits>(writeAddress)),
            ObjectSource(lengthInBits, value)));

        // MARQUAGE DE LA DESTINATION, par octet pour �viter surmarquage
        for (UINT32 byteNbr = 0 ; byteNbr < (lengthInBits >> 3) ; ++byteNbr, ++writeAddress)
        {
            // dest non marqu�e : ne rien faire
            if (!pTmgrGlobal->isMemoryTainted<8>(writeAddress)) continue;
        
            UINT32 value8bits = EXTRACTBYTE(value, byteNbr);  
            
            // 1er cas : valeur = 0 :  aucun changement
            if  (!value8bits) continue;         

            // 2eme cas : valeur 0xff => d�marquage (OR x, 0xff = tjs 0xff)
            else if (value8bits == 0xff)    pTmgrGlobal->unTaintMemory<8>(writeAddress);
            
            // sinon, marquage du r�sultat avec op�ration "OR"
            else  pTmgrGlobal->updateMemoryTaint<8>(writeAddress, std::make_shared<TaintByte>(
                    X_OR,
                    ObjectSource(pTmgrGlobal->getMemoryTaint<8>(writeAddress)),  
                    ObjectSource(8, value8bits)));
        }   
    }
} // sOR_IM

template<UINT32 lengthInBits> void LOGICAL::sOR_IR(THREADID tid, ADDRINT value, REG reg, ADDRINT regValue, ADDRINT insAddress)
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);

    if (!pTmgrTls->isRegisterTainted<lengthInBits>(reg)) pTmgrTls->unTaintAllFlags();
    else if (value == ValueFF<lengthInBits>::get()) // OR x, 0xff = 0xff, donc d�marquage destination et flags
    {
        pTmgrTls->unTaintAllFlags();
        pTmgrTls->unTaintRegister<lengthInBits>(reg);
    }
    else 
    {
        _LOGTAINT(tid, insAddress, "orIR" + decstr(lengthInBits));

        // MARQUAGE DES FLAGS, d�pendant uniquement du r�sultat
        fTaintLOGICAL(pTmgrTls, MK_TAINT_OBJECT_PTR(
            X_OR,
            ObjectSource(pTmgrTls->getRegisterTaint<lengthInBits>(reg, regValue)),
            ObjectSource(lengthInBits, value)));

        // MARQUAGE DE LA DESTINATION
        REGINDEX regIndex = getRegIndex(reg);
        for (UINT32 regPart = 0 ; regPart < (lengthInBits >> 3) ; ++regPart) 
        {
            // dest non marqu�e : ne rien faire
            if (!pTmgrTls->isRegisterPartTainted(regIndex, regPart)) continue;
        
            UINT32 value8bits = EXTRACTBYTE(value, regPart);

            // 1er cas : valeur = 0 : aucun changement
            if  (!value8bits) continue; 

            // 2eme cas : valeur nulle => d�marquage (OR x, 0xff = tjs 0xff)
            else if (value8bits == 0xff) pTmgrTls->unTaintRegisterPart(regIndex, regPart);

            // sinon, marquage du r�sultat avec op�ration "OR"
            else 
            { 
                pTmgrTls->updateTaintRegisterPart(regIndex, regPart, std::make_shared<TaintByte>(
                    X_OR,
                    ObjectSource(pTmgrTls->getRegisterPartTaint(regIndex, regPart)),  
                    ObjectSource(8, value8bits)));
            }
        }
    }
} // OR_IR

template<UINT32 lengthInBits> void LOGICAL::sOR_RM
    (THREADID tid, REG regSrc, ADDRINT srcValue, ADDRINT writeAddress, ADDRINT insAddress)
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);

    ADDRINT destValue = getMemoryValue<lengthInBits>(writeAddress);

    // marquage de la totalit� de la destination et de la source
    bool isDestTainted = pTmgrGlobal->isMemoryTainted<lengthInBits>(writeAddress);
    bool isSrcTainted  = pTmgrTls->isRegisterTainted<lengthInBits>(regSrc);
    
    // cf tableau d�marquage Flags sur le OR
    if (!isDestTainted && (!isSrcTainted || (destValue == ValueFF<lengthInBits>::get()))) 
    {
        pTmgrTls->unTaintAllFlags();
    }
    else if ( !isSrcTainted && (srcValue == ValueFF<lengthInBits>::get()))
    {
        pTmgrTls->unTaintAllFlags();
        if (isDestTainted) pTmgrGlobal->unTaintMemory<lengthInBits>(writeAddress);
    }
    else 
    {
        // dans tous les autres cas, marquage d'abord des flags puis destination octet par octet
        _LOGTAINT(tid, insAddress, "orRM" + decstr(lengthInBits));

        // MARQUAGE DES FLAGS, d�pendant uniquement du r�sultat 
        
        // source 1 : source
        ObjectSource objSrc = (isSrcTainted)
            ? ObjectSource(pTmgrTls->getRegisterTaint<lengthInBits>(regSrc, srcValue)) 
            : ObjectSource(lengthInBits, srcValue);
        
        // source 2 : source/destination
        ObjectSource objSrcDest = (isDestTainted)
            ? ObjectSource(pTmgrGlobal->getMemoryTaint<lengthInBits>(writeAddress)) 
            : ObjectSource(lengthInBits, destValue);

        fTaintLOGICAL(pTmgrTls, MK_TAINT_OBJECT_PTR(
            X_OR,
            objSrc,
            objSrcDest));

        // MARQUAGE DE LA DESTINATION
        // plusieurs cas selon le marquage des op�randes, cf tableau OR 
        REGINDEX regSrcIndex = getRegIndex(regSrc);
        for (UINT32 byteNbr = 0 ; byteNbr < (lengthInBits >> 3) ; ++byteNbr, ++writeAddress) 
        {   
            UINT32 src8bitsValue    = EXTRACTBYTE(srcValue, byteNbr);
            UINT32 dest8bitsValue   = EXTRACTBYTE(destValue, byteNbr);
            bool isSrc8bitsTainted  = pTmgrTls->isRegisterPartTainted(regSrcIndex, byteNbr);
            bool isDest8bitsTainted = pTmgrGlobal->isMemoryTainted<8>(writeAddress);

            // CAS 1 : destination et sources non marqu�es => continuer pour prochain octet
            if (!(isDest8bitsTainted || isSrc8bitsTainted)) continue; 

            // CAS 2 : destination non marqu�e (et donc source marqu�e)
            else if (!isDest8bitsTainted) 
            {
                // cas 2.1 : dest = 0xff ; (OR 0xff, src) = 0xff, donc go on (dest d�j� d�marqu�e)
                if (dest8bitsValue == 0xff) continue;
                // cas 2.2 : destination vaut 0 ; (OR 0, src) �quivaut � MOV dest, src
                else if (!dest8bitsValue) 
                {
                    pTmgrGlobal->updateMemoryTaint<8>(writeAddress, std::make_shared<TaintByte>(
                        X_ASSIGN,
                        ObjectSource(pTmgrTls->getRegisterPartTaint(regSrcIndex, byteNbr))));
                }
                // cas 2.3 : autres valeur de la destination => marquage via OR
                else
                {
                    pTmgrGlobal->updateMemoryTaint<8>(writeAddress, std::make_shared<TaintByte>(
                        X_OR,
                        ObjectSource(pTmgrTls->getRegisterPartTaint(regSrcIndex, byteNbr)),
                        ObjectSource(8, dest8bitsValue)));
                }
            }

            // CAS 3 : source non marqu�e (et donc destination marqu�e)
            else if (!isSrc8bitsTainted) 
            {
                // cas 3.1 : src vaut 0xff ; (OR dest, 0xff = 0xff) donc d�marquer destination
                if (src8bitsValue == 0xff) pTmgrGlobal->unTaintMemory<8>(writeAddress);
                // cas 3.2 : src vaut 0 ; (OR dest, 0) ne modifie pas dest => continuer
                else if (!src8bitsValue) continue;
                // cas 3.3 : autres valeur de la source => marquage via OR
                else pTmgrGlobal->updateMemoryTaint<8>(writeAddress, std::make_shared<TaintByte>(
                        X_OR,
                        ObjectSource(pTmgrGlobal->getMemoryTaint<8>(writeAddress)),
                        ObjectSource(8, src8bitsValue)));                      
            }

            // CAS 4 : source et destination marqu�es => marquage via OR
            else 
            {
                pTmgrGlobal->updateMemoryTaint<8>(writeAddress, std::make_shared<TaintByte>(
                    X_OR,
                    ObjectSource(pTmgrGlobal->getMemoryTaint<8>(writeAddress)),  
                    ObjectSource(pTmgrTls->getRegisterPartTaint(regSrcIndex, byteNbr))));
            }
        }
    }
} // sOR_RM

template<UINT32 lengthInBits> void LOGICAL::sOR_MR
    (THREADID tid, ADDRINT readAddress, REG regSrcDest, ADDRINT destValue, ADDRINT insAddress) 
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);

    ADDRINT srcValue = getMemoryValue<lengthInBits>(readAddress);

    // marquage de la totalit� de la destination et de la source
    bool isDestTainted = pTmgrTls->isRegisterTainted<lengthInBits>(regSrcDest);
    bool isSrcTainted  = pTmgrGlobal->isMemoryTainted<lengthInBits>(readAddress);
    
    // cf tableau d�marquage Flags sur le OR
    if (!isDestTainted && (!isSrcTainted || (destValue == ValueFF<lengthInBits>::get()))) 
    {
        pTmgrTls->unTaintAllFlags();
    }
    else if ( !isSrcTainted && (srcValue == ValueFF<lengthInBits>::get()))
    {
        pTmgrTls->unTaintAllFlags();
        if (isDestTainted) pTmgrTls->unTaintRegister<lengthInBits>(regSrcDest);
    }
    else 
    {
        // dans tous les autres cas, marquage d'abord des flags, puis destination octet par octet
        _LOGTAINT(tid, insAddress, "orMR" + decstr(lengthInBits));

        // MARQUAGE DES FLAGS, d�pendant uniquement du r�sultat 

        // source 1 : source
        ObjectSource objSrc = (isSrcTainted)
            ? ObjectSource(pTmgrGlobal->getMemoryTaint<lengthInBits>(readAddress))
            : ObjectSource(lengthInBits, srcValue);
        
        // source 2 : source/destination
        ObjectSource objSrcDest = (isDestTainted)
            ? ObjectSource(pTmgrTls->getRegisterTaint<lengthInBits>(regSrcDest, destValue)) 
            : ObjectSource(lengthInBits, destValue);
        
        fTaintLOGICAL(pTmgrTls, MK_TAINT_OBJECT_PTR(
            X_OR,
            objSrc,
            objSrcDest));

        // MARQUAGE DE LA DESTINATION, octet par octet
        // plusieurs cas selon le marquage des op�randes, cf tableau OR
        
        REGINDEX regSrcDestIndex = getRegIndex(regSrcDest);
        for (UINT32 byteNbr = 0 ; byteNbr < (lengthInBits >> 3) ; ++byteNbr, ++readAddress) 
        {   
            UINT32 src8bitsValue    = EXTRACTBYTE(srcValue, byteNbr);
            UINT32 dest8bitsValue   = EXTRACTBYTE(destValue, byteNbr);
            bool isSrc8bitsTainted  = pTmgrGlobal->isMemoryTainted<8>(readAddress);
            bool isDest8bitsTainted = pTmgrTls->isRegisterPartTainted(regSrcDestIndex, byteNbr);

            // CAS 1 : destination et sources non marqu�es => continuer pour prochain octet
            if (!(isDest8bitsTainted || isSrc8bitsTainted)) continue;  

            // CAS 2 : destination non marqu�e (et donc source marqu�e)
            else if (!isDest8bitsTainted) 
            {
                // cas 2.1 : dest = 0xff ; (OR 0xff, src) = 0xff, donc go on (dest d�j� d�marqu�e)
                if (dest8bitsValue == 0xff) continue;
                // cas 2.2 : destination vaut 0 ; (OR 0, src) �quivaut � MOV dest, src
                else if (!dest8bitsValue) 
                {
                    pTmgrTls->updateTaintRegisterPart(regSrcDestIndex, byteNbr, std::make_shared<TaintByte>(
                        X_ASSIGN,
                        ObjectSource(pTmgrGlobal->getMemoryTaint<8>(readAddress))));
                }
                // cas 2.3 : autres valeur de la destination => marquage via OR
                else 
                {
                    pTmgrTls->updateTaintRegisterPart(regSrcDestIndex, byteNbr, std::make_shared<TaintByte>(
                        X_OR,
                        ObjectSource(pTmgrGlobal->getMemoryTaint<8>(readAddress)),
                        ObjectSource(8, dest8bitsValue)));
                }
            }
            // CAS 3 : source non marqu�e (et donc destination marqu�e)
            else if (!isSrc8bitsTainted) 
            {
                // cas 3.1 : src vaut 0xff ; (OR dest, 0xff) = 0xff, donc d�marquer destination
                if (src8bitsValue == 0xff) pTmgrTls->unTaintRegisterPart(regSrcDestIndex, byteNbr);
                // cas 3.2 : src vaut 0 ; (OR dest, 0) ne modifie pas dest => continuer
                else if (!src8bitsValue) continue;
                // cas 3.3 : autres valeur de la source => marquage via OR
                else 
                {
                    pTmgrTls->updateTaintRegisterPart(regSrcDestIndex, byteNbr, std::make_shared<TaintByte>(
                        X_OR,
                        ObjectSource(pTmgrTls->getRegisterPartTaint(regSrcDestIndex, byteNbr)),
                        ObjectSource(8, src8bitsValue)));    
                }                                   
            }
            // CAS 4 : source et destination marqu�es => marquage via OR
            else 
            {
                pTmgrTls->updateTaintRegisterPart(regSrcDestIndex, byteNbr, std::make_shared<TaintByte>(
                    X_OR,
                    ObjectSource(pTmgrTls->getRegisterPartTaint(regSrcDestIndex, byteNbr)),
                    ObjectSource(pTmgrGlobal->getMemoryTaint<8>(readAddress))));
            }   
        }
    }
} // end sOR_MR

template<UINT32 lengthInBits> void LOGICAL::sOR_RR
    (THREADID tid, REG regSrc, ADDRINT srcValue, REG regSrcDest, ADDRINT destValue, ADDRINT insAddress)
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);

    // marquage de la totalit� de la destination et de la source
    bool isDestTainted = pTmgrTls->isRegisterTainted<lengthInBits>(regSrcDest);
    bool isSrcTainted  = pTmgrTls->isRegisterTainted<lengthInBits>(regSrc);
    
    // cf tableau d�marquage Flags sur le OR
    if (!isDestTainted && (!isSrcTainted || (destValue == ValueFF<lengthInBits>::get()))) 
    {
        pTmgrTls->unTaintAllFlags();
    }
    else if ( !isSrcTainted && (srcValue == ValueFF<lengthInBits>::get()))
    {
        pTmgrTls->unTaintAllFlags();
        if (isDestTainted) pTmgrTls->unTaintRegister<lengthInBits>(regSrcDest);
    }
    else 
    {
        _LOGTAINT(tid, insAddress, "orRR" + decstr(lengthInBits));

        // MARQUAGE DES FLAGS, d�pendant uniquement du r�sultat 

        // source 1 : source
        ObjectSource objSrc = (isSrcTainted)
            ? ObjectSource(pTmgrTls->getRegisterTaint<lengthInBits>(regSrc, srcValue)) 
            : ObjectSource(lengthInBits, srcValue);
        
             // source 2 : source/destination
        ObjectSource objSrcDest = (isDestTainted)
            ? ObjectSource(pTmgrTls->getRegisterTaint<lengthInBits>(regSrcDest, destValue)) 
            : ObjectSource(lengthInBits, destValue);
        
        fTaintLOGICAL(pTmgrTls, MK_TAINT_OBJECT_PTR(
            X_OR,
            objSrc,
            objSrcDest));

        // MARQUAGE DE LA DESTINATION
        // plusieurs cas selon le marquage des op�randes, cf tableau OR
        REGINDEX regSrcDestIndex = getRegIndex(regSrcDest);
        REGINDEX regSrcIndex     = getRegIndex(regSrc);

        for (UINT32 regPart = 0 ; regPart < (lengthInBits >> 3) ; ++regPart) 
        {  
            UINT32 src8bitsValue    = EXTRACTBYTE(srcValue, regPart);
            UINT32 dest8bitsValue   = EXTRACTBYTE(destValue, regPart);
            bool isSrc8bitsTainted  = pTmgrTls->isRegisterPartTainted(regSrcIndex, regPart);
            bool isDest8bitsTainted = pTmgrTls->isRegisterPartTainted(regSrcDestIndex, regPart);

            // CAS 1 : destination et sources non marqu�es => continuer pour prochain octet
            if (!(isDest8bitsTainted || isSrc8bitsTainted)) continue;

            // CAS 2 : destination non marqu�e (et donc source marqu�e)
            else if (!isDest8bitsTainted) 
            {
                // cas 2.1 : dest = 0 => (OR 0xff, src) = 0xff donc go on (dest d�j� d�marqu�e)
                if (dest8bitsValue == 0xff) continue;
                // cas 2.2 : destination vaut 0 ; (OR 0, src) �quivaut � MOV dest, src
                else if (!dest8bitsValue)
                {
                    pTmgrTls->updateTaintRegisterPart(regSrcDestIndex, regPart, std::make_shared<TaintByte>(
                        X_ASSIGN,
                        ObjectSource(pTmgrTls->getRegisterPartTaint(regSrcIndex, regPart))));
                }
                // cas 2.3 : autres valeur de la destination => marquage via OR
                else
                {
                    pTmgrTls->updateTaintRegisterPart(regSrcDestIndex, regPart, std::make_shared<TaintByte>(
                        X_OR,
                        ObjectSource(pTmgrTls->getRegisterPartTaint(regSrcIndex, regPart)),
                        ObjectSource(8, dest8bitsValue)));
                }
            }
            // CAS 3 : source non marqu�e (et donc destination marqu�e)
            else if (!isSrc8bitsTainted) 
            {
                // cas 3.1 : src vaut 0xff => (OR dest, 0xff = 0xff) donc d�marquer destination
                if (src8bitsValue == 0xff)  pTmgrTls->unTaintRegisterPart(regSrcDestIndex, regPart);
                // cas 3.2 : src vaut 0 ; (OR dest, 0) ne modifie pas dest => continuer
                else if (!src8bitsValue) continue;
                // cas 3.3 : autres valeur de la source => marquage via OR
                else 
                {
                    pTmgrTls->updateTaintRegisterPart(regSrcDestIndex, regPart, std::make_shared<TaintByte>(
                        X_OR,
                        ObjectSource(pTmgrTls->getRegisterPartTaint(regSrcDestIndex, regPart)),
                        ObjectSource(8, src8bitsValue)));     
                }                                   
            }
            // CAS 4 : source et destination marqu�es => marquage via OR
            else 
            {
                pTmgrTls->updateTaintRegisterPart(regSrcDestIndex, regPart, std::make_shared<TaintByte>(
                    X_OR,
                    ObjectSource(pTmgrTls->getRegisterPartTaint(regSrcDestIndex, regPart)),
                    ObjectSource(pTmgrTls->getRegisterPartTaint(regSrcIndex, regPart)))); 
            }   
        }
    }
} // sOR_RR

///////////////
///// XOR /////
///////////////

// SIMULATE
template<UINT32 lengthInBits> void LOGICAL::sXOR_IM(THREADID tid, ADDRINT value, ADDRINT writeAddress, ADDRINT insAddress)
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);

    if (!pTmgrGlobal->isMemoryTainted<lengthInBits>(writeAddress)) pTmgrTls->unTaintAllFlags();
    else 
    {
        _LOGTAINT(tid, insAddress, "xorIM" + decstr(lengthInBits));

        // MARQUAGE DES FLAGS, d�pendant uniquement du r�sultat
        fTaintLOGICAL(pTmgrTls, MK_TAINT_OBJECT_PTR(
            X_XOR,
            ObjectSource(pTmgrGlobal->getMemoryTaint<lengthInBits>(writeAddress)),
            ObjectSource(lengthInBits, value)));

        // MARQUAGE DE LA DESTINATION, par octet pour �viter surmarquage
        for (UINT32 byteNbr = 0 ; byteNbr < (lengthInBits >> 3) ; ++byteNbr, ++writeAddress)
        {
            // dest non marqu�e : ne rien faire
            if (!pTmgrGlobal->isMemoryTainted<8>(writeAddress)) continue;
        
            UINT32 value8bits = EXTRACTBYTE(value, byteNbr);  
            
            // 1er cas : valeur = 0 :  aucun changement
            if  (!value8bits) continue;         
            else 
            {
                ObjectSource objSrcMem(pTmgrGlobal->getMemoryTaint<8>(writeAddress));

                // 2eme cas : valeur 0xff => NOT de la destination (XOR x, 0xff = NOT x)
                if (value8bits == 0xff) 
                {
                    pTmgrGlobal->updateMemoryTaint<8>(writeAddress, std::make_shared<TaintByte>(
                        X_NOT,
                        objSrcMem));
                }
                // sinon, marquage du r�sultat avec op�ration "XOR"
                else 
                {   
                    pTmgrGlobal->updateMemoryTaint<8>(writeAddress, std::make_shared<TaintByte>(
                        X_XOR,
                        ObjectSource(pTmgrGlobal->getMemoryTaint<8>(writeAddress)),  
                        ObjectSource(8, value8bits)));
                }
            }
        }   
    }
} // sXOR_IM

template<UINT32 lengthInBits> void LOGICAL::sXOR_IR(THREADID tid, ADDRINT value, REG reg, ADDRINT regValue, ADDRINT insAddress) 
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);

    if (!pTmgrTls->isRegisterTainted<lengthInBits>(reg)) pTmgrTls->unTaintAllFlags();
    else 
    {
        _LOGTAINT(tid, insAddress, "xorIR" + decstr(lengthInBits));

        // MARQUAGE DES FLAGS, d�pendant uniquement du r�sultat
        fTaintLOGICAL(pTmgrTls, MK_TAINT_OBJECT_PTR(
            X_XOR,
            ObjectSource(pTmgrTls->getRegisterTaint<lengthInBits>(reg, regValue)),
            ObjectSource(lengthInBits, value)));

        // MARQUAGE DE LA DESTINATION
        REGINDEX regIndex = getRegIndex(reg);
        for (UINT32 regPart = 0 ; regPart < (lengthInBits >> 3) ; ++regPart) 
        {
            // dest non marqu�e : ne rien faire
            if (!pTmgrTls->isRegisterPartTainted(regIndex, regPart)) continue;
            
             UINT32 value8bits = EXTRACTBYTE(value, regPart);
             // 1er cas : valeur = 0 :  aucun changement
            if  (!value8bits) continue; 
            else 
            {   
                ObjectSource objSrcReg(pTmgrTls->getRegisterPartTaint(regIndex, regPart));

                // 2eme cas : valeur 0xff => NOT de la destination (XOR x, 0xff = NOT x)
                if (value8bits == 0xff) 
                {
                    pTmgrTls->updateTaintRegisterPart(regIndex, regPart, std::make_shared<TaintByte>(
                        X_NOT,
                        objSrcReg));
                }
                // sinon, marquage du r�sultat avec op�ration "XOR"
                else 
                { 
                    pTmgrTls->updateTaintRegisterPart(regIndex, regPart, std::make_shared<TaintByte>(
                        X_XOR,
                        ObjectSource(pTmgrTls->getRegisterPartTaint(regIndex, regPart)),  
                        ObjectSource(8, value8bits)));
                }
            }
        }
    }
} // sXOR_IR

template<UINT32 lengthInBits> void LOGICAL::sXOR_RM
    (THREADID tid, REG regSrc, ADDRINT srcValue, ADDRINT writeAddress, ADDRINT insAddress) 
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);

    ADDRINT destValue = getMemoryValue<lengthInBits>(writeAddress);

    // marquage de la totalit� de la destination et de la source
    bool isDestTainted = pTmgrGlobal->isMemoryTainted<lengthInBits>(writeAddress);
    bool isSrcTainted  = pTmgrTls->isRegisterTainted<lengthInBits>(regSrc);
    
    if (!(isSrcTainted || isDestTainted)) pTmgrTls->unTaintAllFlags();
    else 
    {
        // dans tous les autres cas, marquage d'abord des flags puis destination octet par octet
        _LOGTAINT(tid, insAddress, "xorRM" + decstr(lengthInBits));

        // MARQUAGE DES FLAGS, d�pendant uniquement du r�sultat 
        
        // source 1 : source
        ObjectSource objSrc = (isSrcTainted)
            ? ObjectSource(pTmgrTls->getRegisterTaint<lengthInBits>(regSrc, srcValue)) 
            : ObjectSource(lengthInBits, srcValue);
        
        // source 2 : source/destination
        ObjectSource objSrcDest = (isDestTainted)
            ? ObjectSource(pTmgrGlobal->getMemoryTaint<lengthInBits>(writeAddress)) 
            : ObjectSource(lengthInBits, destValue);

        fTaintLOGICAL(pTmgrTls, MK_TAINT_OBJECT_PTR(
            X_XOR,
            objSrc,
            objSrcDest));

        // MARQUAGE DE LA DESTINATION
        // plusieurs cas selon le marquage des op�randes, cf tableau XOR    
        REGINDEX regSrcIndex = getRegIndex(regSrc);
        for (UINT32 byteNbr = 0 ; byteNbr < (lengthInBits >> 3) ; ++byteNbr, ++writeAddress) 
        {   
            UINT32 src8bitsValue    = EXTRACTBYTE(srcValue, byteNbr);
            UINT32 dest8bitsValue   = EXTRACTBYTE(destValue, byteNbr);
            bool isSrc8bitsTainted  = pTmgrTls->isRegisterPartTainted(regSrcIndex, byteNbr);
            bool isDest8bitsTainted = pTmgrGlobal->isMemoryTainted<8>(writeAddress);

            // CAS 1 : destination et sources non marqu�es => continuer pour prochain octet
            if (!(isDest8bitsTainted || isSrc8bitsTainted)) continue;   

            // CAS 2 : destination non marqu�e (et donc source marqu�e)
            else if (!isDest8bitsTainted) 
            {
                // cas 2.1 : destination vaut 0 ; (XOR 0, src) �quivaut � MOV dest, src
                if (!dest8bitsValue)
                {
                    pTmgrGlobal->updateMemoryTaint<8>(writeAddress, std::make_shared<TaintByte>(
                        X_ASSIGN,
                        ObjectSource(pTmgrTls->getRegisterPartTaint(regSrcIndex, byteNbr))));
                }
                // cas 2.2 : dest = 0xff ; XOR 0xff, src �quivaut � NOT src, affect� � la dest
                else if (dest8bitsValue == 0xff) 
                {
                    pTmgrGlobal->updateMemoryTaint<8>(writeAddress, std::make_shared<TaintByte>(
                        X_NOT,
                        ObjectSource(pTmgrTls->getRegisterPartTaint(regSrcIndex, byteNbr))));
                }
                // cas 2.3 : autres valeurs destination => marquage via XOR
                else
                {
                    pTmgrGlobal->updateMemoryTaint<8>(writeAddress, std::make_shared<TaintByte>(
                        X_XOR,
                        ObjectSource(pTmgrTls->getRegisterPartTaint(regSrcIndex, byteNbr)),
                        ObjectSource(8, dest8bitsValue)));  
                }
            }
            // CAS 3 : source non marqu�e (et donc destination marqu�e)
            else if (!isSrc8bitsTainted) 
            {
                // cas 3.1 : src vaut 0 ; (XOR dest, 0) ne modifie pas dest => continuer
                if (!src8bitsValue)  continue;
                // cas 3.2 : src vaut 0xff ; (XOR dest, 0xff) �quivaut � NOT dest
                else if (src8bitsValue == 0xff) 
                {
                    pTmgrGlobal->updateMemoryTaint<8>(writeAddress, std::make_shared<TaintByte>(
                        X_NOT,
                        ObjectSource(pTmgrGlobal->getMemoryTaint<8>(writeAddress))));
                }
                // cas 3.3 : autres valeurs source => marquage via XOR
                else  pTmgrGlobal->updateMemoryTaint<8>(writeAddress, std::make_shared<TaintByte>(
                        X_XOR,
                        ObjectSource(pTmgrGlobal->getMemoryTaint<8>(writeAddress)),
                        ObjectSource(8, src8bitsValue)));
            }
            // CAS 4 : source et destination marqu�es => marquage via XOR
            else 
            {
                pTmgrGlobal->updateMemoryTaint<8>(writeAddress, std::make_shared<TaintByte>(
                    X_XOR,
                    ObjectSource(pTmgrGlobal->getMemoryTaint<8>(writeAddress)),  
                    ObjectSource(pTmgrTls->getRegisterPartTaint(regSrcIndex, byteNbr))));
            }   
        }
    }
} // sXOR_RM

template<UINT32 lengthInBits> void LOGICAL::sXOR_MR
    (THREADID tid, ADDRINT readAddress, REG regSrcDest, ADDRINT destValue, ADDRINT insAddress)
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);

    ADDRINT srcValue = getMemoryValue<lengthInBits>(readAddress);

    // marquage de la totalit� de la destination et de la source
    bool isDestTainted = pTmgrTls->isRegisterTainted<lengthInBits>(regSrcDest);
    bool isSrcTainted  = pTmgrGlobal->isMemoryTainted<lengthInBits>(readAddress);
        
    // cf tableau d�marquage Flags sur le XOR
    if (!(isSrcTainted || isDestTainted)) pTmgrTls->unTaintAllFlags();
    else 
    {
        _LOGTAINT(tid, insAddress, "xorMR" + decstr(lengthInBits));
        // MARQUAGE DES FLAGS, d�pendant uniquement du r�sultat 

        // source 1 : source
        ObjectSource objSrc = (isSrcTainted)
            ? ObjectSource(pTmgrGlobal->getMemoryTaint<lengthInBits>(readAddress))
            : ObjectSource(lengthInBits, srcValue);
        
        // source 2 : source/destination
        ObjectSource objSrcDest = (isDestTainted)
            ? ObjectSource(pTmgrTls->getRegisterTaint<lengthInBits>(regSrcDest, destValue)) 
            : ObjectSource(lengthInBits, destValue);
        
        fTaintLOGICAL(pTmgrTls, MK_TAINT_OBJECT_PTR(
            X_XOR,
            objSrc,
            objSrcDest));

        // MARQUAGE DE LA DESTINATION, octet par octet
        // plusieurs cas selon le marquage des op�randes, cf tableau XOR
        REGINDEX regSrcDestIndex = getRegIndex(regSrcDest);
        for (UINT32 byteNbr = 0 ; byteNbr < (lengthInBits >> 3) ; ++byteNbr, ++readAddress) 
        {   
            UINT32 src8bitsValue    = EXTRACTBYTE(srcValue, byteNbr);
            UINT32 dest8bitsValue   = EXTRACTBYTE(destValue, byteNbr);
            bool isSrc8bitsTainted  = pTmgrGlobal->isMemoryTainted<8>(readAddress);
            bool isDest8bitsTainted = pTmgrTls->isRegisterPartTainted(regSrcDestIndex, byteNbr);

            // CAS 1 : destination et sources non marqu�es => continuer pour prochain octet
            if (!(isDest8bitsTainted || isSrc8bitsTainted)) continue;  

            // CAS 2 : destination non marqu�e (et donc source marqu�e)
            else if (!isDest8bitsTainted) 
            {
                // cas 2.1 : destination vaut 0 ; (XOR 0, src) �quivaut � MOV dest, src
                if (!dest8bitsValue)
                {
                    pTmgrTls->updateTaintRegisterPart(regSrcDestIndex, byteNbr, std::make_shared<TaintByte>(
                        X_ASSIGN,
                        ObjectSource(pTmgrGlobal->getMemoryTaint<8>(readAddress))));
                } 
                // cas 2.2 : dest = 0xff ; (XOR 0xff, src) �quivaut � NOT src, affect� � la dest
                else if (dest8bitsValue == 0xff) 
                {
                    pTmgrTls->updateTaintRegisterPart(regSrcDestIndex, byteNbr, std::make_shared<TaintByte>(
                        X_NOT,
                        ObjectSource(pTmgrGlobal->getMemoryTaint<8>(readAddress))));
                } 
                // cas 2.3 : autres valeurs destination => marquage via XOR
                else 
                {
                    pTmgrTls->updateTaintRegisterPart(regSrcDestIndex, byteNbr, std::make_shared<TaintByte>(
                        X_XOR,
                        ObjectSource(pTmgrGlobal->getMemoryTaint<8>(readAddress)),
                        ObjectSource(8, dest8bitsValue)));
                }
            }
            // CAS 3 : source non marqu�e (et donc destination marqu�e)
            else if (!isSrc8bitsTainted) 
            {
                // cas 3.1 : src vaut 0; (XOR dest, 0) ne modifie pas dest => continuer
                if (!src8bitsValue) continue;
                // cas 3.2 : src vaut 0xff ; (XOR dest, 0xff) �quivaut � NOT dest
                else if (src8bitsValue == 0xff) 
                {
                    pTmgrTls->updateTaintRegisterPart(regSrcDestIndex, byteNbr, std::make_shared<TaintByte>(
                        X_NOT,
                        ObjectSource(pTmgrTls->getRegisterPartTaint(regSrcDestIndex, byteNbr))));
                }
                // cas 3.3 : autres valeur source : marquage via XOR
                else 
                {
                    pTmgrTls->updateTaintRegisterPart(regSrcDestIndex, byteNbr, std::make_shared<TaintByte>(
                        X_XOR,
                        ObjectSource(pTmgrTls->getRegisterPartTaint(regSrcDestIndex, byteNbr)),
                        ObjectSource(8, src8bitsValue)));    
                }                                   
            }
            // CAS 4 : source et destination marqu�es => marquage via XOR
            else 
            {
                pTmgrTls->updateTaintRegisterPart(regSrcDestIndex, byteNbr, std::make_shared<TaintByte>(
                    X_OR,
                    ObjectSource(pTmgrTls->getRegisterPartTaint(regSrcDestIndex, byteNbr)),
                    ObjectSource(pTmgrGlobal->getMemoryTaint<8>(readAddress))));
            }   
        }
    }
} // sXOR_RM

template<UINT32 lengthInBits> void LOGICAL::sXOR_RR
    (THREADID tid, REG regSrc, ADDRINT srcValue, REG regSrcDest, ADDRINT destValue, ADDRINT insAddress) 
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);

    // marquage de la totalit� de la destination et de la source
    bool isDestTainted = pTmgrTls->isRegisterTainted<lengthInBits>(regSrcDest);
    bool isSrcTainted  = pTmgrTls->isRegisterTainted<lengthInBits>(regSrc);
    
    if ( !(isSrcTainted || isDestTainted)) pTmgrTls->unTaintAllFlags();
    else 
    {
        _LOGTAINT(tid, insAddress, "xorRR" + decstr(lengthInBits));

        // MARQUAGE DES FLAGS, d�pendant uniquement du r�sultat 

        // source 1 : source
        ObjectSource objSrc = (isSrcTainted)
            ? ObjectSource(pTmgrTls->getRegisterTaint<lengthInBits>(regSrc, srcValue)) 
            : ObjectSource(lengthInBits, srcValue);
        
        // source 2 : source/destination
        ObjectSource objSrcDest = (isDestTainted)
            ? ObjectSource(pTmgrTls->getRegisterTaint<lengthInBits>(regSrcDest, destValue)) 
            : ObjectSource(lengthInBits, destValue);
        
        fTaintLOGICAL(pTmgrTls, MK_TAINT_OBJECT_PTR(
            X_XOR,
            objSrc,
            objSrcDest));

        // MARQUAGE DE LA DESTINATION
        // plusieurs cas selon le marquage des op�randes, cf tableau XOR
        REGINDEX regSrcDestIndex = getRegIndex(regSrcDest);
        REGINDEX regSrcIndex     = getRegIndex(regSrc);

        for (UINT32 regPart = 0 ; regPart < (lengthInBits >> 3) ; ++regPart) 
        {  
            UINT32 src8bitsValue    = EXTRACTBYTE(srcValue, regPart);
            UINT32 dest8bitsValue   = EXTRACTBYTE(destValue, regPart);
            bool isSrc8bitsTainted  = pTmgrTls->isRegisterPartTainted(regSrcIndex, regPart);
            bool isDest8bitsTainted = pTmgrTls->isRegisterPartTainted(regSrcDestIndex, regPart);

            // CAS 1 : destination et sources non marqu�es => continuer pour prochain octet
            if (!(isDest8bitsTainted || isSrc8bitsTainted)) continue;  

            // CAS 2 : destination non marqu�e (et donc source marqu�e)
            else if (!isDest8bitsTainted) 
            {
                // cas 2.1 : destination vaut 0 ; (XOR 0, src) �quivaut � MOV dest, src
                if (!dest8bitsValue) 
                {
                    pTmgrTls->updateTaintRegisterPart(regSrcDestIndex, regPart, std::make_shared<TaintByte>(
                        X_ASSIGN,
                        ObjectSource(pTmgrTls->getRegisterPartTaint(regSrcIndex, regPart))));   
                }
                // cas 2.2 : dest = 0xff ; (XOR 0xff, src) �quivaut � NOT src, affect� � la dest
                else if (dest8bitsValue == 0xff)
                {
                    pTmgrTls->updateTaintRegisterPart(regSrcDestIndex, regPart, std::make_shared<TaintByte>(
                        X_NOT,
                        ObjectSource(pTmgrTls->getRegisterPartTaint(regSrcIndex, regPart))));   
                }
                // cas 2.3 : autres valeurs destination => marquage via XOR
                else 
                {
                    pTmgrTls->updateTaintRegisterPart(regSrcDestIndex, regPart, std::make_shared<TaintByte>(
                        X_XOR,
                        ObjectSource(pTmgrTls->getRegisterPartTaint(regSrcIndex, regPart)),
                        ObjectSource(8, dest8bitsValue)));    
                }
            }
            // CAS 3 : source non marqu�e (et donc destination marqu�e)
            else if (!isSrc8bitsTainted) 
            {
                // cas 3.1 : src vaut 0 ; (XOR dest, 0) ne modifie pas dest => continuer
                if (!src8bitsValue)  continue;
                // cas 3.2 : src vaut 0xff ; (XOR dest, 0xff) �quivaut � NOT dest
                else if (src8bitsValue == 0xff) 
                {
                    pTmgrTls->updateTaintRegisterPart(regSrcDestIndex, regPart, std::make_shared<TaintByte>(
                        X_NOT,
                        ObjectSource(pTmgrTls->getRegisterPartTaint(regSrcDestIndex, regPart))));   
                }
                // cas 3.3 : autres valeur de la source => marquage via XOR
                else 
                {
                    pTmgrTls->updateTaintRegisterPart(regSrcDestIndex, regPart, std::make_shared<TaintByte>(
                        X_XOR,
                        ObjectSource(pTmgrTls->getRegisterPartTaint(regSrcDestIndex, regPart)),
                        ObjectSource(8, src8bitsValue)));    
                }                                   
            }
            // CAS 4 : source et destination marqu�es => marquage via XOR
            else 
            {
                pTmgrTls->updateTaintRegisterPart(regSrcDestIndex, regPart, std::make_shared<TaintByte>(
                    X_XOR,
                    ObjectSource(pTmgrTls->getRegisterPartTaint(regSrcDestIndex, regPart)),
                    ObjectSource(pTmgrTls->getRegisterPartTaint(regSrcIndex, regPart))));  
            }   
        }
    }
} // end sXOR_RR

template<UINT32 lengthInBits> void LOGICAL::sXOR_RR_EQUAL(THREADID tid, REG reg, ADDRINT insAddress) 
{
    // cas typique xor reg, reg => d�marquage registre et flags 
    //_LOGUNTAINT(" XOR_RR_EQUAL"); 

    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);

    pTmgrTls->unTaintAllFlags();
    pTmgrTls->unTaintRegister<lengthInBits>(reg);
} // sXOR_RR_EQUAL

////////////////
///// TEST /////
////////////////

// SIMULATE
template<UINT32 lengthInBits> void LOGICAL::sTEST_IM(THREADID tid, ADDRINT value, ADDRINT writeAddress, ADDRINT insAddress) 
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    
    if ( !pTmgrGlobal->isMemoryTainted<lengthInBits>(writeAddress) || (value == 0)) 
    {
        pTmgrTls->unTaintAllFlags();
    }
    else 
    {
        _LOGTAINT(tid, insAddress, "testIM" + decstr(lengthInBits));
        // MARQUAGE DES FLAGS, d�pendant uniquement du r�sultat
        fTaintLOGICAL(pTmgrTls, MK_TAINT_OBJECT_PTR(
            X_AND,
            ObjectSource(pTmgrGlobal->getMemoryTaint<lengthInBits>(writeAddress)),
            ObjectSource(lengthInBits, value)));
    }
} // sTEST_IM

template<UINT32 lengthInBits> void LOGICAL::sTEST_IR(THREADID tid, ADDRINT value, REG reg, ADDRINT regValue, ADDRINT insAddress) 
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);

    if ( !pTmgrTls->isRegisterTainted<lengthInBits>(reg) || (value == 0)) pTmgrTls->unTaintAllFlags();
    else 
    {
        _LOGTAINT(tid, insAddress, "testIR" + decstr(lengthInBits));

        // MARQUAGE DES FLAGS, d�pendant uniquement du r�sultat
        fTaintLOGICAL(pTmgrTls, MK_TAINT_OBJECT_PTR(
            X_AND,
            ObjectSource(pTmgrTls->getRegisterTaint<lengthInBits>(reg, regValue)),
            ObjectSource(lengthInBits, value)));
    }
} // sTEST_IR

template<UINT32 lengthInBits> void LOGICAL::sTEST_RM
    (THREADID tid, REG regSrc, ADDRINT srcValue, ADDRINT writeAddress, ADDRINT insAddress) 
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);

    ADDRINT destValue = getMemoryValue<lengthInBits>(writeAddress);

    // marquage de la totalit� de la destination et de la source
    bool isFullDestTainted = pTmgrGlobal->isMemoryTainted<lengthInBits>(writeAddress);
    bool isFullSrcTainted  = pTmgrTls->isRegisterTainted<lengthInBits>(regSrc);
    
    // cas particuliers du AND (cf tableau, avec simplification par loi Morgan)
    /* if ((!isFullDestTainted && !isFullSrcTainted) \
        || (!isFullDestTainted && !destValue) \
        || (!isFullSrcTainted && !srcValue) ) => 11 op�rations */
    //  (!a && !b) || (!a && !c) || (!b || !d) <=> ( !(a && (b || d) || (b && c))
    //  PLUS QUE 5 OPERATIONS                                       
    if (!((isFullDestTainted && (isFullSrcTainted || (srcValue != 0))) || (isFullSrcTainted && (destValue != 0)))) 
    {
        pTmgrTls->unTaintAllFlags();
    }
    
    else // dans tous les autres cas, marquage des flags 
    { 
        _LOGTAINT(tid, insAddress, "testRM" + decstr(lengthInBits));

        // MARQUAGE DES FLAGS, d�pendant uniquement du r�sultat 

        // source 1 : source
        ObjectSource objSrc = (isFullSrcTainted)
            ? ObjectSource(pTmgrTls->getRegisterTaint<lengthInBits>(regSrc, srcValue)) 
            : ObjectSource(lengthInBits, srcValue);
        
        // source 2 : source/destination
        ObjectSource objSrcDest = (isFullDestTainted)
            ? ObjectSource(pTmgrGlobal->getMemoryTaint<lengthInBits>(writeAddress)) 
            : ObjectSource(lengthInBits, destValue);

        fTaintLOGICAL(pTmgrTls, MK_TAINT_OBJECT_PTR(
            X_AND,
            objSrc,
            objSrcDest)); 
    }
} // sTEST_RM

template<UINT32 lengthInBits> void LOGICAL::sTEST_MR
    (THREADID tid, ADDRINT readAddress, REG regSrcDest, ADDRINT destValue, ADDRINT insAddress) 
{   
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);

    ADDRINT srcValue = getMemoryValue<lengthInBits>(readAddress);

    bool isFullDestTainted = pTmgrTls->isRegisterTainted<lengthInBits>(regSrcDest); 
    bool isFullSrcTainted  = pTmgrGlobal->isMemoryTainted<lengthInBits>(readAddress);
    
    // cas particuliers du AND (cf tableau, avec simplification par loi Morgan)
    /* if ((!isFullDestTainted && !isFullSrcTainted) \
        || (!isFullDestTainted && !destValue) \
        || (!isFullSrcTainted && !srcValue) ) => 11 op�rations */
    //  (!a && !b) || (!a && !c) || (!b || !d) <=> ( !(a && (b || d) || (b && c))
    //  PLUS QUE 5 OPERATIONS                                       
    if (!((isFullDestTainted && (isFullSrcTainted || (srcValue != 0))) || (isFullSrcTainted && (destValue != 0)))) 
    {
        pTmgrTls->unTaintAllFlags();
    }
    else 
    {
        _LOGTAINT(tid, insAddress, "testMR" + decstr(lengthInBits));

        // MARQUAGE DES FLAGS, d�pendant uniquement du r�sultat 

        // source 1 : source
        ObjectSource objSrc = (isFullSrcTainted)
            ? ObjectSource(pTmgrGlobal->getMemoryTaint<lengthInBits>(readAddress))
            : ObjectSource(lengthInBits, srcValue);
        
        // source 2 : source/destination
        ObjectSource objSrcDest = (isFullDestTainted)
            ? ObjectSource(pTmgrTls->getRegisterTaint<lengthInBits>(regSrcDest, destValue)) 
            : ObjectSource(lengthInBits, destValue);
        
        fTaintLOGICAL(pTmgrTls, MK_TAINT_OBJECT_PTR(
            X_AND,
            objSrc,
            objSrcDest));
    }
} // sTEST_MR

template<UINT32 lengthInBits> void LOGICAL::sTEST_RR
    (THREADID tid, REG regSrc, ADDRINT srcValue, REG regSrcDest, ADDRINT destValue, ADDRINT insAddress) 
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);

    bool isFullDestTainted = pTmgrTls->isRegisterTainted<lengthInBits>(regSrcDest); 
    bool isFullSrcTainted  = pTmgrTls->isRegisterTainted<lengthInBits>(regSrc);
    
    // cas particuliers du AND (cf tableau, avec simplification par loi Morgan)
    /* if ((!isFullDestTainted && !isFullSrcTainted) \
        || (!isFullDestTainted && !destValue) \
        || (!isFullSrcTainted && !srcValue) ) => 11 op�rations */
    //  (!a && !b) || (!a && !c) || (!b || !d) <=> ( !(a && (b || d) || (b && c))
    //  PLUS QUE 5 OPERATIONS                                       
    if (!((isFullDestTainted && (isFullSrcTainted || (srcValue != 0))) || (isFullSrcTainted && (destValue != 0)))) 
    {
        pTmgrTls->unTaintAllFlags();
    }
    else 
    {
        _LOGTAINT(tid, insAddress, "testRR" + decstr(lengthInBits));

        // MARQUAGE DES FLAGS, d�pendant uniquement du r�sultat 

        // source 1 : source
        ObjectSource objSrc = (isFullSrcTainted)
            ? ObjectSource(pTmgrTls->getRegisterTaint<lengthInBits>(regSrc, srcValue)) 
            : ObjectSource(lengthInBits, srcValue);
        
        // source 2 : source/destination
        ObjectSource objSrcDest = (isFullDestTainted)
            ? ObjectSource(pTmgrTls->getRegisterTaint<lengthInBits>(regSrcDest, destValue)) 
            : ObjectSource(lengthInBits, destValue);
        
        fTaintLOGICAL(pTmgrTls, MK_TAINT_OBJECT_PTR(
            X_AND,
            objSrc,
            objSrcDest));
    }
} // sTEST_RR

template<UINT32 lengthInBits> void LOGICAL::sTEST_RR_EQUAL(THREADID tid, REG regSrc, ADDRINT srcValue, ADDRINT insAddress) 
{
    // cas particulier des registres �gaux : moins d'arguments pass�s
    // et marquage des flags simplifi�
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);

    if (!pTmgrTls->isRegisterTainted<lengthInBits>(regSrc))  pTmgrTls->unTaintAllFlags();
    else 
    {
        _LOGTAINT(tid, insAddress, "testRR_EQUAL" + decstr(lengthInBits));

        // marquage des flags avec la source uniquement (AND a, a = a)
        fTaintLOGICAL(pTmgrTls, pTmgrTls->getRegisterTaint<lengthInBits>(regSrc, srcValue));
    }
} // sTEST_RR_EQUAL

///////////////
///// NOT /////
///////////////

// SIMULATE
template<UINT32 lengthInBits> void LOGICAL::sNOT_M(ADDRINT writeAddress, ADDRINT insAddress) 
{
    // pas besoin du thread (rien sur les flags ni registres)
    if (pTmgrGlobal->isMemoryTainted<lengthInBits>(writeAddress)) 
    {
        _LOGTAINT(PIN_ThreadId(), insAddress, "notM" + decstr(lengthInBits));
        ADDRINT lastAddress = writeAddress + (lengthInBits >> 3);
        do
        {   
            // si octet marqu�, marquage dest = NOT(dest), sinon rien
            if (pTmgrGlobal->isMemoryTainted<8>(writeAddress)) 
            {   
                pTmgrGlobal->updateMemoryTaint<8>(writeAddress, std::make_shared<TaintByte>(
                    X_NOT,
                    ObjectSource(pTmgrGlobal->getMemoryTaint<8>(writeAddress))));
            }
        } while (++writeAddress < lastAddress);
    }
} // sNOT_M

template<UINT32 lengthInBits> void LOGICAL::sNOT_R(THREADID tid, REG reg, ADDRINT insAddress) 
{
    // masque binaire du marquage de la source
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);

    if (pTmgrTls->isRegisterTainted<lengthInBits>(reg)) // si registre marqu�
    {
        _LOGTAINT(tid, insAddress, "notR" + decstr(lengthInBits));
        REGINDEX regIndex = getRegIndex(reg);
        for (UINT32 regPart = 0 ; regPart < (lengthInBits >> 3) ; ++regPart)       
        {
            // si octet marqu�, marquage dest = NOT(dest)
            if (pTmgrTls->isRegisterPartTainted(regIndex, regPart))   
            {   
                pTmgrTls->updateTaintRegisterPart(regIndex, regPart, std::make_shared<TaintByte>(
                    X_NOT,
                    ObjectSource(pTmgrTls->getRegisterPartTaint(regIndex, regPart))));
            }
        }
    }
} // sNOT_R
