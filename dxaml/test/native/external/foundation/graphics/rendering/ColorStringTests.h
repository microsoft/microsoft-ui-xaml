// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

class ColorStringTests : public WEX::TestClass<ColorStringTests>
{
public:
    BEGIN_TEST_CLASS(ColorStringTests)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
    END_TEST_CLASS()

    TEST_CLASS_SETUP(ClassSetup)
    TEST_METHOD_SETUP(TestSetup)
    TEST_METHOD_CLEANUP(TestCleanup)

    BEGIN_TEST_METHOD(ShortHexColorStrings)
        TEST_METHOD_PROPERTY(L"Description", L"Parses #RGB and #ARGB color strings from markup and rejects hex strings of unsupported length.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ScRgbColorStrings)
        TEST_METHOD_PROPERTY(L"Description", L"Parses sc# color strings with three and four channels, saturates out-of-range channels and rejects other channel counts.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(NamedAndInvalidColorStrings)
        TEST_METHOD_PROPERTY(L"Description", L"Parses known color names case-insensitively with trailing white space and rejects integers, context colors and unknown names.")
    END_TEST_METHOD()
};

} } } } } }
