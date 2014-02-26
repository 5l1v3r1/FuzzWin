===========================
FuzzWin V1.0 - 26/02/2014
Auteur : S�bastien LECOMTE
===========================

------------
Installation
------------
1) T�l�charger la derni�re version de PIN pour Windows, pour plateforme 
   Visual Studio 2012 (kit vc11) 
   Lien : http://software.intel.com/en-us/articles/pintool-downloads
2) T�l�charger Microsoft Z3
   Lien : http://z3.codeplex.com/releases 
3) Extraire les deux archives et d�finir les variables d'environnement 
   syst�me PIN_ROOT et Z3_ROOT pointant vers la racine des dossiers idoines.
4) Cloner le d�pot, ouvrir la solution et compiler en 32bits et en 64bits.

------------
Utilisation:
------------

1) Mode "standalone"
Le pintool est utilis� sans le binaire "FuzzWin". Cela permet d'obtenir la trace
d'ex�cution d'un programme avec une entr�e donn�e.
Le mode "standalone" necessite la compilation en mode "DEBUG", en 32 et 64bits
Le mode standalone s'ex�cute � l'aide du script "PINTOOL_STANDALONE.bat" � la racine
de FuzzWin. Le script contient les instructions et les options possibles

2) Mode "fuzzing"
Apr�s compilation de la solution compl�te, le r�pertoire "BUILD" contient les binaires
fuzzwin.exe (mode 32bits) et fuzzwin_x64.exe (mode 64bits), ainsi que les pintools
fuzzwinX86.dll et fuzzwinX64.dll .Pour afficher les options, taper fuzzwin[_x64].exe -h

NB : les scripts "fuzzwin_x86.bat" et "fuzzwin_x64.bat" sont fournis � titre d'exemple