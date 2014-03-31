#include "buildFormula.h"

// CONSTRUCTEUR
SolverFormula::SolverFormula(): 
    _formula(std::ostringstream::ate),
    // num�ro de contrainte initialis� � 0
    // mais la premi�re contrainte sera la n�1 (cf fonction declareHeader)
    _iAssert(0),
    _iTbit(0), _iTb(0), _iTw(0), _iTdw(0), _iTqw(0), _iTdqw(0),
    _isDeBruijnDeclared(false)
{}

const std::string SolverFormula::getDeBruijnArray()
{
    // variables utilis�es par les instructions BSR et BSF
    // cf. commentaires dans buildFormula.h

    std::string result;

    // d�claration de la constante multiplicative
    result += "(define-const constante (_ BitVec 64) #x";
    result += hexstr(debruijn64, 8).substr(2); // pour �liminer le "Ox"
    result += ")\n";

    // d�finition du tableau debruijn64. Il se cr�e via une fonction:
    // param�tre 'x'    = index
    // valeur de retour = valeur du tableau
    // ALTERNATIVE A UN ARRAY CAR UN ARRAY NE PERMET PAS D'UTILISER UN '(get-model)'
    // pour obtenir les r�sultats !!!!!
    result += "(define-fun index64 ((x (_ BitVec 64))) (_ BitVec 64)\n";
        
    // stockage de 64 valeurs : 64x la fonction "(ite (= x (_ bvAA 64)) (_ bvVAL 64) ";
    for (UINT32 index = 0 ; index < 64 ; ++index)   
    {
        result += "  (ite (= x (_ bv" + decstr(index) + " 64)) ";
        result +=             "(_ bv" + decstr(index64[index]) + " 64)\n";
    }

    // pour le dernier ite, si non �gal renvoyer 0 (comme si une erreur �tait arriv�e)
    result +=     "  (_ bv0 64)";

    // 64 parenth�ses fermantes (une par 'ite') 
    for (UINT32 index = 0 ; index < 8 ; ++index)
    {
        result += "))))))))"; // 8 parenth�ses par 8
    }

    // parenth�se finale
    result += ")\n";

    // le tableau et la constante sont d�clar�s
    this->_isDeBruijnDeclared = true;

    return (result);
}

// nom de variable pour les objets, utilis�es dans les formules SMTLIB
void SolverFormula::insertSourceName(std::string &out, const ObjectSource &objSrc)
{
    // si objet marqu�, r�cup�rer son nom de variable
    if (objSrc.isSrcTainted())	 out += objSrc.getTaintedSource()->getName(); 
    // sinon fabrication de la valeur, en format SMTLIB
    else 
    {
        UINT32 srcLength = objSrc.getLength(); // longueur de l'objet (en bits)
        ADDRINT value =  objSrc.getValue(); // valeur num�rique repr�sent�e par l'objet
        
        // cas TaintBit : r�sultat d�crit en binaire
        if (srcLength == 1)	  out += (value) ? "#b1" : "#b0";
        // dans les autres cas : resultat en hexa
        else
        {
            // la fonction StringFromAddrint (API PIN) convertit un ADDRINT 
            // en chaine de caract�res. Or la source est encod�e sur 'srcLength' bits
            // donc extraire les 2/4/8/16 derniers chiffres, selon la taille

            std::string valueString(StringFromAddrint(value));
            out += "#x";
            out += valueString.substr(valueString.size() - (srcLength >> 2)); 
#if 0
            out += "( _bv" + decstr(value) + ' ' + decstr(srcLength) + ')';
#endif        
        }
    }
} // insertSourceName

// formule correspondant au calcul d'un pr�dicat
const std::string SolverFormula::getPredicateFormula(TaintManager_Thread *pTmgrTls, 
    PREDICATE pred, ADDRINT flagsOrRegValue)
{
    // la d�claration d'un pr�dicat se fait en deux �tapes
    // 1) d�claration recursive des objets utilis�s dans le calcul du ou des flags concern�s
    // 2) construction de l'expression repr�sentant le pr�dicat
    //    c'est � dire un 'ite' sur la valeur du ou des flags
    
    std::string result;
    UINT32 oneFlagValue = 0;    // valeur d'un flag (extraction de FlagsOrRegValue)
    
    switch (pred)
    {       
    case PREDICATE_BELOW: 	    // Below (CF==1).
    case PREDICATE_NOT_BELOW: 	// Not Below (CF==0).
    {
        // 1) d�claration r�cursive des sources du flag
        this->declareObject(pTmgrTls->getTaintCarryFlag());
        
        // 2) construction de l'expression
        result += "(= ";
        result += pTmgrTls->getTaintCarryFlag()->getName();
        result += (PREDICATE_BELOW == pred) ? " #b1)" : " #b0)";
        break;
    }

    case PREDICATE_SIGN:        // Sign (SF==1).
    case PREDICATE_NOT_SIGN :	// Not Sign (SF==0).
    {
        // 1) d�claration r�cursive des sources du flag
        this->declareObject(pTmgrTls->getTaintSignFlag());
        
        // 2) construction de l'expression
        result += "(= ";
        result += pTmgrTls->getTaintSignFlag()->getName();
        result += (PREDICATE_SIGN == pred) ? " #b1)" : " #b0)";
        break;
    }

    case PREDICATE_ZERO: 	    // Zero (ZF==1).
    case PREDICATE_NOT_ZERO:	// Not Zero (ZF==0).
    {
        // 1) d�claration r�cursive des sources du flag
        this->declareObject(pTmgrTls->getTaintZeroFlag());
        
        // 2) construction de l'expression
        result += "(= ";
        result += pTmgrTls->getTaintZeroFlag()->getName();
        result += (PREDICATE_ZERO == pred) ? " #b1)" : " #b0)";
        break;
    }

    case PREDICATE_OVERFLOW: 	 // Overflow (OF==1).
    case PREDICATE_NOT_OVERFLOW: // Not Overflow (OF==0)
    {
         // 1) d�claration r�cursive des sources du flag
        this->declareObject(pTmgrTls->getTaintOverflowFlag());
        
        // 2) construction de l'expression
        result += "(= ";
        result += pTmgrTls->getTaintOverflowFlag()->getName();
        result += (PREDICATE_OVERFLOW == pred) ? " #b1)" : " #b0)";
        break;
     }

    case PREDICATE_PARITY: 	    // Parity (PF==1)
    case PREDICATE_NOT_PARITY: 	// Not Parity (PF==0).
    {
        // 1) d�claration r�cursive des sources du flag
        this->declareObject(pTmgrTls->getTaintParityFlag());

        // 2) construction de l'expression
        result += "(= ";
        result += pTmgrTls->getTaintParityFlag()->getName();
        result += (PREDICATE_PARITY == pred) ? " #b1)" : " #b0)";
        break;
    }

    case PREDICATE_LESS: 	    // Less (SF!=OF).
    case PREDICATE_NOT_LESS: 	// Greater or Equal (SF==OF).
    {
        result += "(ite (= ";

        // insertion SIGN_FLAG : objet ou valeur selon marquage
        if (pTmgrTls->isSignFlagTainted())
        {
            // 1) d�claration r�cursive des sources du flag
            this->declareObject(pTmgrTls->getTaintSignFlag());
            result += pTmgrTls->getTaintSignFlag()->getName();
        }
        else
        {
            oneFlagValue = EXTRACTBIT(flagsOrRegValue, SIGN_FLAG);
            result      += (oneFlagValue) ? "#b1" : "#b0";
        }

        result += ' ';

        // insertion OVERFLOW_FLAG : objet ou valeur selon marquage
        if (pTmgrTls->isOverflowFlagTainted())
        {
            // 1) d�claration r�cursive des sources du flag
            this->declareObject(pTmgrTls->getTaintOverflowFlag());
            result += pTmgrTls->getTaintOverflowFlag()->getName();
        }
        else
        {
            oneFlagValue = EXTRACTBIT(flagsOrRegValue, OVERFLOW_FLAG);
            result      += (oneFlagValue) ? "#b1" : "#b0";
        }

        // si egaux, flag vaut 0 dans cas LESS et 1 dans cas NOT_LESS
        result += (PREDICATE_LESS == pred) ? ") false true)" : ") true false)";
        break;
    }

    case PREDICATE_BELOW_OR_EQUAL: 	    // Below or Equal (CF==1 or ZF==1), ou (CF or ZF) == 1
    case PREDICATE_NOT_BELOW_OR_EQUAL: 	// Above (CF==0 and ZF==0), ou (CF or ZF) == 0
    {
        // calcul et test de la valeur du OR entre les deux flags
        result += "(= (bvor ";

        // insertion CARRY_FLAG : objet ou valeur selon marquage
        if (pTmgrTls->isCarryFlagTainted())
        {
            // 1) d�claration r�cursive des sources du flag
            this->declareObject(pTmgrTls->getTaintCarryFlag());
            result += pTmgrTls->getTaintCarryFlag()->getName();
        }
        else
        {
            oneFlagValue = EXTRACTBIT(flagsOrRegValue, CARRY_FLAG);
            result      += (oneFlagValue) ? "#b1" : "#b0";
        }

        result += ' ';

        // insertion ZERO_FLAG : objet ou valeur selon marquage
        if (pTmgrTls->isZeroFlagTainted())
        {
            // 1) d�claration r�cursive des sources du flag
            this->declareObject(pTmgrTls->getTaintZeroFlag());
            result += pTmgrTls->getTaintZeroFlag()->getName();
        }
        else
        {
            oneFlagValue = EXTRACTBIT(flagsOrRegValue, ZERO_FLAG);
            result      += (oneFlagValue) ? "#b1" : "#b0";
        }

        // BELOW_OR_EQUAL: le flag vaut 1 si le 'OR' vaut 1
        result += (PREDICATE_BELOW_OR_EQUAL == pred) ? ") #b1)" : ") #b0)";
        break;
    }

    case PREDICATE_LESS_OR_EQUAL: // Less or Equal (ZF==1 or SF!=OF), ou ((SF ^ OF) or ZF) == 1
    case PREDICATE_NOT_LESS_OR_EQUAL: // Greater (ZF==0 and SF==OF),  ou ((SF ^ OF) or ZF) == 0
    {
        // calcul et test de la valeur du OR entre (bvxor SF OF) et ZF
        result += "(= (bvor (bvxor ";

        // insertion SIGN_FLAG : objet ou valeur selon marquage
        if (pTmgrTls->isSignFlagTainted())
        {
            // 1) d�claration r�cursive des sources du flag
            this->declareObject(pTmgrTls->getTaintSignFlag());
            result += pTmgrTls->getTaintSignFlag()->getName();
        }
        else
        {
            oneFlagValue = EXTRACTBIT(flagsOrRegValue, SIGN_FLAG);
            result      += (oneFlagValue) ? "#b1" : "#b0";
        }

        result += ' ';

        // insertion OVERFLOW_FLAG : objet ou valeur selon marquage
        if (pTmgrTls->isOverflowFlagTainted())
        {
            // 1) d�claration r�cursive des sources du flag
            this->declareObject(pTmgrTls->getTaintOverflowFlag());
            result += pTmgrTls->getTaintOverflowFlag()->getName();
        }
        else
        {
            oneFlagValue = EXTRACTBIT(flagsOrRegValue, OVERFLOW_FLAG);
            result      += (oneFlagValue) ? "#b1" : "#b0";
        }

        result += ") ";

        // insertion ZERO_FLAG : objet ou valeur selon marquage
        if (pTmgrTls->isZeroFlagTainted())
        {
            // 1) d�claration r�cursive des sources du flag
            this->declareObject(pTmgrTls->getTaintZeroFlag());
            result += pTmgrTls->getTaintZeroFlag()->getName();
        }
        else
        {
            oneFlagValue = EXTRACTBIT(flagsOrRegValue, ZERO_FLAG);
            result      += (oneFlagValue) ? "#b1" : "#b0";
        }

        // LESS_OR_EQUAL: le flag vaut 1 si le 'OR' vaut 1
        result += (PREDICATE_LESS_OR_EQUAL == pred) ? ") #b1)" : ") #b0)";
        break;
    }

    case PREDICATE_CX_NON_ZERO: 	// CX != 0.
    {
        // r�cup�ration de l'objet repr�sentant CX, et d�claration de celui-ci
        TaintWordPtr regCXPtr = pTmgrTls->getRegisterTaint<16>(REG_CX, flagsOrRegValue);
        this->declareObject(regCXPtr);

        // insertion de nom de l'objet, maintenant qu'il est d�clar�, et comparaison � 0
        result += "(not (= " + regCXPtr->getName() + " (_ bv0 16)))";
        break;
    }

    case PREDICATE_ECX_NON_ZERO: 	// ECX != 0.
    {
        // r�cup�ration de l'objet repr�sentant ECX, et d�claration de celui-ci
        TaintDwordPtr regECXPtr = pTmgrTls->getRegisterTaint<32>(REG_ECX, flagsOrRegValue);
        this->declareObject(regECXPtr);

        // insertion de nom de l'objet, maintenant qu'il est d�clar�, et comparaison � 0
        result += "(not (= " + regECXPtr->getName() + " (_ bv0 32)))";
        break;
    }
#if TARGET_IA32E
    case PREDICATE_RCX_NON_ZERO: 	// RCX != 0.
    {
        // r�cup�ration de l'objet repr�sentant RCX, et d�claration de celui-ci
        TaintQwordPtr regRCXPtr = pTmgrTls->getRegisterTaint<64>(REG_RCX, flagsOrRegValue);
        this->declareObject(regRCXPtr);

        // insertion de nom de l'objet, maintenant qu'il est d�clar�, et comparaison � 0
        result += "(not (= " + regRCXPtr->getName() + " (_ bv0 64)))";
        break;
    }
#endif
    }
    return (result);
} // getPredicateFormula

////////////////////////////////////
// DECLARATION DES OBJETS MARQUES //
////////////////////////////////////

void SolverFormula::declareObject(const TaintPtr &tPtr) 
{
    // traitement uniquement si la variable n'a jamais �t� d�clar�e 
    if ( !tPtr->isDeclared()) 
    {
        // d�claration de la variable
        tPtr->setDeclared();

        // r�cup�ration des sources, et d�clarations r�cursives de celles ci
        auto sources = tPtr->getSources();
        for (auto it = sources.begin() ; it != sources.end() ; ++it) 
        {
            if ((*it).isSrcTainted()) declareObject((*it).getTaintedSource());
        }

        // Maintenant que les sources sont d�clar�es, d�claration de l'objet
        // r�cup�ration de la relation de l'objet
        Relation rel = tPtr->getSourceRelation();
        
        if (rel == X_ASSIGN) 
        {
            // on affecte � l'objet le nom de la source
            // cela evite de d�clarer une nouvelle variable dans le solveur 
            tPtr->setName(sources.front().getTaintedSource()->getName()); 
        }
        else if (rel == BYTESOURCE) 
        {	
            // d�claration du nom de variable
            std::string objectName("OFF");
            objectName += decstr(sources.front().getValue());
            tPtr->setName(objectName);

            // enregistrement dans la formule
            this->_formula << "(declare-const " << objectName << " (_ BitVec 8))\n";
        }
       
        // relation n�e d'une instruction x86 ou d'un marquage de flag
        else this->declareRelation(tPtr, sources); 
    } 
} // declareObject

///////////////////////////////
// DECLARATION DES RELATIONS //
///////////////////////////////

void SolverFormula::declareRelation(const TaintPtr &tPtr, const vector<ObjectSource> &sources)
{
    UINT32 lengthInBitsOfResult = tPtr->getLength();  // longueur du r�sultat  
    const std::string lengthOfResultString(decstr(lengthInBitsOfResult)); // longueur en 'string'
    Relation rel = tPtr->getSourceRelation();   // relation r�sultat <-> sources
    
    // fabrication du nom de variable unique selon la taille du resultat
    std::string name; 
    switch (lengthInBitsOfResult) 
    {
    case 1:	 name = "TBIT"  + decstr(++(this->_iTbit));  break;
    case 8:	 name = "TB"    + decstr(++(this->_iTb));  break;
    case 16: name = "TW"    + decstr(++(this->_iTw));  break;
    case 32: name = "TDW"   + decstr(++(this->_iTdw)); break;
    case 64: name = "TQW"   + decstr(++(this->_iTqw)); break;
    case 128: name = "TDQW" + decstr(++(this->_iTdqw)); break;
    default : name = "error"; break;
    }
    // Affectation du nom � l'objet
    tPtr->setName(name);

    // declaration de l'ent�te de ligne : nom de variable et longueur
    std::string out("(define-fun " + name);
    out += " () (_ BitVec " + lengthOfResultString + ") (";

    // insertion des arguments selon la relation
    /****** MEGA BIG SWITCH ;););););); *****/
    switch (rel) 
    {
    case EXTRACT: 
    {
        // taille destination = taille de l'intervalle d'extraction (moins 1)
        UINT32 indexOfExtraction = static_cast<UINT32>(sources[1].getValue());

        // borne min = index d'extraction * longueur
        UINT32 lowerLimit = indexOfExtraction * lengthInBitsOfResult;
        // borne max = borne min + (longueur - 1)
        UINT32 higherLimit = lowerLimit + (lengthInBitsOfResult - 1);

        out += "(_ extract " + decstr(higherLimit) + ' ' + decstr(lowerLimit) + ") ";     

        // ajout de l'objet subissant l'extraction
        out += sources.front().getTaintedSource()->getName();
        break;
    }		
    case CONCAT:	
    {    
        // CONCAT : concat�nation des objets fournis en Sources
        // Attention : les objets seront ins�res du plus fort au plus faible
        // d'ou le rbegin/rend (cf def CONCAT du langage SMTLIB)

        out += "concat";
        for (auto it = sources.rbegin();  it != sources.rend() ; ++it) 
        {
            out += ' ';
            this->insertSourceName(out, *it);
        } 
        break;
    }
    case X_SIGNEXTEND:
    case X_ZEROEXTEND:
    {
        out += (X_SIGNEXTEND == rel) ? "(_ sign_extend " : "(_ zero_extend ";
        // nombre de bits � ajouter = longueur resultat - longueur source
        out += decstr(lengthInBitsOfResult - sources.front().getLength());
        out += ") ";

        // insertion du nom de l'objet
        this->insertSourceName(out, sources.front());
        break;
    }  
    case X_ADD:
    {
        out += "bvadd ";
        this->insertSourceName(out, sources.front());
        out += ' ';
        this->insertSourceName(out, sources[1]);
        break;
    }
    case X_SUB:
    {
        out += "bvsub ";
        this->insertSourceName(out, sources.front());
        out += ' ';
        this->insertSourceName(out, sources[1]);
        break;
    }
    case X_AND:
    {
        out += "bvand ";
        this->insertSourceName(out, sources.front());
        out += ' ';
        this->insertSourceName(out, sources[1]);
        break;
    }
    case X_OR:
    {
        out += "bvor ";
        this->insertSourceName(out, sources.front());
        out += ' ';
        this->insertSourceName(out, sources[1]);
        break;
    }
    case X_XOR:
    {
        out += "bvxor ";
        this->insertSourceName(out, sources.front());
        out += ' ';
        this->insertSourceName(out, sources[1]);
        break;
    }
    case X_INC:
    case X_DEC:
    {
        out += (rel == X_INC) ? "bvadd " : "bvsub ";
        out += sources.front().getTaintedSource()->getName();
        out += " (_ bv1 " + lengthOfResultString + ')';
        break;
    }
    case X_NEG:	
    case X_NOT:
    {
        out += (rel == X_NEG) ? "bvneg " : "bvnot ";
        this->insertSourceName(out, sources.front());
        break;
    }
    case X_SHL:
    case X_SHR:
    case X_SAR:
    {
        out += (rel == X_SHL) ? "bvshl " : ((rel == X_SHR) ? "bvlshr " : "bvashr ");
        this->insertSourceName(out, sources.front());
        out += ' ';

        // si le d�calage est non marqu� : il a d�j� �t� masqu� dans l'analyse
        // insertion du d�calage au format d�cimal
        if (!sources[1].isSrcTainted())
        {
            out += "(_ bv";
            out += decstr(sources[1].getValue());
            out += ' ';
            out += lengthOfResultString;
            out += ')';
        } 
        else // si d�calage marqu� (8bits) : masquage � 0x1f (ou 0x3f si 64 bits)
        {
            if (lengthInBitsOfResult != 8) // ajustement � la longueur de la source d�cal�e
            {  
                out += "((_ zero_extend ";
                out += decstr(lengthInBitsOfResult - 8);
                out += ") ";
            }
            out += "(bvand ";
            out += sources[1].getTaintedSource()->getName();
            out += (lengthInBitsOfResult == 64) ? " #x3f)" : " #x1f)";
            // si utilisation de zero_ext : parenth�se fermante en +
            if (lengthInBitsOfResult != 8)   out += ')'; 
        }
        break;
    }
    case X_ROR:
    {
        // src 0 = objet qui subit le d�calage (marqu� ou non), src 1 = d�calage (marqu� ou non)
        
        // si d�placement non marqu� : utilisation de l'instruction SMTLIB '_ rotate_right'
        if (!sources[1].isSrcTainted())
        {
            out += "(_ rotate_right ";
            out += decstr(sources[1].getValue());
            out += ") ";
            this->insertSourceName(out, sources.front());
        }
        // si depl marqu�, obligation de passer par deux shifts (cf _rotr de stblib.h)
        else
        {
            // _rotr(val, depl) = (val >> depl) | (val << (sizeof(val) * 8 - depl))
            // comme pour shift, obligation de mettre l'objet "depl" (sur 8 bits) 
            // � la meme longueur que l'objet d�plac� : utilisation de ZeroExtend si necessaire
            const std::string adjustLengthForShift = (lengthInBitsOfResult == 8) 
                ? "" : ("((_ zero_extend " + decstr(lengthInBitsOfResult - 8) + ") ");

            // masquage du d�placement � 0x1f (ou 0x3f en 64 bits)
            out += "let ((l_depl (bvand ";
            out += sources[1].getTaintedSource()->getName();  // objet repr�sentant le d�placement
            out += (lengthInBitsOfResult == 64) ? " #x3f" : " #x1f";

            // (val >> s) |
            out += "))) (bvor (bvlshr ";
            // objet subissant la rotation, marqu� ou non
            this->insertSourceName(out, sources.front()); 
            out += ' ';
            out += adjustLengthForShift;
            out += "l_depl)) (bvshl ";

            // (val << (sizeof(val) * 8 - s)) <=> bvshl(rotSrc((_ ze bvsub (lengthInBitsOfResult deplSrc)) 
            this->insertSourceName(out, sources.front());
            out += ' ';
            out += adjustLengthForShift;
            out += "(bvsub (_ bv";
            out += lengthOfResultString;
            out += " 8) l_depl))))";
        }
        break;
    }
    case X_ROL: 
    {
        // src 0 = objet qui subit le d�calage (marqu� ou non), src 1 = d�calage (marqu� ou non)
        
        // si d�placement non marqu� : utilisation de l'instruction SMTLIB '_ rotate_left'
        if (!sources[1].isSrcTainted())
        {
            out += "(_ rotate_left ";
            out += decstr(sources[1].getValue());
            out += ") ";
            this->insertSourceName(out, sources.front());
        }
        // si depl marqu�, obligation de passer par deux shifts (cf _rotl de stblib.h)
        else
        {
            // _rotl(val, s) = (val << s) | (val >> (sizeof(val) * 8 - s))
            // comme pour shift, obligation de mettre l'objet "depl" (sur 8 bits) 
            // � la meme longueur que l'objet d�plac� : utilisation de ZeroExtend si n�cessaire
            const std::string adjustLengthForShift = (lengthInBitsOfResult == 8) 
                ? "" : ("((_ zero_extend " + decstr(lengthInBitsOfResult - 8) + ") ");

            // masquage du d�placement � 0x1f (ou 0x3f en 64 bits)
            out += "let ((l_depl (bvand ";
            out += sources[1].getTaintedSource()->getName();  // objet repr�sentant le d�placement
            out += (lengthInBitsOfResult == 64) ? " #x3f" : " #x1f";

            // (val >> s) |
            out += "))) (bvor (bvshl ";
            // objet subissant la rotation, marqu� ou non
            this->insertSourceName(out, sources.front());
            out += ' ';
            out += adjustLengthForShift;
            out += "l_depl)) (bvlshr ";

            // (val << (sizeof(val) * 8 - s)) <=> bvshl(rotSrc((_ ze bvsub (lengthInBitsOfResult deplSrc)) 
            this->insertSourceName(out, sources.front());
            out += ' ';
            out += adjustLengthForShift;
            out += "(bvsub (_ bv";
            out += lengthOfResultString;
            out += " 8) l_depl))))";
        }
        break;
    }
    case X_RCR: 
    {
        // idem dans l'approche que pour ROR, sauf que la source est ici la concat�nation 
        // de sources[0](source) avec sources[1](CarryFlag) 
        // => obligation de faire toutes les operations (bvshl, bvsub, ... ) sur (lengthInBits + 1 bits)
        // et au final d'extraire les 'lengthInBits' bits forts

        out += "(_ extract ";
        out += lengthOfResultString;
        out += " 1)";

        // si d�placement non marqu� : utilisation de l'instruction SMTLIB '_ rotate_right'
        if (!sources[2].isSrcTainted())
        {
            out += "((_ rotate_right ";
            out += decstr(sources[2].getValue());

            // concatenation de la source avec le carryFlag
            out += ") (concat ";
            this->insertSourceName(out, sources.front());
            out += ' ';
            this->insertSourceName(out, sources[1]);
            out += "))";
        }
        // si depl marqu�, obligation de passer par deux shifts (cf _rotr de stblib.h)
        else
        {
            // _rotr(val, s) = (val >> s) | (val << (sizeof(val) * 8 - s))
            // comme pour shift, obligation de mettre l'objet "depl" (sur 8 bits) 
            // � la meme longueur que l'objet d�plac� : utilisation de ZeroExtend � lengthInBits+1 bits
            const std::string adjustLengthForShift
                ("((_ zero_extend " + decstr(lengthInBitsOfResult - 7) + ") ");

            // masquage du d�placement � 0x1f (ou 0x3f en 64 bits)
            out += "(let ((l_depl (bvand ";
            out += sources[2].getTaintedSource()->getName();  // objet repr�sentant le d�placement
            out += (lengthInBitsOfResult == 64) ? " #x3f" : " #x1f";

            // construction de la concatenation source/carry Flag
            out += ")) (src_cf (concat ";
            this->insertSourceName(out, sources.front());
            out += ' ';
            this->insertSourceName(out, sources[1]);

            // (val >> s) | (val << (sizeof(val) * 8 - s)) 
            out += "))) (bvor (bvlshr src_cf ";
            out += adjustLengthForShift;
            out += "l_depl)) (bvshl src_cf ";

            out += adjustLengthForShift;
            out += "(bvsub (_ bv";
            out += lengthOfResultString;
            out += " 8) l_depl)))))";
        }
        break;
    }
    case X_RCL:
    {
        // idem dans l'approche que pour ROL, sauf que la source est ici la concat�nation 
        // de sources[0](source) avec sources[1](CarryFlag) : (CF)(Src0)
        // => obligation de faire toutes les operations (bvshr, bvsub, ... ) sur (lengthInBits + 1 bits)
        // et au final d'extraire les 'lengthInBits' bits faibles

        out += "(_ extract ";
        out += decstr(lengthInBitsOfResult - 1);
        out += " 0)";

        // si d�placement non marqu� : utilisation de l'instruction SMTLIB '_ rotate_left'
        if (!sources[2].isSrcTainted())
        {
            out += "((_ rotate_left ";
            out += decstr(sources[2].getValue());
            
            // concatenation de la source avec le carryFlag
            out += ") (concat ";
            this->insertSourceName(out, sources.front());
            out += ' ';
            this->insertSourceName(out, sources[1]);
            out += "))";
        }
        // si depl marqu�, obligation de passer par deux shifts (cf _rotl de stblib.h)
        else
        {
            // _rotl(val, s) = (val << s) | (val >> (sizeof(val) * 8 - s))
            // comme pour shift, obligation de mettre l'objet "depl" (sur 8 bits) 
            // � la meme longueur que l'objet d�plac� : utilisation de ZeroExtend
            const std::string adjustLengthForShift
                ("((_ zero_extend " + decstr(lengthInBitsOfResult - 7) + ") ");

            // masquage du d�placement � 0x1f (ou 0x3f en 64 bits)
            out += "(let ((l_depl (bvand ";
            out += sources[2].getTaintedSource()->getName();  // objet repr�sentant le d�placement
            out += (lengthInBitsOfResult == 64) ? " #x3f" : " #x1f";
            
            // construction de la concatenation source/carry Flag
            out += ")) (src_cf (concat ";
            this->insertSourceName(out, sources.front());
            out += ' ';
            this->insertSourceName(out, sources[1]);
            
            // (val << s) | (val >> (sizeof(val) * 8 - s)) 
            out += "))) (bvor (bvshl src_cf ";
            out += adjustLengthForShift;
            out += "l_depl)) (bvlshr src_cf ";
            
            out += adjustLengthForShift;
            out += "(bvsub (_ bv";
            out += lengthOfResultString;
            out += " 8) l_depl)))))";
        }
        break;
    }
    case X_COMPLEMENT_BIT:
    case X_SET_BIT:
    {
        // COMPLEMENT : XOR de la source avec (1<<numero bit)
        // SET : OR de la source avec (1<<numero bit)
        // position du bit calcul�e modulo 16/32/64 (soit un AND avec 15/31/63)
        // ie (bvor/bvxor 'src0' (bvshl (_bv1 'srcLen') (bvand 'src1' (_bv15/31/63 'srcLen')))
       
        UINT32 srcLength = sources.front().getLength(); // nombre de bits de la source
        const std::string srcLengthName(decstr(srcLength)); // repr�sentation en string

        out += (rel == X_SET_BIT) ? "bvor " : "bvxor ";
        this->insertSourceName(out, sources.front());

        out += " (bvshl (_ bv1 ";
        out += srcLengthName; // nombre 1 sur x bits
        out += ") (bvand ";
        this->insertSourceName(out, sources[1]);

        // nombre 15/31/64 sur 'lengthInBits' bits
        out += " (_ bv";
        out += decstr(srcLength - 1);
        out += ' ';
        out += srcLengthName;
        out += ")))";
        break;
    }
    case X_CLEAR_BIT:
    {
        // AND de la source avec ~(1<<numero bit)
        // position du bit calcul�e modulo 16/32/64 (soit un AND avec 15/31/63)
        // ie (bvand 'src0' (bvnot (bvshl (_bv1 'srcLen') (bvand 'src1' (_bv15/31/63 'srcLen'))))
       
        UINT32 srcLength = sources.front().getLength(); // nombre de bits de la source
        const std::string srcLengthName(decstr(srcLength)); // repr�sentation en string

        out += "bvand ";
        this->insertSourceName(out, sources.front());

        out += " (bvnot (bvshl (_ bv1 ";
        out += srcLengthName; // nombre 1 sur x bits
        out += ") (bvand ";
        this->insertSourceName(out, sources[1]);

        // nombre 15/31/64 sur 'lengthInBits' bits
        out += " (_ bv";
        out += decstr(srcLength - 1);
        out += ' ';
        out += srcLengthName;
        out += "))))";
        break;
    }
    case X_IMUL:
    {  
        // longueur resultat = 2*longueur des sources 
        // donc necessite de mettre les op�randes � la longueur de
        // la destination ; le nombre de z�ros � ajouter est �gal � 
        // la longueur de la source (= moiti� longueur du r�sultat)
        const std::string zeroExtend("((_ zero_extend " + decstr(lengthInBitsOfResult >> 1) + ") ");

        out += "bvmul ";
        // 1ere source, zero-�tendue
        out += zeroExtend;
        this->insertSourceName(out, sources.front());
        out += ") ";
        // 2eme source, zero-�tendue
        out += zeroExtend;
        this->insertSourceName(out, sources[1]);
        out += ')';
    
        break;
    }
    case X_DIV_QUO:
    case X_DIV_REM:
    case X_IDIV_QUO:
    case X_IDIV_REM:
    {
        // taille resultat = taille diviseur = 1/2 taille dividende
        // il faut donc effectuer "Dividende / zero_extend(diviseur)" 
        // et en extraire la partie basse pour obtenir 
        // le r�sultat (DIV_QUO) ou le reste (DIV_REM)
        // Src 0 : dividende (entier si 8 bits, sinon partie haute)
        // Src 1 : dividende (partie basse) ou diviseur (si 8 bits)
        // Src 2 : diviseur (si diff�rent 8 bits), sinon rien
        const std::string instruction( 
                       (rel == X_DIV_QUO) ? "(bvudiv "
                    : ((rel == X_DIV_REM) ? "(bvurem "
                    : ((rel == X_IDIV_QUO) ? "(bvsdiv "
                    : "(bvsrem ")));

        out += "(_ extract ";
        switch (lengthInBitsOfResult)
        {
        case 8: // resultat 8bits : dividende en src0, diviseur 8b en src 1
            out += "7 0) ";
            out += instruction;
            
            // dividende (16 bits) - source 0
            this->insertSourceName(out, sources.front());    

            // extension sur 16 bits du diviseur (8 bits) - source 1
            out += " ((_ zero_extend 8) ";       
            this->insertSourceName(out, sources[1]);
            break;
        // Autres longueurs => Source0 : partie forte du dividende
        // Source1 : partie faible du dividende
        // Source2 : diviseur
        case 16:
            // extraction de la partie faible de la division
            out += "15 0) " ;
            out += instruction;

            // division : concat�nation du dividende (sources 0 et 1)
            out += "(concat ";
            this->insertSourceName(out, sources.front());          
            out += ' ';
            this->insertSourceName(out, sources[1]); 

            // extension du diviseur � la longueur du dividende
            out += ") ((_ zero_extend 16) ";
            this->insertSourceName(out, sources[2]);
            break;
        case 32:
            // extraction de la partie faible de la division
            out += "31 0) " ;
            out += instruction;

            // division : concat�nation du dividende (sources 0 et 1)
            out += "(concat ";
            this->insertSourceName(out, sources.front());          
            out += ' ';
            this->insertSourceName(out, sources[1]);

            // extension du diviseur � la longueur du dividende
            out += ") ((_ zero_extend 32) ";
            this->insertSourceName(out, sources[2]);
            break;
        #if TARGET_IA32E
        case 64:
            // extraction de la partie faible de la division
            out += "63 0) " ;
            out += instruction;

            // division : concat�nation du dividende (sources 0 et 1)
            out += "(concat ";
            this->insertSourceName(out, sources.front());          
            out += ' ';
            this->insertSourceName(out, sources[1]); 

            // extension du diviseur � la longueur du dividende
            out += ") ((_ zero_extend 64) ";
            this->insertSourceName(out, sources[2]);
            break;
        #endif
        }
        // fin de formule
        out += "))";
        break;
    }
          
    case X_BSF:
    {
        /* Bit Scan Forward : index du LSB de la source
        cet index vaut index64[((bb ^ (bb-1)) * debruijn64) >> 58]
        les variables interm�diaires seront impl�ment�es via des 'let'
        Le calcul se fait sur 64 bits : la source devra �tre zero extended
        si besoin. Exemple pour une source de 32 bits:
        (define-fun BSF32 () (_ BitVec 64) (
           let ((bb TDW1)) 
          (let ((bbMin1 (bvsub bb (_ bv1 32)))) 
          (let ((bbXor (bvxor bb bbMin1)))
          (let ((bbZeroExt ((_ zero_extend 32) bbXor))) 
          (let ((bbMul (bvmul bbZeroExt constante))) 
          (let ((bbShift (bvlshr bbMul (_ bv58 64)))) 
          ((_ extract 31 0) (index64 bbShift)))))))))  => ce qui sera affect� � BSF */

        UINT32 srcLength = sources.front().getLength(); // nombre de bits de la source 

        // si le tableau et la constante n'ont pas encore �t� d�clar�, le faire
        if (!this->_isDeBruijnDeclared) out = this->getDeBruijnArray() + out;

        out += "\n  let ((bb ";
        this->insertSourceName(out, sources.front());
        out += "))\n";

        // bbMin1 = bb-1
        out += "  (let ((bbMin1 (bvsub bb (_ bv1 " + decstr(srcLength) + "))))\n";
        // bbXor = bb ^ (bb-1)
        out += "  (let ((bbXor (bvxor bb bbMin1)))\n";
        
        // traitement diff�rent selon que la source soit sur 64 bits ou non
        if (srcLength != 64)
        {
            out += "  (let ((bbZeroExt ((_ zero_extend " + decstr(64 - srcLength) + ") bbXor)))\n";
            // multiplication par la constante
            out += "  (let ((bbMul (bvmul bbZeroExt constante)))\n";
            // shift de 58 bits
            out += "  (let ((bbShift (bvlshr bbMul (_ bv58 64))))\n";
            // extraction sur 'length' bits de la valeur du tableau � l'index calcul�
            out += "  ((_ extract " + decstr(srcLength - 1) + " 0) (index64 bbShift))";
            // 5 lets (en dehors du premier) => 5 parenth�ses fermantes
            out += ")))))";
        }
        else 
        {
            out += "  (let ((bbMul (bvmul bbXor constante)))\n";
            // shift de 58 bits
            out += "  (let ((bbShift (bvlshr bbMul (_ bv58 64))))\n";
            // r�cup�ration de la valeur du tableau � l'index calcul�
            out += "  (index64 bbShift)";
            // 4 lets (en dehors du premier) => 4 parenth�ses fermantes
            out += "))))";
        }        

        break;
    }

    case X_BSR:
    {
        /* Bit Scan Reverse : index du MSB de la source
        cet index vaut :
        bb |= bb >> 1; bb |= bb >> 2;  bb |= bb >> 4; bb |= bb >> 8; 
        bb |= bb >> 16; (cas 32 et 64bits)
        bb |= bb >> 32; (cas 64bits)
        index = index64[(bb * debruijn64) >> 58];
        
        les variables interm�diaires seront impl�ment�es via des 'let'
        Le calcul se fait sur 64 bits : la source devra �tre zero extended si besoin. 
        TODO : en 32bits on s'arrete au shift16. En 16 bits on s'arrete au shift8
       
        Exemple pour une source de 32 bits:
         (define-fun BSR32 () (_ BitVec 32) (
          let ((bbShift0 TDW1)) 
         (let ((bbShift1 (bvor bbShift0 (bvlshr bbShift0 (_ bv1 32))))) 
         (let ((bbShift2 (bvor bbShift1 (bvlshr bbShift1 (_ bv2 32))))) 
         (let ((bbShift3 (bvor bbShift2 (bvlshr bbShift2 (_ bv4 32))))) 
         (let ((bbShift4 (bvor bbShift3 (bvlshr bbShift3 (_ bv8 32))))) 
         (let ((bbShift5 (bvor bbShift4 (bvlshr bbShift4 (_ bv16 32))))) 
         (let ((bbShift6 (bvor bbShift5 (bvlshr bbShift5 (_ bv32 32)))))
         (let ((bbZeroExt ((_ zero_extend 32) bbShift6))) ; ZERO_EXTEND car calcul sur 64bits
         (let ((bbMul (bvmul bbZeroExt constante))) 
         (let ((bbShift (bvlshr bbMul (_ bv58 64)))) 
         ((_ extract 31 0) (index64 bbShift)))))))))))) => ce qui sera affect� � BSR32 */

        // si le tableau et la constante n'ont pas encore �t� d�clar�, le faire
        if (!this->_isDeBruijnDeclared) out = this->getDeBruijnArray() + out;

        const std::string firstVar("bbShift0 ");
        out += "\n   let ((" + firstVar;
        this->insertSourceName(out, sources.front());
        out += " ))\n";

        // nombre d'iteration de shifts
        UINT32 iterationRound = 1;
        // valeur du shift
        UINT32 shiftValue     = 1;
        // longueur en bit de la source
        UINT32 srcLength      = sources.front().getLength();
        // longueur en bit de la source (sous forme de string)
        std::string lengthOfSrcString(decstr(sources.front().getLength())); 
        // nom de la derni�re variable temporaire d�finie
        std::string lastVar(firstVar);
        // nom de la variable temporaire qui va �tre d�finie
        std::string var;

        // on s'arrete lorsque la valeur du shift atteint la longueur des sources
        // car les shifts suivants n'auront plus aucun effet
        while (shiftValue < srcLength)
        {
            // nom de la variable temporaire ins�r�e
            var = "bbShift" + decstr(iterationRound) + ' ';
            out += "  (let ((" + var + "(bvor " + lastVar + "(bvlshr " + lastVar;
            
            // insertion valeur du shift sur 'lengthOfResult' bits
            out += "(_ bv" + decstr(shiftValue) + ' ' + lengthOfResultString + ")))))\n";
            
            // mise � jour des variables pour prochain tour
            ++iterationRound;
            shiftValue <<= 1;
            lastVar = var;
        }

        // zero_extend de la source si elle n'�tait pas sur 64bits
        if (srcLength != 64)
        {
            out += "  (let ((bbZeroExt ((_ zero_extend ";
            out += decstr(64 - srcLength) + ") ";
            // insertion du nom de la derni�re variable � �tendre
            out += lastVar + ")))\n";
            // la derni�re variable sera d�sormais "bbZeroExt"
            lastVar = "bbZeroExt";
        }

        // multiplication par la constante, et shift
        out += "  (let ((bbMul (bvmul " + lastVar + " constante)))\n";
        out += "  (let ((bbShift (bvlshr bbMul (_ bv58 64))))\n";

        // r�cup�ration de la valeur dans le tableau. Mise � la taille du resultat
        // (par une extraction) si besoin
        if (srcLength != 64)
        {
            out += "  ((_ extract " + decstr(srcLength - 1) + " 0) (index64 bbShift))";
            // nombre de parenth�ses fermantes 
            //      1 par bbShiftXX (4 ou 5)
            //      1 pour le ZeroExt
            //      2 pour bbMul et bbShift => TOTAL de 7 (cas 16 bits) ou 8 (cas 32bits)
            out += ")))))))";
            if (srcLength == 32) out += ')';
        }
        else
        {
            out += "  (index64 bbShift)";
            // nombre de parenth�ses fermantes
            //      1 par bbShift (soit 6)
            //      2 pour bbMul et bbShift => TOTAL de 8
            out += "))))))))";
        }
        break;
    }

    /*************************************/
    /************ FLAGS ******************/
    /*************************************/

    case F_LSB:	
    {
        out += "(_ extract 0 0) ";
        this->insertSourceName(out, sources.front()); 
        break;
    }
    case F_MSB:		
    {
        // position du signe d�pendant de la longueur de la source
        const std::string extractIndex(decstr(sources.front().getLength() - 1));
        out += "(_ extract " + extractIndex + ' ' + extractIndex + ") ";

        this->insertSourceName(out, sources.front());  
        break;
    }
    case F_CARRY_ADD:
    {
        // Extension d'1 bit des op�randes afin d'extraire le bit fort de leur somme

        const std::string extractIndex(decstr(sources.front().getLength()));
        const std::string extendSource("((_ zero_extend 1) ");

        out += "(_ extract " + extractIndex + ' ' + extractIndex + ") (bvadd ";

        // zeroextend de l'op�rande src/dest
        out += extendSource;
        this->insertSourceName(out, sources.front());
        out += ") ";

        // zeroextend de l'op�rande src
        out += extendSource;
        this->insertSourceName(out, sources[1]);
        out += ")) ";

        break;
    }
    case F_CARRY_SUB:
    {
        // bvult renvoie True si op�rande 1 < op�rande 2
        out += "ite (bvult ";
        this->insertSourceName(out, sources.front());
        out += ' ';
        this->insertSourceName(out, sources[1]);
        out += ") #b1 #b0";
        break;   
    }
    case F_CARRY_SHL:
    {
        // dernier bit eject� vers la gauche = bit (lengthInBits - count), count marqu� (8 bits)
        // r�cup�ration par LSB (src >> (lengthInBits - count))
        // ATTENTION : count doit etre auparavant masqu� � 0x1f ou 0x3f
        const std::string lengthOfResultString(lengthOfResultString);

        // &ere partie du shift : source d�plac�e
        out += "(_ extract 0 0) (bvlshr ";
        this->insertSourceName(out, sources.front());

        // 2eme partie du shift = d�placement (lengthInBits - count), 
        // la soustraction est calcul�e sur 8 bits puis zero_ext � la longueur
        // de la source (cad si source != 8 bits)

        if (lengthInBitsOfResult != 8)
        {
            out += " ((_ zero_extend " + decstr(lengthInBitsOfResult - 8) + ") ";
        }
        out += " (bvsub #" + StringHex(lengthInBitsOfResult, 2, false);

        // masquage du d�placement 0x1f ou 0x3f en 64bits 
        out += " (bvand ";
        this->insertSourceName(out, sources[1]);
        out += (lengthInBitsOfResult == 64) ? " #x3f)))" : " #x1f)))";

        // parenth�se fermante correspondant � _ zero_extend
        if (lengthInBitsOfResult != 8) out += ')';

        break;
    }
    case F_CARRY_SHR:
    {
        // dernier bit eject� vers la gauche = bit (count - 1), count marqu� (8 bits)
        // r�cup�ration par LSB (src >> (count - 1))
        // ATTENTION : count doit etre auparavant masqu� � 0x1f ou 0x3f
        out += "(_ extract 0 0) (bvlshr ";
        this->insertSourceName(out, sources.front());

        // la soustraction est calcul�e sur 8 bits puis zero_ext � la longueur
        // de la source (cad si source != 8 bits)

        if (lengthInBitsOfResult != 8)
        {
            out += " ((_ zero_extend " + decstr(lengthInBitsOfResult - 8) + ") ";
        }

        // soustraction apres masquage du d�placement 0x1f ou 0x3f en 64bits 
        out += " (bvsub (bvand ";
        this->insertSourceName(out, sources[1]);
        out += "(_ bv1 ";
        out += lengthOfResultString;
        out += "))))";

        // parenth�se fermante correspondant � _ zero_extend
        if (lengthInBitsOfResult != 8) out += ')';

        break;
    }
    case F_CARRY_BITBYTE:
    {
        // AND de la source avec (1<<numero bit). Si r�sultat nul => bool�en vaudra 0, sinon 1
        // le num�ro du bit est calcul� modulo 16/32/64 (soit un AND avec 15/31/63)
        UINT32 srcLength = sources.front().getLength(); // nombre de bits de la source
        const std::string srcLengthName(decstr(srcLength)); // repr�sentation en string

        out += "ite (= (_ bv0 ";
        out += srcLengthName; // nombre 0 sur x bits
        
        // AND de source0 avec (bvshl (_bv1 'lengthInBits') (bvand 'numero' (_bv15/31/63 'lengthInBits')))
        out += ") (bvand ";
        this->insertSourceName(out, sources.front());
        out += " (bvshl (_ bv1 ";
        out += srcLengthName; // nombre 1 sur x bits
        out += ") (bvand ";
        this->insertSourceName(out, sources[1]);

        // nombre 15/31/64 sur 'lengthInBits' bits
        out += " (_ bv";
        out += decstr(srcLength - 1);
        out += ' ';
        out += srcLengthName;
        out += "))))) #b0 #b1";
        break;
    }
    case F_PARITY:	
    {
        // PARITY : parit� de l'octet faible de la variable
        // source http://graphics.stanford.edu/~seander/bithacks.html
        // char v;  v ^= v >> 4;   v &= 0xf;   p = (0x6996 >> v) & 1; 
        //   parit� de v = parit� de p. 
        //   Si p = 1, parit� sera 0 => inverser le r�sultat (d'ou le bvnot)

        out += "\n   let ((t0 ((_ extract 7 0) "; // seuls les 8 bits faibles sont consid�r�s
        this->insertSourceName(out, sources.front());
        out += ")))\n" \
          "  (let ((t1 (bvlshr t0 #x04)))\n" \
          "  (let ((t2 (bvxor t1 t0)))\n" \
          "  (let ((t3 (bvand t2 #x0f)))\n" \
          "  (let ((t4 (bvlshr #x6996 ((_ zero_extend 8) t3))))\n" \
          "  (let ((t5 ((_ extract 0 0) t4)))\n" \
          "  (bvnot t5))))))";

        break;
    }
    case F_IS_NULL:	
    case F_CARRY_NEG:
    {
        UINT32 srcLength = sources.front().getLength();
        // comparaison de la source au nombre 0 repr�sent� sur "lengthInBits" bits
        out += "ite (= (_ bv0 ";
        out += decstr(srcLength);
        out += ") ";
        this->insertSourceName(out, sources.front());

        // IS_ZERO = si �gal � 0 => 1, F_CARRY_NEG l'inverse
        out += (rel == F_IS_NULL) ? " ) #b1 #b0" : " ) #b0 #b1";	
        break;
    }
    case F_ARE_EQUAL:
    {
        out += "ite (= ";
        this->insertSourceName(out, sources.front());
        out += ' ';
        this->insertSourceName(out, sources[1]);
        out += ") #b1 #b0";
        break;
    }
    case F_OVERFLOW_ADD:	
    case F_OVERFLOW_SUB:	
    {
        // source : http://www.emulators.com/docs/nx11_flags.htm
        // ADD : Formule reprise � partir de BOCHS 2.6
        // OF(A+B) = MSB de (A XOR RESULT) AND (B XOR RESULT)
        // SUB : formule de BOCHS 2.3.5 (BOCHS 2.6 fait diff�remment)
        // OF(A-B) = MSB de (A XOR RESULT) AND (B XOR A)

        // source 0 = source A (marqu�e ou non) 
        // source 1 = source B (marqu�e ou non)   
        // source 2 = r�sultat (forc�ment marqu�)

        // index d'extraction du MSB
        const std::string extractIndex(decstr(sources.front().getLength() - 1));
       
        out += "(_ extract " + extractIndex + ' ' + extractIndex + ") ";
                
        // A XOR RESULT
        out += "(bvand (bvxor ";
        this->insertSourceName(out, sources.front());  
        out += ' ';
        this->insertSourceName(out, sources[2]);
                
        // B XOR RESULT (si ADD) ou B XOR A (si SUB)
        out += ") (bvxor ";  
        this->insertSourceName(out, sources[1]);
        out += ' ';
        if (rel == F_OVERFLOW_ADD)  this->insertSourceName(out, sources[2]);
        else                        this->insertSourceName(out, sources.front());

        out += "))";	
        break;
    }
    case F_OVERFLOW_INC:	
    case F_OVERFLOW_DEC: // idem OVERFLOW_NEG
    {
        // Le flag est mis � 1 si et seulement si
        // INC : la source vaut 0x7f/0x7fff/0x7fffffff
        // DEC : la source vaut 0x80/0x8000/0x80000000
               
        // ajout des "f" ou des "0" selon la longueur de la source
        // 8 bits : ajout d'un caract�re, 16b : ajout de 3 caract�res
        // 32b : 7 caract�res, 64b : ajout de 15 caract�res
        // => formule = ajout de "(srcLength / 4) - 1" caract�res
        char c = (rel == F_OVERFLOW_INC) ? 'f' : '0';
        UINT32 nb = (sources.front().getLength() >> 2) - 1;
        const std::string suffix(nb, c);

        out += "ite (= ";
        this->insertSourceName(out, sources.front());
        out += (rel == F_OVERFLOW_INC) ? " #x7" : " #x8";
        out += suffix;
        out += ") #b1 #b0";	
        break;
    }
    case F_OVERFLOW_SHL:	
    {
        // For 1-left shifts, the OF flag is set to 0 if the most-significant bit of the result 
        // is the same as the CF flag; otherwise, it is set to 1.
        // OF = (MSB "RESULT" == MSB "SOURCE") ? 0 : 1
        
        const std::string indexMSB(decstr(sources.front().getLength() - 1));
        const std::string extractMsb("((_ extract " + indexMSB + ' ' + indexMSB + ") ");

        out += "ite (= ";
        // 1) extraction msb du resultat
        out += extractMsb;
        this->insertSourceName(out, sources.front());
                
        // 2) extraction msb de la source
        out += ") ";
        extractMsb;
        this->insertSourceName(out, sources[1]);

        out += ")) #b0 #b1";
        break;
    }
    case F_OVERFLOW_ROL: // idem pour OVERFLOW_ROR
    {
        // ite (= (LSB 'src0' MSB 'src0') #b0 #b1)
        const std::string indexMSB = decstr(sources.front().getLength() - 1);
        
        out += "ite (= ((_ extract 0 0) ";
        this->insertSourceName(out, sources.front());
        out += ") ((_ extract " + indexMSB + ' ' + indexMSB + ") ";
        this->insertSourceName(out, sources.front());
        out +=")) #b0 #b1";
        
        break;
    }
    case F_OVERFLOW_RCL: // idem pour OVERFLOW_RCR
    {
        // ite (= ('src1' MSB 'src0') #b0 #b1)
        const std::string indexMSB = decstr(sources.front().getLength() - 1);
        
        out += "ite (= ";
        this->insertSourceName(out, sources[1]);
        out += " ((_ extract " + indexMSB + ' ' + indexMSB + ") ";
        this->insertSourceName(out, sources.front());
        out +=")) #b0 #b1";
        
        break;
    }
    case F_CARRY_IMUL:
    case F_CARRY_MUL:
    default: 
        out += "non cod� default"; 
        break;
    } // end switch 

    // Parenth�ses fermantes puis enregistrement dans la formule
    // en mode debug, ajout de la relation en toutes lettres
    #if DEBUG
    this->_formula << out << "));" << enum_strings[rel] << '\n';
    #else
    this->_formula << out << "));" << '\n';
    #endif
}

/////////////////////////////////
// DECLARATION DES CONTRAINTES //
/////////////////////////////////

void SolverFormula::addConstraintJcc(TaintManager_Thread *pTmgrTls, PREDICATE pred, 
                          bool isTaken, ADDRINT insAddress, ADDRINT flagsOrRegValue)
{
    // nom donn� � la contrainte : un "C" + un num�ro unique
    // La premi�re contrainte porte le num�ro 1 (_iAssert � 0 dans constructeur)
    const std::string constraintName("C_" + decstr(++this->_iAssert));
    // ligne de formule qui d�clare la contrainte
    std::string constraintFormula("(define-fun " + constraintName + " () Bool ");

    // 1) d�finition de l'ent�te de la contrainte : insertion d'un commentaire
    // avec le n� de contrainte, l'instruction et son adresse 
    this->_formula << ";\n; contrainte " << constraintName;
    this->_formula << " - " << hexstr(insAddress);
    this->_formula << " - J" << predicateToString[pred] << "\n;\n";

    // 2) construction du nom de l'objet repr�sentant la contrainte
    // il s'agit d'un bool�en �gal � la valeur du pr�dicat
    // NB : il n'est pas ins�r� directement dans la formule
    // car la construction du pr�dicat va d�clarer r�cursivement les objets n�cessaires
    constraintFormula += this->getPredicateFormula(pTmgrTls, pred, flagsOrRegValue) + ")\n";
    this->_formula << constraintFormula;

    // 3) d�claration de l'assertion en fin de contrainte 
    // selon que la branche a �t� prise ou non
    this->_formula << "(assert (= " << constraintName << ' ';
    this->_formula << ((isTaken) ? "true" : "false") << "))\n";

    // Si le nombre maximal de contraintes est atteint : quitter le pintool 
    // via la fonction "Fini" avec le code EXIT_MAX_CONSTRAINTS
    // si g_maxConstraints est nul, ce cas n'arrive jamais (la premi�re contrainte est la n�1)
    if (this->_iAssert == g_maxConstraints)  PIN_ExitApplication(EXIT_MAX_CONSTRAINTS);
} // addConstraintJcc

#if 0
void SolverFormula::addConstraint_OVERFLOW(TaintManager_Thread *pTmgrTls, ADDRINT insAddress, bool isTaken) 
{
    // 1) d�finition de l'ent�te de la contrainte
    this->declareConstraintHeader(insAddress, PREDICATE_OVERFLOW);

    // 2) d�claration de la formule de contrainte  OF = 1  Overflow
    // d�claration du flag marqu�, et r�cursivement de ses sources
    this->declareObject(pTmgrTls->getTaintOverflowFlag());
    
    // declaration de la contrainte : declaration de l'objet
    const std::string constraintNumber(decstr(this->_iAssert));
    std::string constraint("(define-fun C_");
    constraint += constraintNumber; 
        
    // formule : OF == 1
    constraint += "() Bool (= ";
    constraint += pTmgrTls->getTaintOverflowFlag()->getName();
    constraint += " #b1))\n";

    this->_formula << constraint;


    this->declareConstraintFooter(constraintNumber, isTaken);
} // addConstraint_OVERFLOW

void SolverFormula::addConstraint_PARITY(TaintManager_Thread *pTmgrTls, ADDRINT insAddress, bool isTaken) 
{
    // 1) d�finition de l'ent�te de la contrainte
    this->declareConstraintHeader(insAddress, PREDICATE_PARITY);

    // 2) d�claration de la formule de contrainte  PF = 1 Parity
    // d�claration du flag marqu�, et r�cursivement de ses sources
    this->declareObject(pTmgrTls->getTaintParityFlag());
    
    // declaration de la contrainte : declaration de l'objet
    const std::string constraintNumber(decstr(this->_iAssert));
    std::string constraint("(define-fun C_");
    constraint += constraintNumber;  
        
    // formule : PF == 1
    constraint += "() Bool (= ";
    constraint += pTmgrTls->getTaintParityFlag()->getName();
    constraint += " #b1))\n";

    this->_formula << constraint;

    // 3) d�claration de l'assertion en fin de contrainte
    // selon que la branche a �t� prise ou non
    this->declareConstraintFooter(constraintNumber, isTaken);
} // addConstraint_PARITY

void SolverFormula::addConstraint_SIGN(TaintManager_Thread *pTmgrTls, ADDRINT insAddress, bool isTaken) 
{
    // 1) d�finition de l'ent�te de la contrainte
    this->declareConstraintHeader(insAddress, PREDICATE_SIGN);
    
    // 2) d�claration de la formule de contrainte   SIGN : SF = 1
    // d�claration du flag marqu�, et r�cursivement de ses sources
    this->declareObject(pTmgrTls->getTaintSignFlag());
    
    // declaration de la contrainte : declaration de l'objet
    const std::string constraintNumber(decstr(this->_iAssert));
    std::string constraint("(define-fun C_");
    constraint += constraintNumber; 
        
    // formule : SF == 1
    constraint += "() Bool (= ";
    constraint += pTmgrTls->getTaintSignFlag()->getName();
    constraint += " #b1))\n";

    this->_formula << constraint;

    // 3) d�claration de l'assertion en fin de contrainte
    // selon que la branche a �t� prise ou non
    this->declareConstraintFooter(constraintNumber, isTaken);
} // addConstraint_SIGN

void SolverFormula::addConstraint_ZERO(TaintManager_Thread *pTmgrTls, ADDRINT insAddress, bool isTaken) 
{
    // 1) d�finition de l'ent�te de la contrainte
    this->declareConstraintHeader(insAddress, PREDICATE_ZERO);
    
    // 2) d�claration de la formule de contrainte ZF = 1 Equal/zero
    // d�claration du flag marqu�, et r�cursivement de ses sources
    this->declareObject(pTmgrTls->getTaintZeroFlag());
    
    // declaration de la contrainte : declaration de l'objet
    const std::string constraintNumber(decstr(this->_iAssert));
    std::string constraint("(define-fun C_");
    constraint += constraintNumber;
        
    // formule : ZF == 1
    constraint += "() Bool (= ";
    constraint += pTmgrTls->getTaintZeroFlag()->getName();
    constraint += " #b1))\n";

    this->_formula << constraint;

    // 3) d�claration de l'assertion en fin de contrainte
    // selon que la branche a �t� prise ou non
    this->declareConstraintFooter(constraintNumber, isTaken);
} // addConstraint_ZERO

void SolverFormula::addConstraint_BELOW(TaintManager_Thread *pTmgrTls, ADDRINT insAddress, bool isTaken) 
{
    // 1) d�finition de l'ent�te de la contrainte
    this->declareConstraintHeader(insAddress, PREDICATE_BELOW);

    // 2) d�claration de la formule de contrainte	CF = 1   Below	
    // d�claration du flag marqu�, et r�cursivement de ses sources
    this->declareObject(pTmgrTls->getTaintCarryFlag());
    
    // declaration de la contrainte : declaration de l'objet
    const std::string constraintNumber(decstr(this->_iAssert));
    std::string constraint("(define-fun C_");
    constraint += constraintNumber;  
        
    // formule : ZF == 1
    constraint += "() Bool (= ";
    constraint += pTmgrTls->getTaintCarryFlag()->getName();
    constraint += " #b1))\n";
    
    this->_formula << constraint;

    // 3) d�claration de l'assertion en fin de contrainte
    // selon que la branche a �t� prise ou non
    this->declareConstraintFooter(constraintNumber, isTaken);
} // addConstraint_BELOW

void SolverFormula::addConstraint_BELOW_OR_EQUAL
    (TaintManager_Thread *pTmgrTls, ADDRINT insAddress, bool isTaken, ADDRINT flagsValue) 
{
    // 1) d�finition de l'ent�te de la contrainte
    this->declareConstraintHeader(insAddress, PREDICATE_BELOW_OR_EQUAL);
    
    // 2) formule de contrainte  (CF or ZF) = 1  Below or equal	
    // d�claration des flags marqu�s, et r�cursivement de leurs sources
    if (pTmgrTls->isCarryFlagTainted()) this->declareObject(pTmgrTls->getTaintCarryFlag());
    if (pTmgrTls->isZeroFlagTainted())  this->declareObject(pTmgrTls->getTaintZeroFlag());

    // declaration de la contrainte : declaration de l'objet
    const std::string constraintNumber(decstr(this->_iAssert));
    std::string constraint("(define-fun C_");
    constraint += constraintNumber;
        
    // formule : (CF or ZF) == 1
    constraint += "() Bool (= (bvor ";
        
    if (pTmgrTls->isCarryFlagTainted())
    {
        constraint += pTmgrTls->getTaintCarryFlag()->getName();
        constraint += ' ';
    }
    else constraint += ((flagsValue >> CARRY_FLAG) & 1) ? "#b1 " : "#b0 ";

    if (pTmgrTls->isZeroFlagTainted()) constraint += pTmgrTls->getTaintZeroFlag()->getName();
    else constraint += ((flagsValue >> ZERO_FLAG) & 1) ? "#b1" : "#b0";

    constraint += ") #b1))\n";
 
    this->_formula << constraint;

    // 3) d�claration de l'assertion en fin de contrainte
    // selon que la branche a �t� prise ou non
    this->declareConstraintFooter(constraintNumber, isTaken);
} // addConstraint_BELOW_OR_EQUAL

void SolverFormula::addConstraint_LESS
    (TaintManager_Thread *pTmgrTls, ADDRINT insAddress, bool isTaken, ADDRINT flagsValue) 
{
    // 1) d�finition de l'ent�te de la contrainte
    this->declareConstraintHeader(insAddress, PREDICATE_LESS);
    
    // 2) d�claration de la formule de contrainte  (SF xor OF) = 1 Less
    // d�claration des flags marqu�s, et r�cursivement de leurs sources
    if (pTmgrTls->isSignFlagTainted())     this->declareObject(pTmgrTls->getTaintSignFlag());
    if (pTmgrTls->isOverflowFlagTainted()) this->declareObject(pTmgrTls->getTaintOverflowFlag());

    // declaration de la contrainte : declaration de l'objet
    const std::string constraintNumber(decstr(this->_iAssert));
    std::string constraint("(define-fun C_");
    constraint += constraintNumber;
        
    // formule : (SF Xor OF) == 1
    constraint += "() Bool (= (bvxor ";
    
    if (pTmgrTls->isSignFlagTainted())
    {
        constraint += pTmgrTls->getTaintSignFlag()->getName();
        constraint += ' ';
    }
    else constraint += ((flagsValue >> SIGN_FLAG) & 1) ? "#b1 " : "#b0 ";
        
    if (pTmgrTls->isOverflowFlagTainted()) constraint += pTmgrTls->getTaintOverflowFlag()->getName();
    else constraint += ((flagsValue >> OVERFLOW_FLAG) & 1) ? "#b1" : "#b0";

    constraint += ") #b1))\n";

    this->_formula << constraint;

    // 3) d�claration de l'assertion en fin de contrainte
    // selon que la branche a �t� prise ou non
    this->declareConstraintFooter(constraintNumber, isTaken);
} // addConstraint_LESS

void SolverFormula::addConstraint_LESS_OR_EQUAL
    (TaintManager_Thread *pTmgrTls, ADDRINT insAddress, bool isTaken, ADDRINT flagsValue) 
{
    // 1) d�finition de l'ent�te de la contrainte
    this->declareConstraintHeader(insAddress, PREDICATE_LESS_OR_EQUAL);
    
    // 2) d�claration de la formule de contrainte (SF xor OF) or ZF) = 1 		
    // d�claration des flags marqu�s, et r�cursivement de leurs sources
    if (pTmgrTls->isSignFlagTainted())     this->declareObject(pTmgrTls->getTaintSignFlag());
    if (pTmgrTls->isOverflowFlagTainted())	this->declareObject(pTmgrTls->getTaintOverflowFlag());
    if (pTmgrTls->isZeroFlagTainted())	    this->declareObject(pTmgrTls->getTaintZeroFlag());

    // declaration de la contrainte : declaration de l'objet
    const std::string constraintNumber(decstr(this->_iAssert));
    std::string constraint("(define-fun C_");
    constraint += constraintNumber;
        
    // formule : ((SF xor OF) or ZF) == 1
    constraint += "() Bool (= (bvor (bvxor ";

    if (pTmgrTls->isSignFlagTainted())
    {
        constraint += pTmgrTls->getTaintSignFlag()->getName();
        constraint += ' ';
    }
    else constraint += ((flagsValue >> SIGN_FLAG) & 1) ? "#b1 " : "#b0 ";
        
    if (pTmgrTls->isOverflowFlagTainted())	
    {
        constraint += pTmgrTls->getTaintOverflowFlag()->getName();
        constraint += ") ";
    }
    else constraint += ((flagsValue >> OVERFLOW_FLAG) & 1) ? "#b1) " : "#b0)";

    if (pTmgrTls->isZeroFlagTainted()) constraint += pTmgrTls->getTaintZeroFlag()->getName();
    else constraint += ((flagsValue >> ZERO_FLAG) & 1) ? "#b1" : "#b0";

    constraint += ") #b1))\n";
    
    this->_formula << constraint;

    // 3) d�claration de l'assertion en fin de contrainte
    // selon que la branche a �t� prise ou non
    this->declareConstraintFooter(constraintNumber, isTaken);
} // addConstraint_LESS_OR_EQUAL
#endif

// envoi des deni�res donn�es : nombre total de contraintes
void SolverFormula::final() 
{
    WINDOWS::DWORD cbWritten = 0;
    std::string finalFormula;
    
#if DEBUG
    // MODE DEBUG : construction d'une formule "prete � l'emploi"
    // avec insertion de la logique, et des commandes "check-sat" et "get-model" 
    std::string formulaHeader =
        ";********************************\n" \
        ";*** FUZZWIN 1.0 - MODE DEBUG ***\n" \
        ";********************************\n" \
        "; Fichier instrument� : " + g_inputFile + "\n\n";
    formulaHeader +=
        "(set-option:produce-models true)\n" \
        "(set-option:print-success false)\n" \
        "(set-logic QF_BV)\n";

    std::string formulaFooter = 
        "(check-sat)\n" \
        "(get-model)\n";

    finalFormula = formulaHeader + this->_formula.str() + formulaFooter;
#else
    // MODE RELEASE : insertion du nombre total de contraintes 
    this->_formula << ";@" << std::dec << this->_iAssert;
    finalFormula = this->_formula.str();
#endif

    // envoi de la formule en entier dans le pipe (ou fichier en mode debug)
    WINDOWS::WriteFile(g_hPipe, 
        finalFormula.c_str(), 
        static_cast<WINDOWS::DWORD>(finalFormula.size()), // pour eviter C4267 en x64 
        &cbWritten, 
        NULL);
    WINDOWS::FlushFileBuffers(g_hPipe);
}
