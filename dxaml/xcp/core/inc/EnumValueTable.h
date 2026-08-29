// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <xtable.h>
#include <EnumDefs.h>
#include <EnumValueTable.g.h>

static const XTABLE satBoolean[] =
{
    { XSTRING_PTR_STORAGE(L"False"), 0 },
    { XSTRING_PTR_STORAGE(L"True"), 1 }
};

static const XTABLE satMouseCursor[] =
{
    { XSTRING_PTR_STORAGE(L"Default"),    XINT32(MouseCursorDefault) },
    { XSTRING_PTR_STORAGE(L"Arrow"),      XINT32(MouseCursorArrow) },
    { XSTRING_PTR_STORAGE(L"Hand"),       XINT32(MouseCursorHand) },
    { XSTRING_PTR_STORAGE(L"Wait"),       XINT32(MouseCursorWait) },
    { XSTRING_PTR_STORAGE(L"IBeam"),      XINT32(MouseCursorIBeam) },
    { XSTRING_PTR_STORAGE(L"Stylus"),     XINT32(MouseCursorStylus) },
    { XSTRING_PTR_STORAGE(L"Eraser"),     XINT32(MouseCursorEraser) },
    { XSTRING_PTR_STORAGE(L"SizeNS"),     XINT32(MouseCursorSizeNS) },
    { XSTRING_PTR_STORAGE(L"SizeWE"),     XINT32(MouseCursorSizeWE) },
    { XSTRING_PTR_STORAGE(L"SizeNESW"),   XINT32(MouseCursorSizeNESW) },
    { XSTRING_PTR_STORAGE(L"SizeNWSE"),   XINT32(MouseCursorSizeNWSE) },
    { XSTRING_PTR_STORAGE(L"None"),       XINT32(MouseCursorNone) }
};

static const XTABLE satFontWeight[] =
{
    { XSTRING_PTR_STORAGE(L"Thin"), /* WeightThin */ 100 },
    { XSTRING_PTR_STORAGE(L"ExtraLight"), /* WeightExtraLight */ 200 },
    { XSTRING_PTR_STORAGE(L"Light"), /* WeightLight */ 300 },
    { XSTRING_PTR_STORAGE(L"SemiLight"), /* WeightSemiLight */ 350 },
    { XSTRING_PTR_STORAGE(L"Normal"), /* WeightNormal */ 400 },
    { XSTRING_PTR_STORAGE(L"Medium"), /* WeightMedium */ 500 },
    { XSTRING_PTR_STORAGE(L"SemiBold"), /* WeightSemiBold */ 600 },
    { XSTRING_PTR_STORAGE(L"Bold"), /* WeightBold */ 700 },
    { XSTRING_PTR_STORAGE(L"ExtraBold"), /* WeightExtraBold */ 800 },
    { XSTRING_PTR_STORAGE(L"Black"), /* WeightBlack */ 900 },
    { XSTRING_PTR_STORAGE(L"ExtraBlack"), /* WeightExtraBlack */ 950 },
};
