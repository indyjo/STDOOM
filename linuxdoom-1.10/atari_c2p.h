#ifndef ATARI_C2P_H
#define ATARI_C2P_H

void init_c2p_table();
void c2p(unsigned char *out, const unsigned char *in, unsigned short pixels, unsigned char phase);
void set_doom_palette(const unsigned char *colors);

#endif // ATARI_C2P_H