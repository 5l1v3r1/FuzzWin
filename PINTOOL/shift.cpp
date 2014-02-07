#include "SHIFT.h"

// FLAGS
// cas particuliers par rapport au manuel Intel : 
// pour AF : "undefined" selon Intel => d�marquage dans FuzzWin
// pour OF : "undefined" si d�placement != 1 => d�marquage dans FuzzWin

// d�marquage OZSAP
void SHIFT::fUnTaintOZSAP(TaintManager_Thread *pTmgrTls)
{
    pTmgrTls->unTaintParityFlag(); 
    pTmgrTls->unTaintZeroFlag();
    pTmgrTls->unTaintSignFlag();  
    pTmgrTls->unTaintOverflowFlag(); 
    pTmgrTls->unTaintAuxiliaryFlag();
}

// Les instructions SHIFT sont trait�es de mani�re tr�s pr�cise, afin
// de limiter les faux-positifs en marquage.
// deux cas sont distingu�s : d�placement par valeur ou par registre (CL)
// si CL n'est pas marqu�, l'instruction sera trait�e comme un d�placement par valeur

// plusieurs cas sont envisag�s
// d�placement num�rique nul => aucune action
// deplacement num�rique = 1 => cas sp�cifique marquage OF en plus
// d�placement num�rique multiple de 8 => traitement par MOV octet par octet
// d�placement num�rique sup�rieur � la taille destination => d�marquage
// 
// si d�placement avec CL marqu�, on ne pourra pas faire d'optimisations
// il y aura possibilit� de surmarquage selon la valeur r�elle de CL

// chaque proc�dure d'analyse SHIFT sera d�coup�e en plusieurs parties
// 0) (instrumentation) masquage du d�placement � 0x1f ou 0x3f selon archi
// 1) (analyse) test du marquage des op�randes (registre ET d�calage)
// 2) Marquage destination avec SHL/SHR/SAR
// 3) Proc�dure marquage flags selon instruction SHL/SHR/SAR
// 4) d�marquage de certaines parties de la destination, en fonction de 
//    la valeur du d�placement et de la taille du registre (uniquement pour
//    les d�placements num�riques)

/*********/
/** SHL **/
/*********/

// CALLBACKS
void SHIFT::cSHL(INS &ins) 
{
    void (*callback)() = nullptr;     // pointeur sur la fonction a appeler
    
    // DECALAGE PAR VALEUR : SHL_IM ou SHL_IR
    if (INS_OperandIsImmediate(ins, 1)) 
    {    
        // masquage du d�placement � 0x1f (meme en x64), sauf si op�rande 64bits (masquage � 0x3f) 
        UINT32 depl = static_cast<UINT32>(INS_OperandImmediate(ins, 1));
        UINT32 maskedDepl = depl & 0x1f;   
        
        if (INS_IsMemoryWrite(ins)) // DESTINATION = MEMOIRE (SHL_IM)
        {   
            switch (INS_MemoryWriteSize(ins)) 
            {
                case 1: callback = (AFUNPTR) sSHL_IM<8>;  break;
                case 2: callback = (AFUNPTR) sSHL_IM<16>; break;
                case 4: callback = (AFUNPTR) sSHL_IM<32>; break;
                #if TARGET_IA32E
                case 8: 
                    callback = (AFUNPTR) sSHL_IM<64>; 
                    maskedDepl = depl & 0x3f;   // masquage � 0x3f en 64bits
                    break;
                #endif
                default: return;
            } 
            // d�placement non nul : instrumentation (sinon aucun chgt)
            if (maskedDepl) INS_InsertCall (ins, IPOINT_BEFORE, callback,
                IARG_THREAD_ID,
                IARG_UINT32, maskedDepl,
                IARG_MEMORYWRITE_EA,   
                CALLBACK_DEBUG IARG_END);
        }
        else // DESTINATION = REGISTRE (SHL_IR)
        {         
            REG reg = INS_OperandReg(ins, 0); // registre qui sera d�cal� 
            switch (getRegSize(reg)) 
            {
                case 1: callback = (AFUNPTR) sSHL_IR<8>;  break;
                case 2: callback = (AFUNPTR) sSHL_IR<16>; break;
                case 4: callback = (AFUNPTR) sSHL_IR<32>; break;
                #if TARGET_IA32E
                case 8: 
                    callback = (AFUNPTR) sSHL_IR<64>; 
                    maskedDepl = depl & 0x3f;   // masquage � 0x3f en 64bits
                    break;
                #endif
                default: return;
            } 
            
            // d�placement non nul : instrumentation (sinon aucun chgt)
            if (maskedDepl) INS_InsertCall (ins, IPOINT_BEFORE, callback,
                IARG_THREAD_ID,
                IARG_UINT32, maskedDepl,
                IARG_UINT32,    reg,    // registre d�cal�
                IARG_REG_VALUE, reg,    // sa valeur lors du callback
                CALLBACK_DEBUG IARG_END);
        }
    }
    // DECALAGE PAR REGISTRE : SHL_RM ou SHL_RR
    else 
    {  
        // le masquage � 0x1f ou 0x3f du registre sera fait dans le callback
        if (INS_IsMemoryWrite(ins)) // DESTINATION = MEMOIRE (SHL_RM)
        {   
            switch (INS_MemoryWriteSize(ins)) 
            {
                case 1: callback = (AFUNPTR) sSHL_RM<8>;  break;
                case 2: callback = (AFUNPTR) sSHL_RM<16>; break;
                case 4: callback = (AFUNPTR) sSHL_RM<32>; break;
                #if TARGET_IA32E
                case 8: callback = (AFUNPTR) sSHL_RM<64>; break;
                #endif
                default: return;
            } 
            INS_InsertCall (ins, IPOINT_BEFORE, callback,
                IARG_THREAD_ID,
                IARG_REG_VALUE, REG_CL, // valeur num�rique du d�placement
                IARG_MEMORYWRITE_EA,   
                CALLBACK_DEBUG IARG_END);
        }
        else // DESTINATION = REGISTRE (SHL_RR)
        {         
            REG reg = INS_OperandReg(ins, 0); // registre qui sera d�cal� 
            switch (getRegSize(reg)) 
            {
                case 1: callback = (AFUNPTR) sSHL_RR<8>;  break;
                case 2: callback = (AFUNPTR) sSHL_RR<16>; break;
                case 4: callback = (AFUNPTR) sSHL_RR<32>; break;
                #if TARGET_IA32E
                case 8: callback = (AFUNPTR) sSHL_RR<64>; break;
                #endif
                default: return;
            } 
            
            INS_InsertCall (ins, IPOINT_BEFORE, callback,
                IARG_THREAD_ID,
                IARG_REG_VALUE, REG_CL, // valeur num�rique du d�placement
                IARG_UINT32,    reg,    // registre d�cal�
                IARG_REG_VALUE, reg,    // sa valeur lors du callback
                CALLBACK_DEBUG IARG_END);
        }
    }
} // cSHL

/** FLAGS **/
// marquage flags sp�cifique pour SHL, d�placement non marqu�
void SHIFT::fSHL(TaintManager_Thread *pTmgrTls, const TaintPtr &resultPtr, const ObjectSource &objSrcShiftedSrc,
          UINT32 maskedDepl)
{
    ObjectSource objResult(resultPtr);
    
    // d�marquage AF (undefined selon Intel)
    pTmgrTls->unTaintAuxiliaryFlag();
    // marquage ZF et SF
    pTmgrTls->updateTaintZeroFlag(std::make_shared<TaintBit>(F_IS_NULL, objResult));
    pTmgrTls->updateTaintSignFlag(std::make_shared<TaintBit>(F_MSB,     objResult));

    // marquage Carry : bit (length-depl) de la source
    // peut quand meme provoquer un surmarquage si l'octet concern� n'est pas marqu�
    // mais le test de marquage serait trop gourmand en temps d'execution
    pTmgrTls->updateTaintCarryFlag(std::make_shared<TaintBit>(
        EXTRACT,
        objSrcShiftedSrc,
        ObjectSource(8, resultPtr->getLength() - maskedDepl)));

    // marquage PF ssi deplacement inf�rieur � 8 (sinon que des 0)
    if (maskedDepl < 8)  pTmgrTls->updateTaintParityFlag(std::make_shared<TaintBit>(F_PARITY, objResult));
    else pTmgrTls->unTaintParityFlag();

    // marquage Overflow, ssi depl = 1
    if (maskedDepl == 1)  pTmgrTls->updateTaintOverflowFlag(std::make_shared<TaintBit>(
            F_OVERFLOW_SHL,
            objResult,
            objSrcShiftedSrc));
    else pTmgrTls->unTaintOverflowFlag();
} // fSHL

// marquage flags sp�cifique pour SHL, d�placement marqu�
void SHIFT::fSHL(TaintManager_Thread *pTmgrTls, const TaintPtr &resultPtr, const ObjectSource &objSrcShiftedSrc,
          const ObjectSource &objTbCount)
{
    ObjectSource objResult(resultPtr);
    
    // d�marquage AF (undefined selon Intel)
    pTmgrTls->unTaintAuxiliaryFlag();
    // marquage ZF et SF
    pTmgrTls->updateTaintZeroFlag(std::make_shared<TaintBit>(F_IS_NULL, objResult));
    pTmgrTls->updateTaintSignFlag(std::make_shared<TaintBit>(F_MSB,     objResult));

    // marquage PF 
    pTmgrTls->updateTaintParityFlag(std::make_shared<TaintBit>(F_PARITY,  objResult));
        
    // marquage Carry : dernier bit de la source �ject� suite au shift       
    pTmgrTls->updateTaintCarryFlag(std::make_shared<TaintBit>(
        F_CARRY_SHL,
        objSrcShiftedSrc,
        objTbCount));

    // d�marquage Overflow (tant pis si le d�placement vaut 1...il y aura sous marquage)
    pTmgrTls->unTaintOverflowFlag();
} // fSHL

/*********/
/** SHR **/
/*********/

// CALLBACKS
void SHIFT::cSHR(INS &ins) 
{
    void (*callback)() = nullptr;     // pointeur sur la fonction a appeler
    
    // DECALAGE PAR VALEUR : SHR_IM ou SHR_IR
    if (INS_OperandIsImmediate(ins, 1)) 
    {    
        // masquage du d�placement par d�faut � 0x1f
        // sera modifi� par la suite si op�rande 64bits 
        UINT32 depl = static_cast<UINT32>(INS_OperandImmediate(ins, 1));
        UINT32 maskedDepl = depl & 0x1f;   
        
        if (INS_IsMemoryWrite(ins)) // DESTINATION = MEMOIRE (SHR_IM)
        {   
            switch (INS_MemoryWriteSize(ins)) 
            {
                case 1: callback = (AFUNPTR) sSHR_IM<8>;  break;
                case 2: callback = (AFUNPTR) sSHR_IM<16>; break;
                case 4: callback = (AFUNPTR) sSHR_IM<32>; break;
                #if TARGET_IA32E
                case 8: 
                    callback = (AFUNPTR) sSHR_IM<64>; 
                    maskedDepl = depl & 0x3f;   // masquage � 0x3f en 64bits
                    break;
                #endif
                default: return;
            } 
            // d�placement non nul : instrumentation (sinon aucun chgt)
            if (maskedDepl) INS_InsertCall (ins, IPOINT_BEFORE, callback,
                IARG_THREAD_ID,
                IARG_UINT32, maskedDepl,
                IARG_MEMORYWRITE_EA,   
                CALLBACK_DEBUG IARG_END);
        }
        else // DESTINATION = REGISTRE (SHR_IR)
        {         
            REG reg = INS_OperandReg(ins, 0); // registre qui sera d�cal� 
            switch (getRegSize(reg)) 
            {
                case 1: callback = (AFUNPTR) sSHR_IR<8>;  break;
                case 2: callback = (AFUNPTR) sSHR_IR<16>; break;
                case 4: callback = (AFUNPTR) sSHR_IR<32>; break;
                #if TARGET_IA32E
                case 8: 
                    callback = (AFUNPTR) sSHR_IR<64>; 
                    maskedDepl = depl & 0x3f;   // masquage � 0x3f en 64bits
                    break;
                #endif
                default: return;
            }          
            // d�placement non nul : instrumentation (sinon aucun chgt)
            if (maskedDepl) INS_InsertCall (ins, IPOINT_BEFORE, callback,
                IARG_THREAD_ID,
                IARG_UINT32, maskedDepl,
                IARG_UINT32,    reg,    // registre d�cal�
                IARG_REG_VALUE, reg,    // sa valeur lors du callback
                CALLBACK_DEBUG IARG_END);
        }
    }
    // DECALAGE PAR REGISTRE : SHR_RM ou SHR_RR
    else 
    {  
        // le masquage � 0x1f ou 0x3f du registre sera fait dans le callback
        if (INS_IsMemoryWrite(ins)) // DESTINATION = MEMOIRE (SHR_RM)
        {   
            switch (INS_MemoryWriteSize(ins)) 
            {
                case 1: callback = (AFUNPTR) sSHR_RM<8>;  break;
                case 2: callback = (AFUNPTR) sSHR_RM<16>; break;
                case 4: callback = (AFUNPTR) sSHR_RM<32>; break;
                #if TARGET_IA32E
                case 8: callback = (AFUNPTR) sSHR_RM<64>; break;
                #endif
                default: return;
            } 
            INS_InsertCall (ins, IPOINT_BEFORE, callback,
                IARG_THREAD_ID,
                IARG_REG_VALUE, REG_CL, // valeur num�rique du d�placement
                IARG_MEMORYWRITE_EA,   
                CALLBACK_DEBUG IARG_END);
        }
        else // DESTINATION = REGISTRE (SHR_RR)
        {         
            REG reg = INS_OperandReg(ins, 0); // registre qui sera d�cal� 
            switch (getRegSize(reg)) 
            {
                case 1: callback = (AFUNPTR) sSHR_RR<8>;  break;
                case 2: callback = (AFUNPTR) sSHR_RR<16>; break;
                case 4: callback = (AFUNPTR) sSHR_RR<32>; break;
                #if TARGET_IA32E
                case 8: callback = (AFUNPTR) sSHR_RR<64>; break;
                #endif
                default: return;
            } 
            
            INS_InsertCall (ins, IPOINT_BEFORE, callback,
                IARG_THREAD_ID,
                IARG_REG_VALUE, REG_CL, // valeur num�rique du d�placement
                IARG_UINT32,    reg,    // registre d�cal�
                IARG_REG_VALUE, reg,    // sa valeur lors du callback
                CALLBACK_DEBUG IARG_END);
        }
    }
} // cSHR

/** FLAGS **/
// marquage flags sp�cifique pour SHR, d�placement non marqu�
void SHIFT::fSHR(TaintManager_Thread *pTmgrTls, const TaintPtr &resultPtr, const ObjectSource &objSrcShiftedSrc,
          UINT32 maskedDepl)
{
    ObjectSource objResult(resultPtr);
    
    // d�marquage AF (undefined selon Intel) et SF (puisque insertion z�ros)
    pTmgrTls->unTaintAuxiliaryFlag();
    pTmgrTls->unTaintSignFlag();

    // marquage ZF et PF
    pTmgrTls->updateTaintZeroFlag  (std::make_shared<TaintBit>(F_IS_NULL, objResult));
    pTmgrTls->updateTaintParityFlag(std::make_shared<TaintBit>(F_PARITY,  objResult));
    
    // marquage Carry : bit (depl - 1) de la source
    // peut provoquer un surmarquage si l'octet concern� n'est pas marqu�
    // mais le test de marquage serait trop gourmand en temps d'execution
    pTmgrTls->updateTaintCarryFlag(std::make_shared<TaintBit>(
        EXTRACT,
        objSrcShiftedSrc,
        ObjectSource(8, maskedDepl - 1)));

    // marquage Overflow, ssi depl = 1 POUR SHR, OF vaut le bit de poids fort de la source (cf manuel Intel)
    if (maskedDepl == 1)  pTmgrTls->updateTaintOverflowFlag(std::make_shared<TaintBit>(
            F_MSB,
            objSrcShiftedSrc));
    else pTmgrTls->unTaintOverflowFlag();
} // fSHR

// marquage flags sp�cifique pour SHR, d�placement marqu�
void SHIFT::fSHR(TaintManager_Thread *pTmgrTls, const TaintPtr &resultPtr, const ObjectSource &objSrcShiftedSrc,
          const ObjectSource &objTbCount)
{
    ObjectSource objResult(resultPtr);
    
    // d�marquage AF (undefined selon Intel) et SF (puisque insertion z�ros)
    pTmgrTls->unTaintAuxiliaryFlag();
    pTmgrTls->unTaintSignFlag();

    // marquage ZF et PF
    pTmgrTls->updateTaintZeroFlag  (std::make_shared<TaintBit>(F_IS_NULL, objResult));
    pTmgrTls->updateTaintParityFlag(std::make_shared<TaintBit>(F_PARITY,  objResult));
    
    // marquage Carry : dernier bit de la source �ject� suite au shift       
    pTmgrTls->updateTaintCarryFlag(std::make_shared<TaintBit>(
        F_CARRY_SHR,
        objSrcShiftedSrc,
        objTbCount));

    // d�marquage Overflow (tant pis si le d�placement vaut 1...il y aura sous marquage)
    pTmgrTls->unTaintOverflowFlag();
} // fSHR

/*********/
/** SAR **/
/*********/

// CALLBACKS
void SHIFT::cSAR(INS &ins) 
{
    void (*callback)() = nullptr;     // pointeur sur la fonction a appeler
    
    // DECALAGE PAR VALEUR : SAR_IM ou SAR_IR
    if (INS_OperandIsImmediate(ins, 1)) 
    {    
        // masquage du d�placement par d�faut � 0x1f
        // sera modifi� par la suite si op�rande 64bits 
        UINT32 depl = static_cast<UINT32>(INS_OperandImmediate(ins, 1));
        UINT32 maskedDepl = depl & 0x1f;   
        
        if (INS_IsMemoryWrite(ins)) // DESTINATION = MEMOIRE (SAR_IM)
        {   
            switch (INS_MemoryWriteSize(ins)) 
            {
                case 1: callback = (AFUNPTR) sSAR_IM<8>;  break;
                case 2: callback = (AFUNPTR) sSAR_IM<16>; break;
                case 4: callback = (AFUNPTR) sSAR_IM<32>; break;
                #if TARGET_IA32E
                case 8: 
                    callback = (AFUNPTR) sSAR_IM<64>; 
                    maskedDepl = depl & 0x3f;   // masquage � 0x3f en 64bits
                    break;
                #endif
                default: return;
            } 
            // d�placement non nul : instrumentation (sinon aucun chgt)
            if (maskedDepl) INS_InsertCall (ins, IPOINT_BEFORE, callback,
                IARG_THREAD_ID,
                IARG_UINT32, maskedDepl,
                IARG_MEMORYWRITE_EA,   
                CALLBACK_DEBUG IARG_END);
        }
        else // DESTINATION = REGISTRE (SAR_IR)
        {         
            REG reg = INS_OperandReg(ins, 0); // registre qui sera d�cal� 
            switch (getRegSize(reg)) 
            {
                case 1: callback = (AFUNPTR) sSAR_IR<8>;  break;
                case 2: callback = (AFUNPTR) sSAR_IR<16>; break;
                case 4: callback = (AFUNPTR) sSAR_IR<32>; break;
                #if TARGET_IA32E
                case 8: 
                    callback = (AFUNPTR) sSAR_IR<64>; 
                    maskedDepl = depl & 0x3f;   // masquage � 0x3f en 64bits
                    break;
                #endif
                default: return;
            } 
            // d�placement non nul : instrumentation (sinon aucun chgt)
            if (maskedDepl) INS_InsertCall (ins, IPOINT_BEFORE, callback,
                IARG_THREAD_ID,
                IARG_UINT32, maskedDepl,
                IARG_UINT32,    reg,    // registre d�cal�
                IARG_REG_VALUE, reg,    // sa valeur lors du callback
                CALLBACK_DEBUG IARG_END);
        }
    }
    // DECALAGE PAR REGISTRE : SAR_RM ou SAR_RR
    else 
    {  
        // le masquage � 0x1f ou 0x3f sera fait dans le callback
        if (INS_IsMemoryWrite(ins)) // DESTINATION = MEMOIRE (SAR_RM)
        {   
            switch (INS_MemoryWriteSize(ins)) 
            {
                case 1: callback = (AFUNPTR) sSAR_RM<8>;  break;
                case 2: callback = (AFUNPTR) sSAR_RM<16>; break;
                case 4: callback = (AFUNPTR) sSAR_RM<32>; break;
                #if TARGET_IA32E
                case 8: callback = (AFUNPTR) sSAR_RM<64>; break;
                #endif
                default: return;
            } 
            INS_InsertCall (ins, IPOINT_BEFORE, callback,
                IARG_THREAD_ID,
                IARG_REG_VALUE, REG_CL, // valeur num�rique du d�placement
                IARG_MEMORYWRITE_EA,   
                CALLBACK_DEBUG IARG_END);
        }
        else // DESTINATION = REGISTRE (SAR_RR)
        {         
            REG reg = INS_OperandReg(ins, 0); // registre qui sera d�cal� 
            switch (getRegSize(reg)) 
            {
                case 1: callback = (AFUNPTR) sSAR_RR<8>;  break;
                case 2: callback = (AFUNPTR) sSAR_RR<16>; break;
                case 4: callback = (AFUNPTR) sSAR_RR<32>; break;
                #if TARGET_IA32E
                case 8: callback = (AFUNPTR) sSAR_RR<64>; break;
                #endif
                default: return;
            } 
            INS_InsertCall (ins, IPOINT_BEFORE, callback,
                IARG_THREAD_ID,
                IARG_REG_VALUE, REG_CL, // valeur num�rique du d�placement
                IARG_UINT32,    reg,    // registre d�cal�
                IARG_REG_VALUE, reg,    // sa valeur lors du callback
                CALLBACK_DEBUG IARG_END);
        }
    }
} // cSAR

/** FLAGS **/
// marquage flags sp�cifique pour SAR, d�placement non marqu�
void SHIFT::fSAR(TaintManager_Thread *pTmgrTls, const TaintPtr &resultPtr, const ObjectSource &objSrcShiftedSrc,
          UINT32 maskedDepl) 
{
    ObjectSource objResult(resultPtr);
    
    // d�marquage AF (undefined selon Intel) et OF (sp�cifique SAR) 
    pTmgrTls->unTaintAuxiliaryFlag();
    pTmgrTls->unTaintOverflowFlag();

    // marquage SF avec le signe de la source
    pTmgrTls->updateTaintSignFlag(std::make_shared<TaintBit>(F_MSB, objSrcShiftedSrc));

    // marquage ZF et PF
    pTmgrTls->updateTaintZeroFlag  (std::make_shared<TaintBit>(F_IS_NULL, objResult));
    pTmgrTls->updateTaintParityFlag(std::make_shared<TaintBit>(F_PARITY,  objResult));
    
    // marquage Carry : bit (depl - 1) de la source
    // peut provoquer un surmarquage si l'octet concern� n'est pas marqu�
    // mais le test de marquage serait trop gourmand en temps d'execution
    pTmgrTls->updateTaintCarryFlag(std::make_shared<TaintBit>(
        EXTRACT,
        objSrcShiftedSrc,
        ObjectSource(8, maskedDepl - 1))); 
} // fSAR 

// marquage flags sp�cifique pour SAR, d�placement marqu�
void SHIFT::fSAR(TaintManager_Thread *pTmgrTls, const TaintPtr &resultPtr, const ObjectSource &objSrcShiftedSrc,
          const ObjectSource &objTbCount) 
{
    ObjectSource objResult(resultPtr);
    
    // d�marquage AF (undefined selon Intel) et OF (sp�cifique SAR) 
    pTmgrTls->unTaintAuxiliaryFlag();
    pTmgrTls->unTaintOverflowFlag();

    // marquage SF avec le signe de la source
    pTmgrTls->updateTaintSignFlag(std::make_shared<TaintBit>(F_MSB, objSrcShiftedSrc));

    // marquage ZF et PF
    pTmgrTls->updateTaintZeroFlag  (std::make_shared<TaintBit>(F_IS_NULL, objResult));
    pTmgrTls->updateTaintParityFlag(std::make_shared<TaintBit>(F_PARITY,  objResult));
    
    // marquage Carry : dernier bit de la source �ject� suite au shift       
    pTmgrTls->updateTaintCarryFlag(std::make_shared<TaintBit>(
        F_CARRY_SAR,
        objSrcShiftedSrc,
        objTbCount));
} // fSAR 

/**********/
/** SHLD **/
/**********/

// CALLBACKS
void SHIFT::cSHLD(INS &ins) 
{
    void (*callback)() = nullptr;     // pointeur sur la fonction a appeler
    REG regSrc = INS_OperandReg(ins, 1); // registre "source" (2eme op�rande, tjs registre)
    UINT32 regSrcSize = getRegSize(regSrc);
    if (!regSrc) return;    // registre non support�

    // DECALAGE PAR VALEUR : SHLD_IM ou SHLD_IR
    if (INS_OperandIsImmediate(ins, 1)) 
    {    
        // masquage du d�placement � 0x1f (meme en x64), sauf si op�rande 64bits (masquage � 0x3f) 
        UINT32 depl = static_cast<UINT32>(INS_OperandImmediate(ins, 1));
        UINT32 maskedDepl = depl & 0x1f;   
        
        if (INS_IsMemoryWrite(ins)) // DESTINATION = MEMOIRE (SHLD_IM)
        {   
            switch (INS_MemoryWriteSize(ins)) 
            {
                // case 1: impossible
                case 2: callback = (AFUNPTR) sSHLD_IM<16>; break;
                case 4: callback = (AFUNPTR) sSHLD_IM<32>; break;
                #if TARGET_IA32E
                case 8: 
                    callback = (AFUNPTR) sSHLD_IM<64>; 
                    maskedDepl = depl & 0x3f;   // masquage � 0x3f en 64bits
                    break;
                #endif
                default: return;
            } 
            // d�placement non nul : instrumentation (sinon aucun chgt)
            if (maskedDepl) INS_InsertCall (ins, IPOINT_BEFORE, callback,
                IARG_THREAD_ID,
                IARG_UINT32, maskedDepl,
                IARG_UINT32, regSrc,
                IARG_REG_VALUE, regSrc,
                IARG_MEMORYWRITE_EA,   
                CALLBACK_DEBUG IARG_END);
        }
        else // DESTINATION = REGISTRE (SHLD_IR)
        {         
            REG regDest = INS_OperandReg(ins, 0); // registre qui sera d�cal� 
            switch (getRegSize(regDest)) 
            {
                // case 1: impossible
                case 2: callback = (AFUNPTR) sSHLD_IR<16>; break;
                case 4: callback = (AFUNPTR) sSHLD_IR<32>; break;
                #if TARGET_IA32E
                case 8: 
                    callback = (AFUNPTR) sSHLD_IR<64>; 
                    maskedDepl = depl & 0x3f;   // masquage � 0x3f en 64bits
                    break;
                #endif
                default: return;
            } 
            
            // d�placement non nul : instrumentation (sinon aucun chgt)
            if (maskedDepl) INS_InsertCall (ins, IPOINT_BEFORE, callback,
                IARG_THREAD_ID,
                IARG_UINT32, maskedDepl,
                IARG_UINT32, regSrc,
                IARG_REG_VALUE, regSrc,
                IARG_UINT32,    regDest,    // registre d�cal�
                IARG_REG_VALUE, regDest,    // sa valeur lors du callback
                CALLBACK_DEBUG IARG_END);
        }
    }
    // DECALAGE PAR REGISTRE : SHLD_RM ou SHLD_RR
    else 
    {  
        // le masquage � 0x1f ou 0x3f sera fait dans le callback
        if (INS_IsMemoryWrite(ins)) // DESTINATION = MEMOIRE (SHLD_RM)
        {   
            switch (regSrcSize) 
            {
                // case 1: impossible
                case 2: callback = (AFUNPTR) sSHLD_RM<16>; break;
                case 4: callback = (AFUNPTR) sSHLD_RM<32>; break;
                #if TARGET_IA32E
                case 8: callback = (AFUNPTR) sSHLD_RM<64>; break;
                #endif
                default: return;
            } 
            INS_InsertCall (ins, IPOINT_BEFORE, callback,
                IARG_THREAD_ID,
                IARG_REG_VALUE, REG_CL, // valeur num�rique du d�placement
                IARG_UINT32, regSrc,
                IARG_REG_VALUE, regSrc,
                IARG_MEMORYWRITE_EA,   
                CALLBACK_DEBUG IARG_END);
        }
        else // DESTINATION = REGISTRE (SHLD_RR)
        {         
            REG regDest = INS_OperandReg(ins, 0); // registre qui sera d�cal� 
            switch (getRegSize(regDest)) 
            {
                // case 1: impossible
                case 2: callback = (AFUNPTR) sSHLD_RR<16>; break;
                case 4: callback = (AFUNPTR) sSHLD_RR<32>; break;
                #if TARGET_IA32E
                case 8: callback = (AFUNPTR) sSHLD_RR<64>; break;
                #endif
                default: return;
            }   
            INS_InsertCall (ins, IPOINT_BEFORE, callback,
                IARG_THREAD_ID,
                IARG_REG_VALUE, REG_CL, // valeur num�rique du d�placement
                IARG_UINT32, regSrc,
                IARG_REG_VALUE, regSrc,
                IARG_UINT32,    regDest,    // registre d�cal�
                IARG_REG_VALUE, regDest,    // sa valeur lors du callback
                CALLBACK_DEBUG IARG_END);
        }
    }
} // cSHLD

/** FLAGS **/
// marquage flags sp�cifique pour SHLD, d�placement non marqu�
void SHIFT::fSHLD(TaintManager_Thread *pTmgrTls, const TaintPtr &resultPtr, const ObjectSource &objSrcShiftedSrc,
          UINT32 maskedDepl)
{
    ObjectSource objResult(resultPtr);
    
    // d�marquage AF (undefined selon Intel)
    pTmgrTls->unTaintAuxiliaryFlag();
    // marquage ZF et SF
    pTmgrTls->updateTaintZeroFlag(std::make_shared<TaintBit>(F_IS_NULL, objResult));
    pTmgrTls->updateTaintSignFlag(std::make_shared<TaintBit>(F_MSB,     objResult));
    // marquage PF dans tous les cas (contrairement � SHL ou on le faisait si depl < 8)
    pTmgrTls->updateTaintParityFlag(std::make_shared<TaintBit>(F_PARITY, objResult));

    // marquage Carry : bit (length-depl) de la source
    // peut quand meme provoquer un surmarquage si l'octet concern� n'est pas marqu�
    // mais le test de marquage serait trop gourmand en temps d'execution
    pTmgrTls->updateTaintCarryFlag(std::make_shared<TaintBit>(
        EXTRACT,
        objSrcShiftedSrc,
        ObjectSource(8, resultPtr->getLength() - maskedDepl)));

    // marquage Overflow, ssi depl = 1
    if (maskedDepl == 1)  pTmgrTls->updateTaintOverflowFlag(std::make_shared<TaintBit>(
            F_OVERFLOW_SHL,
            objResult,
            objSrcShiftedSrc));
    else pTmgrTls->unTaintOverflowFlag();
} // fSHLD

// marquage flags sp�cifique pour SHLD, d�placement marqu�
void SHIFT::fSHLD(TaintManager_Thread *pTmgrTls, const TaintPtr &resultPtr, const ObjectSource &objSrcShiftedSrc,
          const ObjectSource &objTbCount)
{
    // pour le cas 'deplacement marqu�', flags identiques � SHL
    fSHL(pTmgrTls, resultPtr, objSrcShiftedSrc, objTbCount);
} // fSHLD

/**********/
/** SHRD **/
/**********/

void SHIFT::cSHRD(INS &ins) 
{
    void (*callback)() = nullptr;     // pointeur sur la fonction a appeler
    REG regSrc = INS_OperandReg(ins, 1); // registre "source" (2eme op�rande, tjs registre)
    UINT32 regSrcSize = getRegSize(regSrc);
    if (!regSrc) return;    // registre non support�

    // DECALAGE PAR VALEUR : SHRD_IM ou SHRD_IR
    if (INS_OperandIsImmediate(ins, 1)) 
    {    
        // masquage du d�placement � 0x1f (meme en x64), sauf si op�rande 64bits (masquage � 0x3f) 
        UINT32 depl = static_cast<UINT32>(INS_OperandImmediate(ins, 1));
        UINT32 maskedDepl = depl & 0x1f;   
        
        if (INS_IsMemoryWrite(ins)) // DESTINATION = MEMOIRE (SHRD_IM)
        {   
            switch (INS_MemoryWriteSize(ins)) 
            {
                // case 1: impossible
                case 2: callback = (AFUNPTR) sSHRD_IM<16>; break;
                case 4: callback = (AFUNPTR) sSHRD_IM<32>; break;
                #if TARGET_IA32E
                case 8: 
                    callback = (AFUNPTR) sSHRD_IM<64>; 
                    maskedDepl = depl & 0x3f;   // masquage � 0x3f en 64bits
                    break;
                #endif
                default: return;
            } 
            // d�placement non nul : instrumentation (sinon aucun chgt)
            if (maskedDepl) INS_InsertCall (ins, IPOINT_BEFORE, callback,
                IARG_THREAD_ID,
                IARG_UINT32, maskedDepl,
                IARG_UINT32, regSrc,
                IARG_REG_VALUE, regSrc,
                IARG_MEMORYWRITE_EA,   
                CALLBACK_DEBUG IARG_END);
        }
        else // DESTINATION = REGISTRE (SHRD_IR)
        {         
            REG regDest = INS_OperandReg(ins, 0); // registre qui sera d�cal� 
            switch (getRegSize(regDest)) 
            {
                // case 1: impossible
                case 2: callback = (AFUNPTR) sSHRD_IR<16>; break;
                case 4: callback = (AFUNPTR) sSHRD_IR<32>; break;
                #if TARGET_IA32E
                case 8: 
                    callback = (AFUNPTR) sSHRD_IR<64>; 
                    maskedDepl = depl & 0x3f;   // masquage � 0x3f en 64bits
                    break;
                #endif
                default: return;
            } 
            
            // d�placement non nul : instrumentation (sinon aucun chgt)
            if (maskedDepl) INS_InsertCall (ins, IPOINT_BEFORE, callback,
                IARG_THREAD_ID,
                IARG_UINT32, maskedDepl,
                IARG_UINT32, regSrc,
                IARG_REG_VALUE, regSrc,
                IARG_UINT32,    regDest,    // registre d�cal�
                IARG_REG_VALUE, regDest,    // sa valeur lors du callback
                CALLBACK_DEBUG IARG_END);
        }
    }
    // DECALAGE PAR REGISTRE : SHRD_RM ou SHRD_RR
    else 
    {  
        // le masquage � 0x1f ou 0x3f sera fait dans le callback
        if (INS_IsMemoryWrite(ins)) // DESTINATION = MEMOIRE (SHRD_RM)
        {   
            switch (regSrcSize) 
            {
                // case 1: impossible
                case 2: callback = (AFUNPTR) sSHRD_RM<16>; break;
                case 4: callback = (AFUNPTR) sSHRD_RM<32>; break;
                #if TARGET_IA32E
                case 8: callback = (AFUNPTR) sSHRD_RM<64>; break;
                #endif
                default: return;
            } 
            INS_InsertCall (ins, IPOINT_BEFORE, callback,
                IARG_THREAD_ID,
                IARG_REG_VALUE, REG_CL, // valeur num�rique du d�placement
                IARG_UINT32, regSrc,
                IARG_REG_VALUE, regSrc,
                IARG_MEMORYWRITE_EA,   
                CALLBACK_DEBUG IARG_END);
        }
        else // DESTINATION = REGISTRE (SHRD_RR)
        {         
            REG regDest = INS_OperandReg(ins, 0); // registre qui sera d�cal� 
            switch (getRegSize(regDest)) 
            {
                // case 1: impossible
                case 2: callback = (AFUNPTR) sSHRD_RR<16>; break;
                case 4: callback = (AFUNPTR) sSHRD_RR<32>; break;
                #if TARGET_IA32E
                case 8: callback = (AFUNPTR) sSHRD_RR<64>; break;
                #endif
                default: return;
            }   
            INS_InsertCall (ins, IPOINT_BEFORE, callback,
                IARG_THREAD_ID,
                IARG_REG_VALUE, REG_CL, // valeur num�rique du d�placement
                IARG_UINT32, regSrc,
                IARG_REG_VALUE, regSrc,
                IARG_UINT32,    regDest,    // registre d�cal�
                IARG_REG_VALUE, regDest,    // sa valeur lors du callback
                CALLBACK_DEBUG IARG_END);
        }
    }
} // cSHRD

/** FLAGS **/
// marquage flags sp�cifique pour SHRD, d�placement non marqu�
void SHIFT::fSHRD(TaintManager_Thread *pTmgrTls, const TaintPtr &resultPtr, const ObjectSource &objConcatenatedSrc,
          UINT32 maskedDepl)
{
    ObjectSource objResult(resultPtr);
    
    // d�marquage AF (undefined selon Intel) et SF (puisque insertion z�ros)
    pTmgrTls->unTaintAuxiliaryFlag();
    pTmgrTls->unTaintSignFlag();

    // marquage ZF et PF
    pTmgrTls->updateTaintZeroFlag  (std::make_shared<TaintBit>(F_IS_NULL, objResult));
    pTmgrTls->updateTaintParityFlag(std::make_shared<TaintBit>(F_PARITY,  objResult));
    
    // marquage Carry : bit (depl - 1) de la source, (PEU IMPORTE QU'ELLE SOIT ICI CONCATENEE)
    // peut provoquer un surmarquage si l'octet concern� n'est pas marqu�
    // mais le test de marquage serait trop gourmand en temps d'execution
    pTmgrTls->updateTaintCarryFlag(std::make_shared<TaintBit>(
        EXTRACT,
        objConcatenatedSrc,
        ObjectSource(8, maskedDepl - 1)));

    // marquage Overflow, ssi depl = 1 
    // POUR SHRD, OF vaut 1 si changement de signe, donc si MSB srcDest != LSB BitPattern
    if (maskedDepl == 1)  pTmgrTls->updateTaintOverflowFlag(std::make_shared<TaintBit>(
            F_OVERFLOW_SHRD,
            objConcatenatedSrc));
    else pTmgrTls->unTaintOverflowFlag();
} // fSHRD

// marquage flags sp�cifique pour SHRD, d�placement marqu�
void SHIFT::fSHRD(TaintManager_Thread *pTmgrTls, const TaintPtr &resultPtr, const ObjectSource &objConcatenatedSrc,
          const ObjectSource &objTbCount)
{
    ObjectSource objResult(resultPtr);
    
    // d�marquage AF (undefined selon Intel) et SF (puisque insertion z�ros)
    pTmgrTls->unTaintAuxiliaryFlag();
    pTmgrTls->unTaintSignFlag();

    // marquage ZF et PF
    pTmgrTls->updateTaintZeroFlag  (std::make_shared<TaintBit>(F_IS_NULL, objResult));
    pTmgrTls->updateTaintParityFlag(std::make_shared<TaintBit>(F_PARITY,  objResult));
    
    // marquage Carry : dernier bit de la source �ject� suite au shift       
    pTmgrTls->updateTaintCarryFlag(std::make_shared<TaintBit>(
        F_CARRY_SHR,
        objConcatenatedSrc,
        objTbCount));

    // POUR SHRD, OF vaut 1 si changement de signe, donc si MSB srcDest != LSB BitPattern
    // probable surmarquage car OF marqu� ssi depl == 1; solution : d�marquer !!!
    pTmgrTls->unTaintOverflowFlag();
} // fSHRD

