#pragma once

#include <pintool.h>
#include "TaintObject.h"
#include "ObjectSource.h"

#include <map>
#include <regex> // parsing des octets � suivre en marquage
#include <array>

// position des flags
enum FLAGS_INDEX
{
    CARRY_FLAG      = 0,    /*!< Carry Flag  */
    PARITY_FLAG     = 2,    /*!< Parity Flag */
    AUXILIARY_FLAG  = 4,    /*!< Auxiliary Carry Flag */
    ZERO_FLAG       = 6,    /*!< Zero Flag */
    SIGN_FLAG       = 7,    /*!< Sign Flag */
    DIRECTION_FLAG  = 10,   /*!< Direction Flag */
    OVERFLOW_FLAG   = 11,   /*!< Overflow Flag */
};

// type d'adressage indirect utilis� dans une instruction
// sert a d�terminer les arguments pass�s aux fonctions d'analyse
enum KIND_OF_EFFECTIVE_ADDRESS
{
    EA_IMMEDIATE,         /*!< cas ou l'adresse est juste une valeur : peu interessant */
    EA_BASE,              /*!< base, sans index ni d�placement    */
    EA_BASE_DISPL,        /*!< base, sans index, d�placement non nul  */
    // les cas INDEX et INDEX_DISPL sont impossibles (dans ce cas c'est un registre de base)
    EA_INDEX_SCALE,       /*!< index, scale != 1, sans base ni d�placement */
    EA_INDEX_SCALE_DISPL, /*!< index, scale != 1, sans base, d�placement non nul */
    EA_BASE_INDEX,        /*!< base, index, pas de scale ni d�placement */
    EA_BASE_INDEX_DISPL,  /*!< base, index, pas de scale, d�placement non nul */
    EA_BASE_INDEX_SCALE,  /*!< base, index, scale != 1, pas de scale ni d�placement */
    EA_BASE_INDEX_SCALE_DISPL,  /*!< base, index, scale != 1, d�placement non nul */
};

// extraction de l'octet n� "index" de la valeur "value"
static inline UINT32 EXTRACTBYTE(ADDRINT value, UINT32 index) 
{
    return (static_cast<UINT32>(0xff & (value >> (index << 3)))); 
}

// extraction du bit n� "index" de la valeur "value"
static inline UINT32 EXTRACTBIT(ADDRINT value, UINT32 index) 
{
    return (static_cast<UINT32>(0x1 & (value >> index))); 
}

// d�r�f�rencement de la m�moire pour r�cup�rer la valeur. 
// utilis� dans les fonctions d'analyse pour cr�er 
// des 'ObjectSource' lorsque la m�moire n'est pas marqu�e
template<UINT32 lengthInBits> ADDRINT getMemoryValue(ADDRINT address)
{
    ADDRINT returnValue = 0;
    // d�r�f�rencement de 'lengthInBits>>3' octets � partir de 'address'
    // Equivalent de Memcpy pour PIN
    PIN_SafeCopy(&returnValue, reinterpret_cast<ADDRINT*>(address), lengthInBits >> 3);
    return (returnValue);
}

// Sauvegarde des �l�ments constitutifs d'une instruction de la famille STRINGOP (SCAS / CMPS)
class StringOpInfo
{
public:
    bool isREPZ;        // VRAI si REPZ, faux si REPNZ
    bool isRegTainted;  // Vrai si registre AL/AX/EAX/RAX marqu� - Valable pour SCAS
    TaintPtr tPtr;      // objet marqu� repr�sentant AL/AX/EAX/RAX - Valable pour SCAS
    ADDRINT regValue;   // Valeur de AL/AX/EAX/RAX - Valable pour SCAS
    ADDRINT insAddress;   // adresse de l'instruction SCAS
};

/************************************/
/** CLASSES DE GESTION DU MARQUAGE **/
/************************************/
class TaintManager_Global;
class TaintManager_Thread;

// TaintManager_Global = gestion de la m�moire et des objets globaux (ex : octets � suivre en marquage)
// �tant une classe commune � tous les threads, les fonctions membres utilisent PIN_LOCK.
class TaintManager_Global
{

private:
    // Marquage de la m�moire : ind�pendante des threads
    std::map<ADDRINT, TaintBytePtr> _memoryPtrs;

    // offsets des octets d�j� lus dans le fichier-cible. Evite la d�claration de plusieurs
    // objets TaintByte repr�sentant le m�me octet dans le fichier. Ind�pendante des threads
    std::map<UINT32, TaintBytePtr> _offsets;

    // offset des octets � surveiller en marquage. Pass� en argument d'entr�e au pintool
    // stockage des valeurs sous forme d'intervalle (min,max), min pouvant etre �gal � max
    std::vector<std::pair<UINT32, UINT32>> _bytesToTaint;

public:
    /***********************************/
    /** STOCKAGE DES OCTETS A MARQUER **/
    /***********************************/
    void setBytesToTaint(const std::string &bytesString)
    {
        // expression r�guliere pour octets � marquer
        // option qui permet de sp�cifier uniquement certains octets � suivre en marquage. 
        // type formulaire d'impression (1,3,5-30,34,...)
        // s�paration par virgules ou point-virgules, 
        // pas d'espaces entre chiffres (sinon fait planter le parsing de argv)
        // syntaxe de la regex: ,(pr�sent ou non)'nb'-(pr�sent ou non)'nb'
        const std::regex bytesModel("[,;]?\\s*(\\d+)\\s*-?\\s*([0-9]+)?", std::regex::ECMAScript); 
        int tokens[2] = {1,2};
            
        // it�rateur de type string sur chaque groupe d'octets qui matche
        std::sregex_token_iterator it(bytesString.begin(), bytesString.end(), bytesModel, tokens);
        std::sregex_token_iterator end;
            
        while (it != end) 
        {
            int minBound, maxBound;
                
            // 1ere valeur = borne minimale (tjs pr�sente)
            minBound = std::stoi(it->str());  
            ++it; 

            // 2eme valeur = si pr�sente ca sera la borne maximale
            // sinon reprendre la borne minimale
            if (it->matched) 
            {
                maxBound = std::stoi(it->str());
                // juste pour s'assurer que le minimum l'est bien...
                if (maxBound < minBound) std::swap(maxBound, minBound);
            }
            else maxBound = minBound;
            ++it;  

            // stockage du couple dans la liste
            _bytesToTaint.push_back(std::make_pair(minBound, maxBound));
        }
    }

    /*******************************/
    /** INTERROGATION DU MARQUAGE **/
    /*******************************/

    // renvoie TRUE si tout ou partie de la plage [address, address+size] est marqu�e
    template<UINT32 lengthInBits> bool isMemoryTainted(ADDRINT address) const 
    {
        bool result = false;
        ADDRINT lastAddress = address + (lengthInBits >> 3);

        PIN_GetLock(&g_lock, 0); // obligatoire car classe globale
        while (address < lastAddress)
        { 
            if (_memoryPtrs.find(address++) != _memoryPtrs.end())
            {
                result = true;
                break;
            }
        }
        PIN_ReleaseLock(&g_lock);

        // si on arrive ici c'est que aucun emplacement m�moire n'est marqu�
        return (result); 
    }

    // sp�cialisation 8bits
    template<> bool isMemoryTainted<8>(ADDRINT address) const 
    {
        PIN_GetLock(&g_lock, 0); // obligatoire car classe globale
        auto it    = _memoryPtrs.find(address);
        auto itEnd = _memoryPtrs.end();
        PIN_ReleaseLock(&g_lock);

        return (it != itEnd);
    }

    /******************************/
    /** RECUPERATION DU MARQUAGE **/
    /******************************/

    // renvoie un objet repr�sentant le marquage de la plage d'adresses
    template<UINT32 lengthInBits> TAINT_OBJECT_PTR getMemoryTaint(ADDRINT address) const
    { 
        static_assert((lengthInBits % 8 == 0), "taille non multiple de 8 bits");
        TaintObject<lengthInBits> result(CONCAT);
        ADDRINT lastAddress = address + (lengthInBits >> 3);
        
        PIN_GetLock(&g_lock, 0); // obligatoire car classe globale
        auto itEnd = _memoryPtrs.end();
        
        while (address < lastAddress) 
        {
            auto it = _memoryPtrs.find(address);
            if (it != itEnd) // si une entr�e � �t� trouv�e => adresse marqu�e
            {       
                result.addSource(it->second);
            }
            else result.addConstantAsASource<8>(getMemoryValue<8>(address));

            address++;
        }
        PIN_ReleaseLock(&g_lock);
        return (MK_TAINT_OBJECT_PTR(result));
    }

    // sp�cialisation pour le cas 8 bits
    template<> TaintBytePtr getMemoryTaint<8>(ADDRINT address) const
    {
        PIN_GetLock(&g_lock, 0); // obligatoire car classe globale
        auto it    = _memoryPtrs.find(address);
        auto itEnd = _memoryPtrs.end();
        PIN_ReleaseLock(&g_lock);

        return ( (it == itEnd) ? nullptr : it->second);
    }
    
    /*******************************/
    /** CREATION D'OBJETS SOURCES **/
    /*******************************/

    // cr�ation de "nb" nouveaux TaintBytes initiaux � la position "offset" du fichier
    // et marquage de la m�moire point�e par "buffer"
    void createNewSourceTaintBytes(ADDRINT startAddress, UINT32 nb, UINT32 offset) 
    {
        PIN_GetLock(&g_lock, 0); // obligatoire car classe globale

        // Si le vecteur est vide, tous les octets de la source seront marqu�s
        // sinon il faudra v�rifier que chaque octet est dans les intervelles sp�cifi�s
        bool mustAllBytesBeTainted = _bytesToTaint.empty(); 

        // cr�ation de 'nb' objets � partir de l'adresse de d�part
        ADDRINT currentAddress = startAddress;
        ADDRINT lastAddress    = startAddress + nb;

        while (currentAddress < lastAddress)
        {
            // recherche de la pr�sence de l'offset dans l'un des intervalles
            // par d�faut on consid�re qu'il ne l'est pas
            bool isThisByteToBeTainted = false;

            // v�rification du fait que cet octet doit �tre marqu�
            if (mustAllBytesBeTainted) isThisByteToBeTainted = true;
            else
            {
                // recherche de la pr�sence de l'offset dans l'un des intervalles
                std::vector<std::pair<UINT32, UINT32>>::const_iterator it    = _bytesToTaint.begin();
                std::vector<std::pair<UINT32, UINT32>>::const_iterator itEnd = _bytesToTaint.end();
                for (; it != itEnd ; ++it)
                {
                    // sup�rieur ou �gal � borne min et inf�rieur ou �gal � max
                    if ((offset >= it->first) && (offset <= it->second))
                    {
                        isThisByteToBeTainted = true; 
                        break; // l'octet figure dans au moins un intervalle
                    }
                }
            }
            
            // si cet octet ne doit pas �tre marqu�
            // alors effacer l'�ventuelle pr�sence d'un marquage ant�rieur pr�sent � cette addresse
            if (!isThisByteToBeTainted) _memoryPtrs.erase(currentAddress);
            else
            {
                TaintBytePtr tbPtr; // objet qui va repr�senter le marquage de cet offset

                // recherche de l'existence d'un objet d�j� associ� � cet offset
                std::map<UINT32, TaintBytePtr>::iterator it = _offsets.find(offset);
                
                if (it == _offsets.end())  // offset non pr�sent : cr�ation d'un nouvel objet
                {
                    // cr�ation du nouvel objet 8bits et ajout de son offset comme source
                    tbPtr = std::make_shared<TaintByte>(BYTESOURCE, ObjectSource(32, offset));
                    // Ajout de cet objet dans la liste des octets d�j� lus dans la source
                    _offsets.insert(pair<UINT32, TaintBytePtr>(offset, tbPtr));
                    if (g_verbose)  g_taint <<  "cr�ation nouvel objet pour l'offset " << offset << std::endl;
                }
                // sinon r�cup�rer l'objet existant
                else 
                {
                    tbPtr = it->second;
                    if (g_verbose) g_taint << "r�cup�ration objet existant pour l'offset " << offset << std::endl;
                }
                    
                // marquage de la m�moire avec l'objet
                _memoryPtrs[currentAddress] = tbPtr;  
            }
            // ajustement pour le prochain tour de boucle
            ++currentAddress;
            ++offset;
        }
        PIN_ReleaseLock(&g_lock);
    } // createNewSourceTaintBytes

    /*****************************************/
    /** FONCTIONS DE MARQUAGE DE LA MEMOIRE **/
    /*****************************************/

    // marquage de 'lengthInBits' octets avec l'objet 'tPtr' � partir de l'adresse 'address'
    // si 'tPtr' est nullptr, provoque le d�marquage des 'lengthInBits' octets
    template<UINT32 lengthInBits> void updateMemoryTaint
        (ADDRINT address, TAINT_OBJECT_PTR tPtr) 
    {
        static_assert((lengthInBits & 0x7) == 0, "taille memoire non valide");           
        PIN_GetLock(&g_lock, 0); // obligatoire car classe globale
        
        if (!tPtr) this->unTaintMemory<lengthInBits>(address);
        else
        {
            ObjectSource objSrc(tPtr);
            // cr�ation des taintBytes extraits de l'objet pour affectation
            for (UINT32 i = 0 ; i < (lengthInBits >> 3) ; ++i, ++address) 
            { 
                // extraction de 'objSrc' de l'octet (8bits) n�i et affectation � l'adresse ad�quate
                _memoryPtrs[address] = std::make_shared<TaintByte>
                    (EXTRACT, objSrc, ObjectSource(8, i));
            }
        }

        PIN_ReleaseLock(&g_lock);
    }

    // sp�cialisation pour le cas 8bits
    template<> void updateMemoryTaint<8>(ADDRINT address, TaintBytePtr tbPtr) 
    { 
        PIN_GetLock(&g_lock, 0); // obligatoire car classe globale
        if (!tbPtr) _memoryPtrs.erase(address);
        else        _memoryPtrs[address] = std::move(tbPtr);
        PIN_ReleaseLock(&g_lock);
    }

    /****************************/
    /** FONCTION DE DEMARQUAGE **/
    /****************************/
   
    // Efface le marquage de la plage [address, address + lengthInBits];
    template<UINT32 lengthInBits> void unTaintMemory(ADDRINT address) 
    {            
        ADDRINT lastAddress = address + (lengthInBits >> 3);
        PIN_GetLock(&g_lock, 0); // obligatoire car classe globale
        while (address < lastAddress) _memoryPtrs.erase(address++);
        PIN_ReleaseLock(&g_lock);
    }
};


// TaintManager_Thread = gestion des registres et des fonctions sp�cifiques
// (suivi des CALL/RET) qui SONT DEPENDANTES DES THREADS
class TaintManager_Thread
{

private:
    // Marquage des registres d'usage g�n�ral (GP) repr�sent�s en sous-registres de 8 bits
    TaintBytePtr _registers8Ptr[regIndexLAST + 1][BYTEPARTS];

    // marquage des registres "entiers" de 16, 32 (et 64bits)
    // lors de la r�cup�ration du marquage d'un registre de plus de 8bits
    // on testera d'abord si le registre n'est pas marqu� avec un objet de taille sup�rieure
    // afin de ne pas recr�er une concat�nation d'objets 8 bits alors que l'objet existe
    // l'index dans le tableau est celui du registre "plein" (32 ou 64bits)
    // ex : le marquage de R10W se trouvera � registres64[rR10]
    TaintWordPtr  _registers16Ptr[regIndexLAST + 1];
    TaintDwordPtr _registers32Ptr[regIndexLAST + 1];
#if TARGET_IA32E
    TaintQwordPtr _registers64Ptr[regIndexLAST + 1];
#endif

    // cas du registre Eflags : marquage niveau bit 
    TaintBitPtr  _cFlagPtr, _pFlagPtr, _aFlagPtr, _zFlagPtr, _sFlagPtr, _oFlagPtr;

    // stockage de l'objet permettant de calculer une addresse effective 
    // (taille selon architecture)
#if TARGET_IA32
    TaintDwordPtr _effectiveAddressPtr; // 32bits
#else
    TaintQwordPtr _effectiveAddressPtr; // 64bits
#endif

public:

    /**************************************************/
    /** GESTION DU MARQUAGE DES ADDRESSES EFFECTIVES **/
    /**************************************************/

#if TARGET_IA32
    // mise en cache d'un objet calculant une addresse effective (32bits)
    void storeTaintEffectiveAddress(TaintDwordPtr tdwPtr)
    { _effectiveAddressPtr = tdwPtr; }

    // r�cup�ration de l'objet mis en cache
    TaintDwordPtr getTaintEffectiveAddress() const
    { return (_effectiveAddressPtr); }

#else
    // mise en cache d'un objet calculant une addresse effective (64bits)
    void storeTaintEffectiveAddress(TaintQwordPtr tqwPtr)
    { _effectiveAddressPtr = tqwPtr; }

    // r�cup�ration de l'objet mis en cache
    TaintQwordPtr getTaintEffectiveAddress() const
    { return (_effectiveAddressPtr); }

#endif

    // test si un objet a �t� mis en cache
    bool isEffectiveAddressTainted() const
    { return (_effectiveAddressPtr != nullptr); }

    // sp�cifie que la valeur de l'adresse effective n'est pas marqu�e
    // fonction appel�e syst�matiquement apr�s le traitement de l'instruction
    void clearTaintEffectiveAddress() 
    { _effectiveAddressPtr.reset(); }

    /*******************************/
    /** INTERROGATION DU MARQUAGE **/
    /*******************************/

    // renvoie TRUE si la TOTALITE du registre reg (de type PIN) est marqu�
    template<UINT32 lengthInBits> TAINT_OBJECT_PTR getFullRegisterTaint(REG reg) const
    {
        static_assert((lengthInBits % 8 == 0), "registre non multiple de 8 bits");
    }

    template<> TaintBytePtr getFullRegisterTaint<8>(REG reg) const
    {
        return (this->getRegisterTaint(reg));
    }

    template<> TaintWordPtr getFullRegisterTaint<16>(REG reg) const
    {
        return (_registers16Ptr[getRegIndex(reg)] );
    }
    
    template<> TaintDwordPtr getFullRegisterTaint<32>(REG reg) const
    {
        return (_registers32Ptr[getRegIndex(reg)] );
    }

#if TARGET_IA32E
    template<> TaintQwordPtr getFullRegisterTaint<64>(REG reg) const
    {
        return (_registers64Ptr[getRegIndex(reg)] );
    }
#endif

        // renvoie TRUE si au moins une partie du registre reg (de type PIN) est marqu�
    template<UINT32 lengthInBits> bool isRegisterTainted(REG reg) const
    {
        REGINDEX regIndex = getRegIndex(reg);

        bool result = false;
        for (UINT32 i = 0 ; i < (lengthInBits >> 3) ; ++i) 
        { 
            if ((bool) _registers8Ptr[regIndex][i])
            {
                result = true;
                break;
            }
        }
        // si on arrive ici c'est que aucune partie de registre n'est marqu�
        return (result); 
    }

    // cas particulier 8 bits : partie haute ou basse
    template<> bool isRegisterTainted<8>(REG reg) const 
    {
        REGINDEX regIndex = getRegIndex(reg);
       
        return (REG_is_Upper8(reg) 
            ? (bool) _registers8Ptr[regIndex][1] 
            : (bool) _registers8Ptr[regIndex][0]);
    }

    // cas d'une partie sp�cifique du registre
    bool isRegisterPartTainted(REGINDEX regIndex, UINT32 regPart) const 
    { 
        return ((bool) _registers8Ptr[regIndex][regPart]); 
    }

    // renvoie TRUE si le flag est marqu�
    bool isCarryFlagTainted() const    { return ((bool) _cFlagPtr); }
    bool isParityFlagTainted() const   { return ((bool) _pFlagPtr); }
    bool isAuxiliaryFlagTainted() const{ return ((bool) _aFlagPtr); }
    bool isZeroFlagTainted() const     { return ((bool) _zFlagPtr); }
    bool isSignFlagTainted() const     { return ((bool) _sFlagPtr); }
    bool isOverflowFlagTainted() const { return ((bool) _oFlagPtr); }

    /******************************/
    /** RECUPERATION DU MARQUAGE **/
    /******************************/

    // renvoie un objet repr�sentant le marquage d'un registre 
    // template enti�rement sp�cialis� pour prendre en compte
    // les registres 8/16/32/64bits "entiers"
    template<UINT32 lengthInBits> TAINT_OBJECT_PTR getRegisterTaint(REG reg, ADDRINT regValue) 
    {
        static_assert((lengthInBits % 8 == 0), "registre non multiple de 8 bits");
    }

    // cas 8bits : appel de la fonction surcharg�e � 1 seul param�tre
    template<> TaintBytePtr getRegisterTaint<8>(REG reg8, ADDRINT unUsed)
    {
        return (this->getRegisterTaint(reg8));
    }
    
    // cas 16 bits : renvoi du TaintWord correspondant (si existant) 
    // ou concat�nation des 2 registres 8 bits 
    template<> TaintWordPtr getRegisterTaint<16>(REG reg16, ADDRINT reg16Value)
    {
        REGINDEX regIndex = getRegIndex(reg16);
       
        // test du marquage 16 bits. Si absent => cr�er un nouvel objet
        if (!(bool) _registers16Ptr[regIndex])
        {
            TaintWord result(CONCAT);
            for (UINT32 i = 0 ; i < 2 ; ++i) 
            {
                // si la partie du registre est marqu�, ajout de cet objet � la concat�nation   
                if ((bool) _registers8Ptr[regIndex][i]) 
                {
                    result.addSource(_registers8Ptr[regIndex][i]);
                }
                // sinon, ajout d'une source de type imm�diate
                else result.addConstantAsASource<8>(EXTRACTBYTE(reg16Value, i));
            }
            // association de l'objet nouvellement cr�� au registre 16bits
            _registers16Ptr[regIndex] = std::make_shared<TaintWord>(result);
        }
        return (_registers16Ptr[regIndex]);
    }

    // cas 32 bits : renvoi du TaintDword correspondant (si existant) 
    // ou concat�nation des 4 registres 8 bits 
    template<> TaintDwordPtr getRegisterTaint<32>(REG reg32, ADDRINT reg32Value)
    {
        REGINDEX regIndex = getRegIndex(reg32);      

        // test du marquage 32 bits. si aucune variable => la fabriquer
        if (!(bool) _registers32Ptr[regIndex])
        {
            TaintDword result(CONCAT);
            for (UINT32 i = 0 ; i < 4 ; ++i) 
            {
                // si la partie du registre est marqu�, ajout de cet objet 
                // � la concat�nation   
                if ((bool) _registers8Ptr[regIndex][i]) 
                {
                    result.addSource(_registers8Ptr[regIndex][i]);
                }
                // sinon, ajout d'une source de type imm�diate
                else result.addConstantAsASource<8>(EXTRACTBYTE(reg32Value, i));
            }
            
            // association de l'objet nouvellement cr�� au registre 32bits
            _registers32Ptr[regIndex] = std::make_shared<TaintDword>(result);
        }
        // retour de l'objet 32bits existant ou cr��
        return (_registers32Ptr[regIndex]);
    }
 
#if TARGET_IA32E
    // cas 64 bits : renvoi du TaintQword correspondant (si existant) 
    // ou concat�nation des 8 registres 8 bits 
    template<> TaintQwordPtr getRegisterTaint<64>(REG reg64, ADDRINT reg64Value)
    {
        REGINDEX regIndex = getRegIndex(reg64);

        // test du marquage 32 bits
        if (!(bool) _registers64Ptr[regIndex])
        {
            TaintQword result(CONCAT);
            for (UINT32 i = 0 ; i < 8 ; ++i) 
            {
                // si la partie du registre est marqu�, ajout de cet objet 
                // � la concat�nation   
                if ((bool) _registers8Ptr[regIndex][i]) 
                {
                    result.addSource(_registers8Ptr[regIndex][i]);
                }
                // sinon, ajout d'une source de type imm�diate
                else result.addConstantAsASource<8>(EXTRACTBYTE(reg64Value, i));
            }
        
            // association de l'objet nouvellement cr�� au registre 64bits
            _registers64Ptr[regIndex] = std::make_shared<TaintQword>(result);
        }
        return (_registers64Ptr[regIndex]);
    }
#endif

    // renvoie un objet repr�sentant le marquage d'un registre 8 bits
    // surcharge du template normal (passage d'un seul param�tre)
    TaintBytePtr getRegisterTaint(REG reg) const
    { 
        UINT32 regPart = REG_is_Upper8(reg) ? 1 : 0;
        REGINDEX regIndex = getRegIndex(reg);

        return (_registers8Ptr[regIndex][regPart]);
    }

    // renvoie le marquage d'une partie de registre
    TaintBytePtr getRegisterPartTaint(REGINDEX regIndex, UINT32 regPart) const  
    {  return (_registers8Ptr[regIndex][regPart]); }

    // renvoie le marquage correspondant aux flags
    TaintBitPtr getTaintCarryFlag()     const   { return (_cFlagPtr); }
    TaintBitPtr getTaintParityFlag()    const   { return (_pFlagPtr); }
    TaintBitPtr getTaintAuxiliaryFlag() const   { return (_aFlagPtr); }
    TaintBitPtr getTaintZeroFlag()      const   { return (_zFlagPtr); }
    TaintBitPtr getTaintSignFlag()      const   { return (_sFlagPtr); }
    TaintBitPtr getTaintOverflowFlag()  const   { return (_oFlagPtr); }

    /*****************************************/
    /** FONCTIONS DE MARQUAGE DES REGISTRES **/
    /*****************************************/

    // associe au registre "regIndex", partie "regPart"
    // l'objet TaintByte fourni
    void updateTaintRegisterPart(REGINDEX regIndex, UINT32 regPart, TaintBytePtr tbPtr) 
    {
        _registers8Ptr[regIndex][regPart] = tbPtr;
        // si un registre plein �tait pr�sent (16, ou 32, ou 64)
        // effacer le marquage, car une partie 8 bits a �t� modifi�e
        _registers16Ptr[regIndex].reset();
        _registers32Ptr[regIndex].reset();
        #if TARGET_IA32E
        _registers64Ptr[regIndex].reset();
        #endif
    }

    // mise � jour du marquage du registre avec l'objet fourni
    // sp�cialisation complete du template pour marquer les registres "entiers"
    template<UINT32 lengthInBits> 
    void updateTaintRegister(REG reg, TAINT_OBJECT_PTR tPtr)
    {  static_assert((lengthInBits % 8 == 0), "registre non valide");  }

    // cas 8bits
    template<> void updateTaintRegister<8>(REG reg8, TaintBytePtr tbPtr) 
    {
        REGINDEX regIndex = getRegIndex(reg8);

        // si un registre plein �tait pr�sent (16, ou 32, ou 64)
        // effacer le marquage, car une partie 8 bits a �t� modifi�e
        _registers16Ptr[regIndex].reset();
        _registers32Ptr[regIndex].reset();
        #if TARGET_IA32E
        _registers64Ptr[regIndex].reset();
        #endif
        // marquage
        _registers8Ptr[regIndex][(REG_is_Upper8(reg8)) ? 1 : 0] = tbPtr;
    }

    // cas 16bits
    template<> void updateTaintRegister<16>(REG reg16, TaintWordPtr twPtr) 
    {
        REGINDEX regIndex = getRegIndex(reg16);

        // si un registre plein �tait pr�sent : effacer le marquage
        // car une partie a �t� modifi�e
        _registers32Ptr[regIndex].reset();
        #if TARGET_IA32E
        _registers64Ptr[regIndex].reset();
        #endif
        
        // marquage d'abord de la partie 16 bits, 
        _registers16Ptr[regIndex] = twPtr;
 
        // si l'objet fourni est la concat�nation de deux Objets de 8 bits, alors
        // r�cup�rer ces objets (evite la multiplication des CONCAT/EXTRACT)
        if ((CONCAT == twPtr->getSourceRelation()) && (twPtr->getNumberOfSources() == 2))
        {
            for (UINT32 i = 0 ; i < 2 ; ++i)
            {
                // Traitement de chaque source de la concat�nation. 
                // Si source marqu�e, affectation au registre via X_ASSIGN, sinon d�marquage
                if (twPtr->getSource(i).isSrcTainted())
                {
                    // on consid�re que lorsqu'il y a concat�nation de 2 sources, 
                    // elles sont toutes de 8 bits. A priori il ne peut pas avoir d'autres cas
                    _registers8Ptr[regIndex][i] = twPtr->getSource(i).isSrcTainted() 
                        ? std::make_shared<TaintByte>(X_ASSIGN, ObjectSource(twPtr->getSource(i).getTaintedSource())) 
                        : nullptr;
                }
                else _registers8Ptr[regIndex][i].reset();
            }
        }
        else
        {
            // cr�ation de TaintBytes extraits de l'objet pour affectation
            ObjectSource objSrc(twPtr); 
            _registers8Ptr[regIndex][0] = std::make_shared<TaintByte>(EXTRACT, objSrc, ObjectSource(8, 0));
            _registers8Ptr[regIndex][1] = std::make_shared<TaintByte>(EXTRACT, objSrc, ObjectSource(8, 1));
        }
    }

    // cas 32bits
    template<> void updateTaintRegister<32>(REG reg32, TaintDwordPtr tdwPtr) 
    {
        REGINDEX regIndex = getRegIndex(reg32);
        
        // si un registre plein �tait pr�sent : effacer le marquage, car une partie a �t� modifi�e
        _registers16Ptr[regIndex].reset();        
        #if TARGET_IA32E
        _registers64Ptr[regIndex].reset();
        #endif
        
        // marquage d'abord de la partie 32 bits 
        _registers32Ptr[regIndex] = tdwPtr;

        // si l'objet fourni est la concat�nation de 4 Objets de 8 bits, alors
        // r�cup�rer ces objets (evite la multiplication des CONCAT/EXTRACT)
        if ((CONCAT == tdwPtr->getSourceRelation()) && (tdwPtr->getNumberOfSources() == 4))
        {
            // on consid�re que lorsqu'il y a concat�nation de 4 sources, elles sont toutes de 8 bits
            // a priori il ne peut pas avoir d'autres cas
            for (UINT32 i = 0 ; i < 4 ; ++i)
            {
                _registers8Ptr[regIndex][i] = tdwPtr->getSource(i).isSrcTainted() 
                    ? std::make_shared<TaintByte>(X_ASSIGN, ObjectSource(tdwPtr->getSource(i).getTaintedSource())) 
                    : nullptr;
            }
        }
        else
        {
            // cr�ation de TaintBytes extraits de l'objet pour affectation
            ObjectSource objSrc(tdwPtr); 
            for (UINT32 i = 0 ; i < 4 ; ++i)
            {
                _registers8Ptr[regIndex][i] = std::make_shared<TaintByte>(EXTRACT, objSrc, ObjectSource(8, i));
            }
        }
    }

#if TARGET_IA32E
    // cas 64bits
    template<> void updateTaintRegister<64>(REG reg64, TaintQwordPtr tqwPtr) 
    {
        REGINDEX regIndex = getRegIndex(reg64);
        
        // si un registre plein de 16 ou 32bits �tait pr�sent 
        // effacer le marquage, car une partie a �t� modifi�e
        _registers16Ptr[regIndex].reset();  
        _registers32Ptr[regIndex].reset();

        // marquage d'abord de la partie 64 bits, 
        _registers64Ptr[regIndex] = tqwPtr;
        
        // si l'objet fourni est la concat�nation de 4 Objets de 8 bits, alors
        // r�cup�rer ces objets (evite la multiplication des CONCAT/EXTRACT)
        if ((CONCAT == tqwPtr->getSourceRelation()) && (tqwPtr->getNumberOfSources() == 8))
        {
            // on consid�re que lorsqu'il y a concat�nation de 4 sources, elles sont toutes de 8 bits
            // a priori il ne peut pas avoir d'autres cas
            for (UINT32 i = 0 ; i < 8 ; ++i)
            {
                _registers8Ptr[regIndex][i] = tdwPtr->getSource(i).isSrcTainted() 
                    ? std::make_shared<TaintByte>(X_ASSIGN, ObjectSource(tdwPtr->getSource(i).getTaintedSource())) 
                    : nullptr;
            }
        }
        else
        {
            // cr�ation de TaintBytes extraits de l'objet pour affectation
            ObjectSource objSrc(tqwPtr); 
            for (UINT32 i = 0 ; i < 8 ; ++i)
            {
                _registers8Ptr[regIndex][i] = std::make_shared<TaintByte>(EXTRACT, objSrc, ObjectSource(8, i));
            }
        }
    }
#endif

    /*************************************/
    /** FONCTIONS DE MARQUAGE DES FLAGS **/
    /*************************************/

    void updateTaintCarryFlag  (TaintBitPtr ptr)  { _cFlagPtr = ptr;}
    void updateTaintParityFlag (TaintBitPtr ptr)  { _pFlagPtr = ptr;}
    void updateTaintAuxiliaryFlag(TaintBitPtr ptr){ _aFlagPtr = ptr;}
    void updateTaintZeroFlag    (TaintBitPtr ptr) { _zFlagPtr = ptr;}
    void updateTaintSignFlag    (TaintBitPtr ptr) { _sFlagPtr = ptr;}
    void updateTaintOverflowFlag(TaintBitPtr ptr) { _oFlagPtr = ptr;}

    /*****************************/
    /** FONCTIONS DE DEMARQUAGE **/
    /*****************************/

    // Efface le marquage d'une partie de registre
    void unTaintRegisterPart(REGINDEX regIndex, UINT32 regPart) 
    { 
        // effacement de la partie 8 bits (et forcement des registres 16/32/64)
        _registers8Ptr[regIndex][regPart].reset();  
        _registers16Ptr[regIndex].reset();
        _registers32Ptr[regIndex].reset();
        #if TARGET_IA32E
        _registers64Ptr[regIndex].reset();
        #endif
    }
    
    // Efface le marquage du registre fourni en argument
    template<UINT32 lengthInBits> void unTaintRegister(REG reg)  
    { 
        REGINDEX regIndex = getRegIndex(reg);
       
        // effacement des parties 8 bits 
        for (UINT32 i = 0 ; i < (lengthInBits >> 3) ; ++i)   _registers8Ptr[regIndex][i].reset();
        
        // effacement forcement des registres 16/32/64
        _registers16Ptr[regIndex].reset();
        _registers32Ptr[regIndex].reset();
        #if TARGET_IA32E
        _registers64Ptr[regIndex].reset();
        #endif
    }

    // sp�cialisation pour le cas 8bits
    template<> void unTaintRegister<8>(REG reg)  
    { 
        REGINDEX regIndex = getRegIndex(reg);
        UINT32 regPart    = REG_is_Upper8(reg) ? 1 : 0;
        
        // effacement des registres 8/16/32/64
        _registers8Ptr[regIndex][regPart].reset();
        _registers16Ptr[regIndex].reset();
        _registers32Ptr[regIndex].reset();
        #if TARGET_IA32E
        _registers64Ptr[regIndex].reset();
        #endif
    }
    
    // d�marque le flag
    void unTaintCarryFlag()    { _cFlagPtr.reset(); }
    void unTaintParityFlag()   { _pFlagPtr.reset(); }
    void unTaintAuxiliaryFlag(){ _aFlagPtr.reset(); }
    void unTaintZeroFlag()     { _zFlagPtr.reset(); }
    void unTaintSignFlag()     { _sFlagPtr.reset(); }
    void unTaintOverflowFlag() { _oFlagPtr.reset(); }

    // supprime le marquage de tous les flags
    void unTaintAllFlags() 
    {
        _cFlagPtr.reset();      
        _pFlagPtr.reset();
        _aFlagPtr.reset();      
        _zFlagPtr.reset();
        _sFlagPtr.reset();   
        _oFlagPtr.reset();
    }
};

// pointeur global vers classe de gestion du marquage m�moire
extern TaintManager_Global *pTmgrGlobal;

// fonction inline pour r�cup�rer la classe de marquage locale stock�e dans la TLS
static inline TaintManager_Thread* getTmgrInTls(THREADID tid)
{
    return (static_cast<TaintManager_Thread*>(PIN_GetThreadData(g_tlsKeyTaint, tid)));
}
