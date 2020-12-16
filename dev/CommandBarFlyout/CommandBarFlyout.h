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

    void OnApplyTemplate();

    winrt::IObservableVector<winrt::ICommandBarElement> PrimaryCommands();
    winrt::IObservableVector<winrt::ICommandBarElement> SecondaryCommands();

    // IFlyoutOverrides overrides
    winrt::Control CreatePresenter();

protected:
    tracker_ref<winrt::CommandBarFlyoutCommandBar> m_commandBar{ this };
    tracker_ref<winrt::Popup> m_acrylicBackgroundPopup{ this };

private:
    void SetSecondaryCommandsToCloseWhenExecuted();
    void CreateAcrylicBackgroundPopup();
    void ToggleAcrylicBackgroundPopup();

    winrt::IObservableVector<winrt::ICommandBarElement> m_primaryCommands{ nullptr };
    winrt::IObservableVector<winrt::ICommandBarElement> m_secondaryCommands{ nullptr };

    // winrt::IObservableVector<winrt::ICommandBarElement>::VectorChanged_revoker throws a weird build error when used,
    // so we're using the event token pattern for manual detachment instead.
    winrt::event_token m_primaryCommandsVectorChangedToken{};
    winrt::event_token m_secondaryCommandsVectorChangedToken{};

    winrt::CommandBar::Opened_revoker m_commandBarOpenedRevoker{};

    std::map<int, winrt::ButtonBase::Click_revoker> m_secondaryButtonClickRevokerByIndexMap;
    std::map<int, winrt::ToggleButton::Checked_revoker> m_secondaryToggleButtonCheckedRevokerByIndexMap;
    std::map<int, winrt::ToggleButton::Unchecked_revoker> m_secondaryToggleButtonUncheckedRevokerByIndexMap;

    bool m_isClosingAfterCloseAnimation{ false };
}; 

