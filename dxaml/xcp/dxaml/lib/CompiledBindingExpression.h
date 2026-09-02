// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Expression used to evaluate a programmatic compiled binding: the value is
//      produced by invoking an app-supplied getter delegate against a source object,
//      rather than by walking a property path.
//
//      This is the runtime primitive behind a programmatic "compiled binding" - the
//      code-behind analog of {x:Bind}. The app supplies a delegate that maps the
//      source object to the target value (for example, a lambda that reads and
//      combines properties off a view model). The expression keeps the delegate
//      alive, re-invokes it whenever the source raises INotifyPropertyChanged, and
//      writes the result into the target dependency property.
//
//      Modeled on DirectSourceBindingExpression (which is itself modeled on
//      TemplateBindingExpression). The member layout, attach/detach lifecycle, weak
//      source / no-ref target ownership model, and reentrancy guard are all the same.
//      The two deliberate differences are:
//        1. Source change tracking uses INotifyPropertyChanged rather than a single
//           source dependency property. Because the getter is opaque, the expression
//           cannot know which source property the value depends on, so it re-evaluates
//           on EVERY notification from the source (including the "all properties
//           changed" empty/null PropertyName), matching what {x:Bind} does when it
//           binds a function against a receiver it cannot decompose.
//        2. GetValue invokes the getter delegate instead of reading a source property.
//
//      Limited to OneWay:
//        - OneTime would be a plain property set from app code, so it needs no
//          expression at all.
//        - TwoWay would require a second (setter) delegate plus an
//          UpdateSourceTrigger / reentrancy-loop story, which is more than a single
//          getter delegate can express. See BindingExpression for the full TwoWay
//          engine.

#pragma once

#include "BindingExpressionBase.g.h"
// PropertyChangedEventCallback (from EventCallbacks.h) and the projected xaml_data types are
// provided via precomp.h, which every consuming translation unit includes first - matching how
// INPCListenerBase.h declares its EventPtr<PropertyChangedEventCallback> member.

namespace DirectUI
{
    class CompiledBindingExpressionDataContextChangedHandler;

    // Lightweight binding expression for a programmatic compiled binding: an app-supplied
    // getter delegate produces the value from a source object, and the expression
    // re-evaluates whenever the source raises INotifyPropertyChanged.
    //
    // Reference ownership notes (same model as DirectSourceBindingExpression):
    // - Source: The expression holds a weak reference (IWeakReference) to the source to
    //   avoid cycles, since the source's PropertyChanged event holds a strong reference
    //   to this expression's handler.
    // - Getter: The expression holds a STRONG reference to the getter delegate. The
    //   target owns the expression (via EffectiveValueEntry), the expression owns the
    //   getter, and the getter is what keeps the app's lambda (and its captures) alive
    //   for the lifetime of the binding. This is not a cycle: the getter does not
    //   reference the target. When the target tears the expression down (OnDetach ->
    //   release), the getter - and the lambda it wraps - is released with it.
    // - Target: The expression holds a no-ref raw pointer to the target. This is safe
    //   because the target owns this expression. The expression cannot outlive the
    //   target - OnDetach is always called before the expression is released, which
    //   clears m_pTarget.
    class CompiledBindingExpression :
        public BindingExpressionBase
    {

    protected:
        CompiledBindingExpression() = default;
        ~CompiledBindingExpression() override;

        // Non-copyable, non-movable (pointers and registration state are not safe to duplicate)
        CompiledBindingExpression(const CompiledBindingExpression&) = delete;
        CompiledBindingExpression& operator=(const CompiledBindingExpression&) = delete;
        CompiledBindingExpression(CompiledBindingExpression&&) = delete;
        CompiledBindingExpression& operator=(CompiledBindingExpression&&) = delete;

    private:
        ctl::ComPtr<IWeakReference> m_spSourceRef;                       // Weak reference to source object
        ctl::ComPtr<xaml_data::ICompiledBindingGetter> m_spGetter;      // Strong ref: app getter delegate (keeps the lambda alive)
        DependencyObject* m_pTarget = nullptr;                          // The target instance (no-ref, kept valid by attach/detach lifecycle)
        const CDependencyProperty* m_pTargetProperty = nullptr;         // The target property
        ctl::EventPtr<PropertyChangedEventCallback> m_epSourceChangedHandler; // INPC subscription on the source
        ctl::ComPtr<IDataContextChangedHandler> m_spDataContextChangedHandler;
        bool m_bRegisteredForSourceChanges = false;                    // Flag indicating registration with source's PropertyChanged event
        bool m_bRegisteredForDataContextChanges = false;
        bool m_ignoreSourceChanges = false;                            // Flag to ignore source notifications while we are producing a value
        bool m_dataContextChangedDuringEvaluation = false;

        // Re-invoke the getter and push the value into the target when the source changes.
        _Check_return_ HRESULT OnSourceChanged();

        // Resolves the (weakly-held) source object; may be null if it has been collected.
        _Check_return_ HRESULT GetSource(_Outptr_result_maybenull_ IInspectable** ppSource);

        // Attaches / detaches the INotifyPropertyChanged handler on the source.
        _Check_return_ HRESULT AttachSourceChangedHandler(_In_ IInspectable* pSource);
        _Check_return_ HRESULT DetachSourceChangedHandler(_In_ IInspectable* pSource);

        _Check_return_ HRESULT SetSource(_In_opt_ IInspectable* pSource);
        _Check_return_ HRESULT OnDataContextChanged(
            _In_ DependencyObject* pSender,
            _In_ const DataContextChangedParams* pArgs);

        friend class CompiledBindingExpressionDataContextChangedHandler;

    public:
        // Creates a new instance of CompiledBindingExpression.
        // pSource: The source object the getter reads from (typically the DataContext). May be null and may implement
        //          INotifyPropertyChanged; if it does not, the binding evaluates once and never updates.
        // pGetter: The app-supplied getter delegate that maps the source to the target value. Required.
        static _Check_return_ HRESULT Create(
            _In_opt_ IInspectable* pSource,
            _In_ xaml_data::ICompiledBindingGetter* pGetter,
            _Out_ CompiledBindingExpression** ppExpression);

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
    };
}
