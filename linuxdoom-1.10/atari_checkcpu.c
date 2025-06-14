#include <mint/osbind.h>
#include <mint/cookie.h>

// This function is mainly for suppressing mintlib's CPU and FPU detection, but we can also print some 
void _checkcpu() {
    long cookie = 0;
    if (C_FOUND == Getcookie(C__CPU, &cookie)) {
        Cconws("CPU: ");
        if (cookie == 0) Cconws("68000");
        else if (cookie == 10) Cconws("68010");
        else if (cookie == 20) Cconws("68020");
        else if (cookie == 30) Cconws("68030");
        else if (cookie == 40) Cconws("68040");
        else if (cookie == 60) Cconws("68060");
        else Cconws("unknown");
        Cconws("\r\n");
    } else {
        Cconws("CPU type not detected\r\n");
    }
    if (C_FOUND == Getcookie(C__FPU, &cookie)) {
        Cconws("FPU:");
        if (1 & (cookie >> 16)) {
            Cconws(" SFP-004");
        }
        if (3 & (cookie >> 17)) {
            unsigned short fpu = 3 & (cookie >> 17);
            if (fpu == 1) Cconws(" 6888?");
            else if (fpu == 2) Cconws(" 68881");
            else if (fpu == 3) Cconws(" 68882");
        }
        if (1 & (cookie >> 19)) {
            Cconws(" integrated");
        }
        Cconws("\r\n");
    } else {
        Cconws("FPU type not detected\r\n");
    }
}