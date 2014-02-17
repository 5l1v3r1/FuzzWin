#pragma once 
#include "TaintManager.h"

// Macros composant les arguments utilis�s lors de l'insertion d'une fonction d'analyse 
// pour construire un objet repr�sentant l'adressage indirect d'une op�rande m�moire 
// ces arguments seront ajout�s � une IARGLIST

#define IARGLIST_BISD IARG_UINT32,    baseReg, /* registre de base utilis�*/ \
                      IARG_REG_VALUE, baseReg, /* valeur lors du callback */ \
                      IARG_UINT32,    indexReg,/* registre d'index utilis�*/ \
                      IARG_REG_VALUE, indexReg, /* valeur lors du callback*/ \
                      IARG_UINT32,    scale,    /* valeur du scale        */ \
                      IARG_UINT32,    displ     /* d�placement (ici cast�)*/ 
#define FUNCARGS_BISD THREADID tid,                        \
                      REG baseReg,  ADDRINT baseRegValue,  \
                      REG indexReg, ADDRINT indexRegValue, \
                      UINT32 scale,                        \
                      INT32 displ                          \
                      ADDRESS_DEBUG

#define IARGLIST_BIS  IARG_UINT32,    baseReg, /* registre de base utilis�*/ \
                      IARG_REG_VALUE, baseReg, /* valeur lors du callback */ \
                      IARG_UINT32,    indexReg,/* registre d'index utilis�*/ \
                      IARG_REG_VALUE, indexReg, /* valeur lors du callback*/ \
                      IARG_UINT32,    scale     /* valeur du scale */
#define FUNCARGS_BIS  THREADID tid,                        \
                      REG baseReg,  ADDRINT baseRegValue,  \
                      REG indexReg, ADDRINT indexRegValue, \
                      UINT32 scale                         \
                      ADDRESS_DEBUG

#define IARGLIST_BID  IARG_UINT32,    baseReg, /* registre de base utilis�*/ \
                      IARG_REG_VALUE, baseReg, /* valeur lors du callback */ \
                      IARG_UINT32,    indexReg,/* registre d'index utilis�*/ \
                      IARG_REG_VALUE, indexReg, /* valeur lors du callback*/ \
                      IARG_UINT32,    displ     /* d�placement (ici cast�)*/ 
#define FUNCARGS_BID  THREADID tid,                        \
                      REG baseReg,  ADDRINT baseRegValue,  \
                      REG indexReg, ADDRINT indexRegValue, \
                      INT32 displ                          \
                      ADDRESS_DEBUG

#define IARGLIST_BI   IARG_UINT32,    baseReg, /* registre de base utilis�*/ \
                      IARG_REG_VALUE, baseReg, /* valeur lors du callback */ \
                      IARG_UINT32,    indexReg,/* registre d'index utilis�*/ \
                      IARG_REG_VALUE, indexReg  /* valeur lors du callback*/ 
#define FUNCARGS_BI   THREADID tid,                        \
                      REG baseReg,  ADDRINT baseRegValue,  \
                      REG indexReg, ADDRINT indexRegValue  \
                      ADDRESS_DEBUG

#define IARGLIST_BD   IARG_UINT32,    baseReg, /* registre de base utilis� */\
                      IARG_REG_VALUE, baseReg, /* valeur lors du callback */ \
                      IARG_UINT32,    displ    /* d�placement (ici cast�)*/ 
#define FUNCARGS_BD   THREADID tid,                        \
                      REG baseReg,  ADDRINT baseRegValue,  \
                      INT32 displ                          \
                      ADDRESS_DEBUG

#define IARGLIST_B    IARG_UINT32,    baseReg, /* registre de base utilis� */\
                      IARG_REG_VALUE, baseReg  /* valeur lors du callback */ 
#define FUNCARGS_B    THREADID tid,                        \
                      REG baseReg,  ADDRINT baseRegValue   \
                      ADDRESS_DEBUG

#define IARGLIST_ISD  IARG_UINT32,    indexReg,/* registre d'index utilis�*/ \
                      IARG_REG_VALUE, indexReg, /* valeur lors du callback*/ \
                      IARG_UINT32,    scale,    /* valeur du scale        */ \
                      IARG_UINT32,    displ     /* d�placement (ici cast�)*/
#define FUNCARGS_ISD  THREADID tid,                        \
                      REG indexReg, ADDRINT indexRegValue, \
                      UINT32 scale,                        \
                      INT32 displ                          \
                      ADDRESS_DEBUG

#define IARGLIST_IS   IARG_UINT32,    indexReg,/* registre d'index utilis�*/ \
                      IARG_REG_VALUE, indexReg, /* valeur lors du callback*/ \
                      IARG_UINT32,    scale     /* valeur du scale        */
#define FUNCARGS_IS   THREADID tid,                        \
                      REG indexReg, ADDRINT indexRegValue, \
                      UINT32 scale                         \
                      ADDRESS_DEBUG

// Macro de construction d'une liste d'arguments
#define _MAKE_ARGS_EA(x)   callback = (AFUNPTR) UTILS::computeEA_##x##; \
                           IARGLIST_AddArguments(args, IARG_THREAD_ID,  \
                           IARGLIST_##x##, CALLBACK_DEBUG IARG_END); 


namespace UTILS 
{

void cUNHANDLED(INS &ins);

void cGetKindOfEA(const INS &ins);


template<UINT32 len> 
void PIN_FAST_ANALYSIS_CALL uREG  (THREADID tid, REG reg);
void PIN_FAST_ANALYSIS_CALL uMEM  (ADDRINT address, UINT32 sizeInBytes);
void PIN_FAST_ANALYSIS_CALL uFLAGS(THREADID tid);

// calcule et stocke un objet correspondant au marquage de l'adresse indirecte
// l'adresse est calcul�e sur 32bits (x86) ou 64bits (x64) et stock�e dans TaintManager
// l'appelant pourra alors l'adapter � la longueur r�elle de l'adresse effective (OperandSize)

void computeEA_BISD(FUNCARGS_BISD);
void computeEA_BIS (FUNCARGS_BIS);
void computeEA_BID (FUNCARGS_BID);
void computeEA_BI  (FUNCARGS_BI);
void computeEA_BD  (FUNCARGS_BD);
void computeEA_B   (FUNCARGS_B);
void computeEA_ISD (FUNCARGS_ISD);
void computeEA_IS  (FUNCARGS_IS);
} // namespace UTILS

#include "utils.hpp"
