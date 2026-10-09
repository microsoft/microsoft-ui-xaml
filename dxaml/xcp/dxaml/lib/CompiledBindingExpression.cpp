// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Expression used to evaluate a programmatic compiled binding by invoking an
//      app-supplied getter delegate against a source object.

#include "precomp.h"
#include "FrameworkElement.g.h"
#include "CompiledBindingExpression.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

class DirectUI::CompiledBindingExpressionDataContextChangedHandler final :
    public ctl::implements<IDataContextChangedHandler>
{
public:
    explicit CompiledBindingExpressionDataContextChangedHandler(_In_ CompiledBindingExpression* pExpression)
    {
        VERIFYHR(ctl::AsWeak(pExpression, &m_spWeakRef));
    }

    _Check_return_ HRESULT Invoke(
        _In_ DependencyObject* pSender,
        _In_ const DataContextChangedParams* pArgs) override
    {
        ctl::ComPtr<xaml_data::IBindingExpressionBase> spExpressionInterface;
        IFC_RETURN(m_spWeakRef.As(&spExpressionInterface));

        auto pExpression = spExpressionInterface.Cast<CompiledBindingExpression>();
        auto pegClass = ctl::try_make_autopeg(pExpression);
        if (pegClass)
        {
            IFC_RETURN(pExpression->OnDataContextChanged(pSender, pArgs));
        }

        return S_OK;
    }

private:
    ctl::WeakRefPtr m_spWeakRef;
};

class DirectUI::CompiledBindingExpressionDPChangedHandler final :
    public ctl::implements<IDPChangedEventHandler>
{
public:
    explicit CompiledBindingExpressionDPChangedHandler(_In_ CompiledBindingExpression* pExpression)
    {
        VERIFYHR(ctl::AsWeak(pExpression, &m_spWeakRef));
    }

    IFACEMETHODIMP Invoke(
        _In_ xaml::IDependencyObject*,
        _In_ const CDependencyProperty* pDP) override
    {
        ctl::ComPtr<xaml_data::IBindingExpressionBase> spExpressionInterface;
        IFC_RETURN(m_spWeakRef.As(&spExpressionInterface));

        auto pExpression = spExpressionInterface.Cast<CompiledBindingExpression>();
        auto pegClass = ctl::try_make_autopeg(pExpression);
        if (pegClass && pDP->GetIndex() == pExpression->m_pTargetProperty->GetIndex())
        {
            IFC_RETURN(pExpression->OnTargetChanged());
        }

        return S_OK;
    }

private:
    ctl::WeakRefPtr m_spWeakRef;
};

CompiledBindingExpression::~CompiledBindingExpression()
{
    m_spSourceRef.Reset();
    m_spGetter.Reset();
    m_spSetter.Reset();
    m_spDataContextChangedHandler.Reset();
    m_spTargetChangedHandler.Reset();
    m_pTarget = nullptr;
}

// Factory method modeled after DirectSourceBindingExpression::Create.
_Check_return_ HRESULT CompiledBindingExpression::Create(
    _In_opt_ IInspectable* pSource,
    _In_ xaml_data::ICompiledBindingGetter* pGetter,
    _In_opt_ xaml_data::ICompiledBindingSetter* pSetter,
    _Out_ CompiledBindingExpression** ppExpression)
{
    IFCPTR_RETURN(pGetter);
    IFCPTR_RETURN(ppExpression);

    ctl::ComPtr<CompiledBindingExpression> spExpression;
    IFC_RETURN(ctl::ComObject<CompiledBindingExpression>::CreateInstance(spExpression.ReleaseAndGetAddressOf()));

    IFC_RETURN(spExpression->SetSource(pSource));

    // Strong reference to the getter delegate: this is what keeps the app's lambda (and its
    // captured state) alive for the lifetime of the binding.
    spExpression->m_spGetter = pGetter;
    spExpression->m_spSetter = pSetter;

    *ppExpression = spExpression.Detach();
    return S_OK;
}

_Check_return_ HRESULT CompiledBindingExpression::GetCanSetValue(_Out_ bool *pValue)
{
    *pValue = !!m_spSetter;
    return S_OK;
}

bool CompiledBindingExpression::GetIsAssociated()
{
    return !!m_pTarget;
}

// Attach lifecycle follows the same pattern as DirectSourceBindingExpression::OnAttach.
_Check_return_ HRESULT CompiledBindingExpression::OnAttach(
    _In_ DependencyObject* pTarget,
    _In_ const CDependencyProperty* pTargetProperty)
{
    IFCPTR_RETURN(pTarget);
    IFCPTR_RETURN(pTargetProperty);
    IFCEXPECT_RETURN(!m_bRegisteredForSourceChanges);

    m_pTarget = pTarget;    // No reference
    m_pTargetProperty = pTargetProperty;

    auto attachCleanupGuard = wil::scope_exit([this]
    {
        VERIFYHR(OnDetach());
    });

    auto spTargetAsFrameworkElementInterface = ctl::query_interface_cast<xaml::IFrameworkElement>(pTarget);
    auto spTargetAsFrameworkElement = spTargetAsFrameworkElementInterface.AsOrNull<FrameworkElement>();
    if (spTargetAsFrameworkElement)
    {
        ctl::ComPtr<IDataContextChangedEventSource> spEventSource;
        ctl::ComPtr<CompiledBindingExpressionDataContextChangedHandler> spHandler;
        spHandler.Attach(new CompiledBindingExpressionDataContextChangedHandler(this));
        m_spDataContextChangedHandler = spHandler;

        IFC_RETURN(spTargetAsFrameworkElement->GetDataContextChangedSource(&spEventSource));
        IFC_RETURN(spEventSource->AddHandler(m_spDataContextChangedHandler.Get()));
        m_bRegisteredForDataContextChanges = true;
    }

    // Resolve the source and, if it supports change notification, register for it.
    ctl::ComPtr<IInspectable> spSource;
    IFC_RETURN(GetSource(&spSource));

    if (spSource)
    {
        IFC_RETURN(AttachSourceChangedHandler(spSource.Get()));
    }

    attachCleanupGuard.release();
    return S_OK;
}

// Detach lifecycle follows the same pattern as DirectSourceBindingExpression::OnDetach.
_Check_return_ HRESULT CompiledBindingExpression::OnDetach()
{
    if (m_pTarget == nullptr && m_pTargetProperty == nullptr)
    {
        // Already detached
        ASSERT(!m_bRegisteredForSourceChanges);
        ASSERT(!m_bRegisteredForTargetChanges);
        return S_OK;
    }

    if (m_bRegisteredForTargetChanges)
    {
        ctl::ComPtr<IDPChangedEventSource> spEventSource;
        IFC_RETURN(m_pTarget->GetDPChangedEventSource(&spEventSource));
        IFC_RETURN(spEventSource->RemoveHandler(m_spTargetChangedHandler.Get()));
        m_bRegisteredForTargetChanges = false;
    }
    m_spTargetChangedHandler.Reset();

    if (m_bRegisteredForDataContextChanges)
    {
        auto spTargetAsFrameworkElementInterface = ctl::query_interface_cast<xaml::IFrameworkElement>(m_pTarget);
        auto spTargetAsFrameworkElement = spTargetAsFrameworkElementInterface.AsOrNull<FrameworkElement>();
        if (spTargetAsFrameworkElement)
        {
            ctl::ComPtr<IDataContextChangedEventSource> spEventSource;
            IFC_RETURN(spTargetAsFrameworkElement->GetDataContextChangedSource(&spEventSource));
            IFC_RETURN(spEventSource->RemoveHandler(m_spDataContextChangedHandler.Get()));
        }
        m_bRegisteredForDataContextChanges = false;
        m_spDataContextChangedHandler.Reset();
    }

    // Unregister from source change notifications.
    if (m_bRegisteredForSourceChanges)
    {
        ctl::ComPtr<IInspectable> spSource;
        if (SUCCEEDED(GetSource(&spSource)) && spSource)
        {
            IFC_RETURN(DetachSourceChangedHandler(spSource.Get()));
        }
        m_bRegisteredForSourceChanges = false;
    }

    m_pTarget = nullptr;
    m_pTargetProperty = nullptr;

    return S_OK;
}

_Check_return_ HRESULT CompiledBindingExpression::GetValue(
    _In_ DependencyObject* pObject,
    _In_ const CDependencyProperty* pProperty,
    _Out_ IInspectable** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFCEXPECT_RETURN(m_pTarget);
    IFCEXPECT_RETURN(m_pTargetProperty);

    m_ignoreSourceChanges = true;
    auto ignoreSourceChangesGuard = wil::scope_exit([&]
    {
        m_ignoreSourceChanges = false;
    });

    m_dataContextChangedDuringEvaluation = false;

    ctl::ComPtr<IInspectable> spSource;
    IFC_RETURN(GetSource(&spSource));

    ctl::ComPtr<IInspectable> spValue;
    if (spSource == nullptr)
    {
        IFC_RETURN(m_pTarget->GetDefaultValueInternal(
            m_pTargetProperty,
            spValue.ReleaseAndGetAddressOf()));
    }
    else
    {
        // SYNC_CALL_TO_APP DIRECT - This next line may directly call out to app code.
        IFC_RETURN(m_spGetter->Invoke(spSource.Get(), spValue.ReleaseAndGetAddressOf()));
    }

    if (m_dataContextChangedDuringEvaluation)
    {
        return E_INVALID_OPERATION;
    }

    *ppValue = spValue.Detach();
    return S_OK;
}

_Check_return_ HRESULT CompiledBindingExpression::OnSourceChanged()
{
    if (m_updatingSource)
    {
        m_sourceChangedDuringUpdate = true;
        return S_OK;
    }

    if (!m_ignoreSourceChanges)
    {
        IFC_RETURN(RefreshTarget());
    }

    return S_OK;
}

_Check_return_ HRESULT CompiledBindingExpression::RefreshTarget()
{
    IFCEXPECT_RETURN(m_pTarget);
    IFCEXPECT_RETURN(m_pTargetProperty);

    m_ignoreTargetChanges = true;
    auto ignoreTargetChangesGuard = wil::scope_exit([this]
    {
        m_ignoreTargetChanges = false;
    });

    // Re-pull the value; RefreshExpression calls back into GetValue, which re-invokes the getter.
    return m_pTarget->RefreshExpression(m_pTargetProperty);
}

_Check_return_ HRESULT CompiledBindingExpression::ConnectToTargetChanges()
{
    IFCEXPECT_RETURN(m_spSetter);
    IFCEXPECT_RETURN(m_pTarget);
    IFCEXPECT_RETURN(!m_bRegisteredForTargetChanges);

    ctl::ComPtr<IDPChangedEventSource> spEventSource;
    ctl::ComPtr<CompiledBindingExpressionDPChangedHandler> spHandler;
    spHandler.Attach(new CompiledBindingExpressionDPChangedHandler(this));

    IFC_RETURN(m_pTarget->GetDPChangedEventSource(&spEventSource));
    IFC_RETURN(spHandler.As(&m_spTargetChangedHandler));
    IFC_RETURN(spEventSource->AddHandler(m_spTargetChangedHandler.Get()));

    m_bRegisteredForTargetChanges = true;
    return S_OK;
}

_Check_return_ HRESULT CompiledBindingExpression::OnTargetChanged()
{
    if (m_ignoreTargetChanges || m_updatingSource)
    {
        return S_OK;
    }

    IFCEXPECT_RETURN(m_spSetter);

    ctl::ComPtr<IInspectable> spSource;
    IFC_RETURN(GetSource(&spSource));
    if (!spSource)
    {
        return S_OK;
    }

    ctl::ComPtr<IInspectable> spValue;
    IFC_RETURN(m_pTarget->GetValue(m_pTargetProperty, &spValue));

    m_updatingSource = true;
    m_sourceChangedDuringUpdate = false;
    auto updatingSourceGuard = wil::scope_exit([this]
    {
        m_updatingSource = false;
    });

    // SYNC_CALL_TO_APP DIRECT - This next line may directly call out to app code.
    IFC_RETURN(m_spSetter->Invoke(spSource.Get(), spValue.Get()));

    m_updatingSource = false;
    updatingSourceGuard.release();

    if (m_sourceChangedDuringUpdate)
    {
        m_sourceChangedDuringUpdate = false;
        if (m_pTarget)
        {
            IFC_RETURN(RefreshTarget());
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CompiledBindingExpression::AttachSourceChangedHandler(_In_ IInspectable* pSource)
{
    IFCEXPECT_RETURN(!m_bRegisteredForSourceChanges);

    // Only sources that raise INotifyPropertyChanged can update the binding. A source without it
    // yields a single evaluation at attach time (equivalent to OneTime), which is acceptable.
    ctl::ComPtr<xaml_data::INotifyPropertyChanged> spINPC;
    if ((spINPC = ctl::query_interface_cast<xaml_data::INotifyPropertyChanged>(pSource)))
    {
        // Unlike INPCListenerBase, the getter is opaque, so we cannot filter by PropertyName -
        // we re-evaluate on EVERY notification (including the empty/null "everything changed"
        // name). This mirrors how {x:Bind} re-runs a function binding it cannot decompose.
        IFC_RETURN(m_epSourceChangedHandler.AttachEventHandler(spINPC.Get(),
            [this](IInspectable*, xaml_data::IPropertyChangedEventArgs*)
            {
                return OnSourceChanged();
            }));
        m_bRegisteredForSourceChanges = true;
    }

    return S_OK;
}

_Check_return_ HRESULT CompiledBindingExpression::DetachSourceChangedHandler(_In_ IInspectable* pSource)
{
    if (m_epSourceChangedHandler)
    {
        IFC_RETURN(m_epSourceChangedHandler.DetachEventHandler(pSource));
    }

    return S_OK;
}

_Check_return_ HRESULT CompiledBindingExpression::GetSource(
    _Outptr_result_maybenull_ IInspectable** ppSource)
{
    IFCPTR_RETURN(ppSource);
    *ppSource = nullptr;

    if (!DXamlCore::GetCurrent()->IsShuttingDown())
    {
        if (m_spSourceRef)
        {
            IFC_RETURN(ctl::resolve_weakref(m_spSourceRef.Get(), *ppSource));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CompiledBindingExpression::SetSource(_In_opt_ IInspectable* pSource)
{
    if (m_bRegisteredForSourceChanges)
    {
        ctl::ComPtr<IInspectable> spOldSource;
        IFC_RETURN(GetSource(&spOldSource));
        if (spOldSource)
        {
            IFC_RETURN(DetachSourceChangedHandler(spOldSource.Get()));
        }
        m_bRegisteredForSourceChanges = false;
    }

    m_spSourceRef.Reset();
    if (pSource)
    {
        IWeakReference* pSourceRef = nullptr;
        IFC_RETURN(ctl::as_weakref(pSourceRef, pSource));
        m_spSourceRef.Attach(pSourceRef);

        if (m_pTarget)
        {
            IFC_RETURN(AttachSourceChangedHandler(pSource));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CompiledBindingExpression::OnDataContextChanged(
    _In_ DependencyObject* pSender,
    _In_ const DataContextChangedParams* pArgs)
{
    IFCPTR_RETURN(pSender);
    IFCPTR_RETURN(pArgs);

    ctl::ComPtr<IInspectable> spDataContext;
    auto spFrameworkElementInterface = ctl::query_interface_cast<xaml::IFrameworkElement>(pSender);
    auto spFrameworkElement = spFrameworkElementInterface.AsOrNull<FrameworkElement>();
    IFCEXPECT_RETURN(spFrameworkElement);

    if (pArgs->m_fResolvedNewDataContext && !spFrameworkElement->IsDataContextBound())
    {
        IFC_RETURN(pArgs->GetNewDataContext(&spDataContext));
    }
    else
    {
        IFC_RETURN(spFrameworkElement->get_DataContext(&spDataContext));
    }

    IFC_RETURN(SetSource(spDataContext.Get()));

    if (m_ignoreSourceChanges)
    {
        m_dataContextChangedDuringEvaluation = true;
        return S_OK;
    }

    return OnSourceChanged();
}
