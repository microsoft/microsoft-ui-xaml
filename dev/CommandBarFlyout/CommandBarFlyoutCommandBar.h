// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CommandBarFlyoutCommandBar.g.h"
#include "CommandBarFlyoutCommandBar.properties.h"

class CommandBarFlyoutCommandBar :
    public ReferenceTracker<CommandBarFlyoutCommandBar, winrt::implementation::CommandBarFlyoutCommandBarT>,
    public CommandBarFlyoutCommandBarProperties
{
public:
    CommandBarFlyoutCommandBar();
    virtual ~CommandBarFlyoutCommandBar();

    // IFrameworkElementOverrides
    void OnApplyTemplate();
    
    void SetOwningFlyout(winrt::CommandBarFlyout const& owningFlyout);

    bool HasOpenAnimation();
    void PlayOpenAnimation();
    bool HasCloseAnimation();
    void PlayCloseAnimation(std::function<void()> onCompleteFunc);
    void ClearShadow();

private:
    void AttachEventHandlers();
    void DetachEventHandlers(bool useSafeGet = false);

    void AddShadow();

    void UpdateUI(bool useTransitions = true);
    void UpdateVisualState(bool useTransitions);
    void UpdateTemplateSettings();
    void UpdateShadow();

    tracker_ref<winrt::FrameworkElement> m_primaryItemsRoot{ this };
    tracker_ref<winrt::FrameworkElement> m_secondaryItemsRoot{ this };
    weak_ref<winrt::CommandBarFlyout> m_owningFlyout{ nullptr };
    winrt::FrameworkElement::SizeChanged_revoker m_secondaryItemsRootSizeChangedRevoker{};
    winrt::IInspectable m_keyDownHandler{ nullptr };
    winrt::FrameworkElement::Loaded_revoker m_firstSecondaryItemLoadedRevoker{};

    tracker_ref<winrt::Storyboard> m_openingStoryboard{ this };
    tracker_ref<winrt::Storyboard> m_closingStoryboard{ this };
    winrt::Storyboard::Completed_revoker m_openingStoryboardCompletedRevoker{};
    winrt::Storyboard::Completed_revoker m_closingStoryboardCompletedRevoker{};
    winrt::Storyboard::Completed_revoker m_closingStoryboardCompletedCallbackRevoker{};

    bool m_secondaryItemsRootSized{ false };
};
