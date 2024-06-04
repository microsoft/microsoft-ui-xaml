// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//+-----------------------------------------------------------------------------
//
//  Description:
//
//      Pixel utility & conversion functions for various formats.
//
//------------------------------------------------------------------------------
#include "ColorUtil.h"
#include <paltypes.h>

// The AGRB64TEXEL structure is sort of funky in order to optimize the
// inner loop of our C-code linear gradient routine.
struct AGRB64TEXEL      // Note that it's 'AGRB', not 'ARGB'
{
    UINT32 A00rr00bb;   // Texel's R and B components
    UINT32 A00aa00gg;   // Texel's A and G components
};

//
// Lookup tables defined in pixelformatutils.cpp
//

extern const UINT32 UnpremultiplyTable[];

UINT32 Premultiply(_In_ UINT32 argb);
UINT32 Unpremultiply(_In_ UINT32 argb);

void Premultiply(_Inout_ XCOLORF *pColor);
void Unpremultiply(_Inout_ XCOLORF *pColor);

//+----------------------------------------------------------------------------
//
//  Function:   ByteSaturate
//
//  Synopsis:   Converts an integer to a byte by clamping it to the range
//              that can be represented by a byte//
//  Returns:    UINT8 containing converted integer value
//
//-----------------------------------------------------------------------------
UINT8
ByteSaturate(
    _In_ INT32 i   // Integer value of
    );

//============================================================================
//
// Pixel format conversion prototypes & inline methods
//
//----------------------------------------------------------------------------

//============================================================================
//
// Unaligned & non-inlined pixel format conversions
//
// These functions perform conversions on unaligned memory.
//

//
// scRGB -> sRGB conversions
//

XCOLORF Convert_MILColorF_scRGB_To_MILColorF_sRGB(_In_ const XCOLORF * pColor);
UINT32 Convert_MILColorF_scRGB_To_MILColor_sRGB(_In_ const XCOLORF* pColor);
UINT32 Convert_MILColorF_scRGB_To_Premultiplied_MILColor_sRGB(_In_ const XCOLORF* pColor);

//============================================================================
//
// Aligned pixel format conversions
//
// These functions do not perform UNALIGNED casts.  Thus, the input
// must be guaranteed to be aligned.
//

//
// scRGB -> sRGB conversions
//

UINT16 Convert_scRGB_float_To_sRGB_UINT16(_In_ FLOAT v);

// Definition for Convert_scRGB_Channel_To_sRGB_Byte
#include "gammaluts.h"

//+----------------------------------------------------------------------------
//
//  Function:  Convert_scRGB_Channel_To_sRGB_Float
//
//  Synopsis:  Convert a color channel from scRGB to "sRGB XFLOAT" (0.0f to 1.0f)
//
//  Returns:   Converted color channel
//
//-----------------------------------------------------------------------------
FLOAT Convert_scRGB_Channel_To_sRGB_Float(
    _In_ FLOAT rColorComponent
    );

//+---------------------------------------------------------------------------
//
//  Member:    Inline_Convert_MILColorF_scRGB_To_MILColor_sRGB
//
//  Synopsis:  Converts a non-premultiplied scRGB XCOLORF to sRGB XUINT32 inline
//             without performing unaligned casts.
//
//             The premultiplication state is important when converting between
//             different color spaces because the conversion must be done on
//             non-premultiplied colors.
//
//-----------------------------------------------------------------------------
UINT32
Inline_Convert_MILColorF_scRGB_To_MILColor_sRGB(
    _In_ const XCOLORF *pInputColor
    );

//+---------------------------------------------------------------------------
//
//  Member:    Inline_Convert_Premultiplied_MILColorF_scRGB_To_Premultipled_MILColor_sRGB
//
//  Synopsis:  Converts a premultipled scRGB XCOLORF to sRGB XUINT32 inline
//             without performing unaligned casts.
//
//             The premultiplication state is important when converting between
//             different color spaces because the conversion must be done on
//             non-premultiplied colors.
//
//-----------------------------------------------------------------------------
UINT32
Inline_Convert_Premultiplied_MILColorF_scRGB_To_Premultipled_MILColor_sRGB(
    _In_ const XCOLORF *pInputColor
    );

//
// sRGB -> sRGB conversions
//

//+---------------------------------------------------------------------------
//
//  Member:    Inline_Convert_MILColorF_sRGB_To_MILColor_sRGB
//
//  Synopsis:  Converts a sRGB XCOLORF to a sRGB XUINT32 inline
//             without performing unaligned casts.
//
//-----------------------------------------------------------------------------
UINT32
Inline_Convert_MILColorF_sRGB_To_MILColor_sRGB(
    _In_ const XCOLORF *pInputColor
    );

//+---------------------------------------------------------------------------
//
//  Member:    Inline_Convert_MILColor_sRGB_To_AGRB64TEXEL_sRGB
//
//  Synopsis:  Converts a sRGB XUINT32 to a sRGB AGRB64TEXEL.
//
//-----------------------------------------------------------------------------
void
Inline_Convert_MILColor_sRGB_To_AGRB64TEXEL_sRGB(
    _In_ const UINT32 inputColor,
    _Out_ AGRB64TEXEL *pOutputColor
    );

//
// sRGB -> scRGB conversions
//

FLOAT Convert_sRGB_UINT16_To_scRGB_float(UINT16 v);

//
// Conversions within scRGB between different types
//

INT16 Convert_scRGB_float_To_scRGB_INT16(FLOAT v);
INT32 Convert_scRGB_float_To_scRGB_INT32(FLOAT v);
INT16 Convert_scRGB_INT32_To_scRGB_INT16(INT32 v);

//+-----------------------------------------------------------------------------
//
//  Struct:    OpacityToBlendInt
//
//  Synopsis:
//      Converts a 0..1 float opacity to an integer we can use for rasterizer
//      blending
//
//------------------------------------------------------------------------------

UINT32
OpacityToBlendInt(FLOAT rOpacity);

