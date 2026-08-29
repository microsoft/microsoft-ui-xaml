// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Text {
        class TextControlsSizingTests : public WEX::TestClass<TextControlsSizingTests>
        {
        public:
            BEGIN_TEST_CLASS(TextControlsSizingTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"32301317-5c46-4350-8af6-a06552076e89;3192b2bd-30c5-4c19-a6c1-9856b940df63")
                TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_CLASS_CLEANUP(ClassCleanup)
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(ValidateInitialTextBoxSizes)
                TEST_METHOD_PROPERTY(L"Description", L"Validates the original size of various TextBox controls.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateInitialTextBoxMeasureOverride)
                TEST_METHOD_PROPERTY(L"Description", L"Validates the first MeasureOverride return value.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateAutoGrowTextBoxMeasureOverride)
                TEST_METHOD_PROPERTY(L"Description", L"Validates the auto grow textbox does not jump height in between two lines and one line for Measure.")
            END_TEST_METHOD()

        private:
            Platform::String^ GetPathToFiles() const;
        };

    } }
} } } }

