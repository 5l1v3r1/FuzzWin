#pragma once
#include "fuzzwin.h"

static const std::string helpMessage
(
"\n"
"FuzzWin - Fuzzer automatique sous plateforme Windows\n"
"\n"
"Usage:  fuzzwin.exe -t <targetExe> - i <firstInput> [options]\n"
"\n"
"Options:\n"
"--help        \t -h : affiche ce message\n"
"--keepfiles   \t -k : conserve les fichiers interm�diaires\n"
"--range       \t -r : intervalles d'octets � marquer (ex: 1-10;15;17-51)\n"
"--dir         \t -d : r�pertoire de sortie (d�faut : './results/')\n"
"--maxconstraints -c : nombre maximal de contraintes � r�cuperer\n"
"--maxtime     \t -m : temps limite d'ex�cution de l'ex�cutable �tudie\n"
"--score       \t -s : calcul du score de chaque fichier\n"
"--verbose     \t -v : mode verbeux\n"
);


std::string initialize(int argc, char** argv);