#include "algorithmeExpandExecution.h"
#include "check.h"
#include "solver.h"
#include "pintoolFuncs.h"

static UINT32 numberOfChilds = 0;	// num�rotation des fichiers d�riv�s

/*********************************/
/**  ALGORITHME EXPANDEXECUTION **/
/*********************************/

// cette fonction g�n�re de nouveaux fichiers d'entr�e � partir d'une entr�e fournie
// 1) ex�cution du premier PINTOOL ("FuzzWin") retournant une formule SMT2 brute
// 2) r�cup�ration du nombre de contraintes dans ce fichier
// 3) pour j dans [bound + 1, nbContraintes] :
//		inverser la contrainte num�ro J et envoyer la formule au solveur
//		si une solution existe :  
//			g�n�ration d'un nouveau fichier avec les octets modifi�s
//			calcul du hash, si d�j� pr�sent ne pas retenir la solution
//			atribution du "bound" = j � ce nouveau fichier
//			ajout du fichier � la liste des fichiers g�n�r�s
//	 (fin de la boucle j)
// retour de la liste des fichiers ainsi g�n�r�s

static size_t sendNonInvertedConstraints(const std::string &formula, UINT32 bound)
{
    if (bound) 
    {
        size_t posBeginOfLine = 0; // position du d�but de ligne de la contrainte 'bound'
        size_t posEndOfLine   = 0; // position du fin de ligne de la contrainte 'bound'
    
        // recherche de la contrainte "bound" dans la formule
        posBeginOfLine	= formula.find("(assert (= C_" + std::to_string((_Longlong) bound));
        // recherche de la fin de la ligne
        posEndOfLine	= formula.find_first_of('\n', posBeginOfLine);
        // extraction des contraintes non invers�es et envoi au solveur
        sendToSolver(formula.substr(0, posEndOfLine + 1));
        return (posEndOfLine); // position de la fin de la derni�re contrainte dans la formule
    }
    else return (0);
}

// renvoie l'inverse de la contrainte fournie en argument
// la contrainte originale (argument fourni) reste inchang�e
static std::string invertConstraint(const std::string &constraint) 
{
    // copie de la contrainte
    std::string invertedConstraint(constraint);
    size_t pos = invertedConstraint.find("true");
    
    if (pos != std::string::npos)  // 'true'  -> 'false' 
    {
        invertedConstraint.replace(pos, 4, "false");   
    }
    else  // 'false' -> 'true'
    {
        invertedConstraint.replace(invertedConstraint.find("false"), 5, "true");  
    }
    return (invertedConstraint);
}

ListOfInputs expandExecution(CInput *pInput, HashTable &h, UINT32 *nbFautes) 
{  
    ListOfInputs result;	                // liste de nouveaux objets de type CInput g�n�r�es
    UINT32	bound = pInput->getBound();     // bound du fichier actuellement �tudi�
    size_t pos = 0, posLastConstraint = 0;  // indexs de position dans une chaine de caract�res
    
    std::string formula;                // formule fournie en sortie par le pintool
    std::string inputContent;           // contenu du fichier �tudi�
    std::string	constraintJ;	        // partie de formule associ�e � la contrainte J
    std::string constraintJ_inverted;   // la m�me contrainte, invers�e

    /********************************************************/
    /** Execution du pintool et r�cup�ration de la formule **/
    /********************************************************/
    formula = callFuzzwin(pInput);
    if (formula.empty())
    {
        VERBOSE("\tAucune formule recue du pintool !!\n");
        return result; // aucune formule ou erreur
    }

    // r�cup�ration du nombre de contraintes dans la formule
    pos = formula.find_last_of('@');
    if (std::string::npos == pos) return result;
    UINT32 nbContraintes = std::stoi(formula.substr(pos + 1));

    // si le "bound" est sup�rieur aux nombre de contraintes => aucune nouvelle entr�e, retour
    if (bound >= nbContraintes) 	
    {
        VERBOSE("\tPas de nouvelles contraintes inversibles\n");
        return result;
    }
    
    /********************************************/
    /** Traitement et r�solution de la formule **/
    /********************************************/

    // les contraintes de 0 � bound ne seront pas invers�es => les envoyer au solveur
    // en retour, un index pointant vers la fin de la derni�re contrainte envoy�e
    posLastConstraint = sendNonInvertedConstraints(formula, bound);
    
    // r�cup�ration du contenu du fichier cible �tudi�
    inputContent = pInput->getFileContent(); 

    // => boucle sur les contraintes de "bound + 1" � "nbContraintes" invers�es et resolues successivement
    for (UINT32 j = bound + 1 ; j <= nbContraintes ; ++j) 
    {	
        VERBOSE("\tinversion contrainte " + std::to_string(j));
            
        // recherche de la prochaine contrainte dans la formule, � partir de la position de la pr�c�dente contrainte 
        pos = formula.find("(assert (=", posLastConstraint);          
        // envoi au solveur de toutes les d�clarations avant la contrainte
        sendToSolver(formula.substr(posLastConstraint, (pos - posLastConstraint)));
        // envoi au solveur de la commande "push 1"
        // reserve une place sur la pile pour la prochaine d�claration (la contrainte invers�e)
        sendToSolver("(push 1)\n");

        // recherche de la fin de la ligne de la contrainte actuelle (et future pr�c�dente contrainte)
        posLastConstraint    = formula.find_first_of('\n', pos);     
        // extraction de la contrainte, et inversion
        constraintJ          = formula.substr(pos, (posLastConstraint - pos));
        constraintJ_inverted = invertConstraint(constraintJ);    

        // envoi de la contrainte J invers�e au solveur, et recherche de la solution
        sendToSolver(constraintJ_inverted + '\n');

        if (checkSatFromSolver())	// SOLUTION TROUVEE : DEMANDER LES RESULTATS
        {							
            std::string newInputContent(inputContent); // copie du contenu du fichier initial
            std::string solutions;

            // r�cup�ration des solutions du solveur
            solutions = getModelFromSolver();
                
            // modification des octets concern�s dans le contenu du nouveau fichier
            int tokens[2] = {1, 2};
            std::sregex_token_iterator it (solutions.begin(), solutions.end(), pGlobals->regexModel, tokens);
            std::sregex_token_iterator end;
            while (it != end) 
            {
                int offset = std::stoi(it->str(), nullptr, 10); ++it; // 1ere valeur = offset
                int value =  std::stoi(it->str(), nullptr, 16); ++it; // 2eme valeur = valeur
                newInputContent[offset] = static_cast<char>(value);
            }

            // calcul du hash du nouveau contenu et insertion dans la table de hachage. 
            // Si duplicat (retour 'false'), ne pas cr�er le fichier
            auto insertResult = h.insert(pGlobals->fileHash(newInputContent));
            if (insertResult.second != false)
            {
                // fabrication du nouvel objet "fils" � partir du p�re
                CInput *newChild = new CInput(pInput, ++numberOfChilds, j);

                // cr�ation du fichier et insertion du contenu modifi�
                std::ofstream newFile(newChild->getFilePath(), std::ios::binary);
                newFile.write(newInputContent.c_str(), newInputContent.size());
                newFile.close();

                VERBOSE("-> nouveau fichier " + newChild->getFileName());

                // test du fichier de suite; si retour nul le fichier est valide, donc l'ins�rer dans la liste
                DWORD checkError = debugTarget(newChild);
                if (!checkError) result.push_back(newChild);
                else ++*nbFautes;
            }	
            // le fichier a d�j� �t� g�n�r� (hash pr�sent ou ... collision)
            else VERBOSE("-> deja g�n�r�\n");
        }
        // pas de solution trouv�e par le solveur
        else VERBOSE(" : aucune solution\n");   
       
        // enlever la contrainte invers�e de la pile du solveur, et remettre l'originale
        sendToSolver("(pop 1)\n" + constraintJ + '\n');
    } // end for
       
    // effacement de toutes les formules pour laisser une pile vierge
    // pour le prochain fichier d'entr�e qui sera test�
    sendToSolver("(reset)\n");

    return (result);
}
