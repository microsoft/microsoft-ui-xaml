// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace TextBox {

    class TextBoxIntegrationTests : public WEX::TestClass<TextBoxIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(TextBoxIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"a62e3c8d-69d4-44de-95b5-a62be5062286;57e0de30-efb3-4001-9ccc-b38032fd1974;cbb6c59f-3ce2-4ed3-8eaa-f598566c2755")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(ClearButton)
            TEST_METHOD_PROPERTY(L"Description", L"Checks that the TextBox clear all button is visible and erases text.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ClearButtonOnXBox)
            TEST_METHOD_PROPERTY(L"Description", L"Checks that when on XBox the clear all button is not visible")
                                                          // since we're faking the platform anyway.
            TEST_METHOD_PROPERTY(L"IsolationLevel", L"Method") // This test mocks the platform -- isolate it to avoid instability
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ChangeViewInInnerScrollViewer)
            TEST_METHOD_PROPERTY(L"Description", L"Exercises the ChangeView method on the TextBox's inner ScrollViewer and scrolls it with the mouse wheel.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyContextMenuAppearsAtCaretWithShiftF10)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that using Shift+F10 to show the context menu shows it at the caret position, rather than below the control.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyContextMenuRaisesCutCopyPasteEvents)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that using the context menu buttons to cut, copy, and paste raises the associated events.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyPressAndHoldOnlyShowsContextMenu)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that the press-and-hold gesture brings up the context menu, and not the selection flyout.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()
        
        BEGIN_TEST_METHOD(VerifySelectingTextWithTouchShowsSelectionFlyout)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that selecting text with touch shows the selection flyout.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyHwndFromElementIsFocusedOne)
            TEST_METHOD_PROPERTY(L"Description", L"Checks if window element is attached to equal the one in focus when textbox has the same.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

    private:
        void ClearButtonHelper(bool clearButtonExpected) const;
    };

} } } } } }
