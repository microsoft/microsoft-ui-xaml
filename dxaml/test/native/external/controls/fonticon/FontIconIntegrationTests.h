// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace FontIcon {

    class FontIconIntegrationTests : public WEX::TestClass<FontIconIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(FontIconIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"465cba5c-d9c4-40ac-933a-f238efc26016;a69ddfa4-5142-4bed-887d-6d0ca14a3473;b34da8d2-333d-40a9-a19c-94b1f9785580")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(CanInstantiate)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully create a FontIcon.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanEnterAndLeaveLiveTree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully add/remove a FontIcon from the live tree.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSetAndGetProperties)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully set/get the FontIcon properties.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSetForegroundWithVSM)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(TextFormattingPropertiesInheritRS4)
            TEST_METHOD_PROPERTY(L"Description", L"FontIcon.Font* properties no longer inherit from the parent on RS4+.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(TextFormattingPropertiesOverride)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(TextBlockIgnoresImplicitStyle)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateFootprint)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the ActualWidth and ActualHeight of FontIcon.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(FontPropertiesDefaultValuesRS4)
            TEST_METHOD_PROPERTY(L"Description", L"Verify default ClearValue'd values of FontIcon.Font* properties on RS4.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(FontPropertiesFromStyleRS4)
            TEST_METHOD_PROPERTY(L"Description", L"FontIcon.Font* properties can be set from Style on RS4+.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyFontPropertyChanges)
            TEST_METHOD_PROPERTY(L"Description", L"Verify changing the source of FontIcon.Font* properties updates the values properly.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(MirroredWhenRightToLeft)
            TEST_METHOD_PROPERTY(L"Description", L"Verify MirroredWhenRightToLeft is applied properly.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

    private:
        void TextFormattingPropertiesInheritInternal(bool areFontIconFontPropertiesInheritable);
        void FontPropertiesDefaultValuesInternal(bool isRS4andNewer);
        void FontPropertiesFromStyleInternal(bool canValuesComeFromStyle);
    };

} } } } } }
