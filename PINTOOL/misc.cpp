#include "MISC.h"
#include "utils.h"

/////////
// LEA //
/////////
// CALLBACKS
void MISC::cLEA(INS &ins) 
{
    REG regDest        = INS_RegW(ins, 0);               // registre de destination 
    UINT32 regDestSize = getRegSize(regDest);            // taille du registre dest ("Operand Size") 
    UINT32 addrSize    = INS_EffectiveAddressWidth(ins); // taille de l'adresse ("Address Size")
    void (*callback)() = nullptr;                        // pointeur sur la fonction a appeler
    
    // 1ER TEMPS : calcul de l'objet repr�sentant l'adresse effective utilis�e.
    // cela va ins�rer une fonction d'instrumentation qui va stocker l'objet dans TaintManager
    UTILS::cGetKindOfEA(ins);

    // seconde fonction d'analyse selon le couple OpSize/AddrSize
    
#if TARGET_IA32
    /** CAS 32BITS : 16/16, 16/32, 32/16 et 32/32 **/
    // cas 16/??
    if      (regDestSize == 2) callback = (AFUNPTR) ((16 == addrSize) ? sLEA<16,16> : sLEA<16,32>);
    // cas 32/??
    else if (regDestSize == 4) callback = (AFUNPTR) ((16 == addrSize) ? sLEA<32,16> : sLEA<32,32>);
    // si le registre destination n'est pas g�r�, ne pas instrumenter
    else return; 
#else
    /** CAS 64BITS : 16/32, 16/64, 32/32, 32/64, 64/32, 64/64 **/
    // cas 16/??
    if      (regDestSize == 2) callback = (AFUNPTR) ((32 == addrSize) ? sLEA<16,32> : sLEA<16,64>);
    // cas 32/??
    else if (regDestSize == 4) callback = (AFUNPTR) ((32 == addrSize) ? sLEA<32,32> : sLEA<16,64>);
    // cas 64/??
    else if (regDestSize == 8) callback = (AFUNPTR) ((32 == addrSize) ? sLEA<32,32> : sLEA<32,64>);
    // si le registre destination n'est pas g�r�, ne pas instrumenter
    else return; 
#endif

    INS_InsertCall(ins, IPOINT_BEFORE, callback,
                IARG_THREAD_ID,
                IARG_UINT32,    regDest, // registre de destination 
                CALLBACK_DEBUG IARG_END);
}   // sLEA

///////////
// LEAVE //
///////////

#if TARGET_IA32
void MISC::cLEAVE(INS &ins) 
{
    // LEAVE (32bits) <=> MOV (E)SP, (E)BP + POP (E)BP (POP simul� en MOVMR)
    // par d�faut sur 32 bits
    void (*cMOV)() = (AFUNPTR) DATAXFER::sMOV_RR<32>;
    void (*cPOP)() = (AFUNPTR) DATAXFER::sMOV_MR<32>;
    REG regEbp = REG_EBP;
    REG regEsp = REG_ESP;
 
    if (INS_MemoryReadSize(ins) == 2) // sur 16bit : changement en BP et SP
    {
        cMOV = (AFUNPTR) DATAXFER::sMOV_RR<16>;
        cPOP = (AFUNPTR) DATAXFER::sMOV_MR<16>;
        regEbp = REG_BP;
        regEsp = REG_SP;
    }

    INS_InsertCall(ins, IPOINT_BEFORE, cMOV,
        IARG_CALL_ORDER, CALL_ORDER_FIRST,
        IARG_THREAD_ID,
        IARG_UINT32, regEbp,   // registre source = (E)BP
        IARG_UINT32, regEsp,   // registre de destination = (E)SP 
        CALLBACK_DEBUG IARG_END);

    INS_InsertCall(ins, IPOINT_BEFORE, cPOP,
        IARG_CALL_ORDER, CALL_ORDER_LAST,
        IARG_THREAD_ID,
        IARG_REG_VALUE, regEbp, // ATTENTION : valeur d'ESP � ce moment l�  = EBP (suite au MOV plus haut) 
        IARG_UINT32,    regEbp, // registre de destination
        CALLBACK_DEBUG IARG_END);
}
#else
void MISC::cLEAVE(INS &ins) 
{
    // LEAVE (64bits) <=> MOV RSP, RBP + POP RBP (POP simul� en MOVMR)
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) DATAXFER::sMOV_RR<64>,
        IARG_CALL_ORDER, CALL_ORDER_FIRST,
        IARG_THREAD_ID,
        IARG_UINT32, REG_RBP,   // registre source = RBP
        IARG_UINT32, REG_RSP,   // registre de destination = RSP 
        CALLBACK_DEBUG IARG_END);

    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) DATAXFER::sMOV_MR<64>,
        IARG_CALL_ORDER, CALL_ORDER_LAST,
        IARG_THREAD_ID,
        IARG_REG_VALUE, REG_RBP, // ATTENTION : valeur d'ESP � ce moment l�  = EBP (suite au MOV plus haut)
        IARG_UINT32,    REG_RBP, // registre de destination
        CALLBACK_DEBUG IARG_END);
}
#endif
