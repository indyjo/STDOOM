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
//	System interface for sound.
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: i_unix.c,v 1.5 1997/02/03 22:45:10 b1 Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <math.h>

#include <sys/time.h>
#include <sys/types.h>

#ifndef LINUX
#include <sys/filio.h>
#endif

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

// Linux voxware output.
//#include <linux/soundcard.h>

// Timer stuff. Experimental.
#include <time.h>
#include <signal.h>

#include "z_zone.h"

#include "i_system.h"
#include "i_sound.h"
#include "m_argv.h"
#include "m_misc.h"
#include "m_swap.h"
#include "w_wad.h"
#include "atari_ym.h"

#include "doomdef.h"

// UNIX hack, to be removed.
#ifdef SNDSERV
// Separate sound server process.
FILE*	sndserver=0;
char*	sndserver_filename = "./sndserver ";
#elif SNDINTR

// Update all 30 millisecs, approx. 30fps synchronized.
// Linux resolution is allegedly 10 millisecs,
//  scale is microseconds.
#define SOUND_INTERVAL     500

// Get the interrupt. Set duration in millisecs.
int I_SoundSetTimer( int duration_of_tick );
void I_SoundDelTimer( void );
#else
// None?
#endif

// Pull in command line options
extern boolean nomusic, nosfx;

// A quick hack to establish a protocol between
// synchronous mix buffer updates and asynchronous
// audio writes. Probably redundant with gametic.
static int flag = 0;

// The number of internal mixing channels,
//  the samples calculated for each mixing step,
//  the size of the 16bit, 2 hardware channel (stereo)
//  mixing buffer, and the samplerate of the raw data.


// Needed for calling the actual sound output.
#define SAMPLECOUNT		256
#define NUM_CHANNELS		4
// It is 1 for 8bit times 2 for two channels.
#define BUFMUL                  2
#define MIXBUFFERSIZE		(SAMPLECOUNT*BUFMUL)

// Atari STE DMA sound hardware registers
static volatile unsigned char *pDmaSndCtrl = (void*) 0xff8901;
static volatile unsigned char *pDmaSndBasHi = (void*) 0xff8903;
static volatile unsigned char *pDmaSndBasMi = (void*) 0xff8905;
static volatile unsigned char *pDmaSndBasLo = (void*) 0xff8907;
static volatile unsigned char *pDmaSndAdrHi = (void*) 0xff8909;
static volatile unsigned char *pDmaSndAdrMi = (void*) 0xff890b;
static volatile unsigned char *pDmaSndAdrLo = (void*) 0xff890d;
static volatile unsigned char *pDmaSndEndHi = (void*) 0xff890f;
static volatile unsigned char *pDmaSndEndMi = (void*) 0xff8911;
static volatile unsigned char *pDmaSndEndLo = (void*) 0xff8913;
static volatile unsigned char *pDmaSndMode = (void*) 0xff8921;

// Palette register for debugging 
//static volatile unsigned short *pPalette = (void*)0xff8240;


#define DMASND_CTRL_OFF 0
#define DMASND_CTRL_ON 1
#define DMASND_CTRL_LOOP 2

#define DMASND_MODE_STEREO 0
#define DMASND_MODE_MONO 128
#define DMASND_MODE_HZ_6258 0
#define DMASND_MODE_HZ_12517 1
#define DMASND_MODE_HZ_25033 2
#define DMASND_MODE_HZ_50066 3

// The actual lengths of all sound effects.
int 		lengths[NUMSFX];

// The global mixing buffer.
// Basically, samples from all active internal channels
//  are modifed and added, and stored in the buffer
//  that is submitted to the audio device.
// Atari ST: For DMA sound, we keep three buffers around, internally called
//           sndbuffer1..3:
//           - mixbuffer points to one that is currently free to write into for mixing
//           - playbuffer points to the one that has most recently been sent to DMA sound
//           - lastbuffer points to the one that was playing before the current playbuffer, but which
//             might still be being finished by the DMA sound
signed char sndbuffers[4*MIXBUFFERSIZE];
#define zerobuffer (sndbuffers)
#define sndbuffer1 (sndbuffers + 1*MIXBUFFERSIZE)
#define sndbuffer2 (sndbuffers + 2*MIXBUFFERSIZE)
#define sndbuffer3 (sndbuffers + 3*MIXBUFFERSIZE)
signed char	*mixbuffer = sndbuffer1, *playbuffer = sndbuffer2, *lastbuffer = sndbuffer3;


// The channel step amount...
unsigned int	channelstep[NUM_CHANNELS];
// ... and a 0.16 bit remainder of last step.
unsigned int	channelstepremainder[NUM_CHANNELS];


// The channel data pointers, start and end.
unsigned char*	channels[NUM_CHANNELS];
unsigned char*	channelsend[NUM_CHANNELS];


// Time/gametic that the channel started playing,
//  used to determine oldest, which automatically
//  has lowest priority.
// In case number of active sounds exceeds
//  available channels.
int		channelstart[NUM_CHANNELS];

// The sound in channel handles,
//  determined on registration,
//  might be used to unregister/stop/modify,
//  currently unused.
int 		channelhandles[NUM_CHANNELS];

// SFX id of the playing sound effect.
// Used to catch duplicates (like chainsaw).
int		channelids[NUM_CHANNELS];			

// Pitch to stepping lookup, unused.
int		steptable[256];

// Volume lookups.
char		vol_lookup[128*256];

// Hardware left and right channel volume lookup.
char*		channelleftvol_lookup[NUM_CHANNELS];
char*		channelrightvol_lookup[NUM_CHANNELS];

//
// This function loads the sound data from the WAD lump,
//  for single sound.
//
void*
getsfx
( char*         sfxname,
  int*          len )
{
    unsigned char*      sfx;
    unsigned char*      paddedsfx;
    int                 i;
    int                 size;
    int                 paddedsize;
    char                name[20];
    int                 sfxlump;

    
    // Get the sound data from the WAD, allocate lump
    //  in zone memory.
    sprintf(name, "ds%s", sfxname);

    // Now, there is a severe problem with the
    //  sound handling, in it is not (yet/anymore)
    //  gamemode aware. That means, sounds from
    //  DOOM II will be requested even with DOOM
    //  shareware.
    // The sound list is wired into sounds.c,
    //  which sets the external variable.
    // I do not do runtime patches to that
    //  variable. Instead, we will use a
    //  default sound for replacement.
    if ( W_CheckNumForName(name) == -1 )
      sfxlump = W_GetNumForName("dspistol");
    else
      sfxlump = W_GetNumForName(name);
    
    size = W_LumpLength( sfxlump );

    // Debug.
    // fprintf( stderr, "." );
    //fprintf( stderr, " -loading  %s (lump %d, %d bytes)\n",
    //	     sfxname, sfxlump, size );
    //fflush( stderr );
    
    sfx = (unsigned char*)W_CacheLumpNum( sfxlump, PU_STATIC );

    // Pads the sound effect out to the mixing buffer size.
    // The original realloc would interfere with zone memory.
    paddedsize = ((size-8 + (SAMPLECOUNT-1)) / SAMPLECOUNT) * SAMPLECOUNT;

    // Allocate from zone memory.
    paddedsfx = (unsigned char*)Z_Malloc( paddedsize+8, PU_STATIC, 0 );
    // ddt: (unsigned char *) realloc(sfx, paddedsize+8);
    // This should interfere with zone memory handling,
    //  which does not kick in in the soundserver.

    // Now copy and pad.
    memcpy(  paddedsfx, sfx, size );
    for (i=size ; i<paddedsize+8 ; i++)
        paddedsfx[i] = 128;

    // Remove the cached lump.
    Z_Free( sfx );
    
    // Preserve padded length.
    *len = paddedsize;

    // Return allocated padded data.
    return (void *) (paddedsfx + 8);
}





//
// This function adds a sound to the
//  list of currently active sounds,
//  which is maintained as a given number
//  (eight, usually) of internal channels.
// Returns a handle.
//
int
addsfx
( int		sfxid,
  int		volume,
  int		step,
  int		seperation )
{
    static unsigned short	handlenums = 0;
 
    int		i;
    int		rc = -1;
    
    int		oldest = gametic;
    int		oldestnum = 0;
    int		slot;

    int		rightvol;
    int		leftvol;

    // Chainsaw troubles.
    // Play these sound effects only one at a time.
    if ( sfxid == sfx_sawup
	 || sfxid == sfx_sawidl
	 || sfxid == sfx_sawful
	 || sfxid == sfx_sawhit
	 || sfxid == sfx_stnmov
	 || sfxid == sfx_pistol	 )
    {
	// Loop all channels, check.
	for (i=0 ; i<NUM_CHANNELS ; i++)
	{
	    // Active, and using the same SFX?
	    if ( (channels[i])
		 && (channelids[i] == sfxid) )
	    {
		// Reset.
		channels[i] = 0;
		// We are sure that iff,
		//  there will only be one.
		break;
	    }
	}
    }

    // Loop all channels to find oldest SFX.
    for (i=0; (i<NUM_CHANNELS) && (channels[i]); i++)
    {
	if (channelstart[i] < oldest)
	{
	    oldestnum = i;
	    oldest = channelstart[i];
	}
    }

    // Tales from the cryptic.
    // If we found a channel, fine.
    // If not, we simply overwrite the first one, 0.
    // Probably only happens at startup.
    if (i == NUM_CHANNELS)
	slot = oldestnum;
    else
	slot = i;

    // Okay, in the less recent channel,
    //  we will handle the new SFX.
    // Set pointer to raw data.
    channels[slot] = (unsigned char *) S_sfx[sfxid].data;
    // Set pointer to end of raw data.
    channelsend[slot] = channels[slot] + lengths[sfxid];

    // Reset current handle number, limited to 0..100.
    if (!handlenums)
	handlenums = 100;

    // Assign current handle number.
    // Preserved so sounds could be stopped (unused).
    channelhandles[slot] = rc = handlenums++;

    // Set stepping???
    // Kinda getting the impression this is never used.
    channelstep[slot] = step;
    // ???
    channelstepremainder[slot] = 0;
    // Should be gametic, I presume.
    channelstart[slot] = gametic;

    // Separation, that is, orientation/stereo.
    //  range is: 1 - 256
    seperation += 1;

    // Per left/right channel.
    //  x^2 seperation,
    //  adjust volume properly.
    leftvol =
	volume - ((volume*seperation*seperation) >> 16); ///(256*256);
    seperation = seperation - 257;
    rightvol =
	volume - ((volume*seperation*seperation) >> 16);	

    // Sanity check, clamp volume.
    if (rightvol < 0 || rightvol > 127)
	I_Error("rightvol out of bounds");
    
    if (leftvol < 0 || leftvol > 127)
	I_Error("leftvol out of bounds");
    
    // Get the proper lookup table piece
    //  for this volume level???
    channelleftvol_lookup[slot] = &vol_lookup[leftvol*256];
    channelrightvol_lookup[slot] = &vol_lookup[rightvol*256];

    // Preserve sound SFX id,
    //  e.g. for avoiding duplicates of chainsaw.
    channelids[slot] = sfxid;

    // You tell me.
    return rc;
}





//
// SFX API
// Note: this was called by S_Init.
// However, whatever they did in the
// old DPMS based DOS version, this
// were simply dummies in the Linux
// version.
// See soundserver initdata().
//
void I_SetChannels()
{
  // Init internal lookups (raw data, mixing buffer, channels).
  // This function sets up internal lookups used during
  //  the mixing process. 
  int		i;
  int		j;
    
  int*	steptablemid = steptable + 128;
  
  // Okay, reset internal mixing channels to zero.
  /*for (i=0; i<NUM_CHANNELS; i++)
  {
    channels[i] = 0;
  }*/

  // This table provides step widths for pitch parameters.
  // I fail to see that this is currently used.
  for (i=-128 ; i<128 ; i++)
    steptablemid[i] = (int)(pow(2.0, (i/64.0))*65536.0);
  
  
  // Generates volume lookup tables
  //  which also turn the unsigned samples
  //  into signed samples.
  for (i=0 ; i<128 ; i++)
    for (j=0 ; j<256 ; j++)
      vol_lookup[i*256+j] = (char)((i*(j-128))/32);
}	

 
void I_SetSfxVolume(int volume)
{
  // Identical to DOS.
  // Basically, this should propagate
  //  the menu/config file setting
  //  to the state variable used in
  //  the mixing.
  snd_SfxVolume = volume;
}

// MUSIC API - dummy. Some code from DOS version.
void I_SetMusicVolume(int volume)
{
  // Internal state variable.
  snd_MusicVolume = volume;
  // Now set volume on output device.
  // Whatever( snd_MusciVolume );
}


//
// Retrieve the raw data lump index
//  for a given SFX name.
//
int I_GetSfxLumpNum(sfxinfo_t* sfx)
{
    char namebuf[9];
    sprintf(namebuf, "ds%s", sfx->name);
    return W_GetNumForName(namebuf);
}

//
// Starting a sound means adding it
//  to the current list of active sounds
//  in the internal channels.
// As the SFX info struct contains
//  e.g. a pointer to the raw data,
//  it is ignored.
// As our sound handling does not handle
//  priority, it is ignored.
// Pitching (that is, increased speed of playback)
//  is set, but currently not used by mixing.
//
int
I_StartSound
( int		id,
  int		vol,
  int		sep,
  int		pitch,
  int		priority )
{

  // UNUSED
  priority = 0;
  
#ifdef SNDSERV 
    if (sndserver)
    {
	fprintf(sndserver, "p%2.2x%2.2x%2.2x%2.2x\n", id, pitch, vol, sep);
	fflush(sndserver);
    }
    // warning: control reaches end of non-void function.
    return id;
#else
    // Debug.
    //fprintf( stderr, "starting sound %d", id );
    
    // Returns a handle (not used).
    id = addsfx( id, vol, steptable[pitch], sep );

    // fprintf( stderr, "/handle is %d\n", id );
    
    return id;
#endif
}



void I_StopSound (int handle)
{
  // You need the handle returned by StartSound.
  // Would be looping all channels,
  //  tracking down the handle,
  //  an setting the channel to zero.
  
  // UNUSED.
  handle = 0;
}


int I_SoundIsPlaying(int handle)
{
    // Ouch.
    return gametic < handle;
}


// Check whether the time is right to submit new sound.
// When either no sound is playing or the current sound buffer is almost finished.
__attribute_noinline__ boolean I_ShouldSubmitSound() {
  unsigned long addr = *pDmaSndAdrLo | (*pDmaSndAdrMi << 8) | (*pDmaSndAdrHi << 16);
  // Sometimes we'll see a runaway DMA sound chip. Stop it in its tracks!
  if (addr < sndbuffers || addr > sndbuffers + sizeof(sndbuffers)) {
        *pDmaSndCtrl &= ~DMASND_CTRL_ON;
        return 1;
  }
  boolean on = *pDmaSndCtrl & DMASND_CTRL_ON;
  // Assumption: if the most recently submitted buffer is not playing yet,
  // we can safely wait for the next VBL interrupt.
  boolean result = !on || addr == 0 || (addr >= playbuffer && addr <= playbuffer + MIXBUFFERSIZE);
  return result;
}

#define BLOCKSIZE 4

static void mix_channel(short *out, short chan, boolean first)
{
  char *leftvol_lookup = channelleftvol_lookup[chan];
  char *rightvol_lookup = channelrightvol_lookup[chan];
  unsigned char *in = channels[chan], *channelend = channelsend[chan];
  unsigned int step = channelstep[chan];
  unsigned int remainder = channelstepremainder[chan];
  short *end = out + SAMPLECOUNT;

  // Optimization: sample won't end in this frame
  if (in + ((SAMPLECOUNT * step + remainder) >> 16) < channelend)
  {
    while (out != end)
    {
      // Optimization: try BLOCKSIZE blocks of contiguous samples hoping that most samples are played
      // near their native frequency
      while (out + BLOCKSIZE < end - 1 && ((remainder + BLOCKSIZE * step) >> 16) == BLOCKSIZE) {
        for (short i = 0; i<BLOCKSIZE; i++) {
          unsigned char sample = *in++;
          short dl = leftvol_lookup[sample] << 8 | (rightvol_lookup[sample] & 0xff);
          if (first)
            *out++ = dl;
          else
            *out++ += dl;
        }
        remainder += BLOCKSIZE * step;
      }
      remainder &= 0xffff;

      unsigned char sample = *in;
      short dl = leftvol_lookup[sample] << 8 | (rightvol_lookup[sample] & 0xff);
      if (first)
        *out++ = dl;
      else
        *out++ += dl;
      remainder += step;
      in += remainder >> 16;
      remainder &= 0xffff;
    }
    channels[chan] = in;
    channelstepremainder[chan] = remainder;
    return;
  }
  
  // Sample will end in this frame. Fill frame until end of sample.
  do
  {
    unsigned char sample = *in;
    short dl = leftvol_lookup[sample] << 8 | (rightvol_lookup[sample] & 0xff);
    if (first)
      *out++ = dl;
    else
      *out++ += dl;
    remainder += step;
    in += remainder >> 16;
    remainder &= 65536 - 1;
  } while (in < channelend);

  // The channel has ended.
  channels[chan] = 0;

  // Fill remaining buffer bytes with zeros if this is the first sample.
  if (first) {
    while (out != end)
      *out++ = 0;
  }
}

//
// This function loops all active (internal) sound
//  channels, retrieves a given number of samples
//  from the raw sound data, modifies it according
//  to the current (internal) channel parameters,
//  mixes the per channel samples into the global
//  mixbuffer, clamping it to the allowed range,
//  and sets up everything for transferring the
//  contents of the mixbuffer to the (two)
//  hardware channels (left and right, that is).
//
// This function currently supports only 16bit.
//
void I_UpdateSound( void )
{
#ifdef SNDINTR
  // Debug. Count buffer misses with interrupt.
  static int misses = 0;
#endif

  short num_active_channels = 0;
  for (short chan = 0; chan < NUM_CHANNELS; chan++)
  {
    if (channels[chan])
    {
      mix_channel((short*) mixbuffer, chan, num_active_channels == 0);
      num_active_channels++;
    }
  }

  if (num_active_channels == 0) {
    // Optimization: If no channels were active, use the pre-initialized zero frame.
    mixbuffer = zerobuffer;
  }

#ifdef SNDINTR
    // Debug check.
    if ( flag )
    {
      misses += flag;
      flag = 0;
    }
    
    if ( misses > 10 )
    {
      fprintf( stderr, "I_SoundUpdate: missed 10 buffer writes\n");
      misses = 0;
    }
    
    // Increment flag for update.
    flag++;
#endif
}


// 
// This would be used to write out the mixbuffer
//  during each game loop update.
// Updates sound buffer and audio device at runtime. 
// It is called during Timer interrupt with SNDINTR.
// Mixing now done synchronous, and
//  only output be done asynchronous?
//
void
I_SubmitSound(void)
{
  signed char *tmp = lastbuffer;
  if (tmp == zerobuffer) {
    // If I_UpdateSound took the shortcut, i.e. it set mixbuffer=zerobuffer, one of the
    // sndbufferN buffers got lost. Search for a free one.
    if (playbuffer == sndbuffer1) {
      tmp = mixbuffer == sndbuffer2 ? sndbuffer3 : sndbuffer2;
    } else if (playbuffer == sndbuffer2) {
      tmp = mixbuffer == sndbuffer1 ? sndbuffer3 : sndbuffer1;
    } else if (mixbuffer == sndbuffer1) {
      tmp = sndbuffer2;
    } else {
      tmp = sndbuffer1;
    }
  }
  lastbuffer = playbuffer;
  playbuffer = mixbuffer;
  mixbuffer = tmp;
  unsigned long addr = (long) playbuffer;
  *pDmaSndBasLo = (unsigned char) (addr & 0xff);
  *pDmaSndBasMi = (unsigned char) ((addr >> 8) & 0xff);
  *pDmaSndBasHi = (unsigned char) ((addr >> 16) & 0xff);
  addr += MIXBUFFERSIZE;
  *pDmaSndEndLo = (unsigned char) (addr & 0xff);
  *pDmaSndEndMi = (unsigned char) ((addr >> 8) & 0xff);
  *pDmaSndEndHi = (unsigned char) ((addr >> 16) & 0xff);
  *pDmaSndCtrl |= DMASND_CTRL_ON | DMASND_CTRL_LOOP;
  //printf("Snd: %d %d %d %d \r", playbuffer[0], playbuffer[1], playbuffer[2], playbuffer[3]);
}



void
I_UpdateSoundParams
( int	handle,
  int	vol,
  int	sep,
  int	pitch)
{
  // I fail too see that this is used.
  // Would be using the handle to identify
  //  on which channel the sound might be active,
  //  and resetting the channel parameters.

  // UNUSED.
  handle = vol = sep = pitch = 0;
}




void I_ShutdownSound(void)
{    
#ifdef SNDSERV
  if (sndserver)
  {
    // Send a "quit" command.
    fprintf(sndserver, "q\n");
    fflush(sndserver);
  }
#else
  // Wait till all pending sounds are finished.
  int done = 0;
  int i;
  

  // FIXME (below).
  fprintf( stderr, "I_ShutdownSound: NOT finishing pending sounds\n");
  fflush( stderr );
  
  while ( !done )
  {
    for( i=0 ; i<8 && !channels[i] ; i++);
    
    // FIXME. No proper channel output.
    //if (i==8)
    done=1;
  }
#ifdef SNDINTR
  if (!nosfx || !nomusic) I_SoundDelTimer();
#endif
  
  *pDmaSndCtrl = DMASND_CTRL_OFF;
#endif

  // Done.
  return;
}






void
I_InitSound()
{ 
#ifdef SNDSERV
  char buffer[256];
  
  if (getenv("DOOMWADDIR"))
    sprintf(buffer, "%s/%s",
	    getenv("DOOMWADDIR"),
	    sndserver_filename);
  else
    sprintf(buffer, "%s", sndserver_filename);
  
  // start sound process
  if ( !access(buffer, X_OK) )
  {
    strcat(buffer, " -quiet");
    sndserver = popen(buffer, "w");
  }
  else
    fprintf(stderr, "Could not start sound server [%s]\n", buffer);
#else
    
  int i;
  
#ifdef SNDINTR
  if (nosfx && nomusic) {
    fprintf( stderr, "I_InitSound: skipping VBLANK interrupt\n" );
  } else {
    fprintf( stderr, "I_InitSound: installing VBLANK interrupt\n" );
    I_SoundSetTimer( SOUND_INTERVAL );
  }
#endif
  
  if (nosfx) {
    fprintf( stderr, "I_InitSound: nosfx\n");
    return;
  }
  // Secure and configure sound device first.
  fprintf( stderr, "I_InitSound: ");
  *pDmaSndMode = DMASND_MODE_STEREO | DMASND_MODE_HZ_12517;
  *pDmaSndCtrl = DMASND_CTRL_OFF;
  fprintf(stderr, " configured audio device\n" );

    
  // Initialize external data (all sounds) at start, keep static.
  fprintf( stderr, "I_InitSound: ");
  
  for (i=1 ; i<NUMSFX ; i++)
  { 
    // Alias? Example is the chaingun sound linked to pistol.
    if (!S_sfx[i].link)
    {
      // Load data from WAD file.
      S_sfx[i].data = getsfx( S_sfx[i].name, &lengths[i] );
    }	
    else
    {
      // Previously loaded already?
      S_sfx[i].data = S_sfx[i].link->data;
      lengths[i] = lengths[(S_sfx[i].link - S_sfx)/sizeof(sfxinfo_t)];
    }
  }

  fprintf( stderr, " pre-cached all sound data\n");
  
  // Now initialize all audio buffers with zero.
  for ( i = 0; i< MIXBUFFERSIZE; i++ ) {
    zerobuffer[i] = sndbuffer1[i] = sndbuffer2[i] = sndbuffer3[i] = 0;
  }
  
  // Finished initialization.
  fprintf(stderr, "I_InitSound: sound module ready\n");
    
#endif
}

void I_InitMusic(void)		{
  if (nomusic) {
    fprintf(stderr, "I_InitMusic: nomusic\n");
  } else {
    fprintf(stderr, "I_InitMusic: [        ]\033D\033D\033D\033D\033D\033D\033D\033D\033D");
    ymmusic_init();
    fprintf(stderr, "] done.\n");
  }
}
void I_ShutdownMusic(void)	{ }

void I_PlaySong(int handle, int looping)
{
  //printf("I_PlaySong %d %d\n", handle, looping);
  ymmusic_cmd_nr_begin++;
  ymmusic_state_cmd = YMMUSIC_PLAY | (looping?YMMUSIC_LOOP:0);
  ymmusic_cmd_nr_end++;
}

void I_PauseSong (int handle)
{
  //printf("I_PauseSong %d\n", handle);
  ymmusic_cmd_nr_begin++;
  ymmusic_state_cmd &= ~YMMUSIC_PLAY;
  ymmusic_cmd_nr_end++;
}

void I_ResumeSong (int handle)
{
  //printf("I_ResumeSong %d\n", handle);
  ymmusic_cmd_nr_begin++;
  ymmusic_state_cmd |= ~YMMUSIC_PLAY;
  ymmusic_cmd_nr_end++;
}

void I_StopSong(int handle)
{
  //printf("I_StopSong %d\n", handle);
  ymmusic_cmd_nr_begin++;
  ymmusic_state_cmd &= ~YMMUSIC_PLAY;
  ymmusic_state_cmd |= YMMUSIC_STOP;
  ymmusic_cmd_nr_end++;
}

void I_UnRegisterSong(int handle)
{
  //printf("I_UnRegisterSong %d\n", handle);
  ymmusic_cmd_nr_begin++;
  ymmusic_data_cmd = NULL;
  ymmusic_state_cmd = 0;
  ymmusic_cmd_nr_end++;
}

int I_RegisterSong(void* data)
{
  //printf("I_RegisterSong %p\n", data);
  ymmusic_cmd_nr_begin++;
  ymmusic_data_cmd = data;
  ymmusic_state_cmd = 0;
  ymmusic_cmd_nr_end++;
  return 1;
}

// Is the song playing?
int I_QrySongPlaying(int handle)
{
  return ymmusic_state & YMMUSIC_PLAY;
}


// Interrupt handler.
void I_HandleSoundTimer( int ignore )
{
  // Debug.
  //fprintf( stderr, "%c", '+' ); fflush( stderr );
  
  // Feed sound device if necesary.
  if ( flag )
  {
    // Write it to DSP device.
    I_SubmitSound();

    // Reset flag counter.
    flag = 0;
  }
  else
    return;
  
  // UNUSED, but required.
  ignore = 0;
  return;
}

static volatile void (**pVblVec)() = (void *)0x70;
static void (*pOldVblVec)() = 0;

__attribute__((interrupt)) void vbl_interrupt() {
  static boolean in_vbl = false;
  if (in_vbl) return;
  in_vbl = true;
  //volatile unsigned short *reg = (unsigned short*) 0xff8240;
  //unsigned short old = *reg;
  //*reg = 0x000f;
  if (!nomusic) ymmusic_update();
  //*reg = 0x0f00;
  if (!nosfx) {
    if (!flag) {
      I_UpdateSound();
    }
    //*reg = 0x00f0;
    if (I_ShouldSubmitSound()) {
      I_HandleSoundTimer(0);
    }
  }
  in_vbl = false;
  //*reg = old;
}

// Get the interrupt. Set duration in millisecs.
int I_SoundSetTimer( int duration_of_tick )
{
  printf("I_SoundSetTimer(%d)\n", duration_of_tick);
  if (pOldVblVec == 0) {
    pOldVblVec = *pVblVec;
  }
  *pVblVec = vbl_interrupt;
  
  return 0;
}


// Remove the interrupt. Set duration to zero.
void I_SoundDelTimer()
{
  *pVblVec = pOldVblVec;
}
