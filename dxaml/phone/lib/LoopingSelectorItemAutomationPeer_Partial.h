// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Automation { namespace Peers
{
    class LoopingSelectorItemAutomationPeer :
        public LoopingSelectorItemAutomationPeerGenerated
    {

    public:

        _Check_return_ HRESULT UpdateEventSource();
        _Check_return_ HRESULT SetEventSource(_In_ xaml_automation_peers::ILoopingSelectorItemDataAutomationPeer* pLSIDAP);
        _Check_return_ HRESULT UpdateItemIndex(_In_ int itemIndex);

        // IAutomationPeerOverrides
        _Check_return_ HRESULT GetPatternCoreImpl(
            _In_ xaml::Automation::Peers::PatternInterface patternInterface,
            _Outptr_ IInspectable **returnValue) override;
        _Check_return_ HRESULT GetAutomationControlTypeCoreImpl(
            _Out_ xaml::Automation::Peers::AutomationControlType *returnValue) override;
        _Check_return_ HRESULT GetClassNameCoreImpl(
            _Out_ HSTRING *returnValue) override;

        _Check_return_ HRESULT IsKeyboardFocusableCoreImpl(_Out_ BOOLEAN* returnValue) override;
        _Check_return_ HRESULT HasKeyboardFocusCoreImpl(_Out_ BOOLEAN* returnValue) override;

    private:

        ~LoopingSelectorItemAutomationPeer() {}

        _Check_return_ HRESULT InitializeImpl(
            _In_ xaml_primitives::ILoopingSelectorItem* pOwner) override;

        _Check_return_ HRESULT GetOwnerAsInternalPtrNoRef(_Outptr_ xaml_primitives::LoopingSelectorItem** ppOwnerNoRef);
        _Check_return_ HRESULT GetDataAutomationPeer(_Outptr_ xaml_automation_peers::ILoopingSelectorItemDataAutomationPeer** ppLSIDAP);

    public:
         // IScrollItemProvider Impl
        _Check_return_ HRESULT ScrollIntoViewImpl();

        // ISelectionItemProvider Impl
        _Check_return_ HRESULT get_IsSelectedImpl(_Out_ BOOLEAN *value) override;

        _Check_return_ HRESULT get_SelectionContainerImpl(
            _Outptr_ xaml_automation::Provider::IIRawElementProviderSimple **value) override;

        _Check_return_ HRESULT AddToSelectionImpl();
        _Check_return_ HRESULT RemoveFromSelectionImpl();
        _Check_return_ HRESULT SelectImpl();

    private:
        _Check_return_ HRESULT GetParentAutomationPeer(_Outptr_ xaml_automation_peers::IAutomationPeer **parentAutomationPeer);
    };

} } } } } XAML_ABI_NAMESPACE_END

