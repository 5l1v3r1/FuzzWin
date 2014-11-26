﻿#pragma once

#include <vector>
#include <memory> // shared_ptr

#include "pintool.h"
#include "relations.h"

// classe décrivant la source d'un objet marqué
// déclaration et implémentation dans objectSource.h et .cpp
class   ObjectSource;

// classe "mère", décrivant un objet représentant un marquage
class   Taint; 

// classe "fille", représentant un objet de taille définie (sur 'lengthInBits' bits)
template<UINT32 lengthInBits> class TaintObject;         

typedef TaintObject<1> TaintBit;    // objet marqué de taille 1 bit (= Flag)
typedef TaintObject<8> TaintByte;   // objet marqué de taille 8 bits
typedef TaintObject<16> TaintWord;  // objet marqué de taille 16 bits
typedef TaintObject<32> TaintDword; // objet marqué de taille 32 bits
typedef TaintObject<64> TaintQword; // objet marqué de taille 64 bits
typedef TaintObject<128> TaintDoubleQword; // objet marqué de taille 128 bits

typedef std::shared_ptr<Taint>      TaintPtr;       
typedef std::shared_ptr<TaintBit>   TaintBitPtr;    
typedef std::shared_ptr<TaintByte>  TaintBytePtr;   
typedef std::shared_ptr<TaintWord>  TaintWordPtr;   
typedef std::shared_ptr<TaintDword> TaintDwordPtr; 
typedef std::shared_ptr<TaintQword> TaintQwordPtr;
typedef std::shared_ptr<TaintDoubleQword> TaintDoubleQwordPtr;

// syntax sugar pour un pointeur intelligent d'un objet marqué
#define TAINT_OBJECT_PTR    std::shared_ptr<TaintObject<lengthInBits>>
// syntax sugar pour la construction d'un pointeur intelligent d'un objet marqué
#define MK_TAINT_OBJECT_PTR std::make_shared<TaintObject<lengthInBits>>

class Taint 
{
protected:                        
    // taille de l'objet marqué, en bits
    UINT32 _lengthInBits;

    // type de relation qui aboutit à l'existence de cet objet
    // Si BYTESOURCE : l'objet est issu du fichier-cible (offset en source)
    const Relation _sourceRelation;  

    // vrai si l'objet figure dans la formule pour le solveur
    bool _isDeclaredFlag;

    // chaine de caractères représentant l'objet dans la formule
    std::string _objectName;         

    // sources de cet objet
    std::vector<ObjectSource> _sources;  

    // Mode verbeux uniquement : détails supplémentaires sur l'objet
    // En particulier : adresse et nom de l'instruction ayant généré cet objet.
    std::string _verboseData;
    
    // constructeurs privés : classe non instanciable
    // obligation de passer par les classes filles
    Taint(Relation rel, UINT32 lengthInBits, std::string VerboseDetails);
    Taint(Relation rel, UINT32 lengthInBits, const ObjectSource &os1, std::string VerboseDetails);
    Taint(Relation rel, UINT32 lengthInBits, const ObjectSource &os1, const ObjectSource &os2,
            std::string VerboseDetails);
    Taint(Relation rel, UINT32 lengthInBits, const ObjectSource &os1, const ObjectSource &os2,
            const ObjectSource &os3, std::string VerboseDetails);
    Taint(Relation rel, UINT32 lengthInBits, const ObjectSource &os1, const ObjectSource &os2,
            const ObjectSource &os3, const ObjectSource &os4, std::string VerboseDetails);
public:   
    // renvoie la longueur en bits de l'objet
    UINT32 getLength() const;

    // renvoie le nombre de sources de l'objet
    UINT32 getNumberOfSources() const;
    
    // renvoie le type de relation qui lie cet objet à ses sources
    Relation getSourceRelation() const;
    
    // retourne VRAI si l'objet a été déclaré dans la formule du solveur
    bool    isDeclared() const;
    
    // indique que l'objet a été déclaré dans la formule du solveur
    void    setDeclared();

    // fixe le nom de variable correspondant à cet objet
    void    setName(std::string name);

    // retourne le nom de variable de cet objet dans la formule du solveur
    std::string getName() const;

    // retourne les sources de l'objet
    std::vector<ObjectSource> getSources() const;
    // retourne la source 'i' de l'objet
    ObjectSource getSource(UINT32 i) const;

    // ajoute l'objet marque 'taintPtr' en tant que source à l'objet
    void addSource(const TaintPtr &taintPtr);
    // ajoute la structure ObjectSource 'src' en tant que source à l'objet
    void addSource(ObjectSource src);

    // ajoute la valeur constante 'value' sur 'lengthInBits' bits en tant que source à l'objet
    template<UINT32 lengthInBits> inline void addConstantAsASource(ADDRINT value) 
    { 
        _sources.push_back(ObjectSource(lengthInBits, value)); 
    }

    // retourne les détails supplémentaires de cet objet (mode verbose)
    std::string getVerboseDetails() const;
};

template<UINT32 lengthInBits> class TaintObject : public Taint 
{
public:
    TaintObject(Relation rel, std::string VerboseDetails = "");
    TaintObject(Relation rel, const ObjectSource &os1, std::string VerboseDetails = "");
    TaintObject(Relation rel, const ObjectSource &os1, const ObjectSource &os2,
        std::string VerboseDetails = "");
    TaintObject(Relation rel, const ObjectSource &os1, const ObjectSource &os2, 
        const ObjectSource &os3, std::string VerboseDetails = "");
    TaintObject(Relation rel, const ObjectSource &os1, const ObjectSource &os2, 
        const ObjectSource &os3, const ObjectSource &os4, std::string VerboseDetails = "");
};


