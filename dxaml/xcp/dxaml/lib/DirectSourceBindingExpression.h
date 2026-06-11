// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Expression used to evaluate single-level bindings with an explicit source object.
//      This is an optimization for bindings like DXamlCore::SetBinding where
//      the source is an explicit IInspectable and the path is a single property name.
//
//      Based on TemplateBindingExpression. The design, member layout, attach/detach
//      lifecycle, and IDPChangedEventHandler pattern are all modeled after
//      TemplateBindingExpression but simplified for the explicit-source, single-property case.

#pragma once

#include "BindingExpressionBase.g.h"

namespace DirectUI
{
    // Lightweight binding expression for single-property bindings with an explicit source,
    // modeled after TemplateBindingExpression. Like TemplateBindingExpression, this class
    // implements IDPChangedEventHandler directly (rather than using a separate handler
    // object) to save one heap allocation per binding.
    //
    // Reference ownership notes (same model as TemplateBindingExpression):
    // - Source: The expression holds a weak reference (IWeakReference) to the source to
    //   avoid cycles, since the source's DPChangedEventSource holds a strong reference
    //   to this expression via its handler list.
    // - Target: The expression holds a no-ref raw pointer to the target. This is safe
    //   because the target owns this expression (via EffectiveValueEntry in its value
    //   table). The expression cannot outlive the target - OnDetach is always called
    //   before the expression is released, which clears m_pTarget. A strong reference
    //   would create a cycle (target -> expression -> target), and a weak reference
    //   would add unnecessary overhead for a back-pointer to the owning object.
    class DirectSourceBindingExpression :
        public BindingExpressionBase,
        public IDPChangedEventHandler
    {

    protected:
        DirectSourceBindingExpression() = default;
        ~DirectSourceBindingExpression() override;

        // Non-copyable, non-movable (pointers and registration state are not safe to duplicate)
        DirectSourceBindingExpression(const DirectSourceBindingExpression&) = delete;
        DirectSourceBindingExpression& operator=(const DirectSourceBindingExpression&) = delete;
        DirectSourceBindingExpression(DirectSourceBindingExpression&&) = delete;
        DirectSourceBindingExpression& operator=(DirectSourceBindingExpression&&) = delete;

    private:
        ctl::ComPtr<IWeakReference> m_spSourceRef;                // Weak reference to source object
        const CDependencyProperty* m_pSourceProperty = nullptr;   // Source property to read from
        DependencyObject* m_pTarget = nullptr;                    // The target instance (no-ref, prevented from dangling by attach/detach lifecycle)
        const CDependencyProperty* m_pTargetProperty = nullptr;   // The target property
        ctl::ComPtr<xaml_data::IValueConverter> m_spConverter;    // Optional value converter
        bool m_bRegisteredForSourceChanges = false;               // Flag indicating registration with source's property changed event
        bool m_ignoreSourcePropertyChanges = false;               // Flag to ignore source updates during initialization

        // Refresh the expression when the source object's property has changed.
        _Check_return_ HRESULT OnSourcePropertyChanged();

        // Gets the source DependencyObject
        _Check_return_ HRESULT GetSource(_Outptr_result_maybenull_ DependencyObject** ppSource);

        // Applies the converter if one is set
        _Check_return_ HRESULT ApplyConverter(
            _In_opt_ IInspectable* pValue,
            _Out_ IInspectable** ppConvertedValue);

    public:
        // Creates a new instance of DirectSourceBindingExpression.
        // pSource: The source DependencyObject to read from
        // pSourceProperty: The property on the source to bind to
        // pConverter: Optional value converter (may be null)
        static _Check_return_ HRESULT Create(
            _In_ DependencyObject* pSource,
            _In_ const CDependencyProperty* pSourceProperty,
            _In_opt_ xaml_data::IValueConverter* pConverter,
            _Out_ DirectSourceBindingExpression** ppExpression);

    // BindingExpressionBase members
        _Check_return_ HRESULT GetCanSetValue(_Out_ bool *pValue) override;
        bool GetIsAssociated() override;
        _Check_return_ HRESULT OnAttach(
            _In_ DependencyObject* pTarget,
            _In_ const CDependencyProperty* pTargetProperty) override;
        _Check_return_ HRESULT OnDetach() override;
        _Check_return_ HRESULT GetValue(
            _In_ DependencyObject* pObject,
            _In_ const CDependencyProperty* pProperty,
            _Out_ IInspectable** ppValue) override;

    // IDPChangedEventHandler
        IFACEMETHODIMP Invoke(
            _In_ xaml::IDependencyObject* pSender,
            _In_ const CDependencyProperty* pDP) override;

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID riid, _Out_ void** ppObject) override;
    };
}
