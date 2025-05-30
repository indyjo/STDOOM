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
#include "m_bbox.h"
#include "d_main.h"

#include "doomdef.h"

byte		*st_screen;
static void *old_interrupt_handler;
static unsigned short old_palette[16];


void I_ShutdownGraphics(void)
{
    *(void **)0x118 = old_interrupt_handler;
    install_palette(old_palette);
}



//
// I_StartFrame
//
void I_StartFrame (void)
{
    // er?

}

boolean		mousemoved = false;
boolean		shmFinished;

#define ACIA_RX_OVERRUN (1<<5)
#define ACIA_RX_DATA    (1<<0)
static volatile unsigned char *pAciaCtrl = (void*) 0xfffc00; 
static volatile unsigned char *pAciaData = (void*) 0xfffc02;
static volatile unsigned char *pMfpIsrb = (void*) 0xfffa11;

static unsigned char input_buffer[256];
static unsigned char *next_read = input_buffer, *next_write = input_buffer;

static char input_buffer_full() {
    unsigned char *p = next_write + 1;
    if (p == input_buffer + sizeof(input_buffer))
        p = input_buffer;
    return p == next_read;
}

static char input_buffer_empty() {
    return next_read == next_write;
}

__attribute__((interrupt)) void keyboard_interrupt() {
    unsigned char aciaCtrl = *pAciaCtrl;
    if (aciaCtrl & ACIA_RX_OVERRUN) {
        // Handle RX overrun
        *pAciaData;
    } else while (aciaCtrl & ACIA_RX_DATA) {
        if (input_buffer_full()) {
            printf("Input buffer overflow\n");
            break;
        }
        *next_write++ = *pAciaData;
        if (next_write == input_buffer + sizeof(input_buffer)) {
            next_write = input_buffer;
        }
        aciaCtrl = *pAciaCtrl;
    }
    // Clear interrupt
    *pMfpIsrb &= ~(1<<6);
}

static char can_read_bytes_from_ringbuf(size_t num) {
    if (num > sizeof(input_buffer)) {
        return 0;
    }
    unsigned char * next = next_read;
    while (next != next_write) {
        if (--num == 0) {
            return 1;
        }
        if (++next == input_buffer + sizeof(input_buffer)) {
            next = input_buffer;
        }
    }
    return 0;
}

// Read one byte from ringbuffer and return it.
// Return -1 if no byte could be read.
static int read_byte_from_ringbuf() {
    if (input_buffer_empty()) {
        return -1;
    }
    unsigned char result = *next_read++;
    if (next_read == input_buffer + sizeof(input_buffer)) {
        next_read = input_buffer;
    }
    return result; 
}

//
// I_StartTic
//
void I_StartTic (void)
{
    while (!input_buffer_empty()) {
        event_t event;
        unsigned char *next_read_copy = next_read;
        unsigned char data = read_byte_from_ringbuf();
        unsigned char scan = data & 0x7f;
        event.type = (data & 0x80) ? ev_keyup : ev_keydown;
        // Atari ST keyboard layout: https://temlib.org/AtariForumWiki/index.php/Atari_ST_Scancode_diagram_by_Unseen_Menace
        if (data == 0) {
            // Ignore. Slightly worrying that we're seeing these.
            continue;
        } if (scan == 75) {
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
        } else if (scan == 0x2a || scan == 0x36) {
            event.data1 = KEY_RSHIFT;
        } else if (scan == 0x1d) {
            event.data1 = KEY_RCTRL;
        } else if (scan == 0x38) {
            event.data1 = KEY_RALT;
        } else if (scan == 0x39) {
            event.data1 = ' ';
        } else if (scan == 0x3a) {
            // Don't know how to send CAPSLOCK key
            continue;
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
        } else if (data >= 0xf8 && data <= 0xfb) {
            // Relative mouse event
            if (!can_read_bytes_from_ringbuf(2)) {
                // Event not complete! Unread event and break loop.
                next_read = next_read_copy;
                break;
            }
            event.type = ev_mouse;
            event.data1 = (data&1 ? BT_USE : 0) | (data&2 ? BT_ATTACK : 0);
            event.data2 = 100 * (char) read_byte_from_ringbuf();
            event.data3 = -100 * (char) read_byte_from_ringbuf();
        } else if (data == 0xf6) {
            // Status report. Can be ignored?
            if (!can_read_bytes_from_ringbuf(7)) {
                // Event not complete! Unread event and break loop.
                next_read = next_read_copy;
                break;
            }
            for (int i=0; i<7; i++) {
                read_byte_from_ringbuf();
            }
            continue;
        } else if (data == 0xfc) {
            // Time of day. Can be ignored?
            if (!can_read_bytes_from_ringbuf(6)) {
                // Event not complete! Unread event and break loop.
                next_read = next_read_copy;
                break;
            }
            for (int i=0; i<6; i++) {
                read_byte_from_ringbuf();
            }
            continue;
        } else if (data == 0xfd) {
            // Joystick report. Can be ignored?
            if (!can_read_bytes_from_ringbuf(2)) {
                // Event not complete! Unread event and break loop.
                next_read = next_read_copy;
                break;
            }
            for (int i=0; i<2; i++) {
                read_byte_from_ringbuf();
            }
            continue;
        } else if (data >= 0xfe) {
            if (!can_read_bytes_from_ringbuf(1)) {
                // Event not complete! Unread event and break loop.
                next_read = next_read_copy;
                break;
            }
            read_byte_from_ringbuf();
            // TODO: Support joystick events
            continue;
        } else {
            // TODO: Implement all other IKBD event types.
            // Do not post this event.
            printf("Unknown IKBD event type %02x\n", data);
            continue;
        }
        D_PostEvent(&event);
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

    c2p_screen(st_screen, screens[0]);
    c2p_statusbar(st_screen, screens[0], dirtybox[BOXBOTTOM], dirtybox[BOXTOP] + 1, dirtybox[BOXLEFT], dirtybox[BOXRIGHT] + 1);
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
    st_screen = Physbase();
    printf("Replacing keyboard interrupt.\n");
    old_interrupt_handler = *(void**)0x118;
    *(void**)0x118 = keyboard_interrupt;
    printf("Initializing c2p tables...\n");
    save_palette(old_palette);
    init_c2p_table();
    draw_palette_table(st_screen);
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

