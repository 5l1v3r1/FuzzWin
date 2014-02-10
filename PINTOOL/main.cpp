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

#include "instrumentation.h"
#include "pintool.h"
#include "syscalls.h"
#include "TaintManager.h"
#include "buildFormula.h"

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

/** VARIABLES FOURNIES VIA PIPE OU LIGNE DE COMMANDE (DEBUG) **/
// chemin vers le fichier d'entr�e pour le programme fuzz�
std::string         g_inputFile;
// nombre maximal de contraintes � r�cup�rer (illimit� si nul) 
UINT32              g_maxConstraints;
// temps maximal d'ex�cution du pintool (en secondes)
UINT32              g_maxTime;

// type d'OS hote. Sert � d�terminer les num�ros de syscalls
OSTYPE              g_osType;

/** OPTION CHECKSCORE **/
// nombre d'instructions ex�cut�es
UINT64              g_nbIns;

#if DEBUG
ofstream    g_debug;      // fichier de log de dessassemblage
ofstream    g_taint;      // fichier de log du marquage
clock_t     g_timeBegin;  // temps d'ex�cution du pintool pour statistiques
#endif

/* ===================================================================== */
// Command line switches
/* ===================================================================== */

#if DEBUG
KNOB<std::string> KnobInputFile(KNOB_MODE_WRITEONCE,        "pintool", "i", "",    "fichier d'entree");
KNOB<UINT32>      KnobMaxExecutionTime(KNOB_MODE_WRITEONCE, "pintool", "m", "0",   "temps maximal d'execution");
KNOB<std::string> KnobBytesToTaint(KNOB_MODE_WRITEONCE,     "pintool", "b", "all", "intervelles d'octets � surveiller");
KNOB<UINT32>      KnobMaxConstraints(KNOB_MODE_WRITEONCE,   "pintool", "c", "0",   "nombre maximal de contraintes");
KNOB<std::string> KnobOption(KNOB_MODE_WRITEONCE, "pintool", "o", "",  "option 'taint' ou 'checkscore'");
KNOB<UINT32>      KnobOsType(KNOB_MODE_WRITEONCE, "pintool", "k", "11", "OS hote (de 1 � 10)");
#endif

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
    // Initialisation de PIN. Renvoie TRUE si erreur trouv�e
    if (PIN_Init(argc, argv))  return (Usage()); 

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
