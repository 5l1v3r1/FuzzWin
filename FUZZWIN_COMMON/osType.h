// codes d�finissant le type d'OS pour la d�termination des num�ros d'appels syst�mes
// Le type d'OS est d�termin� par fuzzwin.exe et pass� en argument au pintool
enum OSTYPE 
{
    HOST_X86_2000,
    HOST_X86_XP,
    HOST_X86_2003,

    // pour cette version, le syscall 'setinformationfile' est diff�rent des autres SP...
    HOST_X86_VISTA_SP0, 
    HOST_X86_VISTA,
    HOST_X86_2008 = HOST_X86_VISTA,   // les index des syscalls sont les m�mes
    HOST_X86_2008_R2 = HOST_X86_2008, // les index des syscalls sont les m�mes

    HOST_X86_SEVEN,

    HOST_X86_WIN80,
    HOST_X86_2012 = HOST_X86_WIN80, 

    HOST_X86_WIN81,
    HOST_X86_2012_R2 = HOST_X86_WIN81, // a priori ce sont les memes

    BEGIN_HOST_64BITS,
    HOST_X64_BEFORE_WIN8 = BEGIN_HOST_64BITS,
    HOST_X64_WIN80,
    HOST_X64_WIN81,
    HOST_UNKNOWN
};
