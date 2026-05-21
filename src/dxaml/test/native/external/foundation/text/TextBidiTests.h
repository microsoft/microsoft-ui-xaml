// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Text {

        class TextBidiTests : public WEX::TestClass<TextBidiTests>
        {
        public:
            BEGIN_TEST_CLASS(TextBidiTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"32301317-5c46-4350-8af6-a06552076e89;3192b2bd-30c5-4c19-a6c1-9856b940df63;d04573b8-e899-4822-bb72-9f4743c89d36")
                TEST_CLASS_PROPERTY(L"Ignore", L"TRUE") // DCPP: Test failures from Lifted IXP content bridge's lifetime issues
                TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_METHOD_SETUP(TestSetup)

            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(TextBlockBidiFlowLTR)
                TEST_METHOD_PROPERTY(L"Description", L"Renders TextBlocks with RTL text for flow direction LTR.")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TextBlockBidiFlowRTL)
                TEST_METHOD_PROPERTY(L"Description", L"Renders TextBlocks with RTL text for flow direction RTL.")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TextBlockBidiFlowLTREnglish)
                TEST_METHOD_PROPERTY(L"Description", L"Renders TextBlocks with RTL text for flow direction LTR.")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TextBlockBidiFlowRTLEnglish)
                TEST_METHOD_PROPERTY(L"Description", L"Renders TextBlocks with RTL text for flow direction RTL.")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(RichTextBlockBidiFlowLTR)
                TEST_METHOD_PROPERTY(L"Description", L"Renders RichTextBlocks with RTL text for flow direction LTR.")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(RichTextBlockBidiFlowRTL)
                TEST_METHOD_PROPERTY(L"Description", L"Renders RichTextBlocks with RTL text for flow direction RTL.")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(RichTextBlockBidiFlowLTREnglish)
                TEST_METHOD_PROPERTY(L"Description", L"Renders RichTextBlocks with RTL text for flow direction LTR.")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(RichTextBlockBidiFlowRTLEnglish)
                TEST_METHOD_PROPERTY(L"Description", L"Renders RichTextBlocks with RTL text for flow direction RTL.")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TextBoxBidiFlowLTR)
                TEST_METHOD_PROPERTY(L"Description", L"Renders TextBox with RTL text for flow direction LTR.")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TextBoxBidiFlowRTL)
                TEST_METHOD_PROPERTY(L"Description", L"Renders TextBox with RTL text for flow direction RTL.")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TextBoxBidiFlowLTREnglish)
                TEST_METHOD_PROPERTY(L"Description", L"Renders TextBox with RTL text for flow direction LTR.")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TextBoxBidiFlowRTLEnglish)
                TEST_METHOD_PROPERTY(L"Description", L"Renders TextBox with RTL text for flow direction RTL.")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TextBoxBidiCursorPlacement)
                TEST_METHOD_PROPERTY(L"Description", L"Verify TextBox initial cursor placement.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PasswordBoxBidiFlowLTR)
                TEST_METHOD_PROPERTY(L"Description", L"Renders PasswordBox with RTL text for flow direction LTR.")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PasswordBoxBidiFlowRTL)
                TEST_METHOD_PROPERTY(L"Description", L"Renders PasswordBox with RTL text for flow direction RTL.")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PasswordBoxBidiFlowLTREnglish)
                TEST_METHOD_PROPERTY(L"Description", L"Renders PasswordBox with RTL text for flow direction LTR.")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PasswordBoxBidiFlowRTLEnglish)
                TEST_METHOD_PROPERTY(L"Description", L"Renders PasswordBox with RTL text for flow direction RTL.")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

        private:
            inline Platform::String^ GetResourcesPath() const;
            void TextBidiTestHelper(Platform::String^ filename);
            void GenerateAndTestPasswordBoxUIText(bool useEnglish, FlowDirection flowDirection);

        };

    } }
} } } }

