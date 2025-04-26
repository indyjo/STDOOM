#include <mint/osbind.h>
#include "atari_c2p.h"
#include "w_wad.h"
#include "z_zone.h"

#define USE_MIDRES 1

#if !USE_MIDRES
static void move_p_ofs(unsigned char *p, unsigned int data, unsigned char ofs) {
	asm ("movep.l %0, %c2(%1)" : : "d" (data), "a" (p), "i" (ofs));
}
#endif

// [phase 0..3][pixel 0..7][color 0..255]
static unsigned long c2p_table[4][8][256];

static unsigned short convert_channel(unsigned char v) {
    unsigned short r = (v & 0xe0) >> 5; // Bits 7,6,5 shifted to 2,1,0
    r |= (v & 0x10) >> 1; // STe color bit
    return r;
}
static unsigned short stcolor(unsigned char r, unsigned char g, unsigned char b) {
    unsigned short entry = convert_channel(r);
    entry <<= 4;
    entry |= convert_channel(g);
    entry <<= 4;
    entry |= convert_channel(b);
    return entry;
}

void install_palette(unsigned short *palette) {
    volatile unsigned short *reg = (unsigned short*) 0xff8240;
#if USE_MIDRES
    short numColors = 4;
#else
    short numColors = 16;
#endif
    for (short n=0; n<numColors; n++) *reg++ = *palette++;
}

void init_c2p_table() {
#if USE_MIDRES
    unsigned short stpalette[] = {stcolor(0,0,0), stcolor(85,85,85), stcolor(170,170,170), stcolor(255,255,255)};
    install_palette(stpalette);

    unsigned char* palette = W_CacheLumpName("PLAYPAL", PU_CACHE);
	unsigned short bayer[4][4] = {
		{0,  8, 2,10},
		{12, 4,14, 6},
		{ 3,11, 1, 9},
		{15, 7,13, 5}
	};
    for (int i=0; i<256; i++) {
        unsigned short r=palette[3*i], g=palette[3*i+1], b=palette[3*i+2];
        // printf("C %d: %d %d %d\n", i, r, g, b);
        // calculate brightness (0..765)
        unsigned short l = r+g+b;
        // calculate weights (0..255) of color indices 0, 1, 2, 3
        unsigned short w[] = {
            l < 256 ? 255 - l : 0,
            l < 256 ? l : (l < 512 ? 511 - l : 0),
            l < 256 ? 0 : (l < 512 ? l - 256 : 767 - l),
            l < 512 ? 0 : (l - 512),
        };
        // make weights cumulative
        for (int j=0; j<3; j++) w[j+1] += w[j];
        // renormalize weights into bayer range 0..15
        // for (int j=0; j<4; j++) w[j] >>= 4;
        // printf("  w: %2d %2d %2d %2d\n", w[0], w[1], w[2], w[3]);

        for (int phase=0; phase<4; phase++) {
            // iterate over 8 consecutive input pixels
            unsigned long combined_pdata = 0;
            for (int px = 0; px < 8; px++) {
                unsigned long pdata = 0;
                // per input pixel, iterate over two output pixels
                for (int opx = 2*px; opx < 2*px+2; opx++) {
                    // Fetch bayer threshold for output pixel.
                    unsigned short bayer_w = (bayer[phase][opx % 4] << 4) + 7;
                    // Iterate four possible output colors.
                    // Select first one that has cumulative weight >= bayer threshold.
                    // Apply bits to pixel data in opx position.
                    for (int oc = 0; oc < 4; oc++) {
                        if (oc < 3 && w[oc] < bayer_w) continue;
                        if (oc & 1) pdata |= 0x80000000 >> opx;
                        if (oc & 2) pdata |= 0x00008000 >> opx;
                        break;
                    }
                }
                c2p_table[phase][px][i] = pdata;
                combined_pdata |= pdata;
            }
        }
    }
#else
	for (int i=0; i<256; i++) {
        for (int j=0; j<4; j++) {
            unsigned int pdata = 0;
            int c = i >> j;
            if (c & 1) pdata |= 0x01000000;
            if (c & 2) pdata |= 0x00010000;
            if (c & 4) pdata |= 0x00000100;
            if (c & 8) pdata |= 0x00000001;
            for (int px = 0; px < 8; px++) {
                c2p_table[j][px][i] = pdata << (7-px);
            }
        }
	}
#endif
}

inline void c2p(register unsigned char *out, const unsigned char *in, unsigned short pixels, unsigned char phase) {
#if USE_MIDRES
	while (pixels > 7) {
		register unsigned long pdata = 0;
        for(int i=0; i<8; i++) {
            pdata |= c2p_table[phase][i][*in++];
        }
        *(unsigned long*)out = pdata;
		pixels -= 8;
		out += 4;
	}
#else
	while (pixels > 15) {
		register unsigned int pdata; // 8 pixel data for use with movep
		for (int j=0; j<2; j++) {
			pdata = 0;
			for(int i=0; i<8; i++) {
				pdata <<= 1;
				pdata |= c2p_table[phase][i][*in++];
			}
			move_p_ofs(out, pdata, j);
		}
		pixels -= 16;
		out += 8;
	}
#endif
}
