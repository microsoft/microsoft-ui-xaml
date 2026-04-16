// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Text {

class HyperlinkInTextTests : public WEX::TestClass<HyperlinkInTextTests>
{
public:
    BEGIN_TEST_CLASS(HyperlinkInTextTests)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"32301317-5c46-4350-8af6-a06552076e89;3192b2bd-30c5-4c19-a6c1-9856b940df63;d04573b8-e899-4822-bb72-9f4743c89d36")
        TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
    END_TEST_CLASS()

    TEST_CLASS_SETUP(ClassSetup)
    TEST_METHOD_SETUP(TestSetup)
    TEST_METHOD_CLEANUP(TestCleanup)

    BEGIN_TEST_METHOD(ValidateHyperlinkInText)
        TEST_METHOD_PROPERTY(L"Description", L"Validates the look of hyperlink in text")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO 36060166: Re-enable after fixing unreliability.
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateHyperlinkInTextWithoutPointerOverUnderline)
        TEST_METHOD_PROPERTY(L"Description", L"Validates the look of hyperlink in text without underline in PointerOver visual state")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore") // TODO: 31563479 - Mouse input helper not reliable on WindowsCore, Santorini
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateHyperlinkInTextWithHighContrastUnderline)
        TEST_METHOD_PROPERTY(L"Description", L"Validates the look of hyperlink in text with underline in HighContrast theme")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore") // TODO: 31563479 - Mouse input helper not reliable on WindowsCore, Santorini
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(UnderlineStyle)
        TEST_METHOD_PROPERTY(L"Description", L"Validates Hyperlink.UnderlineStyle property")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO 36060166: Re-enable after fixing unreliability.
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateHyperlinkWithRequestedTheme)
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore") // TODO: 31563479 - Mouse input helper not reliable on WindowsCore, Santorini
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateHyperlinkWithLocalForeground)
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore") // TODO: 31563479 - Mouse input helper not reliable on WindowsCore, Santorini
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // [DCPP-test] WPF tests are failing with AnimationIdle timeout during test cleanup
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(FocusHyperLinkWithSIPShowing)
        TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Phone")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HyperLinkBringIntoView)
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP") // Explicitly tests RootScrollViewer
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HyperLinkBringIntoViewWithGamepad)
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP") // Explicitly tests RootScrollViewer
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateHyperlinkEnterKeyInput)
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateHyperlinkSpaceKeyInput)
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateHyperlinkGamePadAKeyInput)
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateNavigationOnKeyUp)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies that Hyperlink processes a KeyUp to Navigate only after it sees a KeyDown on NavigationKey first")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateKeyDownBubbling)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies that Hyperlink bubbles the KeyDown event to its containing element")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(PointerOverHighContrast)
        TEST_METHOD_PROPERTY(L"Description", L"Validates Hyperlink pointer over color for high contrast theme")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore") // TODO: 31563479 - Mouse input helper not reliable on WindowsCore, Santorini
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateDefaultSystemControlForegroundBrushes)
        TEST_METHOD_PROPERTY(L"Description", L"Validates Hyperlink's usage of default SystemControlHyperlinkTextBrush, SystemControlHyperlinkBaseMediumBrush & SystemControlHighlightBaseMediumLowBrush fallback theme resources.")
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO 36060166: Re-enable after fixing unreliability.
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateCustomSystemControlForegroundBrushes)
        TEST_METHOD_PROPERTY(L"Description", L"Validates Hyperlink's usage of custom SystemControlHyperlinkTextBrush, SystemControlHyperlinkBaseMediumBrush & SystemControlHighlightBaseMediumLowBrush fallback theme resources.")
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO 36060166: Re-enable after fixing unreliability.
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateHyperlinkForegroundBrushes)
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")
        TEST_METHOD_PROPERTY(L"Description", L"Validates Hyperlink's usage of custom HyperlinkForeground, HyperlinkForegroundPointerOver & HyperlinkForegroundPressed theme resources.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateProgrammaticHyperlinkFocus)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies that Hyperlink can programmatically receive focus")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateHyperlinkLostFocus)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies that Hyperlink LostFocus event is fired when focus is lost")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateGotFocusBubbling)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies that Hyperlink bubbles the GotFocus event to its containing element")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateProgrammaticHyperlinkGotFocusBubbling)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies that Hyperlink bubbles the GotFocus event to its containing element when focus is set Programmatically")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateFocusRectOnFocusKeyboard)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies that a focusrect is rendered when Focus is called with FocusState::Keyboard")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateFocusRectOnFocusPointer)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies that a focusrect is not rendered when Focus is called with FocusState::Pointer")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateProgrammaticFocusRectOnPreviousInputPointer)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies that a focusrect is not rendered for FocusState::Programmatic when previous input device was pointer")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateHyperlinkGotFocusOnLoad)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies that Hyperlink GotFocus event is fired on load.")
        TEST_METHOD_PROPERTY(L"IsolationLevel", L"Method")
        TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateCollapsedHyperlinkFocus)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies that a Hyperlink in a collapsed TextBlock cannot get focus.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateHyperlinkIsTabStop)
        TEST_METHOD_PROPERTY(L"Description", L"Validates Hyperlink IsTabStop property affects its focusability.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateHyperlinkIsTabStopGamepadBehavior)
        TEST_METHOD_PROPERTY(L"Description", L"Validates Hyperlink IsTabStop behavior with Gamepad.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(UIAPeerLifetime)
        TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateTapAndClick)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies that tapping and clicking on the Hyperlink work")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
    END_TEST_METHOD()

private:
    inline Platform::String^ GetResourcesPath() const;
    void ValidateHyperlinkInText(bool testPointerOver, bool withUnderline, bool useHighContrast);
    void ValidateHyperlinkKeyInput(const Platform::String ^key);
    void ValidateForegroundBrushes(bool withSystemControlColors, bool withCustomSystemControlColors = false);
    bool IsSameColor(::Windows::UI::Color expected, ::Windows::UI::Color actual)
    {
        return (expected.R == actual.R
            && expected.G == actual.G
            && expected.B == actual.B
            && expected.A == actual.A);
    }
};

} } } } } }

