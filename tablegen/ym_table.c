#include <math.h>
#include <stdio.h>

int main() {
    printf("// Divisor table for MUS notes * 4 bit precision for pitch bend\n");
    printf("// [note 0..127][pitch bend 0..15]\n");
    printf("// This data was generated using the 'ym_table.c' tool\n");
    printf("static short ymmusic_divisors[128][16] = {\n");

    double a = 125000.0 / 440.0;
    double b = 1.0 / (16.0 * 12.0);
    double bendfactor[16];
    for (short bend = 0; bend < 16; bend++) {
        bendfactor[bend] = pow(2.0, b * bend);
    }
    for (short note = 0; note < 128; note++)
    {
        printf("  { ");
        double notefactor = pow(2.0, b * (16 * (69 - note)));
        for (short bend = 0; bend < 16; bend++)
        {
            if (bend != 0) printf(", ");
            short divisor = ceil(a * notefactor * bendfactor[bend]);
            if (divisor > 4095) divisor = 4095;
            printf("%d", divisor);
        }
        printf("},\n");
    }

    printf("};\n\n");
}