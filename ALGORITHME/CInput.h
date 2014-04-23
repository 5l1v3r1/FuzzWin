#pragma once
#include <list>
#include <vector>
#include <string>
#include <fstream>

typedef unsigned char         UINT8;
typedef unsigned int          UINT32;
typedef unsigned long long    UINT64;

class   CInput;
typedef std::list<CInput*>    ListOfInputs;

class CInput
{
private:
    UINT32       _refCount;      // impl�mentation "basique" d'un sharedPtr
    UINT32       _bound;         // num�ro de contrainte invers�e qui a men� � la cr�ation du fichier
    UINT32       _exceptionCode; // code d'erreur engendr� par le fichier. 0 si aucun
    UINT32       _score;         // score de l'entr�e (couverture de code)
    std::string  _filePath;      // chemin vers le fichier
    std::string  _fileName;      // nom de fichier uniquement (sans le chemin)
    CInput*      _pFather;       // entr�e de laquelle cette entr�e est d�riv�e
    
public:
    // constructeur sp�cifique 1ere entr�e
    CInput(const std::string &firstInputPath, bool keepfiles);
    // cr�ation du 'nb' nouvel objet d�riv� de l'objet 'pFather' � la contrainte 'b'
    CInput(CInput* pFather, UINT64 nb, UINT32 b); 

    // destructeur n'agit pas sur les ressources mais sur l'effacement du fichier
    // donc pas besoin d'appliquer la "rule of three"
    ~CInput();  
    const bool   _keepFiles;  // si VRAI, ne pas effacer physiquement le fichier � la destruction 

    CInput* getFather() const;

    // Accesseurs renvoyant les membres priv�s de la classe
    UINT32 getBound() const;
    UINT32 getScore() const;
    UINT32 getExceptionCode() const;
    std::string getFilePath() const;
    std::string getFileName() const;

    // num�ro de contrainte invers�e qui a donn� naissance � cette entr�e
    void setBound(const UINT32 b);
    // Affectation d'un score � cette entr�e
    void setScore(const UINT32 s);
    // num�ro d'Exception g�n�r� par cette entr�e
    void setExceptionCode(const UINT32 e);

    // gestion de la vie des objets "CInput" : refCount basique
    void   incRefCount();
    UINT32 decRefCount();
    UINT32 getRefCount() const;

    // renvoie la taille de l'entr�e en octets
    UINT32 getFileSize() const;

    // renvoie le chemin vers le fichier qui contiendra la formule SMT2
    // associ�e � l'execution de cette entr�e (option --keepfiles mise � TRUE)
    std::string getLogFile() const;

    // renvoie le contenu du fichier sous la forme de string
    std::string getFileContent() const;
}; // class CInput

// fonction de tri de la liste d'entr�es de tests
bool sortCInputs(CInput* pA, CInput* pB);