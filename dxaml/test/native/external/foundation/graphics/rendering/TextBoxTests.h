// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Graphics {

        class TextBoxTests : public WEX::TestClass<TextBoxTests>
        {
        public:
            BEGIN_TEST_CLASS(TextBoxTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"1d91ef47-c885-45e2-a578-7aaf1a1b1296;df11dd90-2e1d-45ff-93cb-cd6c0b87e24d;d04573b8-e899-4822-bb72-9f4743c89d36")
                TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_METHOD_SETUP(TestSetup)

            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(RenderTextBoxWith18Zoom)
                TEST_METHOD_PROPERTY(L"Classification", L"Integration")
                TEST_METHOD_PROPERTY(L"Description", L"Renders a TextBox with zoom scale 1.8x. Ensures the TextBox is drawn as a single primitive so that seams don't show up.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Zoom scale not applied
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")    // [DCPPTest] Xaml tests are failing because Xaml no longer applies the plateau scale
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(StartEndHorizontalTextAlignment)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies new TextAlignment enums and new HorizontalTextAlignment property")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyForegroundAlphaOnlyChangeTakesEffect)
                TEST_METHOD_PROPERTY(L"Classification", L"Integration")
                TEST_METHOD_PROPERTY(L"Description", L"Displays a TextBox and then changes its Foreground to a brush that differs only in alpha.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

        private:
            inline Platform::String^ GetResourcesPath() const;
        };

    } }
} } } }

