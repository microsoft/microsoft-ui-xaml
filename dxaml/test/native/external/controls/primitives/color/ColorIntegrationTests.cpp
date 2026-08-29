// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ColorIntegrationTests.h"

#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace Primitives { namespace Color {

    bool ColorIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool ColorIntegrationTests::TestSetup()
    {
        TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool ColorIntegrationTests::TestCleanup()
    {
        TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Color names corresponding to each hue bucket for light, mid, and dark colors:
    //
    Platform::String^ colorNamesLight[] =
    {
        L"Coral",
        L"Rose",
        L"Light orange",
        L"Tan",
        L"Tan",
        L"Light yellow",
        L"Light yellow",
        L"Tan",
        L"Light green",
        L"Lime",
        L"Light green",
        L"Light green",
        L"Aqua",
        L"Sky blue",
        L"Light turquoise",
        L"Pale blue",
        L"Light blue",
        L"Ice blue",
        L"Periwinkle",
        L"Lavender",
        L"Pink",
        L"Tan",
        L"Rose",
    };

    Platform::String^ colorNamesMid[] =
    {
        L"Coral",
        L"Red",
        L"Orange",
        L"Brown",
        L"Tan",
        L"Gold",
        L"Yellow",
        L"Olive green",
        L"Olive green",
        L"Green",
        L"Green",
        L"Bright green",
        L"Teal",
        L"Aqua",
        L"Turquoise",
        L"Pale blue",
        L"Blue",
        L"Blue-gray",
        L"Indigo",
        L"Purple",
        L"Pink",
        L"Brown",
        L"Red",
    };

    Platform::String^ colorNamesDark[] =
    {
        L"Brown",
        L"Dark red",
        L"Brown",
        L"Brown",
        L"Brown",
        L"Dark yellow",
        L"Dark yellow",
        L"Brown",
        L"Dark green",
        L"Dark green",
        L"Dark green",
        L"Dark green",
        L"Dark teal",
        L"Dark teal",
        L"Dark teal",
        L"Dark blue",
        L"Dark blue",
        L"Blue-gray",
        L"Indigo",
        L"Dark purple",
        L"Plum",
        L"Brown",
        L"Dark red",
    };

    // Luminosity limits for low/high luminosity:
    //
    // 1.  Coral - < 130, > 170
    // 2.  Red - < 100, > 170
    // 3.  Orange - < 115, > 170
    // 4.  Brown - < 100, > 155
    // 5.  Tan - < 100, > 170
    // 6.  Gold - < 100, > 170
    // 7.  Yellow - < 110, > 170
    // 8.  Olive green (with brown) - < 75, > 170
    // 9.  Olive green (with green) - < 100, > 170
    // 10. Lime green - < 90, > 115
    // 11. Green - < 100, > 170
    // 12. Bright green - < 100, > 170
    // 13. Teal - < 100, > 170
    // 14. Aqua - < 100, > 170
    // 15. Turquoise - < 80, > 170
    // 16. Pale blue - < 100, > 170
    // 17. Blue - < 100, > 170
    // 18. Blue-gray - < 100, > 170
    // 19. Indigo - < 100, > 150
    // 20. Purple - < 100, > 150
    // 21. Pink - < 100, > 170
    // 22. Brown - < 100, > 140
    // 23. Red - < 100, > 165
    //
    int lowLuminosityLimits[] =
    {
        130, 100, 115, 100, 100, 100, 110,  75, 100,  90, 100, 100, 100, 100,  80, 100, 100, 100, 100, 100, 100, 100, 100
    };

    int highLuminosityLimits[] =
    {
        170, 170, 170, 155, 170, 170, 170, 170, 170, 115, 170, 170, 170, 170, 170, 170, 170, 170, 150, 150, 170, 140, 165
    };

#define BEGIN_SATURATION_RANGE(sMin, sMax) \
        { \
           int minSaturation = sMin; \
           int maxSaturation = sMax; \
           int hMin = 0;

#define END_SATURATION_RANGE() \
        }

#define TEST_NEXT_HUE_RANGE(hMax, colorIndex) \
            LOG_OUTPUT(L"\r\nVerifying hue range %d-%d in saturation range %d-%d. Luminosity limits are %d and %d.", \
                hMin, hMax - 1, minSaturation, maxSaturation, lowLuminosityLimits[colorIndex - 1], highLuminosityLimits[colorIndex - 1]); \
            LOG_OUTPUT(L"Should contain light, mid, and dark colors \"%s\", \"%s\", and \"%s\"...\r\n", \
                colorNamesLight[colorIndex - 1]->Data(), colorNamesMid[colorIndex - 1]->Data(), colorNamesDark[colorIndex - 1]->Data()); \
            \
            ValidateColorNameFromHslRange(hMin, hMax - 1, minSaturation, maxSaturation, highLuminosityLimits[colorIndex - 1] + 1, 240, colorNamesLight[colorIndex - 1]); \
            ValidateColorNameFromHslRange(hMin, hMax - 1, minSaturation, maxSaturation, lowLuminosityLimits[colorIndex - 1], highLuminosityLimits[colorIndex - 1], colorNamesMid[colorIndex - 1]); \
            ValidateColorNameFromHslRange(hMin, hMax - 1, minSaturation, maxSaturation, 20, lowLuminosityLimits[colorIndex - 1], colorNamesDark[colorIndex - 1]); \
            \
            hMin = hMax;

    //
    // Test Cases
    //
    void ColorIntegrationTests::CanRetrieveColorNames()
    {
        TestCleanupWrapper cleanup;

        // Achromatic colors:
        //
        // Black = L < 20
        // White = L > 240
        // Light gray = S <= 20, 170 < L <= 240
        // Gray = S <= 20, 100 < L <= 170
        // Dark gray = S <= 20, 20 <= L <= 100
        LOG_OUTPUT(L"Verifying achromatic colors \"Black\", \"White\", \"Light gray\", \"Gray\", and \"Dark gray\"...\r\n");
        ValidateColorNameFromHslRange(0, 0, 0, 255, 0, 19, L"Black");
        ValidateColorNameFromHslRange(0, 0, 0, 255, 241, 255, L"White");
        ValidateColorNameFromHslRange(0, 0, 0, 20, 171, 240, L"Light gray");
        ValidateColorNameFromHslRange(0, 0, 0, 20, 101, 170, L"Gray");
        ValidateColorNameFromHslRange(0, 0, 0, 20, 20, 100, L"Dark gray");

        // Saturation level 1: 20-75
        //
        // Hue ranges:
        //
        // 0-7 -     1 (Coral)
        // 8-43 -    4 (Brown)
        // 44-62 -   8 (Olive green (with brown))
        // 63-121 -  11 (Green)
        // 122-133 - 13 (Teal)
        // 134-165 - 18 (Blue-gray)
        // 166-175 - 19 (Indigo)
        // 176-240 - 20 (Purple)
        // 241-255 - 22 (Brown)
        //
        BEGIN_SATURATION_RANGE(20, 75)
        TEST_NEXT_HUE_RANGE(8, 1)
        TEST_NEXT_HUE_RANGE(44, 4)
        TEST_NEXT_HUE_RANGE(63, 8)
        TEST_NEXT_HUE_RANGE(122, 11)
        TEST_NEXT_HUE_RANGE(134, 13)
        TEST_NEXT_HUE_RANGE(166, 18)
        TEST_NEXT_HUE_RANGE(176, 19)
        TEST_NEXT_HUE_RANGE(241, 20)
        TEST_NEXT_HUE_RANGE(256, 22)
        END_SATURATION_RANGE()

        // Saturation level 2: 76-115
        //
        // Hue ranges:
        //
        // 0-9 -     2 (Red)
        // 10-31 -   4 (Brown)
        // 32-45 -   5 (Tan)
        // 46-60 -   9 (Olive green (with green))
        // 61-105 -  11 (Green)
        // 106-135 - 13 (Teal)
        // 135-143 - 14 (Aqua)
        // 144-157 - 18 (Blue-gray)
        // 158-165 - 19 (Indigo)
        // 166-240 - 20 (Purple)
        // 241-255 - 23 (Red)
        //
        BEGIN_SATURATION_RANGE(76, 115)
        TEST_NEXT_HUE_RANGE(10, 2)
        TEST_NEXT_HUE_RANGE(32, 4)
        TEST_NEXT_HUE_RANGE(46, 5)
        TEST_NEXT_HUE_RANGE(60, 9)
        TEST_NEXT_HUE_RANGE(120, 11)
        TEST_NEXT_HUE_RANGE(131, 13)
        TEST_NEXT_HUE_RANGE(144, 14)
        TEST_NEXT_HUE_RANGE(158, 18)
        TEST_NEXT_HUE_RANGE(166, 19)
        TEST_NEXT_HUE_RANGE(241, 20)
        TEST_NEXT_HUE_RANGE(256, 23)
        END_SATURATION_RANGE()

        // Saturation level 3: 116-150
        //
        // Hue ranges:
        //
        // 0-7 -     2 (Red)
        // 8-38 -    5 (Tan)
        // 39-45 -   6 (Gold)
        // 46-70 -   10 (Lime green)
        // 71-119 -  11 (Green)
        // 120-130 - 13 (Teal)
        // 131-143 - 14 (Aqua)
        // 144-162 - 17 (Blue)
        // 163-176 - 19 (Indigo)
        // 177-210 - 20 (Purple)
        // 211-248 - 21 (Pink)
        // 249-255 - 23 (Red)
        //
        BEGIN_SATURATION_RANGE(116, 150)
        TEST_NEXT_HUE_RANGE(8, 2)
        TEST_NEXT_HUE_RANGE(39, 5)
        TEST_NEXT_HUE_RANGE(46, 6)
        TEST_NEXT_HUE_RANGE(71, 10)
        TEST_NEXT_HUE_RANGE(120, 11)
        TEST_NEXT_HUE_RANGE(131, 13)
        TEST_NEXT_HUE_RANGE(144, 14)
        TEST_NEXT_HUE_RANGE(163, 17)
        TEST_NEXT_HUE_RANGE(177, 19)
        TEST_NEXT_HUE_RANGE(211, 20)
        TEST_NEXT_HUE_RANGE(249, 21)
        TEST_NEXT_HUE_RANGE(256, 23)
        END_SATURATION_RANGE()

        // Saturation level 4: 151-240
        //
        // Hue ranges:
        //
        // 0-10 -    2 (Red)
        // 11-25 -   3 (Orange)
        // 26-37 -   6 (Gold)
        // 38-44 -   7 (Yellow)
        // 45-55 -   10 (Lime green)
        // 56-99 -   11 (Green)
        // 100-120 - 12 (Bright green)
        // 121-128 - 13 (Teal)
        // 129-139 - 15 (Turquoise)
        // 140-179 - 17 (Blue)
        // 180-223 - 20 (Purple)
        // 224-240 - 21 (Pink)
        // 241-255 - 23 (Red)
        //
        BEGIN_SATURATION_RANGE(151, 240)
        TEST_NEXT_HUE_RANGE(11, 2)
        TEST_NEXT_HUE_RANGE(26, 3)
        TEST_NEXT_HUE_RANGE(38, 6)
        TEST_NEXT_HUE_RANGE(45, 7)
        TEST_NEXT_HUE_RANGE(56, 10)
        TEST_NEXT_HUE_RANGE(100, 11)
        TEST_NEXT_HUE_RANGE(121, 12)
        TEST_NEXT_HUE_RANGE(129, 13)
        TEST_NEXT_HUE_RANGE(140, 15)
        TEST_NEXT_HUE_RANGE(180, 17)
        TEST_NEXT_HUE_RANGE(224, 20)
        TEST_NEXT_HUE_RANGE(241, 21)
        TEST_NEXT_HUE_RANGE(256, 23)
        END_SATURATION_RANGE()

        // Saturation level 5: 241-255
        //
        // Hue ranges:
        //
        // 0-12 -    2 (Red)
        // 13-26 -   3 (Orange)
        // 27-35 -   6 (Gold)
        // 36-44 -   7 (Yellow)
        // 45-58 -   10 (Lime green)
        // 59-117 -  11 (Green)
        // 118-126 - 13 (Teal)
        // 127-135 - 14 (Aqua)
        // 136-141 - 15 (Turquoise)
        // 142-184 - 17 (Blue)
        // 185-215 - 20 (Purple)
        // 216-238 - 21 (Pink)
        // 239-255 - 23 (Red)
        //
        BEGIN_SATURATION_RANGE(241, 255)
        TEST_NEXT_HUE_RANGE(13, 2)
        TEST_NEXT_HUE_RANGE(27, 3)
        TEST_NEXT_HUE_RANGE(36, 6)
        TEST_NEXT_HUE_RANGE(45, 7)
        TEST_NEXT_HUE_RANGE(59, 10)
        TEST_NEXT_HUE_RANGE(118, 11)
        TEST_NEXT_HUE_RANGE(127, 13)
        TEST_NEXT_HUE_RANGE(136, 14)
        TEST_NEXT_HUE_RANGE(142, 15)
        TEST_NEXT_HUE_RANGE(185, 17)
        TEST_NEXT_HUE_RANGE(216, 20)
        TEST_NEXT_HUE_RANGE(239, 21)
        TEST_NEXT_HUE_RANGE(256, 23)
        END_SATURATION_RANGE()
    }

    void ColorIntegrationTests::ValidateColorNameFromHslRange(double hMin, double hMax, double sMin, double sMax, double lMin, double lMax, Platform::String^ expectedName)
    {
        // To test an HSL range, we'll test three positions in the range to verify that they all return the same expected name.
        // Due to possible rounding errors, we can't test the extremeties of the range,
        // so we'll test points at 1/4, 1/2, and 3/4 of the way through the range.
        double h1 = (hMin * 3 + hMax) / 4;
        double s1 = (sMin * 3 + sMax) / 4;
        double l1 = (lMin * 3 + lMax) / 4;
        double h2 = (hMin + hMax) / 2;
        double s2 = (sMin + sMax) / 2;
        double l2 = (lMin + lMax) / 2;
        double h3 = (hMin + hMax * 3) / 4;
        double s3 = (sMin + sMax * 3) / 4;
        double l3 = (lMin + lMax * 3) / 4;

        // Testing indicates that without a buffer of at least 5, we can hit rounding errors that can erroneously
        // make us think we're in a different section than we actually are since we're converting from HSL to RGB
        // and then back to HSL, so we'll nudge any point that is less than 5 away from the extremeties to be
        // at least that far away.
        int bufferSize = 5;

        h1 = h1 < hMin + bufferSize ? hMin + bufferSize : h1;
        s1 = s1 < sMin + bufferSize ? sMin + bufferSize : s1;
        l1 = l1 < lMin + bufferSize ? lMin + bufferSize : l1;

        h3 = h3 > hMax + bufferSize ? hMax - bufferSize : h3;
        s3 = s3 > sMax + bufferSize ? sMax - bufferSize : s3;
        l3 = l3 > lMax + bufferSize ? lMax - bufferSize : l3;

        // We'll only bother testing the 1/4 and 3/4 positions if they're actually meaningful - if applying the buffer
        // pushed us beyond the halfway point, then the range is so small that we'll just test the halfway point.

        if (h1 < h2 && s1 < s2 && l1 < l2)
        {
            ValidateColorNameFromHsl(h1, s1, l1, expectedName);
        }

        ValidateColorNameFromHsl(h2, s2, l2, expectedName);

        if (h3 > h2 && s3 > s2 && l3 > l2)
        {
            ValidateColorNameFromHsl(h3, s3, l3, expectedName);
        }
    }

    void ColorIntegrationTests::ValidateColorNameFromHsl(double h, double s, double l, Platform::String^ expectedName)
    {
        ::Windows::UI::Color color = ColorFromHsl(h, s, l);

        LOG_OUTPUT(L"Testing HSL = (%.0f, %.0f, %.0f), RGB = (%d, %d, %d)", h, s, l, color.R, color.G, color.B);

        Platform::String^ colorName = nullptr;

        RunOnUIThread([&]()
        {
            colorName = Microsoft::UI::Xaml::ColorDisplayNameHelper::ToDisplayName(color);
        });

        LOG_OUTPUT(L"Expected color name: %s", expectedName->Data());
        LOG_OUTPUT(L"Actual color name:   %s", colorName->Data());
        VERIFY_ARE_EQUAL(expectedName, colorName);
    }

    ::Windows::UI::Color ColorIntegrationTests::ColorFromHsl(double h, double s, double l)
    {
        // We'll first convert the HSL values from the range [0, 255] to the range [0, 1].
        h /= 255.0;
        s /= 255.0;
        l /= 255.0;

        // The first thing that we need to do is to determine the chroma, which is a value indicating
        // the extent to which the color diverges from grayscale.  For HSL, you can retrieve this value
        // by multiplying saturation by 1 - |2L - 1|, which is what we initially divided the chroma by
        // to get the saturation in the first place.
        double chroma = (1 - abs(2 * l - 1)) * s;

        // We next need the min channel value.  Since chroma was defined as C = max(R, G, B) - min(R, G, B),
        // and luminance was defined as L = (max(R, G, B) + min(R, G, B)) / 2, we can thus retrieve the min
        // value by doing
        //
        // 2L - C = (max(R, G, B) + min(R, G, B)) - (max(R, G, B) - min(R, G, B))
        // 2L - C = 2 min(R, G, B)
        //
        // so, dividing both sides by 2 and reversing the sides of the equals sign,
        //
        // min(R, G, B) = L - C / 2
        //
        double min = l - chroma / 2;

        // If the chroma is zero, then we have a greyscale color.  In that case, the maximum and the minimum RGB channels
        // have the same value (and, indeed, all of the RGB channels are the same), so we can just immediately return
        // the minimum value as the value of all the channels.
        if (chroma == 0)
        {
            Microsoft::UI::ColorHelper::FromArgb(255, static_cast<byte>(min * 255), static_cast<byte>(min * 255), static_cast<byte>(min * 255));
        }

        // If the chroma is not zero, then we need to continue.  The first step is to figure out
        // what section of the color wheel we're located in.  In order to do that, we'll multiply the hue by 6.
        // The resulting value means we're in one of the following locations:
        //
        // 0 - Between red and yellow.
        // 1 - Between yellow and green.
        // 2 - Between green and cyan.
        // 3 - Between cyan and blue.
        // 4 - Between blue and purple.
        // 5 - Between purple and red.
        //
        // In each of these sextants, one of the RGB channels is completely present, one is partially present, and one is not present.
        // For example, as we transition between red and yellow, red is completely present, green is becoming increasingly present, and blue is not present.
        // Then, as we transition from yellow and green, green is now completely present, red is becoming decreasingly present, and blue is still not present.
        // As we transition from green to cyan, green is still completely present, blue is becoming increasingly present, and red is no longer present.  And so on.
        //
        // To convert from hue to RGB value, we first need to figure out which of the three channels is in which configuration
        // in the sextant that we're located in.  Next, we figure out what value the completely-present color should have.
        // We know that chroma = (max - min), and we know that this color is the max color, so to find its value we simply add
        // min to chroma to retrieve max.  Finally, we consider how far we've transitioned from the pure form of that color
        // to the next color (e.g., how far we are from pure red towards yellow), and give a value to the partially present channel
        // equal to the minimum plus the chroma (i.e., the max minus the min), multiplied by the percentage towards the new color.
        // This gets us a value between the maximum and the minimum representing the partially present channel.
        // Finally, the not-present color must be equal to the minimum value, since it is the one least participating in the overall color.
        int sextant = static_cast<int>(h * 6);
        double intermediateColorPercentage = h * 6 - sextant;
        double max = chroma + min;

        double r = 0;
        double g = 0;
        double b = 0;

        switch (sextant)
        {
        case 0:
            r = max;
            g = min + chroma * intermediateColorPercentage;
            b = min;
            break;
        case 1:
            r = min + chroma * (1 - intermediateColorPercentage);
            g = max;
            b = min;
            break;
        case 2:
            r = min;
            g = max;
            b = min + chroma * intermediateColorPercentage;
            break;
        case 3:
            r = min;
            g = min + chroma * (1 - intermediateColorPercentage);
            b = max;
            break;
        case 4:
            r = min + chroma * intermediateColorPercentage;
            g = min;
            b = max;
            break;
        case 5:
            r = max;
            g = min;
            b = min + chroma * (1 - intermediateColorPercentage);
            break;
        }

        return Microsoft::UI::ColorHelper::FromArgb(255, static_cast<byte>(r * 255), static_cast<byte>(g * 255), static_cast<byte>(b * 255));
    }

} } } } } } } // Microsoft::UI::Xaml::Tests::Controls::Primitives::Color
