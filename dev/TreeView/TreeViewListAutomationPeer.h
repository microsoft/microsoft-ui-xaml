// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TreeViewListAutomationPeer.g.h"

class TreeViewListAutomationPeer :
    public ReferenceTracker<
        TreeViewListAutomationPeer,
        winrt::implementation::TreeViewListAutomationPeerT,
        winrt::IDropTargetProvider,
        winrt::ISelectionProvider>
{
public:
    TreeViewListAutomationPeer(winrt::TreeViewList const& owner);

    // IAutomationPeerOverrides
    winrt::IInspectable GetPatternCore(winrt::PatternInterface const& patternInterface);
    winrt::AutomationControlType GetAutomationControlTypeCore();

    // IItemsControlAutomationPeerOverrides2
    winrt::ItemAutomationPeer OnCreateItemAutomationPeer(winrt::IInspectable const& item);

    // IDropTargetProvider
    winrt::hstring DropEffect();
    winrt::com_array<winrt::hstring> DropEffects();

    // ISelectionProvider
    bool CanSelectMultiple();
    bool IsSelectionRequired();
    winrt::com_array<winrt::Windows::UI::Xaml::Automation::Provider::IRawElementProviderSimple> GetSelection();

private:
    bool IsMultiselect();
};
