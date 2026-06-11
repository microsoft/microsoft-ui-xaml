// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <Versioning.h>

#include <AutomationClient\AutomationClientManager.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace RichEditBox {

    class RichEditBoxAutomationIntegrationTests : public WEX::TestClass<RichEditBoxAutomationIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(RichEditBoxAutomationIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"IsolationLevel", L"Test") //DCPP: Crash in Microsoft.UI.Input.dll!UIAutomationIslandForwarder::EnsureAutomationHostProvider(HWND__ * hwnd)
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(VerifyDefaultAutomationName)
            TEST_METHOD_PROPERTY(L"Description", L"Validates supported UIA patterns for RichEditBox..")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyRichEditBoxPlaceholderTextIsMovedToDescribedBy)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that placeholder text is moved to DescribedBy field.")
            TEST_CLASS_PROPERTY(L"TestPass:MinOSVer", WINDOWS_OS_VERSION_RS5) // RS4 Test Failure: RichEditBoxAutomationIntegrationTests::VerifyRichEditBoxPlaceholderTextIsMovedToDescribedBy
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(MoveEndpoint)
            TEST_METHOD_PROPERTY(L"Description", L"Validates MoveEndPointByUnit and MoveEndPointByRange calls on RichEditBox including adjustment made for EOP.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(TextRangeMove)
            TEST_METHOD_PROPERTY(L"Description", L"Testing range move forward and backward by character, word and line.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(TextRangeMoveOverLinkAndObject)
            TEST_METHOD_PROPERTY(L"Description", L"Testing range move forward and backward over link and object.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(GetVisibleRangesOnRTLBox)
            TEST_METHOD_PROPERTY(L"Description", L"Testing GetClientRect on RTL box using UIA method.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyContextMenuIsFocused)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that the context menu is correctly focused when invoked by UIA.")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // WinUI 3: RichEditBoxAutomationIntegrationTests::VerifyContextMenuIsFocused is unreliable
        END_TEST_METHOD()

    private:
        void VerifyRangeText(wrl::ComPtr<IUIAutomationTextRange>& spUIAutomationTextRange, LPCWSTR expectedText);
    };

} } } } } }
