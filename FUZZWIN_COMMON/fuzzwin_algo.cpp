#include "fuzzwin_algo.h"

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

/*************************************************/
/* CLASSES DE BASE (DERIVEES DANS CMDLINE ET GUI */
/*************************************************/

/*********************************************/
/* CINPUT : description d'une entr�e de test */
/*********************************************/

CInput::CInput(const std::string &firstInputPath, bool keepfiles)
    : _keepFiles(keepfiles),
    _refCount(1),
    _bound(0), 
    _exceptionCode(0), 
    _score(0),
    _filePath(firstInputPath), 
    _pFather(nullptr)    // pas de p�re pour la premi�re entr�e
{
    std::string::size_type pos = std::string(firstInputPath).find_last_of("\\/");
    this->_fileName = firstInputPath.substr(pos + 1); // antislash exclus 
}

// cr�ation du 'nb' nouvel objet d�riv� de l'objet 'pFather' � la contrainte 'b'
CInput::CInput(CInput* pFather, UINT64 nb, UINT32 b) 
    : _keepFiles(pFather->_keepFiles),
    _refCount(1),  // sachant qu'� 0 il est detruit
    _bound(b), 
    _exceptionCode(0),
    _pFather(pFather),
    _score(0),
    _fileName("input" + std::to_string((_Longlong) nb))
{ 	
    // construction du chemin de fichier � partir de celui du p�re
    std::string fatherFilePath = pFather->getFilePath();
    std::string::size_type pos = fatherFilePath.rfind(pFather->getFileName());
    this->_filePath = fatherFilePath.substr(0, pos) + this->_fileName;

    // nouvel enfant : augmentation du refCount du p�re (qui existe forc�ment)
    this->_pFather->incRefCount();
}

CInput::~CInput()
{    
    // effacement du fichier, si l'option 'keepfiles' n'est pas sp�cifi�e
    // ET que l'entr�e n'a pas provoqu�e de fautes
    if (!_keepFiles && !_exceptionCode)  remove(this->_filePath.c_str());
}

// Accesseurs renvoyant les membres priv�s de la classe
CInput* CInput::getFather() const { return _pFather; }
UINT32  CInput::getBound() const  { return _bound; }
UINT32  CInput::getScore() const  { return _score; }
UINT32  CInput::getExceptionCode() const       { return _exceptionCode; }
const std::string& CInput::getFilePath() const { return _filePath; }
const std::string& CInput::getFileName() const { return _fileName; }

// num�ro de contrainte invers�e qui a donn� naissance � cette entr�e
void CInput::setBound(const UINT32 b)	{ _bound = b; }
// Affectation d'un score � cette entr�e
void CInput::setScore(const UINT32 s)	{ _score = s; }
// num�ro d'Exception g�n�r� par cette entr�e
void CInput::setExceptionCode(const UINT32 e)	{ _exceptionCode = e; }

// gestion de la vie des objets "CInput" : refCount basique
void   CInput::incRefCount()       { _refCount++; }
UINT32 CInput::decRefCount()       { return (--_refCount); }
UINT32 CInput::getRefCount() const { return (_refCount); }

// renvoie la taille de l'entr�e en octets
UINT32 CInput::getFileSize() const
{
    std::ifstream in(_filePath.c_str(), std::ifstream::in | std::ifstream::binary);
    in.seekg(0, std::ifstream::end);
    UINT32 length = static_cast<UINT32>(in.tellg()); 
    in.close();
    return (length);
}

// renvoie le chemin vers le fichier qui contiendra la formule SMT2
// associ�e � l'execution de cette entr�e (option --keepfiles mise � TRUE)
std::string CInput::getLogFile() const { return (_filePath + ".fzw"); }

// renvoie le contenu du fichier sous la forme de string
std::string CInput::getFileContent() const
{
    UINT32 fileSize = this->getFileSize(); // UINT32 => fichier < 2Go...
    std::vector<char> contentWithChars(fileSize);

    // ouverture en mode binaire, lecture et retour des donn�es
    std::ifstream is (_filePath.c_str(), std::ifstream::binary);
    is.read(&contentWithChars[0], fileSize);

    return (std::string(contentWithChars.begin(), contentWithChars.end()));    
}

// fonction de tri de la liste d'entr�es de tests
bool sortCInputs(CInput* pA, CInput* pB) 
{ 
    return (pA->getScore() < pB->getScore()); 
}

/*********************/
/* FUZZWIN_ALGORITHM */
/*********************/

FuzzwinAlgorithm::FuzzwinAlgorithm(OSTYPE osType)
    : _osType(osType), 
      _hZ3_process (nullptr), _hZ3_stdin(nullptr),  _hZ3_stdout(nullptr),   _hZ3_thread(nullptr),
      _hReadFromZ3 (nullptr), _hWriteToZ3(nullptr), _hPintoolPipe(nullptr), _hTimer(nullptr),
      _regexModel(parseZ3ResultRegex, std::regex::ECMAScript),
      _nbFautes(0), _numberOfChilds(0) {}

FuzzwinAlgorithm::~FuzzwinAlgorithm()
{
    // fermeture du processus Z3
    CloseHandle(_hZ3_process); CloseHandle(_hZ3_thread);
    // fermeture des diff�rents tubes de communication avec Z3
    CloseHandle(_hZ3_stdout); 	CloseHandle(_hZ3_stdin);
    CloseHandle(_hReadFromZ3);	CloseHandle(_hWriteToZ3);
    // fermeture des tubes nomm�s avec le pintool Fuzzwin
    CloseHandle(_hPintoolPipe);
    // fermeture handle du timer
    if (_maxExecutionTime) CloseHandle(this->_hTimer);
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
    // calcul du hash de la premi�re entr�e, et insertion dans la liste des hashes
    // seulement si l'option est pr�sente
    if (_hashFiles)
    {
        _hashTable.insert(calculateHash(
            _pCurrentInput->getFileContent().c_str(), 
            _pCurrentInput->getFileContent().size()));
    }

    // initialisation de la liste de travail avec la premi�re entr�e
    _workList.push_back(_pCurrentInput);

    /**********************/
    /** BOUCLE PRINCIPALE */
    /**********************/
    while ( !_workList.empty() ) 
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

        this->logVerboseTimeStamp();
        this->logVerbose("    destruction objet " + pInput->getFileName());
        this->logVerboseEndOfLine();
  
        // si un anc�tre existe : diminution de son RefCount (recursivit�)
        if (pFather) this->updateRefCounts(pFather);

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

UINT32 FuzzwinAlgorithm::sendNonInvertedConstraints(UINT32 bound)
{
    if (bound) 
    {
        UINT32 posBeginOfLine = 0; // position du d�but de ligne de la contrainte 'bound'
        UINT32 posEndOfLine   = 0; // position du fin de ligne de la contrainte 'bound'
    
        // recherche de la contrainte "bound" dans la formule
        posBeginOfLine	= _formula.find("(assert (= C_" + std::to_string((_Longlong) bound));
        // recherche de la fin de la ligne
        posEndOfLine	= _formula.find_first_of('\n', posBeginOfLine);
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
    UINT32 pos = invertedConstraint.find("true");
    
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
    UINT32       pos = 0;
    UINT32       posLastConstraint = 0;  // indexs de position dans une chaine de caract�res
    
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
        this->logTimeStamp();
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
                    calculateHash(newInputContent.c_str(), newInputContent.size()));
                
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

                // test du fichier de suite; si retour nul le fichier est valide, donc l'ins�rer dans la liste
                UINT32 checkError = this->debugTarget(newChild);
                if (!checkError) result.push_back(newChild);
                else ++_nbFautes;
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

UINT32 FuzzwinAlgorithm::getNumberOfFaults() const { return _nbFautes; }

void FuzzwinAlgorithm::start() { this->algorithmSearch(); }