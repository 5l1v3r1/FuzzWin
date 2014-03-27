#pragma once
#include <iostream>

#include "../FUZZWIN_COMMON/fuzzwin_algo.h"

// cr�ation de la classe algorithme adapt� � la ligne de commande

class FuzzwinAlgorithm_cmdLine : public FuzzwinAlgorithm
{    
private:  
    /**  impl�mentation des m�thodes virtuelles pures **/

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
};
