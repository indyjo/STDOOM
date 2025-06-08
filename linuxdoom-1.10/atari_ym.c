#include <stdio.h>
#include <stddef.h>
#include "m_swap.h"
#include "doomstat.h"
#include "atari_ym.h"

typedef struct
{
    // Points to an array of data points.
    char *data;
    // Indexes into array which mark certain points in time.
    unsigned char sustain_begin, release_begin, last;
} envelope_t;

typedef struct
{
    const char *name;
    envelope_t *volume_envelope;
    envelope_t *pitch_envelope;
    envelope_t *note_envelope;
    unsigned char override_note;
    unsigned char overrides_note : 1;
    unsigned char enables_noise : 1;
} instrument_t;

// Contains all data needed to drive a single YM-2149 hardware channel
typedef struct
{
    // Number of ticks since note was pressed or released (0xffff means off).
    unsigned short ticks;
    // MUS Channel
    unsigned char channel;
    // MUS Note 0..127
    unsigned char note;
    // Index of YM channel
    unsigned char ymidx;
    // Whether the note has been released
    unsigned char released;
    // Instrument playing
    instrument_t *instrument;
} ymmusic_voice_t;

typedef struct
{
    // MUS instrument index assigned to channel
    unsigned char instrument;
    // MUS Volume 0..127
    unsigned char volume;
    // MUS Pitch bend (0..255)
    unsigned char pitch_bend;
} ymmusic_channel_t;

#define ENVELOPE(dataname, sustain, release) {          \
    .data = dataname,                                   \
    .sustain_begin = sustain,                           \
    .release_begin = release,                           \
    .last = sizeof(dataname)-1}

static char overdriven_guitar_volume_envelope_data[] =
    {-70, -40, -10, 0, -1, -1, -2, -2, -3, -3, -4, -4, -5, -5, -6, -6, -7, -7, -8, -8, -9, -9, -10, -10};
static envelope_t overdriven_guitar_volume_envelope = ENVELOPE(overdriven_guitar_volume_envelope_data, 3, 5);
static instrument_t overdriven_guitar = {
    .name = "Overdriven Guitar",
    .volume_envelope = &overdriven_guitar_volume_envelope,
};

static char distortion_guitar_volume_envelope_data[] =
    {-20, 60, 10, 0, 8, -1, -1, -2, -3, -3, -4, -4, -5, -5, -6, -6, -7, -7, -8, -8, -9, -9, -10, -10};
static char distortion_guitar_pitch_envelope_data[] =
    {80, 40, 20, 0, 20, 0};
static envelope_t distortion_guitar_volume_envelope = ENVELOPE(distortion_guitar_volume_envelope_data, 3, 5);
static envelope_t distortion_guitar_pitch_envelope = ENVELOPE(distortion_guitar_pitch_envelope_data, 3, 5);
static instrument_t distortion_guitar = {
    .name = "Distortion Guitar",
    .volume_envelope = &distortion_guitar_volume_envelope,
    .pitch_envelope = &distortion_guitar_pitch_envelope,
};

static char dummy_instrument_volume_envelope_data[] =
    {-20, 0, -16, -32, -64};
static envelope_t dummy_instrument_volume_envelope = ENVELOPE(dummy_instrument_volume_envelope_data, 1, 2);
static instrument_t dummy_instrument = {
    .name = "Dummy Instrument",
    .volume_envelope = &dummy_instrument_volume_envelope,
};

static char bass_drum_volume_envelope_data[] =
    {0, 0, 0, 0, 0, -60, -80, -100, -120};
static char bass_drum_note_envelope_data[] =
    {0, -4, -8, -18, -26, -32, -35, -35, -36};
static envelope_t bass_drum_volume_envelope = ENVELOPE(bass_drum_volume_envelope_data, 16, 16);
static envelope_t bass_drum_note_envelope = ENVELOPE(bass_drum_note_envelope_data, 16, 16);
static instrument_t bass_drum = {
    .name = "Bass Drum",
    .volume_envelope = &bass_drum_volume_envelope,
    .note_envelope = &bass_drum_note_envelope,
    .override_note = 64,
    .overrides_note = 1,
};

static char snare_volume_envelope_data[] =
    {120, 20, 10, 4, 0, -4, -8, -12, -16, -20, -24, -28, -32, -34, -36, -38, -40, -41, -42, -43, -44, -45, -46, -47,
     -48, -49, -50, -51, -52, -53, -54, -55, -56, -57, -58, -59, -60, -61, -62, -63};
static envelope_t snare_volume_envelope = ENVELOPE(snare_volume_envelope_data, 127, 127);
static char electric_snare_note_envelope_data[] =
    {+48, +0, -16, -10, -30, -28, -37, -33, -36};
static envelope_t electric_snare_note_envelope = ENVELOPE(electric_snare_note_envelope_data, 127, 127);
static instrument_t electric_snare = {
    .name = "Electric Snare",
    .volume_envelope = &snare_volume_envelope,
    .note_envelope = &electric_snare_note_envelope,
    .override_note = 81,
    .overrides_note = 1,
    .enables_noise = 1,
};

static char dummy_percussion_volume_envelope_data[] =
    {40, -60, -98, -120};
static envelope_t dummy_percussion_volume_envelope = ENVELOPE(dummy_percussion_volume_envelope_data, 127, 127);
static instrument_t dummy_percussion = {
    .name = "Dummy Percussion",
    .volume_envelope = &dummy_percussion_volume_envelope,
    .override_note = 50,
    .overrides_note = 1,
    .enables_noise = 1,
};

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
    double bendfactor[16];
    for (short bend = 0; bend < 16; bend++) {
        bendfactor[bend] = pow(2.0, b * bend);
    }
    for (short note = 0; note < 128; note++)
    {
        if (note % 16 == 15)
        {
            fprintf(stderr, ".");
        }
        double notefactor = pow(2.0, b * (16 * (69 - note)));
        for (short bend = 0; bend < 16; bend++)
        {
            short divisor = ceil(a * notefactor * bendfactor[bend]);
            if (divisor > 4095) divisor = 4095;
            ymmusic_divisors[note][bend] = divisor;
        }
    }
}

#define FIXED_CHANNELS 0
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
    // If that fails, try to replace a released note
    for (int i = FIXED_CHANNELS; i < YMMUSIC_NUMVOICES; i++)
    {
        if (ymmusic_voices[i].released)
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
    voice->released = false;

    if (channel == 15) {
        // Percussion channel
        if (note == 36) {
            voice->instrument = &bass_drum;
        } else if (note == 40) {
            voice->instrument = &electric_snare;
        } else {
            voice->instrument = &dummy_percussion;
        }
    } else if (ymmusic_channels[channel].instrument == 29) {
        voice->instrument = &overdriven_guitar;
    } else if (ymmusic_channels[channel].instrument == 30) {
        voice->instrument = &distortion_guitar;
    } else {
        voice->instrument = &dummy_instrument;
    }
    
    if (voice->instrument && voice->instrument->overrides_note) {
        voice->note = voice->instrument->override_note;
    }
}

static void ymmusic_release_note(unsigned char channel, unsigned char note)
{
    for (int i = 0; i < YMMUSIC_NUMVOICES; i++)
    {
        ymmusic_voice_t *voice = ymmusic_voices + i;
        if (voice->channel == channel && voice->note == note)
        {
            voice->ticks = 0;
            voice->released = true;
            break;
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

// Retrieves the value belonging to the current tick from an envelope.
static char ymmusic_envelope_value(envelope_t *env, unsigned short ticks, unsigned char released) {
    if (released) {
        // In release phase
        ticks += env->release_begin;
    } else if (ticks >= env->sustain_begin) {
        // In sustain phase
        unsigned short sustain_len = env->release_begin - env->sustain_begin;
        ticks -= env->sustain_begin;
        if ((sustain_len - 1) & sustain_len) {
            // sustain length is arbitrary number
            ticks %= sustain_len;
        } else {
            // sustain length is power of two
            ticks &= sustain_len - 1;
        }
        ticks += env->sustain_begin;
    }
    if (ticks > env->last) {
        ticks = env->last;
    }
    return env->data[ticks];
}

// Calculates the volume of a voice at the current tick.
static unsigned char ymmusic_voice_volume(ymmusic_voice_t *voice)
{
    unsigned char volume = ymmusic_channels[voice->channel].volume;
    if (voice->instrument && voice->instrument->volume_envelope) {
        envelope_t *env = voice->instrument->volume_envelope;
        volume += ymmusic_envelope_value(env, voice->ticks, voice->released);
    }
    if (volume > 127) volume = 127;
    return volume;
}

// Calculates the note of a voice at the current tick, in 128th of a note
static short ymmusic_voice_note(ymmusic_voice_t *voice)
{
    short note = voice->note << 7;
    note += (short)ymmusic_channels[voice->channel].pitch_bend - 128;
    if (voice->instrument && voice->instrument->note_envelope) {
        envelope_t *env = voice->instrument->note_envelope;
        note += ymmusic_envelope_value(env, voice->ticks, voice->released) << 7;
    }
    if (voice->instrument && voice->instrument->pitch_envelope) {
        envelope_t *env = voice->instrument->pitch_envelope;
        note += ymmusic_envelope_value(env, voice->ticks, voice->released);
    }
    if (note < 0) {
        note = 0;
    }
    return note;
}

// Determines whether a voice has finished playing.
static boolean ymmusic_voice_finished(ymmusic_voice_t *voice)
{
    if (voice->released)
    {
        // If there is a volume envelope, it controls how long the voice keeps playing after release.
        if (voice->instrument && voice->instrument->volume_envelope) {
            return voice->ticks > voice->instrument->volume_envelope->last;
        }
        // Otherwise, the voice is finished on release.
        return true;
    } else {
        // If there is a volume_envelope and it has no sustain cycle (release_begin > last), the voice ends on its own.
        if (voice->instrument && voice->instrument->volume_envelope
            && voice->instrument->volume_envelope->release_begin > voice->instrument->volume_envelope->last)
        {
            return voice->ticks > voice->instrument->volume_envelope->last;
        }
        // Otherwise, the voice does not end until released.
        return false;
    }
}

// Debug function to dump a human-readable transcript of the MUS file.
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

        if (ymmusic_voice_finished(voice)) {
            voice->ticks = 0xffff;
        } else {
            // Frequency in 128th of a note
            unsigned short note = ymmusic_voice_note(voice);

            short divisor = ymmusic_divisors[note >> 7][(note >> 3) & 15];

            // Push note to soundchip
            *pPsgSndCtrl = 0 + 2 * voice->ymidx;
            *pPsgSndData = divisor & 0xff;
            *pPsgSndCtrl = 1 + 2 * voice->ymidx;
            *pPsgSndData = divisor >> 8;

            // Amplitude
            unsigned char new_volume = (ymmusic_voice_volume(voice) >> 3) + snd_MusicVolume;
            if (new_volume > 15)
            {
                new_volume -= 15;
            }
            else
            {
                new_volume = 0;
            }
            
            // Push amplitude to soundchip
            *pPsgSndCtrl = 8 + voice->ymidx;
            if (*pPsgSndCtrl != new_volume)
            {
                *pPsgSndData = new_volume;
            }

            // Enable mixer
            if (voice->ticks == 0 && !voice->released)
            {
                // Note just pressed? Enable mixer for channel.
                *pPsgSndCtrl = 7;
                // Enable voice
                unsigned char data = *pPsgSndCtrl & ~(1 << voice->ymidx);
                // Enable or disable noise
                if (voice->instrument && voice->instrument->enables_noise) {
                    data &=  ~(8 << voice->ymidx);
                } else {
                    data |= 8 << voice->ymidx;
                }
                *pPsgSndData = data;
            } 

            voice->ticks++;
        }

        if (voice->ticks == 0xffff)
        {
            // Reaching end? Disable mixer for channel.
            *pPsgSndCtrl = 7;
            // 9 disables both voice and noise generator
            *pPsgSndData = *pPsgSndCtrl | (9 << voice->ymidx);
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
