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
// This data was generated using the 'ym_table.c' tool
static short ymmusic_divisors[128][16] = {
  { 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095},
  { 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095},
  { 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095},
  { 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095},
  { 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095},
  { 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095},
  { 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095},
  { 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095},
  { 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095},
  { 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095},
  { 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095},
  { 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095},
  { 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095},
  { 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095},
  { 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095},
  { 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095},
  { 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095},
  { 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095},
  { 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095},
  { 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095},
  { 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095},
  { 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095},
  { 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095},
  { 4050, 4065, 4079, 4094, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095},
  { 3823, 3837, 3850, 3864, 3878, 3892, 3906, 3921, 3935, 3949, 3963, 3978, 3992, 4006, 4021, 4035},
  { 3608, 3621, 3634, 3648, 3661, 3674, 3687, 3701, 3714, 3727, 3741, 3754, 3768, 3782, 3795, 3809},
  { 3406, 3418, 3430, 3443, 3455, 3468, 3480, 3493, 3506, 3518, 3531, 3544, 3557, 3569, 3582, 3595},
  { 3215, 3226, 3238, 3250, 3261, 3273, 3285, 3297, 3309, 3321, 3333, 3345, 3357, 3369, 3381, 3393},
  { 3034, 3045, 3056, 3067, 3078, 3089, 3101, 3112, 3123, 3134, 3146, 3157, 3169, 3180, 3191, 3203},
  { 2864, 2874, 2885, 2895, 2906, 2916, 2927, 2937, 2948, 2959, 2969, 2980, 2991, 3002, 3012, 3023},
  { 2703, 2713, 2723, 2733, 2743, 2752, 2762, 2772, 2782, 2793, 2803, 2813, 2823, 2833, 2843, 2854},
  { 2552, 2561, 2570, 2579, 2589, 2598, 2607, 2617, 2626, 2636, 2645, 2655, 2664, 2674, 2684, 2694},
  { 2408, 2417, 2426, 2435, 2443, 2452, 2461, 2470, 2479, 2488, 2497, 2506, 2515, 2524, 2533, 2542},
  { 2273, 2281, 2290, 2298, 2306, 2315, 2323, 2331, 2340, 2348, 2357, 2365, 2374, 2382, 2391, 2400},
  { 2146, 2153, 2161, 2169, 2177, 2185, 2193, 2201, 2209, 2217, 2225, 2233, 2241, 2249, 2257, 2265},
  { 2025, 2033, 2040, 2047, 2055, 2062, 2070, 2077, 2085, 2092, 2100, 2107, 2115, 2123, 2130, 2138},
  { 1912, 1919, 1925, 1932, 1939, 1946, 1953, 1961, 1968, 1975, 1982, 1989, 1996, 2003, 2011, 2018},
  { 1804, 1811, 1817, 1824, 1831, 1837, 1844, 1851, 1857, 1864, 1871, 1877, 1884, 1891, 1898, 1905},
  { 1703, 1709, 1715, 1722, 1728, 1734, 1740, 1747, 1753, 1759, 1766, 1772, 1779, 1785, 1791, 1798},
  { 1608, 1613, 1619, 1625, 1631, 1637, 1643, 1649, 1655, 1661, 1667, 1673, 1679, 1685, 1691, 1697},
  { 1517, 1523, 1528, 1534, 1539, 1545, 1551, 1556, 1562, 1567, 1573, 1579, 1585, 1590, 1596, 1602},
  { 1432, 1437, 1443, 1448, 1453, 1458, 1464, 1469, 1474, 1480, 1485, 1490, 1496, 1501, 1506, 1512},
  { 1352, 1357, 1362, 1367, 1372, 1376, 1381, 1386, 1391, 1397, 1402, 1407, 1412, 1417, 1422, 1427},
  { 1276, 1281, 1285, 1290, 1295, 1299, 1304, 1309, 1313, 1318, 1323, 1328, 1332, 1337, 1342, 1347},
  { 1204, 1209, 1213, 1218, 1222, 1226, 1231, 1235, 1240, 1244, 1249, 1253, 1258, 1262, 1267, 1271},
  { 1137, 1141, 1145, 1149, 1153, 1158, 1162, 1166, 1170, 1174, 1179, 1183, 1187, 1191, 1196, 1200},
  { 1073, 1077, 1081, 1085, 1089, 1093, 1097, 1101, 1105, 1109, 1113, 1117, 1121, 1125, 1129, 1133},
  { 1013, 1017, 1020, 1024, 1028, 1031, 1035, 1039, 1043, 1046, 1050, 1054, 1058, 1062, 1065, 1069},
  { 956, 960, 963, 966, 970, 973, 977, 981, 984, 988, 991, 995, 998, 1002, 1006, 1009},
  { 902, 906, 909, 912, 916, 919, 922, 926, 929, 932, 936, 939, 942, 946, 949, 953},
  { 852, 855, 858, 861, 864, 867, 870, 874, 877, 880, 883, 886, 890, 893, 896, 899},
  { 804, 807, 810, 813, 816, 819, 822, 825, 828, 831, 834, 837, 840, 843, 846, 849},
  { 759, 762, 764, 767, 770, 773, 776, 778, 781, 784, 787, 790, 793, 795, 798, 801},
  { 716, 719, 722, 724, 727, 729, 732, 735, 737, 740, 743, 745, 748, 751, 753, 756},
  { 676, 679, 681, 684, 686, 688, 691, 693, 696, 699, 701, 704, 706, 709, 711, 714},
  { 638, 641, 643, 645, 648, 650, 652, 655, 657, 659, 662, 664, 666, 669, 671, 674},
  { 602, 605, 607, 609, 611, 613, 616, 618, 620, 622, 625, 627, 629, 631, 634, 636},
  { 569, 571, 573, 575, 577, 579, 581, 583, 585, 587, 590, 592, 594, 596, 598, 600},
  { 537, 539, 541, 543, 545, 547, 549, 551, 553, 555, 557, 559, 561, 563, 565, 567},
  { 507, 509, 510, 512, 514, 516, 518, 520, 522, 523, 525, 527, 529, 531, 533, 535},
  { 478, 480, 482, 483, 485, 487, 489, 491, 492, 494, 496, 498, 499, 501, 503, 505},
  { 451, 453, 455, 456, 458, 460, 461, 463, 465, 466, 468, 470, 471, 473, 475, 477},
  { 426, 428, 429, 431, 432, 434, 435, 437, 439, 440, 442, 443, 445, 447, 448, 450},
  { 402, 404, 405, 407, 408, 410, 411, 413, 414, 416, 417, 419, 420, 422, 423, 425},
  { 380, 381, 382, 384, 385, 387, 388, 389, 391, 392, 394, 395, 397, 398, 399, 401},
  { 358, 360, 361, 362, 364, 365, 366, 368, 369, 370, 372, 373, 374, 376, 377, 378},
  { 338, 340, 341, 342, 343, 344, 346, 347, 348, 350, 351, 352, 353, 355, 356, 357},
  { 319, 321, 322, 323, 324, 325, 326, 328, 329, 330, 331, 332, 333, 335, 336, 337},
  { 301, 303, 304, 305, 306, 307, 308, 309, 310, 311, 313, 314, 315, 316, 317, 318},
  { 285, 286, 287, 288, 289, 290, 291, 292, 293, 294, 295, 296, 297, 298, 299, 300},
  { 269, 270, 271, 272, 273, 274, 275, 276, 277, 278, 279, 280, 281, 282, 283, 284},
  { 254, 255, 255, 256, 257, 258, 259, 260, 261, 262, 263, 264, 265, 266, 267, 268},
  { 239, 240, 241, 242, 243, 244, 245, 246, 246, 247, 248, 249, 250, 251, 252, 253},
  { 226, 227, 228, 228, 229, 230, 231, 232, 233, 233, 234, 235, 236, 237, 238, 239},
  { 213, 214, 215, 216, 216, 217, 218, 219, 220, 220, 221, 222, 223, 224, 224, 225},
  { 201, 202, 203, 204, 204, 205, 206, 207, 207, 208, 209, 210, 210, 211, 212, 213},
  { 190, 191, 191, 192, 193, 194, 194, 195, 196, 196, 197, 198, 199, 199, 200, 201},
  { 179, 180, 181, 181, 182, 183, 183, 184, 185, 185, 186, 187, 187, 188, 189, 189},
  { 169, 170, 171, 171, 172, 172, 173, 174, 174, 175, 176, 176, 177, 178, 178, 179},
  { 160, 161, 161, 162, 162, 163, 163, 164, 165, 165, 166, 166, 167, 168, 168, 169},
  { 151, 152, 152, 153, 153, 154, 154, 155, 155, 156, 157, 157, 158, 158, 159, 159},
  { 143, 143, 144, 144, 145, 145, 146, 146, 147, 147, 148, 148, 149, 149, 150, 150},
  { 135, 135, 136, 136, 137, 137, 138, 138, 139, 139, 140, 140, 141, 141, 142, 142},
  { 127, 128, 128, 128, 129, 129, 130, 130, 131, 131, 132, 132, 133, 133, 134, 134},
  { 120, 120, 121, 121, 122, 122, 123, 123, 123, 124, 124, 125, 125, 126, 126, 127},
  { 113, 114, 114, 114, 115, 115, 116, 116, 117, 117, 117, 118, 118, 119, 119, 120},
  { 107, 107, 108, 108, 108, 109, 109, 110, 110, 110, 111, 111, 112, 112, 112, 113},
  { 101, 101, 102, 102, 102, 103, 103, 104, 104, 104, 105, 105, 105, 106, 106, 107},
  { 95, 96, 96, 96, 97, 97, 97, 98, 98, 98, 99, 99, 100, 100, 100, 101},
  { 90, 90, 91, 91, 91, 92, 92, 92, 93, 93, 93, 94, 94, 94, 95, 95},
  { 85, 85, 86, 86, 86, 86, 87, 87, 87, 88, 88, 88, 89, 89, 89, 90},
  { 80, 81, 81, 81, 81, 82, 82, 82, 83, 83, 83, 83, 84, 84, 84, 85},
  { 76, 76, 76, 77, 77, 77, 77, 78, 78, 78, 79, 79, 79, 79, 80, 80},
  { 72, 72, 72, 72, 73, 73, 73, 73, 74, 74, 74, 74, 75, 75, 75, 75},
  { 68, 68, 68, 68, 69, 69, 69, 69, 70, 70, 70, 70, 71, 71, 71, 71},
  { 64, 64, 64, 64, 65, 65, 65, 65, 66, 66, 66, 66, 67, 67, 67, 67},
  { 60, 60, 61, 61, 61, 61, 62, 62, 62, 62, 62, 63, 63, 63, 63, 64},
  { 57, 57, 57, 57, 58, 58, 58, 58, 59, 59, 59, 59, 59, 60, 60, 60},
  { 54, 54, 54, 54, 54, 55, 55, 55, 55, 55, 56, 56, 56, 56, 56, 57},
  { 51, 51, 51, 51, 51, 52, 52, 52, 52, 52, 53, 53, 53, 53, 53, 54},
  { 48, 48, 48, 48, 49, 49, 49, 49, 49, 49, 50, 50, 50, 50, 50, 51},
  { 45, 45, 46, 46, 46, 46, 46, 46, 47, 47, 47, 47, 47, 47, 48, 48},
  { 43, 43, 43, 43, 43, 43, 44, 44, 44, 44, 44, 44, 45, 45, 45, 45},
  { 40, 41, 41, 41, 41, 41, 41, 41, 42, 42, 42, 42, 42, 42, 42, 43},
  { 38, 38, 38, 39, 39, 39, 39, 39, 39, 39, 40, 40, 40, 40, 40, 40},
  { 36, 36, 36, 36, 37, 37, 37, 37, 37, 37, 37, 37, 38, 38, 38, 38},
  { 34, 34, 34, 34, 35, 35, 35, 35, 35, 35, 35, 35, 36, 36, 36, 36},
  { 32, 32, 32, 32, 33, 33, 33, 33, 33, 33, 33, 33, 34, 34, 34, 34},
  { 30, 30, 31, 31, 31, 31, 31, 31, 31, 31, 31, 32, 32, 32, 32, 32},
  { 29, 29, 29, 29, 29, 29, 29, 29, 30, 30, 30, 30, 30, 30, 30, 30},
  { 27, 27, 27, 27, 27, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 29},
  { 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 27, 27, 27, 27, 27, 27},
  { 24, 24, 24, 24, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 26},
  { 23, 23, 23, 23, 23, 23, 23, 23, 24, 24, 24, 24, 24, 24, 24, 24},
  { 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 23, 23, 23, 23},
  { 20, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 22},
  { 19, 19, 19, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20},
  { 18, 18, 18, 18, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19},
  { 17, 17, 17, 17, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18},
  { 16, 16, 16, 16, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17},
  { 15, 15, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16},
  { 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15},
  { 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 15},
  { 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 14, 14, 14, 14, 14, 14},
  { 12, 12, 12, 12, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13},
  { 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12},
  { 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 12, 12, 12, 12},
  { 10, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11},
};

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
