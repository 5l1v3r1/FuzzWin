#include "fuzzwin.h"
#include <QtWidgets/QApplication>
#include <QMessagebox>

CGlobals *pGlobals;
FUZZWIN_GUI *w;

int main(int argc, char *argv[])
{
    int returnValue = 0;

    // initialisation des variables globales
    pGlobals = new CGlobals;
    if (!pGlobals) return (EXIT_FAILURE);

    // test de la compatibilit� de l'OS
    pGlobals->osType = getNativeArchitecture();
    if (HOST_UNKNOWN == pGlobals->osType)
    {
        QMessageBox::critical(nullptr, "Erreur", "OS non support�", QMessageBox::Close);
        delete (pGlobals);
        return (EXIT_FAILURE);
    }

    QApplication a(argc, argv);    
    
    // test de la pr�sence des DLL du pintool
    QString exePath = a.applicationDirPath();

    w = new FUZZWIN_GUI;
   
    w->initialize();
    w->show();
    returnValue = a.exec();

    delete (pGlobals);

    return (returnValue);
}
