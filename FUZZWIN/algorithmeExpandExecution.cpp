#include "algorithmeExpandExecution.h"
#include "check.h"
#include "solver.h"

#include <sstream> // stringstream

static UINT32 numberOfChilds = 0;	// num�rotation des fichiers d�riv�s

class CPinCommandsParameters
{
public:
    const char* _filePath;
    DWORD _filePathLength;
    CPinCommandsParameters(const char* f, DWORD l) : _filePath(f), _filePathLength(l) {}
    ~CPinCommandsParameters() {}
};

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
    size_t posBeginOfLine = 0; // position du d�but de ligne de la contrainte 'bound'
    size_t posEndOfLine   = 0; // position du fin de ligne de la contrainte 'bound'

    if (bound) 
    {
        // recherche de la contrainte "bound" dans la formule
        posBeginOfLine	= formula.find("(assert (= C_" + std::to_string((_Longlong) bound));
        // recherche de la fin de la ligne
        posEndOfLine	= formula.find_first_of('\n', posBeginOfLine);
        // extraction des contraintes non invers�es et envoi au solveur
        sendToSolver(formula.substr(0, posEndOfLine + 1));
    }
    return (posEndOfLine); // position de la fin de la derni�re contrainte dans la formule
}

static std::string invertConstraint(std::string constraint) // copie locale de la contrainte originale
{
    size_t pos = constraint.find("true");
    
    if (pos != std::string::npos)    constraint.replace(pos, 4, "false");   // 'true'  -> 'false' 
    else  constraint.replace(constraint.find("false"), 5, "true");          // 'false' -> 'true'

    return (constraint);
}

static int sendArgumentsToPintool(const std::string &command)
{
    // 1) chemin vers l'entr�e �tudi�e
    // 2) nombre maximal de contraintes (TODO)
    // 3) temps limite d'execution en secomdes (TODO)
    // 4) offset des entrees � etudier (TODO)
    
    DWORD commandLength = static_cast<DWORD>(command.size());
    DWORD cbWritten = 0;

    BOOL fSuccess = WriteFile(pGlobals->hPintoolPipe, 
        command.c_str(), 
        commandLength, 
        &cbWritten, 
        NULL);

    // si tout n'a pas �t� �crit ou erreur : le signaler
    if (!fSuccess || cbWritten != commandLength)	
    {   
        std::cout << "erreur envoi arguments au Pintool" << std::endl;
        return 1;   // erreur
    }
    else return 0;  // OK
}


static std::string callFuzzwin(CInput* pInput) 
{
    // ligne de commande pour appel de PIN avec l'entree etudiee
    std::string cmdLine(pInput->getCmdLineFuzzwin()); 
    std::string smtFormula;
  
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    
    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    si.cb = sizeof(si);
    
    if (CreateProcess (nullptr, (LPSTR) cmdLine.c_str(), nullptr, nullptr, TRUE, NULL, nullptr, nullptr, &si, &pi)) 
    {          
        /***********************/
        /** CONNEXION AU PIPE **/
        /***********************/
        BOOL fSuccess;
        fSuccess = ConnectNamedPipe(pGlobals->hPintoolPipe, NULL);
        if (!fSuccess && GetLastError() != ERROR_PIPE_CONNECTED) 
        {	
            std::cout << "erreur de connexion au pipe FuzzWin" << GetLastError() << std::endl;
            return (smtFormula); // formule vide
        }
        
        /************************************/
        /** ENVOI DES ARGUMENTS AU PINTOOL **/
        /************************************/

        int resultOfSend;
        // 1) chemin vers l'entr�e �tudi�e
        resultOfSend = sendArgumentsToPintool(pInput->getFilePath());
        // 2) nombre maximal de contraintes (TODO)
        // 3) temps limite d'execution en secomdes (TODO)
        std::string maxTime(std::to_string(pGlobals->maxExecutionTime));
        resultOfSend = sendArgumentsToPintool(maxTime);
        // 4) offset des entrees � etudier (TODO)
        resultOfSend = sendArgumentsToPintool(pGlobals->bytesToTaint);

        /********************************************************/
        /** ATTENTE DE L'ARRIVEE DES DONNEES DEPUIS LE PINTOOL **/
        /********************************************************/
        char buffer[512]; // buffer de r�cup�ration des donn�es
        DWORD cbBytesRead = 0; 

        // lecture successive de blocs de 512 octets 
        // et construction progressive de la formule
        do 
        { 
            fSuccess = ReadFile( 
                pGlobals->hPintoolPipe,  // pipe handle 
                &buffer[0],    // buffer to receive reply 
                512,            // size of buffer 
                &cbBytesRead,   // number of bytes read 
                NULL);          // not overlapped 
 
            if ( ! fSuccess && GetLastError() != ERROR_MORE_DATA )  break; 
            // ajout des donn�es lues au resultat
            smtFormula.append(&buffer[0], cbBytesRead);

         } while ( ! fSuccess);  // repetition si ERROR_MORE_DATA 

        // deconnexion du pipe
        DisconnectNamedPipe(pGlobals->hPintoolPipe);

        // attente de la fermeture de l'application
        WaitForSingleObject(pi.hProcess, INFINITE);
        
        // recup�ration du code de retour du pinttol (pour �ventuel debug)
        DWORD exitCode = 0;
        GetExitCodeProcess(pi.hProcess, &exitCode);

        // fermeture des handles processus et thread 
        CloseHandle(pi.hProcess); 
        CloseHandle(pi.hThread);

        // si option 'keepfiles' : sauvegarde de la formule (extension .fzw)
        if (pGlobals->keepFiles) 
        {
            std::ofstream ofs(pInput->getLogFile());
            ofs << infoHeader;                  // entete (version pin Z3 etc)
            ofs << "; Fichier instrument� : ";  // fichier d'entree etudi�
            ofs << pInput->getFileName() << std::endl;  
            ofs << smtFormula;                  // formule brute smt2
            ofs.close();
        }
    }	
    else // si erreur de CreateProcess
    {
        VERBOSE(std::cout << "erreur process FuzzWin" << GetLastError() << std::endl;)
    }
    
    return (smtFormula); // retour de la formule SMT2
}

ListOfInputs expandExecution(CInput *pInput, HashTable &h, UINT32 *nbFautes) 
{  
    ListOfInputs result;	                // liste de nouveaux objets de type CInput g�n�r�es
    UINT32	bound = pInput->getBound();     // bound du fichier actuellement �tudi�
    size_t pos = 0, posLastConstraint = 0;  // indexs de position dans une chaine de caract�res
    std::string inputContent;               // contenu du fichier �tudi�
    std::string	constraintJ;	        // partie de formule associ�e � la contrainte J
    std::string constraintJ_inverted;   // la m�me contrainte, invers�e
    std::string formula;            // formule fournie par le pintool lors de l'ex�cution du fichier 'pInput'
    static HashValue   fileHash;    // structure pour calculer le hash d'un fichier

    /********************************************************/
    /** Execution du pintool et r�cup�ration de la formule **/
    /********************************************************/
    formula = callFuzzwin(pInput);
    if (formula.size() == 0) return result; // aucune formule ou erreur

    // r�cup�ration du nombre de contraintes dans la formule
    pos = formula.find_last_of('@');
    if (pos == std::string::npos) return result;
    UINT32 nbContraintes = std::stoi(formula.substr(pos + 1));

    // si le "bound" est sup�rieur aux nombre de contraintes => aucune nouvelle entr�e, retour
    if (bound >= nbContraintes) 	
    {
        std::cout << "\tPas de nouvelles contraintes inversibles" << std::endl;
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
        VERBOSE(std::cout << "\tinversion contrainte " << j);
            
        // recherche de la prochaine contrainte dans la formule, � partir de la position de la pr�c�dente contrainte 
        pos = formula.find("(assert (=", posLastConstraint);          
        // envoi au solveur de toutes les d�clarations avant la contrainte
        sendToSolver(formula.substr(posLastConstraint, (pos - posLastConstraint)));
        // envoi au solveur de la commande "push 1"
        // reserve une place sur la pile pour la prochaine d�claration (la contrainte invers�e)
        sendToSolver("(push 1)\n");

        // recherche de la fin de la ligne de la contrainte actuelle (et future pr�c�dente contrainte)
        posLastConstraint = formula.find_first_of('\n', pos);     
        // extraction de la contrainte, et inversion
        constraintJ = formula.substr(pos, (posLastConstraint - pos));
        constraintJ_inverted = invertConstraint(constraintJ);    

        // envoi de la contrainte J invers�e au solveur, et recherche de la solution
        sendToSolver(constraintJ_inverted + '\n');

        if (checkSatFromSolver())	// SOLUTION TROUVEE : DEMANDER LES RESULTATS
        {							
            std::string newInputContent(inputContent); // copie du contenu du fichier initial
            std::string solutions;
            size_t      newFileHash = 0;
             
            // r�cup�ration des solutions du solveur
            solutions = getModelFromSolver();
                
            // modification des octets concern�s dans le contenu du nouveau fichier
            std::sregex_token_iterator it (solutions.begin(), solutions.end(), pGlobals->regexModel, pGlobals->tokens);
            std::sregex_token_iterator end;
            while (it != end) 
            {
                int offset = std::stoi(it->str(), nullptr, 10); ++it; // 1ere valeur = offset
                int value =  std::stoi(it->str(), nullptr, 16); ++it; // 2eme valeur = valeur
                newInputContent[offset] = static_cast<char>(value);
            }

            // calcul du hash du nouveau contenu et insertion dans la table de hachage. 
            // Si duplicat (retour 'false'), ne pas cr�er le fichier
            auto insertResult = h.insert(fileHash(newInputContent));
            if (insertResult.second != false)
            {
                // fabrication du nouvel objet "fils" � partir du p�re
                CInput *newChild = new CInput(pInput, ++numberOfChilds, j);

                // cr�ation du fichier et insertion du contenu modifi�
                std::ofstream newFile(newChild->getFilePath(), std::ios::binary);
                newFile.write(newInputContent.c_str(), newInputContent.size());
                newFile.close();

                VERBOSE(std::cout << "-> nouveau fichier " << newChild->getFileName() << std::endl);

                // test du fichier de suite; si retour nul le fichier est valide, donc l'ins�rer dans la liste
                DWORD checkError = debugTarget(newChild);
                if (!checkError) result.push_back(newChild);
                else ++*nbFautes;

            }	// fin de la boucle de test sur le hash du contenu
            else 	VERBOSE(std::cout << "-> deja genere\n");
        } // end "hasSolution"
        else VERBOSE(std::cout << " : aucune solution\n");   
       
        // enlever la contrainte invers�e de la pile du solveur, et remettre l'originale
        sendToSolver("(pop 1)\n" + constraintJ);
    } // end for
       
    // effacement de toutes les formules pour laisser une pile vierge pour le prochain fichier
    sendToSolver("(reset)\n");

    return (result);
}
