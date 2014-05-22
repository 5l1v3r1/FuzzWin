#include "unconditional_branch.h"

// CALLBACKS
void UNCONDITIONAL_BR::cJMP(INS &ins) 
{
    void (*callback)() = nullptr;
    
    /* JMP est �tudi� lorsqu'il est possible d'influer sur l'adresse du saut
    donc les cas "JMP relative" (adresse imm�diate ou relative � EIP) sont non trait�s */

    if (INS_IsDirectBranch(ins)) return;

    /* dans le cas contraire il reste � traiter deux cas:
      1) JMP [EFFECTIVE ADRESS] : il faut tester � la fois si le calcul de l'adresse
         effective est marqu� (appel � cGetKindOfEA) et si l'adresse du saut, figurant
         � l'adresse effective, est marqu�e
         qui va v�rifier si le calcul de l'EA est marqu� ou non
      2) JMP REG : il faut tester le marquage du registre concern�. si c'est le cas
         il faudra essayer de changer sa valeur pour sauter autre part */

    // cas n�1 : JMP [EFFECTIVE ADDRESS] 
    if (INS_OperandIsMemory(ins, 0)) 
    {
        // test du marquage de l'adresse effective par insertion de callbacks sp�cifiques
        UTILS::cGetKindOfEA(ins);

        // taille de lecture de la m�moire (16, 32 ou 64 bits)
        switch (INS_MemoryReadSize(ins))
        {
        // case 1:	impossible
        case 2:	callback = (AFUNPTR) sJMP_M<16>; break;
        case 4:	callback = (AFUNPTR) sJMP_M<32>; break;
        #if TARGET_IA32E
        case 8: callback = (AFUNPTR) sJMP_M<64>; break;
        #endif
        }

        // insertion du callback pour le test du marquage de l'adresse point�e par l'EA
        INS_InsertCall(ins, IPOINT_BEFORE, callback, 
            IARG_THREAD_ID,
            IARG_MEMORYREAD_EA,    // adresse ou se trouve l'adresse de saut
            IARG_BRANCH_TARGET_ADDR,    // adresse de saut     
            IARG_INST_PTR, IARG_END);
    } 
    // cas n�2 : JMP 'REG'
    else
    {
        REG reg = INS_OperandReg(ins, 0);

        switch (getRegSize(reg))
        {
        // case 1:	impossible
        case 2:	callback = (AFUNPTR) sJMP_R<16>; break;
        case 4:	callback = (AFUNPTR) sJMP_R<32>; break;
        #if TARGET_IA32E
        case 8: callback = (AFUNPTR) sJMP_R<64>; break;
        #endif
        }
      
        INS_InsertCall(ins, IPOINT_BEFORE, callback, 
            IARG_THREAD_ID,   
            IARG_UINT32,    reg,     // registre d�finissant l'adresse de saut
            IARG_BRANCH_TARGET_ADDR, // adresse de saut   
            IARG_INST_PTR, IARG_END);
    }
} // cJMP
