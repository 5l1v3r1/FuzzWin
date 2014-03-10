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
    HashTable	 hashTable;		// table de hachage des fichiers d�j� trait�s, pour �viter les doublons
    CInput*		 pFirstInput;	// objet repr�sentant l'entr�e initiale
    UINT32		 nbFautes = 0;	// nombre de fautes trouv�es au cours de l'analyse
    
    // cr�ation de l'objet repr�sentant l'entr�e initiale
    std::string firstFilePath(pGlobals->firstInputPath);
    pFirstInput = new CInput(firstFilePath);
 
    // calcul du hash de la premi�re entr�e, et insertion dans la liste des hashes
    hashTable.insert(pGlobals->fileHash(pFirstInput->getFileContent()));

    // initialisation de la liste de travail avec la premi�re entr�e
    workList.push_back(pFirstInput);

    /**********************/
    /** BOUCLE PRINCIPALE */
    /**********************/
    while ( !workList.empty() ) 
    {
        LOG("[" + std::to_string(workList.size()) + "] ELEMENTS DANS LA WORKLIST\n");
        
        // tri des entr�es selon leur score (si option activ�e)
        if (pGlobals->computeScore) workList.sort(sortInputs);

        // r�cup�ration et retrait du fichier ayant le meilleur score (dernier de la liste)
        CInput* pCurrentInput = workList.back();
        workList.pop_back();

        LOG("[!] ex�cution de " + pCurrentInput->getFileName());
        
        VERBOSE(" (bound = " + std::to_string(pCurrentInput->getBound()) + ')');
        if (pGlobals->computeScore)
        {
            VERBOSE(" (score = " + std::to_string(pCurrentInput->getScore()) + ')');
        }
        if (pCurrentInput->getFather()) 
        {
            VERBOSE(" (p�re = " + pCurrentInput->getFather()->getFileName() + ')');
        }

        LOG("\n");

        // ex�cution de PIN avec cette entr�e (fonction ExpandExecution)
        // et recup�ration d'une liste de fichiers d�riv�s
        // la table de hachage permet d'�carter les doublons d�j� g�n�r�s
        auto childInputs = expandExecution(pCurrentInput, hashTable, &nbFautes);

        if (!childInputs.size())  LOG("\t pas de nouveaux fichiers\n")  
        else   LOG("\t " + std::to_string(childInputs.size()) + " nouveau(x) fichier(s)\n");

        // insertion des fichiers dans la liste, et diminution du 
        // refcount de l'objet venant d'�tre test�
        workList.insert(workList.cbegin(), childInputs.cbegin(), childInputs.cend());
        pCurrentInput->decRefCount();
    }
    return (nbFautes);
}
