// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Expression used to evaluate template bindings.

#pragma once

#include "BindingExpressionBase.g.h"

namespace DirectUI
{
    class Control;
    class TemplateBindingExpressionCustomPropertyChangedHandler;

    // Expression used to evaluate template bindings.  The core will create
    // TemplateBindingExpressions by calling the SetTemplateBinding callbacks
    // that are also defined below on TemplateBindingExpression.
    class TemplateBindingExpression :
        public BindingExpressionBase
    {
        // Grant friend access to the dependency property changed handlers so
        // they can refresh the value of the experssion when the source
        // property is changed.
        friend class TemplateBindingExpressionCustomPropertyChangedHandler;

    protected:
        TemplateBindingExpression() = default;

        // Destroys an instance of the TemplateBindingExpression class.
        ~TemplateBindingExpression() override;

    private:
        IWeakReference* m_pSource = nullptr;                  // The source instance
        const CDependencyProperty* m_pSourceProperty = nullptr;  // The source property
        DependencyObject* m_pTarget = nullptr;                // The target instance (this is a weak reference)
        const CDependencyProperty* m_pTargetProperty = nullptr;  // The target property
        TemplateBindingExpressionCustomPropertyChangedHandler* m_pCustomHandler = nullptr; // Custom DependencyProperty changed handler
        bool m_bRequiresRuntimeTypeCheck = false;          // Flag indicating whether a runtime type check is required when getting the value from the target
        // Flag indicating whether we should ignore source updates.
        // If the effective source property is an on-demand property, then retrieving its value
        // can cause it to be created, which will then trigger a property changed notification
        // (which it probably shouldn't do, but also probably a decade too late to fix that) and
        // *that* will cause badness if it's happening during initial connection of the binding
        // due to the subsequent attempt to refresh the expression while the corresponding EffectiveValueEntry
        // in the value table is only partially initialized.
        bool m_ignoreSourcePropertyChanges = false;

        // Refresh the expression when the source property has been changed.
        _Check_return_ HRESULT OnSourcePropertyChanged();

        // Determine whether a value is compatible with a given source type.
        static _Check_return_ HRESULT IsValidValueForUpdate(
            _In_ IInspectable* pValue,
            _In_ const CClassInfo* pTypeInfo,
            _Out_ bool* pbIsValid);

        // Determine whether two property types are compatible and whether they
        // are potentially compatible but require a runtime type check.
        static _Check_return_ HRESULT ArePropertyTypesCompatible(
            _In_ const CClassInfo* pSourceType,
            _In_ const CClassInfo* pTargetType,
            _Out_ bool* pbTypesAreCompatible,
            _Out_ bool* pbRequiresRuntimeTypeCheck);

        // Create and set a template binding from the source property of the
        // source Control to the target property of the target DependencyObject.
        // This method is called by the four callbacks used to set template
        // bindings.
        static _Check_return_ HRESULT SetTemplateBinding(
            _In_ Control* pSource,
            _In_ const CDependencyProperty* pSourceProperty,
            _In_ DependencyObject* pTarget,
            _In_ const CDependencyProperty* pTargetProperty);

        // Gets or sets the Source weak reference
        _Check_return_ HRESULT get_Source(
            _Outptr_ Control** ppSource);
        _Check_return_ HRESULT put_Source(
            _In_ Control* pSource);

    public:
        // Initializes a new instance of the TemplateBindingExpression class.
        static _Check_return_ HRESULT Create(
            _In_ Control* pSource,
            _In_ const CDependencyProperty* pSourceProperty,
            _In_ bool bRequiresRuntimeTypeCheck,
            _Out_ TemplateBindingExpression** ppExpression);

    // BindingExpressionBase members
        // Gets a value indicating whether the value can be set.
        // TemplateBindings are always removed if a new value is set, so this
        // always returns FALSE.
        _Check_return_ HRESULT GetCanSetValue(_Out_ bool *pValue) override;

        // Gets a value indicating whether the expression has been associated
        // with a target.
        bool GetIsAssociated() override;

        // Attach the expression to a specific property on the target object.
        _Check_return_ HRESULT OnAttach(
            _In_ DependencyObject* pTarget,
            _In_ const CDependencyProperty* pTargetProperty) override;

        // Detach the expression from the target object.
        _Check_return_ HRESULT OnDetach() override;

        // Get the value of the target object's target property.
        _Check_return_ HRESULT GetValue(
            _In_ DependencyObject* pObject,
            _In_ const CDependencyProperty* pProperty,
            _Out_ IInspectable** ppValue) override;

    // Callbacks
        // Set a template binding between two properties on two objects.  This
        // is a callback from the core.
        static _Check_return_ HRESULT SetTemplateBinding(
            _In_ CDependencyObject* source,
            _In_ const CDependencyProperty* sourceProperty,
            _In_ CDependencyObject* target,
            _In_ const CDependencyProperty* targetProperty);
    };

    // Event handler for custom DependencyProperty change events that need to
    // refresh the TemplateBindingExpression when the source property changes.
    class TemplateBindingExpressionCustomPropertyChangedHandler
        : public ctl::implements<IDPChangedEventHandler>
    {
    private:

        ~TemplateBindingExpressionCustomPropertyChangedHandler() override
        {
            ReleaseInterface(m_pExpressionRef);
        }
        
        // The TemplateBindingExpression to update when the property is changed
        IWeakReference * m_pExpressionRef;

    public:
        // Initializes a new instance of the
        // TemplateBindingExpressionCustomPropertyChangedHandler class.
        TemplateBindingExpressionCustomPropertyChangedHandler()
            : m_pExpressionRef(NULL)
        {
        }

        _Check_return_ HRESULT Initialize( _In_ TemplateBindingExpression *pExpression)
        {
            return pExpression->GetWeakReference( &m_pExpressionRef );
        }

        // Handle the custom DependencyProperty changed event for the source
        // property and refresh the TemplateBindingExpression.
        IFACEMETHOD(Invoke)(
            _In_ xaml::IDependencyObject* pSender,
            _In_ const CDependencyProperty* pDP) override;
    };
}
