#include "utils.h"

void UTILS::cUNHANDLED(INS &ins) 
{ 
    // si l'instruction �crit en m�moire : d�marquage de la m�moire
    if (INS_IsMemoryWrite(ins)) 
    {   
        // contrairement � ce qui est fait pour les instructions, il n'est pas possible d'utiliser des templates
        // car la taille d'�criture peut etre > � 8 octets (cas SSE et AVX...)
        // on passe donc la taille en parametre, et la fonction appellera UnTaintMemory<8> dans une boucle
        // avec la taille pour compteur
        
        INS_InsertCall (ins, IPOINT_BEFORE, (AFUNPTR) uMEM,
            IARG_FAST_ANALYSIS_CALL,  
            IARG_MEMORYWRITE_EA,  
            IARG_MEMORYWRITE_SIZE,
            IARG_END);
    } 
    
    // r�cup�ration de tous les registres de destination, y compris Eflags, et d�marquage
    REG reg = INS_RegW(ins, 0); // premier registre acc�d� en �criture (REG_INVALID si aucun)
    UINT32 kthReg = 0;          // ki�me registre acc�d� en �criture

    while ( reg != REG_INVALID() ) // parcours de tous les registres acc�d�s en �criture
    {
        // proc�dure sp�cifique pour les flags
        if (reg == REG_GFLAGS) // GFLAGS = (E/R)FLAGS selon l'architecture
        {
            INS_InsertCall (ins, IPOINT_BEFORE, (AFUNPTR) uFLAGS,
                IARG_FAST_ANALYSIS_CALL,
                IARG_THREAD_ID, 
                IARG_END);
        }
        else
        {
            UINT32 regSize = getRegSize(reg); 
            if (regSize) // si registre suivi en marquage, le d�marquer
            {     
                 void (*callbackReg)() = nullptr; // pointeur sur la fonction � appeler
                 switch (regSize)        
                {
                    case 1: callbackReg = (AFUNPTR) uREG<8>;  break;
                    case 2: callbackReg = (AFUNPTR) uREG<16>; break;
                    case 4: callbackReg = (AFUNPTR) uREG<32>; break;
                    case 8: callbackReg = (AFUNPTR) uREG<64>; break;
                }
                INS_InsertCall (ins, IPOINT_BEFORE, callbackReg,
                    IARG_FAST_ANALYSIS_CALL, 
                    IARG_THREAD_ID, 
                    IARG_UINT32, reg, 
                    IARG_END);                   
            }
        }
        // r�cup�ration du prochain registre sur la liste avant rebouclage
        reg = INS_RegW(ins, ++kthReg);
    } 
} // cUNHANDLED

/*****************************************************************************/
/** UTILITAIRE DE CREATION DE L'OBJET CORRESPONDANT A UN ADRESSAGE INDIRECT **/
/*****************************************************************************/
// Fonction surcharg�e : computeTaintEffectiveAddress
// 1) (Base+valeur)  + (Index+valeur) + Scale + displ (6 arguments - BISD)
// 2) (Index+valeur) + Scale + displ                  (4 arguments - ISD)
// 3) (Base+valeur) + displ                           (3 arguments - BD)

// elles font partie des fonctions d'analyse (appel�es par un callback)
// avec une priorit� haute (1er callback appel� pour une instruction)
// lorsque l'isntruction acc�de � la m�moire, en lecture ou �criture

// Chaque fonction va v�rifier si la valeur de l'adresse calcul�e est marqu�e
// (NB : on teste la valeur de l'adresse, pas la valeur point�e par l'adresse !!)

// s'il y a marquage, un objet symbolique repr�sentant le calcul de l'adresse
// est cr��, et stock� dans TaintManager ('storeTaintEffectiveAddress')
// si pas de marquage, cela est inscrit dans TaintManager ('clearTaintEffectiveAddress')
// TODO : VERIFIER L'UTILITE car la valeur est remise � 0 apr�s chaque instruction

// dans les fonctions d'analyse ayant pour source ou destination la m�moire
// la fonction testera le marquage dans taintManager ('getTaintEffectiveAddress')
// si marqu�, un objet de type "LOAD" ou "STORE" sera cr�� avec pour sources 
// 1) le marquage de la m�moire (ce peut etre une valeur ou un objet marqu�)
// 2) l'adresse r�elle de la m�moire acc�dee (ObjectSource valeur sur 32 ou 64 bits)
// 3) l'objet qui permet de calculer cette addresse

// lors de la construction de la formule, lors de la rencontre d'un objet "LOAD" ou "STORE"
// une contrainte sera ajout�e du type 'adresse r�elle = objet'
// cette contrainte, lorsqu'elle sera invers�e, permettra de trouver une autre valeur pour l'adresse

#if TARGET_IA32
// Adresse du type BISD 32bits
void UTILS::computeTaintEffectiveAddress(THREADID tid, REG baseReg, ADDRINT baseRegValue, 
    REG indexReg, ADDRINT indexRegValue, INT32 displ, UINT32 scale)
{
// pour construire l'EA, on additionne deux objets : la base, et (index*scale+displ)
//  SI BASE MARQUEE source1 = marquage base
//  SINON source1 = valeur registre de base
//
//  SI INDEX MARQUE 
//      SI = index*scale (via SHL)
//      SI DISPL NON NUL
//          ISD = IS +/- displ (via ADD ou SUB)
//          source2 = ISD
//      SINON source2 = IS
//  SINON source2 = valeur (index*scale +/- displ)
    
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    
    bool isIndexRegTainted = pTmgrTls->isRegisterTainted<32>(indexReg);
    bool isBaseRegTainted =  pTmgrTls->isRegisterTainted<32>(baseReg);
    // il faut au moins un des registres marqu�s, sinon ne rien faire
    if (isIndexRegTainted || isBaseRegTainted) 
    {
        // construction du squelette de l'objet (il y aura au moins une addition)
        // ajout de la base (SOURCE 1) lors de la construction, selon son marquage
        TaintDword result(
            X_ADD, 
            (isBaseRegTainted) ? ObjectSource(pTmgrTls->getRegisterTaint<32>(baseReg, baseRegValue))
                               : ObjectSource(32, baseRegValue));  
    
        // ajout d'index*scale +/- displ (SOURCE 2), selon son marquage 
        if (isIndexRegTainted) 
        {      
            // 1) traitement de l'op�ration index*scale
            TaintDwordPtr tdw_IS_ptr;
            if (scale == 1)  tdw_IS_ptr = pTmgrTls->getRegisterTaint<32>(indexReg, indexRegValue);
            else // cas Scale = 2, 4 ou 8
            { 
                // valeur du d�placement (avec scale = 2^depl)
                UINT32 shiftValue = (scale == 2) ? 1 : ((scale == 4) ? 2 : 3); 
                // nouvel objet index * scale, trait� comme un SHL car d�placement multiple de 2 
                // source 1 : le registre d'index (forc�ment marqu�)
                // source 2 : shiftValue (sur 8 bits, comme tous les shifts)
                tdw_IS_ptr = std::make_shared<TaintDword>
                    (X_SHL, 
                    ObjectSource(pTmgrTls->getRegisterTaint<32>(indexReg, indexRegValue)),
                    ObjectSource(8, shiftValue)
                    );  
            }
            
            // 2) traitement du d�placement, et ajout de la source2 au resultat
            // si pas de d�placement, source2 vaut la valeur de IS
            if (!displ) result.addSource(tdw_IS_ptr);   
            else // sinon, construction de l'objet ISD
            {
                // addition ou soustraction selon signe du d�placement
                // et ajout comme source2 au r�sultat
                result.addSource(std::make_shared<TaintDword>( 
                    (displ > 0) ? X_ADD : X_SUB, 
                    ObjectSource(tdw_IS_ptr),
                    ObjectSource(32, abs(displ))));  
                // ajout comme source2 au r�sultat
            }
        }
        else result.addConstantAsASource<32>(indexRegValue*scale + displ);

        // stockage dans TaintManager
        pTmgrTls->storeTaintEffectiveAddress(std::make_shared<TaintDword>(result));
    }
}
  
// Adresse du type ISD 32bits
void UTILS::computeTaintEffectiveAddress
    (THREADID tid, REG indexReg, ADDRINT indexRegValue, INT32 displ, UINT32 scale)
{ 
//  SI = index*scale (via SHL)
//  SI DISPL NON NUL
//      ISD = IS +/- displ (via ADD ou SUB)
//      resul = ISD
//  SINON result = IS
    
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    
    // traitement si le registre d'index est marqu� 
    if (pTmgrTls->isRegisterTainted<32>(indexReg)) 
    {
        // 1) traitement de l'op�ration index*scale
        TaintDwordPtr tdw_IS_ptr = nullptr;

        // pas de scale => index tout court
        if (scale == 1)  tdw_IS_ptr = pTmgrTls->getRegisterTaint<32>(indexReg, indexRegValue);
        else // cas Scale = 2, 4 ou 8
        { 
            // valeur du d�placement (avec scale = 2^depl)
            UINT32 shiftValue = (scale == 2) ? 1 : ((scale == 4) ? 2 : 3); 
            // nouvel objet index * scale, trait� comme un SHL car d�placement multiple de 2 
            // source 1 : le registre d'index (forc�ment marqu�)
            // source 2 : shiftValue (sur 8 bits, comme tous les shifts)
            tdw_IS_ptr = std::make_shared<TaintDword>
                (X_SHL, 
                ObjectSource(pTmgrTls->getRegisterTaint<32>(indexReg, indexRegValue)),
                ObjectSource(8, shiftValue)
                );   
        }

        // stockage du r�sultat dans TaintManager
        // si d�placement nul, resultat vaut la valeur de IS, sinon il vaut l'objet IS +/- displ
        if (!displ)    pTmgrTls->storeTaintEffectiveAddress(tdw_IS_ptr);
        else // sinon, construction de l'objet ISD
        {
            // addition ou soustraction selon signe du d�placement
            pTmgrTls->storeTaintEffectiveAddress(std::make_shared<TaintDword>(
                (displ > 0) ? X_ADD : X_SUB, 
                ObjectSource(tdw_IS_ptr),
                ObjectSource(32, abs(displ))));
        }
    }
} 

// Adresse du type BD 32bits
void UTILS::computeTaintEffectiveAddress
    (THREADID tid, REG baseReg, ADDRINT baseRegValue, INT32 displ)
{ 
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    
    // traitement si le registre de base est marqu� 
    if (pTmgrTls->isRegisterTainted<32>(baseReg)) 
    {
        // d�placement nul : r�cup�ration du marquage du registre
        if (!displ) 
        {
            pTmgrTls->storeTaintEffectiveAddress(pTmgrTls->getRegisterTaint<32>(baseReg, baseRegValue));
        }
        else // sinon, construction de l'objet BD        
        {
            // addition ou soustraction selon signe du d�placement
            pTmgrTls->storeTaintEffectiveAddress(std::make_shared<TaintDword>(
                (displ > 0) ? X_ADD : X_SUB, 
                ObjectSource(pTmgrTls->getRegisterTaint<32>(baseReg, baseRegValue)),
                ObjectSource(32, abs(displ))));
        }
    }
} 
  
#else

// Adresse du type BISD 64bits
void UTILS::computeTaintEffectiveAddress(THREADID tid, REG baseReg, ADDRINT baseRegValue, 
    REG indexReg, ADDRINT indexRegValue, INT32 displ, UINT32 scale)
{
// pour construire l'EA, on additionne deux objets : la base, et (index*scale+displ)
//  SI BASE MARQUEE source1 = marquage base
//  SINON source1 = valeur registre de base
//
//  SI INDEX MARQUE 
//      SI = index*scale (via SHL)
//      SI DISPL NON NUL
//          ISD = IS +/- displ (via ADD ou SUB)
//          source2 = ISD
//      SINON source2 = IS
//  SINON source2 = valeur (index*scale +/- displ)

    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    
    bool isIndexRegTainted = pTmgrTls->isRegisterTainted<64>(indexReg);
    bool isBaseRegTainted =  pTmgrTls->isRegisterTainted<64>(baseReg);
    // il faut au moins un des registres marqu�s, sinon ne rien faire
    if (isIndexRegTainted || isBaseRegTainted) 
    {
        // construction du squelette de l'objet (il y aura au moins une addition)
        // ajout de la base (SOURCE 1) lors de la construction, selon son marquage
        TaintQword result(
            X_ADD, 
            (isBaseRegTainted) ? ObjectSource(pTmgrTls->getRegisterTaint<64>(baseReg, baseRegValue))
                               : ObjectSource(64, baseRegValue));  
    
        // ajout d'index*scale +/- displ (SOURCE 2), selon son marquage 
        if (isIndexRegTainted) 
        {      
            // 1) traitement de l'op�ration index*scale
            TaintQwordPtr tqw_IS_ptr;
            if (scale == 1)  tqw_IS_ptr = pTmgrTls->getRegisterTaint<64>(indexReg, indexRegValue);
            else // cas Scale = 2, 4 ou 8
            { 
                // valeur du d�placement (avec scale = 2^depl)
                UINT32 shiftValue = (scale == 2) ? 1 : ((scale == 4) ? 2 : 3); 
                // nouvel objet index * scale, trait� comme un SHL car d�placement multiple de 2 
                // source 1 : le registre d'index (forc�ment marqu�)
                // source 2 : shiftValue (sur 8 bits, comme tous les shifts)
                tqw_IS_ptr = std::make_shared<TaintQword>(
                    X_SHL, 
                    ObjectSource(pTmgrTls->getRegisterTaint<64>(indexReg, indexRegValue)),
                    ObjectSource(8, shiftValue));  
            }
            
            // 2) traitement du d�placement, et ajout de la source2 au resultat
            // si pas de d�placement, source2 vaut la valeur de IS
            if (!displ) result.addSource(tqw_IS_ptr);   
            else // sinon, construction de l'objet ISD
            {
                // addition ou soustraction selon signe du d�placement
                // et ajout comme source2 au r�sultat
                result.addSource(std::make_shared<TaintQword>( 
                    (displ > 0) ? X_ADD : X_SUB, 
                    ObjectSource(tqw_IS_ptr),
                    ObjectSource(64, abs(displ))));  
                // ajout comme source2 au r�sultat
            }
        }
        else result.addConstantAsASource<64>(indexRegValue*scale + displ);

        // stockage dans TaintManager
        pTmgrTls->storeTaintEffectiveAddress(std::make_shared<TaintQword>(result));
    }
}
  
// Adresse du type ISD 64bits
void UTILS::computeTaintEffectiveAddress
    (THREADID tid, REG indexReg, ADDRINT indexRegValue, INT32 displ, UINT32 scale)
{ 
//  SI = index*scale (via SHL)
//  SI DISPL NON NUL
//      ISD = IS +/- displ (via ADD ou SUB)
//      resul = ISD
//  SINON result = IS

    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    
    // traitement si le registre d'index est marqu� 
    if (pTmgrTls->isRegisterTainted<64>(indexReg)) 
    {
        // 1) traitement de l'op�ration index*scale
        TaintQwordPtr tqw_IS_ptr = nullptr;

        // pas de scale => index tout court
        if (scale == 1)  tqw_IS_ptr = pTmgrTls->getRegisterTaint<64>(indexReg, indexRegValue);
        else // cas Scale = 2, 4 ou 8
        { 
            // valeur du d�placement (avec scale = 2^depl)
            UINT32 shiftValue = (scale == 2) ? 1 : ((scale == 4) ? 2 : 3); 
            // nouvel objet index * scale, trait� comme un SHL car d�placement multiple de 2 
            // source 1 : le registre d'index (forc�ment marqu�)
            // source 2 : shiftValue (sur 8 bits, comme tous les shifts)
            tqw_IS_ptr = std::make_shared<TaintQword>(
                X_SHL, 
                ObjectSource(pTmgrTls->getRegisterTaint<64>(indexReg, indexRegValue)),
                ObjectSource(8, shiftValue));   
        }

        // stockage du r�sultat dans TaintManager
        // si d�placement nul, resultat vaut la valeur de IS, sinon il vaut l'objet IS +/- displ
        if (!displ)   pTmgrTls->storeTaintEffectiveAddress(tqw_IS_ptr);
        else // sinon, construction de l'objet ISD
        {
            // addition ou soustraction selon signe du d�placement
            pTmgrTls->storeTaintEffectiveAddress(std::make_shared<TaintQword>(
                (displ > 0) ? X_ADD : X_SUB, 
                ObjectSource(tqw_IS_ptr),
                ObjectSource(64, abs(displ))));
        }
    }
} 

// Adresse du type BD 64bits
void UTILS::computeTaintEffectiveAddress
    (THREADID tid, REG baseReg, ADDRINT baseRegValue, INT32 displ)
{ 
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
        
    // traitement si le registre de base est marqu� 
    if (pTmgrTls->isRegisterTainted<64>(baseReg)) 
    {
        // d�placement nul : r�cup�ration du marquage du registre
        if (!displ) 
        {
            pTmgrTls->storeTaintEffectiveAddress(pTmgrTls->getRegisterTaint<64>(baseReg, baseRegValue));
        }
        else // sinon, construction de l'objet BD        
        {
            // addition ou soustraction selon signe du d�placement
            pTmgrTls->storeTaintEffectiveAddress(std::make_shared<TaintQword>(
                (displ > 0) ? X_ADD : X_SUB, 
                ObjectSource(pTmgrTls->getRegisterTaint<64>(baseReg, baseRegValue)),
                ObjectSource(64, abs(displ))));
        }
    }
} // computeEA_BD
#endif // TARGET_IA32