#include <mint/osbind.h>
#include "atari_c2p.h"
#include "r_main.h"
#include "w_wad.h"
#include "z_zone.h"
#include "doomdef.h"

// Set to 1 to use grayscale medium resolution (640x200) rendering.
#define USE_MIDRES 0

#if !USE_MIDRES
// Subset of DOOM colors to use for Atari palette
const unsigned char subset[] = 
    {0, 90, 101, 241, 202, 252, 38, 219, 144, 136, 158, 120, 72, 58, 249, 4};

// [DOOM color 0..255][weight of ST color 0..16]
static unsigned char mix_weights[256][16] = {
{ 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,}, // 0: 0.000000
{ 13, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0,}, // 1: 1.071797
{ 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,}, // 2: 0.956298
{ 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 12, 0, 0, 0, 0, 0,}, // 3: 1.333179
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16,}, // 4: 0.000000
{ 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0,}, // 5: 0.926619
{ 14, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0,}, // 6: 0.832113
{ 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,}, // 7: 0.455137
{ 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,}, // 8: 0.220837
{ 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10, 0, 0, 0, 0, 0,}, // 9: 0.895823
{ 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0,}, // 10: 0.943190
{ 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0,}, // 11: 0.901076
{ 14, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0,}, // 12: 0.886246
{ 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 12, 0, 0, 0, 0, 0,}, // 13: 0.792451
{ 3, 0, 0, 0, 0, 0, 4, 0, 0, 0, 9, 0, 0, 0, 0, 0,}, // 14: 1.327025
{ 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0,}, // 15: 1.297141
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 11, 0, 5,}, // 16: 5.175427
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 13, 0, 3,}, // 17: 5.070978
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16, 0, 0,}, // 18: 3.740315
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16, 0, 0,}, // 19: 3.397381
{ 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 13, 0, 0,}, // 20: 2.724434
{ 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 11, 0, 0,}, // 21: 3.351038
{ 0, 0, 0, 0, 0, 4, 0, 4, 0, 0, 0, 0, 0, 8, 0, 0,}, // 22: 3.785924
{ 0, 5, 0, 0, 0, 0, 0, 11, 0, 0, 0, 0, 0, 0, 0, 0,}, // 23: 3.668197
{ 0, 4, 0, 0, 0, 3, 0, 9, 0, 0, 0, 0, 0, 0, 0, 0,}, // 24: 2.975642
{ 0, 0, 0, 0, 0, 3, 0, 8, 5, 0, 0, 0, 0, 0, 0, 0,}, // 25: 2.551214
{ 0, 0, 0, 0, 0, 3, 0, 8, 0, 5, 0, 0, 0, 0, 0, 0,}, // 26: 2.586681
{ 0, 0, 0, 0, 0, 3, 0, 8, 0, 5, 0, 0, 0, 0, 0, 0,}, // 27: 2.488141
{ 0, 0, 0, 0, 0, 3, 0, 7, 0, 0, 0, 0, 6, 0, 0, 0,}, // 28: 2.513363
{ 0, 0, 0, 0, 0, 3, 0, 6, 0, 0, 0, 0, 7, 0, 0, 0,}, // 29: 3.194356
{ 0, 0, 0, 0, 0, 0, 8, 8, 0, 0, 0, 0, 0, 0, 0, 0,}, // 30: 3.200888
{ 0, 0, 0, 0, 0, 0, 9, 7, 0, 0, 0, 0, 0, 0, 0, 0,}, // 31: 2.933251
{ 0, 0, 0, 0, 0, 0, 11, 5, 0, 0, 0, 0, 0, 0, 0, 0,}, // 32: 2.586370
{ 0, 0, 0, 0, 0, 0, 11, 5, 0, 0, 0, 0, 0, 0, 0, 0,}, // 33: 2.521603
{ 0, 0, 0, 0, 0, 0, 13, 3, 0, 0, 0, 0, 0, 0, 0, 0,}, // 34: 2.409303
{ 0, 0, 0, 0, 0, 0, 13, 3, 0, 0, 0, 0, 0, 0, 0, 0,}, // 35: 2.273410
{ 0, 0, 0, 0, 0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0,}, // 36: 1.836479
{ 0, 0, 0, 0, 0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0,}, // 37: 1.134662
{ 0, 0, 0, 0, 0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0,}, // 38: 0.000000
{ 0, 0, 0, 0, 0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0,}, // 39: 0.640480
{ 3, 0, 0, 0, 0, 0, 13, 0, 0, 0, 0, 0, 0, 0, 0, 0,}, // 40: 1.145095
{ 4, 0, 0, 0, 0, 0, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0,}, // 41: 1.291945
{ 5, 0, 0, 0, 0, 0, 11, 0, 0, 0, 0, 0, 0, 0, 0, 0,}, // 42: 1.349836
{ 6, 0, 0, 0, 0, 0, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0,}, // 43: 1.279766
{ 7, 0, 0, 0, 0, 0, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0,}, // 44: 1.239212
{ 8, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0,}, // 45: 1.300995
{ 9, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0,}, // 46: 1.262678
{ 10, 0, 0, 0, 0, 0, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0,}, // 47: 1.181943
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 12,}, // 48: 3.429480
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 11,}, // 49: 3.677174
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 0, 9,}, // 50: 3.976376
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 0, 8,}, // 51: 4.036829
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 9, 0, 7,}, // 52: 4.279774
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10, 0, 6,}, // 53: 4.411775
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 11, 0, 5,}, // 54: 4.677157
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 12, 0, 4,}, // 55: 4.758358
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 11, 5, 0,}, // 56: 3.863176
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 14, 2, 0,}, // 57: 2.715494
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16, 0, 0,}, // 58: 0.000000
{ 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 13, 0, 0,}, // 59: 2.295243
{ 0, 2, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 9, 0, 0,}, // 60: 2.703692
{ 0, 0, 0, 0, 0, 0, 0, 4, 4, 0, 0, 0, 0, 8, 0, 0,}, // 61: 2.475757
{ 0, 0, 0, 0, 0, 0, 0, 6, 5, 0, 0, 0, 0, 5, 0, 0,}, // 62: 2.554035
{ 0, 0, 0, 0, 0, 0, 0, 6, 6, 0, 0, 0, 0, 4, 0, 0,}, // 63: 2.343091
{ 0, 0, 0, 0, 0, 0, 0, 6, 8, 0, 0, 0, 0, 2, 0, 0,}, // 64: 2.267649
{ 0, 0, 0, 0, 0, 0, 0, 6, 10, 0, 0, 0, 0, 0, 0, 0,}, // 65: 1.652324
{ 0, 0, 0, 0, 0, 0, 0, 6, 4, 6, 0, 0, 0, 0, 0, 0,}, // 66: 1.748492
{ 0, 0, 0, 0, 0, 0, 0, 6, 0, 10, 0, 0, 0, 0, 0, 0,}, // 67: 1.680201
{ 0, 0, 9, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 0,}, // 68: 1.897540
{ 0, 0, 0, 0, 0, 0, 0, 3, 0, 6, 0, 0, 7, 0, 0, 0,}, // 69: 1.855619
{ 0, 0, 0, 0, 0, 0, 0, 2, 0, 3, 0, 0, 11, 0, 0, 0,}, // 70: 1.717632
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 13, 0, 0, 0,}, // 71: 1.190802
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16, 0, 0, 0,}, // 72: 0.000000
{ 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 4, 0, 10, 0, 0, 0,}, // 73: 1.056909
{ 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 8, 0, 5, 0, 0, 0,}, // 74: 1.143480
{ 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 11, 0, 0, 0, 0, 0,}, // 75: 0.990608
{ 2, 0, 0, 0, 0, 0, 4, 0, 0, 0, 10, 0, 0, 0, 0, 0,}, // 76: 1.383569
{ 5, 0, 0, 0, 0, 0, 3, 0, 0, 0, 8, 0, 0, 0, 0, 0,}, // 77: 1.409117
{ 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0,}, // 78: 1.342925
{ 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 0, 0,}, // 79: 1.220895
{ 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 13,}, // 80: 3.447241
{ 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 11,}, // 81: 2.901400
{ 0, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 9,}, // 82: 3.679514
{ 0, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 9,}, // 83: 3.418444
{ 0, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7,}, // 84: 3.088181
{ 0, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6,}, // 85: 3.484154
{ 0, 11, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5,}, // 86: 2.990328
{ 0, 13, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3,}, // 87: 3.680284
{ 0, 14, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2,}, // 88: 2.907619
{ 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,}, // 89: 2.785724
{ 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,}, // 90: 0.000000
{ 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,}, // 91: 1.364029
{ 0, 13, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,}, // 92: 1.853908
{ 0, 11, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,}, // 93: 1.890917
{ 0, 10, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,}, // 94: 1.957156
{ 0, 8, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,}, // 95: 2.146028
{ 0, 6, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,}, // 96: 2.411509
{ 0, 6, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,}, // 97: 2.316760
{ 0, 4, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,}, // 98: 1.932855
{ 0, 0, 12, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0,}, // 99: 1.996055
{ 0, 0, 10, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 0, 0, 0,}, // 100: 1.599108
{ 0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,}, // 101: 0.000000
{ 0, 0, 12, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0,}, // 102: 0.880436
{ 0, 0, 10, 0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 0, 0,}, // 103: 0.992895
{ 0, 0, 6, 0, 0, 0, 0, 0, 0, 0, 10, 0, 0, 0, 0, 0,}, // 104: 1.214676
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16, 0, 0, 0, 0, 0,}, // 105: 1.434212
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16, 0, 0, 0, 0, 0,}, // 106: 1.021929
{ 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 13, 0, 0, 0, 0, 0,}, // 107: 1.110718
{ 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 11, 0, 0, 0, 0, 0,}, // 108: 0.974324
{ 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 9, 0, 0, 0, 0, 0,}, // 109: 1.138157
{ 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0,}, // 110: 1.090376
{ 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 0, 0,}, // 111: 1.040065
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 9,}, // 112: 16.614658
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 8,}, // 113: 14.712250
{ 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,}, // 114: 12.200306
{ 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,}, // 115: 10.746216
{ 0, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 0,}, // 116: 9.007668
{ 0, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 9, 0, 0, 0, 0,}, // 117: 6.848719
{ 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 12, 0, 0, 0, 0,}, // 118: 4.901704
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16, 0, 0, 0, 0,}, // 119: 3.436837
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16, 0, 0, 0, 0,}, // 120: 0.000000
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 11, 0, 0, 0, 0,}, // 121: 1.406646
{ 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10, 0, 0, 0, 0,}, // 122: 1.705639
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16, 0, 0, 0, 0, 0,}, // 123: 1.855249
{ 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 5, 0, 0, 0, 0,}, // 124: 1.877111
{ 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0,}, // 125: 1.531078
{ 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0,}, // 126: 1.073595
{ 14, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0,}, // 127: 0.908961
{ 0, 11, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0,}, // 128: 2.154619
{ 0, 7, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 4, 0, 0,}, // 129: 2.400809
{ 0, 6, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 2, 0, 0,}, // 130: 2.310873
{ 0, 5, 0, 0, 0, 0, 0, 0, 11, 0, 0, 0, 0, 0, 0, 0,}, // 131: 1.665370
{ 0, 0, 0, 0, 0, 0, 0, 0, 16, 0, 0, 0, 0, 0, 0, 0,}, // 132: 1.244718
{ 0, 0, 0, 0, 0, 0, 0, 0, 16, 0, 0, 0, 0, 0, 0, 0,}, // 133: 1.115634
{ 0, 0, 0, 0, 0, 0, 0, 0, 10, 6, 0, 0, 0, 0, 0, 0,}, // 134: 0.947659
{ 0, 0, 0, 0, 0, 0, 0, 0, 5, 11, 0, 0, 0, 0, 0, 0,}, // 135: 0.721692
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 16, 0, 0, 0, 0, 0, 0,}, // 136: 0.000000
{ 0, 0, 3, 0, 0, 0, 0, 0, 0, 9, 0, 0, 4, 0, 0, 0,}, // 137: 0.868438
{ 0, 0, 4, 0, 0, 0, 0, 0, 0, 6, 0, 0, 6, 0, 0, 0,}, // 138: 0.802896
{ 0, 0, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10, 0, 0, 0,}, // 139: 0.477037
{ 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 3, 0, 8, 0, 0, 0,}, // 140: 1.084529
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 0, 8, 0, 0, 0,}, // 141: 1.005164
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 11, 0, 5, 0, 0, 0,}, // 142: 1.258128
{ 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 12, 0, 0, 0, 0, 0,}, // 143: 1.100123
{ 0, 0, 0, 0, 0, 0, 0, 0, 16, 0, 0, 0, 0, 0, 0, 0,}, // 144: 0.000000
{ 0, 0, 0, 0, 0, 0, 0, 0, 7, 9, 0, 0, 0, 0, 0, 0,}, // 145: 1.072868
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 16, 0, 0, 0, 0, 0, 0,}, // 146: 0.660511
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 9, 0, 0, 0,}, // 147: 1.114002
{ 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 4, 0, 9, 0, 0, 0,}, // 148: 1.175253
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 9, 0, 7, 0, 0, 0,}, // 149: 0.953383
{ 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 12, 0, 0, 0, 0, 0,}, // 150: 0.846146
{ 4, 0, 0, 0, 0, 0, 3, 0, 0, 0, 9, 0, 0, 0, 0, 0,}, // 151: 1.319937
{ 0, 0, 0, 0, 0, 0, 0, 0, 9, 0, 0, 7, 0, 0, 0, 0,}, // 152: 2.085407
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 11, 0, 5, 0, 0, 0, 0,}, // 153: 1.182728
{ 0, 0, 9, 0, 0, 0, 0, 0, 0, 4, 0, 3, 0, 0, 0, 0,}, // 154: 1.287859
{ 0, 0, 8, 0, 0, 0, 3, 0, 0, 0, 0, 5, 0, 0, 0, 0,}, // 155: 1.618167
{ 0, 0, 8, 0, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0,}, // 156: 1.310739
{ 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 13, 0, 0, 0, 0, 0,}, // 157: 0.954865
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16, 0, 0, 0, 0, 0,}, // 158: 0.000000
{ 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 13, 0, 0, 0, 0, 0,}, // 159: 0.775682
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10, 6,}, // 160: 5.819015
{ 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 12, 0,}, // 161: 2.960200
{ 0, 0, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 8, 0,}, // 162: 4.081151
{ 0, 0, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 8, 0, 0,}, // 163: 4.397435
{ 0, 0, 0, 0, 0, 0, 0, 6, 10, 0, 0, 0, 0, 0, 0, 0,}, // 164: 2.661159
{ 0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 0, 10, 0, 0, 0,}, // 165: 2.064728
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16, 0, 0, 0,}, // 166: 2.436373
{ 0, 0, 0, 0, 0, 0, 13, 0, 0, 0, 0, 0, 3, 0, 0, 0,}, // 167: 1.147461
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16,}, // 168: 0.000000
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 0, 10,}, // 169: 3.893218
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 11, 0, 5,}, // 170: 4.953310
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16, 0, 0,}, // 171: 3.789859
{ 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 11, 0, 0,}, // 172: 5.930332
{ 0, 0, 0, 0, 0, 0, 0, 11, 0, 0, 0, 0, 0, 5, 0, 0,}, // 173: 7.035384
{ 0, 0, 0, 0, 0, 0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0,}, // 174: 6.880080
{ 0, 0, 0, 0, 0, 0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0,}, // 175: 8.339644
{ 0, 0, 0, 0, 0, 0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0,}, // 176: 9.516675
{ 0, 0, 0, 0, 0, 0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0,}, // 177: 8.796343
{ 0, 0, 0, 0, 0, 0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0,}, // 178: 8.883517
{ 0, 0, 0, 0, 0, 0, 6, 10, 0, 0, 0, 0, 0, 0, 0, 0,}, // 179: 7.986820
{ 0, 0, 0, 0, 0, 0, 7, 9, 0, 0, 0, 0, 0, 0, 0, 0,}, // 180: 7.142746
{ 0, 0, 0, 0, 0, 0, 9, 7, 0, 0, 0, 0, 0, 0, 0, 0,}, // 181: 6.338595
{ 0, 0, 0, 0, 0, 0, 10, 6, 0, 0, 0, 0, 0, 0, 0, 0,}, // 182: 5.596178
{ 0, 0, 0, 0, 0, 0, 12, 4, 0, 0, 0, 0, 0, 0, 0, 0,}, // 183: 4.896016
{ 0, 0, 0, 0, 0, 0, 13, 3, 0, 0, 0, 0, 0, 0, 0, 0,}, // 184: 4.207591
{ 0, 0, 0, 0, 0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0,}, // 185: 2.403677
{ 0, 0, 0, 0, 0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0,}, // 186: 1.235740
{ 0, 0, 0, 0, 0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0,}, // 187: 1.371050
{ 4, 0, 0, 0, 0, 0, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0,}, // 188: 1.564400
{ 6, 0, 0, 0, 0, 0, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0,}, // 189: 1.427190
{ 8, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0,}, // 190: 1.300995
{ 10, 0, 0, 0, 0, 0, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0,}, // 191: 1.181943
{ 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 12,}, // 192: 5.055549
{ 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 11,}, // 193: 7.155777
{ 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 9,}, // 194: 8.179933
{ 0, 0, 0, 0, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7,}, // 195: 8.808949
{ 0, 0, 0, 0, 11, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5,}, // 196: 9.081787
{ 0, 5, 0, 0, 11, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,}, // 197: 8.766707
{ 0, 3, 0, 0, 13, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,}, // 198: 8.339449
{ 0, 0, 0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,}, // 199: 5.975742
{ 0, 0, 0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,}, // 200: 5.338061
{ 0, 0, 0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,}, // 201: 2.373141
{ 0, 0, 0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,}, // 202: 0.000000
{ 0, 0, 0, 4, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,}, // 203: 1.079432
{ 0, 0, 0, 7, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,}, // 204: 0.906142
{ 0, 0, 0, 10, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,}, // 205: 0.898699
{ 0, 0, 0, 13, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,}, // 206: 1.068732
{ 0, 0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,}, // 207: 0.640728
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16,}, // 208: 0.000000
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 12,}, // 209: 3.564189
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 0, 8,}, // 210: 4.394603
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 9, 3, 4,}, // 211: 4.814260
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 11, 5, 0,}, // 212: 3.585161
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16, 0, 0,}, // 213: 3.314802
{ 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 12, 0, 0,}, // 214: 5.676889
{ 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 9, 0, 0,}, // 215: 6.480447
{ 0, 0, 0, 0, 0, 0, 0, 10, 0, 0, 0, 0, 0, 6, 0, 0,}, // 216: 4.926618
{ 0, 0, 0, 0, 0, 0, 0, 12, 0, 0, 0, 0, 0, 4, 0, 0,}, // 217: 3.920170
{ 0, 0, 0, 0, 0, 0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0,}, // 218: 2.242744
{ 0, 0, 0, 0, 0, 0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0,}, // 219: 0.000000
{ 0, 0, 0, 0, 0, 0, 2, 14, 0, 0, 0, 0, 0, 0, 0, 0,}, // 220: 1.981120
{ 0, 0, 0, 0, 0, 0, 4, 12, 0, 0, 0, 0, 0, 0, 0, 0,}, // 221: 1.968149
{ 0, 0, 0, 0, 0, 0, 6, 10, 0, 0, 0, 0, 0, 0, 0, 0,}, // 222: 2.022903
{ 0, 0, 0, 0, 0, 0, 7, 9, 0, 0, 0, 0, 0, 0, 0, 0,}, // 223: 2.182865
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16,}, // 224: 0.000000
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 13,}, // 225: 3.005310
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 10,}, // 226: 4.068863
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 9, 7,}, // 227: 5.092631
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 11, 5,}, // 228: 5.994957
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16, 0,}, // 229: 6.628020
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16, 0,}, // 230: 6.396210
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16, 0,}, // 231: 6.412309
{ 0, 0, 0, 0, 0, 0, 9, 7, 0, 0, 0, 0, 0, 0, 0, 0,}, // 232: 2.022156
{ 0, 0, 0, 0, 0, 0, 10, 6, 0, 0, 0, 0, 0, 0, 0, 0,}, // 233: 1.984484
{ 0, 0, 0, 0, 0, 0, 12, 4, 0, 0, 0, 0, 0, 0, 0, 0,}, // 234: 1.916203
{ 0, 0, 0, 0, 0, 0, 14, 2, 0, 0, 0, 0, 0, 0, 0, 0,}, // 235: 2.018483
{ 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 12, 0, 0, 0, 0, 0,}, // 236: 0.790128
{ 5, 0, 0, 0, 0, 0, 4, 0, 0, 0, 7, 0, 0, 0, 0, 0,}, // 237: 1.343363
{ 8, 0, 0, 0, 0, 0, 3, 0, 0, 0, 5, 0, 0, 0, 0, 0,}, // 238: 1.442368
{ 13, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0,}, // 239: 1.466571
{ 0, 0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,}, // 240: 0.640728
{ 0, 0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,}, // 241: 0.000000
{ 4, 0, 0, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,}, // 242: 0.206658
{ 8, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,}, // 243: 0.228210
{ 11, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,}, // 244: 0.214072
{ 13, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,}, // 245: 0.240497
{ 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,}, // 246: 0.114238
{ 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,}, // 247: 0.000000
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16, 0, 0,}, // 248: 4.088013
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16, 0,}, // 249: 0.000000
{ 0, 0, 0, 0, 0, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6,}, // 250: 9.552780
{ 0, 0, 0, 0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,}, // 251: 10.018232
{ 0, 0, 0, 0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,}, // 252: 0.000000
{ 6, 0, 0, 0, 0, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,}, // 253: 2.779808
{ 10, 0, 0, 0, 0, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,}, // 254: 2.503329
{ 0, 0, 0, 0, 0, 3, 0, 0, 9, 4, 0, 0, 0, 0, 0, 0,}, // 255: 2.857005
};
#endif

// C2P table for full resolution
// [phase 0..3][color 0..255][pixel 0..7]
static unsigned long c2p_table[4][256][8];

// C2P table for half resolution (2x2 pixels)
// [phase 0..3][color 0..255][pixel 0..3]
static unsigned long c2p_2x_table[4][256][4];

// C2P table for quarter resolution (4x4 pixels)
// [phase 0..3][color 0..255][pixel 0..1]
static unsigned long c2p_4x_table[4][256][2];

static unsigned short convert_channel(unsigned char v) {
    unsigned short r = (v & 0xe0) >> 5; // Bits 7,6,5 shifted to 2,1,0
    r |= (v & 0x10) >> 1; // STe color bit
    return r;
}
static unsigned short stcolor(unsigned char r, unsigned char g, unsigned char b) {
    unsigned short entry = convert_channel(r);
    entry <<= 4;
    entry |= convert_channel(g);
    entry <<= 4;
    entry |= convert_channel(b);
    return entry;
}

void install_palette(const unsigned short *palette) {
    volatile unsigned short *reg = (unsigned short*) 0xff8240;
#if USE_MIDRES
    short numColors = 4;
#else
    short numColors = 16;
#endif
    for (short n=0; n<numColors; n++) *reg++ = *palette++;
}

void save_palette(unsigned short *palette) {
    volatile unsigned short *reg = (unsigned short*) 0xff8240;
#if USE_MIDRES
    short numColors = 4;
#else
    short numColors = 16;
#endif
    for (short n=0; n<numColors; n++) *palette++ = *reg++;
}


/// @brief Computes a movep-compatible Bayer-dithered pixel given a set of mixing weights.
/// @param weights An array of mixing weights, each weight corresponding to a palette color. Must sum up to 16.
/// @param phase The vertical phase within the bayer pattern (0..3).
/// @param px The pixel within the group of 8 pixels covered by a movep-DWORD (0..7).
/// @return A DWORD that can be written into an Atari ST lo-res framebuffer using movep.l.
unsigned long bayer4_pdata(const unsigned char *weights, short phase, short px) {
	unsigned char bayer[4][4] = {
		{0,  8, 2,10},
		{12, 4,14, 6},
		{ 3,11, 1, 9},
		{15, 7,13, 5}
	};
    // Find the ST palette color index to fill the pixel with.
    unsigned char bayer_lwb = 0, bayer_upb = 0;
    short c;
    for (c=0; c<16; c++) {
        bayer_upb += weights[c];
        if (bayer[phase][px%4] >= bayer_lwb && bayer[phase][px%4] < bayer_upb) {
            break; // Search for color is finished.
        }
        bayer_lwb += weights[c];
    }
    // Compute a pdata-compatible pixel representation.
    unsigned long pdata = 0;
    if (c & 1) pdata |= 0x01000000;
    if (c & 2) pdata |= 0x00010000;
    if (c & 4) pdata |= 0x00000100;
    if (c & 8) pdata |= 0x00000001;
    return pdata << (7-px);
}

void init_c2p_table() {
    set_doom_palette(W_CacheLumpName("PLAYPAL", PU_CACHE));
#if USE_MIDRES
	unsigned short bayer[4][4] = {
		{0,  8, 2,10},
		{12, 4,14, 6},
		{ 3,11, 1, 9},
		{15, 7,13, 5}
	};
    unsigned short stpalette[] = {stcolor(0,0,0), stcolor(85,85,85), stcolor(170,170,170), stcolor(255,255,255)};
    install_palette(stpalette);

    unsigned char* palette = W_CacheLumpName("PLAYPAL", PU_CACHE);
    for (int i=0; i<256; i++) {
        unsigned short r=palette[3*i], g=palette[3*i+1], b=palette[3*i+2];
        // printf("C %d: %d %d %d\n", i, r, g, b);
        // calculate brightness (0..765)
        unsigned short l = r+g+b;
        // calculate weights (0..255) of color indices 0, 1, 2, 3
        unsigned short w[] = {
            l < 256 ? 255 - l : 0,
            l < 256 ? l : (l < 512 ? 511 - l : 0),
            l < 256 ? 0 : (l < 512 ? l - 256 : 767 - l),
            l < 512 ? 0 : (l - 512),
        };
        // make weights cumulative
        for (int j=0; j<3; j++) w[j+1] += w[j];
        // renormalize weights into bayer range 0..15
        // for (int j=0; j<4; j++) w[j] >>= 4;
        // printf("  w: %2d %2d %2d %2d\n", w[0], w[1], w[2], w[3]);

        for (int phase=0; phase<4; phase++) {
            // iterate over 8 consecutive input pixels
            unsigned long combined_pdata = 0;
            for (int px = 0; px < 8; px++) {
                unsigned long pdata = 0;
                // per input pixel, iterate over two output pixels
                for (int opx = 2*px; opx < 2*px+2; opx++) {
                    // Fetch bayer threshold for output pixel.
                    unsigned short bayer_w = (bayer[phase][opx % 4] << 4) + 7;
                    // Iterate four possible output colors.
                    // Select first one that has cumulative weight >= bayer threshold.
                    // Apply bits to pixel data in opx position.
                    for (int oc = 0; oc < 4; oc++) {
                        if (oc < 3 && w[oc] < bayer_w) continue;
                        if (oc & 1) pdata |= 0x80000000 >> opx;
                        if (oc & 2) pdata |= 0x00008000 >> opx;
                        break;
                    }
                }
                c2p_table[phase][i][px] = pdata;
                combined_pdata |= pdata;
            }
        }
    }
#else
	for (int i=0; i<256; i++) {
        unsigned char *weights = mix_weights[i];
        for (int phase=0; phase<4; phase++) {
            // Fill 1x1 c2p table
            for (int px=0; px<8; px++) {
                c2p_table[phase][i][px] = bayer4_pdata(weights, phase, px);
            }
            // Fill 2x2 c2p table
            for (int ipx=0; ipx<4; ipx++) {
                unsigned long ipx_pdata = 0;
                for (int opx=2*ipx; opx<2*ipx+2; opx++) {
                    ipx_pdata |= bayer4_pdata(weights, phase, opx);
                }
                c2p_2x_table[phase][i][ipx] = ipx_pdata;
            }
            // Fill 4x4 c2p table
            for (int ipx=0; ipx<2; ipx++) {
                unsigned long ipx_pdata = 0;
                for (int opx=4*ipx; opx<4*ipx+4; opx++) {
                    ipx_pdata |= bayer4_pdata(weights, phase, opx);
                }
                c2p_4x_table[phase][i][ipx] = ipx_pdata;
            }
        }
	}
#endif
}

static void c2p(register unsigned char *out, const unsigned char *in, unsigned short pixels, unsigned long table[][8]) {
#if USE_MIDRES
	while (pixels > 7) {
		register unsigned long pdata = 0;
        for(int i=0; i<8; i++) {
            pdata |= table[*in++][i];
        }
        *(unsigned long*)out = pdata;
		pixels -= 8;
		out += 4;
	}
#else
    if (pixels < 16) return;
    unsigned short groups = pixels / 16 - 1;
    unsigned long pdata = 0; // 32 bits of planar pixel data
    unsigned long mask = 0x00ff00ff<<5; // Mask for isolating table indices (after shifting)
    asm volatile (
        // Beginning of dbra loop
        "0:                                         \n\t"

        // Read eight consecutive pixels from buffer into two 32 bit registers
        "movem.l    (%[in])+, %%d0-%%d1             \n\t"

        // Prepare pixels 1, 3
        "move.l     %%d0,%%d2                       \n\t"
        "lsl.l      #5,%%d2                         \n\t"
        "and.l      %[mask],%%d2                    \n\t"

        // Pixel 3
        "move.l     12(%[table],%%d2.w), %[pdata]   \n\t"

        // Pixel 1
        "swap       %%d2                            \n\t"
        "or.l       4(%[table],%%d2.w), %[pdata]    \n\t"

        // Prepare pixels 0, 2
        "lsr.l      #3,%%d0                         \n\t"
        "and.l      %[mask],%%d0                    \n\t"

        // Pixel 2
        "or.l       8(%[table],%%d0.w), %[pdata]    \n\t"

        // Pixel 0
        "swap       %%d0                            \n\t"
        "or.l       (%[table],%%d0.w), %[pdata]     \n\t"

        // Prepare pixels 5,7
        "move.l     %%d1,%%d2                       \n\t"
        "lsl.l      #5,%%d2                         \n\t"
        "and.l      %[mask],%%d2                    \n\t"

        // Pixel 7
        "or.l       28(%[table],%%d2.w), %[pdata]   \n\t"

        // Pixel 5
        "swap       %%d2                            \n\t"
        "or.l       20(%[table],%%d2.w), %[pdata]   \n\t"

        // Prepare pixels 4, 6
        "lsr.l      #3,%%d1                         \n\t"
        "and.l      %[mask],%%d1                    \n\t"

        // Pixel 6
        "or.l       24(%[table],%%d1.w), %[pdata]   \n\t"

        // Pixel 4
        "swap       %%d1                            \n\t"
        "or.l       16(%[table],%%d1.w), %[pdata]   \n\t"

        // Write these pixels into ST screen buffer
        "movep.l    %[pdata], 0(%[out])             \n\t"

        // Read another eight consecutive pixels from buffer into two 32 bit registers
        "movem.l    (%[in])+, %%d0-%%d1             \n\t"

        // Prepare pixels 1, 3
        "move.l     %%d0,%%d2                       \n\t"
        "lsl.l      #5,%%d2                         \n\t"
        "and.l      %[mask],%%d2                    \n\t"

        // Pixel 3
        "move.l     12(%[table],%%d2.w), %[pdata]   \n\t"

        // Pixel 1
        "swap       %%d2                            \n\t"
        "or.l       4(%[table],%%d2.w), %[pdata]    \n\t"

        // Prepare pixels 0, 2
        "lsr.l      #3,%%d0                         \n\t"
        "and.l      %[mask],%%d0                    \n\t"

        // Pixel 2
        "or.l       8(%[table],%%d0.w), %[pdata]    \n\t"

        // Pixel 0
        "swap       %%d0                            \n\t"
        "or.l       (%[table],%%d0.w), %[pdata]     \n\t"

        // Prepare pixels 5,7
        "move.l     %%d1,%%d2                       \n\t"
        "lsl.l      #5,%%d2                         \n\t"
        "and.l      %[mask],%%d2                    \n\t"

        // Pixel 7
        "or.l       28(%[table],%%d2.w), %[pdata]   \n\t"

        // Pixel 5
        "swap       %%d2                            \n\t"
        "or.l       20(%[table],%%d2.w), %[pdata]   \n\t"

        // Prepare pixels 4, 6
        "lsr.l      #3,%%d1                         \n\t"
        "and.l      %[mask],%%d1                    \n\t"

        // Pixel 6
        "or.l       24(%[table],%%d1.w), %[pdata]   \n\t"

        // Pixel 4
        "swap       %%d1                            \n\t"
        "or.l       16(%[table],%%d1.w), %[pdata]   \n\t"

        // Write these pixels into ST screen buffer
        "movep.l    %[pdata], 1(%[out])             \n\t"
        
        // Advance out address by 16 pixels (8 bytes) and loop
        "lea        8(%[out]), %[out]               \n\t"
        "dbra.w     %[groups],0b                    \n\t"

        // Outputs
        : [out] "+a" (out)
        , [in] "+a" (in)
        , [pdata] "+d" (pdata)
        , [groups] "+d" (groups)
        
        // Inputs
        : [table] "a" (table)
        , [mask] "d" (mask)
        
        // Clobbers
        : "d0", "d1", "d2", "memory"
    );
#endif
}

#if !USE_MIDRES
static void c2p_2x(register unsigned char *out, const unsigned char *in, unsigned short pixels, unsigned long table[][4]) {
    if (pixels < 8) return;
    short groups = pixels / 8 - 1;
    unsigned long pdata = 0; // 32 bits of planar pixel data
    unsigned long mask = 0x00ff00ff<<4; // Mask for isolating table indices (after shifting)
    asm volatile (
        // Beginning of dbra loop
        "0:                                         \n\t"

        // Read eight consecutive pixels from buffer into two 32 bit registers
        "movem.l    (%[in])+, %%d0-%%d1             \n\t"

        // Prepare pixels 1, 3
        "move.l     %%d0,%%d2                       \n\t"
        "lsl.l      #4,%%d2                         \n\t"
        "and.l      %[mask],%%d2                    \n\t"

        // Pixel 3
        "move.l     12(%[table],%%d2.w), %[pdata]   \n\t"

        // Pixel 1
        "swap       %%d2                            \n\t"
        "or.l       4(%[table],%%d2.w), %[pdata]    \n\t"

        // Prepare pixels 0, 2
        "lsr.l      #4,%%d0                         \n\t"
        "and.l      %[mask],%%d0                    \n\t"

        // Pixel 2
        "or.l       8(%[table],%%d0.w), %[pdata]    \n\t"

        // Pixel 0
        "swap       %%d0                            \n\t"
        "or.l       (%[table],%%d0.w), %[pdata]     \n\t"

        // Write these pixels into ST screen buffer
        "movep.l    %[pdata], 0(%[out])             \n\t"

        // Prepare pixels 5,7
        "move.l     %%d1,%%d2                       \n\t"
        "lsl.l      #4,%%d2                         \n\t"
        "and.l      %[mask],%%d2                    \n\t"

        // Pixel 7
        "move.l     12(%[table],%%d2.w), %[pdata]   \n\t"

        // Pixel 5
        "swap       %%d2                            \n\t"
        "or.l       4(%[table],%%d2.w), %[pdata]   \n\t"

        // Prepare pixels 4, 6
        "lsr.l      #4,%%d1                         \n\t"
        "and.l      %[mask],%%d1                    \n\t"

        // Pixel 6
        "or.l       8(%[table],%%d1.w), %[pdata]   \n\t"

        // Pixel 4
        "swap       %%d1                            \n\t"
        "or.l       (%[table],%%d1.w), %[pdata]     \n\t"

        // Write these pixels into ST screen buffer
        "movep.l    %[pdata], 1(%[out])             \n\t"

        // Advance out address by 16 pixels (8 bytes) and loop
        "lea        8(%[out]), %[out]               \n\t"
        "dbra.w     %[groups],0b                    \n\t"

        // Outputs
        : [out] "+a" (out)
        , [in] "+a" (in)
        , [pdata] "+d" (pdata)
        , [groups] "+d" (groups)
        
        // Inputs
        : [table] "a" (table)
        , [mask] "d" (mask)
        
        // Clobbers
        : "d0", "d1", "d2", "memory"
    );
}

static void c2p_4x(register unsigned char *out, const unsigned char *in, unsigned short pixels, unsigned long table[][2]) {
    if (pixels < 4) return;
    short groups = pixels / 4 - 1;
    unsigned long pdata = 0; // 32 bits of planar pixel data
    unsigned short mask = 0x00ff<<3; // Mask for isolating table indices (after shifting)
    asm volatile (
        // Beginning of dbra loop
        "0:                                         \n\t"

        // Read four consecutive pixels from buffer into two 16 bit registers
        "movem.w    (%[in])+, %%d0-%%d1             \n\t"

        // Pixel 0
        "move.w     %%d0,%%d2                       \n\t"
        "lsr.w      #5,%%d2                         \n\t"
        "and.w      %[mask],%%d2                    \n\t"
        "move.l     (%[table],%%d2.w), %[pdata]     \n\t"

        // Pixel 1
        "lsl.w      #3,%%d0                         \n\t"
        "and.w      %[mask],%%d0                    \n\t"
        "or.l       4(%[table],%%d0.w), %[pdata]    \n\t"

        // Write these pixels into ST screen buffer
        "movep.l    %[pdata], 0(%[out])             \n\t"

        // Pixel 2
        "move.w     %%d1,%%d2                       \n\t"
        "lsr.w      #5,%%d2                         \n\t"
        "and.w      %[mask],%%d2                    \n\t"
        "move.l     0(%[table],%%d2.w), %[pdata]    \n\t"

        // Pixel 3
        "lsl.w      #3,%%d1                         \n\t"
        "and.w      %[mask],%%d1                    \n\t"
        "or.l       4(%[table],%%d1.w), %[pdata]    \n\t"

        // Write these pixels into ST screen buffer
        "movep.l    %[pdata], 1(%[out])             \n\t"

        // Advance out address by 16 pixels (8 bytes) and loop
        "lea        8(%[out]), %[out]               \n\t"
        "dbra.w     %[groups],0b                    \n\t"

        // Outputs
        : [out] "+a" (out)
        , [in] "+a" (in)
        , [pdata] "+d" (pdata)
        , [groups] "+d" (groups)
        
        // Inputs
        : [table] "a" (table)
        , [mask] "d" (mask)
        
        // Clobbers
        : "d0", "d1", "d2", "memory"
    );
}
#endif

void set_doom_palette(const unsigned char *colors) {
#if USE_MIDRES
    // do nothing
#else
    unsigned short stpalette[sizeof(subset)];
    for (int i=0; i<sizeof(subset); i++) {
        const unsigned char *c = &colors[3*subset[i]];
        stpalette[i] = stcolor(c[0], c[1], c[2]);
    }
    install_palette(stpalette);
#endif
}

void draw_palette_table(unsigned char *st_screen) {
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
		for (int i=0; i<8; i++) c2p(st_screen + 160*(32+y+i), buf, 128+16, c2p_table[i%4]);
	}
}

extern boolean inhelpscreens;
extern boolean menuactive;
extern boolean automapactive;
extern gamestate_t gamestate;

void c2p_screen(unsigned char *out, const unsigned char *in) {
#if USE_MIDRES
    for (int line = 0; line < SCREENHEIGHT; line++ ) {
	    c2p(out + 160*line, in + 320*line, 320, c2p_table[line&3]);
    }
#else
    boolean zoom_allowed = gamestate == GS_LEVEL
        && !menuactive && !inhelpscreens && !automapactive;
    short splitline = !zoom_allowed || viewheight == SCREENHEIGHT ? SCREENHEIGHT : SCREENHEIGHT - 32;
    if (!zoom_allowed || viewwidth > SCREENWIDTH/2) {
        for (short line = 0; line < splitline; line++ ) {
            c2p(out + 160*line, in + 320*line, 320, c2p_table[line&3]);
        }
    } else if(viewwidth > SCREENWIDTH/4) {
        // 2x zoom
        for (short line = 0; line < splitline; line++ ) {
            c2p_2x(out + 160*line, in + SCREENWIDTH*(42 + line/2) + 80, 160, c2p_2x_table[line&3]);
        }
    } else {
        // 4x zoom
        for (short line = 0; line < splitline; line++ ) {
            short phase = line & 3;
            if (phase < 2) {
                c2p_4x(out + 160*line, in + SCREENWIDTH*(63 + line/4) + 120, 80, c2p_4x_table[phase]);
            } else if (phase == 2) {
                memcpy(out + 160*line, out + 160*line - 320, 320);
            }
        }
    }
#endif
}

void c2p_statusbar(unsigned char *out, const unsigned char *in, short y_begin, short y_end, short x_begin, short x_end) {
    if (y_end <= 0 || x_end <= 0)
        return;
    boolean statusbar_drawn = gamestate == GS_LEVEL
        && !menuactive && !inhelpscreens && !automapactive && viewheight < SCREENHEIGHT;
    if (!statusbar_drawn)
        return;
    if (y_begin < SCREENHEIGHT - 32)
        y_begin = SCREENHEIGHT - 32;
    if (y_end > SCREENHEIGHT)
        y_end = SCREENHEIGHT;
    if (x_begin < 0)
        x_begin = 0;
    if (x_end > SCREENWIDTH)
        x_end = SCREENWIDTH;

    x_begin &= ~15;
    x_end = (x_end + 15) & ~15;
    out += y_begin * 160 + x_begin / 2;
    in += y_begin * 320 + x_begin;
    //fprintf(stderr, "%d %d %d %d \r", y_begin, y_end, x_begin, x_end);

    for (int line = y_begin; line < y_end; line++ ) {
        c2p(out, in, x_end - x_begin, c2p_table[line&3]);
        out += 160;
        in += 320;
    }
}
