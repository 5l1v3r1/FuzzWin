#include "fuzzwin_widgets.h"

// Bouton de selection personnalis�
FuzzwinButton::FuzzwinButton(QWidget *parent, const QString &text)
    : QPushButton(text, parent),
    _buttonStatus(false)    // non-OK par d�faut
{
    // mise au rouge (couleur non-ok � la construction)
    this->setStyleSheet("background-color:rgb(255,50,20)");  
}

FuzzwinButton::~FuzzwinButton() {}

void FuzzwinButton::setButtonOk()
{
    this->setStyleSheet("background-color:rgb(100,255,100)");
    this->_buttonStatus = true;
}

void FuzzwinButton::setButtonError()
{
    this->setStyleSheet("background-color:rgb(255,50,20)");
    this->_buttonStatus = false;
}

bool FuzzwinButton::getStatus() const
{
    return (this->_buttonStatus);
}

// lignes de textes personnalis�es pour accepter le "drag & drop"

FuzzwinLineEdit::FuzzwinLineEdit(QWidget *parent) : QLineEdit(parent)
{
    // accpeter le drag, le drop, mais lecture seule
    this->setDragEnabled(true);   
    this->setAcceptDrops(true);
    this->setReadOnly(true);
    this->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
}

void FuzzwinLineEdit::dragEnterEvent(QDragEnterEvent *e)
{
    // n'accepter que les URLS
    if (e->mimeData()->hasUrls()) 
    {
        QUrl url = e->mimeData()->urls().first();
        if ("file" == url.scheme()) e->acceptProposedAction();
    }
}

// initial input
InitialInputLineEdit::InitialInputLineEdit(QWidget *parent) : FuzzwinLineEdit(parent) {}

void InitialInputLineEdit::dropEvent(QDropEvent *e)
{
    // n'accepter que les URLS
    if (e->mimeData()->hasUrls()) 
    {
        QUrl url = e->mimeData()->urls().first();
        if ("file" == url.scheme())
        {           
            QFile path = url.toLocalFile();
            if (QFileInfo(path).isDir())  e->ignore();
            else 
            {
                if (path.exists())
                {
                    w->_selectInitialInput->setButtonOk();
                    this->setText(path.fileName());
                    e->acceptProposedAction();
                }
                else 
                {
                    w->_selectInitialInput->setButtonError();  
                    this->clear();
                }
                this->setCursorPosition(0);
            }
        }
    }
}

// target path
TargetPathLineEdit::TargetPathLineEdit(QWidget *parent) : FuzzwinLineEdit(parent) {}

void TargetPathLineEdit::dropEvent(QDropEvent *e)
{
    // n'accepter que les URLS
    if (e->mimeData()->hasUrls()) 
    {
        QUrl url = e->mimeData()->urls().first();
        if ("file" == url.scheme())
        {
            e->acceptProposedAction();
            bool result = this->check(url.toLocalFile());
            if (result)     w->_selectTarget->setButtonOk();
            else            w->_selectTarget->setButtonError();
        }
        this->setCursorPosition(0);
    }
}

bool TargetPathLineEdit::check(const QString &path)
{
    // conversion QString -> encodage local (avec prise en charge des accents..)
    std::string filenameLocal(path.toLocal8Bit().constData());
    bool checkResult = false;    // par d�faut, le fichier n'est pas support�

    switch (getKindOfExecutable(filenameLocal))
    {
    case SCS_64BIT_BINARY:
        if (pGlobals->osType < BEGIN_HOST_64BITS)
        {
            w->sendToLogWindow("Ex�cutable 64 bits non support� sur OS 32bits");
        }
        else 
        {
            w->sendToLogWindow("Ex�cutable 64 bits s�lectionn�");
            this->setText(path);
            checkResult = true;
        }
        break;

    case SCS_32BIT_BINARY:
        w->sendToLogWindow("Ex�cutable 32bits s�lectionn�");
        checkResult = true;
        this->setText(path);
        break;

    default:
        w->sendToLogWindow("Ex�cutable cible incompatible");  
        break;
    }
    return (checkResult);
}

// resultsDir
ResultsDirLineEdit::ResultsDirLineEdit(QWidget *parent) : FuzzwinLineEdit(parent) {}

void ResultsDirLineEdit::dropEvent(QDropEvent *e)
{
    // n'accepter que les URLS
    if (e->mimeData()->hasUrls()) 
    {
        QUrl url = e->mimeData()->urls().first();
        QString path = url.toLocalFile();
        QFileInfo pathInfo(path);
        // transformation des liens symboliques en liens complets
        if (pathInfo.isSymLink()) pathInfo = QFileInfo(pathInfo.symLinkTarget());

        if ("file" == url.scheme() && pathInfo.isDir())
        {
            e->acceptProposedAction();
            switch (this->check(pathInfo.absoluteFilePath()))
            {
            case 2: // ok => passage du bouton au vert
                w->_selectResultsDir->setButtonOk();
                break;
            case 1: // erreur => passage du bouton au rouge
                w->_selectResultsDir->setButtonError();
                break;
            case 0: // abandon
                break;
            }
        }
        this->setCursorPosition(0);
    }
}

int ResultsDirLineEdit::check(const QString &dirPath)
{
    int checkResult = 0; // par d�faut, ne rien faire
    QDir resultsDir(dirPath);

    // demander confirmation d'effacement du contenu si non vide
    if (resultsDir.entryInfoList(QDir::NoDotAndDotDot|QDir::AllEntries).count() != 0)
    {
        QMessageBox::StandardButton result = QMessageBox::question(
            this, "Confirmer l'effacement du dossier ?",
            "Le r�pertoire suivant existe d�j� : " + dirPath + "\n"
            "Voulez-vous effacer son contenu avant ex�cution ?",
            QMessageBox::Ok | QMessageBox::No | QMessageBox::Abort,
            QMessageBox::No);
            
        // Abandon => aucun changement
        if      (QMessageBox::Abort == result) return (checkResult);
        // OK => effacement du dossier
        else if (QMessageBox::Ok    == result)
        {
            bool isSuccess = resultsDir.removeRecursively(); // nouveaut� Qt 5.0
            isSuccess &= resultsDir.mkpath("."); 

            if (isSuccess) // tout s'est bien pass�
            {
                w->sendToLogWindow("Effacement du dossier de r�sultats -> Ok");
                checkResult = 2;
                this->setText(dirPath);     
            }
            else
            {
                w->sendToLogWindow("Effacement du dossier de r�sultats -> Erreur");
                checkResult = 1;
                this->clear();
            }
        }
        // No => dossier inchang�
        else
        {
            w->sendToLogWindow("Dossier de r�sultats s�lectionn� (pas d'effacement)");
            checkResult = 2;
            this->setText(dirPath);
        }
    }
    // dossier existant et vide
    else
    {
        w->sendToLogWindow("Dossier de r�sultats s�lectionn� (vide)");
        checkResult = 2;
        this->setText(dirPath);
    }

    return (checkResult);
}
