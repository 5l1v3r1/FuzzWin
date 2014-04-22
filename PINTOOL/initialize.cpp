#include <Syscalls\syscalls.h>
#include <TaintEngine\TaintManager.h>
#include <Translate\translate.h>
#include <Instrumentation\instrumentation.h>

/* ===================================================================== */
// Procedures d'initialisation
/* ===================================================================== */

// thread interne au pintool pour gerer la dur�e de l'instrumentation
static void maxTimeThread(void *nothing)
{
    // attente du temps sp�cifi�
    PIN_Sleep(static_cast<UINT32>(1000 * g_maxTime));

    // Si le pintool n'a pas d�j� termin�, le faire (code 2 = TIMEOUT)
    PIN_ExitApplication(EXIT_TIMEOUT); 
} // controlThread

// ouverture du pipe de communication avec le programme FuzzWin 
static int openPipe()
{
    g_hPipe = WINDOWS::CreateFile(
        "\\\\.\\pipe\\fuzzwin", // pipe name 
        GENERIC_WRITE | GENERIC_READ, // acces en lecture/ecriture
        0,             // pas de partage 
        nullptr,       // attributs de s�curit� par d�faut
        OPEN_EXISTING, // pipe existant
        0,             // attributs par d�faut
        nullptr);	   // pas de mod�le

    if ((HANDLE)(WINDOWS::LONG_PTR) (-1) == g_hPipe)  return (EXIT_FAILURE);
    
    // Changement du mode en message
    DWORD dwMode = PIPE_READMODE_MESSAGE; 
    WINDOWS::BOOL fSuccess = WINDOWS::SetNamedPipeHandleState( 
        g_hPipe,  // handle � modifier 
        &dwMode,  // nouveau mode 
        nullptr, nullptr); // pas de maximum de longueur ni de temps 
    
    if (fSuccess)  return (EXIT_SUCCESS);
    else return (EXIT_FAILURE);
} // openPipe

static std::string readFromPipe()
{   
    // pointeur vers buffer de r�cup�ration des donn�es
    // 1024 octets sont LARGEMENT suffisants pour chaque commande recue
    // A modifier au cas ou on aurait besoin de plus.... mais improbable   
    char buffer[1024] = {0}; 
    DWORD nbRead = 0; 
    
    // lecture de maxi 1024 octets. Le cas ERROR_MORE_DATA n'est pas test�
    // car il supposerait un message > 1024 octets
    WINDOWS::BOOL fSuccess = WINDOWS::ReadFile( 
        g_hPipe,    // handle du pipe
        &buffer[0], // adresse du buffer
        1024,       // taille � lire 
        &nbRead,    // nombre d'octets lus 
        nullptr);   // pas d'overlap
 
    // retour du message lu uniquement en cas de r�ussite de ReadFile
    if (fSuccess) return (std::string(&buffer[0], nbRead));    
    else return ("");
} // readFromPipe

int pintoolInit()
{ 
    int returnValue = INIT_ERROR;

    // option du pintool('taint' ou 'checkscore')    
    g_option = KOption.Value();
    
    /**** OPTION TAINT ***/
    if ("taint" == g_option)
    {
        returnValue = OPTION_TAINT;

        // instanciation des classes globales : marquage m�moire et formule SMT2
        pTmgrGlobal   = new TaintManager_Global;
        g_pFormula    = new SolverFormula;
        if (!pTmgrGlobal || !g_pFormula)  return (INIT_ERROR);

        /*** RECUPERATION DES ARGUMENTS DE LA LIGNE DE COMMANDE ***/
        // 1) si pipe => ouverture, sinon r�cup�ration de l'option 'input'
        g_nopipe = KNoPipe.Value();
        if (!g_nopipe)
        {
            // erreur si pipe ne peut pas �tre ouvert
            if (EXIT_FAILURE == openPipe()) return (INIT_ERROR);
            // r�cup�ration de l'entr�e �tudi�e via le pipe
            g_inputFile = readFromPipe();
            if (g_inputFile.empty()) return (INIT_ERROR);
        }
        else
        {
            // si pas de pipe, nom de l'entr�e pass�e par ligne de commande
            g_inputFile = KInputFile.Value();    
            // l'�criture des resultats (formule SMT2) se fera dans un fichier
            // donc r�cup�rer le handle du fichier qui sera utilis�
            std::string formulaFile(g_inputFile + "_formula.smt2");
            g_hPipe = WINDOWS::CreateFile(
                formulaFile.c_str(), // nom du fichier 
                GENERIC_WRITE, // acces en ecriture
                0,             // pas de partage 
                nullptr,       // attributs de s�curit� par d�faut
                CREATE_ALWAYS, // �crasement du pr�c�dent fichier, si existant
                0,             // attributs par d�faut
                nullptr);	   // pas de mod�le

            if ((HANDLE)(WINDOWS::LONG_PTR) (-1) == g_hPipe)  return (INIT_ERROR);
        }

        // 2) mode verbeux : cr�ation des fichiers de log
        g_verbose = KVerbose.Value();
        if (g_verbose)
        {
            // cr�ation et ouverture du fichier de log dessasemblage
            std::string logfile(g_inputFile + "_asm.log");
            g_debug.open(logfile); 

            // cr�ation et ouverture du fichier de log de marquage
            std::string taintfile(g_inputFile + "_taint.log");
            g_taint.open(taintfile);
            if (!g_taint.good() || !g_debug.good()) return (INIT_ERROR);

            // stockage de l'heure du top d�part pour statistiques
            g_timeBegin = clock();
        }

        // 4) intervalles d'octets source � marquer, si option pr�sente
        if (!KBytes.Value().empty())  pTmgrGlobal->setBytesToTaint(KBytes.Value());

        // 5) nombre maximal de contraintes ((0 si aucune)
        g_maxConstraints = KMaxConstr.Value();
        
        // 6) type d'OS hote (d�termin� par le programme appelant)
        g_osType = static_cast<OSTYPE>(KOsType.Value());
        // d�termination des num�ros de syscalls � monitorer (d�pend de l'OS)
        if (HOST_UNKNOWN == g_osType) return (INIT_ERROR);
        else SYSCALLS::defineSyscallsNumbers(g_osType);
            
        // initialisation de variable globale indiquant le d�part de l'instrumentation
        // est mise � vrai par la partie syscalls lorsque les premi�res donn�es
        // marqu�es sont cr��es (= lecture dans l'entr�e)
        g_beginInstrumentationOfInstructions = false;

        // cr�ation des clefs pour les TLS, pas de fonction de destruction
        g_tlsKeyTaint       = PIN_CreateThreadDataKey(nullptr);
        g_tlsKeySyscallData = PIN_CreateThreadDataKey(nullptr);
    }
    else if ("checkscore" == g_option) // option....checkscore :)   
    {
        returnValue = OPTION_CHECKSCORE;
        // initialisation de la la variable globale qui compte le nombre d'instructions
        g_nbIns = 0;
    }
    else return (INIT_ERROR); // option inconnue : erreur

    /**** DERNIERES INITIALISATIONS COMMUNES AUX DEUX OPTIONS **/

    // Initialisation des verrous pour le multithreading
    PIN_InitLock(&g_lock);
  
    // temps maximal d'ex�cution (valable en 'taint' et en 'checkscore')
    g_maxTime = KMaxTime.Value();
    // si non nul, cr�ation d'un thread interne au pintool : 
    // sert � la surveillance du temps maximal d'execution 
    if (g_maxTime)
    {
        THREADID tid = PIN_SpawnInternalThread(maxTimeThread, nullptr, 0, nullptr);
        if (INVALID_THREADID == tid)   return (INIT_ERROR);  
    }

    return (returnValue); // option choisie (taint ou checkScore), ou erreur d'init
} // pintoolInit()