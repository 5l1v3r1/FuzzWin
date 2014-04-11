/*BEGIN_LEGAL 
Intel Open Source License 

Copyright (c) 2002-2013 Intel Corporation. All rights reserved.
 
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.  Redistributions
in binary form must reproduce the above copyright notice, this list of
conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.  Neither the name of
the Intel Corporation nor the names of its contributors may be used to
endorse or promote products derived from this software without
specific prior written permission.
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL OR
ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
END_LEGAL */

#include <TaintEngine\TaintManager.h>
#include <Instrumentation\instrumentation.h>
#include <Syscalls\syscalls.h>
#include <Translate\translate.h>

/* ================================================================== */
// Doxygen MainPage
/* ================================================================== */

//! \mainpage Page d'accueil
//! \section intro Introduction
//!
//! Ceci est l'introduction

/* ================================================================== */
// Global variables
/* ================================================================== */

// pointeur global vers classe de gestion du marquage m�moire
TaintManager_Global *pTmgrGlobal;
// pointeur global vers classe de gestion de la traduction SMT-LIB
SolverFormula       *g_pFormula;

// Clef de la TLS pour la classe de gestion du marquage registres
TLS_KEY             g_tlsKeyTaint;
// Clef de la TLS pour la classe de gestion des arguments des appels syst�mes
TLS_KEY             g_tlsKeySyscallData;

// structure de verrou, utilis�e pour acc�der aux variables globales
// en multithread
PIN_LOCK            g_lock;      

// handle du pipe de communication avec l'ext�rieur
// en MODE DEBUG : correspond � STDOUT
HANDLE              g_hPipe;     

// variable d�terminant l'instrumentation ou non des instructions
bool                g_beginInstrumentationOfInstructions; 

/** VARIABLES FOURNIES VIA PIPE ou LIGNE DE COMMANDE  **/
// chemin vers le fichier d'entr�e pour le programme fuzz�
std::string         g_inputFile;
// option du pintool : taint ou ckeckscore
std::string         g_option;
// nombre maximal de contraintes � r�cup�rer (illimit� si nul) 
UINT32              g_maxConstraints;
// temps maximal d'ex�cution du pintool (en secondes)
UINT32              g_maxTime;
// log de dessasemblage du binaire
bool                g_logasm;
// log de marquage
bool                g_logtaint;
// option pipe ou non pour l'�change du fichier initial  et de la formule finale
bool                g_nopipe;
// type d'OS hote. Sert � d�terminer les num�ros de syscalls
OSTYPE              g_osType;
// adresse de la fonction NtQueryObject (NtDll.dll)
t_NtQueryObject     g_AddressOfNtQueryObject = nullptr;

/** OPTION CHECKSCORE **/
// nombre d'instructions ex�cut�es
UINT64              g_nbIns;

ofstream            g_debug;      // fichier de log de dessassemblage
ofstream            g_taint;      // fichier de log du marquage
clock_t             g_timeBegin;  // temps d'ex�cution du pintool pour statistiques

/* ===================================================================== */
// Command line switches
/* ===================================================================== */

KNOB<std::string> KInputFile(KNOB_MODE_WRITEONCE, "pintool", "input", "", "fichier d'entree");
KNOB<UINT32>      KMaxTime(KNOB_MODE_WRITEONCE,   "pintool", "maxtime", "0", "temps maximal d'execution");
KNOB<std::string> KBytes(KNOB_MODE_WRITEONCE,     "pintool", "range", "", "intervalles d'octets � surveiller");
KNOB<UINT32>      KMaxConstr(KNOB_MODE_WRITEONCE, "pintool", "maxconstraints", "0", "nombre maximal de contraintes");
KNOB<std::string> KOption(KNOB_MODE_WRITEONCE,    "pintool", "option", "", "option 'taint' ou 'checkscore'");
KNOB<UINT32>      KOsType(KNOB_MODE_WRITEONCE,    "pintool", "os", "11", "OS hote (de 1 � 10); 11 = erreur");
KNOB<BOOL>        KLogAsm(KNOB_MODE_WRITEONCE,    "pintool", "logasm", "0", "log de dessasemblage");
KNOB<BOOL>        KLogTaint(KNOB_MODE_WRITEONCE,  "pintool", "logtaint", "0", "log de marquage");
KNOB<BOOL>        KNoPipe(KNOB_MODE_WRITEONCE,    "pintool", "nopipe", "0", "ne pas utiliser de tube nomm� (option 'input' obligatoire)");

/* ===================================================================== */
// Main procedure
/* ===================================================================== */

INT32 Usage()
{
    std::cerr << "FuzzWin : erreur d'initialisation\n";
    std::cerr << KNOB_BASE::StringKnobSummary() << endl;
    return (EXIT_FAILURE);
}

int main(int argc, char *argv[]) 
{
    // Initialisation de la gestion des symboles 'exports
    // obligatoire pour l'instrumentation des images
    //PIN_InitSymbolsAlt(EXPORT_SYMBOLS);
    
    // Initialisation de PIN. Renvoie TRUE si erreur trouv�e
    if (PIN_Init(argc, argv)) return (Usage());

    // initialisation des arguments pass�s au pintool
    // et traitement de l'instrumentation correspondante
    int initStatus = pintoolInit();
    if (OPTION_TAINT == initStatus)
    {
        // fonctions d'instrumentation des appels syst�mes
        PIN_AddSyscallEntryFunction(SYSCALLS::syscallEntry, 0);
        PIN_AddSyscallExitFunction(SYSCALLS::syscallExit, 0);

        // fonction d'instrumentation de chaque instruction
        INS_AddInstrumentFunction(INSTRUMENTATION::Instruction, 0);
    
        // Fonction appel�e lors de la fin du programme
        // version sp�cifique pour le multithreading
        PIN_AddFiniUnlockedFunction(INSTRUMENTATION::FiniTaint, 0);

        // Fonctions de notification de la creation et de la
        // suppression des threads de l'application
        PIN_AddThreadStartFunction(INSTRUMENTATION::threadStart, 0);
        PIN_AddThreadFiniFunction (INSTRUMENTATION::threadFini, 0);

        // fonction d'instrumentattion des images
        // permet d'obtenir dynamiquement l'adresse de fonctions
        // contenues dans Ntdll. Utilis� par la partie 'syscalls'
        IMG_AddInstrumentFunction(INSTRUMENTATION::Image, 0);

    }
    else if (OPTION_CHECKSCORE == initStatus)
    {
        // fonction de notification des changements de contexte
        PIN_AddContextChangeFunction(INSTRUMENTATION::changeCtx, 0);

        // fonction d'instrumentation de chaque trace d'ex�cution
        TRACE_AddInstrumentFunction(INSTRUMENTATION::insCount, 0);

        // Fonction de notification lors de la fin du programme
        // version sp�cifique pour le multithreading
        PIN_AddFiniUnlockedFunction(INSTRUMENTATION::FiniCheckScore, 0);
    }
    else return (Usage());

    // D�marrage de l'instrumentation, ne retourne jamais
    PIN_StartProgram();

    return (EXIT_SUCCESS); 
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
