// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Text {
        class TextControlsExpansionTests : public WEX::TestClass<TextControlsExpansionTests>
        {
        public:
            BEGIN_TEST_CLASS(TextControlsExpansionTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"32301317-5c46-4350-8af6-a06552076e89;3192b2bd-30c5-4c19-a6c1-9856b940df63")
                TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(AnimateTextBoxHeightByAddingNewLine)
                TEST_METHOD_PROPERTY(L"Description", L"Validates animated growth of a TextBox height when new line is added with keyboard.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(AnimateRichEditBoxHeightByAddingNewLine)
                TEST_METHOD_PROPERTY(L"Description", L"Validates animated growth of a RichEditBox height when new line is added with keyboard.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(AnimateTextBoxHeightByAddingText)
                TEST_METHOD_PROPERTY(L"Description", L"Validates animated growth of a TextBox height when text is added programmatically while it has focus.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(AnimateRichEditBoxHeightByAddingText)
                TEST_METHOD_PROPERTY(L"Description", L"Validates animated growth of a RichEditBox height when text is added programmatically while it has focus.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ChangeTextBoxHeightWithoutAnimationNoFocus)
                TEST_METHOD_PROPERTY(L"Description", L"Validates there is no height animation when changing the text of a TextBox while it does not have focus.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ChangeRichEditBoxHeightWithoutAnimationNoFocus)
                TEST_METHOD_PROPERTY(L"Description", L"Validates there is no height animation when changing the text of a RichEditBox while it does not have focus.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ChangeTextBoxHeightWithoutAnimationNoWrapAndNoReturn)
                TEST_METHOD_PROPERTY(L"Description", L"Validates there is no height animation when changing the height of a TextBox while wrapping is off and carriage returns are not accepted.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ChangeRichEditBoxHeightWithoutAnimationNoWrapAndNoReturn)
                TEST_METHOD_PROPERTY(L"Description", L"Validates there is no height animation when changing the height of a RichEditBox while wrapping is off and carriage returns are not accepted.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ChangeTextBoxHeightWithoutAnimationNoVariableHeight)
                TEST_METHOD_PROPERTY(L"Description", L"Validates there is no height animation when changing the height of a TextBox while original height is different from NaN.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ChangeRichEditBoxHeightWithoutAnimationNoVariableHeight)
                TEST_METHOD_PROPERTY(L"Description", L"Validates there is no height animation when changing the height of a RichEditBox while original height is different from NaN.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            END_TEST_METHOD()

        private:
            Platform::String^ GetPathToFiles() const;

            void AnimateTextControlHeight(
                _In_ xaml::FrameworkElement^ root,
                _In_ bool addNewLine);

            void ChangeTextControlHeightWithoutAnimation(
                _In_ xaml::FrameworkElement^ root,
                _In_ bool setFocus,
                _In_ bool canWrap,
                _In_ bool acceptsReturn,
                _In_ bool isHeightNaN);
        };

    } }
} } } }
