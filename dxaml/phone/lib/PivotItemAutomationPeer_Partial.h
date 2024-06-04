// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Automation { namespace Peers
{
    class PivotItemAutomationPeer :
        public PivotItemAutomationPeerGenerated
    {

    public:


        // IAutomationPeerOverrides

        _Check_return_ HRESULT GetAutomationControlTypeCoreImpl(
            _Out_ xaml::Automation::Peers::AutomationControlType *returnValue) override;

        _Check_return_ HRESULT GetClassNameCoreImpl(
            _Out_ HSTRING *returnValue) override;

        _Check_return_ HRESULT GetNameCoreImpl(
            _Out_ HSTRING *returnValue) override;

        _Check_return_ HRESULT GetBoundingRectangleCoreImpl(
            _Out_ wf::Rect* returnValue) override;

        _Check_return_ HRESULT GetClickablePointCoreImpl(
            _Out_ wf::Point* returnValue) override;

        _Check_return_ HRESULT IsOffscreenCoreImpl(
            _Out_ BOOLEAN* returnValue) override;

        _Check_return_ HRESULT IsKeyboardFocusableCoreImpl(
            _Out_ BOOLEAN* returnValue) override;

        _Check_return_ HRESULT HasKeyboardFocusCoreImpl(
            _Out_ BOOLEAN* returnValue) override;

        _Check_return_ HRESULT SetFocusCoreImpl() override;

    private:
        ~PivotItemAutomationPeer() {}

        _Check_return_ HRESULT InitializeImpl(
            _In_ xaml_controls::IPivotItem* pOwner) override;

        _Check_return_ HRESULT GetPivot(
            _Out_ xaml_controls::Pivot** pivotNoRef);

        _Check_return_ HRESULT GetOwnerIndex(
            _Out_ xaml_controls::Pivot* pivotNoRef,
            _Out_ INT* index);
    };

    ActivatableClassWithFactory(PivotItemAutomationPeer, PivotItemAutomationPeerFactory);

} } } } } XAML_ABI_NAMESPACE_END
