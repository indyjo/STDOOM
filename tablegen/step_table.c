#include <math.h>
#include <stdio.h>

int main() {
    printf("// Pitch to stepping lookup\n");
    printf("// This data was generated using the 'step_table.c' tool\n");
    printf("static int steptable[256] = {");

    for (int i=0 ; i<256 ; i++) {
        if (i % 16 == 0) {
            printf("\n  ");
        } else {
            printf(" ");
        }
        int step = (int)(pow(2.0, ((i-128)/64.0))*65536.0);
        printf("%d,", step);
    }

    printf("\n};\n\n");
}