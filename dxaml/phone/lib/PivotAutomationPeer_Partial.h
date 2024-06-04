// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Automation { namespace Peers
{
    class PivotAutomationPeer :
        public PivotAutomationPeerGenerated
    {

    public:

        // IAutomationPeerOverrides methods

        _Check_return_ HRESULT GetPatternCoreImpl(
            _In_ xaml::Automation::Peers::PatternInterface patternInterface,
            _Outptr_ IInspectable **returnValue) override;

        _Check_return_ HRESULT GetAutomationControlTypeCoreImpl(
            _Out_ xaml::Automation::Peers::AutomationControlType *returnValue) override;

        _Check_return_ HRESULT OnCreateItemAutomationPeerImpl(
            _In_ IInspectable* pItem,
            _Outptr_ xaml_automation_peers::IItemAutomationPeer** ppReturnValue);

        _Check_return_ HRESULT GetClassNameCoreImpl(
            _Out_ HSTRING *returnValue) override;

        _Check_return_ HRESULT GetNameCoreImpl(
            _Out_ HSTRING *returnValue) override;

        _Check_return_ HRESULT GetChildrenCoreImpl(
            _Outptr_ wfc::IVector<xaml::Automation::Peers::AutomationPeer*> **returnValue) override;

        _Check_return_ HRESULT GetElementFromPointCoreImpl(
            _In_ wf::Point pointInWindowCoordinates,
            _Outptr_ IInspectable** returnValue) override;

    private:
        ~PivotAutomationPeer() {}

        _Check_return_ HRESULT InitializeImpl(
            _In_ xaml_controls::IPivot* pOwner) override;

    public:
        // ISelectionProvider

        _Check_return_ HRESULT get_CanSelectMultipleImpl(
            _Out_ boolean *value) override;

        _Check_return_ HRESULT get_IsSelectionRequiredImpl(
            _Out_ boolean *value) override;

        _Check_return_ HRESULT GetSelectionImpl(
            _Out_ UINT32* pReturnValueSize,
            _Outptr_result_buffer_maybenull_(*pReturnValueSize) xaml::Automation::Provider::IIRawElementProviderSimple ***pppReturnValue);

        // IScrollProvider

        _Check_return_ HRESULT get_HorizontallyScrollableImpl(
            _Out_ BOOLEAN *pValue) override;

        _Check_return_ HRESULT get_HorizontalScrollPercentImpl(
            _Out_ DOUBLE *pValue) override;

        _Check_return_ HRESULT get_HorizontalViewSizeImpl(
            _Out_ DOUBLE *pValue) override;

        _Check_return_ HRESULT get_VerticallyScrollableImpl(
            _Out_ BOOLEAN *pValue) override;

        _Check_return_ HRESULT get_VerticalScrollPercentImpl(
            _Out_ DOUBLE *pValue) override;

        _Check_return_ HRESULT get_VerticalViewSizeImpl(
            _Out_ DOUBLE *pValue) override;

        _Check_return_ HRESULT ScrollImpl(
            _In_ xaml_automation::ScrollAmount horizontalAmount,
            _In_ xaml_automation::ScrollAmount verticalAmount);

        _Check_return_ HRESULT SetScrollPercentImpl(
            _In_ DOUBLE horizontalPercent,
            _In_ DOUBLE verticalPercent);

    private:
        _Check_return_ HRESULT AddAutomationPeerChildrenToCollection(
            _In_ xaml::IUIElement *element,
            _In_ wfc::IVector<xaml::Automation::Peers::AutomationPeer*> *collection,
            _In_ bool addAtEnd);
    };

    ActivatableClassWithFactory(PivotAutomationPeer, PivotAutomationPeerFactory);

} } } } } XAML_ABI_NAMESPACE_END
