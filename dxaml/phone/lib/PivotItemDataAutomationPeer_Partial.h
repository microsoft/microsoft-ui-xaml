// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Automation { namespace Peers
{
    class PivotItemDataAutomationPeer :
        public PivotItemDataAutomationPeerGenerated
    {

    public:


        // IAutomationPeerOverrides

        _Check_return_ HRESULT GetPatternCoreImpl(
            _In_ xaml::Automation::Peers::PatternInterface patternInterface,
            _Outptr_ IInspectable **returnValue) override;

        _Check_return_ HRESULT GetBoundingRectangleCoreImpl(
            _Out_ wf::Rect* returnValue);

    private:
        ~PivotItemDataAutomationPeer() {}

        _Check_return_ HRESULT InitializeImpl(
            _In_ IInspectable* item,
            _In_ IPivotAutomationPeer* parent) override;

    public:
        // ISelectionItemProvider

        _Check_return_ HRESULT get_IsSelectedImpl(
            _Out_ boolean *value) override;

        _Check_return_ HRESULT get_SelectionContainerImpl(
            _Outptr_ xaml::Automation::Provider::IIRawElementProviderSimple **value) override;

        _Check_return_ HRESULT AddToSelectionImpl();
        _Check_return_ HRESULT RemoveFromSelectionImpl();
        _Check_return_ HRESULT SelectImpl();

        // IScrollItemProvider

        _Check_return_ HRESULT ScrollIntoViewImpl();

        // IVirtualizedItemProvider

        _Check_return_ HRESULT RealizeImpl();
    };

    ActivatableClassWithFactory(PivotItemDataAutomationPeer, PivotItemDataAutomationPeerFactory);

} } } } } XAML_ABI_NAMESPACE_END
