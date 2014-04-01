#include "algorithm_cmdLine.h"

#include <clocale>  // pour passage de la ligne de commande en francais

int main(int argc, char *argv[]) 
{
    int returnCode = EXIT_FAILURE;  // pessimisme

    // passage en Francais !!!!
    std::locale locFrench("French_France.1252");
    std::locale::global(locFrench);

    // test de la compatibilit� de l'OS
    OSTYPE osType = getNativeArchitecture();
    if (HOST_UNKNOWN == osType)
    {
        std::cout << "OS non support� : abandon\n";
        exit(EXIT_FAILURE);
    }
    
    FuzzwinAlgorithm_cmdLine *algo = new FuzzwinAlgorithm_cmdLine(osType);
    if (!algo) return (EXIT_FAILURE);
    
    // initialisation des variables � partir de la ligne de commande
    // vaut "OK" si tout s'est bien pass�, ou le message d'erreur sinon
    std::string initResult = algo->initialize(argc, argv);

    if ("OK" == initResult)   
    {
        // lancement de l'algorithme
        algo->run();
        
        std::cout << "\nAppuyer sur une touche pour quitter";
        fflush(stdin);
        getchar();

        returnCode = EXIT_SUCCESS;
    }
    else std::cout << initResult << " --> ABANDON !!!" << std::endl;
    
    delete (algo);
    return (returnCode);
}
