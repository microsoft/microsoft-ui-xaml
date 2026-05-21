// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Text {
        class TextControlsRenderingTests : public WEX::TestClass<TextControlsRenderingTests>
        {
        public:
            BEGIN_TEST_CLASS(TextControlsRenderingTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"32301317-5c46-4350-8af6-a06552076e89")
                TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_CLASS_CLEANUP(ClassCleanup)
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(ValidateTextControlsRendering)
                TEST_METHOD_PROPERTY(L"Description", L"Validates various text controls rendering.")
                #if defined(_WIN64) //Due to the floating point precision issues, disable this test for x64
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")
                #endif
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"UAP:WaitForXamlWindowActivation", L"false")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateTextControlsWithLargeFontSize)
                TEST_METHOD_PROPERTY(L"Description", L"Validates various text controls rendering of larget font size.")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // WinUI 3: TextControlsRenderingTests::ValidateTextControlsWithLargeFontSize has an unreliable baseline file
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateTextControlsWithAlgerianFont)
                TEST_METHOD_PROPERTY(L"Description", L"Validates simulated bold font weight for algerian font.")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateTextControlForegroundUpdate)
                TEST_METHOD_PROPERTY(L"Description", L"Validates foreground color update on Text Control.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateRichEditBoxTOMLargeFontSize)
                TEST_METHOD_PROPERTY(L"Description", L"Validates setting large fontsize through TOM Api")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TextControlSupportAlphaForegroundColor)
                TEST_METHOD_PROPERTY(L"Description", L"Validates user can set alpha value on Text color brush.")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateEmojiRendering)
                TEST_METHOD_PROPERTY(L"Description", L"Validates rendering of emoji character.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
                TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateTextFormattersLowMemoryRelease)
                TEST_METHOD_PROPERTY(L"Description", L"Validates the releasing of unused textformatters in response to low memory condition.")
            END_TEST_METHOD()

        private:
            Platform::String^ GetPathToFiles() const;

            bool IsSameColor(::Windows::UI::Color expected, ::Windows::UI::Color actual)
            {
                return (expected.R == actual.R
                    && expected.G == actual.G
                    && expected.B == actual.B
                    && expected.A == actual.A);
            }
        };

    } }
} } } }

