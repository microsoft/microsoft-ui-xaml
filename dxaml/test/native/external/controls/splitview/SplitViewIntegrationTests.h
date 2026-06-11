// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <RuntimeEnabledFeatureOverride.h>

#include <CommonInputHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace SplitView {

    class SplitViewIntegrationTests : public WEX::TestClass<SplitViewIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(SplitViewIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"1bb20c90-a558-491b-b76d-55bdb9a46911;57e0de30-efb3-4001-9ccc-b38032fd1974;bdc5c68b-03c1-4217-832c-4c09f99946f4")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_CLASS_CLEANUP(ClassCleanup)
        TEST_METHOD_SETUP(TestSetup)

        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(CanInstantiate)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully create a SplitView.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanEnterAndLeaveLiveTree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully add/remove a SplitView from the live tree.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateUIElementTree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the UIElement tree of the SplitView control in its various configurations.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // Move windowed popups to lifted input
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateLightDismissBehavior)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the SplitView control Overlay and CompactOverlay modes support light dismiss, whereas Inline and CompactInline modes do not.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop") // TODO: UtilitiesRoutineHelper::IsOneCore does not return true on OneCore
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            TEST_METHOD_PROPERTY(L"Ignore", L"True") // Disabled due to #38065901
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSetCompactPaneLengthProperty)
            TEST_METHOD_PROPERTY(L"Description", L"Verfies that app authors can set the compact length of the pane.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateElementResizeCountForTransitions)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we're not excessively re-laying out our content when transitioning between states.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoesFireEventsWhenOpeningOrClosingPane)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that the opening, opened, closing, closed events fire when changing IsPaneOpen.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanCancelPaneClosing)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that app authors can cancel a pane that is closing.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoSplitViewThemeResourcesExist)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that SplitView themeresources are available.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoesFocusWorkWithHyperlink)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that focus can be restored & lost to a hyperlink.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanFocusInlineSplitViewContentAreaAfterOpening)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that there is no crash when you try to focus the content area of an inline splitview after it opens.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSetOpenPaneLengthToAuto)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that the OpenPaneLength property supports being set to 'Auto'.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanTouchSplitViewWithInfiniteWidth)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that a SplitView measured with infinite width is still tappable and doesn't crash.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoesFirePaneClosingEventWhenClosedProgrammatically)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that the SplitView fires the Pane closing event when closed programmatically.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanInteractWithItemsInOpenPane)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that SplitViews in various configurations still allow their pane content to be interacted with.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanInteractWithPaneContentIfOpenedByDefault)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that a Splitview that is opened by default allows their controls to be interacted with.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSetOpenPaneLengthFromXamlString)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that SplitView.OpenPaneLength can be set correctly from a XAML string (e.g. VSM Setter).")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanChangeOpenPaneLength)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that apps can change the OpenPaneLength property and have it take effect.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanChangeCompactPaneLength)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that apps can change the CompactPaneLength property and have it take effect.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyGamepadFocusBehavior)
            TEST_METHOD_PROPERTY(L"Description", L"Verfies that only the overlay pane traps focus within the pane until it is open")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateFootprint)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the ActualWidth of SplitView's Content and Pane content in various configurations.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanNotShiftTabOutOfPaneWhenContentsIsListView)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that users can't shift-tab out of the light-dismissible pane when the pane content is a ListView.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyKeyboardFocusBehavior)
            TEST_METHOD_PROPERTY(L"Description", L"Verfies that the overlay pane will capture keyboard focus when opened and cycles focus within the pane when tab is pressed repeatedly.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateLightDismissOverlayMode)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the behavior of the LightDismissOverlayMode property.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(IsAutoLightDismissOverlayModeVisibleOnXbox)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting LightDismissOverlayMode to Auto on Xbox causes the overlay to be visible.")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateOverlayBrush)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the brush used for the overlay matches the 'SplitViewLightDismissOverlayBackground' resource.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateOverlayUIETree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates UIElement tree with an overlay-enabled SplitView.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // Move windowed popups to lifted input
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(OpenSplitViewWithNoElementsFocused)
            TEST_METHOD_PROPERTY(L"Description", L"When a SplitView is opened when nothing has focus, it should correctly focus its Pane")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoesNotFireOpenedOrClosedEventOnDisplayModeChange)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the Opened event is not re-fired if changing display modes while open.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoesFireOpenedAndClosedEventsWhenRetemplated)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that the opened &closed events fire even when SplitView is re-templated to excluded the DisplayModesStates state group.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanDragFromPane)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that we can drag an item from the SplitView pane to another location.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop") // TODO: 31563479 - Mouse input helper doesn't work on OneCore
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            TEST_METHOD_PROPERTY(L"Ignore", L"True")
        END_TEST_METHOD()

    private:
        void ValidateLightDismissBehaviorWorker(
            xaml_controls::SplitViewDisplayMode displayMode,
            xaml_controls::SplitViewPanePlacement placement,
            xaml::FlowDirection flowDirection,
            bool shouldLightDismiss
            );

        void VerifyKeyboardFocusBehaviorWorker(xaml_controls::SplitViewDisplayMode displayMode, xaml_controls::SplitViewPanePlacement placement);

        void VerifyGamepadOrRemoteFocusBehaviorWorker(xaml_controls::SplitViewDisplayMode displayMode, xaml_controls::SplitViewPanePlacement placement, InputDevice device);

        void ValidateElementResizeCountForTransitionsWorker(
            xaml_controls::SplitViewDisplayMode displayMode,
            xaml_controls::SplitViewPanePlacement placement,
            size_t expectedContentAreaClosedToOpenCount,
            size_t expectedContentAreaOpenToClosedCount,
            size_t expectedPaneAreaClosedToOpenCount,
            size_t expectedPaneAreaOpenToClosedCount
            );

        void CanInteractWithItemsInOpenPaneWorker(
            xaml_controls::SplitViewDisplayMode displayMode,
            xaml_controls::SplitViewPanePlacement placement,
            xaml::FlowDirection flowDirection
            );

        void CanDragFromPane(Platform::String^ targetListViewName);

        void CanNotShiftTabOutOfPaneWhenContentsIsListViewWorker(xaml_controls::SplitViewDisplayMode displayMode);

        // Builds up a tree of SplitView elements that are configured in all the various
        // state combinations that we care about.
        static xaml_controls::Panel^ BuildAllStatesTree();

        Platform::String^ GetResourcesPath() const;

        Microsoft::UI::Xaml::Tests::Common::RuntimeEnabledFeatureOverride featureDisableTransitionsForTest;
    };

} } } } } }
