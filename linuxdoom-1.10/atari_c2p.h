#ifndef ATARI_C2P_H
#define ATARI_C2P_H

void init_c2p_table();
void c2p(unsigned char *out, const unsigned char *in, unsigned short pixels, unsigned char phase);

#endif // ATARI_C2P_H