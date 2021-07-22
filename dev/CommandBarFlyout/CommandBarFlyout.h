// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CommandBarFlyout.g.h"
#include "CommandBarFlyoutTrace.h"

class CommandBarFlyout :
    public ReferenceTracker<CommandBarFlyout, winrt::implementation::CommandBarFlyoutT>
{
public:
    CommandBarFlyout();
    ~CommandBarFlyout();

    bool AlwaysExpanded() { return m_alwaysExpanded; }
    void AlwaysExpanded(bool value) { m_alwaysExpanded = value; }

    winrt::IObservableVector<winrt::ICommandBarElement> PrimaryCommands();
    winrt::IObservableVector<winrt::ICommandBarElement> SecondaryCommands();

    // IFlyoutOverrides overrides
    winrt::Control CreatePresenter();

    void AddDropShadow();
    void RemoveDropShadow();
    tracker_ref<winrt::FlyoutPresenter> GetPresenter();

protected:
    tracker_ref<winrt::CommandBarFlyoutCommandBar> m_commandBar{ this };

private:
    static constexpr int s_commandBarElementDependencyPropertiesCount{ 3 };
    static constexpr int s_commandBarElementDependencyPropertiesCountRS3{ 2 };

    static winrt::DependencyProperty s_appBarButtonDependencyProperties[s_commandBarElementDependencyPropertiesCount];
    static winrt::DependencyProperty s_appBarToggleButtonDependencyProperties[s_commandBarElementDependencyPropertiesCount];

    void SetSecondaryCommandsToCloseWhenExecuted();
    void HookAppBarButtonDependencyPropertyChanges(winrt::AppBarButton const& appBarButton, int index);
    void HookAppBarToggleButtonDependencyPropertyChanges(winrt::AppBarToggleButton const& appBarToggleButton, int index);
    void HookAllCommandBarElementDependencyPropertyChanges();
    void UnhookCommandBarElementDependencyPropertyChanges(int index, bool eraseRevokers = true);
    void UnhookAllCommandBarElementDependencyPropertyChanges();
    void OnCommandBarElementDependencyPropertyChanged(winrt::DependencyObject const& dependencyObject, winrt::DependencyProperty const& dependencyProperty);

    bool m_alwaysExpanded;

    winrt::IObservableVector<winrt::ICommandBarElement> m_primaryCommands{ nullptr };
    winrt::IObservableVector<winrt::ICommandBarElement> m_secondaryCommands{ nullptr };

    // winrt::IObservableVector<winrt::ICommandBarElement>::VectorChanged_revoker throws a weird build error when used,
    // so we're using the event token pattern for manual detachment instead.
    winrt::event_token m_primaryCommandsVectorChangedToken{};
    winrt::event_token m_secondaryCommandsVectorChangedToken{};

    winrt::CommandBar::Opened_revoker m_commandBarOpenedRevoker{};
    winrt::CommandBar::Opening_revoker m_commandBarOpeningRevoker{};
    winrt::CommandBar::Closed_revoker m_commandBarClosedRevoker{};
    winrt::CommandBar::Closing_revoker m_commandBarClosingRevoker{};

    std::map<int, winrt::ButtonBase::Click_revoker> m_secondaryButtonClickRevokerByIndexMap;
    std::map<int, winrt::ToggleButton::Checked_revoker> m_secondaryToggleButtonCheckedRevokerByIndexMap;
    std::map<int, winrt::ToggleButton::Unchecked_revoker> m_secondaryToggleButtonUncheckedRevokerByIndexMap;
    std::map<int, PropertyChanged_revoker[s_commandBarElementDependencyPropertiesCount]> m_propertyChangedRevokersByIndexMap;

    tracker_ref<winrt::FlyoutPresenter> m_presenter{ this };

    bool m_isClosingAfterCloseAnimation{ false };
}; 

