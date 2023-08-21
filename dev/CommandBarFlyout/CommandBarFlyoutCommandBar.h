﻿// Copyright (c) Microsoft Corporation. All rights reserved.
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
    void EnsureFocusedPrimaryCommand();
    void PopulateAccessibleControls();

    void SetPresenterName(winrt::FlyoutPresenter const& presenter);

    static bool IsControlFocusable(
        winrt::Control const& control,
        bool checkTabStop);
    static winrt::Control GetFirstTabStopControl(
        winrt::IObservableVector<winrt::ICommandBarElement> const& commands);
    static bool FocusControl(
        winrt::Control const& newFocus,
        winrt::Control const& oldFocus,
        winrt::FocusState const& focusState,
        bool updateTabStop);
    static bool FocusCommand(
        winrt::IObservableVector<winrt::ICommandBarElement> const& commands,
        winrt::Control const& moreButton,
        winrt::FocusState const& focusState,
        bool firstCommand,
        bool ensureTabStopUniqueness);
    static void EnsureTabStopUniqueness(
        winrt::IObservableVector<winrt::ICommandBarElement> const& commands,
        winrt::Control const& moreButton);

    void AddProjectedShadow();
    void UpdateProjectedShadow();
    void ClearProjectedShadow();

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
};
