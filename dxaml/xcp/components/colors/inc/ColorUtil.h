// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <NamespaceAliases.h>

const INT32 MIL_ALPHA_SHIFT = 24;
const INT32 MIL_RED_SHIFT = 16;
const INT32 MIL_GREEN_SHIFT = 8;
const INT32 MIL_BLUE_SHIFT = 0;

const UINT32 MIL_ALPHA_MASK = 0xffUL << MIL_ALPHA_SHIFT;
const UINT32 MIL_RED_MASK   = 0xffUL << MIL_RED_SHIFT;
const UINT32 MIL_GREEN_MASK = 0xffUL << MIL_GREEN_SHIFT;
const UINT32 MIL_BLUE_MASK  = 0xffUL << MIL_BLUE_SHIFT;

inline UINT32 MIL_COLOR(UINT32 a, UINT32 r, UINT32 g, UINT32 b)
{
    return (a & 0xFF) << MIL_ALPHA_SHIFT |
        (r & 0xFF) << MIL_RED_SHIFT |
        (g & 0xFF) << MIL_GREEN_SHIFT |
        (b & 0xFF) << MIL_BLUE_SHIFT;
}

inline UINT32 MIL_COLOR_GET_ALPHA(UINT32 c) { return (c & MIL_ALPHA_MASK) >> MIL_ALPHA_SHIFT; }
inline UINT32 MIL_COLOR_GET_RED(UINT32 c)   { return (c & MIL_RED_MASK)   >> MIL_RED_SHIFT; }
inline UINT32 MIL_COLOR_GET_GREEN(UINT32 c) { return (c & MIL_GREEN_MASK) >> MIL_GREEN_SHIFT; }
inline UINT32 MIL_COLOR_GET_BLUE(UINT32 c)  { return (c & MIL_BLUE_MASK)  >> MIL_BLUE_SHIFT; }

inline UINT32 MIL_COLOR_MAKE_ALPHA(UINT32 c) { return (c << MIL_ALPHA_SHIFT) & MIL_ALPHA_MASK; }
inline UINT32 MIL_COLOR_MAKE_RED(UINT32 c)   { return (c << MIL_RED_SHIFT)   & MIL_RED_MASK; }
inline UINT32 MIL_COLOR_MAKE_GREEN(UINT32 c) { return (c << MIL_GREEN_SHIFT) & MIL_GREEN_MASK; }
inline UINT32 MIL_COLOR_MAKE_BLUE(UINT32 c)  { return (c << MIL_BLUE_SHIFT)  & MIL_BLUE_MASK; }

inline unsigned int ConvertFromABGRToARGB(unsigned int colorABGR)
{
    return (colorABGR & 0xFF00FF00) | ((colorABGR & 0xFF) << 16) | ((colorABGR >> 16) & 0xFF);
}

namespace ColorUtils {
    wu::Color GetWUColor(UINT32 argb);

    const wchar_t* GetColorName(const wu::Color& color);
    
    bool IsOpaqueColor(UINT32 color);

    bool IsTransparentColor(UINT32 color);

    UINT32 ApplyAlphaScale(UINT32 argb, double scaleFactor);
}

