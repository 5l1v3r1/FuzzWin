@echo off

rem #############################################
rem #  SCRIPT D'UTILISATION DE FUZZWIN 64BITS  	#
rem #										 	#
rem # Ce script permet de lancer FuzzWin en 	#
rem # mode 64bits en ligne de commande			#
rem #										 	#
rem # PREREQUIS 								#
rem # - variables d'environnement PIN_ROOT 		#
rem #   et Z3_ROOT d�finies						#
rem #############################################

set currentDir=%~dp0

set fuzzwinExe="%currentDir%\fuzzwin.exe"
set resultDir="path/to/resultsDir"
set INPUT="path/to/initialInput"
set TARGET="path/to/target"

rem ## Options disponibles ##
rem ## Options pour l'algorithme ##
rem --verbose / -v : mode verbeux
rem --help         : affiche ce message
rem --traceonly    : g�n�ration d'une seule trace avec le fichier d'entr�e
rem --keepfiles    : conserve les fichiers interm�diaires
rem --dir          : r�pertoire de sortie sp�cifique (d�faut : './results/')
rem --score        : calcul du score de chaque fichier
rem --timestamp    : ajout de l'heure aux lignes de log
rem ## Options pour le pintool ##
rem --maxconstraints : nombre maximal de contraintes � r�cup�rer
rem --maxtime      : temps limite d'ex�cution de l'ex�cutable �tudie
rem --range        : intervalles d'octets � marquer (ex: 1-10;15;17-51) 

set OPTIONS= --keepfiles --timestamp --verbose --hash --score

%fuzzwinExe% ^
-t %EXE% ^
-i %INPUT% ^
--dir %resultDir% ^
%OPTIONS%

pause
 
