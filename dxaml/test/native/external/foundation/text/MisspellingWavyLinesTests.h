// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Text {
        class MisspellingWavyLinesTests : public WEX::TestClass < MisspellingWavyLinesTests >
        {
        public:
            BEGIN_TEST_CLASS(MisspellingWavyLinesTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_METHOD_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"32301317-5c46-4350-8af6-a06552076e89")
                TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_CLASS_CLEANUP(ClassCleanup)
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(CheckTextControlsWavyLines)
                TEST_METHOD_PROPERTY(L"Description", L"Validates the TextBox and RichEditBox controls show red misspelling wavy lines.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")    // DCPP: CheckTextControlsWavyLines is timing out on ResetWindowContentAndScaleWaitForIdle
            END_TEST_METHOD()

        private:
            Platform::String^ GetPathToFiles() const;
        };
    } }
} } } }
