#ifndef ATARI_YM_H
#define ATARI_YM_H

#define YMMUSIC_PLAY 1
#define YMMUSIC_LOOP 2
#define YMMUSIC_STOP 4

// Set by gameloop when a music change is requested.
extern unsigned char *ymmusic_data_cmd;
extern unsigned short ymmusic_state_cmd;

// Incremented by gameloop before making any changes (to ensure atomicity).
extern unsigned short ymmusic_cmd_nr_begin;
// Incremented by gameloop after making any changes (to ensure atomicity).
extern unsigned short ymmusic_cmd_nr_end;

// Actual state. Written only in interrupt.
extern unsigned short ymmusic_state;

// Called to initialize tables.
extern void ymmusic_init();
// Called cyclically in VBL interrupt to drive the internal playback state and to push commands to YM-2149 hardware.
extern void ymmusic_update();

#endif // ATARI_YM_H