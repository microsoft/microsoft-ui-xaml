// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "NavigationViewItem.h"

#include "NavigationViewItemAutomationPeer.g.h"

class NavigationViewItemAutomationPeer :
    public ReferenceTracker<
        NavigationViewItemAutomationPeer,
        winrt::implementation::NavigationViewItemAutomationPeerT,
        winrt::IInvokeProvider, winrt::ISelectionItemProvider>
{
public:
    NavigationViewItemAutomationPeer(winrt::NavigationViewItem const& owner);

    // IAutomationPeerOverrides 
    winrt::hstring GetNameCore();
    winrt::IInspectable GetPatternCore(winrt::PatternInterface const& patternInterface);
    winrt::AutomationControlType GetAutomationControlTypeCore();
    winrt::hstring GetClassNameCore();

    // IAutomationPeerOverrides3
    int32_t GetPositionInSetCore();
    int32_t GetSizeOfSetCore();
    int32_t GetLevelCore();

    // IInvokeProvider
    void Invoke();

    // ISelectionItemProvider
    bool IsSelected();
    winrt::IRawElementProviderSimple SelectionContainer();
    void AddToSelection();
    void RemoveFromSelection();
    void Select();

    // IExpandCollapseProvider
    winrt::ExpandCollapseState ExpandCollapseState();
    void Collapse();
    void Expand();
    void RaiseExpandCollapseAutomationEvent(winrt::ExpandCollapseState newState);

private:

    enum class AutomationOutput
    {
        Position,
        Size,
    };

    winrt::NavigationView GetParentNavigationView();
    winrt::ItemsRepeater GetParentRepeater();
    bool IsOnTopNavigation();
    bool IsOnTopNavigationOverflow();
    bool IsSettingsItem();
    NavigationViewRepeaterPosition GetNavigationViewRepeaterPosition();
    int32_t GetNavigationViewItemCountInPrimaryList();
    int32_t GetNavigationViewItemCountInTopNav();
    int32_t GetPositionOrSetCountInLeftNavHelper(AutomationOutput automationOutput);
    int32_t GetPositionOrSetCountInTopNavHelper(AutomationOutput automationOutput);
    void ChangeSelection(bool isSelected);
};
