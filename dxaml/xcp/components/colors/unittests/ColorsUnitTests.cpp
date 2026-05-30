// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ColorsUnitTests.h"
#include <XamlLogging.h>
#include <KnownColors.h>
#include <CColor.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Colors {
    void VerifyKnownColorNameMatches(const WCHAR* colorName, KnownColors colorValue)
    {
        static_assert(std::is_same<UINT32, std::underlying_type<KnownColors>::type>::value, "KnownColors is not UINT32");
        UINT32 rgbResult;
        VERIFY_SUCCEEDED(CColor::ColorFromString(xephemeral_string_ptr(colorName, static_cast<XUINT32>(wcslen(colorName))), &rgbResult), colorName);
        VERIFY_ARE_EQUAL(rgbResult, static_cast<UINT32>(colorValue));
    }

#define MATCH_COLOR(name) VerifyKnownColorNameMatches(L#name, KnownColors::##name);

    void ColorsUnitTests::ValidateKnownColorNamesMatchRGBA()
    {
        MATCH_COLOR(AliceBlue);
        MATCH_COLOR(AntiqueWhite);
        MATCH_COLOR(Aqua);
        MATCH_COLOR(Aquamarine);
        MATCH_COLOR(Azure);
        MATCH_COLOR(Beige);
        MATCH_COLOR(Bisque);
        MATCH_COLOR(Black);
        MATCH_COLOR(BlanchedAlmond);
        MATCH_COLOR(Blue);
        MATCH_COLOR(BlueViolet);
        MATCH_COLOR(Brown);
        MATCH_COLOR(BurlyWood);
        MATCH_COLOR(CadetBlue);
        MATCH_COLOR(Chartreuse);
        MATCH_COLOR(Chocolate);
        MATCH_COLOR(Coral);
        MATCH_COLOR(CornflowerBlue);
        MATCH_COLOR(Cornsilk);
        MATCH_COLOR(Crimson);
        MATCH_COLOR(Cyan);
        MATCH_COLOR(DarkBlue);
        MATCH_COLOR(DarkCyan);
        MATCH_COLOR(DarkGoldenrod);
        MATCH_COLOR(DarkGray);
        MATCH_COLOR(DarkGreen);
        MATCH_COLOR(DarkKhaki);
        MATCH_COLOR(DarkMagenta);
        MATCH_COLOR(DarkOliveGreen);
        MATCH_COLOR(DarkOrange);
        MATCH_COLOR(DarkOrchid);
        MATCH_COLOR(DarkRed);
        MATCH_COLOR(DarkSalmon);
        MATCH_COLOR(DarkSeaGreen);
        MATCH_COLOR(DarkSlateBlue);
        MATCH_COLOR(DarkSlateGray);
        MATCH_COLOR(DarkTurquoise);
        MATCH_COLOR(DarkViolet);
        MATCH_COLOR(DeepPink);
        MATCH_COLOR(DeepSkyBlue);
        MATCH_COLOR(DimGray);
        MATCH_COLOR(DodgerBlue);
        MATCH_COLOR(Firebrick);
        MATCH_COLOR(FloralWhite);
        MATCH_COLOR(ForestGreen);
        MATCH_COLOR(Fuchsia);
        MATCH_COLOR(Gainsboro);
        MATCH_COLOR(GhostWhite);
        MATCH_COLOR(Gold);
        MATCH_COLOR(Goldenrod);
        MATCH_COLOR(Gray);
        MATCH_COLOR(Green);
        MATCH_COLOR(GreenYellow);
        MATCH_COLOR(Honeydew);
        MATCH_COLOR(HotPink);
        MATCH_COLOR(IndianRed);
        MATCH_COLOR(Indigo);
        MATCH_COLOR(Ivory);
        MATCH_COLOR(Khaki);
        MATCH_COLOR(Lavender);
        MATCH_COLOR(LavenderBlush);
        MATCH_COLOR(LawnGreen);
        MATCH_COLOR(LemonChiffon);
        MATCH_COLOR(LightBlue);
        MATCH_COLOR(LightCoral);
        MATCH_COLOR(LightCyan);
        MATCH_COLOR(LightGoldenrodYellow);
        MATCH_COLOR(LightGray);
        MATCH_COLOR(LightGreen);
        MATCH_COLOR(LightPink);
        MATCH_COLOR(LightSalmon);
        MATCH_COLOR(LightSeaGreen);
        MATCH_COLOR(LightSkyBlue);
        MATCH_COLOR(LightSlateGray);
        MATCH_COLOR(LightSteelBlue);
        MATCH_COLOR(LightYellow);
        MATCH_COLOR(Lime);
        MATCH_COLOR(LimeGreen);
        MATCH_COLOR(Linen);
        MATCH_COLOR(Magenta);
        MATCH_COLOR(Maroon);
        MATCH_COLOR(MediumAquamarine);
        MATCH_COLOR(MediumBlue);
        MATCH_COLOR(MediumOrchid);
        MATCH_COLOR(MediumPurple);
        MATCH_COLOR(MediumSeaGreen);
        MATCH_COLOR(MediumSlateBlue);
        MATCH_COLOR(MediumSpringGreen);
        MATCH_COLOR(MediumTurquoise);
        MATCH_COLOR(MediumVioletRed);
        MATCH_COLOR(MidnightBlue);
        MATCH_COLOR(MintCream);
        MATCH_COLOR(MistyRose);
        MATCH_COLOR(Moccasin);
        MATCH_COLOR(NavajoWhite);
        MATCH_COLOR(Navy);
        MATCH_COLOR(OldLace);
        MATCH_COLOR(Olive);
        MATCH_COLOR(OliveDrab);
        MATCH_COLOR(Orange);
        MATCH_COLOR(OrangeRed);
        MATCH_COLOR(Orchid);
        MATCH_COLOR(PaleGoldenrod);
        MATCH_COLOR(PaleGreen);
        MATCH_COLOR(PaleTurquoise);
        MATCH_COLOR(PaleVioletRed);
        MATCH_COLOR(PapayaWhip);
        MATCH_COLOR(PeachPuff);
        MATCH_COLOR(Peru);
        MATCH_COLOR(Pink);
        MATCH_COLOR(Plum);
        MATCH_COLOR(PowderBlue);
        MATCH_COLOR(Purple);
        MATCH_COLOR(Red);
        MATCH_COLOR(RosyBrown);
        MATCH_COLOR(RoyalBlue);
        MATCH_COLOR(SaddleBrown);
        MATCH_COLOR(Salmon);
        MATCH_COLOR(SandyBrown);
        MATCH_COLOR(SeaGreen);
        MATCH_COLOR(SeaShell);
        MATCH_COLOR(Sienna);
        MATCH_COLOR(Silver);
        MATCH_COLOR(SkyBlue);
        MATCH_COLOR(SlateBlue);
        MATCH_COLOR(SlateGray);
        MATCH_COLOR(Snow);
        MATCH_COLOR(SpringGreen);
        MATCH_COLOR(SteelBlue);
        MATCH_COLOR(Tan);
        MATCH_COLOR(Teal);
        MATCH_COLOR(Thistle);
        MATCH_COLOR(Tomato);
        MATCH_COLOR(Transparent);
        MATCH_COLOR(Turquoise);
        MATCH_COLOR(Violet);
        MATCH_COLOR(Wheat);
        MATCH_COLOR(White);
        MATCH_COLOR(WhiteSmoke);
        MATCH_COLOR(Yellow);
        MATCH_COLOR(YellowGreen);
    }

    void ColorsUnitTests::ThrowIfColorNameDoesNotMatchAnyKnownColors()
    {
        const WCHAR* colorName = L"Foo";
        UINT32 rgbResult;
        HRESULT hr = CColor::ColorFromString(xephemeral_string_ptr(colorName, static_cast<XUINT32>(wcslen(colorName))), &rgbResult);
        VERIFY_ARE_EQUAL(E_UNEXPECTED, hr);
    }

    void ColorsUnitTests::ValidateColorMatchesForTrailingSpaces()
    {
        const WCHAR* colorName = L"White   ";
        UINT32 rgbResult;
        HRESULT hr = CColor::ColorFromString(xephemeral_string_ptr(colorName, static_cast<XUINT32>(wcslen(colorName))), &rgbResult);
        VERIFY_ARE_EQUAL(S_OK, hr);
    }

    void ColorsUnitTests::ValidateColorMatchesForLeadingSpaces()
    {
        const WCHAR* colorName = L"   White";
        UINT32 rgbResult;
        HRESULT hr = CColor::ColorFromString(xephemeral_string_ptr(colorName, static_cast<XUINT32>(wcslen(colorName))), &rgbResult);
        VERIFY_ARE_EQUAL(S_OK, hr);
    }

    void ColorsUnitTests::ValidateColorMatchesForTrailingAndLeadingSpaces()
    {
        const WCHAR* colorName = L"   White   ";
        UINT32 rgbResult;
        HRESULT hr = CColor::ColorFromString(xephemeral_string_ptr(colorName, static_cast<XUINT32>(wcslen(colorName))), &rgbResult);
        VERIFY_ARE_EQUAL(S_OK, hr);
    }

} } } } }