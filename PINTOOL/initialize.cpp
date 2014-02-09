#include "pintool.h"
#include "syscalls.h"
#include "TaintManager.h"
#include "buildFormula.h"
#include "instrumentation.h"

/* ===================================================================== */
// Procedures d'initialisation
/* ===================================================================== */

// thread interne au pintool pour gerer la dur�e de l'instrumentation
static void maxTimeThread(void *nothing)
{
    // attente du temps sp�cifi�
    PIN_Sleep(static_cast<UINT32>(1000 * g_maxTime));

    // Si le pintool n'a pas d�j� termin�, le faire (code 2 = TIMEOUT)
    PIN_ExitApplication(2); 
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

static int initOptionTaint()
{
    // chaine de caract�res d�finissant les intervalles d'octets � marquer
    // ils sont fournis grace � un argument (knob ou pipe)
    std::string bytesToTaint;

    /*** RECUPERATION DES ARGUMENTS ***/
#if DEBUG
    // l'�criture des resultats (formule SMT2) se fera � l'�cran
    g_hPipe = WINDOWS::GetStdHandle(STD_OUTPUT_HANDLE);

    if ((HANDLE)(WINDOWS::LONG_PTR) (-1) == g_hPipe)  return (EXIT_FAILURE);

    // 1) r�cup�ration des options via la ligne de commande (KNOB)
    g_inputFile      = KnobInputFile.Value();
    g_maxTime        = KnobMaxExecutionTime.Value();
    g_maxConstraints = KnobMaxConstraints.Value();
    bytesToTaint     = KnobBytesToTaint.Value(); 

    // 2) cr�ation des fichiers de d�ssasemblage et de suivi du marquage
    std::string logfile  (g_inputFile + "_dis.txt");
    std::string taintfile(g_inputFile + "_taint.txt");
 
    g_debug.open(logfile);
    g_taint.open(taintfile);
    if (!g_debug.good() || !g_taint.good()) return (EXIT_FAILURE);

    // 3) stockage de l'heure du top d�part pour statistiques
    g_timeBegin = clock();
#else
    // r�cup�ration des arguments envoy�s dans le pipe
    // 1) chemin vers l'entr�e �tudi�e
    g_inputFile = readFromPipe();
    // 2) nombre maximal de contraintes
    g_maxConstraints = LEVEL_BASE::Uint32FromString(readFromPipe());   
    // 3) temps limite d'execution en secondes
    g_maxTime = LEVEL_BASE::Uint32FromString(readFromPipe());
    // 4) offset des entrees a etudier
    bytesToTaint = readFromPipe();
#endif

    /*** INITIALISATION VARIABLES GLOBALES ET INSTANCIATIONS ***/
        
    // instanciation des classes globales (marquage m�moire et formule SMT2)
    pTmgrGlobal   = new TaintManager_Global;
    g_pFormula    = new SolverFormula;
    if (!pTmgrGlobal || !g_pFormula)  return (EXIT_FAILURE);
    
    // stockage des intervalles d'octets source � marquer
    // si la chaine de caract�res vaut "all" tout sera marqu�
    pTmgrGlobal->setBytesToTaint(bytesToTaint);

    // initialisation de variable globale indiquant le d�part de l'instrumentation
    // est mise � vrai par la partie syscalls lorsque les premi�res donn�es
    // marqu�es sont cr��es (= lecture dans l'entr�e)
    g_beginInstrumentationOfInstructions = false;

    // cr�ation des clefs pour les TLS, pas de fonction de destruction
    g_tlsKeyTaint       = PIN_CreateThreadDataKey(nullptr);
    g_tlsKeySyscallData = PIN_CreateThreadDataKey(nullptr);

    // d�termination des num�ros de syscalls � monitorer (d�pend de l'OS)
    SYSCALLS::defineSyscallsNumbers();

    // initialisation r�ussie
    return (EXIT_SUCCESS);
} // initOptionTaint

int pintoolInit()
{ 
    // Initialisation des verrous pour le multithreading
    PIN_InitLock(&g_lock);
    
    // valeur de retour (par d�faut fix� � erreur d'initialisation)
    int returnValue = EXIT_FAILURE;
    
    // r�cup�ration du handle de communication
    // DEBUG   : lecture via ligne de commande, sortie en STDOUT
    // RELEASE : lecture et �criture dans pipe "fuzzwin"
    // puis r�cup�ration de l'option du pintool('taint' ou 'checkscore')
    
    #if DEBUG
    // l'�criture des resultats (formule SMT2) se fera � l'�cran
    g_hPipe = WINDOWS::GetStdHandle(STD_OUTPUT_HANDLE);
    if ((HANDLE)(WINDOWS::LONG_PTR) (-1) == g_hPipe)  return (EXIT_FAILURE);
    
    std::string pintoolOption(KnobOption.Value());
    #else
    // ouverture du pipe de communication avec FuzzWin
    if (EXIT_FAILURE == openPipe()) return (EXIT_FAILURE);

    std::string pintoolOption(readFromPipe());
    #endif

    /*** OPTION TAINT ***/
    if (pintoolOption == "taint")
    {
        if (EXIT_FAILURE == initOptionTaint()) return (EXIT_FAILURE);
        returnValue = OPTION_TAINT;
    }
    /*** OPTION CHECKSCORE ***/
    else if (pintoolOption == "checkscore")
    {
        #if DEBUG
        g_maxTime = KnobMaxExecutionTime.Value();
        #else
        g_maxTime = LEVEL_BASE::Uint32FromString(readFromPipe());
        #endif

        // INITIALISATION VARIABLES GLOBALES
        g_nbIns          = 0;
        g_foundException = false;

        returnValue = OPTION_CHECKSCORE;  
    }
    // Option inconnue
    else return (EXIT_FAILURE);

    // cr�ation d'un thread interne au pintool : 
    // sert � la surveillance du temps maximal d'execution (si diff�rent de 0)
    if (g_maxTime)
    {
        THREADID tid = PIN_SpawnInternalThread(maxTimeThread, nullptr, 0, nullptr);
        if (INVALID_THREADID == tid)   return (EXIT_FAILURE);  
    }

    return (returnValue); // option choisie (taint ou checkScore)
} // pintoolInit()