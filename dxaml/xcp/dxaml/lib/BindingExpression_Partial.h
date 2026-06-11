// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "InternalDebugInterop.h"

#include "BindingExpression.g.h"
#include "Binding.g.h"
#include "CollectionViewSource.g.h"
#include "Microsoft.UI.Xaml.private.h"

namespace DirectUI
{
    class PropertyPathListener;

    class BindingOperations
        : public xaml_data::IBindingOperationsStatics,
          public ctl::AbstractActivationFactory
    {
        BEGIN_INTERFACE_MAP(BindingOperations, ctl::AbstractActivationFactory)
            INTERFACE_ENTRY(BindingOperations, xaml_data::IBindingOperationsStatics)
        END_INTERFACE_MAP(BindingOperations, ctl::AbstractActivationFactory)

    protected:

        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override
        {
            if (InlineIsEqualGUID(iid, __uuidof(xaml_data::IBindingOperationsStatics)))
            {
                *ppObject = static_cast<xaml_data::IBindingOperationsStatics *>(this);
            }
            else
            {
                return ctl::AbstractActivationFactory::QueryInterfaceImpl(iid, ppObject);
            }

            AddRefOuter();
            RRETURN(S_OK);
        }

    public:

        IFACEMETHOD(SetBinding)(
            _In_ xaml::IDependencyObject* pTarget,
            _In_ xaml::IDependencyProperty* pProperty,
            _In_ xaml_data::IBindingBase* pBinding);

        _Check_return_ HRESULT SetBinding(
            _In_ xaml::IDependencyObject* pTarget,
            _In_ const CDependencyProperty* pProperty,
            _In_ xaml_data::IBindingBase* pBinding);

        static _Check_return_ HRESULT SetBindingImpl(
            _In_ xaml::IDependencyObject* pTarget,
            _In_ xaml::IDependencyProperty* pProperty,
            _In_ xaml_data::IBindingBase* pBinding);

    };

    class BindingExpressionDPChangedHandler;
    class BindingExpressionLostFocusHandler;
    class BindingExpressionDataContextChangedHandler;
    class BindingExpressionInheritanceContextChangedHandler;
    class BindingExpressionTargetLoadedHandler;
    class BindingExpressionCVSViewChangedHandler;
    interface IValueConverterInternal;

    PARTIAL_CLASS(BindingExpression),
        public IPropertyPathListenerHost
        , public DebugTool::IDebugBindingExpression
    {
    protected:
        BindingExpression();
        ~BindingExpression() override;

        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override
        {
            if (InlineIsEqualGUID(iid, __uuidof(IDebugBindingExpression)))
            {
                *ppObject = static_cast<IDebugBindingExpression *>(this);
            }
            else
            {
                return __super::QueryInterfaceImpl(iid, ppObject);
            }

            AddRefOuter();
            RRETURN(S_OK);
        }

    private:

        static _Check_return_ HRESULT CreateObject(_In_ Binding *pBinding, _Out_ BindingExpression **ppExpr);

    public:

        // Methods accessible only internally 
        _Check_return_ HRESULT GetValue(_In_ DependencyObject* pObject, _In_ const CDependencyProperty* pdp, _Out_ IInspectable** ppValue) override;

        _Check_return_ HRESULT OnAttach(_In_ DependencyObject *pTarget, _In_ const CDependencyProperty* pDP) override;
        _Check_return_ HRESULT OnDetach() override;

        _Check_return_ HRESULT GetCanSetValue(_Out_ bool *pValue) override;

        bool GetIsAssociated() override;

        // IBindingExpression interface
        // Override the property getters to get them from the internal value store instead of the static dependency properties
        _Check_return_ HRESULT get_DataItemImpl(_Outptr_ IInspectable **ppReturnValue);
        _Check_return_ HRESULT get_ParentBindingImpl(_Outptr_ xaml_data::IBinding **ppReturnValue);
        _Check_return_ HRESULT UpdateSourceImpl();

        // Tracing
        _Check_return_ HRESULT GetTraceString(_Outptr_result_z_ const WCHAR **pszTraceString) override;
        
        // IDebugBindingExpression
        _Check_return_ HRESULT GetBindingString(_Out_ HSTRING *phDebugString) override;
        bool IsBindingValid() override;
        PropertyPathStep* GetFirstPropertyPathStep() override;
        xaml::IDependencyObject* GetBindingAsDO() override { return m_tpBinding.Get(); }

        _Check_return_ HRESULT GetPath(_Out_ HSTRING *phPath) override;
        _Check_return_ HRESULT GetBindingMode(_Out_ xaml_data::BindingMode* bindingMode) override;
        _Check_return_ HRESULT GetSource(_Outptr_opt_ IInspectable **ppSource) override;
        _Check_return_ HRESULT GetElementName(_Out_ HSTRING *phElementName) override;
        _Check_return_ HRESULT GetConverter(_Outptr_opt_ void **ppConverter) override;
        _Check_return_ HRESULT GetConverterParameter(_Outptr_opt_ IInspectable **ppConverterParameter) override;
        _Check_return_ HRESULT GetConverterLanguage(_Out_ HSTRING *phConverterLanguage) override;
        _Check_return_ HRESULT GetFallbackValue(_Outptr_opt_ IInspectable **ppFallbackValue) override;
        _Check_return_ HRESULT GetTargetNullValue(_Outptr_opt_ IInspectable **ppTargetNullValue) override;
        void NotifyThemeChanged(
            _In_ Theming::Theme theme, 
            _In_ bool forceRefresh,
            _Out_ bool& valueChanged) override;

    private:

        // Methods called from event handlers to notify of changes
        // to the source in various ways
        _Check_return_ HRESULT BindingSourceChanged();

        _Check_return_ HRESULT SourceChanged() override;
        _Check_return_ HRESULT TargetChanged();

        _Check_return_ HRESULT TargetLostFocus();

        _Check_return_ HRESULT DataContextChanged(_In_ const DataContextChangedParams* pArgs);

        _Check_return_ HRESULT InheritanceContextChanged();

        _Check_return_ HRESULT TargetLoaded();

        _Check_return_ HRESULT CVSViewChanged();

    private:

        // Utility methods used to move the data around when an event happens
        _Check_return_ HRESULT RefreshBindingValue();

        _Check_return_ HRESULT UpdateTarget();
        _Check_return_ HRESULT UpdateSourceInternal();

        void TraceUpdateBindingStart(_In_ bool updateTarget);
        void TraceUpdateBindingEnd(_In_ bool updateTarget);
    private:

        // Methods used to control the lifetime of the target reference

        // These methods are used to peg and unpeg the target object
        // only entry points to the binding expression, public methods and event handlers
        // will ever call these methods
        _Check_return_ HRESULT PegTargetReference();
        bool TryPegTargetReference();
        void UnpegTargetReference();

        // The rest of the methods will use this accessor to get to the target pointer
        // at this point 
        DependencyObject *GetTargetNoRef();

    private:

        void BeginSetBinding()
        {
            m_state = BEState::SettingBinding;
        }

        void EndSetBinding()
        {
            m_state = BEState::Normal;
        }

        // TwoWay binding to the target
        _Check_return_ HRESULT ConnectToTarget();
        _Check_return_ HRESULT DisconnectFromTarget();

        _Check_return_ HRESULT StopListeningForInheritanceContextChanges();

        // Effective source management
        _Check_return_ HRESULT ConnectToEffectiveSource();
        _Check_return_ HRESULT DisconnectFromEffectiveSource();

        // Calcuates the effective source
        _Check_return_ HRESULT CalculateEffectiveSource();

        // Master methods to attach/detach from the effective source   
        _Check_return_ HRESULT AttachToEffectiveSource();
        _Check_return_ HRESULT DetachFromEffectiveSource();

        // Specific methods to attach/detach from the particular effective source

        // Used when the effective source is the DataContext
        _Check_return_ HRESULT AttachToDataContext();
        _Check_return_ HRESULT DetachFromDataContext();

        // Used when the effective source is the target itself
        _Check_return_ HRESULT AttachToTarget();

        // Used when the effective source is an ElementName
        _Check_return_ HRESULT AttachToElementName();
        _Check_return_ HRESULT GetSourceElement(_In_ FrameworkElement *pMentor, _Outptr_ IInspectable **ppSource);
        _Check_return_ HRESULT DetachFromElementName();

        // Used when the effective source is the templated parent
        _Check_return_ HRESULT AttachToTemplatedParent();
        _Check_return_ HRESULT DetachFromTemplatedParent();

        // Metor methods
        _Check_return_ HRESULT AttachToMentor();
        _Check_return_ HRESULT AttachToMentor(_In_ FrameworkElement *pMentor);
        _Check_return_ HRESULT DetachFromMentor();

        // CollectionViewSource support methods
        _Check_return_ HRESULT AttachToCollectionViewSource();
        _Check_return_ HRESULT DetachFromCollectionViewSource();

        // Utility methods
        _Check_return_ HRESULT GetDataContext(
            _Outptr_ IInspectable **ppValue);

        _Check_return_ HRESULT GetDataContext(
            _In_ FrameworkElement *pDCSource,
            _Outptr_ IInspectable **ppValue);

        _Check_return_ HRESULT GetConvertedFallbackOrDefaultValue(
            _Outptr_ IInspectable **ppValue);

        _Check_return_ HRESULT GetConvertedTargetNullOrDefaultValue(
            _Outptr_ IInspectable **ppValue);

        // Value converters
        _Check_return_ HRESULT ConvertValue(
            _In_ IInspectable *pInValue,
            _Outptr_ IInspectable **ppConvertedValue);

        _Check_return_ HRESULT ConvertToTarget(
            _In_ IInspectable *pInValue,
            _In_ const bool useTargetNullValue,
            _Outptr_ IInspectable **ppConvertedValue);

        _Check_return_ HRESULT EnsureValueConverter();

        static _Check_return_ HRESULT IsValidValueForUpdate(
            _In_ IInspectable* pInValue,
            _In_ const CClassInfo* pSourceType,
            _Out_ bool* pfIsValidForUpdate);

        void TraceSetterFailed();
        void TraceConvertFailed(_In_ IInspectable *pValue, _In_ const CClassInfo* pTargetType);

        void SetBinding(_In_ Binding* const pValue)
        {
            SetPtrValue(m_tpBinding, pValue);
        }

    private:

        const CDependencyProperty* m_pDP = nullptr;

        TrackerPtr<Binding> m_tpBinding;
        TrackerPtr<PropertyPathListener> m_tpListener;
        TrackerPtr<IInspectable> m_tpEffectiveSource;

        bool m_fListeningToLostFocus = false;
        TrackerPtr<BindingExpressionDPChangedHandler> m_tpSyncHandler;
        TrackerPtr<BindingExpressionLostFocusHandler> m_tpLostFocusHandler;
        TrackerPtr<BindingExpressionDataContextChangedHandler> m_tpDataContextChangedChangedHandler;
        TrackerPtr<BindingExpressionInheritanceContextChangedHandler> m_tpInheritanceContextChangedHandler;
        TrackerPtr<BindingExpressionTargetLoadedHandler> m_tpTargetLoadedHandler;
        TrackerPtr<BindingExpressionCVSViewChangedHandler> m_tpCVSViewChangedHandler;
        EventRegistrationToken             m_lostFocusHandlerToken{};
        ctl::WeakRefPtr m_spMentorRef;
        TrackerPtr<IValueConverterInternal> m_tpValueConverter;    // This is the internal value converter
        TrackerPtr<CollectionViewSource> m_tpCVS;

        WCHAR *m_szTraceString = nullptr;

        // Lifetime of the target weak references
        ctl::WeakRefPtr m_spTargetRef;    // Weak reference
        ctl::ComPtr<DependencyObject> m_spPeggedTarget; // Pegged reference, COM reference by design
        INT32 m_iTargetPegCount = 0;

        // Flag indicating whether we should ignore source updates.
        // If the effective source property is an on-demand property, then retrieving its value
        // can cause it to be created, which will then trigger a property changed notification
        // (which it probably shouldn't do, but also probably a decade too late to fix that) and
        // *that* will cause badness if it's happening during initial connection of the binding
        // due to the subsequent attempt to refresh the expression while the corresponding EffectiveValueEntry
        // in the value table is only partially initialized.
        bool m_ignoreSourcePropertyChanges = false;

        // The updating state of the expression
        enum class BEState
        {
            Normal,
            SettingSource,
            SettingTarget,
            SettingBinding,
        };

        enum class TargetPropertyState
        {
            Clean ,
            Dirty,
        };

        // Where did the evaluated value of the BindingExpression come from.
        // We really only care whether it came from FallbackValue or TargetNullValue;
        // all other sources (e.g. Path or target property's default value) just fall
        // into the 'Other' bucket
        // 'Unknown' indicates that the binding isn't providing the value at all
        enum class BindingValueSource
        {
            Unknown,
            FallbackValue,
            TargetNullValue,
            Other,
        };

        BEState m_state = BEState::Normal;
        TargetPropertyState m_targetPropertyState = TargetPropertyState::Clean;
        xaml_data::EffectiveSourceType m_effectiveSourceType = xaml_data::EffectiveSourceType_None;
        BindingValueSource m_bindingValueSource = BindingValueSource::Unknown;

        friend class PropertyPathListener;
        friend class BindingExpressionDPChangedHandler;
        friend class BindingExpressionLostFocusHandler;
        friend class BindingExpressionDataContextChangedHandler;
        friend class BindingExpressionInheritanceContextChangedHandler;
        friend class BindingExpressionTargetLoadedHandler;
        friend class BindingExpressionCVSViewChangedHandler;
        friend class DependencyObject;
    };

    class BindingExpressionDPChangedHandler: public ctl::implements<IDPChangedEventHandler>
    {
    public:

        BindingExpressionDPChangedHandler(_In_ BindingExpression *pExpr) : m_pExpression(pExpr)
        {}

    public:

        IFACEMETHODIMP Invoke(
            _In_ xaml::IDependencyObject* pSender, 
            _In_ const CDependencyProperty* pDP) override
        {
            HRESULT hr = S_OK;

            IFCEXPECT(m_pExpression);

            if (pDP->GetIndex() == m_pExpression->m_pDP->GetIndex())
            {
                IFC(m_pExpression->TargetChanged());
            }

        Cleanup:
            RRETURN(hr);
        }

    private:
        BindingExpression *m_pExpression;    // NOTE: This is a weak reference back to the property path step
    };

    class BindingExpressionLostFocusHandler: public ctl::implements<xaml::IRoutedEventHandler>
    {
    public:

        BindingExpressionLostFocusHandler(_In_ BindingExpression *pExpr)
        {
            VERIFYHR(ctl::AsWeak(pExpr, &m_spWeakRef));
        }

    public:

        STDMETHODIMP Invoke(_In_ IInspectable* pSender, _In_ xaml::IRoutedEventArgs* pArgs) override
        {
            HRESULT hr = S_OK;
            ctl::ComPtr<xaml_data::IBindingExpression> spBindingExpressionInterface;
            ctl::ComPtr<BindingExpression> spBindingExpression;

            IFC(m_spWeakRef.As(&spBindingExpressionInterface));
            spBindingExpression = spBindingExpressionInterface.Cast<BindingExpression>();

            {
                auto pegClass = ctl::try_make_autopeg(spBindingExpression.Get());

                if (pegClass)
                {
                    IFC(spBindingExpression->TargetLostFocus());
                }
            }

        Cleanup:
            RRETURN(hr);
        }

    private:
        // The weak reference with the binding expression.
        ctl::WeakRefPtr m_spWeakRef;
    };

    class BindingExpressionDataContextChangedHandler: public ctl::implements<IDataContextChangedHandler>
    {
    public:
        BindingExpressionDataContextChangedHandler(_In_ BindingExpression *pExpr)
        {
            VERIFYHR(ctl::AsWeak(pExpr, &m_spWeakRef));
        }

        _Check_return_ HRESULT Invoke(_In_ DependencyObject *pSender, _In_ const DataContextChangedParams* pArgs) override
        {
            HRESULT hr = S_OK;
            ctl::ComPtr<xaml_data::IBindingExpression> spBindingExpressionInterface;
            ctl::ComPtr<BindingExpression> spBindingExpression;

            IFC(m_spWeakRef.As(&spBindingExpressionInterface));
            spBindingExpression = spBindingExpressionInterface.Cast<BindingExpression>();

            {
                auto pegClass = ctl::try_make_autopeg(spBindingExpression.Get());

                if (pegClass)
                {
                    IFC(spBindingExpression->DataContextChanged(pArgs));
                }
            }

        Cleanup:
            RRETURN(hr);
        }

    private:

        // The weak reference with the binding expression.
        ctl::WeakRefPtr m_spWeakRef;
    };

    class BindingExpressionInheritanceContextChangedHandler: public ctl::implements<IInheritanceContextChangedEventHandler>
    {
    public:
        BindingExpressionInheritanceContextChangedHandler(_In_ BindingExpression *pExpr)
        {
            VERIFYHR(ctl::AsWeak(pExpr, &m_spWeakRef));
        }

    public:

        _Check_return_ HRESULT Invoke(_In_ DependencyObject *pSender, _In_opt_ IInspectable *pArgs) override
        {
            HRESULT hr = S_OK;
            ctl::ComPtr<xaml_data::IBindingExpression> spBindingExpressionInterface;
            ctl::ComPtr<BindingExpression> spBindingExpression;

            IFC(m_spWeakRef.As(&spBindingExpressionInterface));
            spBindingExpression = spBindingExpressionInterface.Cast<BindingExpression>();

            {
                auto pegClass = ctl::try_make_autopeg(spBindingExpression.Get());

                if (pegClass)
                {
                    IFC(spBindingExpression->InheritanceContextChanged());
                }
            }

        Cleanup:
            RRETURN(hr);
        }

    private:

        // The weak reference with the binding expression.
        ctl::WeakRefPtr m_spWeakRef;
    };

    class BindingExpressionTargetLoadedHandler: public ctl::implements<xaml::IRoutedEventHandler>
    {
    public:
        BindingExpressionTargetLoadedHandler(_In_ BindingExpression *pExpr)
        {
            VERIFYHR(ctl::AsWeak(pExpr, &m_spWeakRef));
        }

    public:

        IFACEMETHODIMP Invoke(_In_ IInspectable *pSender, _In_ xaml::IRoutedEventArgs *pArgs) override
        {
            HRESULT hr = S_OK;
            ctl::ComPtr<xaml_data::IBindingExpression> spBindingExpressionInterface;
            ctl::ComPtr<BindingExpression> spBindingExpression;

            IFC(m_spWeakRef.As(&spBindingExpressionInterface));
            spBindingExpression = spBindingExpressionInterface.Cast<BindingExpression>();

            {
                auto pegClass = ctl::try_make_autopeg(spBindingExpression.Get());

                if (pegClass)
                {
                    IFC(spBindingExpression->TargetLoaded());
                }
            }

        Cleanup:
            RRETURN(hr);
        }

    private:

        // The weak reference with the binding expression.
        ctl::WeakRefPtr m_spWeakRef;
    };

    class BindingExpressionCVSViewChangedHandler: public ctl::implements<ICVSViewChangedHandler>
    {
    public:
        BindingExpressionCVSViewChangedHandler(_In_ BindingExpression *pExpr)
        {
            VERIFYHR(ctl::AsWeak(pExpr, &m_spWeakRef));
        }

    public:

        _Check_return_ HRESULT Invoke(_In_ IInspectable *pSender, _In_ IInspectable *pArgs) override
        {
            HRESULT hr = S_OK;
            ctl::ComPtr<xaml_data::IBindingExpression> spBindingExpressionInterface;
            ctl::ComPtr<BindingExpression> spBindingExpression;

            IFC(m_spWeakRef.As(&spBindingExpressionInterface));
            spBindingExpression = spBindingExpressionInterface.Cast<BindingExpression>();

            {
                auto pegClass = ctl::try_make_autopeg(spBindingExpression.Get());

                if (pegClass)
                {
                    IFC(spBindingExpression->CVSViewChanged());
                }
            }

        Cleanup:
            RRETURN(hr);
        }

    private:

        // The weak reference with the binding expression.
        ctl::WeakRefPtr m_spWeakRef;
    };
}
