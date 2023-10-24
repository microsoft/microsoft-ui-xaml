// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// Pixel utility & conversion functions for various formats.
#include "precomp.h"
#include <pixelformatutils.h>
#include <paltypes.h>
#include <real.h>
#include <GammaLUTs.h>
//
// Lookup tables
//

// UnpremultiplyTable[x] = floor(65536 * (255.0 / x))
const UINT32 UnpremultiplyTable[256] =
{
    0x000000,0xff0000,0x7f8000,0x550000,0x3fc000,0x330000,0x2a8000,0x246db6,
    0x1fe000,0x1c5555,0x198000,0x172e8b,0x154000,0x139d89,0x1236db,0x110000,
    0x0ff000,0x0f0000,0x0e2aaa,0x0d6bca,0x0cc000,0x0c2492,0x0b9745,0x0b1642,
    0x0aa000,0x0a3333,0x09cec4,0x0971c7,0x091b6d,0x08cb08,0x088000,0x0839ce,
    0x07f800,0x07ba2e,0x078000,0x074924,0x071555,0x06e453,0x06b5e5,0x0689d8,
    0x066000,0x063831,0x061249,0x05ee23,0x05cba2,0x05aaaa,0x058b21,0x056cef,
    0x055000,0x05343e,0x051999,0x050000,0x04e762,0x04cfb2,0x04b8e3,0x04a2e8,
    0x048db6,0x047943,0x046584,0x045270,0x044000,0x042e29,0x041ce7,0x040c30,
    0x03fc00,0x03ec4e,0x03dd17,0x03ce54,0x03c000,0x03b216,0x03a492,0x03976f,
    0x038aaa,0x037e3f,0x037229,0x036666,0x035af2,0x034fca,0x0344ec,0x033a54,
    0x033000,0x0325ed,0x031c18,0x031281,0x030924,0x030000,0x02f711,0x02ee58,
    0x02e5d1,0x02dd7b,0x02d555,0x02cd5c,0x02c590,0x02bdef,0x02b677,0x02af28,
    0x02a800,0x02a0fd,0x029a1f,0x029364,0x028ccc,0x028656,0x028000,0x0279c9,
    0x0273b1,0x026db6,0x0267d9,0x026217,0x025c71,0x0256e6,0x025174,0x024c1b,
    0x0246db,0x0241b2,0x023ca1,0x0237a6,0x0232c2,0x022df2,0x022938,0x022492,
    0x022000,0x021b81,0x021714,0x0212bb,0x020e73,0x020a3d,0x020618,0x020204,
    0x01fe00,0x01fa0b,0x01f627,0x01f252,0x01ee8b,0x01ead3,0x01e72a,0x01e38e,
    0x01e000,0x01dc7f,0x01d90b,0x01d5a3,0x01d249,0x01cefa,0x01cbb7,0x01c880,
    0x01c555,0x01c234,0x01bf1f,0x01bc14,0x01b914,0x01b61e,0x01b333,0x01b051,
    0x01ad79,0x01aaaa,0x01a7e5,0x01a529,0x01a276,0x019fcb,0x019d2a,0x019a90,
    0x019800,0x019577,0x0192f6,0x01907d,0x018e0c,0x018ba2,0x018940,0x0186e5,
    0x018492,0x018245,0x018000,0x017dc1,0x017b88,0x017957,0x01772c,0x017507,
    0x0172e8,0x0170d0,0x016ebd,0x016cb1,0x016aaa,0x0168a9,0x0166ae,0x0164b8,
    0x0162c8,0x0160dd,0x015ef7,0x015d17,0x015b3b,0x015965,0x015794,0x0155c7,
    0x015400,0x01523d,0x01507e,0x014ec4,0x014d0f,0x014b5e,0x0149b2,0x01480a,
    0x014666,0x0144c6,0x01432b,0x014193,0x014000,0x013e70,0x013ce4,0x013b5c,
    0x0139d8,0x013858,0x0136db,0x013562,0x0133ec,0x01327a,0x01310b,0x012fa0,
    0x012e38,0x012cd4,0x012b73,0x012a15,0x0128ba,0x012762,0x01260d,0x0124bc,
    0x01236d,0x012222,0x0120d9,0x011f93,0x011e50,0x011d10,0x011bd3,0x011a98,
    0x011961,0x01182b,0x0116f9,0x0115c9,0x01149c,0x011371,0x011249,0x011123,
    0x011000,0x010edf,0x010dc0,0x010ca4,0x010b8a,0x010a72,0x01095d,0x01084a,
    0x010739,0x01062b,0x01051e,0x010414,0x01030c,0x010206,0x010102,0x010000,
};

//+-----------------------------------------------------------------------------
//
//  Function:   Unpremultiply
//
//  Synopsis:   Unpremultiplies an XUINT32 value
//
//  Returns:    Unpremultipied XUINT32 value
//
//----------------------------------------------------------------------------
UINT32
Unpremultiply(
    _In_ UINT32 argb   // Premultied XUINT32 value to convert
    )
{
// Get alpha value

    UINT32 a = argb >> MIL_ALPHA_SHIFT;

// Special case: fully transparent or fully opaque

    if (a == 0 || a == 255)
        return argb;

    UINT32 f = UnpremultiplyTable[a];

    UINT32 r = ((argb >> MIL_RED_SHIFT) & 0xff) * f >> 16;
    UINT32 g = ((argb >> MIL_GREEN_SHIFT) & 0xff) * f >> 16;
    UINT32 b = ((argb >> MIL_BLUE_SHIFT) & 0xff) * f >> 16;

    return (a << MIL_ALPHA_SHIFT) |
           ((r > 255 ? 255 : r) << MIL_RED_SHIFT) |
           ((g > 255 ? 255 : g) << MIL_GREEN_SHIFT) |
           ((b > 255 ? 255 : b) << MIL_BLUE_SHIFT);
}

//+-----------------------------------------------------------------------------
//
//  Function:   Premultiply
//
//  Synopsis:   Premultiplies an XUINT32 value
//
//  Returns:    Premultipied XUINT32 value
//
//----------------------------------------------------------------------------
UINT32
Premultiply(
    _In_ UINT32 argb   // Non-premultiplied XUINT32 value
    )
{
    UINT32 a = (argb >> MIL_ALPHA_SHIFT);

    if (a == 255)
        return argb;
    else if (a == 0)
        return 0;

    UINT32 _000000gg = (argb >> 8) & 0x000000ff;
    UINT32 _00rr00bb = (argb & 0x00ff00ff);

    UINT32 _0000gggg = _000000gg * a + 0x00000080;
    _0000gggg += (_0000gggg >> 8);

    UINT32 _rrrrbbbb = _00rr00bb * a + 0x00800080;
    _rrrrbbbb += ((_rrrrbbbb >> 8) & 0x00ff00ff);

    return (a << MIL_ALPHA_SHIFT) |
           (_0000gggg & 0x0000ff00) |
           ((_rrrrbbbb >> 8) & 0x00ff00ff);
}

//+-----------------------------------------------------------------------------
//
//  Function:   Unpremultiply
//
//  Synopsis:   Unpremultiplies a XCOLORF
//
//  Returns:    void
//
//----------------------------------------------------------------------------
void
Unpremultiply(
    _Inout_ XCOLORF *pColor // Color to unpremultiply
    )
{
    if (pColor->a > 0)
    {
        pColor->r /= pColor->a;
        pColor->g /= pColor->a;
        pColor->b /= pColor->a;
    }
}

//+-----------------------------------------------------------------------------
//
//  Function:   Premultiply
//
//  Synopsis:   Premultiplies a XCOLORF
//
//  Returns:    Premultipied XCOLORF
//
//------------------------------------------------------------------------------
void
Premultiply(
    _Inout_ XCOLORF *pColor   // Color to premultiply
    )
{
    pColor->r *= pColor->a;
    pColor->g *= pColor->a;
    pColor->b *= pColor->a;
}


//+-----------------------------------------------------------------------------
//
//  Function:   Convert_MILColorF_scRGB_To_MILColorF_sRGB
//
//  Synopsis:   Given a non-premultiplied scRGB XCOLORF return a non-premultiplied
//              XCOLORF in normalized sRGB space.
//
//              This method does not require aligned memory.  It does an alignment
//              cast before conversion.
//
//              The premultiplication state is important when converting between
//              different color spaces because the conversion must be done on
//              non-premultiplied colors.
//
//  Returns:    sRGB XCOLORF
//
//------------------------------------------------------------------------------
XCOLORF
Convert_MILColorF_scRGB_To_MILColorF_sRGB(
    _In_ const XCOLORF * pColor    // scRGB XCOLORF to convert
    )
{
    ASSERT(pColor != nullptr);

    XCOLORF colorf = *pColor;

    colorf.a = ClampReal(pColor->a, 0.0f, 1.0f);

    colorf.r = Convert_scRGB_Channel_To_sRGB_Float(pColor->r);
    colorf.g = Convert_scRGB_Channel_To_sRGB_Float(pColor->g);
    colorf.b = Convert_scRGB_Channel_To_sRGB_Float(pColor->b);

    return colorf;
}


//+---------------------------------------------------------------------------
//
//  Function:   Convert_sRGB_UINT16_To_scRGB_float
//
//  Synopsis:   Convert non-premultiplied unsigned 16-bit value in sRGB space,
//              in the range 0 <= value <= 0xFFFF,
//              to a non-premultiplied 32-bit floating point value in 1.0 gamma space,
//              in the range 0 <= value <= 1.
//
//  Note:       sRGB values are close but not equal to ones
//              in gamma 2.2 space.
//
//              The premultiplication state is important when converting between
//              different color spaces because the conversion must be done on
//              non-premultiplied colors.
//
//  Returns:    scRGB float
//
//----------------------------------------------------------------------------
FLOAT
Convert_sRGB_UINT16_To_scRGB_float(UINT16 v)
{
    // We'll use Gamma_sRGBLUT[256] table with linear
    // interpolation between two neighboring values.
    // The first step is to map the range [0,0xffff]
    // to fixed point 16.16 value so as 0xffff will
    // precisely give 255.0. You may verify that
    // ratio*0xFFFF = 0xFF0000FF so it works.

    static const UINT32 ratio = 0xFF01;  // == (0xFF000000/0xFFFF)
                                       // [but rounded up instead of down]
    UINT32 v16_16 = (ratio*v) >> 8;

    UINT32 index = v16_16 >> 16;
    UINT32 fraction = v16_16 & 0xFFFF;

    DOUBLE r = GammaLUT_sRGB_to_scRGB[index];
    if (fraction)
    {
        r += (GammaLUT_sRGB_to_scRGB[index+1] - r)*fraction*(1./0x10000);
    }

    r *= 1./255;
    ASSERT(r >= 0 && r <= 1);

    return static_cast<FLOAT>(r);
}

UINT32
OpacityToBlendInt(FLOAT rOpacity)
{
    UINT32 uOpacity;

// Any opacity that needs to be blended is in the range of 0..256 so that
// the multiply and shift by 8 works.  (Shift by 8 is a 256 divide).  Also
// this lets us convert from float without loss of precision.
//
// Opacity in color channels is 0..255 range.

    if (rOpacity <= 0.0f)
    {
        uOpacity = 0;
    }
    else if (rOpacity >= 1.0f)
    {
        uOpacity = 256;
    }
    else
    {
        uOpacity = static_cast<UINT32>(XcpRound(rOpacity * 256.0f));
    }

    return uOpacity;
}

UINT8
ByteSaturate(
    _In_ INT32 i   // Integer value of
    )
{
    return static_cast<UINT8>(ClampInteger(i, 0, 255));
}

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
    )
{
    return static_cast<FLOAT>(Convert_scRGB_Channel_To_sRGB_Byte(rColorComponent))/255.0f;
}

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
    _In_ const XCOLORF *pInputColor)
{
// Convert XCOLORF scRGB color channels to XUINT32 sRGB

    return MIL_COLOR(
        ByteSaturate(XcpRound(255.0f*pInputColor->a)),
        Convert_scRGB_Channel_To_sRGB_Byte(pInputColor->r),
        Convert_scRGB_Channel_To_sRGB_Byte(pInputColor->g),
        Convert_scRGB_Channel_To_sRGB_Byte(pInputColor->b)
        );
}

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
    _In_ const XCOLORF *pInputColor)
{
    XCOLORF unPremultipliedColor = *pInputColor;
    Unpremultiply(&unPremultipliedColor);

    return Premultiply(
        Inline_Convert_MILColorF_scRGB_To_MILColor_sRGB(&unPremultipliedColor)
        );
}

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
    _In_ const XCOLORF *pInputColor)
{
    return MIL_COLOR(
        ByteSaturate(XcpRound(pInputColor->a*255.0f)),
        ByteSaturate(XcpRound(pInputColor->r*255.0f)),
        ByteSaturate(XcpRound(pInputColor->g*255.0f)),
        ByteSaturate(XcpRound(pInputColor->b*255.0f))
        );
}

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
    )
{
    pOutputColor->A00aa00gg =
        (MIL_COLOR_GET_ALPHA(inputColor) << 16) | MIL_COLOR_GET_GREEN(inputColor);

    pOutputColor->A00rr00bb =
        (MIL_COLOR_GET_RED(inputColor) << 16) | MIL_COLOR_GET_BLUE(inputColor);

}
