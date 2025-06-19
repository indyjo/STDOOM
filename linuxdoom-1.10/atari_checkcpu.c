#include <mint/osbind.h>
#include <mint/cookie.h>

// This function is mainly for suppressing mintlib's CPU and FPU detection, but we can also print some 
void _checkcpu() {
    long cookie = 0;
    char m020_detected = 0;
    if (C_FOUND == Getcookie(C__CPU, &cookie)) {
        Cconws("CPU: ");
        if (cookie == 0) Cconws("68000");
        else if (cookie == 10) Cconws("68010");
        else if (cookie == 20) Cconws("68020");
        else if (cookie == 30) Cconws("68030");
        else if (cookie == 40) Cconws("68040");
        else if (cookie == 60) Cconws("68060");
        else Cconws("unknown");
        m020_detected = cookie >= 20;
        Cconws("\r\n");
    } else {
        Cconws("CPU type not detected\r\n");
    }
    char fpu_detected = 0;
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
            fpu_detected = 1;
        }
        if (1 & (cookie >> 19)) {
            Cconws(" integrated");
            fpu_detected = 1;
        }
        Cconws("\r\n");
    } else {
        Cconws("FPU type not detected\r\n");
    }

    char sufficient = 1;
    #ifdef __HAVE_68881__
    sufficient &= fpu_detected;
    #endif
    
    #if defined(__mc68020__) || defined(__mc68030__) || defined(__mc68040__) || defined(__mc68060__)
    sufficient &= m020_detected;
    #endif

    if (sufficient) {
        Cconws("CPU check passed.\r\n");
        return;
    }

    Cconws("\r\nDetected hardware does not\r\n");
    Cconws("meet the requirements for\r\n");
    Cconws("running this executable.\r\n\r\n");
    #if defined(__mc68020__) || defined(__mc68030__) || defined(__mc68040__) || defined(__mc68060__)
    Cconws("This executable requires a 68020+ CPU.\r\n");
    #endif
    #ifdef __HAVE_68881__
    Cconws("This executable requires an FPU.\r\n");
    #endif

    Cconws("\r\n");
    Cconws("Press [i] to ignore.\r\n");
    Cconws("Press any other key to abort.\r\n");

    long key = Cconin() & 0x7f;
    Cconws("\r\n");
    if (key == 'i' || key == 'I') {
        return;
    }

    Pterm(-1);
}