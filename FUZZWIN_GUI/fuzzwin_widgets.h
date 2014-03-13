#pragma once

#include <QtGui/QDragEnterEvent>
#include <QtGui/QDropEvent>

#include <QtWidgets/QPushButton>
#include <QtWidgets/QLineEdit>

// Bouton de selection personnalis�
class FuzzwinButton : public QPushButton
{
private:
    bool _buttonStatus;     // �tat du bouton (donn�e ok ou non)
public:
    FuzzwinButton(QWidget *parent, const QString &text);
    ~FuzzwinButton();

    void setButtonOk();     // bouton OK  => couleur verte
    void setButtonError();  // bouton NON => couleur rougeatre
    bool getStatus() const; // status du bouton
};

// lignes de textes personnalis�es pour accepter le "drag & drop"
// seul le drag est commun, le drop d�pend du type de ligne (classes d�riv�es)
class FuzzwinLineEdit : public QLineEdit
{
public:
    FuzzwinLineEdit(QWidget *parent);
protected:
    void dragEnterEvent(QDragEnterEvent *event);
};

// ligne de texte pour entr�e initiale
class InitialInputLineEdit : public FuzzwinLineEdit
{
public:
    InitialInputLineEdit(QWidget *parent);
protected:
    void dropEvent(QDropEvent *event);
};

// ligne de texte pour ex�cutable cible
class TargetPathLineEdit : public FuzzwinLineEdit
{
public:
    TargetPathLineEdit(QWidget *parent);
    bool check(const QString &path);
protected:
    void dropEvent(QDropEvent *event);
};

// ligne de texte pour dossier de r�sultats
class ResultsDirLineEdit : public FuzzwinLineEdit
{
public:
    ResultsDirLineEdit(QWidget *parent);
    int check(const QString &dirPath);
protected:
    void dropEvent(QDropEvent *event);
};
