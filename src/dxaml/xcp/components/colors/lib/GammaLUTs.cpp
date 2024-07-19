// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// Look-up tables for gamma conversion (converting a color channel
// from sRGB to scRGB, or vice versa).

#include "precomp.h"
#include <GammaLUTs.h>
#include <real.h>

UINT8 Convert_scRGB_Channel_To_sRGB_Byte(FLOAT rColorComponent)
{
    if (!(rColorComponent > 0.0f))   // Handles the NaN case
        return 0;
    else if (rColorComponent < 1.0f)
        return GammaLUT_scRGB_to_sRGB[XcpRound(GammaLUT_scRGB_to_sRGB_scale * rColorComponent)];
    else
        return 255;
}

// Return smallest argument for Convert_scRGB_Channel_To_sRGB_Byte that causes nonzero result
FLOAT Get_Smallest_scRGB_Significant_for_sRGB()
{
    XFLOAT rColor = 0.5f / GammaLUT_scRGB_to_sRGB_scale;

    ASSERT(Convert_scRGB_Channel_To_sRGB_Byte(rColor) == 1);
    ASSERT(Convert_scRGB_Channel_To_sRGB_Byte(XcpNextSmaller(rColor)) == 0);

    return rColor;
}

// Look-up table for converting an scRGB color channel to sRGB.
//
// To use this table:
//   * Preferably, just use Convert_scRGB_Channel_To_sRGB_Byte or something that uses it.
//   * Or if you must, do what it does ... be careful.
//
// Speaking approximately, you can call this a "gamma 1/2.2" table.
// But it actually uses the inverse of the sRGB gamma profile (which is a little different).
//
// The table size is chosen so that if you:
//   1) take an input floating-point sRGB value of n/255 or n/100 (n is an integer)
//   2) convert it to floating-point scRGB using the sRGB gamma profile
//   3) use this table to convert it back to an sRGB value
// you get the same result as round(x * 255).
//
// i.e. the fact that we're using scRGB internally is reasonably well hidden from users who
// are working entirely in sRGB.
//
// This table was generated by gen_GammaLUTs.bat
// Code for the corresponding function:
//   double scRGBTosRGB(double x)
//   {
//      if (!(x > 0.0))    // Handles NaN case too
//      {
//          return 0.0;
//      }
//      else if (x <= 0.0031308)
//      {
//          return x * 12.92;
//      }
//      else if (x < 1.0)
//      {
//          return ((1.055 * pow(x, 1.0/2.4))-0.055);
//      }
//      else
//      {
//          return 1.0;
//      }
//   }
const UINT8 GammaLUT_scRGB_to_sRGB [GammaLUT_scRGB_to_sRGB_size] = {    
      0,   1,   2,   3,   4,   5,   6,   7,
      8,   9,  10,  11,  12,  13,  13,  14,
     15,  16,  16,  17,  18,  18,  19,  20,
     20,  21,  21,  22,  23,  23,  24,  24,
     25,  25,  26,  26,  27,  27,  28,  28,
     28,  29,  29,  30,  30,  31,  31,  31,
     32,  32,  33,  33,  33,  34,  34,  34,
     35,  35,  36,  36,  36,  37,  37,  37,
     38,  38,  38,  39,  39,  39,  40,  40,
     40,  41,  41,  41,  41,  42,  42,  42,
     43,  43,  43,  44,  44,  44,  44,  45,
     45,  45,  46,  46,  46,  46,  47,  47,
     47,  47,  48,  48,  48,  48,  49,  49,
     49,  50,  50,  50,  50,  51,  51,  51,
     51,  51,  52,  52,  52,  52,  53,  53,
     53,  53,  54,  54,  54,  54,  55,  55,
     55,  55,  55,  56,  56,  56,  56,  57,
     57,  57,  57,  57,  58,  58,  58,  58,
     58,  59,  59,  59,  59,  59,  60,  60,
     60,  60,  60,  61,  61,  61,  61,  61,
     62,  62,  62,  62,  62,  63,  63,  63,
     63,  63,  64,  64,  64,  64,  64,  65,
     65,  65,  65,  65,  66,  66,  66,  66,
     66,  66,  67,  67,  67,  67,  67,  67,
     68,  68,  68,  68,  68,  69,  69,  69,
     69,  69,  69,  70,  70,  70,  70,  70,
     70,  71,  71,  71,  71,  71,  71,  72,
     72,  72,  72,  72,  72,  73,  73,  73,
     73,  73,  73,  74,  74,  74,  74,  74,
     74,  75,  75,  75,  75,  75,  75,  75,
     76,  76,  76,  76,  76,  76,  77,  77,
     77,  77,  77,  77,  77,  78,  78,  78,
     78,  78,  78,  79,  79,  79,  79,  79,
     79,  79,  80,  80,  80,  80,  80,  80,
     80,  81,  81,  81,  81,  81,  81,  81,
     82,  82,  82,  82,  82,  82,  82,  83,
     83,  83,  83,  83,  83,  83,  84,  84,
     84,  84,  84,  84,  84,  84,  85,  85,
     85,  85,  85,  85,  85,  86,  86,  86,
     86,  86,  86,  86,  87,  87,  87,  87,
     87,  87,  87,  87,  88,  88,  88,  88,
     88,  88,  88,  88,  89,  89,  89,  89,
     89,  89,  89,  90,  90,  90,  90,  90,
     90,  90,  90,  91,  91,  91,  91,  91,
     91,  91,  91,  92,  92,  92,  92,  92,
     92,  92,  92,  92,  93,  93,  93,  93,
     93,  93,  93,  93,  94,  94,  94,  94,
     94,  94,  94,  94,  95,  95,  95,  95,
     95,  95,  95,  95,  95,  96,  96,  96,
     96,  96,  96,  96,  96,  97,  97,  97,
     97,  97,  97,  97,  97,  97,  98,  98,
     98,  98,  98,  98,  98,  98,  98,  99,
     99,  99,  99,  99,  99,  99,  99, 100,
    100, 100, 100, 100, 100, 100, 100, 100,
    101, 101, 101, 101, 101, 101, 101, 101,
    101, 101, 102, 102, 102, 102, 102, 102,
    102, 102, 102, 103, 103, 103, 103, 103,
    103, 103, 103, 103, 104, 104, 104, 104,
    104, 104, 104, 104, 104, 104, 105, 105,
    105, 105, 105, 105, 105, 105, 105, 106,
    106, 106, 106, 106, 106, 106, 106, 106,
    106, 107, 107, 107, 107, 107, 107, 107,
    107, 107, 107, 108, 108, 108, 108, 108,
    108, 108, 108, 108, 109, 109, 109, 109,
    109, 109, 109, 109, 109, 109, 110, 110,
    110, 110, 110, 110, 110, 110, 110, 110,
    110, 111, 111, 111, 111, 111, 111, 111,
    111, 111, 111, 112, 112, 112, 112, 112,
    112, 112, 112, 112, 112, 113, 113, 113,
    113, 113, 113, 113, 113, 113, 113, 113,
    114, 114, 114, 114, 114, 114, 114, 114,
    114, 114, 115, 115, 115, 115, 115, 115,
    115, 115, 115, 115, 115, 116, 116, 116,
    116, 116, 116, 116, 116, 116, 116, 116,
    117, 117, 117, 117, 117, 117, 117, 117,
    117, 117, 117, 118, 118, 118, 118, 118,
    118, 118, 118, 118, 118, 118, 119, 119,
    119, 119, 119, 119, 119, 119, 119, 119,
    119, 120, 120, 120, 120, 120, 120, 120,
    120, 120, 120, 120, 121, 121, 121, 121,
    121, 121, 121, 121, 121, 121, 121, 121,
    122, 122, 122, 122, 122, 122, 122, 122,
    122, 122, 122, 123, 123, 123, 123, 123,
    123, 123, 123, 123, 123, 123, 123, 124,
    124, 124, 124, 124, 124, 124, 124, 124,
    124, 124, 125, 125, 125, 125, 125, 125,
    125, 125, 125, 125, 125, 125, 126, 126,
    126, 126, 126, 126, 126, 126, 126, 126,
    126, 126, 127, 127, 127, 127, 127, 127,
    127, 127, 127, 127, 127, 127, 128, 128,
    128, 128, 128, 128, 128, 128, 128, 128,
    128, 128, 128, 129, 129, 129, 129, 129,
    129, 129, 129, 129, 129, 129, 129, 130,
    130, 130, 130, 130, 130, 130, 130, 130,
    130, 130, 130, 131, 131, 131, 131, 131,
    131, 131, 131, 131, 131, 131, 131, 131,
    132, 132, 132, 132, 132, 132, 132, 132,
    132, 132, 132, 132, 132, 133, 133, 133,
    133, 133, 133, 133, 133, 133, 133, 133,
    133, 133, 134, 134, 134, 134, 134, 134,
    134, 134, 134, 134, 134, 134, 134, 135,
    135, 135, 135, 135, 135, 135, 135, 135,
    135, 135, 135, 135, 136, 136, 136, 136,
    136, 136, 136, 136, 136, 136, 136, 136,
    136, 137, 137, 137, 137, 137, 137, 137,
    137, 137, 137, 137, 137, 137, 138, 138,
    138, 138, 138, 138, 138, 138, 138, 138,
    138, 138, 138, 138, 139, 139, 139, 139,
    139, 139, 139, 139, 139, 139, 139, 139,
    139, 140, 140, 140, 140, 140, 140, 140,
    140, 140, 140, 140, 140, 140, 140, 141,
    141, 141, 141, 141, 141, 141, 141, 141,
    141, 141, 141, 141, 141, 142, 142, 142,
    142, 142, 142, 142, 142, 142, 142, 142,
    142, 142, 142, 143, 143, 143, 143, 143,
    143, 143, 143, 143, 143, 143, 143, 143,
    143, 144, 144, 144, 144, 144, 144, 144,
    144, 144, 144, 144, 144, 144, 144, 145,
    145, 145, 145, 145, 145, 145, 145, 145,
    145, 145, 145, 145, 145, 146, 146, 146,
    146, 146, 146, 146, 146, 146, 146, 146,
    146, 146, 146, 146, 147, 147, 147, 147,
    147, 147, 147, 147, 147, 147, 147, 147,
    147, 147, 148, 148, 148, 148, 148, 148,
    148, 148, 148, 148, 148, 148, 148, 148,
    148, 149, 149, 149, 149, 149, 149, 149,
    149, 149, 149, 149, 149, 149, 149, 149,
    150, 150, 150, 150, 150, 150, 150, 150,
    150, 150, 150, 150, 150, 150, 150, 151,
    151, 151, 151, 151, 151, 151, 151, 151,
    151, 151, 151, 151, 151, 151, 152, 152,
    152, 152, 152, 152, 152, 152, 152, 152,
    152, 152, 152, 152, 152, 153, 153, 153,
    153, 153, 153, 153, 153, 153, 153, 153,
    153, 153, 153, 153, 153, 154, 154, 154,
    154, 154, 154, 154, 154, 154, 154, 154,
    154, 154, 154, 154, 155, 155, 155, 155,
    155, 155, 155, 155, 155, 155, 155, 155,
    155, 155, 155, 155, 156, 156, 156, 156,
    156, 156, 156, 156, 156, 156, 156, 156,
    156, 156, 156, 157, 157, 157, 157, 157,
    157, 157, 157, 157, 157, 157, 157, 157,
    157, 157, 157, 158, 158, 158, 158, 158,
    158, 158, 158, 158, 158, 158, 158, 158,
    158, 158, 158, 159, 159, 159, 159, 159,
    159, 159, 159, 159, 159, 159, 159, 159,
    159, 159, 159, 160, 160, 160, 160, 160,
    160, 160, 160, 160, 160, 160, 160, 160,
    160, 160, 160, 160, 161, 161, 161, 161,
    161, 161, 161, 161, 161, 161, 161, 161,
    161, 161, 161, 161, 162, 162, 162, 162,
    162, 162, 162, 162, 162, 162, 162, 162,
    162, 162, 162, 162, 162, 163, 163, 163,
    163, 163, 163, 163, 163, 163, 163, 163,
    163, 163, 163, 163, 163, 164, 164, 164,
    164, 164, 164, 164, 164, 164, 164, 164,
    164, 164, 164, 164, 164, 164, 165, 165,
    165, 165, 165, 165, 165, 165, 165, 165,
    165, 165, 165, 165, 165, 165, 165, 166,
    166, 166, 166, 166, 166, 166, 166, 166,
    166, 166, 166, 166, 166, 166, 166, 166,
    167, 167, 167, 167, 167, 167, 167, 167,
    167, 167, 167, 167, 167, 167, 167, 167,
    167, 168, 168, 168, 168, 168, 168, 168,
    168, 168, 168, 168, 168, 168, 168, 168,
    168, 168, 168, 169, 169, 169, 169, 169,
    169, 169, 169, 169, 169, 169, 169, 169,
    169, 169, 169, 169, 170, 170, 170, 170,
    170, 170, 170, 170, 170, 170, 170, 170,
    170, 170, 170, 170, 170, 170, 171, 171,
    171, 171, 171, 171, 171, 171, 171, 171,
    171, 171, 171, 171, 171, 171, 171, 172,
    172, 172, 172, 172, 172, 172, 172, 172,
    172, 172, 172, 172, 172, 172, 172, 172,
    172, 173, 173, 173, 173, 173, 173, 173,
    173, 173, 173, 173, 173, 173, 173, 173,
    173, 173, 173, 174, 174, 174, 174, 174,
    174, 174, 174, 174, 174, 174, 174, 174,
    174, 174, 174, 174, 174, 175, 175, 175,
    175, 175, 175, 175, 175, 175, 175, 175,
    175, 175, 175, 175, 175, 175, 175, 176,
    176, 176, 176, 176, 176, 176, 176, 176,
    176, 176, 176, 176, 176, 176, 176, 176,
    176, 176, 177, 177, 177, 177, 177, 177,
    177, 177, 177, 177, 177, 177, 177, 177,
    177, 177, 177, 177, 178, 178, 178, 178,
    178, 178, 178, 178, 178, 178, 178, 178,
    178, 178, 178, 178, 178, 178, 178, 179,
    179, 179, 179, 179, 179, 179, 179, 179,
    179, 179, 179, 179, 179, 179, 179, 179,
    179, 179, 180, 180, 180, 180, 180, 180,
    180, 180, 180, 180, 180, 180, 180, 180,
    180, 180, 180, 180, 180, 181, 181, 181,
    181, 181, 181, 181, 181, 181, 181, 181,
    181, 181, 181, 181, 181, 181, 181, 181,
    182, 182, 182, 182, 182, 182, 182, 182,
    182, 182, 182, 182, 182, 182, 182, 182,
    182, 182, 182, 183, 183, 183, 183, 183,
    183, 183, 183, 183, 183, 183, 183, 183,
    183, 183, 183, 183, 183, 183, 184, 184,
    184, 184, 184, 184, 184, 184, 184, 184,
    184, 184, 184, 184, 184, 184, 184, 184,
    184, 184, 185, 185, 185, 185, 185, 185,
    185, 185, 185, 185, 185, 185, 185, 185,
    185, 185, 185, 185, 185, 185, 186, 186,
    186, 186, 186, 186, 186, 186, 186, 186,
    186, 186, 186, 186, 186, 186, 186, 186,
    186, 187, 187, 187, 187, 187, 187, 187,
    187, 187, 187, 187, 187, 187, 187, 187,
    187, 187, 187, 187, 187, 188, 188, 188,
    188, 188, 188, 188, 188, 188, 188, 188,
    188, 188, 188, 188, 188, 188, 188, 188,
    188, 189, 189, 189, 189, 189, 189, 189,
    189, 189, 189, 189, 189, 189, 189, 189,
    189, 189, 189, 189, 189, 190, 190, 190,
    190, 190, 190, 190, 190, 190, 190, 190,
    190, 190, 190, 190, 190, 190, 190, 190,
    190, 190, 191, 191, 191, 191, 191, 191,
    191, 191, 191, 191, 191, 191, 191, 191,
    191, 191, 191, 191, 191, 191, 192, 192,
    192, 192, 192, 192, 192, 192, 192, 192,
    192, 192, 192, 192, 192, 192, 192, 192,
    192, 192, 192, 193, 193, 193, 193, 193,
    193, 193, 193, 193, 193, 193, 193, 193,
    193, 193, 193, 193, 193, 193, 193, 194,
    194, 194, 194, 194, 194, 194, 194, 194,
    194, 194, 194, 194, 194, 194, 194, 194,
    194, 194, 194, 194, 195, 195, 195, 195,
    195, 195, 195, 195, 195, 195, 195, 195,
    195, 195, 195, 195, 195, 195, 195, 195,
    195, 196, 196, 196, 196, 196, 196, 196,
    196, 196, 196, 196, 196, 196, 196, 196,
    196, 196, 196, 196, 196, 196, 196, 197,
    197, 197, 197, 197, 197, 197, 197, 197,
    197, 197, 197, 197, 197, 197, 197, 197,
    197, 197, 197, 197, 198, 198, 198, 198,
    198, 198, 198, 198, 198, 198, 198, 198,
    198, 198, 198, 198, 198, 198, 198, 198,
    198, 199, 199, 199, 199, 199, 199, 199,
    199, 199, 199, 199, 199, 199, 199, 199,
    199, 199, 199, 199, 199, 199, 199, 200,
    200, 200, 200, 200, 200, 200, 200, 200,
    200, 200, 200, 200, 200, 200, 200, 200,
    200, 200, 200, 200, 200, 201, 201, 201,
    201, 201, 201, 201, 201, 201, 201, 201,
    201, 201, 201, 201, 201, 201, 201, 201,
    201, 201, 202, 202, 202, 202, 202, 202,
    202, 202, 202, 202, 202, 202, 202, 202,
    202, 202, 202, 202, 202, 202, 202, 202,
    203, 203, 203, 203, 203, 203, 203, 203,
    203, 203, 203, 203, 203, 203, 203, 203,
    203, 203, 203, 203, 203, 203, 203, 204,
    204, 204, 204, 204, 204, 204, 204, 204,
    204, 204, 204, 204, 204, 204, 204, 204,
    204, 204, 204, 204, 204, 205, 205, 205,
    205, 205, 205, 205, 205, 205, 205, 205,
    205, 205, 205, 205, 205, 205, 205, 205,
    205, 205, 205, 206, 206, 206, 206, 206,
    206, 206, 206, 206, 206, 206, 206, 206,
    206, 206, 206, 206, 206, 206, 206, 206,
    206, 206, 207, 207, 207, 207, 207, 207,
    207, 207, 207, 207, 207, 207, 207, 207,
    207, 207, 207, 207, 207, 207, 207, 207,
    207, 208, 208, 208, 208, 208, 208, 208,
    208, 208, 208, 208, 208, 208, 208, 208,
    208, 208, 208, 208, 208, 208, 208, 208,
    209, 209, 209, 209, 209, 209, 209, 209,
    209, 209, 209, 209, 209, 209, 209, 209,
    209, 209, 209, 209, 209, 209, 209, 210,
    210, 210, 210, 210, 210, 210, 210, 210,
    210, 210, 210, 210, 210, 210, 210, 210,
    210, 210, 210, 210, 210, 210, 211, 211,
    211, 211, 211, 211, 211, 211, 211, 211,
    211, 211, 211, 211, 211, 211, 211, 211,
    211, 211, 211, 211, 211, 212, 212, 212,
    212, 212, 212, 212, 212, 212, 212, 212,
    212, 212, 212, 212, 212, 212, 212, 212,
    212, 212, 212, 212, 213, 213, 213, 213,
    213, 213, 213, 213, 213, 213, 213, 213,
    213, 213, 213, 213, 213, 213, 213, 213,
    213, 213, 213, 213, 214, 214, 214, 214,
    214, 214, 214, 214, 214, 214, 214, 214,
    214, 214, 214, 214, 214, 214, 214, 214,
    214, 214, 214, 214, 215, 215, 215, 215,
    215, 215, 215, 215, 215, 215, 215, 215,
    215, 215, 215, 215, 215, 215, 215, 215,
    215, 215, 215, 215, 216, 216, 216, 216,
    216, 216, 216, 216, 216, 216, 216, 216,
    216, 216, 216, 216, 216, 216, 216, 216,
    216, 216, 216, 216, 217, 217, 217, 217,
    217, 217, 217, 217, 217, 217, 217, 217,
    217, 217, 217, 217, 217, 217, 217, 217,
    217, 217, 217, 217, 218, 218, 218, 218,
    218, 218, 218, 218, 218, 218, 218, 218,
    218, 218, 218, 218, 218, 218, 218, 218,
    218, 218, 218, 218, 219, 219, 219, 219,
    219, 219, 219, 219, 219, 219, 219, 219,
    219, 219, 219, 219, 219, 219, 219, 219,
    219, 219, 219, 219, 219, 220, 220, 220,
    220, 220, 220, 220, 220, 220, 220, 220,
    220, 220, 220, 220, 220, 220, 220, 220,
    220, 220, 220, 220, 220, 221, 221, 221,
    221, 221, 221, 221, 221, 221, 221, 221,
    221, 221, 221, 221, 221, 221, 221, 221,
    221, 221, 221, 221, 221, 221, 222, 222,
    222, 222, 222, 222, 222, 222, 222, 222,
    222, 222, 222, 222, 222, 222, 222, 222,
    222, 222, 222, 222, 222, 222, 222, 223,
    223, 223, 223, 223, 223, 223, 223, 223,
    223, 223, 223, 223, 223, 223, 223, 223,
    223, 223, 223, 223, 223, 223, 223, 223,
    224, 224, 224, 224, 224, 224, 224, 224,
    224, 224, 224, 224, 224, 224, 224, 224,
    224, 224, 224, 224, 224, 224, 224, 224,
    224, 225, 225, 225, 225, 225, 225, 225,
    225, 225, 225, 225, 225, 225, 225, 225,
    225, 225, 225, 225, 225, 225, 225, 225,
    225, 225, 225, 226, 226, 226, 226, 226,
    226, 226, 226, 226, 226, 226, 226, 226,
    226, 226, 226, 226, 226, 226, 226, 226,
    226, 226, 226, 226, 227, 227, 227, 227,
    227, 227, 227, 227, 227, 227, 227, 227,
    227, 227, 227, 227, 227, 227, 227, 227,
    227, 227, 227, 227, 227, 227, 228, 228,
    228, 228, 228, 228, 228, 228, 228, 228,
    228, 228, 228, 228, 228, 228, 228, 228,
    228, 228, 228, 228, 228, 228, 228, 228,
    229, 229, 229, 229, 229, 229, 229, 229,
    229, 229, 229, 229, 229, 229, 229, 229,
    229, 229, 229, 229, 229, 229, 229, 229,
    229, 230, 230, 230, 230, 230, 230, 230,
    230, 230, 230, 230, 230, 230, 230, 230,
    230, 230, 230, 230, 230, 230, 230, 230,
    230, 230, 230, 230, 231, 231, 231, 231,
    231, 231, 231, 231, 231, 231, 231, 231,
    231, 231, 231, 231, 231, 231, 231, 231,
    231, 231, 231, 231, 231, 231, 232, 232,
    232, 232, 232, 232, 232, 232, 232, 232,
    232, 232, 232, 232, 232, 232, 232, 232,
    232, 232, 232, 232, 232, 232, 232, 232,
    233, 233, 233, 233, 233, 233, 233, 233,
    233, 233, 233, 233, 233, 233, 233, 233,
    233, 233, 233, 233, 233, 233, 233, 233,
    233, 233, 233, 234, 234, 234, 234, 234,
    234, 234, 234, 234, 234, 234, 234, 234,
    234, 234, 234, 234, 234, 234, 234, 234,
    234, 234, 234, 234, 234, 235, 235, 235,
    235, 235, 235, 235, 235, 235, 235, 235,
    235, 235, 235, 235, 235, 235, 235, 235,
    235, 235, 235, 235, 235, 235, 235, 235,
    236, 236, 236, 236, 236, 236, 236, 236,
    236, 236, 236, 236, 236, 236, 236, 236,
    236, 236, 236, 236, 236, 236, 236, 236,
    236, 236, 236, 237, 237, 237, 237, 237,
    237, 237, 237, 237, 237, 237, 237, 237,
    237, 237, 237, 237, 237, 237, 237, 237,
    237, 237, 237, 237, 237, 237, 237, 238,
    238, 238, 238, 238, 238, 238, 238, 238,
    238, 238, 238, 238, 238, 238, 238, 238,
    238, 238, 238, 238, 238, 238, 238, 238,
    238, 238, 239, 239, 239, 239, 239, 239,
    239, 239, 239, 239, 239, 239, 239, 239,
    239, 239, 239, 239, 239, 239, 239, 239,
    239, 239, 239, 239, 239, 240, 240, 240,
    240, 240, 240, 240, 240, 240, 240, 240,
    240, 240, 240, 240, 240, 240, 240, 240,
    240, 240, 240, 240, 240, 240, 240, 240,
    240, 241, 241, 241, 241, 241, 241, 241,
    241, 241, 241, 241, 241, 241, 241, 241,
    241, 241, 241, 241, 241, 241, 241, 241,
    241, 241, 241, 241, 241, 242, 242, 242,
    242, 242, 242, 242, 242, 242, 242, 242,
    242, 242, 242, 242, 242, 242, 242, 242,
    242, 242, 242, 242, 242, 242, 242, 242,
    242, 243, 243, 243, 243, 243, 243, 243,
    243, 243, 243, 243, 243, 243, 243, 243,
    243, 243, 243, 243, 243, 243, 243, 243,
    243, 243, 243, 243, 243, 244, 244, 244,
    244, 244, 244, 244, 244, 244, 244, 244,
    244, 244, 244, 244, 244, 244, 244, 244,
    244, 244, 244, 244, 244, 244, 244, 244,
    244, 245, 245, 245, 245, 245, 245, 245,
    245, 245, 245, 245, 245, 245, 245, 245,
    245, 245, 245, 245, 245, 245, 245, 245,
    245, 245, 245, 245, 245, 246, 246, 246,
    246, 246, 246, 246, 246, 246, 246, 246,
    246, 246, 246, 246, 246, 246, 246, 246,
    246, 246, 246, 246, 246, 246, 246, 246,
    246, 246, 247, 247, 247, 247, 247, 247,
    247, 247, 247, 247, 247, 247, 247, 247,
    247, 247, 247, 247, 247, 247, 247, 247,
    247, 247, 247, 247, 247, 247, 248, 248,
    248, 248, 248, 248, 248, 248, 248, 248,
    248, 248, 248, 248, 248, 248, 248, 248,
    248, 248, 248, 248, 248, 248, 248, 248,
    248, 248, 248, 249, 249, 249, 249, 249,
    249, 249, 249, 249, 249, 249, 249, 249,
    249, 249, 249, 249, 249, 249, 249, 249,
    249, 249, 249, 249, 249, 249, 249, 249,
    250, 250, 250, 250, 250, 250, 250, 250,
    250, 250, 250, 250, 250, 250, 250, 250,
    250, 250, 250, 250, 250, 250, 250, 250,
    250, 250, 250, 250, 250, 251, 251, 251,
    251, 251, 251, 251, 251, 251, 251, 251,
    251, 251, 251, 251, 251, 251, 251, 251,
    251, 251, 251, 251, 251, 251, 251, 251,
    251, 251, 251, 252, 252, 252, 252, 252,
    252, 252, 252, 252, 252, 252, 252, 252,
    252, 252, 252, 252, 252, 252, 252, 252,
    252, 252, 252, 252, 252, 252, 252, 252,
    253, 253, 253, 253, 253, 253, 253, 253,
    253, 253, 253, 253, 253, 253, 253, 253,
    253, 253, 253, 253, 253, 253, 253, 253,
    253, 253, 253, 253, 253, 253, 254, 254,
    254, 254, 254, 254, 254, 254, 254, 254,
    254, 254, 254, 254, 254, 254, 254, 254,
    254, 254, 254, 254, 254, 254, 254, 254,
    254, 254, 254, 254, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255};


// Look-up table for converting an sRGB color channel to scRGB.
//
// To use on a channel, use the following code:
//
//   REAL ConvertChannel_sRGB_scRGB(XUINT8 bInput)
//   {
//       return GammaLUT_sRGB_to_scRGB[bInput];
//   }
//
// Speaking approximately, you can call this a "gamma 2.2" table.
// But it actually uses the sRGB gamma profile (which is a little different).
//
//
// This table was generated by gen_GammaLUTs.bat
// Code for the corresponding function:
//   double sRGBToscRGB(double x)
//   {
//      if (!(x > 0.0))    // Handles NaN case too
//      {
//         return 0.0;
//      }
//      else if (x <= 0.04045)
//      {
//         return x / 12.92;
//      }
//      else if (x < 1.0)
//      {
//         return pow((x + 0.055) / 1.055, 2.4);
//      }
//      else
//      {
//         return 1.0;
//      }
//   }

const FLOAT GammaLUT_sRGB_to_scRGB [256] = {
    0.000000000f, 0.077399381f, 0.154798762f, 0.232198142f,
    0.309597523f, 0.386996904f, 0.464396285f, 0.541795666f,
    0.619195046f, 0.696594427f, 0.773993808f, 0.853366620f,
    0.937509368f, 1.026302840f, 1.119817720f, 1.218123138f,
    1.321286759f, 1.429374864f, 1.542452421f, 1.660583152f,
    1.783829598f, 1.912253171f, 2.045914211f, 2.184872033f,
    2.329184969f, 2.478910417f, 2.634104873f, 2.794823972f,
    2.961122521f, 3.133054531f, 3.310673247f, 3.494031177f,
    3.683180117f, 3.878171178f, 4.079054808f, 4.285880817f,
    4.498698395f, 4.717556133f, 4.942502044f, 5.173583579f,
    5.410847646f, 5.654340622f, 5.904108375f, 6.160196274f,
    6.422649205f, 6.691511583f, 6.966827368f, 7.248640074f,
    7.536992782f, 7.831928152f, 8.133488434f, 8.441715476f,
    8.756650736f, 9.078335293f, 9.406809852f, 9.742114757f,
    10.084289996f, 10.433375211f, 10.789409708f, 11.152432461f,
    11.522482118f, 11.899597016f, 12.283815178f, 12.675174326f,
    13.073711885f, 13.479464991f, 13.892470493f, 14.312764963f,
    14.740384699f, 15.175365732f, 15.617743829f, 16.067554502f,
    16.524833007f, 16.989614354f, 17.461933310f, 17.941824403f,
    18.429321924f, 18.924459937f, 19.427272278f, 19.937792560f,
    20.456054180f, 20.982090318f, 21.515933944f, 22.057617819f,
    22.607174503f, 23.164636352f, 23.730035526f, 24.303403991f,
    24.884773521f, 25.474175703f, 26.071641937f, 26.677203443f,
    27.290891259f, 27.912736248f, 28.542769098f, 29.181020326f,
    29.827520278f, 30.482299137f, 31.145386919f, 31.816813478f,
    32.496608511f, 33.184801556f, 33.881421996f, 34.586499062f,
    35.300061833f, 36.022139241f, 36.752760069f, 37.491952957f,
    38.239746402f, 38.996168759f, 39.761248245f, 40.535012941f,
    41.317490788f, 42.108709598f, 42.908697048f, 43.717480687f,
    44.535087932f, 45.361546076f, 46.196882284f, 47.041123598f,
    47.894296937f, 48.756429099f, 49.627546763f, 50.507676488f,
    51.396844718f, 52.295077780f, 53.202401887f, 54.118843141f,
    55.044427529f, 55.979180931f, 56.923129116f, 57.876297745f,
    58.838712374f, 59.810398451f, 60.791381322f, 61.781686229f,
    62.781338310f, 63.790362606f, 64.808784054f, 65.836627495f,
    66.873917670f, 67.920679225f, 68.976936708f, 70.042714575f,
    71.118037187f, 72.202928810f, 73.297413620f, 74.401515703f,
    75.515259054f, 76.638667576f, 77.771765088f, 78.914575318f,
    80.067121911f, 81.229428422f, 82.401518324f, 83.583415004f,
    84.775141768f, 85.976721837f, 87.188178352f, 88.409534371f,
    89.640812873f, 90.882036757f, 92.133228845f, 93.394411878f,
    94.665608521f, 95.946841363f, 97.238132915f, 98.539505616f,
    99.850981826f, 101.172583835f, 102.504333857f, 103.846254035f,
    105.198366438f, 106.560693066f, 107.933255846f, 109.316076637f,
    110.709177225f, 112.112579329f, 113.526304602f, 114.950374624f,
    116.384810911f, 117.829634912f, 119.284868009f, 120.750531518f,
    122.226646691f, 123.713234714f, 125.210316711f, 126.717913741f,
    128.236046798f, 129.764736818f, 131.304004671f, 132.853871167f,
    134.414357055f, 135.985483023f, 137.567269698f, 139.159737649f,
    140.762907386f, 142.376799357f, 144.001433955f, 145.636831514f,
    147.283012310f, 148.939996562f, 150.607804434f, 152.286456033f,
    153.975971408f, 155.676370556f, 157.387673417f, 159.109899877f,
    160.843069768f, 162.587202868f, 164.342318902f, 166.108437542f,
    167.885578406f, 169.673761062f, 171.473005024f, 173.283329756f,
    175.104754671f, 176.937299129f, 178.780982443f, 180.635823872f,
    182.501842629f, 184.379057875f, 186.267488723f, 188.167154237f,
    190.078073433f, 192.000265278f, 193.933748692f, 195.878542548f,
    197.834665671f, 199.802136839f, 201.780974785f, 203.771198194f,
    205.772825706f, 207.785875915f, 209.810367371f, 211.846318578f,
    213.893747994f, 215.952674035f, 218.023115072f, 220.105089431f,
    222.198615396f, 224.303711206f, 226.420395060f, 228.548685110f,
    230.688599470f, 232.840156207f, 235.003373351f, 237.178268886f,
    239.364860757f, 241.563166867f, 243.773205079f, 245.994993213f,
    248.228549052f, 250.473890335f, 252.731034764f, 255.000000000f
};
