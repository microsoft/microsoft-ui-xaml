// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CommandBarFlyoutCommandBar.g.h"
#include "CommandBarFlyoutCommandBar.properties.h"
#include "CommandBarFlyoutTrace.h"

enum class CommandBarFlyoutOpenCloseAnimationKind
{
    Opacity,
    Clip
};

class CommandBarFlyoutCommandBar :
    public ReferenceTracker<CommandBarFlyoutCommandBar, winrt::implementation::CommandBarFlyoutCommandBarT>,
    public CommandBarFlyoutCommandBarProperties
{
public:
    CommandBarFlyoutCommandBar();
    ~CommandBarFlyoutCommandBar();

    // IFrameworkElementOverrides
    void OnApplyTemplate();

    void SetOwningFlyout(winrt::CommandBarFlyout const& owningFlyout);

    bool HasOpenAnimation();
    CommandBarFlyoutOpenCloseAnimationKind OpenAnimationKind() { return m_openAnimationKind; };
    void PlayOpenAnimation();
    bool HasCloseAnimation();
    bool HasSecondaryOpenCloseAnimations();
    void PlayCloseAnimation(const winrt::weak_ref<winrt::CommandBarFlyout>& weakCommandBarFlyout, std::function<void()> onCompleteFunc);

    void BindOwningFlyoutPresenterToCornerRadius();

    void ClearShadow();
    void SetPresenter(winrt::FlyoutPresenter const& presenter);

    // IControlOverrides / IControlOverridesHelper
    void OnKeyDown(winrt::KeyRoutedEventArgs const& args);

    void OnCommandBarElementDependencyPropertyChanged();

    bool m_commandBarFlyoutIsOpening{ false };

    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

    void CacheLocalizedStringResources();
    void ClearLocalizedStringResourceCache();

private:
    void AttachEventHandlers();
    void DetachEventHandlers();

    void UpdateFlowsFromAndFlowsTo();
    void UpdateUI(bool useTransitions = true, bool isForSizeChange = false);
    void UpdateVisualState(bool useTransitions, bool isForSizeChange = false);
    void UpdateTemplateSettings();
    void EnsureAutomationSetCountAndPosition();
    void EnsureLocalizedControlTypes();
    void SetKnownCommandLocalizedControlTypes(winrt::ICommandBarElement const& command);
    void PopulateAccessibleControls();

    void SetPresenterName(winrt::FlyoutPresenter const& presenter);

    static bool IsControlFocusable(
        winrt::Control const& control,
        bool checkTabStop);
    static winrt::Control GetFirstTabStopControl(
        winrt::IObservableVector<winrt::ICommandBarElement> const& commands);
    static winrt::IAsyncOperation<bool> FocusControlAsync(
        winrt::Control newFocus,
        winrt::Control oldFocus,
        winrt::FocusState focusState,
        bool updateTabStop);
    static winrt::IAsyncOperation<bool> FocusCommandAsync(
        winrt::IObservableVector<winrt::ICommandBarElement> commands,
        winrt::Control moreButton,
        winrt::FocusState focusState,
        bool firstCommand,
        bool ensureTabStopUniqueness);
    static bool FocusControlSync(
        winrt::Control const& newFocus,
        winrt::Control const& oldFocus,
        winrt::FocusState const& focusState,
        bool updateTabStop);
    static bool FocusCommandSync(
        winrt::IObservableVector<winrt::ICommandBarElement> const& commands,
        winrt::Control const& moreButton,
        winrt::FocusState const& focusState,
        bool firstCommand,
        bool ensureTabStopUniqueness);
    static void EnsureTabStopUniqueness(
        winrt::IObservableVector<winrt::ICommandBarElement> const& commands,
        winrt::Control const& moreButton);

    tracker_ref<winrt::FrameworkElement> m_primaryItemsRoot{ this };
    tracker_ref<winrt::Popup> m_overflowPopup{ this };
    tracker_ref<winrt::FrameworkElement> m_secondaryItemsRoot{ this };
    tracker_ref<winrt::ButtonBase> m_moreButton{ this };
    weak_ref<winrt::CommandBarFlyout> m_owningFlyout{ nullptr };
    winrt::Popup::ActualPlacementChanged_revoker m_overflowPopupActualPlacementChangedRevoker{};
    RoutedEventHandler_revoker m_keyDownRevoker{};
    winrt::UIElement::PreviewKeyDown_revoker m_secondaryItemsRootPreviewKeyDownRevoker{};
    winrt::FrameworkElement::SizeChanged_revoker m_secondaryItemsRootSizeChangedRevoker{};
    winrt::FrameworkElement::Loaded_revoker m_firstItemLoadedRevoker{};

    // We need to manually connect the end element of the primary items to the start element of the secondary items
    // for the purposes of UIA items navigation. To ensure that we only have the current start and end elements registered
    // (e.g., if the app adds a new start element to the secondary commands, we want to unregister the previous start element),
    // we'll save references to those elements.
    tracker_ref<winrt::FrameworkElement> m_currentPrimaryItemsEndElement{ this };
    tracker_ref<winrt::FrameworkElement> m_currentSecondaryItemsStartElement{ this };

    CommandBarFlyoutOpenCloseAnimationKind m_openAnimationKind{ CommandBarFlyoutOpenCloseAnimationKind::Clip };
    weak_ref<winrt::FlyoutPresenter> m_flyoutPresenter{};
    tracker_ref<winrt::Storyboard> m_openingStoryboard{ this };
    tracker_ref<winrt::Storyboard> m_closingStoryboard{ this };
    winrt::Storyboard::Completed_revoker m_openingStoryboardCompletedRevoker{};
    winrt::Storyboard::Completed_revoker m_closingStoryboardCompletedCallbackRevoker{};

    bool m_secondaryItemsRootSized{ false };

    void AttachEventsToSecondaryStoryboards();

    winrt::Storyboard::Completed_revoker m_expandedUpToCollapsedStoryboardRevoker{};
    winrt::Storyboard::Completed_revoker m_expandedDownToCollapsedStoryboardRevoker{};
    winrt::Storyboard::Completed_revoker m_collapsedToExpandedUpStoryboardRevoker{};
    winrt::Storyboard::Completed_revoker m_collapsedToExpandedDownStoryboardRevoker{};

    winrt::IVector<winrt::Control> m_horizontallyAccessibleControls{ nullptr };
    winrt::IVector<winrt::Control> m_verticallyAccessibleControls{ nullptr };

    tracker_ref<winrt::FrameworkElement> m_outerOverflowContentRootV2{ this };
    tracker_ref<winrt::FrameworkElement> m_primaryItemsSystemBackdropRoot{ this };
    tracker_ref<winrt::FrameworkElement> m_overflowPopupSystemBackdropRoot{ this };

    // These ContentExternalBackdropLink objects implement the backdrop behind the CommandBarFlyoutCommandBar. We don't
    // use the one built into Popup because we need to animate this backdrop using Storyboards in the CBFCB's template.
    // The one built into Popup is too high up in the Visual tree to be animated by a custom animation.
    winrt::ContentExternalBackdropLink m_backdropLink{ nullptr };
    winrt::ContentExternalBackdropLink m_overflowPopupBackdropLink{ nullptr };

    // A copy of the value in the DependencyProperty. We need to unregister with this SystemBackdrop when this
    // CommandBarFlyoutCommandBar is deleted, but the DP value is already cleared by the time we get to Unloaded or the
    // dtor, so we cache a copy for ourselves to use during cleanup. Another possibility is to do cleanup during Closed,
    // but the app can release and delete this CommandBarFlyoutCommandBar without ever closing it.
    weak_ref<winrt::SystemBackdrop> m_systemBackdrop{ nullptr };

    // Localized string caches. Looking these up from MRTCore is expensive, so we don't want to put the lookups in a
    // loop. Instead, look them up once, cache them, use the cached values, then clear the cache. The values in these
    // caches are only valid after CacheLocalizedStringResources and before ClearLocalizedStringResourceCache.
    bool m_areLocalizedStringResourcesCached{ false };
    winrt::hstring m_localizedCommandBarFlyoutAppBarButtonControlType{};
    winrt::hstring m_localizedCommandBarFlyoutAppBarToggleButtonControlType{};
};
