// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Expression used to evaluate single-level bindings with an explicit source object.

#include "precomp.h"
#include "DirectSourceBindingExpression.h"

using namespace DirectUI;

DirectSourceBindingExpression::~DirectSourceBindingExpression()
{
    m_spSourceRef.Reset();
    m_pTarget = nullptr;
}

// Factory method modeled after TemplateBindingExpression::Create.
_Check_return_ HRESULT DirectSourceBindingExpression::Create(
    _In_ DependencyObject* pSource,
    _In_ const CDependencyProperty* pSourceProperty,
    _In_opt_ xaml_data::IValueConverter* pConverter,
    _Out_ DirectSourceBindingExpression** ppExpression)
{
    IFCPTR_RETURN(pSource);
    IFCPTR_RETURN(pSourceProperty);
    IFCPTR_RETURN(ppExpression);

    ctl::ComPtr<DirectSourceBindingExpression> spExpression;
    IFC_RETURN(ctl::ComObject<DirectSourceBindingExpression>::CreateInstance(spExpression.ReleaseAndGetAddressOf()));

    // Get a weak reference to the source (prevents reference cycles, same as TemplateBindingExpression)
    IWeakReference* pSourceRef = nullptr;
    IFC_RETURN(ctl::as_weakref(pSourceRef, ctl::as_iinspectable(pSource)));
    spExpression->m_spSourceRef.Attach(pSourceRef);
    spExpression->m_pSourceProperty = pSourceProperty;
    spExpression->m_spConverter = pConverter;

    *ppExpression = spExpression.Detach();
    return S_OK;
}

_Check_return_ HRESULT DirectSourceBindingExpression::GetCanSetValue(_Out_ bool *pValue)
{
    // Direct source bindings are always removed if a new value is set
    *pValue = false;
    return S_OK;
}

bool DirectSourceBindingExpression::GetIsAssociated()
{
    return !!m_pTarget;
}

// Attach lifecycle follows the same pattern as TemplateBindingExpression::OnAttach.
_Check_return_ HRESULT DirectSourceBindingExpression::OnAttach(
    _In_ DependencyObject* pTarget,
    _In_ const CDependencyProperty* pTargetProperty)
{
    IFCPTR_RETURN(pTarget);
    IFCPTR_RETURN(pTargetProperty);
    IFCEXPECT_RETURN(!m_bRegisteredForSourceChanges);

    m_pTarget = pTarget;    // No reference
    m_pTargetProperty = pTargetProperty;

    // Get the source object and register for property changes (same as TemplateBindingExpression)
    ctl::ComPtr<DependencyObject> spSource;
    IFC_RETURN(GetSource(&spSource));

    if (spSource)
    {
        ctl::ComPtr<IDPChangedEventSource> spEventSource;
        IFC_RETURN(spSource->GetDPChangedEventSource(&spEventSource));
        IFC_RETURN(spEventSource->AddHandler(this));
        m_bRegisteredForSourceChanges = true;
    }

    return S_OK;
}

// Detach lifecycle follows the same pattern as TemplateBindingExpression::OnDetach.
_Check_return_ HRESULT DirectSourceBindingExpression::OnDetach()
{
    if (m_pTarget == nullptr && m_pTargetProperty == nullptr)
    {
        // Already detached
        ASSERT(!m_bRegisteredForSourceChanges);
        return S_OK;
    }

    m_pTarget = nullptr;
    m_pTargetProperty = nullptr;

    // Unregister from source property changes
    if (m_bRegisteredForSourceChanges)
    {
        ctl::ComPtr<DependencyObject> spSource;
        if (SUCCEEDED(GetSource(&spSource)) && spSource)
        {
            ctl::ComPtr<IDPChangedEventSource> spEventSource;
            IFC_RETURN(spSource->TryGetDPChangedEventSource(&spEventSource));
            if (spEventSource)
            {
                IFC_RETURN(spEventSource->RemoveHandler(this));
            }
        }
        m_bRegisteredForSourceChanges = false;
    }

    return S_OK;
}

_Check_return_ HRESULT DirectSourceBindingExpression::GetValue(
    _In_ DependencyObject* pObject,
    _In_ const CDependencyProperty* pProperty,
    _Out_ IInspectable** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFCEXPECT_RETURN(m_pTarget);
    IFCEXPECT_RETURN(m_pTargetProperty);

    m_ignoreSourcePropertyChanges = true;
    auto ignoreSourcePropertyChangesGuard = wil::scope_exit([&]
    {
        m_ignoreSourcePropertyChanges = false;
    });

    // Get the source object
    ctl::ComPtr<DependencyObject> spSource;
    IFC_RETURN(GetSource(&spSource));

    if (spSource == nullptr)
    {
        // Source is gone, return the default value
        IFC_RETURN(m_pTarget->GetDefaultValueInternal(m_pTargetProperty, ppValue));
        return S_OK;
    }

    // Get the value from the source property
    ctl::ComPtr<IInspectable> spValue;
    IFC_RETURN(spSource->GetValue(m_pSourceProperty, &spValue));

    // Apply converter if one is set
    ctl::ComPtr<IInspectable> spConvertedValue;
    IFC_RETURN(ApplyConverter(spValue.Get(), &spConvertedValue));

    *ppValue = spConvertedValue.Detach();
    return S_OK;
}

_Check_return_ HRESULT DirectSourceBindingExpression::ApplyConverter(
    _In_opt_ IInspectable* pValue,
    _Out_ IInspectable** ppConvertedValue)
{
    IFCPTR_RETURN(ppConvertedValue);

    if (m_spConverter == nullptr)
    {
        // No converter, return value as-is
        *ppConvertedValue = pValue;
        if (pValue)
        {
            pValue->AddRef();
        }
        return S_OK;
    }

    // Apply the converter
    wxaml_interop::TypeName targetType = {};
    targetType.Kind = wxaml_interop::TypeKind_Metadata;

    IFC_RETURN(m_spConverter->Convert(
        pValue,
        targetType,
        nullptr, // parameter
        wrl_wrappers::HStringReference(L"").Get(), // language
        ppConvertedValue));

    return S_OK;
}

_Check_return_ HRESULT DirectSourceBindingExpression::OnSourcePropertyChanged()
{
    if (!m_ignoreSourcePropertyChanges)
    {
        IFCEXPECT_RETURN(m_pTarget);
        IFCEXPECT_RETURN(m_pTargetProperty);

        IFC_RETURN(m_pTarget->RefreshExpression(m_pTargetProperty));
    }

    return S_OK;
}

IFACEMETHODIMP DirectSourceBindingExpression::Invoke(
    _In_ xaml::IDependencyObject* pSender,
    _In_ const CDependencyProperty* pDP)
{
    // Only respond to changes of our source property
    if (pDP == m_pSourceProperty)
    {
        IFC_RETURN(OnSourcePropertyChanged());
    }

    return S_OK;
}

_Check_return_ HRESULT DirectSourceBindingExpression::GetSource(
    _Outptr_result_maybenull_ DependencyObject** ppSource)
{
    IFCPTR_RETURN(ppSource);
    *ppSource = nullptr;

    if (!DXamlCore::GetCurrent()->IsShuttingDown())
    {
        IFCEXPECT_RETURN(m_spSourceRef);
        IInspectable* pSourceAsI = nullptr;
        IFC_RETURN(ctl::resolve_weakref(m_spSourceRef.Get(), pSourceAsI));
        if (pSourceAsI)
        {
            *ppSource = ctl::query_interface<DependencyObject>(pSourceAsI);
            ReleaseInterface(pSourceAsI);
        }
    }

    return S_OK;
}

HRESULT DirectSourceBindingExpression::QueryInterfaceImpl(_In_ REFIID riid, _Out_ void** ppObject)
{
    if (InlineIsEqualGUID(riid, __uuidof(IDPChangedEventHandler)))
    {
        *ppObject = static_cast<IDPChangedEventHandler*>(this);
    }
    else
    {
        return BindingExpressionBase::QueryInterfaceImpl(riid, ppObject);
    }

    AddRefOuter();
    return S_OK;
}
