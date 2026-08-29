// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Text {
        class ContextMenuTests : public WEX::TestClass<ContextMenuTests>
        {
        public:
            BEGIN_TEST_CLASS(ContextMenuTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"32301317-5c46-4350-8af6-a06552076e89;3192b2bd-30c5-4c19-a6c1-9856b940df63")
                TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(SelectAllWithContextMenu)
                TEST_METHOD_PROPERTY(L"Description", L"Brings up a context menu for TextBox and RichEditBox controls. Selects the 'Select All' command.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"TestPass:MinOSVer", WINDOWS_OS_VERSION_19H1) // RS5 Test failure: ContextMenuTests::SelectAllWithContextMenu
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TextBlockContextMenuOpeningEvent)
                TEST_METHOD_PROPERTY(L"Description", L"TextBlock: Verify that the ContextMenuOpening event fires")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // Move windowed popups to lifted input
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(RichTextBlockContextMenuOpeningEvent)
                TEST_METHOD_PROPERTY(L"Description", L"RichTextBlock: Verify that the ContextMenuOpening event fires")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"Ignore", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TextBoxContextMenuOpeningEvent)
                TEST_METHOD_PROPERTY(L"Description", L"TextBox: Verify that the ContextMenuOpening event fires")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

        private:
            Platform::String^ GetPathToFiles() const;
            static void OpenMenuAndInvokeCommand(Platform::String^ openMenuKeyboardSequence);

            Platform::String^ m_keyboardSequenceApps = "$d$_apps#$u$_apps";
            Platform::String^ m_keyboardSequenceShiftF10 = "$d$_shift#$d$_f10#$u$_f10#$u$_shift";
            Platform::String^ m_keyboardSequenceGamepadMenu = "$d$_GamePadMenu#$u$_GamePadMenu";
        };
    } }
} } } }
