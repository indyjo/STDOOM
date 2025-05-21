#include <stdio.h>
#include <stddef.h>
#include "m_swap.h"
#include "doomstat.h"
#include "atari_ym.h"

// Contains all data needed to drive a single YM-2149 hardware channel
typedef struct
{
    // Number of ticks this note is playing.
    unsigned short ticks;
    // MUS Channel
    unsigned char channel;
    // MUS Note 0..127
    unsigned char note;
    // Index of YM channel
    unsigned char ymidx;
} ymmusic_voice_t;

typedef struct
{
    unsigned char instrument;
    // MUS Volume 0..127
    unsigned char volume;
    // MUS Pitch bend (0..255)
    unsigned char pitch_bend;
} ymmusic_channel_t;

#define YMMUSIC_READ_DELAY 16

#define YMMUSIC_PER_BYTE_PENALTY 0

// Set by gameloop when a music change is requested.
unsigned char *ymmusic_data_cmd = NULL;
unsigned short ymmusic_state_cmd = 0;

// Incremented by gameloop before making any changes (to ensure atomicity).
unsigned short ymmusic_cmd_nr_begin = 0;
// Incremented by gameloop after making any changes (to ensure atomicity).
unsigned short ymmusic_cmd_nr_end = 0;

// Set by interrupt on handling a request.
static unsigned char *ymmusic_data = NULL;
static unsigned char *ymmusic_ptr = NULL;
static unsigned char *ymmusic_end = NULL;
unsigned short ymmusic_state = 0;
static unsigned short ymmusic_wait = 0;
static unsigned short ymmusic_wait_remainder = 0;
static ymmusic_voice_t ymmusic_voices[3];
static ymmusic_channel_t ymmusic_channels[16];

#define YMMUSIC_NUMVOICES (sizeof(ymmusic_voices) / sizeof(ymmusic_voice_t))
#define YMMUSIC_NUMCHANNELS (sizeof(ymmusic_channels) / sizeof(ymmusic_channel_t))

// Incremented by interrupt if any request is processed.
static unsigned short ymmusic_ack_nr = 0;

// YM2149 sound chip access
static volatile unsigned char *pPsgSndCtrl = (void *)0xff8800;
static volatile unsigned char *pPsgSndData = (void *)0xff8802;

// Divisor table for MUS notes * 4 bit precision for pitch bend
// [note 0..127][pitch bend 0..15]
static short ymmusic_divisors[128][16];

static void ymmusic_reset()
{
    // Initialize mixer: disable all noise and tone
    *pPsgSndCtrl = 7;
    *pPsgSndData = (*pPsgSndCtrl & 0b11000000) | 0b00111111;

    for (int i = 0; i < YMMUSIC_NUMVOICES; i++)
    {
        ymmusic_voices[i].ticks = 0xffff;
        ymmusic_voices[i].channel = 0xff;
        ymmusic_voices[i].ymidx = i;
    }
    for (int i = 0; i < YMMUSIC_NUMCHANNELS; i++)
    {
        ymmusic_channels[i].instrument = 0;
        ymmusic_channels[i].volume = 100;
        ymmusic_channels[i].pitch_bend = 128;
    }
}

void ymmusic_init()
{
    ymmusic_reset();
    double a = 125000.0 / 440.0;
    double b = 1.0 / (16.0 * 12.0);
    for (short note = 0; note < 128; note++)
    {
        if (note % 16 == 15)
        {
            fprintf(stderr, ".");
        }
        for (short bend = 0; bend < 16; bend++)
        {
            ymmusic_divisors[note][bend] = (short)ceil(a * pow(2.0, b * (16 * (69 - note) + bend)));
        }
    }
}

#define FIXED_CHANNELS 2
static void ymmusic_play_note(unsigned char channel, unsigned char note, unsigned char use_volume, unsigned char volume)
{
    if (volume > 127)
    {
        volume = 127;
    }
    ymmusic_voice_t *voice = NULL;
    if (channel < FIXED_CHANNELS)
    {
        voice = ymmusic_voices + channel;
        goto found;
    }
    // First try to find a voice that is already playing this channel.
    // It's ok to not support polyphonic channels for now.
    for (int i = FIXED_CHANNELS; i < YMMUSIC_NUMVOICES; i++)
    {
        if (ymmusic_voices[i].channel == channel)
        {
            voice = ymmusic_voices + i;
            goto found;
        }
    }
    // If that fails, try to find a free voice
    for (int i = FIXED_CHANNELS; i < YMMUSIC_NUMVOICES; i++)
    {
        if (ymmusic_voices[i].ticks == 0xffff)
        {
            voice = ymmusic_voices + i;
            goto found;
        }
    }
    // If it's a primary channel, we can try to replace a secondary channel. Take the oldest one.
    if (channel < 10)
    {
        for (int i = FIXED_CHANNELS; i < YMMUSIC_NUMVOICES; i++)
        {
            // Also a primary channel? Skip!
            if (ymmusic_voices[i].channel < 10)
                continue;
            if (!voice || voice->ticks < ymmusic_voices[i].ticks)
                voice = ymmusic_voices + i;
        }
        if (voice)
            goto found;
    }
    // No appropriate voice?
    return;

    // We have found a voice.
found:

    if (use_volume)
    {
        ymmusic_channels[channel].volume = volume;
    }
    voice->channel = channel;
    voice->note = note;
    voice->ticks = 0;

    // Mixer: enable
    *pPsgSndCtrl = 7;
    if (channel == 15) {
        *pPsgSndData = *pPsgSndCtrl & ~(1 << (voice->ymidx + 3));
    } else {
        *pPsgSndData = *pPsgSndCtrl & ~(1 << voice->ymidx);
    }
}

static void ymmusic_release_note(unsigned char channel, unsigned char note)
{
    for (int i = 0; i < YMMUSIC_NUMVOICES; i++)
    {
        if (ymmusic_voices[i].channel == channel && ymmusic_voices[i].note == note)
        {
            ymmusic_voices[i].ticks = 0xffff;
            // Disable mixer for channel.
            // TODO: Maybe we want the sound to decay after release
            *pPsgSndCtrl = 7;
            *pPsgSndData = *pPsgSndCtrl | (1 << ymmusic_voices[i].ymidx) | (1 << (ymmusic_voices[i].ymidx + 3));
            return;
        }
    }
}

static void ymmusic_pitch_bend(unsigned char channel, unsigned char pitch_bend)
{
    ymmusic_channels[channel].pitch_bend = pitch_bend;
}

static void ymmusic_controller(unsigned char channel, unsigned char control, unsigned char value)
{
    if (control == 0)
    {
        ymmusic_channels[channel].instrument = value;
    }
    else if (control == 3)
    {
        ymmusic_channels[channel].volume = value;
    }
    else if (control == 4)
    {
        // Pan: ignore
    }
    else
    {
        // fprintf(stderr, "\rC %d %d %d", channel, control, value);
    }
}

static unsigned char ymmusic_voice_volume(ymmusic_voice_t *voice)
{
    return ymmusic_channels[voice->channel].volume;
}

static void ymmusic_dump(unsigned char *data, FILE *f)
{
    unsigned short lenSong = SHORT(*(unsigned short *)(data + 4));
    unsigned short offSong = SHORT(*(unsigned short *)(data + 6));
    fprintf(f, "%d bytes of music at offset %d\n", lenSong, offSong);
    fprintf(f, "%d channels, %d secondary channels, %d instruments\n",
            SHORT(*(unsigned short *)(data + 8)),
            SHORT(*(unsigned short *)(data + 10)),
            SHORT(*(unsigned short *)(data + 12)));
    unsigned char *p = data + offSong;
    short last_channel = -1;
    while (p < p + lenSong + offSong)
    {
        unsigned char last;
        do
        {
            last = (0x80 & *p);
            unsigned char event = (0x70 & *p) >> 4;
            unsigned char channel = *p & 0xf;
            if (channel == last_channel)
            {
                fprintf(f, "       ");
            }
            else
            {
                fprintf(f, " Ch %-2d ", channel);
            }
            last_channel = channel;
            switch (event)
            {
            case 0: // Release note
                fprintf(f, "release note %d", p[1] & 0x7f);
                p += 2;
                break;
            case 1: // Play note
                fprintf(f, "play note %d", p[1] & 0x7f);
                if (p[1] & 0x80)
                    fprintf(f, " at volume %d", p[2]);
                p += p[1] & 0x80 ? 3 : 2;
                break;
            case 2: // Pitch bend
                fprintf(f, "pitch bend %d", p[1]);
                p += 2;
                break;
            case 3: // System event
                fprintf(f, "system event %d", p[1]);
                p += 2;
                break;
            case 4: // Controller
                switch (p[1])
                {
                case 0:
                    fprintf(f, "instrument %d", p[2]);
                    break;
                case 1:
                    fprintf(f, "bank %d", p[2]);
                    break;
                case 2:
                    fprintf(f, "vibrato %d", p[2]);
                    break;
                case 3:
                    fprintf(f, "volume %d", p[2]);
                    break;
                case 4:
                    fprintf(f, "pan %d", p[2]);
                    break;
                case 5:
                    fprintf(f, "expression %d", p[2]);
                    break;
                case 6:
                    fprintf(f, "reverb %d", p[2]);
                    break;
                case 7:
                    fprintf(f, "chorus %d", p[2]);
                    break;
                case 8:
                    fprintf(f, "sustain %d", p[2]);
                    break;
                case 9:
                    fprintf(f, "soft %d", p[2]);
                    break;
                default:
                    fprintf(f, "controller %d %d ", p[1], p[2]);
                    break;
                }
                p += 3;
                break;
            case 5: // End of measure
                fprintf(f, "EOM");
                p += 1;
                break;
            case 6: // Finish
                fprintf(f, "FINISH\n");
                goto finish;
            case 7: // Unused
                fprintf(f, "Unused %d", p[1]);
                p += 2;
                break;
            }
            fprintf(f, "\n");
        } while (!last);

        short delay = 0;
        do
        {
            delay = (delay << 7) + (0x7f & *p);
        } while (0x80 & *p++);
        fprintf(f, "Delay %d\n", delay);
    }
finish:
}

static void ymmusic_dump_file(unsigned char *data)
{
    FILE *f = fopen("musdump.txt", "w");
    ymmusic_dump(data, f);
    fclose(f);
}

// Called cyclically to drive the internal playback state and to push commands to YM-2149 hardware.
void ymmusic_update()
{
    if (ymmusic_cmd_nr_end != ymmusic_ack_nr && ymmusic_cmd_nr_end == ymmusic_cmd_nr_begin)
    {
        // A command has been received and it is consistent.
        if (ymmusic_data != ymmusic_data_cmd)
        {
            ymmusic_data = ymmusic_data_cmd;
            ymmusic_ptr = NULL;
            ymmusic_reset();
        }
        if (ymmusic_state != ymmusic_state_cmd)
        {
            ymmusic_state = ymmusic_state_cmd;
        }
        ymmusic_ack_nr = ymmusic_cmd_nr_end;
    }

    // Skip music data header
    if (ymmusic_data != NULL && ymmusic_ptr == NULL)
    {
        if (ymmusic_data[0] == 'M' && ymmusic_data[1] == 'U' && ymmusic_data[2] == 'S' && ymmusic_data[3] == 0x1a)
        {
            // ymmusic_dump_file(ymmusic_data);
            unsigned short lenSong = SHORT(*(unsigned short *)(ymmusic_data + 4));
            unsigned short offSong = SHORT(*(unsigned short *)(ymmusic_data + 6));
            ymmusic_end = ymmusic_data + offSong + lenSong;
            ymmusic_ptr = ymmusic_data + offSong;
        }
    }

    // No cursor? Do nothing.
    if (!ymmusic_ptr)
    {
        ymmusic_reset();
        return;
    }

    // Not playing? Do nothing.
    if (!(ymmusic_state & YMMUSIC_PLAY))
    {
        ymmusic_reset();
        return;
    }

    // We're playing, advance hardware channels
    for (int i = 0; i < 3; i++)
    {
        ymmusic_voice_t *voice = ymmusic_voices + i;
        if (voice->ticks == 0xffff)
        {
            continue;
        }
        ymmusic_channel_t *channel = ymmusic_channels + voice->channel;

        // Frequency
        short divisor;
        if (channel->pitch_bend < 128 - 8)
        {
            divisor = ymmusic_divisors[voice->note - 1][(channel->pitch_bend + 7) >> 3];
        }
        else
        {
            divisor = ymmusic_divisors[voice->note][(channel->pitch_bend - 128 + 8) >> 3];
        }
        *pPsgSndCtrl = 0 + 2 * voice->ymidx;
        *pPsgSndData = divisor & 0xff;
        *pPsgSndCtrl = 1 + 2 * voice->ymidx;
        *pPsgSndData = divisor >> 8;

        // Amplitude
        *pPsgSndCtrl = 8 + voice->ymidx;
        unsigned char new_volume = (ymmusic_voice_volume(voice) >> 3) + snd_MusicVolume;
        if (new_volume > 15)
        {
            new_volume -= 15;
        }
        else
        {
            new_volume = 0;
        }
        if (*pPsgSndCtrl != new_volume)
        {
            *pPsgSndData = new_volume;
        }

        voice->ticks++;

        if (voice->ticks == 0xffff)
        {
            // Reaching end? Disable mixer for channel.
            *pPsgSndCtrl = 7;
            *pPsgSndData = *pPsgSndCtrl | (1 << voice->ymidx);
        }
    }

    // Waiting? Decrement and do nothing.
    if (ymmusic_wait > 0)
    {
        ymmusic_wait--;
        return;
    }

    // Parse the next event or delay (depending on state) as long as we don't actually have to wait.
    do
    {
        // Save cursor so that we can calculate the time spent reading these bytes.
        unsigned char *ymmusic_ptr_orig = ymmusic_ptr;
        if (ymmusic_state & YMMUSIC_READ_DELAY)
        {
            // Read a delay in 1/140th of a second
            short delay = 0;
            do
            {
                delay = (delay << 7) + (0x7f & *ymmusic_ptr);
            } while (0x80 & *ymmusic_ptr++);
            // Convert delay into 1/512th of 1/55s.
            ymmusic_wait_remainder += 201 * delay;
            // Read regular event next
            ymmusic_state &= ~YMMUSIC_READ_DELAY;
        }
        else
        {
            // Parse a regular event
            if (0x80 & *ymmusic_ptr)
            {
                // Is last regular event? Read delay next.
                ymmusic_state |= YMMUSIC_READ_DELAY;
            }
            unsigned char event = (0x70 & *ymmusic_ptr) >> 4;
            unsigned char channel = *ymmusic_ptr & 0xf;
            switch (event)
            {
            case 0: // Release note
                ymmusic_release_note(channel, ymmusic_ptr[1] & 0x7f);
                ymmusic_ptr += 2;
                break;
            case 1: // Play note
                ymmusic_play_note(channel, ymmusic_ptr[1] & 0x7f, ymmusic_ptr[1] & 0x80, ymmusic_ptr[2]);
                ymmusic_ptr += (ymmusic_ptr[1] & 0x80) ? 3 : 2;
                break;
            case 2: // Pitch bend
                ymmusic_pitch_bend(channel, ymmusic_ptr[1]);
                ymmusic_ptr += 2;
                break;
            case 3: // System event
                fprintf(stderr, "\rSystem event %d %d", channel, ymmusic_ptr[1]);
                ymmusic_ptr += 2;
                break;
            case 4: // Controller
                // fprintf(stderr, "\rC %d %d %d ", channel, ymmusic_ptr[1], ymmusic_ptr[2]);
                ymmusic_controller(channel, ymmusic_ptr[1], ymmusic_ptr[2]);
                ymmusic_ptr += 3;
                break;
            case 5: // End of measure
                fprintf(stderr, "\rEOM %d", channel);
                ymmusic_ptr += 1;
                break;
            case 6: // Finish
                if (!(ymmusic_state & YMMUSIC_LOOP))
                {
                    ymmusic_state &= ~YMMUSIC_PLAY;
                }
                ymmusic_ptr = NULL;
                break;
            case 7: // Unused
                ymmusic_ptr += 2;
                break;
            }
        }
        // Add delay penalty for number of bytes read. MIDI transfers 3125 bytes/s, that's 9/512 of 1/55s per byte.
        ymmusic_wait_remainder += YMMUSIC_PER_BYTE_PENALTY * (ymmusic_ptr - ymmusic_ptr_orig);

        // Repeat until we have to wait at least one tick.
    } while (ymmusic_wait_remainder < 512 && ymmusic_ptr);

    ymmusic_wait = ymmusic_wait_remainder / 512;
    ymmusic_wait_remainder %= 512;
}
