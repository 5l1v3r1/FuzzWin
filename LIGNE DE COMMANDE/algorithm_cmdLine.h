#pragma once
#include <iostream>
#include <ctime> // statistiques de temps

#include "../ALGORITHME/algorithm.h"

// cr�ation de la classe algorithme adapt� � la ligne de commande

class FuzzwinAlgorithm_cmdLine : public FuzzwinAlgorithm
{    
private:  
    clock_t _timeBegin, _timeEnd;
    
    /**  impl�mentation des m�thodes virtuelles pures de log **/
    
    void log(const std::string &msg) const;        // envoi en console
    void logVerbose(const std::string &msg) const; // idem en mode verbose
    void logVerboseEndOfLine() const;
    void logEndOfLine() const;
    void logTimeStamp() const;
    void logVerboseTimeStamp() const;

    // renvoie le r�pertoire ou se trouve l'executable
    std::string getExePath() const;
    // test de l'existence d'un fichier
    bool testFileExists(const std::string &filePath) const;
    // Effacement du contenu du r�pertoire de r�sultats sans l'effacer lui meme
    int deleteDirectory(const std::string &refcstrRootDirectory) const;

public:
    explicit FuzzwinAlgorithm_cmdLine(OSTYPE osType);

    // initialisation des variables de la classe
    std::string initialize(int argc, char** argv);

    // impl�mentation des m�thodes virtuelles pures de contr�le de l'algorithme
    void algorithmFinished();
    void algorithmTraceOnlyFinished() {} // non impl�ment� pour l'instant
    void notifyAlgoIsPaused()         {} // non impl�ment� pour l'instant
    void faultFound()           { ++_nbFautes; }
};
