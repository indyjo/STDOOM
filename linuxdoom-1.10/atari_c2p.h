#ifndef ATARI_C2P_H
#define ATARI_C2P_H

void init_c2p_table();
void c2p_screen(unsigned char *out, const unsigned char *in);
void c2p_statusbar(unsigned char *out, const unsigned char *in, short y_begin, short y_end, short x_begin, short x_end);
extern void (*set_doom_palette)(const unsigned char *colors);
void draw_palette_table(unsigned char *st_screen);
extern void (*install_palette)(const unsigned short *palette);
extern void (*save_palette)(unsigned short *palette);

#endif // ATARI_C2P_H