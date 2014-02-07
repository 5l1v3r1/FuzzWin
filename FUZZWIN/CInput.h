// fichier inclus par SAGE.H

class CInput 
{
private:
    UINT32      _refCount;
    UINT32		_bound;	     // num�ro de contrainte invers�e qui a men� � la cr�ation du fichier
    UINT32	    _exceptionCode; // code d'erreur engendr� par le fichier. 0 si aucun
    std::string _filePath;
    std::string _fileName;      // sans le chemin d'acc�s
    CInput* 	_pFather;

public:
    // cr�ation du 'nb' nouvel objet d�riv� de l'objet 'pFather' � la contrainte 'b'
    CInput(CInput* pFather, UINT64 nb, UINT32 b) 
      : _refCount(1),  // sachant qu'� 0 il est detruit
        _bound(b), 
        _exceptionCode(0),
        _pFather(pFather), 
        _fileName("input" + std::to_string((_Longlong) nb))
    { 	
        if (pFather) // dans le cas contraire probl�me...
        {
            // construction du chemin de fichier � partir de celui du p�re
            std::string fatherFilePath = pFather->getFilePath();
            std::string::size_type pos = fatherFilePath.rfind(pFather->getFileName());
            this->_filePath = fatherFilePath.substr(0, pos) + this->_fileName;

            // nouvel enfant : augmentation du refCount du p�re, si non nul
            this->_pFather->incRefCount();
        }
        // else throw("erreur");
    }

    // constructeur sp�cifique 1ere entr�e
    CInput(const std::string &firstInputPath) 
     : _refCount(1), // sachant qu'� 0 il est detruit
       _bound(0), 
       _exceptionCode(0), 
       _pFather(nullptr),
       _filePath(firstInputPath) 
    {
        std::string::size_type pos = std::string(firstInputPath).find_last_of("\\/");
        this->_fileName = firstInputPath.substr(pos + 1); // antislash exclus 
    }

    ~CInput() 
    {   
        // diminution du refcount du P�re
        if (this->_pFather) this->_pFather->decRefCount() ;

        // effacement du fichier du disque si le fichier n'a pas provoqu� de fautes
        if (!_exceptionCode && !pGlobals->keepFiles) remove(this->_filePath.c_str());
        VERBOSE(std::cout << "destruction fichier " << this->_fileName << std::endl;)
    }

    // Accesseurs renvoyant les membres priv�s de la classe
    UINT32	getBound() const         { return this->_bound; }
    UINT32	getExceptionCode() const { return this->_exceptionCode; }
    std::string& getFilePath() 	{ return this->_filePath; }
    std::string& getFileName()  { return this->_fileName; }
    CInput*	getFather()	const   { return this->_pFather; }

    // num�ro de contrainte invers�e qui a donn� naissance � cette entr�e
    void setBound(const UINT32 b)	{ this->_bound = b; }
    // num�ro d'Exception g�n�r� par cette entr�e
    void setExceptionCode(const UINT32 e)	{ this->_exceptionCode = e; }

    // gestion de la vie des objets "CInput" : refCount basique
    void incRefCount()  { this->_refCount++; }
    void decRefCount()  { if (--this->_refCount == 0) delete (this); }

    // renvoie la taille de l'entr�e en octets
    UINT32 getFileSize() const
    {
        std::ifstream in(this->_filePath.c_str(), std::ifstream::in | std::ifstream::binary);
        in.seekg(0, std::ifstream::end);
        std::ifstream::pos_type length = in.tellg(); 
        in.close();
        return static_cast<UINT32>(length);
    }

    // renvoie le chemin vers le fichier qui contiendra la formule SMT2
    // associ�e � l'execution de cette entr�e (option --keepfiles mise � TRUE)
    std::string getLogFile()
    {
        return (_filePath + ".fzw");
    }

    // renvoie le contenu du fichier sous la forme de string
    std::string getFileContent() const
    {
        std::string result;

        UINT32 fileSize = this->getFileSize(); // UINT32 => fichier < 2Go...
        std::vector<char> contentWithChars(fileSize);

        // ouverture en mode binaire
        std::ifstream is (this->_filePath.c_str(), std::ifstream::binary);

        // lecture des donn�es
        is.read(&contentWithChars[0], fileSize);

        return (std::string(contentWithChars.begin(), contentWithChars.end()));    
    }

    std::string getCmdLineFuzzwin() 
    {
        // 1ere partie : chemin vers pintool 32 et 64bits auquel on ajoute l'entr�e test�e
        return (pGlobals->cmdLinePin + '"' + this->_filePath + '"' ); 
    }

    std::string getCmdLineDebug() 
    {
        return ("\"" + pGlobals->targetFile + "\" \"" + this->_filePath + "\""); 
    }
};