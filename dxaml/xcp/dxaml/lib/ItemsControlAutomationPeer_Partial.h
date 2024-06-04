// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ItemsControlAutomationPeer.g.h"

namespace DirectUI
{
    class ItemAutomationPeer;

    // Represents the ItemsControlAutomationPeer
    PARTIAL_CLASS(ItemsControlAutomationPeer)
    {
        public:
            // Initializes a new instance of the ButtonAutomationPeer class.
            ItemsControlAutomationPeer();
            ~ItemsControlAutomationPeer() override;

            IFACEMETHOD(GetPatternCore)(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** returnValue);
            IFACEMETHOD(GetChildrenCore)(_Outptr_ wfc::IVector<xaml_automation_peers::AutomationPeer*>** returnValue);

            _Check_return_ HRESULT CreateItemAutomationPeerImpl(_In_ IInspectable* item, _Outptr_ xaml_automation_peers::IItemAutomationPeer** returnValue);
            virtual _Check_return_ HRESULT OnCreateItemAutomationPeerImpl(
                _In_ IInspectable* item,
                _Outptr_ xaml_automation_peers::IItemAutomationPeer** returnValue);

            _Check_return_ HRESULT OnCreateItemAutomationPeerProtected(
                _In_ IInspectable* pItem,
                _Outptr_ xaml_automation_peers::IItemAutomationPeer** ppReturnValue) override;

            virtual _Check_return_ HRESULT OnCollectionChanged(
                _In_ wfc::IObservableVector<IInspectable*>* pSender,
                _In_ wfc::IVectorChangedEventArgs* e);

            _Check_return_ HRESULT GetItemsControlChildrenChildren(_In_ wfc::IVector<xaml_automation_peers::AutomationPeer*>* pAPChildren) noexcept;

            // IItemContainerProvider
            _Check_return_ HRESULT FindItemByPropertyImpl(_In_opt_ xaml_automation::Provider::IIRawElementProviderSimple* pStartAfterAsRaw, _In_opt_ xaml_automation::IAutomationProperty* pProperty, _In_opt_ IInspectable* pValue, _Outptr_ xaml_automation::Provider::IIRawElementProviderSimple** returnValue) noexcept;

            _Check_return_ HRESULT RemoveItemAutomationPeerFromStorage(_In_opt_ ItemAutomationPeer* pItemPeer, bool forceRemoveItemPeer = false);

            _Check_return_ HRESULT GetItemPeerFromItemContainerCache(
                _In_ IInspectable* pItem,
                _Outptr_ xaml_automation_peers::IItemAutomationPeer** ppItemPeer,
                BOOLEAN &bFoundInCache);

            _Check_return_ HRESULT GetItemPeerFromChildrenCache(
                _In_ IInspectable* pItem,
                _Outptr_ xaml_automation_peers::IItemAutomationPeer** ppItemPeer);

            _Check_return_ HRESULT GetPositionInSetHelper(_In_ xaml::IUIElement* pItemContainer, _Out_ INT* returnValue);
            _Check_return_ HRESULT GetSizeOfSetHelper(_In_ xaml::IUIElement* pItemContainer, _Out_ INT* returnValue);


            _Check_return_ HRESULT ReleaseItemPeerStorage(_In_ wfc::IVector<xaml_automation_peers::ItemAutomationPeer*>* pItemPeerStorage);
            _Check_return_ HRESULT AddItemAutomationPeerToItemPeerStorage(_In_ ItemAutomationPeer* pItemPeer);
            _Check_return_ HRESULT GenerateEventsSourceForContainerItemPeer(_In_ xaml_automation_peers::IFrameworkElementAutomationPeer* pItemContainerAP);

            static _Check_return_ HRESULT GenerateAutomationPeerEventsSourceStatic(_In_ xaml_automation_peers::IFrameworkElementAutomationPeer* pItemContainerAP, _In_ xaml_automation_peers::IAutomationPeer* pAPParent);

        private:
            _Check_return_ HRESULT GetItemsControlChildrenChildrenHelper(_In_ xaml_controls::IItemsControl* pOwner,
                _In_ wfc::IVector<xaml::UIElement*>* pItemsFromItemsHostPanel,
                _In_ UINT nCount,
                _In_ wfc::IVector<xaml_automation_peers::ItemAutomationPeer*>* pNewChildrenCollection);

            _Check_return_ HRESULT GetModernItemsControlChildrenChildrenHelper(_In_ wfc::IVector<xaml_automation_peers::ItemAutomationPeer*>* pNewChildrenCollection);

            void ClearCacheCollectionUnsafe(_In_ TrackerCollection<xaml_automation_peers::ItemAutomationPeer*>* unsafeTrackerCollection);

            TrackerPtr<TrackerCollection<xaml_automation_peers::ItemAutomationPeer*>> m_tpItemPeerStorage;
            TrackerPtr<TrackerCollection<xaml_automation_peers::ItemAutomationPeer*>> m_tpItemPeerStorageForPattern;

            INT m_lastIndex;
    };
}
