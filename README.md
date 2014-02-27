FuzzWin V1.0
============

Auteur : S�bastien LECOMTE   
Version 1.0 du 26/02/2014
**************************

Installation
------------

1. T�l�charger la derni�re version de PIN pour Windows, kit "vc11"    
   (<http://software.intel.com/en-us/articles/pintool-downloads>) 
2. T�l�charger Microsoft Z3 (<http://z3.codeplex.com/releases>) 
3. Extraire les deux archives et d�finir les variables d'environnement syst�me PIN\_ROOT et Z3\_ROOT pointant vers la racine des dossiers idoines.
4. Cloner le d�pot, ouvrir la solution et compiler en 32bits et en 64bits.

Utilisation
-----------

1. Mode *standalone*  
Le pintool est utilis� sans le binaire "FuzzWin". Cela permet d'obtenir la trace d'ex�cution d'un programme avec une entr�e donn�e.  
Le mode "standalone" n�cessite la compilation en mode *DEBUG* en 32bits et 64bits. Le mode standalone s'ex�cute � l'aide du script "PINTOOL\_STANDALONE.bat" � la racine de FuzzWin. Le script contient les instructions et les options possibles.  
2. Mode *fuzzing*  
Apr�s compilation de la solution compl�te, le r�pertoire "BUILD" contient les binaires *fuzzwin.exe* et *fuzzwin_x64.exe*, ainsi que les pintools *fuzzwinX86.dll* et *fuzzwinX64.dll*.
Pour afficher les options, taper *fuzzwin[\_x64].exe -h*

NB : les scripts *fuzzwin\_x86.bat* et *fuzzwin\_x64.bat* sont fournis � titre d'exemple