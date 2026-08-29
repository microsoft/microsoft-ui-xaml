// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <RuntimeEnabledFeatureOverride.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace ListViewBaseItem {

    class IntegrationTests : public WEX::TestClass < IntegrationTests >
    {
    public:
        BEGIN_TEST_CLASS(IntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"465cba5c-d9c4-40ac-933a-f238efc26016;a69ddfa4-5142-4bed-887d-6d0ca14a3473")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_CLASS_CLEANUP(ClassCleanup)
        TEST_METHOD_SETUP(TestSetup)

        TEST_METHOD_CLEANUP(TestCleanup)

        //
        // Platform:Any
        //
        BEGIN_TEST_METHOD(CanInstantiate)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully create a ListViewItem and GridViewItem.")
        END_TEST_METHOD()
        
        BEGIN_TEST_METHOD(CanEnterAndLeaveLiveTree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully add/remove a ListViewItem and GridViewItem from the live tree.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateVisualStyleListViewItem)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the ListViewItem uses the new visual styles correctly.")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateVisualStyleGridViewItem)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the GridViewItem uses the new visual styles correctly.")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateVisualStyleLVIChrome)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the ListViewItem chrome uses the new visual styles correctly.")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateVisualStyleGVIChrome)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the GridViewItem chrome uses the new visual styles correctly.")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateMultiLineTextBlock)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the textblock reflows in multiple selection mode.")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateRTLSupportForCheckMark)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the checkmark supports RTL situations.")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DisplayListAndGridViews)
            TEST_METHOD_PROPERTY(L"Description", L"Displays ListView and GridView controls with rounded corner styles.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DisplayListAndGridViewsWithSelectionIndicatorModeInline)
            TEST_METHOD_PROPERTY(L"Description", L"Displays ListView and GridView controls with rounded corner styles and SelectionIndicatorMode forced to Inline.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DisplayListAndGridViewsWithSelectionIndicatorModeOverlay)
            TEST_METHOD_PROPERTY(L"Description", L"Displays ListView and GridView controls with rounded corner styles and SelectionIndicatorMode forced to Overlay.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DisplayListAndGridViewsWithOldStyles)
            TEST_METHOD_PROPERTY(L"Description", L"Displays ListView and GridView controls with square corner styles.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyListViewItemContentTemplateRoot)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies ListViewItem.ContentTemplateRoot property with rounded corner styles.")
            TEST_METHOD_PROPERTY(L"RegressionBug", L"21H1:31883866")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyListViewItemContentTemplateRootWithOldStyles)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies ListViewItem.ContentTemplateRoot property with square corner styles.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateChromeNoPointerOverBrush)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that not setting a pointerover brush retains the existing brush.")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSelectedForegroundBrush)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting a selected brush works properly.")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateChangeCheckMode)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that changing the CheckMode property functions properly.")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanUseGridViewItemPresenter)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that using GridViewItemPresenter does not cause a crash with PointerOver state.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyLVIPHasSameSizeAsLVI)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the ListViewItemPresenter has the same size as the ListViewItem it's contained in.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyLVIFocusRect)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that focus rect is rendered correctly.")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyNoPressedStateWhenPanning)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we don't hit the Pressed visual state while panning.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyPressedStateWhenTapping)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that go into the Pressed visual state when quickly tapping the item.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyChangeThemeInMultiSelect)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that changing the theme updates the multiselect checkbox.")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyDragOverStateForLVI)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that DragOver state is being invoked when drag enters drop zone 20/60/20")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyDragOverStateForGVI)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that DragOver state is being invoked when drag enters drop zone 30/40/30")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateChromeAnimationCommandsLifetime)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the chrome animation commands' lifetime is handled correctly in case the ListViewItemPresenter instance doesn't have a child.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateChromeAnimationCommandsLifetimeWithSelectionIndicator)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the chrome animation commands' lifetime is handled correctly in case the ListViewItemPresenter instance uses a selection indicator.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyLVIPWorksWithVSM)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that ListViewItemPresenter goes to visual states declared in the template.")
        END_TEST_METHOD()     

        BEGIN_TEST_METHOD(ValidateLVIPRevealBackgroundOnTopOfContent)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the ListViewItemPresenter's RevealBackground OnTop of Content.")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateLVIPRevealBackgroundBelowContent)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the ListViewItemPresenter's RevealBackground below Content.")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateListViewItemRevealBackgroundShowsAboveContentStyle)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the ListViewItemRevealBackgroundShowsAboveContentStyle.")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateGridViewItemRevealBackgroundShowsAboveContentStyle)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the GridListViewItemRevealBackgroundShowsAboveContentStyle.")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateGridViewItemRevealBackgroundAndBorder)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the GridViewItem's reveal background shows in overlay mode and that borders are hidden when disabled.")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateGridViewItemRevealDisabledStates)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the GridViewItem's transitioning between disabled states.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateListViewItemFocusPropertyThemeChange)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the ListViewItem's FocusVisual* properties respond to theme changes.")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Fails locally in WPF-hosting because Height is slightly off compared with dcomp baseline.
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateGridViewItemFocusPropertyThemeChange)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the GridViewItem's FocusVisual* properties respond to theme changes.")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Fails locally in WPF-hosting because Height is slightly off compared with dcomp baseline.
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyKeyboardReordering)
            TEST_METHOD_PROPERTY(L"Description", L"Validates ListView item reordering with keyboard in LTR/RTL flows, and horizontal/vertical layouts.")
        END_TEST_METHOD()

        //
        // Private Methods
        //
        void TestSelectorItemFocusThemeChanges(
            Platform::String^ xaml,
            Platform::String^ elementName,
            ::Windows::UI::Color lightPrimary,
            ::Windows::UI::Color lightSecondary,
            ::Windows::UI::Color darkPrimary,
            ::Windows::UI::Color darkSecondary);

        bool AreBuffersEqual(::Windows::Storage::Streams::IBuffer^ buffer1, ::Windows::Storage::Streams::IBuffer^ buffer2);
        Platform::Array<byte>^ GetRenderDataBuffer(::Windows::Storage::Streams::IBuffer^ buffer);
        ::Windows::Storage::Streams::IBuffer^ GetRenderStreamBuffer();
        void LoadXamlAndVerifyMockDCompOutput(Platform::String^ xaml);
        void DisplayListAndGridViews(
            bool isInteractive,
            bool enableRoundedListViewBaseItemChrome,
            bool forceSelectionIndicatorModeInline,
            bool forceSelectionIndicatorModeOverlay);
        void VerifyListViewItemContentTemplateRoot(
            bool enableRoundedListViewBaseItemChrome);
        void ValidateChromeAnimationCommandsLifetime(
            bool forceSelectionIndicatorVisualEnabled);
        void VerifyKeyboardReordering(
            bool horizontalPanel,
            bool rightToLeft);
    };

} } } } } }

