// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Controls
{
class PivotItem :
    public PivotItemGenerated
{

public:
    PivotItem();

    // IFrameworkElementOverrides methods
    _Check_return_ HRESULT MeasureOverrideImpl(
        _In_ wf::Size availableSize,
        _Out_ wf::Size* desiredSize) override;
    _Check_return_ HRESULT OnApplyTemplateImpl() override;

    // IUIElementOverrides
    _Check_return_ HRESULT OnCreateAutomationPeerImpl(
        _Outptr_ xaml::Automation::Peers::IAutomationPeer **returnValue) override;

    _Check_return_ HRESULT GetContentVisibility(
        _Out_ xaml::Visibility* visibility);

    _Check_return_ HRESULT SetContentVisibility(
        _In_ xaml::Visibility visibility);

    _Check_return_ HRESULT TrySetContentIsEnabled(
        _In_ bool isEnabled, _Out_ BOOLEAN* successful);

    _Check_return_ HRESULT RealizeContent();

    _Check_return_ HRESULT RegisterSlideInElementNoRef(_In_ xaml::IFrameworkElement* pElement);
    _Check_return_ HRESULT UnregisterSlideInElementNoRef(_In_ xaml::IFrameworkElement* pElement);
    const std::list<xaml::IFrameworkElement*>& GetRegisteredSlideInElementsNoRef() { return m_slideInElementsNoRef; }

    void SetParent(_In_ wrl::ComPtr<xaml_controls::IPivot> spParent);
    wrl::WeakRef GetParent();
    _Check_return_ HRESULT GetFirstChild(_Outptr_result_maybenull_ IUIElement** ppFirstChild);

    _Check_return_ HRESULT GetFirstIControlChild(_Outptr_result_maybenull_ IControl** ppFirstIControlChild);

    _Check_return_ HRESULT SetKeyboardFocus(_In_ bool postponeUntilNextMeasure);

    _Check_return_ HRESULT HasFocusedElement(_Out_ bool* hasFocusedElement);

    _Check_return_ HRESULT HasElement(_In_ xaml::IDependencyObject* element, _Out_ bool* hasElement);

protected:
    _Check_return_ HRESULT InitializeImpl(_In_opt_ IInspectable* outer) override;

private:
    wrl::ComPtr<xaml::IUIElementStaticsPrivate> m_spUIElementStaticsPrivate;

    wrl::WeakRef m_wpParent;
    // List of elements that are going to slide-in when this pivot item gets selected.
    std::list<xaml::IFrameworkElement*> m_slideInElementsNoRef;
    bool m_isContentVisibilityPending;
    bool m_isKeyboardFocusPending;
    xaml::Visibility m_pendingContentVisibility;
    wf::Size m_lastMeasureAvailableSize;

    EventRegistrationToken m_pivotItemAccessKeyInvokedToken;
    _Check_return_ HRESULT OnPivotItemAccessKeyInvoked(_In_ IUIElement* sender, _In_ xaml_input::IAccessKeyInvokedEventArgs* args);
    _Check_return_ HRESULT OnKeyboardAcceleratorInvokedImpl(_In_ xaml_input::IKeyboardAcceleratorInvokedEventArgs* args) override;
    _Check_return_ HRESULT OnProgramaticHeaderItemTapped(bool *isHandled);

    friend class xaml_automation_peers::PivotItemAutomationPeer;
};

ActivatableClassWithFactory(PivotItem, PivotItemFactory);

} } } } XAML_ABI_NAMESPACE_END
