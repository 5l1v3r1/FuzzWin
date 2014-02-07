#include "check.h"

HANDLE hTimoutEvent;

static DWORD WINAPI timerThread(LPVOID lpParam)
{
    // handle du processus � monitorer, pass� en argument
    HANDLE hProcess = reinterpret_cast<HANDLE>(lpParam);

    // Attente du temps imparti, ou du signal envoy� par le thread principal
    // si timeout atteint, terminer le processus de debuggage
    DWORD result = WaitForSingleObject(hTimoutEvent, (DWORD) (pGlobals->maxExecutionTime * 1000));

    if (result == WAIT_TIMEOUT) TerminateProcess(hProcess, 0);    // revoir le code de fin ???
  
    return (result);
}


// cette fonction teste si l'entr�e fait planter le programme
DWORD debugTarget(CInput *pInput) 
{
    // Execution de la cible en mode debug pour r�cup�rer la cause et l'emplacement de l'erreur
    STARTUPINFO         si; 
    PROCESS_INFORMATION pi;
    DEBUG_EVENT         e;
    
    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    ZeroMemory(&e,  sizeof(e));
    si.cb = sizeof(si);
    
    std::string cmdLineDebug(pInput->getCmdLineDebug());	
    DWORD returnCode    = 0; 
    bool  continueDebug = true;
    
    BOOL bSuccess = CreateProcess(
        nullptr,            // passage des arguments par ligne de commande
        (LPSTR) cmdLineDebug.c_str(),
        nullptr, nullptr,   // attributs de processus et de thread par d�faut
        FALSE,              // pas d'h�ritage des handles
        DEBUG_ONLY_THIS_PROCESS | CREATE_NO_WINDOW, // mode DEBUG, pas de fenetres
        nullptr, nullptr,   // Environnement et repertoire d etravail par d�faut
        &si, &pi);          // Infos en entr�e et en sortie
    
    if (!bSuccess)
    {
        VERBOSE(std::cout << "erreur createProcess Debug\n";)
        return 0; // fin de la proc�dure pr�matur�ment
    }

    // creation d'un thread "timer" pour stopper le debuggage au bout du temps sp�cifi�
    HANDLE hTimerThread = CreateThread(
        nullptr, // attributs de s�curit� par d�faut
        0,       // taille de pile par defaut
        timerThread,    // nom de la fonction du thread
        pi.hProcess,    // argument � transmettre : le handle du processus surveill�
        0,              // attributs de creation par d�faut
        nullptr);       // pas besoin du threadId de ce thread

    // cr�ation de l'�venement de fin de debuggage
    hTimoutEvent = CreateEvent( 
        nullptr,  // attributs de s�curit� par d�faut
        TRUE,     // evenement g�r� manuellement
        FALSE,    // �tat initial non signal�
        nullptr); // evenement anonyme

    /**********************/
    /* DEBUT DU DEBUGGAGE */
    /**********************/
    while (continueDebug)	
    {
        // si erreur dans le debuggage : tout stopper et quitter la boucle
        if (!WaitForDebugEvent(&e, INFINITE)) 
        {
            VERBOSE(std::cout << "erreur WaitDebugEvent\n";)
            continueDebug = false;
            break; 
        }

        // parmi les evenements, seuls les evenements "DEBUG" nous interessent
        switch (e.dwDebugEventCode) 
        { 
            // = exception (sauf cas particulier du breakpoint)
            // en particulier, le breakpoint sera d�clench� � la premi�re instruction
            case EXCEPTION_DEBUG_EVENT:
                if (e.u.Exception.ExceptionRecord.ExceptionCode != EXCEPTION_BREAKPOINT) 
                { 
                    std::cout << "\n\t-------------------------------------------------\n ";
                    std::cout << "\t@@@ EXCEPTION @@@ Fichier " << pInput->getFileName() << std::endl;
                    std::cout << "\tAdresse 0x" << std::hex << e.u.Exception.ExceptionRecord.ExceptionAddress << " " ;
                    std::cout << "code " << e.u.Exception.ExceptionRecord.ExceptionCode << std::dec ;
                    std::cout << "\n\t-------------------------------------------------\n";
                    returnCode = e.u.Exception.ExceptionRecord.ExceptionCode;
                    continueDebug = false;
                }
                break;
            // = fin du programme. Il s'agit soit la fin normale
            // soit la fin provoqu� par le thread "gardien du temps"
            case EXIT_PROCESS_DEBUG_EVENT:
                VERBOSE(std::cout << "pas d'erreur trouv�e\n";)
                continueDebug = false;	
                // quitter la boucle break;
        }
        // Acquitter dans tous les cas, (exception ou fin de programme)
        ContinueDebugEvent(e.dwProcessId, e.dwThreadId, DBG_CONTINUE);
    }

    // signaler la fin du debug. Si le timer est toujours actif, cela le lib�rera
    SetEvent(hTimoutEvent);
    // fermeture des handles du programme d�bugg�
    CloseHandle(pi.hProcess); 
    CloseHandle(pi.hThread);
    // fermeture du handle du thread "gardien du temps"
    CloseHandle(hTimerThread);

    // en cas d'exception lev�e, enregistrer l'erreur
    if (returnCode) 
    {
        // enregistrement de l'erreur dans le fichier des fautes
        // ouverture en mode "app(end)"
        std::ofstream fault(pGlobals->faultFile, std::ios::app);
        fault << pInput->getFilePath();
        fault << " Exception n� " << std::hex << returnCode << std::dec << std::endl;
        fault.close();

        // enregistrer le code d'erreur dans l'objet
        pInput->setExceptionCode(returnCode);
    }

    return (returnCode);
}