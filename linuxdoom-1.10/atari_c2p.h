#ifndef ATARI_C2P_H
#define ATARI_C2P_H

void init_c2p_table();
void c2p_screen(unsigned char *out, const unsigned char *in);
void set_doom_palette(const unsigned char *colors);
void draw_palette_table(unsigned char *st_screen);

#endif // ATARI_C2P_H