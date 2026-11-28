// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ColorStringTests.h"
#include <XamlTailored.h>
#include <TestCleanupWrapper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

namespace
{
    Platform::String^ MakeBrushMarkup(Platform::String^ colorString)
    {
        return L"<SolidColorBrush xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' Color='" + colorString + L"'/>";
    }

    // Parses colorString as the Color of a SolidColorBrush in markup and verifies the resulting color.
    void VerifyColorString(Platform::String^ colorString, unsigned char a, unsigned char r, unsigned char g, unsigned char b)
    {
        LOG_OUTPUT(L"Parsing color string \"%s\", expecting #%02X%02X%02X%02X", colorString->Data(), a, r, g, b);

        auto brush = safe_cast<xaml_media::SolidColorBrush^>(xaml_markup::XamlReader::Load(MakeBrushMarkup(colorString)));
        const ::Windows::UI::Color color = brush->Color;

        LOG_OUTPUT(L"Parsed color #%02X%02X%02X%02X", color.A, color.R, color.G, color.B);
        VERIFY_ARE_EQUAL(a, color.A);
        VERIFY_ARE_EQUAL(r, color.R);
        VERIFY_ARE_EQUAL(g, color.G);
        VERIFY_ARE_EQUAL(b, color.B);
    }

    // Verifies that colorString is not a valid color: loading the markup fails.
    void VerifyInvalidColorString(Platform::String^ colorString)
    {
        LOG_OUTPUT(L"Parsing invalid color string \"%s\"", colorString->Data());

        bool threw = false;
        try
        {
            xaml_markup::XamlReader::Load(MakeBrushMarkup(colorString));
        }
        catch (Platform::Exception^ e)
        {
            LOG_OUTPUT(L"XamlReader.Load failed as expected with 0x%08X", e->HResult);
            threw = true;
        }
        VERIFY_IS_TRUE(threw);
    }
}

bool ColorStringTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool ColorStringTests::TestSetup()
{
    TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool ColorStringTests::TestCleanup()
{
    TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

void ColorStringTests::ShortHexColorStrings()
{
    TestCleanupWrapper cleanup;

    RunOnUIThread([]()
    {
        // #RGB: each digit is doubled and the color is opaque.
        VerifyColorString(L"#123", 0xFF, 0x11, 0x22, 0x33);
        VerifyColorString(L"#F0A", 0xFF, 0xFF, 0x00, 0xAA);

        // #ARGB: each digit, including alpha, is doubled.
        VerifyColorString(L"#4567", 0x44, 0x55, 0x66, 0x77);
        VerifyColorString(L"#0E9C", 0x00, 0xEE, 0x99, 0xCC);
        VerifyColorString(L"#8ABC", 0x88, 0xAA, 0xBB, 0xCC);    // alpha digit with the high bit set
        VerifyColorString(L"#F123", 0xFF, 0x11, 0x22, 0x33);

        // The long forms keep working next to the short ones.
        VerifyColorString(L"#1A2B3C", 0xFF, 0x1A, 0x2B, 0x3C);
        VerifyColorString(L"#801A2B3C", 0x80, 0x1A, 0x2B, 0x3C);

        // Only 3, 4, 6 and 8 hex digits are valid.
        VerifyInvalidColorString(L"#12");
        VerifyInvalidColorString(L"#12345");
        VerifyInvalidColorString(L"#1234567");
        VerifyInvalidColorString(L"#123456789");
    });
}

void ColorStringTests::ScRgbColorStrings()
{
    TestCleanupWrapper cleanup;

    RunOnUIThread([]()
    {
        // Three channels: R, G, B with an opaque alpha.
        VerifyColorString(L"sc#1,0,1", 0xFF, 0xFF, 0x00, 0xFF);
        VerifyColorString(L"sc#0,1,0", 0xFF, 0x00, 0xFF, 0x00);

        // Four channels: A, R, G, B. White space around values and separators is allowed.
        VerifyColorString(L"sc# 0.2 , 0 , 1 , 0", 0x33, 0x00, 0xFF, 0x00);
        VerifyColorString(L"sc#1,1,0,1", 0xFF, 0xFF, 0x00, 0xFF);

        // Out-of-range channels saturate.
        VerifyColorString(L"sc#2,-1,0", 0xFF, 0xFF, 0x00, 0x00);

        // Values may be separated by white space only.
        VerifyColorString(L"sc#0 0 1", 0xFF, 0x00, 0x00, 0xFF);

        // Fewer than three channels is invalid.
        VerifyInvalidColorString(L"sc#1,0");
        VerifyInvalidColorString(L"sc#0.5");
    });
}

void ColorStringTests::NamedAndInvalidColorStrings()
{
    TestCleanupWrapper cleanup;

    RunOnUIThread([]()
    {
        // Known color names are case-insensitive, and trailing white space is ignored.
        VerifyColorString(L"Red  ", 0xFF, 0xFF, 0x00, 0x00);
        VerifyColorString(L"mediumseagreen", 0xFF, 0x3C, 0xB3, 0x71);
        VerifyColorString(L"Transparent", 0x00, 0xFF, 0xFF, 0xFF);

        // Names that start with another known name ("White", "Blue") resolve to the longer name.
        VerifyColorString(L"WhiteSmoke", 0xFF, 0xF5, 0xF5, 0xF5);
        VerifyColorString(L"blueviolet", 0xFF, 0x8A, 0x2B, 0xE2);

        // Plain integers are not supported color values.
        VerifyInvalidColorString(L"123");
        VerifyInvalidColorString(L"4294901760");

        // Context colors are not supported.
        VerifyInvalidColorString(L"ContextColor foo.icc 1,0,0");

        // Unknown names, including names that only share a prefix with a known color.
        VerifyInvalidColorString(L"NotAColor");
        VerifyInvalidColorString(L"Redd");

        // Prefixes of known names are not names.
        VerifyInvalidColorString(L"Re");
        VerifyInvalidColorString(L"Gree");
    });
}

} } } } } }
