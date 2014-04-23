#include "algorithm.h"

// cette fonction teste si l'entr�e fait planter le programme
DWORD FuzzwinAlgorithm::debugTarget(CInput *pNewInput) 
{
    // Execution de la cible en mode debug pour r�cup�rer la cause et l'emplacement de l'erreur
    // source http://msdn.microsoft.com/en-us/library/windows/desktop/ms681675(v=vs.85).aspx
    STARTUPINFO         si; 
    PROCESS_INFORMATION pi;
    DEBUG_EVENT         e;
    
    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    ZeroMemory(&e,  sizeof(e));
    si.cb = sizeof(si);
    
    std::string cmdLineDebug(this->getCmdLineDebug(pNewInput));	
    DWORD returnCode    = 0; 
    DWORD exceptionCode = 0;
    bool  continueDebug  = true; // variable de sortie de boucle infinie
        
    BOOL bSuccess = CreateProcess(
        nullptr,            // passage des arguments par ligne de commande
        (LPSTR) cmdLineDebug.c_str(),
        nullptr, nullptr,   // attributs de processus et de thread par d�faut
        FALSE,              // pas d'h�ritage des handles
        DEBUG_PROCESS | CREATE_NO_WINDOW, // mode DEBUG, pas de fenetres
        nullptr, nullptr,   // Environnement et repertoire de travail par d�faut
        &si, &pi);          // Infos en entr�e et en sortie
    
    if (!bSuccess)
    {
        this->logTimeStamp();
        this->log("    Erreur createProcess Debug");
        this->logEndOfLine();
        return 0; // fin de la proc�dure pr�matur�ment
    }

    // activation du "timer" pour stopper le debuggage au bout du temps sp�cifi�
    if (_hTimer)
    {
        // transformation secondes => intervalles de 100 nanosecondes ( x (-10 000 000))
        LARGE_INTEGER liDueTime;
        liDueTime.QuadPart = _maxExecutionTime * (-10000000LL);

        if (!SetWaitableTimer(_hTimer,
            &liDueTime, // temps d'attente
            0,          // pas de p�riodicit�
            &FuzzwinAlgorithm::timerExpired,    // fonction appel�e lors de l'expiration
            &pi.hProcess, // argument pass� : handle du processus en cours de debuggage
            0)) // peu importe le 'resume'
        {
            this->logTimeStamp();
            this->log("    Erreur Lancement du timer !!");
            this->logEndOfLine();
            return 0;
        }
    }

    /**********************/
    /* DEBUT DU DEBUGGAGE */
    /**********************/

    while (continueDebug)	
    {
        WaitForDebugEvent(&e, INFINITE);
        switch (e.dwDebugEventCode) 
        { 
        
        // Exception rencontr�e
        case EXCEPTION_DEBUG_EVENT:
            
            // r�cup�ration du code d'exception rencontr�
            exceptionCode = e.u.Exception.ExceptionRecord.ExceptionCode;
            // cas particulier : breakpoint de d�but d'ex�cution du programme
            if (EXCEPTION_BREAKPOINT == exceptionCode) 
            {
                // acquitter
                ContinueDebugEvent(e.dwProcessId, e.dwThreadId, DBG_CONTINUE); 
                break;
            }
            else // v�ritable exception rencontr�e
            { 
                PVOID exceptionAddr = e.u.Exception.ExceptionRecord.ExceptionAddress;
                char details[64];

                sprintf(&details[0], " Adresse 0x%p - Code %08X", exceptionAddr, exceptionCode);
                
                this->logEndOfLine();
                this->log(std::string(60, '*'));
                this->logEndOfLine();

                this->logTimeStamp();
                this->log(" @@@ ERREUR @@@ Fichier " + pNewInput->getFileName());      
                this->logEndOfLine();

                this->logTimeStamp();
                this->log(&details[0]);
                this->logEndOfLine();

                this->log(std::string(60, '*'));
                this->logEndOfLine();
                this->logEndOfLine();

                // enregistrement de l'erreur dans le fichier des fautes
                // ouverture en mode "app(end)"
                std::ofstream fault(_faultFile, std::ios::app);
                fault << pNewInput->getFilePath();
                fault << " Exception n� " << std::hex << exceptionCode << std::dec << std::endl;
                fault.close();

                // enregistrer le code d'erreur dans l'objet
                pNewInput->setExceptionCode(exceptionCode);

                // actions sp�cifiques cmdline/gui � mener lors de la d�couverte d'une faute
                this->faultFound();

                // arret du debug
                DebugActiveProcessStop(pi.dwProcessId);
                // fermeture des handles du programme d�bugg�
                CloseHandle(pi.hProcess); 
                CloseHandle(pi.hThread);
                // retour du code d'erreur 
                returnCode = exceptionCode;
                continueDebug = false;
            }
            break;

        case EXIT_PROCESS_DEBUG_EVENT: // = fin du programme (normale ou provoqu�e par expiration du timer)
            this->logVerbose(" (OK)");
            this->logVerboseEndOfLine();

            // acquitter
            ContinueDebugEvent(e.dwProcessId, e.dwThreadId, DBG_CONTINUE); 

            // fermeture des handles du programme d�bugg�
            CloseHandle(pi.hProcess); 
            CloseHandle(pi.hThread);

            // quitter la boucle 
            continueDebug = false;	
            break;  // quitter la boucle 
        
        default: 
            // acquitter les autres �v�nements
            ContinueDebugEvent(e.dwProcessId, e.dwThreadId, DBG_CONTINUE); 
            break;
        }      
    }

    // arret du timer (sans effet si c'est le timer qui a provoqu� l'arret du debug)
    if (_hTimer) CancelWaitableTimer(_hTimer);

    // retour du code d'erreur ou 0 si rien trouv�
    return (returnCode);
}

// renvoie la ligne de commande compl�te pour l'execution de la cible en mode debug
std::string FuzzwinAlgorithm::getCmdLineDebug(const CInput *pNewInput) const
{
    return ('"' + _targetPath + "\" \"" + pNewInput->getFilePath() + '"'); 
}

void CALLBACK FuzzwinAlgorithm::timerExpired(LPVOID arg, DWORD, DWORD)
{
    HANDLE hDebugProcess = *(static_cast<PHANDLE>(arg));

    // v�rification que le processus est toujours actif. si tel est le cas => destruction
    DWORD exitCode = 0;
    BOOL status = GetExitCodeProcess(hDebugProcess, &exitCode);

    if (STILL_ACTIVE == exitCode) TerminateProcess(hDebugProcess, 0); // revoir le code de fin ???
}