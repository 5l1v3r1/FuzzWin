#include "algorithm.h"

OSTYPE getNativeArchitecture()
{
    OSTYPE osType = HOST_UNKNOWN;

    // la fonction GetNativeSystemInfo retourne une structure avec notamment
    // 'wProcessorArchitecture' qui d�termine si OS 32 ou 64bits 
    SYSTEM_INFO si;
    ZeroMemory(&si, sizeof(si));
    GetSystemInfo(&si);

    // GetVersionEx r�cup�re la version de l'OS pour fixer le num�ro des syscalls
    // la structure OSVERSIONINFOEX contient ce que l'on cherche � savoir
    OSVERSIONINFOEX osvi;
    ZeroMemory(&osvi, sizeof(osvi));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    GetVersionEx(reinterpret_cast<OSVERSIONINFO*>(&osvi));
    // version = 10*major + minor ; permet de comparer tout de suite les nombres
    int osVersion = (10 * osvi.dwMajorVersion) + osvi.dwMinorVersion;
    
    // isWow64Process d�termine si fuzzwin fonctionne en �mulation 64bits.
    BOOL bIsWow64 = FALSE;
    IsWow64Process(GetCurrentProcess(), &bIsWow64);

    // Architecture de l'OS = 64 bits, ou bien �mulation (Wow64)
    if ((PROCESSOR_ARCHITECTURE_AMD64 == si.wProcessorArchitecture)	|| bIsWow64)
    {
        // Avant Windows 8 (version 6.2), tous les OS 64bits
        // ont les m�mes tables pour les syscalls
        if (osVersion < 62)         osType = HOST_X64_BEFORE_WIN8;

        // Windows 8 : version 6.2
        else if (62 == osVersion)   osType = HOST_X64_WIN80;

        // pour Windows 8.1, getVersionEx est d�pr�ci� => TODO FAIRE AUTREMENT
        else if (63 == osVersion)   osType = HOST_X64_WIN81;
    }
    else if (PROCESSOR_ARCHITECTURE_INTEL == si.wProcessorArchitecture)	
    {
        switch (osVersion)	
        {
        case 50:  // Version 5.0 = Windows 2000
            osType = HOST_X86_2000;
            break;

        case 51:  // Version 5.1 = Windows XP
            osType = HOST_X86_XP;
            break;

        case 52:  // Version 5.2 = Server 2003. XP 64bits n'est pas consid�r� car on est en 32bits
            osType = HOST_X86_2003;
            break;

        case 60:  // Version 6.0 = Vista ou Server 2008
            if (VER_NT_WORKSTATION == osvi.wProductType) // Vista => tester le cas SP0
            {
                // le syscall 'NtSetInformationFile' diff�re entre SP0 et les autres SP
                // on teste donc si un service pack est pr�sent
                osType = (osvi.wServicePackMajor) ? HOST_X86_VISTA : HOST_X86_VISTA_SP0;
            }
            else osType = HOST_X86_2008;
            break;
       
        case 61:  // Version 6.1 = Seven ou Server 2008 R2
            osType = (VER_NT_WORKSTATION == osvi.wProductType) ? HOST_X86_SEVEN : HOST_X86_2008_R2;
            break;
     
        case 62:  // Version 6.2 = Windows 8 ou Server 2012
            osType = (VER_NT_WORKSTATION == osvi.wProductType) ? HOST_X86_WIN80 : HOST_X86_2012;
            break;

        case 63:  // Version 6.3 = Windows 8.1 ou Server 2012R2 (� voir car GetVersionEx d�pr�ci�e)
            osType = (VER_NT_WORKSTATION == osvi.wProductType) ? HOST_X86_WIN81 : HOST_X86_2012_R2;
            break;
      
        default:  // OS non reconnu donc non support� 
            break; 
        }
    }
    return (osType);
} // getNativeArchitecture

int getKindOfExecutable(const std::string &targetPath)
{
    DWORD kindOfExe = 0;
    BOOL  result = GetBinaryType((LPCSTR) targetPath.c_str(), &kindOfExe);
    
    // erreur si fichier non ex�cutable ou si non trouv�
    if (!result) return (-1);
    else         return (kindOfExe);
} // getKindOfExecutable

std::string getAbsoluteFilePath(const std::string &s)
{
    char absolutepath[MAX_PATH];
    LPSTR* a = nullptr;

    // transformation du chemin relatif en chemin absolu
    DWORD retval = GetFullPathName((LPCSTR) s.c_str(), MAX_PATH, absolutepath, a);

    if (!retval) return (std::string()); // erreur (fichier non pr�sent ?)
    else         return (std::string(absolutepath));
}

/*********************/
/* FUZZWIN_ALGORITHM */
/*********************/

FuzzwinAlgorithm::FuzzwinAlgorithm(OSTYPE osType)
    : _osType(osType), _status(ALGORITHM_CREATED),
      _hZ3_process (nullptr), _hZ3_stdin(nullptr),  _hZ3_stdout(nullptr),   _hZ3_thread(nullptr),
      _hReadFromZ3 (nullptr), _hWriteToZ3(nullptr), _hPintoolPipe(nullptr), _hTimer(nullptr),
      _regexModel(parseZ3ResultRegex, std::regex::ECMAScript),
      _nbFautes(0), _numberOfChilds(0) {}

FuzzwinAlgorithm::~FuzzwinAlgorithm()
{
    // fermeture du processus Z3
    CloseHandle(_hZ3_process); CloseHandle(_hZ3_thread);
    // fermeture des diff�rents tubes de communication avec Z3
    CloseHandle(_hZ3_stdout);  CloseHandle(_hZ3_stdin);
    CloseHandle(_hReadFromZ3); CloseHandle(_hWriteToZ3);
    // fermeture des tubes nomm�s avec le pintool Fuzzwin
    CloseHandle(_hPintoolPipe);
    // fermeture handle du timer
    if (_maxExecutionTime) CloseHandle(this->_hTimer);
}

int FuzzwinAlgorithm::finalizeInitialization(
    const std::string &pin_X86,     const std::string &pin_X64,
    const std::string &pintool_X86, const std::string &pintool_X64, 
    const std::string &cmdLineOptions)
{
    /***************************************/
    /** Ligne de commande pour le pintool **/ 
    /***************************************/

    // si OS 32 bits, pas la peine de sp�cifier les modules 64bits
    if (_osType < BEGIN_HOST_64BITS) 
    {
        /* $(pin_X86) -t $(pintool_X86) -- $(targetFile) %INPUT% (ajout� a chaque fichier test�) */
        _cmdLinePin = "\"" + pin_X86 + "\" " \
             + "-follow_execv " \
             + "-t \"" + pintool_X86  + "\" " \
             + cmdLineOptions \
             + " -- \"" + _targetPath + "\" ";
    }
    else
    {
        /* $(pin_X86) -p64 $(pin_X64) -t64 $(pintool_X64) -t $(pintool_X86) 
        -- $(targetFile) %INPUT% (ajout� a chaque fichier test�) */
        _cmdLinePin = "\"" + pin_X86 + "\" " \
            + "-follow_execv " \
            + "-p64 \"" + pin_X64     + "\" " \
            + "-t64 \"" + pintool_X64 + "\" " \
            + "-t \""   + pintool_X86 + "\" " \
            + cmdLineOptions \
            + " -- \""   + _targetPath + "\" ";
    }
    
    /********************************************************/
    /** construction liste de travail et fichier de fautes **/
    /********************************************************/
    // recopie de l'entr�e initiale dans le dossier de r�sultat (sans extension, avec le nom 'input0')
    std::string input0Path(_resultsDir + "\\input0");
    // copie de l'entr�e initiale dans le dossier de r�sultat
    if (!CopyFile(_firstInputPath.c_str(), input0Path.c_str(), false))
    {
        this->log("erreur de recopie du fichier initial");
        this->logEndOfLine();
        return (EXIT_FAILURE);
    }
    
    _pCurrentInput = new CInput(_resultsDir + "\\input0", _keepFiles); 
    // calcul du hash de la premi�re entr�e, et insertion dans la liste des hashes
    // seulement si l'option est pr�sente
    if (_hashFiles)
    {
        _hashTable.insert(calculateHash(
            _pCurrentInput->getFileContent().c_str(), 
            (int) _pCurrentInput->getFileContent().size()));
    }

    // initialisation de la liste de travail avec la premi�re entr�e
    _workList.push_back(_pCurrentInput);

    // chemin pr�rempli pour le fichier de fautes (non cr�� pour l'instant)
    _faultFile = _resultsDir + "\\fault.txt";
    
    /********************************************/
    /** cr�ation du tube nomm� avec le Pintool **/
    /********************************************/
    this->logVerbose("Tube nomm� avec pintool    : ");
    if (this->createNamedPipePintool())
    {
        this->logVerbose("OK");
        this->logVerboseEndOfLine();
    }
    else 
    {
        this->logVerbose("ERREUR");
        return (EXIT_FAILURE);
    }

    /**********************************************************/
    /** cr�ation du process Z3 avec redirection stdin/stdout **/ 
    /**********************************************************/
    this->logVerbose("Processus du solveur       : ");
    if (this->createSolverProcess(_z3Path))
    {
        this->logVerbose("OK");
        this->logVerboseEndOfLine();
    }
    else 
    {
        this->logVerbose("ERREUR");
        return (EXIT_FAILURE);
    }
  
    /*****************************************/
    /*** CREATION DU TIMER (SI NECESSAIRE) ***/
    /*****************************************/
    if (_maxExecutionTime)
    {
        this->logVerbose("Cr�ation du timer   : ");
        _hTimer = CreateWaitableTimer(NULL /* attricuts par d�faut */, TRUE /* timer manuel */, NULL /* anonyme */);
        if (NULL == _hTimer) 
        {
            this->logVerbose("ERREUR");
            return (EXIT_FAILURE);
        }
        else
        {
            this->logVerbose("OK");
            this->logVerboseEndOfLine();
        }
    }
    return (EXIT_SUCCESS);
}

void FuzzwinAlgorithm::run()
{
    _status = ALGORITHM_RUNNING;
    this->algorithmSearch(); 
    
    // retour de la proc�dure, traitement selon le status
    switch (_status)
    {
    case ALGORITHM_STOPPED: // arret volontaire par utilisateur
        this->logTimeStamp();
        this->log("Arret par l'utilisateur");
        this->logEndOfLine();
        /*** PAS DE BREAK : POURSUITE SUR LA PROCEDURE DE FIN NORMALE **/

    case ALGORITHM_RUNNING: // fin "normale"
        this->logEndOfLine();
        this->log("******************************");
        this->logEndOfLine();
        this->log("* ---> FIN DE L'ANALYSE <--- *");
        this->logEndOfLine();
        this->log("******************************");
        this->logEndOfLine();

        if (_nbFautes) // si une faute a �t� trouv�e
        { 
            this->log("---> " + std::to_string(_nbFautes) + " faute(s)");
            this->log(" cf. fichier 'fault.txt' <---");
            this->logEndOfLine();
        }
        else
        {
            this->log("  aucune faute...");
            this->logEndOfLine();
        }

        // appel de la fin sp�cifique (GUI ou cmdline)
        this->algorithmFinished();

        break;

    case ALGORITHM_PAUSED:
        // ne rien faire si ce n'est confirmer � l'utilisateur
        this->logTimeStamp();
        this->log("Pause par l'utilisateur");
        this->logEndOfLine();

        this->notifyAlgoIsPaused();
        break;

    case ALGORITHM_TRACEONLY_FINISHED:
        // ne rien faire si ce n'est confirmer � l'utilisateur
        this->logTimeStamp();
        this->log("Trace disponible dans " + _resultsDir + "\\input0.smt2");
        this->logEndOfLine();

        this->algorithmTraceOnlyFinished();
        break;
    }
}

void FuzzwinAlgorithm::pause()
{
    _algoMutex.lock();
    _status = ALGORITHM_PAUSED;
    _algoMutex.unlock();
}

void FuzzwinAlgorithm::stop()
{
    _algoMutex.lock();
    _status = ALGORITHM_STOPPED;
    _algoMutex.unlock();
}

/************************/
/**  ALGORITHME SEARCH **/
/************************/

// cette fonction entretient une liste de fichiers d'entr�e et 
// proc�de aux tests de ces entr�es. 
// La fonction retourne le nombre de fichiers qui provoquent une faute
// dans le programme �tudi�

// TODO : trier la liste � chaque fois que de nouveaux fichiers sont g�n�r�s, 
// afin d'ex�cuter en priorit� les fichiers ayant une couverture de code maximale
// n�cessite l'impl�mentation d'une fonction de "scoring"

void FuzzwinAlgorithm::algorithmSearch() 
{
    /**********************/
    /** BOUCLE PRINCIPALE */
    /**********************/
    // boucle tant qu'il y a des fichiers, ou qu'un arret (pause ou stop) a �t� demand�.
    while ( !_workList.empty() && (ALGORITHM_RUNNING == _status)) 
    {
        this->logTimeStamp();
        this->log(std::to_string(_workList.size()) + " �l�ment(s) dans la liste de travail");
        this->logEndOfLine();

        // tri des entr�es selon leur score (si option activ�e)
        if (_computeScore) _workList.sort(sortCInputs);

        // r�cup�ration et retrait du fichier ayant le meilleur score (dernier de la liste)
        _pCurrentInput = _workList.back();
        _workList.pop_back();

        this->logTimeStamp();
        this->log("  ex�cution de " + _pCurrentInput->getFileName());
        
        this->logVerbose(" (bound = " + std::to_string(_pCurrentInput->getBound()) + ')');
        if (_computeScore)
        {
            this->logVerbose(" (score = " + std::to_string(_pCurrentInput->getScore()) + ')');
        }
        if (_pCurrentInput->getFather()) 
        {
            this->logVerbose(" (p�re = " + _pCurrentInput->getFather()->getFileName() + ')');
        }

        this->logEndOfLine();

        // ex�cution de PIN avec cette entr�e (fonction ExpandExecution)
        // et recup�ration d'une liste de fichiers d�riv�s
        // la table de hachage permet d'�carter les doublons d�j� g�n�r�s
        ListOfInputs childInputs = expandExecution();

        // si mode "traceonly : sortir de la boucle de suite
        if  (_traceOnly) 
        {
            _status = ALGORITHM_TRACEONLY_FINISHED;
            break;
        }

        this->logTimeStamp();

        if (!childInputs.size())  this->log("  pas de nouveaux fichiers");  
        else  this->log("  " + std::to_string(childInputs.size()) + " nouveau(x) fichier(s)");

        this->logEndOfLine();

        // insertion des fichiers dans la liste, et diminution du 
        // refcount de l'objet venant d'�tre test�
        _workList.insert(_workList.cbegin(), childInputs.cbegin(), childInputs.cend());
        // mise � jour des refcount des anc�tres, avec destruiction si n�cessaire
        this->updateRefCounts(_pCurrentInput);
    }
    // la valeur de '_status' d�terminera la suite � donner (simple pause ou fin)
}

void FuzzwinAlgorithm::updateRefCounts(CInput *pInput) const
{
    // fonction r�cursive de diminution des refcount
    // cela pourrait �tre g�r� directement dans le destructeur de la classe CInput
    // mais il serait impossible de logger les detructions d'objets

    // si refcount de l'objet � 0, diminuer celui du p�re avant destruction
    if (pInput->decRefCount() == 0)
    {
        CInput *pFather = pInput->getFather();

        // si un anc�tre existe : diminution de son RefCount (recursivit�)
        if (pFather) this->updateRefCounts(pFather);

        this->logVerboseTimeStamp();
        this->logVerbose("    destruction objet " + pInput->getFileName());
        this->logVerboseEndOfLine();

        // destruction de l'objet actuel
        delete (pInput);
    }
}

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

size_t FuzzwinAlgorithm::sendNonInvertedConstraints(UINT32 bound)
{
    if (bound) 
    {
        size_t posBeginOfLine = 0; // position du d�but de ligne de la contrainte 'bound'
        size_t posEndOfLine   = 0; // position du fin de ligne de la contrainte 'bound'
    
        // recherche de la contrainte "bound" dans la formule
        posBeginOfLine	= (UINT32) _formula.find("(assert (= C_" + std::to_string((_Longlong) bound));
        // recherche de la fin de la ligne
        posEndOfLine	= (UINT32) _formula.find_first_of('\n', posBeginOfLine);
        // extraction des contraintes non invers�es et envoi au solveur
        sendToSolver(_formula.substr(0, posEndOfLine + 1));
        return (posEndOfLine); // position de la fin de la derni�re contrainte dans la formule
    }
    else return (0);
}

// renvoie l'inverse de la contrainte fournie en argument
// la contrainte originale (argument fourni) reste inchang�e
std::string FuzzwinAlgorithm::invertConstraint(const std::string &constraint) 
{
    // copie de la contrainte
    std::string invertedConstraint(constraint);
    size_t pos = invertedConstraint.find("true"); // 
    
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

ListOfInputs FuzzwinAlgorithm::expandExecution() 
{  
    ListOfInputs result;	                          // liste de nouveaux objets de type CInput g�n�r�es
    UINT32	     bound = _pCurrentInput->getBound();  // bound du fichier actuellement �tudi�
    size_t       pos = 0;
    size_t       posLastConstraint = 0;  // indexs de position dans une chaine de caract�res
    
    std::string  inputContent;         // contenu du fichier �tudi�
    std::string	 constraintJ;	       // partie de formule associ�e � la contrainte J
    std::string  constraintJ_inverted; // la m�me contrainte, invers�e

    /********************************************************/
    /** Execution du pintool et r�cup�ration de la formule **/
    /********************************************************/
    this->callPintool();

    if (_formula.empty())
    {
        this->logTimeStamp();
        this->log("  !! Aucune formule recue du pintool !!");
        this->logEndOfLine();
        return (result); // aucune formule ou erreur
    }

    // mode traceonly : on en reste l� (result est ici vide)
    if (_traceOnly) return (result);

    // r�cup�ration du nombre de contraintes dans la formule
    pos = _formula.find_last_of('@');
    if (std::string::npos == pos) return result;
    UINT32 nbContraintes = std::stoi(_formula.substr(pos + 1));

    this->logTimeStamp();
    this->log("  " + _formula.substr(pos + 1) + " contraintes extraites");
    this->logEndOfLine();

    // si le "bound" est sup�rieur aux nombre de contraintes => aucune nouvelle entr�e, retour
    if (bound >= nbContraintes) 	
    {
        this->logVerboseTimeStamp();
        this->logVerbose("    Pas de nouvelles contraintes inversibles");
        this->logVerboseEndOfLine();
        return (result);
    }
    
    /********************************************/
    /** Traitement et r�solution de la formule **/
    /********************************************/

    // les contraintes de 0 � bound ne seront pas invers�es => les envoyer au solveur
    // en retour, un index pointant vers la fin de la derni�re contrainte envoy�e
    posLastConstraint = sendNonInvertedConstraints(bound);
    
    // r�cup�ration du contenu du fichier cible �tudi�
    inputContent = _pCurrentInput->getFileContent(); 

    // => boucle sur les contraintes de "bound + 1" � "nbContraintes" invers�es et resolues successivement
    for (UINT32 j = bound + 1 ; j <= nbContraintes ; ++j) 
    {	
        this->logVerboseTimeStamp();
        this->logVerbose("    inversion contrainte " + std::to_string(j));
            
        // recherche de la prochaine contrainte dans la formule, � partir de la position de la pr�c�dente contrainte 
        pos = _formula.find("(assert (=", posLastConstraint);          
        // envoi au solveur de toutes les d�clarations avant la contrainte
        this->sendToSolver(_formula.substr(posLastConstraint, (pos - posLastConstraint)));
        // envoi au solveur de la commande "push 1"
        // reserve une place sur la pile pour la prochaine d�claration (la contrainte invers�e)
        this->sendToSolver("(push 1)\n");

        // recherche de la fin de la ligne de la contrainte actuelle (et future pr�c�dente contrainte)
        posLastConstraint    = _formula.find_first_of('\n', pos);     
        // extraction de la contrainte, et inversion
        constraintJ          = _formula.substr(pos, (posLastConstraint - pos));
        constraintJ_inverted = invertConstraint(constraintJ);    

        // envoi de la contrainte J invers�e au solveur, et recherche de la solution
        this->sendToSolver(constraintJ_inverted + '\n');

        if (this->checkSatFromSolver())	// SOLUTION TROUVEE : DEMANDER LES RESULTATS
        {							
            std::string newInputContent(inputContent); // copie du contenu du fichier initial
            std::string solutions;

            // r�cup�ration des solutions du solveur
            solutions = getModelFromSolver();
                
            // modification des octets concern�s dans le contenu du nouveau fichier
            int tokens[2] = {1, 2};
            std::sregex_token_iterator it (solutions.begin(), solutions.end(), _regexModel, tokens);
            std::sregex_token_iterator end;
            while (it != end) 
            {
                int offset = std::stoi(it->str(), nullptr, 10); ++it; // 1ere valeur = offset
                int value =  std::stoi(it->str(), nullptr, 16); ++it; // 2eme valeur = valeur
                newInputContent[offset] = static_cast<char>(value);
            }

            // calcul du hash du nouveau contenu et insertion dans la table de hachage. 
            // seulement si option pr�sente
            // Si duplicat (retour 'false'), ne pas cr�er le fichier
            
            bool createNewInput = true;  // par d�faut on cr�� la nouvelle entr�e

            if (_hashFiles)
            {
                auto insertResult = _hashTable.insert(
                    calculateHash(newInputContent.c_str(), (int) newInputContent.size()));
                
                if (insertResult.second == false) // le hash est pr�sent : le fichier existe d�j� !!
                {
                    this->logVerbose(" -> deja g�n�r� !!");
                    this->logVerboseEndOfLine();
                    createNewInput = false;
                }
            }

            if (createNewInput)
            {
                // fabrication du nouvel objet "fils" � partir du p�re
                CInput *newChild = new CInput(_pCurrentInput, ++_numberOfChilds, j);

                // cr�ation du fichier et insertion du contenu modifi�
                std::ofstream newFile(newChild->getFilePath(), std::ios::binary);
                newFile.write(newInputContent.c_str(), newInputContent.size());
                newFile.close();

                this->logVerbose(" -> nouveau fichier " + newChild->getFileName());

                // test du fichier de suite
                // si retour nul le fichier est valide, donc l'ins�rer dans la liste
                // sinon ne SURTOUT PAS l'ins�rer dans la liste : il va faire planter le pintool
                if (!this->debugTarget(newChild)) result.push_back(newChild);
            }	     
        }
        // pas de solution trouv�e par le solveur
        else 
        {
            this->logVerbose(" : aucune solution");
            this->logVerboseEndOfLine();
        }
       
        // enlever la contrainte invers�e de la pile du solveur, et remettre l'originale
        this->sendToSolver("(pop 1)\n" + constraintJ + '\n');
    } // end for
       
    // effacement de toutes les formules pour laisser une pile vierge
    // pour le prochain fichier d'entr�e qui sera test�
    this->sendToSolver("(reset)\n");

    return (result);
}
