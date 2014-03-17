#pragma once

#include <QtGui/QDragEnterEvent>
#include <QtGui/QDropEvent>

#include <QtCore/QMimeData>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>

#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLineEdit>

#include "globals.h"

// Bouton de selection personnalis�
class FuzzwinButton : public QPushButton
{
    Q_OBJECT
private:
    bool _buttonStatus;     // �tat du bouton (donn�e ok ou non)
public:
    FuzzwinButton(QWidget *parent, const QString &text);
    ~FuzzwinButton();

    bool getStatus() const; // status du bouton

signals:
    void buttonStatusChanged(); // �mis lorsqu'un bouton a �t� modifi� (ok ou error)

public slots:
    void setButtonOk();     // mise du bouton � OK     => couleur verte
    void setButtonError();  // mise du bouton � NON_OK => couleur rougeatre 
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
    Q_OBJECT
public:
    InitialInputLineEdit(QWidget *parent);
signals:
    void conformData();
protected:
    void dropEvent(QDropEvent *event);
};

// ligne de texte pour ex�cutable cible
class TargetPathLineEdit : public FuzzwinLineEdit
{
    Q_OBJECT
public:
    TargetPathLineEdit(QWidget *parent);
signals:
    void newTargetDropped(QString);
protected:
    void dropEvent(QDropEvent *event);
};

// ligne de texte pour dossier de r�sultats
class ResultsDirLineEdit : public FuzzwinLineEdit
{
    Q_OBJECT
public:
    ResultsDirLineEdit(QWidget *parent);
signals:
    void newDirDropped(QString);
protected:
    void dropEvent(QDropEvent *event);
};
