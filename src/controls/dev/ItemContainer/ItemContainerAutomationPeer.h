// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ItemContainer.h"
#include "ItemContainerAutomationPeer.g.h"

class ItemContainerAutomationPeer :
    public ReferenceTracker<ItemContainerAutomationPeer, winrt::implementation::ItemContainerAutomationPeerT>
{
public:
    ItemContainerAutomationPeer(winrt::FrameworkElement const& owner);

    // IAutomationPeerOverrides 
    virtual winrt::hstring GetNameCore();
    winrt::IInspectable GetPatternCore(winrt::PatternInterface const& patternInterface);
    winrt::AutomationControlType GetAutomationControlTypeCore();
    virtual winrt::hstring GetLocalizedControlTypeCore();
    virtual winrt::hstring GetClassNameCore();

    // IInvokeProvider
    void Invoke();

    // ISelectionItemProvider
    bool IsSelected();
    winrt::IRawElementProviderSimple SelectionContainer();
    void AddToSelection();
    void RemoveFromSelection();
    void Select();

private:
    com_ptr<ItemContainer> GetImpl();
    void UpdateSelection(bool isSelected);
};
