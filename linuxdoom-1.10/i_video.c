// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// $Log:$
//
// DESCRIPTION:
//	DOOM graphics stuff for X11, UNIX.
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: i_x.c,v 1.6 1997/02/03 22:45:10 b1 Exp $";

#include <stdlib.h>
#include <unistd.h>
#include <mint/osbind.h>
#include "atari_c2p.h"

#include "doomstat.h"
#include "i_system.h"
#include "v_video.h"
#include "m_argv.h"
#include "d_main.h"

#include "doomdef.h"

byte		*st_screen;
static void *old_interrupt_handler;


void I_ShutdownGraphics(void)
{
    *(void **)0x118 = old_interrupt_handler;
}



//
// I_StartFrame
//
void I_StartFrame (void)
{
    // er?

}

static int	lastmousex = 0;
static int	lastmousey = 0;
boolean		mousemoved = false;
boolean		shmFinished;

#define ACIA_RX_OVERRUN (1<<5)
#define ACIA_RX_DATA    (1<<0)
static volatile unsigned char *pAciaCtrl = (void*) 0xfffc00; 
static volatile unsigned char *pAciaData = (void*) 0xfffc02;
static volatile unsigned char *pMfpIsrb = (void*) 0xfffa11;

static unsigned char input_buffer[32];
static unsigned char *next_read = input_buffer, *next_write = input_buffer;

__attribute__((interrupt)) void keyboard_interrupt() {
    unsigned char aciaCtrl = *pAciaCtrl;
    if (aciaCtrl & ACIA_RX_OVERRUN) {
        // Handle RX overrun
        *pAciaData;
    } else while (aciaCtrl & ACIA_RX_DATA) {
        *next_write++ = *pAciaData;
        if (next_write == input_buffer + sizeof(input_buffer)) {
            next_write = input_buffer;
        }
        aciaCtrl = *pAciaCtrl;
    }
    // Clear interrupt
    *pMfpIsrb &= ~(1<<6);
}

//
// I_StartTic
//
void I_StartTic (void)
{

    while (next_read != next_write) {
        event_t event;
        unsigned char data = *next_read++;
        if (next_read == input_buffer + sizeof(input_buffer)) {
            next_read = input_buffer;
        } 
        unsigned char scan = data & 0x7f;
        event.type = (data & 0x80) ? ev_keyup : ev_keydown;
        // Atari ST keyboard layout: https://temlib.org/AtariForumWiki/index.php/Atari_ST_Scancode_diagram_by_Unseen_Menace
        if (scan == 75) {
            event.data1 = KEY_LEFTARROW;
        } else if (scan == 77) {
            event.data1 = KEY_RIGHTARROW;
        } else if (scan == 72) {
            event.data1 = KEY_UPARROW;
        } else if (scan == 80) {
            event.data1 = KEY_DOWNARROW;
        } else if (scan == 1) {
            event.data1 = KEY_ESCAPE;
        } else if (scan == 28) {
            event.data1 = KEY_ENTER;
        } else if (scan == 15) {
            event.data1 = KEY_TAB;
        } else if (scan >= 59 && scan <= 68) {
            event.data1 = KEY_F1 + scan - 59;
        } else if (scan == 0x62) { // Help key
            event.data1 = KEY_F11;
        } else if (scan == 0x61) { // Undo key
            event.data1 = KEY_F12;
        } else if (scan == 14) {
            event.data1 = KEY_BACKSPACE;
        } else if (scan == 12) {
            event.data1 = KEY_MINUS;
        } else if (scan == 13) {
            event.data1 = KEY_EQUALS;
        } else if (scan == 0x2a) {
            event.data1 = KEY_RSHIFT;
        } else if (scan == 0x1d) {
            event.data1 = KEY_RCTRL;
        } else if (scan == 0x38) {
            event.data1 = KEY_RALT;
        } else if (scan == 0x39) {
            event.data1 = ' ';
        } else if (scan >= 0x2 && scan <= 0xd) {
            event.data1 = "1234567890-="[scan-0x2];
        } else if (scan >= 0x10 && scan <= 0x1B) {
            event.data1 = "qwertyuiop[]"[scan-0x10];
        } else if (scan >= 0x1e && scan <= 0x29) {
            event.data1 = "asdfghjkl;'`"[scan-0x1e];
        } else if (scan == 0x2b) {
            event.data1 = '#';
        } else if (scan >= 0x2c && scan <= 0x35) {
            event.data1 = "zxcvbnm,./"[scan-0x2c];
        } else {
            event.data1 = 0;
            printf("Unknown input data %d\n", data);
        }
        if (event.data1 != 0) {
            D_PostEvent(&event);
        }
    }
}


//
// I_UpdateNoBlit
//
void I_UpdateNoBlit (void)
{
    // what is this?
}

//
// I_FinishUpdate
//
void I_FinishUpdate (void)
{

    static int	lasttic;
    int		tics;
    int		i;
    // UNUSED static unsigned char *bigscreen=0;

    // draws little dots on the bottom of the screen
    if (devparm)
    {

	i = I_GetTime();
	tics = i - lasttic;
	lasttic = i;
	if (tics > 20) tics = 20;

	for (i=0 ; i<tics*2 ; i+=2)
	    screens[0][ (SCREENHEIGHT-1)*SCREENWIDTH + i] = 0xff;
	for ( ; i<20*2 ; i+=2)
	    screens[0][ (SCREENHEIGHT-1)*SCREENWIDTH + i] = 0x0;
    
    }

    for (int line = 0; line < SCREENHEIGHT; line++ ) {
	c2p(st_screen + 160*line, screens[0] + 320*line, 320, line&3);
    }
}


//
// I_ReadScreen
//
void I_ReadScreen (byte* scr)
{
    memcpy (scr, screens[0], SCREENWIDTH*SCREENHEIGHT);
}


//
// I_SetPalette
//
void I_SetPalette (byte* palette)
{
    set_doom_palette(palette);
}

extern const unsigned char subset[16];

void I_InitGraphics(void)
{
    printf("Enabling supervisor mode.\n");
    Super(0L);
    st_screen = Physbase();
    printf("Replacing keyboard interrupt.\n");
    old_interrupt_handler = *(void**)0x118;
    *(void**)0x118 = keyboard_interrupt;
    printf("Initializing c2p tables...\n");
    init_c2p_table();
	unsigned char buf[128+16];
	unsigned char c = 0;
	for (int y=0; y<128; y+=8) {
		for (int x=0; x<128; x+=8) {
			for (int i=0; i<8; i++) buf[x+i] = c;
            for (int i=0; i<16; i++) {
                if (c == subset[i]) {
                    buf[x] = 4;
                    buf[x+7] = 0;
                }
            }
			c++;
		}
        for (int x=128; x<128+8; x++) buf[x] = 0;
        for (int x=128+8; x<128+16; x++) buf[x] = subset[y/8];
		for (int i=0; i<8; i++) c2p(st_screen + 160*(32+y+i), buf, 128+16, i%4);
	}
    printf ("Done.\n");
    // Set cursor to home and stop blinking.
    printf("\33H\33f\n");
}


unsigned	exptable[256];

void InitExpand (void)
{
    int		i;
	
    for (i=0 ; i<256 ; i++)
	exptable[i] = i | (i<<8) | (i<<16) | (i<<24);
}

int	inited;

