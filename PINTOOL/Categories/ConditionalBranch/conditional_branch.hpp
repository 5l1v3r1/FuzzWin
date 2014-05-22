
/***********************/
/*** JUMP CX/ECX/RCX ***/
/***********************/

template<UINT32 lengthInBits>
void CONDITIONAL_BR::sJRCXZ(THREADID tid, bool isTaken, ADDRINT registerValue, ADDRINT insAddress)
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    REG reg        = RegisterCount<lengthInBits>::getReg();
    PREDICATE pred = RegisterCount<lengthInBits>::getPredicate();

    if (pTmgrTls->isRegisterTainted<lengthInBits>(reg)) 
    {
        _LOGTAINT(tid, insAddress, "JRCXZ");
        g_pFormula->addConstraintJcc(pTmgrTls, pred, isTaken, insAddress, registerValue);
    }
}

/************/
/*** LOOP ***/
/************/

template<UINT32 lengthInBits>
void CONDITIONAL_BR::sLOOP(THREADID tid, bool isTaken, ADDRINT regValueAfterLoop, ADDRINT insAddress)
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    REG regCounter = RegisterCount<lengthInBits>::getReg();

    // traitement uniquement si le compteur est marqu�
    if (pTmgrTls->isRegisterTainted<lengthInBits>(regCounter)) 
    {
        // r�cup�ration du marquage du registre compteur
        // ATTENTION ON INSTRUMENTE APRES L'INSTRUCTION DONC LA VALEUR NUMERIQUE 
        // DU REGISTRE A DEJA ETE DECREMENTEE => sa valeur avant ex�cution vaut + 1
        ObjectSource objRegCounter(pTmgrTls->getRegisterTaint<lengthInBits>(regCounter, regValueAfterLoop + 1));
        
        // d�cr�ment du compteur
        TAINT_OBJECT_PTR tRegCounterPtr = MK_TAINT_OBJECT_PTR(X_DEC, objRegCounter);

        // mise � jour du marquage du registre
        pTmgrTls->updateTaintRegister<lengthInBits>(regCounter, tRegCounterPtr);
        
        // insertion d'une contrainte uniquement si la branche est prise
        // afin d'essayer de raccourcir le nombre de tours de boucle
        if (isTaken) 
        {
            _LOGTAINT(tid, insAddress, "LOOP");
            g_pFormula->addConstraintLoop(tRegCounterPtr, insAddress);
        }
    }
}

template<UINT32 lengthInBits>
void CONDITIONAL_BR::sLOOPE(THREADID tid, bool isTaken, ADDRINT regValueAfterLoop, 
                            ADDRINT flagsValue, ADDRINT insAddress)
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    REG regCounter   = RegisterCount<lengthInBits>::getReg();
    bool isZFTainted = pTmgrTls->isZeroFlagTainted();

    // cas o� le compteur est marqu� 
    if (pTmgrTls->isRegisterTainted<lengthInBits>(regCounter)) 
    {
        // r�cup�ration du marquage du registre compteur
        // ATTENTION ON INSTRUMENTE APRES L'INSTRUCTION DONC LA VALEUR NUMERIQUE 
        // DU REGISTRE A DEJA ETE DECREMENTEE => sa valeur avant ex�cution vaut + 1
        ObjectSource objRegCounterBefore(pTmgrTls->getRegisterTaint<lengthInBits>(regCounter, regValueAfterLoop + 1));
       
        // d�cr�ment du compteur
        TAINT_OBJECT_PTR regCounterPtr = MK_TAINT_OBJECT_PTR(X_DEC, objRegCounterBefore);

        // affectation
        pTmgrTls->updateTaintRegister<lengthInBits>(regCounter, regCounterPtr);
       
        // insertion d'une contrainte uniquement si la branche est prise
        // afin d'essayer de raccourcir le nombre de tours de boucle
        if (isTaken)
        {
            _LOGTAINT(tid, insAddress, "LOOPE");

            ObjectSource objZF = isZFTainted
                ? ObjectSource(pTmgrTls->getTaintZeroFlag())
                : ObjectSource(1, EXTRACTBIT(flagsValue, ZERO_FLAG));
            ObjectSource objRegCounterAfter(regCounterPtr);

            // version surcharg�e de la m�thode pour LOOPE/LOOPNE
            g_pFormula->addConstraintLoop(PREDICATE_ZERO, objRegCounterAfter, objZF, insAddress);
        }
    }
    // si le registre n'est pas marqu�, mais ZF oui, et que la branche est prise,
    // alors une contrainte doit �galement etre ajout�e 
    else if (isZFTainted && isTaken)
    {
        _LOGTAINT(tid, insAddress, "LOOPE (compteur non marqu�)");

        ObjectSource objRegCounterAfter(lengthInBits, regValueAfterLoop);
        ObjectSource objZF(pTmgrTls->getTaintZeroFlag());

        // version surcharg�e de la m�thode pour LOOPE/LOOPNE
        g_pFormula->addConstraintLoop(PREDICATE_ZERO, objRegCounterAfter, objZF, insAddress);      
    }
}

template<UINT32 lengthInBits>
void CONDITIONAL_BR::sLOOPNE(THREADID tid, bool isTaken, ADDRINT regValueAfterLoop, 
                             ADDRINT flagsValue, ADDRINT insAddress)
{
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    REG regCounter   = RegisterCount<lengthInBits>::getReg();
    bool isZFTainted = pTmgrTls->isZeroFlagTainted();

    // cas o� le compteur est marqu� 
    if (pTmgrTls->isRegisterTainted<lengthInBits>(regCounter)) 
    {
        // r�cup�ration du marquage du registre compteur
        // ATTENTION ON INSTRUMENTE APRES L'INSTRUCTION DONC LA VALEUR NUMERIQUE 
        // DU REGISTRE A DEJA ETE DECREMENTEE => sa valeur avant ex�cution vaut + 1
        ObjectSource objRegCounterBefore(pTmgrTls->getRegisterTaint<lengthInBits>(regCounter, regValueAfterLoop + 1));
       
        // d�cr�ment du compteur
        TAINT_OBJECT_PTR regCounterPtr = MK_TAINT_OBJECT_PTR(X_DEC, objRegCounterBefore);

        // affectation
        pTmgrTls->updateTaintRegister<lengthInBits>(regCounter, regCounterPtr);
       
        // insertion d'une contrainte uniquement si la branche est prise
        // afin d'essayer de raccourcir le nombre de tours de boucle
        if (isTaken)
        {
            _LOGTAINT(tid, insAddress, "LOOPNE");

            ObjectSource objZF = isZFTainted
                ? ObjectSource(pTmgrTls->getTaintZeroFlag())
                : ObjectSource(1, EXTRACTBIT(flagsValue, ZERO_FLAG));
            ObjectSource objRegCounterAfter(regCounterPtr);

            // version surcharg�e de la m�thode pour LOOPE/LOOPNE
            g_pFormula->addConstraintLoop(PREDICATE_NOT_ZERO, objRegCounterAfter, objZF, insAddress);
        }
    }
    // si le registre n'est pas marqu�, mais ZF oui, et que la branche est prise,
    // alors une contrainte doit �galement etre ajout�e 
    else if (isZFTainted && isTaken)
    {
        _LOGTAINT(tid, insAddress, "LOOPNE (compteur non marqu�)");

        ObjectSource objRegCounterAfter(lengthInBits, regValueAfterLoop);
        ObjectSource objZF(pTmgrTls->getTaintZeroFlag());

        // version surcharg�e de la m�thode pour LOOPE/LOOPNE
        g_pFormula->addConstraintLoop(PREDICATE_NOT_ZERO, objRegCounterAfter, objZF, insAddress);      
    }
}