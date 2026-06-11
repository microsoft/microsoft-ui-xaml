// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Text {

        class TextBoxScrollingTests : public WEX::TestClass<TextBoxScrollingTests>
        {
        public:
            BEGIN_TEST_CLASS(TextBoxScrollingTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_METHOD_SETUP(TestSetup)

            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(ScrollWithoutWrappingText)
                TEST_METHOD_PROPERTY(L"Description", L"Test scroll to go to the parent scrollviewer")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ScrollWithWrappingText)
                TEST_METHOD_PROPERTY(L"Description", L"Test scroll to go to the TextBox itself")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ScrollWithMouse)
                TEST_METHOD_PROPERTY(L"Description", L"Test TextBox scroll with mouse")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ScrollWithKeyboard)
                TEST_METHOD_PROPERTY(L"Description", L"Test TextBox scroll with keyboard")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TextBoxVisualStatePanInsideScrollViewer)
                TEST_METHOD_PROPERTY(L"Description", L"Touch panning for TextBox inside ScrollViewer should not leave the visual state at PointerOver")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(KeyboardScrollingInScrollViewerKeepsCaretInView)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies keyboard scrolling within a textbox keeps the caret in view.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            void TextBoxScrollingTestsHelper(bool doesTextWrap, bool testVisualStatePanFromTextBox=false);
        };

    } }
} } } }
