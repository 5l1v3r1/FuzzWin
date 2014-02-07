#pragma once

#include <vector>
#include <memory> // shared_ptr
#include "pintool.h"
#include "relations.h"

#if DEBUG
static UINT32 idValue = 0;
#endif

// classe d�crivant la source d'un objet marqu�
// d�claration et impl�mentation dans objectSource.h et .cpp
class   ObjectSource;
// classe "m�re", d�crivant un objet repr�sentant un marquage
class   Taint; 
// classe "fille", repr�sentant un objet de taille d�finie (sur 'len' bits)
template<UINT32 len> class TaintObject;         

typedef TaintObject<1> TaintBit;    // objet marqu� de taille 1 bit (= Flag)
typedef TaintObject<8> TaintByte;   // objet marqu� de taille 8 bits
typedef TaintObject<16> TaintWord;  // objet marqu� de taille 16 bits
typedef TaintObject<32> TaintDword; // objet marqu� de taille 32 bits
typedef TaintObject<64> TaintQword; // objet marqu� de taille 64 bits
typedef TaintObject<128> TaintDoubleQword; // objet marqu� de taille 128 bits

typedef std::shared_ptr<Taint>      TaintPtr;       
typedef std::shared_ptr<TaintBit>   TaintBitPtr;    
typedef std::shared_ptr<TaintByte>  TaintBytePtr;   
typedef std::shared_ptr<TaintWord>  TaintWordPtr;   
typedef std::shared_ptr<TaintDword> TaintDwordPtr; 
typedef std::shared_ptr<TaintQword> TaintQwordPtr;
typedef std::shared_ptr<TaintDoubleQword> TaintDoubleQwordPtr;

class Taint 
{
protected:                        
    // taille de l'objet marqu�, en bits
    UINT32 m_len;

    // type de relation qui aboutit � l'existence de cet objet
    // Si BYTESOURCE : l'objet est issu du fichier-cible (offset en source)
    const Relation m_sourceRelation;  

    // vrai si l'objet figure dans la formule pour le solveur
    bool m_isDeclaredFlag;

    // chaine de caract�res repr�sentant l'objet dans la formule
    std::string m_objectName;         

    // sources de cet objet
    std::vector<ObjectSource> m_sources;  
    
#if DEBUG
    // un identificateur unique selon le type de classe fille
    UINT32 m_id;      
#endif

    Taint(Relation rel, UINT32 len);
    Taint(Relation rel, UINT32 len, const ObjectSource &os1);
    Taint(Relation rel, UINT32 len, const ObjectSource &os1, const ObjectSource &os2);
    Taint(Relation rel, UINT32 len, const ObjectSource &os1, const ObjectSource &os2, const ObjectSource &os3);

public:   
    ~Taint();  

    // renvoie la longueur en bits de l'objet
    const   UINT32 getLength() const;
    
    // renvoie le type de relation qui lie cet objet � ses sources
    const   Relation getSourceRelation() const;
    
    // retourne VRAI si l'objet a �t� d�clar� dans la formule du solveur
    bool    isDeclared() const;
    
    // indique que l'objet a �t� d�clar� dans la formule du solveur
    void    setDeclared();

    // fixe le nom de variable correspondant � cet objet
    void    setName(const std::string &name);

    // retourne le nom de variable de cet objet dans la formule du solveur
    const   std::string& getName() const;

    // retourne les sources de l'objet
    std::vector<ObjectSource>& getSources();
    // retourne la source 'i' de l'objet
    ObjectSource& getSources(UINT32 i);

    // ajoute l'objet marque 'taintPtr' en tant que source � l'objet
    void addSource(const TaintPtr &taintPtr);
    // ajoute la structure ObjectSource 'src' en tant que source � l'objet
    void addSource(const ObjectSource &src);

    // ajoute la valeur constante 'value' sur 'length' bits en tant que source � l'objet
    template<UINT32 length> inline void addConstantAsASource(ADDRINT value) 
    { 
        this->m_sources.push_back(ObjectSource(length, value)); 
    }

#if DEBUG
    // un identificateur unique selon le type de classe fille
    UINT32 getId() const { return (this->m_id); }     
#endif
};

template<UINT32 length> class TaintObject : public Taint 
{
public:
    explicit TaintObject(Relation rel);
    TaintObject(Relation rel, const ObjectSource &os1);
    TaintObject(Relation rel, const ObjectSource &os1, const ObjectSource &os2);
    TaintObject(Relation rel, const ObjectSource &os1, const ObjectSource &os2, const ObjectSource &os3);
    ~TaintObject();
};


