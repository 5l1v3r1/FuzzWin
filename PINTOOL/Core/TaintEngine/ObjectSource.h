#pragma once

#include <pintool.h>
#include "TaintObject.h"    // r�f�rences crois�es de classe 
// cf http://cpp.developpez.com/faq/cpp/index.php?page=classes#CLASS_reference_croisee

// classe repr�sentant les sources d'un objet
class ObjectSource 
{ 
private:    
    TaintPtr  _src;
    ADDRINT   _val;   // 32 ou 64bits selon la compilation
    UINT32    _lengthInBits; 
public:
    ObjectSource();
    ObjectSource(const TaintPtr &taintPtr);
    ObjectSource(UINT32 lengthInBits, ADDRINT value);
    ~ObjectSource() {} 

    UINT32  getLength() const;
    bool    isSrcTainted() const;
    ADDRINT getValue() const;
    const   TaintPtr& getTaintedSource() const;
};