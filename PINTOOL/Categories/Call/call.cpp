#include "call.h"

void CALL::cCALL(INS &ins, bool isFarCall)
{
    void (*callback)() = nullptr;

    /* CALL est �tudi� lorsqu'il est possible d'influer sur l'adresse du saut
    donc les cas "CALL relative" (adresse imm�diate ou relative � EIP) sont non trait�s */

    if (INS_IsDirectCall(ins))
    {
        _LOGDEBUG("CALL DIRECT NON SUIVI " + decstr(INS_MemoryWriteSize(ins)));
        // d�marquage de la pile : impossible de faire appel � uMEM
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) UTILS::uMEM, 
            IARG_FAST_ANALYSIS_CALL,  
            IARG_MEMORYWRITE_EA,
            IARG_MEMORYWRITE_SIZE,
            IARG_END);

        return;
    }

    /* traiter deux cas:
      1) CALL [EFFECTIVE ADDRESS] : il faut tester � la fois si le calcul de l'adresse
         effective est marqu� (appel � cGetKindOfEA) et si l'adresse du saut, figurant
         � l'adresse effective, est marqu�e
         qui va v�rifier si le calcul de l'EA est marqu� ou non
      2) CALL REG : il faut tester le marquage du registre concern�. si c'est le cas
         il faudra essayer de changer sa valeur pour sauter autre part */

    // la diff�rence entre call NEAR et FAR, au niveua marquage, est la taille
    // du d�marquage de la pile : le call FAR stocke CS (16 bit) en plus sur la pile

    // cas n�1 : CALL [EFFECTIVE ADDRESS] 
    if (INS_OperandIsMemory(ins, 0)) 
    {
        if (isFarCall)
        {
            _LOGDEBUG("CALL_M (FAR)");
        }
        else  _LOGDEBUG("CALL_M (NEAR)");

        // test du marquage de l'adresse effective par insertion de callbacks sp�cifiques
        UTILS::cGetKindOfEA(ins);

        // taille de lecture de la m�moire (16, 32 ou 64 bits)
        switch (INS_MemoryReadSize(ins))
        {
        // case 1:	impossible
        case 2:	callback = (AFUNPTR) sCALL_M<16>; break;
        case 4:	callback = (AFUNPTR) sCALL_M<32>; break;
        #if TARGET_IA32E
        case 8: callback = (AFUNPTR) sCALL_M<64>; break;
        #endif
        }

        INS_InsertCall(ins, IPOINT_BEFORE, callback, 
            IARG_THREAD_ID,
            IARG_BOOL, isFarCall,   // call NEAR ou FAR
            IARG_MEMORYREAD_EA,             // adresse ou se trouve l'adresse de saut
            IARG_BRANCH_TARGET_ADDR,        // adresse de saut     
            IARG_REG_VALUE, REG_STACK_PTR,  // adresse de la pile (pour d�marquage)
            IARG_INST_PTR, IARG_END);
    } 
    // cas n�2 : CALL 'REG'
    else
    {
        if (isFarCall)
        {
            _LOGDEBUG("CALL_R (FAR)");
        }
        else  _LOGDEBUG("CALL_R (NEAR)");

        REG reg = INS_OperandReg(ins, 0);

        switch (getRegSize(reg))
        {
        // case 1:	impossible
        case 2:	callback = (AFUNPTR) sCALL_R<16>; break;
        case 4:	callback = (AFUNPTR) sCALL_R<32>; break;
        #if TARGET_IA32E
        case 8: callback = (AFUNPTR) sCALL_R<64>; break;
        #endif
        }
      
        INS_InsertCall(ins, IPOINT_BEFORE, callback, 
            IARG_THREAD_ID,   
            IARG_BOOL,      isFarCall,   // call NEAR ou FAR
            IARG_UINT32,    reg,     // registre d�finissant l'adresse de saut
            IARG_BRANCH_TARGET_ADDR, // adresse de saut (=valeur du registre masqu� � 16/32/64bits)  
            IARG_REG_VALUE, REG_STACK_PTR,  // adresse de la pile (pour d�marquage)
            IARG_INST_PTR, IARG_END);
    } 
}

