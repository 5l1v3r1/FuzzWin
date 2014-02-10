// codes d�finissant le type d'OS pour la d�termination des num�ros d'appels syst�mes
// Le type d'OS est d�termin� par fuzzwin.exe et pass� en argument au pintool
//
// Ce fichier est inclus � la fois par le pintool et fuzzwin
//
enum OSTYPE 
{
    HOST_X86_2000,
    HOST_X86_XP,
    HOST_X86_2003,

    HOST_X86_VISTA_SP0, // pour cette version, le syscall 'setinformationfile' n'est pas le meme que pour les autres SP...
    HOST_X86_VISTA,
    HOST_X86_2008 = HOST_X86_VISTA,   // les index des syscalls sont les m�mes
    HOST_X86_2008_R2 = HOST_X86_2008, // les index des syscalls sont les m�mes
  
    HOST_X86_SEVEN,
    
    HOST_X86_8_0,
    HOST_X86_2012 = HOST_X86_8_0,    // a priori ce sont les memes
    
    HOST_X86_8_1,
    HOST_X86_2012_R2 = HOST_X86_8_1, // a priori ce sont les memes
    
    HOST_64BITS,
    HOST_END = HOST_64BITS
};