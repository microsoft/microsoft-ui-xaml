// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <map>

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Automation { namespace Peers
{
    typedef std::map<IInspectable*, ILoopingSelectorItemDataAutomationPeer*> PeerMap;

    class LoopingSelectorAutomationPeer :
        public LoopingSelectorAutomationPeerGenerated
    {

    public:
         // Internal automation methods
        _Check_return_ HRESULT GetDataAutomationPeerForItem(_In_ IInspectable* pItem, _Outptr_ xaml_automation_peers::ILoopingSelectorItemDataAutomationPeer** ppPeer);
        _Check_return_ HRESULT GetContainerAutomationPeerForItem(_In_ IInspectable* pItem, _Outptr_result_maybenull_ xaml_automation_peers::ILoopingSelectorItemAutomationPeer** ppPeer);
        _Check_return_ HRESULT TryScrollItemIntoView(_In_ IInspectable* pItem);
        _Check_return_ HRESULT ClearPeerMap();
        _Check_return_ HRESULT RealizeItemAtIndex(_In_ INT index);

    protected:

        // IAutomationPeerOverrides
        _Check_return_ HRESULT GetPatternCoreImpl(
            _In_ xaml::Automation::Peers::PatternInterface patternInterface,
            _Outptr_ IInspectable **returnValue) override;
        _Check_return_ HRESULT GetAutomationControlTypeCoreImpl(
            _Out_ xaml::Automation::Peers::AutomationControlType *returnValue) override;
        _Check_return_ HRESULT GetChildrenCoreImpl(
            _Outptr_ wfc::IVector<xaml::Automation::Peers::AutomationPeer*> **returnValue) override;
        _Check_return_ HRESULT GetClassNameCoreImpl(
            _Out_ HSTRING *returnValue) override;

    private:
        ~LoopingSelectorAutomationPeer();

        _Check_return_ HRESULT InitializeImpl(
            _In_ xaml_primitives::ILoopingSelector* pOwner) override;

        _Check_return_ HRESULT GetOwnerAsInternalPtrNoRef(_Outptr_ xaml_primitives::LoopingSelector** ppOwnerNoRef);

        _Check_return_ HRESULT FindStartIndex(
            _In_opt_ xaml::Automation::Provider::IIRawElementProviderSimple* pStartAfter,
            _In_ wfc::IVector<IInspectable*>* pItems,
            _Out_ INT* pIndex);

        // IScrollProvider Impl
        _Check_return_ HRESULT get_HorizontallyScrollableImpl(_Out_ BOOLEAN* pValue) override;
        _Check_return_ HRESULT get_HorizontalScrollPercentImpl(_Out_ DOUBLE* pValue) override;
        _Check_return_ HRESULT get_HorizontalViewSizeImpl(_Out_ DOUBLE* pValue) override;
        _Check_return_ HRESULT get_VerticallyScrollableImpl(_Out_ BOOLEAN* pValue) override;
        _Check_return_ HRESULT get_VerticalScrollPercentImpl(_Out_ DOUBLE* pValue) override;
        _Check_return_ HRESULT get_VerticalViewSizeImpl(_Out_ DOUBLE* pValue) override;

    public:
        _Check_return_ HRESULT ScrollImpl (
            _In_ xaml_automation::ScrollAmount horizontalAmount,
            _In_ xaml_automation::ScrollAmount verticalAmount);

        _Check_return_ HRESULT SetScrollPercentImpl (
            _In_ DOUBLE horizontalPercent,
            _In_ DOUBLE verticalPercent);

          // IItemsContainerProvider Impl
        _Check_return_ HRESULT FindItemByPropertyImpl(
            _In_opt_ xaml_automation::Provider::IIRawElementProviderSimple* startAfter,
            _In_opt_ xaml_automation::IAutomationProperty* automationProperty,
            _In_opt_ IInspectable* value,
            _Outptr_ xaml_automation::Provider::IIRawElementProviderSimple** returnValue);

         // ISelectionProvider
        _Check_return_ HRESULT get_CanSelectMultipleImpl(_Out_ BOOLEAN *value) override;
        _Check_return_ HRESULT get_IsSelectionRequiredImpl(_Out_ BOOLEAN *value) override;

        _Check_return_ HRESULT GetSelectionImpl(
            _Out_ UINT32* pReturnValueSize,
            _Outptr_result_buffer_maybenull_(*pReturnValueSize) xaml_automation::Provider::IIRawElementProviderSimple ***pppReturnValue);

    private:
        // PeerMap is used to keep a 1:1 mapping between items in the list and
        // their respective data automation peers. This is done to ensure that
        // no duplicate data peers are created for items. This class does not
        // keep a ref on the data peers, and the peers are responsible for removing
        // themselves from this map when deconstructed.
        PeerMap _peerMap;
    };

} } } } } XAML_ABI_NAMESPACE_END
