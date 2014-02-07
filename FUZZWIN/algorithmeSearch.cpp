#include "algorithmeSearch.h"
#include "algorithmeExpandExecution.h"
#include "check.h"

/**********************/
/**  FONCTION SEARCH **/
/**********************/

// cette fonction entretient une liste de fichiers d'entr�e et 
// proc�de aux tests de ces entr�es. 
// La fonction retourne le nombre de fichiers qui provoquent une faute
// dans le programme �tudi�

// TODO : trier la liste � chaque fois que de nouveaux fichiers sont g�n�r�s, 
// afin d'ex�cuter en priorit� les fichiers ayant une couverture de code maximale
// n�cessite l'impl�mentation d'une fonction de "scoring"

UINT32 algorithmeSearch() 
{
    ListOfInputs workList;		// liste des fichiers � traiter 
    HashValue    fileHash;      // structure pour calculer le hash d'un fichier
    HashTable	 hashTable;		// table de hachage des fichiers d�j� trait�s, pour �viter les doublons
    CInput*		 pFirstInput;	// objet repr�sentant l'entr�e initiale
    UINT32		 nbFautes = 0;	// nombre de fautes trouv�es au cours de l'analyse
    
    // cr�ation de l'objet repr�sentant l'entr�e initiale
    std::string firstFilePath(pGlobals->resultDir + "\\input0");
    pFirstInput = new CInput(firstFilePath);
 
    // calcul du hash de la premi�re entr�e, et insertion dans la liste des hashes
    hashTable.insert(fileHash(pFirstInput->getFileContent()));

    // initialisation de la liste de travail avec la premi�re entr�e
    workList.push_back(pFirstInput);

    /**********************/
    /** BOUCLE PRINCIPALE */
    /**********************/
    while ( !workList.empty() ) 
    {
        std::cout << "[" << workList.size() << "] ELEMENTS DANS LA WORKLIST\n";
        
        // r�cup�ration et retrait du fichier ayant le meilleur score (dernier de la liste)
        CInput* pCurrentInput = workList.back();
        workList.pop_back();

        std::cout << "[!] execution de " << pCurrentInput->getFileName();
        std::cout << " (bound = " << pCurrentInput->getBound();
        if (pCurrentInput->getFather()) 
        {
            std::cout << " pere = " << pCurrentInput->getFather()->getFileName();
        }
        std::cout << ")\n";

        // ex�cution de PIN avec cette entr�e (fonction ExpandExecution)
        // et recup�ration d'une liste de fichiers d�riv�s
        // la table de hachage permet d'�carter les doublons d�j� g�n�r�s
        auto childInputs = expandExecution(pCurrentInput, hashTable, &nbFautes);

        // insertion des fichiers dans la liste, et diminution du refcount de l'objet venant d'�tre test�
        workList.insert(workList.cbegin(), childInputs.cbegin(), childInputs.cend());
        pCurrentInput->decRefCount();
    }
    return nbFautes;
}
