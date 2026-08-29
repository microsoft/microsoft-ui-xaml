// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Text {
        class FloatieContextMenuTests : public WEX::TestClass<FloatieContextMenuTests>
        {
        public:
            BEGIN_TEST_CLASS(FloatieContextMenuTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"32301317-5c46-4350-8af6-a06552076e89;3192b2bd-30c5-4c19-a6c1-9856b940df63")
                TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop") // TODO: remove after the associated issue is fixed
                TEST_CLASS_PROPERTY(L"IsolationLevel", L"Method") // TODO: remove after the associated issue is fixed
                TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(TextBoxProofingMenu_NoErrors)
                TEST_METHOD_PROPERTY(L"Description", L"TextBox: Verify proofing menu is empty when word is not misspelled")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TextBoxProofingMenu_MisspelledWord)
                TEST_METHOD_PROPERTY(L"Description", L"TextBox: Verify proofing menu is has spelling suggestions on misspelled word")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TextBoxProofingMenu_RepeatedWord)
                TEST_METHOD_PROPERTY(L"Description", L"TextBox: Verify can delete a repeated word using the proofing menu")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TextBoxProofingMenu_AutocorrectedWord)
                TEST_METHOD_PROPERTY(L"Description", L"TextBox: Verify can revent an autocorrected word using the proofing menu")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(RichEditBoxProofingMenu_NoErrors)
                TEST_METHOD_PROPERTY(L"Description", L"RichEditBox: Verify proofing menu is empty when word is not misspelled")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(RichEditBoxProofingMenu_MisspelledWord)
                TEST_METHOD_PROPERTY(L"Description", L"RichEditBox: Verify proofing menu is has spelling suggestions on misspelled word")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(RichEditBoxProofingMenu_RepeatedWord)
                TEST_METHOD_PROPERTY(L"Description", L"RichEditBox: Verify can delete a repeated word using the proofing menu")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(RichEditBoxProofingMenu_AutocorrectedWord)
                TEST_METHOD_PROPERTY(L"Description", L"RichEditBox: Verify can revent an autocorrected word using the proofing menu")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TextBoxKeyInputContextMenuOpeningEventWhenFloatieEnabled)
                TEST_METHOD_PROPERTY(L"Description", L"Verify TextBox.ContextMenuOpening event is fired and handled when floatie is enabled for key input")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(RichEditBoxKeyInputContextMenuOpeningEventWhenFloatieEnabled)
                TEST_METHOD_PROPERTY(L"Description", L"Verify RichEditBox.ContextMenuOpening event is fired and handled when floatie is enabled for key input")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PasswordBoxKeyInputContextMenuOpeningEventWhenFloatieEnabled)
                TEST_METHOD_PROPERTY(L"Description", L"Verify PasswordBox.ContextMenuOpening event is fired and handled when floatie is enabled for key input")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TextBlockKeyInputContextMenuOpeningEventWhenFloatieEnabled)
                TEST_METHOD_PROPERTY(L"Description", L"Verify TextBlock.ContextMenuOpening event is fired and handled when floatie is enabled for key input")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(RichTextBlockKeyInputContextMenuOpeningEventWhenFloatieEnabled)
                TEST_METHOD_PROPERTY(L"Description", L"Verify RichTextBlock.ContextMenuOpening event is fired and handled when floatie is enabled for key input")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TextBoxMouseInputContextMenuOpeningEventWhenFloatieEnabled)
                TEST_METHOD_PROPERTY(L"Description", L"Verify TextBox.ContextMenuOpening event is fired and handled when floatie is enabled for pointer events")
            END_TEST_METHOD()
                
            BEGIN_TEST_METHOD(RichEditBoxMouseInputContextMenuOpeningEventWhenFloatieEnabled)
                TEST_METHOD_PROPERTY(L"Description", L"Verify RichEditBox.ContextMenuOpening event is fired and handled when floatie is enabled for pointer events")
                END_TEST_METHOD()

            BEGIN_TEST_METHOD(PasswordBoxMouseInputContextMenuOpeningEventWhenFloatieEnabled)
                TEST_METHOD_PROPERTY(L"Description", L"Verify PasswordBox.ContextMenuOpening event is fired and handled when floatie is enabled for pointer events")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TextBlockMouseInputContextMenuOpeningEventWhenFloatieEnabled)
                TEST_METHOD_PROPERTY(L"Description", L"Verify TextBlock.ContextMenuOpening event is fired and handled when floatie is enabled for pointer events")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(RichTextBlockMouseInputContextMenuOpeningEventWhenFloatieEnabled)
                TEST_METHOD_PROPERTY(L"Description", L"Verify RichTextBlock.ContextMenuOpening event is fired and handled when floatie is enabled for pointer events")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TextBoxTouchInputContextMenuOpeningEventWhenFloatieEnabled)
                TEST_METHOD_PROPERTY(L"Description", L"Verify TextBox.ContextMenuOpening event is fired and handled when floatie is enabled for touch pointer events")
            END_TEST_METHOD()
                
            BEGIN_TEST_METHOD(RichEditBoxTouchInputContextMenuOpeningEventWhenFloatieEnabled)
                TEST_METHOD_PROPERTY(L"Description", L"Verify RichEditBox.ContextMenuOpening event is fired and handled when floatie is enabled for touch pointer events")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PasswordBoxTouchInputContextMenuOpeningEventWhenFloatieEnabled)
                TEST_METHOD_PROPERTY(L"Description", L"Verify PasswordBox.ContextMenuOpening event is fired and handled when floatie is enabled for touch pointer events")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TextBlockTouchInputContextMenuOpeningEventWhenFloatieEnabled)
                TEST_METHOD_PROPERTY(L"Description", L"Verify TextBlock.ContextMenuOpening event is fired and handled when floatie is enabled for touch pointer events")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(RichTextBlockTouchInputContextMenuOpeningEventWhenFloatieEnabled)
                TEST_METHOD_PROPERTY(L"Description", L"Verify RichTextBlock.ContextMenuOpening event is fired and handled when floatie is enabled for touch pointer events")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(RichEditBoxKeyDownDismissFloatie)
                TEST_METHOD_PROPERTY(L"Description", L"Verify unhandled key down dismisses floatie")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()
        private:
            Platform::String^ GetPathToFiles() const;
        };
    } }
} } } }
