// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <CColor.h>
#include <StringConversions.h>
#include <PixelFormatUtils.h>
#include <paltypes.h>
#include <KnownColors.h>

struct KnownColor
{
    const WCHAR* m_strColorStorage;
    KnownColors m_rgb;
};

#define COLOR_ENTRY(name) { L#name, KnownColors::name }

const static KnownColor sakc[] =
{
    COLOR_ENTRY(AliceBlue),
    COLOR_ENTRY(AntiqueWhite),
    COLOR_ENTRY(Aqua),
    COLOR_ENTRY(Aquamarine),
    COLOR_ENTRY(Azure),
    COLOR_ENTRY(Beige),
    COLOR_ENTRY(Bisque),
    COLOR_ENTRY(Black),
    COLOR_ENTRY(BlanchedAlmond),
    COLOR_ENTRY(Blue),
    COLOR_ENTRY(BlueViolet),
    COLOR_ENTRY(Brown),
    COLOR_ENTRY(BurlyWood),
    COLOR_ENTRY(CadetBlue),
    COLOR_ENTRY(Chartreuse),
    COLOR_ENTRY(Chocolate),
    COLOR_ENTRY(Coral),
    COLOR_ENTRY(CornflowerBlue),
    COLOR_ENTRY(Cornsilk),
    COLOR_ENTRY(Crimson),
    COLOR_ENTRY(Cyan),
    COLOR_ENTRY(DarkBlue),
    COLOR_ENTRY(DarkCyan),
    COLOR_ENTRY(DarkGoldenrod),
    COLOR_ENTRY(DarkGray),
    COLOR_ENTRY(DarkGreen),
    COLOR_ENTRY(DarkKhaki),
    COLOR_ENTRY(DarkMagenta),
    COLOR_ENTRY(DarkOliveGreen),
    COLOR_ENTRY(DarkOrange),
    COLOR_ENTRY(DarkOrchid),
    COLOR_ENTRY(DarkRed),
    COLOR_ENTRY(DarkSalmon),
    COLOR_ENTRY(DarkSeaGreen),
    COLOR_ENTRY(DarkSlateBlue),
    COLOR_ENTRY(DarkSlateGray),
    COLOR_ENTRY(DarkTurquoise),
    COLOR_ENTRY(DarkViolet),
    COLOR_ENTRY(DeepPink),
    COLOR_ENTRY(DeepSkyBlue),
    COLOR_ENTRY(DimGray),
    COLOR_ENTRY(DodgerBlue),
    COLOR_ENTRY(Firebrick),
    COLOR_ENTRY(FloralWhite),
    COLOR_ENTRY(ForestGreen),
    COLOR_ENTRY(Fuchsia),
    COLOR_ENTRY(Gainsboro),
    COLOR_ENTRY(GhostWhite),
    COLOR_ENTRY(Gold),
    COLOR_ENTRY(Goldenrod),
    COLOR_ENTRY(Gray),
    COLOR_ENTRY(Green),
    COLOR_ENTRY(GreenYellow),
    COLOR_ENTRY(Honeydew),
    COLOR_ENTRY(HotPink),
    COLOR_ENTRY(IndianRed),
    COLOR_ENTRY(Indigo),
    COLOR_ENTRY(Ivory),
    COLOR_ENTRY(Khaki),
    COLOR_ENTRY(Lavender),
    COLOR_ENTRY(LavenderBlush),
    COLOR_ENTRY(LawnGreen),
    COLOR_ENTRY(LemonChiffon),
    COLOR_ENTRY(LightBlue),
    COLOR_ENTRY(LightCoral),
    COLOR_ENTRY(LightCyan),
    COLOR_ENTRY(LightGoldenrodYellow),
    COLOR_ENTRY(LightGray),
    COLOR_ENTRY(LightGreen),
    COLOR_ENTRY(LightPink),
    COLOR_ENTRY(LightSalmon),
    COLOR_ENTRY(LightSeaGreen),
    COLOR_ENTRY(LightSkyBlue),
    COLOR_ENTRY(LightSlateGray),
    COLOR_ENTRY(LightSteelBlue),
    COLOR_ENTRY(LightYellow),
    COLOR_ENTRY(Lime),
    COLOR_ENTRY(LimeGreen),
    COLOR_ENTRY(Linen),
    COLOR_ENTRY(Magenta),
    COLOR_ENTRY(Maroon),
    COLOR_ENTRY(MediumAquamarine),
    COLOR_ENTRY(MediumBlue),
    COLOR_ENTRY(MediumOrchid),
    COLOR_ENTRY(MediumPurple),
    COLOR_ENTRY(MediumSeaGreen),
    COLOR_ENTRY(MediumSlateBlue),
    COLOR_ENTRY(MediumSpringGreen),
    COLOR_ENTRY(MediumTurquoise),
    COLOR_ENTRY(MediumVioletRed),
    COLOR_ENTRY(MidnightBlue),
    COLOR_ENTRY(MintCream),
    COLOR_ENTRY(MistyRose),
    COLOR_ENTRY(Moccasin),
    COLOR_ENTRY(NavajoWhite),
    COLOR_ENTRY(Navy),
    COLOR_ENTRY(OldLace),
    COLOR_ENTRY(Olive),
    COLOR_ENTRY(OliveDrab),
    COLOR_ENTRY(Orange),
    COLOR_ENTRY(OrangeRed),
    COLOR_ENTRY(Orchid),
    COLOR_ENTRY(PaleGoldenrod),
    COLOR_ENTRY(PaleGreen),
    COLOR_ENTRY(PaleTurquoise),
    COLOR_ENTRY(PaleVioletRed),
    COLOR_ENTRY(PapayaWhip),
    COLOR_ENTRY(PeachPuff),
    COLOR_ENTRY(Peru),
    COLOR_ENTRY(Pink),
    COLOR_ENTRY(Plum),
    COLOR_ENTRY(PowderBlue),
    COLOR_ENTRY(Purple),
    COLOR_ENTRY(Red),
    COLOR_ENTRY(RosyBrown),
    COLOR_ENTRY(RoyalBlue),
    COLOR_ENTRY(SaddleBrown),
    COLOR_ENTRY(Salmon),
    COLOR_ENTRY(SandyBrown),
    COLOR_ENTRY(SeaGreen),
    COLOR_ENTRY(SeaShell),
    COLOR_ENTRY(Sienna),
    COLOR_ENTRY(Silver),
    COLOR_ENTRY(SkyBlue),
    COLOR_ENTRY(SlateBlue),
    COLOR_ENTRY(SlateGray),
    COLOR_ENTRY(Snow),
    COLOR_ENTRY(SpringGreen),
    COLOR_ENTRY(SteelBlue),
    COLOR_ENTRY(Tan),
    COLOR_ENTRY(Teal),
    COLOR_ENTRY(Thistle),
    COLOR_ENTRY(Tomato),
    COLOR_ENTRY(Transparent),
    COLOR_ENTRY(Turquoise),
    COLOR_ENTRY(Violet),
    COLOR_ENTRY(Wheat),
    COLOR_ENTRY(White),
    COLOR_ENTRY(WhiteSmoke),
    COLOR_ENTRY(Yellow),
    COLOR_ENTRY(YellowGreen)
};

_Check_return_ HRESULT CColor::ColorFromString(
_In_ const xstring_ptr_view& str,
_Out_ UINT32 *prgbResult
)
{
    //  Implementation details:
    //      Colors can be listed in several forms.
    //      1) Hex value (e.g. #e0708853)
    //      2) scRGB floats (e.g. sc#0.5,0.75,0.0)
    //      3) Known color names (e.g. Red, Olive, MediumBlue)
    //      4) Context color (e.g. ContextColor foo,bar,huh)
    //
    //  Currently we don't deal with case 4.

    HRESULT hr = E_UNEXPECTED;
    XUINT32 rgb = 0;
    UINT32 cString;
    const WCHAR* pString = str.GetBufferAndCount(&cString);

    // Skip leading spaces
    TrimWhitespace(cString, pString, &cString, &pString);

    // Check for hex formatted colors

    if (cString && (L'#' == *pString))
    {
        pString++;
        cString--;

        XUINT32 hex;
        XUINT32 tmp;

        const XUINT32 cPrevious = cString;

        IFC(UnsignedFromHexString(cString, pString, &cString, &pString, &hex));

        // There are only a limited number of valid hex formats.  One digit each
        // for red, green, and blue with one digit of optional alpha or two digits
        // each for the colors and the optional alpha.  Ignoring the hash mark this
        // gives only four valid lengths to consider (3, 4, 6, and 8).

        switch (cPrevious - cString)
        {
        case 3: // rgb
            rgb = 0xff000000;                  // Make fully opaque
            tmp = (hex & 0x0f00);              // Mask out red value
            tmp *= 17;                          // Scale by 0x11
            rgb |= (tmp << 8);                  // Store it
            tmp = (hex & 0x00f0);              // Mask out green value
            tmp *= 17;                          // Scale by 0x11
            rgb |= (tmp << 4);                  // Store it
            tmp = (hex & 0x000f);              // Mask out blue value
            tmp *= 17;                          // Scale by 0x11
            rgb |= tmp;                         // Store it
            break;

        case 4: // argb
            tmp = (hex & 0xf000);              // Mask out alpha value
            tmp *= 17;                          // Scale by 0x11
            rgb = (tmp << 12);                 // Store it
            tmp = (hex & 0x0f00);              // Mask out red value
            tmp *= 17;                          // Scale by 0x11
            rgb |= (tmp << 8);                  // Store it
            tmp = (hex & 0x00f0);              // Mask out green value
            tmp *= 17;                          // Scale by 0x11
            rgb |= (tmp << 4);                  // Store it
            tmp = (hex & 0x000f);              // Mask out blue value
            tmp *= 17;                          // Scale by 0x11
            rgb |= tmp;                         // Store it
            break;

        case 6: // rrggbb
            rgb = 0xff000000 | hex;             // Make fully opaque
            break;

        case 8: // aarrggbb
            rgb = hex;
            break;

        default:
            return E_UNEXPECTED;
        }
    }

    // Check for scRGB values

    else if ((cString > 2) && (L's' == pString[0]) && (L'c' == pString[1]) && (L'#' == pString[2]))
    {
        XFLOAT  ae[4];
        XUINT32 ce;
        XCOLORF eRGB;

        pString += 3;
        cString -= 3;
        ce = 0;

        TrimWhitespace(cString, pString, &cString, &pString);

        do
        {
            IFC(FloatFromString(cString, pString, &cString, &pString, &ae[ce]));

            ce++;

            // Clear debris from parse string including white spaces and comma.

            TrimWhitespace(cString, pString, &cString, &pString);

            if (cString && L',' == *pString)
            {
                pString++;
                cString--;

                TrimWhitespace(cString, pString, &cString, &pString);
            }
        } while (cString && (ce < 4));

        // We must get either three or four values to be valid.

        switch (ce)
        {
        case 3:
            eRGB.a = 1.0f;
            eRGB.r = ae[0];
            eRGB.g = ae[1];
            eRGB.b = ae[2];
            break;

        case 4:
            eRGB.a = ae[0];
            eRGB.r = ae[1];
            eRGB.g = ae[2];
            eRGB.b = ae[3];
            break;

        default:
            return E_UNEXPECTED;
        }

        rgb = Inline_Convert_MILColorF_scRGB_To_MILColor_sRGB(&eRGB);
    }

    // try to convert the string to an integer
    else if (cString > 0 && xisdigit(pString[0]))
    {
        hr = UnsignedFromDecimalString(cString, pString, &cString, &pString, &rgb);
        if (SUCCEEDED(hr))
        {
            // we don't support the use of straight integers
            IFC(E_UNEXPECTED);
        }
        if (FAILED(hr))
        {
            // If we don't have a decimal number then bail
            rgb = XUINT32(0);
            hr = S_OK;
        }
    }

    // Check for context colors

    else if ((cString > 12) && !wcsncmp(L"ContextColor", pString, 12))
    {
        rgb = XUINT32(0);
    }

    // Check for known color values

    else
    {
        // trim trailing whitespace
        while (cString > 0 && iswspace(pString[cString - 1]))
        {
            cString--;
        }

        const size_t colorCount = std::extent<decltype(sakc)>::value;
        const KnownColor* sakcEnd = sakc + colorCount;

#ifdef DBG
        // Validate that the colors are sorted at least once
        static bool validatedColors = false;
        if (!validatedColors)
        {
            auto sortedUntil = std::is_sorted_until(sakc, sakcEnd,
                [](const KnownColor& lhs, const KnownColor& rhs)
            {
                return _wcsicmp(lhs.m_strColorStorage, rhs.m_strColorStorage) < 0;
            });
            ASSERT(sortedUntil == sakcEnd);
            validatedColors = true;
        }
#endif



        // Binary search doing a string-insensitive search
        auto pos = std::lower_bound(sakc, sakcEnd, pString,
            [cString](const KnownColor& color, const WCHAR* pString)
        {
            // custom comparator for checking a trimmed string
            // first: compare the min of the trimmed string with the KnownColor
            // if they match, then compare the lengths to break the tie
            auto knownColorLength = wcslen(color.m_strColorStorage);
            auto minLength = std::min(static_cast<size_t>(knownColorLength), static_cast<size_t>(cString));
            auto trimmedCompareValue = _wcsnicmp(color.m_strColorStorage, pString, minLength);
            if (trimmedCompareValue == 0)
            {
                return knownColorLength < cString;
            }
            return trimmedCompareValue < 0;
        });

        if (pos != sakcEnd &&
            wcslen(pos->m_strColorStorage) == cString &&
            _wcsnicmp(pos->m_strColorStorage, pString, cString) == 0)
        {
            rgb = static_cast<UINT32>(pos->m_rgb);
        }

        // If we do not find the color in the list of known color values, throw
        if (rgb == 0)
        {
            hr = E_UNEXPECTED;
        }
        else
        {
            hr = S_OK;
        }
    }

    // Return the value to the caller
    *prgbResult = rgb;

Cleanup:
    RRETURN(hr);
}

namespace ColorUtils {
    wu::Color GetWUColor(XUINT32 argb)
    {
        wu::Color color;
        color.A = static_cast<unsigned char>(MIL_COLOR_GET_ALPHA(argb));
        color.R = static_cast<unsigned char>(MIL_COLOR_GET_RED(argb));
        color.G = static_cast<unsigned char>(MIL_COLOR_GET_GREEN(argb));
        color.B = static_cast<unsigned char>(MIL_COLOR_GET_BLUE(argb));
        return color;
    }

    const wchar_t* GetColorName(const wu::Color& color)
    {
        const WCHAR* name = nullptr;
        const auto argb = static_cast<KnownColors>(MIL_COLOR(color.A, color.R, color.G, color.B));
        auto match = std::find_if(std::begin(sakc), std::end(sakc), [argb](const auto& color)
        {
          return argb == color.m_rgb;
        });

        if (match != std::end(sakc))
        {
            name = match->m_strColorStorage;
        }

        return name;
    }

    bool IsOpaqueColor(UINT32 color)
    {
        return ((color & 0xff000000) == 0xff000000);
    }

    bool IsTransparentColor(UINT32 color)
    {
        return (color & 0xff000000) == 0;
    }

    UINT32 ApplyAlphaScale(UINT32 argb, double scaleFactor)
    {
        // Calculate the new alpha then remove and replace
        UINT32 newAlpha = static_cast<UINT32>(MIL_COLOR_GET_ALPHA(argb) * scaleFactor);
        return (argb & ~MIL_ALPHA_MASK) | (newAlpha << MIL_ALPHA_SHIFT);
    }
}
