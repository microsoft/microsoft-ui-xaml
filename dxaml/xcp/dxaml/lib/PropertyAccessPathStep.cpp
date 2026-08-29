// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "PropertyAccessPathStep.h"
#include "PropertyProviderPropertyAccess.h"
#include "PropertyPath.h"
#include "DependencyObjectPropertyAccess.h"
#include "PropertyInfoPropertyAccess.h"
#include "MapPropertyAccess.h"
#include <XStringUtils.h>
#include <xstrutil.h>

using namespace DirectUI;
using namespace DirectUISynonyms;
using namespace xaml_data;

PropertyAccessPathStep::~PropertyAccessPathStep()
{
    if (IsInlineDOAccessEnabled())
    {
        // Detach inline DO event handler before destruction (safe from destructor path).
        IGNOREHR(InlineDODisconnect());
    }

    delete[] m_szProperty;
}

_Check_return_
HRESULT
PropertyAccessPathStep::ReConnect(_In_ IInspectable *pSource)
{
    HRESULT hr = S_OK;

    // Disconnect from the current source
    Disconnect();

    // Null source does nothing for us
    if (pSource == NULL)
    {
        goto Cleanup;
    }

    IFC(ConnectToPropertyOnSource(pSource, m_fListenToChanges));

Cleanup:

    RRETURN(hr);
}

void PropertyAccessPathStep::DisconnectCurrentItem()
{
    if (IsInlineDOAccessEnabled())
    {
        InlineDODisconnect();
    }
    if (m_tpPropertyAccess)
    {
        VERIFYHR(m_tpPropertyAccess->DisconnectEventHandlers());
        VERIFYHR(m_tpPropertyAccess->SetSource(NULL, /* fListenToChanges */ FALSE));
    }
}

bool PropertyAccessPathStep::IsConnected()
{
    if (IsInlineDOAccessEnabled())
    {
        if (IsUsingInlineDOAccess())
        {
            return m_inlineDO.m_pProperty && m_inlineDO.m_tpSource;
        }
        return m_tpPropertyAccess && m_tpPropertyAccess->IsConnected();
    }
    else
    {
        return m_tpPropertyAccess && m_tpPropertyAccess->IsConnected();
    }
}

_Check_return_
HRESULT
PropertyAccessPathStep::GetValue(_Outptr_ IInspectable **ppValue)
{
    HRESULT hr = S_OK;

    if (!IsConnected())
    {
        *ppValue = NULL;
        goto Cleanup;
    }

    if (IsInlineDOAccessEnabled())
    {
        if (IsUsingInlineDOAccess())
        {
            // Inline DO fast path: read value directly from the DependencyObject.
            // This parallels DependencyObjectPropertyAccess::GetValue.
            ctl::ComPtr<DependencyObject> spSource;
            *ppValue = nullptr;
            IFC(InlineDOGetDependencyObject(&spSource));
            if (spSource)
            {
                if (   !m_inlineDO.m_pProperty->ShouldBindingGetValueUseCheckOnDemandProperty()
                    || !spSource->GetHandle()->CheckOnDemandProperty(m_inlineDO.m_pProperty).IsNull())
                {
                    IFC(spSource->GetValue(m_inlineDO.m_pProperty, ppValue));
                }
            }
        }
        else
        {
            IFC(m_tpPropertyAccess->GetValue(ppValue));
        }
    }
    else
    {
        IFC(m_tpPropertyAccess->GetValue(ppValue));
    }

Cleanup:

    if (FAILED(hr))
    {
        TraceGetterError();
    }

    RRETURN(hr);
}

void PropertyAccessPathStep::TraceGetterError()
{
    HRESULT hr = S_OK;
    const CClassInfo *pTypeInfo = NULL;
    const WCHAR *szTraceString = NULL;
    xstring_ptr strSourceClassName;
    IInspectable *pSource = NULL;
    wrl_wrappers::HString strErrorString;
    ctl::ComPtr<IPropertyPathListener> spListener;

    if (!DebugOutput::IsLoggingForBindingEnabled())
    {
        goto Cleanup;
    }

    if (IsInlineDOAccessEnabled())
    {
        if (IsUsingInlineDOAccess())
        {
            // For inline DO path, get source and type from our inline state.
            pSource = m_inlineDO.m_tpSource.Get();
            AddRefInterface(pSource);
            IFC(MetadataAPI::GetFriendlyRuntimeClassName(pSource, &strSourceClassName));
            pTypeInfo = m_inlineDO.m_pProperty->GetPropertyType();
        }
        else
        {
            IFC(m_tpPropertyAccess->GetSource(&pSource));
            IFC(MetadataAPI::GetFriendlyRuntimeClassName(pSource, &strSourceClassName));
            IFC(m_tpPropertyAccess->GetType(&pTypeInfo));
        }
    }
    else
    {
        IFC(m_tpPropertyAccess->GetSource(&pSource));
        IFC(MetadataAPI::GetFriendlyRuntimeClassName(pSource, &strSourceClassName));

        IFC(m_tpPropertyAccess->GetType(&pTypeInfo));
    }

    IFC(m_spListener.As(&spListener));
    IFCEXPECT_ASSERT(spListener.Get());
    IFC(spListener.Cast<PropertyPathListener>()->GetTraceString(&szTraceString));

    IFC(DXamlCore::GetCurrentNoCreate()->GetNonLocalizedErrorString(TEXT_BINDINGTRACE_GETTER_FAILED, strErrorString.GetAddressOf()));

    DebugOutput::LogBindingErrorMessage(StringCchPrintfWWrapper(
        const_cast<WCHAR*>(strErrorString.GetRawBuffer(nullptr)),
        m_szProperty,
        pTypeInfo->GetName().GetBuffer(),
        const_cast<WCHAR*>(strSourceClassName.GetBuffer()),
        szTraceString));

Cleanup:
    ReleaseInterface(pSource);
    return;
}


_Check_return_
HRESULT
PropertyAccessPathStep::SetValue(_In_ IInspectable *pValue)
{
    HRESULT hr = S_OK;

    IFCEXPECT(IsConnected());

    if (IsInlineDOAccessEnabled())
    {
        if (IsUsingInlineDOAccess())
        {
            ctl::ComPtr<DependencyObject> spSource;
            IFC(InlineDOGetDependencyObject(&spSource));
            if (spSource)
            {
                IFC(spSource->SetValue(m_inlineDO.m_pProperty, pValue));
            }
        }
        else
        {
            IFC(m_tpPropertyAccess->SetValue(pValue));
        }
    }
    else
    {
        IFC(m_tpPropertyAccess->SetValue(pValue));
    }

Cleanup:

    RRETURN(hr);
}

_Check_return_
HRESULT
PropertyAccessPathStep::ConnectToPropertyOnSource(
    _In_ IInspectable *pSource,
    _In_ bool fListenToChanges)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spInsp;
    BOOLEAN bConnected = FALSE;
    ctl::ComPtr<xaml_data::ICollectionView> spSourceAsCV;

    // If the incoming source is null or empty then
    // we're done
    if (PropertyValue::IsNullOrEmpty(pSource))
    {
        if (IsInlineDOAccessEnabled())
        {
            InlineDODisconnect();
        }

        if (m_tpPropertyAccess)
        {
            IFC(m_tpPropertyAccess->DisconnectEventHandlers());
            IFC(m_tpPropertyAccess->SetSource(NULL, /* fListenToChanges */ FALSE));
        }
        goto Cleanup;
    }

    // First try to access the property on the source itself
    IFC(ConnectPropertyAccessForObject(pSource, fListenToChanges, &bConnected));

    // If accessing the property on the object failed
    // we will check if the source is a collection view and if so
    // will get the property from the current item
    if (!bConnected)
    {
        spSourceAsCV = ctl::query_interface_cast<xaml_data::ICollectionView>(pSource);
        SetPtrValue(m_tpSourceAsCV, spSourceAsCV);
        if (!m_tpSourceAsCV)
        {
            goto Cleanup;
        }

        IFC(AddCurrentChangedEventHandler());

        // Before Phase 2 the current item is suposed to be
        // a PropertyValue
        IFC(m_tpSourceAsCV->get_CurrentItem(&spInsp));

        IFC(ConnectPropertyAccessForObject(spInsp.Get(), fListenToChanges, &bConnected));
    }

Cleanup:

    RRETURN(hr);
}

_Check_return_
HRESULT
PropertyAccessPathStep::ConnectPropertyAccessForObject(
    _In_ IInspectable *pSource,
    _In_ bool fListenToChanges,
    _Out_ BOOLEAN* pbConnected)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IMap<HSTRING, IInspectable *>> spMap;
    ctl::ComPtr<PropertyAccess> spResult;
    ctl::ComPtr<xaml_data::ICustomPropertyProvider> spPropertyProvider;
    ctl::ComPtr<IInspectable> spInsp;
    ctl::ComPtr<IInspectable> spSourceForDP;
    const CClassInfo* pSourceType = NULL;

    *pbConnected = FALSE;

    // If the incoming source is a weak IInspectable then
    // we need to extract the wrapped value from it to
    // see what kind of property access to create
    if (ctl::is<IWeakInspectable>(pSource))
    {
        // We will never wrap an IInspectable in a ValueWeakReference
        spInsp.Attach(ValueWeakReference::get_value_as<xaml::IDependencyObject>(pSource));
        spSourceForDP = pSource;     // If the source is a weak reference wrapper we want to keep that

        ASSERT(!ctl::is<wf::IPropertyValue>(spInsp));
    }
    else
    {
        // The incoming source is just an IInspectable, if it happens to be an
        // IPropertyValue we will get the IInspectable out of it
        spInsp = pSource;
        spSourceForDP = spInsp;       // The source is the object itself then
    }

    // Nothing we can create if the value is NULL
    if (!spInsp)
    {
        if (IsInlineDOAccessEnabled())
        {
            InlineDODisconnect();
        }

        if (m_tpPropertyAccess)
        {
            IFC(m_tpPropertyAccess->DisconnectEventHandlers());
            IFC(m_tpPropertyAccess->SetSource(nullptr, /* fListenToChanges */ FALSE));
        }
        goto Cleanup;
    }

    // Try reconnect fast path.
    if (IsInlineDOAccessEnabled())
    {
        // Parallel of DependencyObjectPropertyAccess::TryReconnect and
        // DependencyObjectPropertyAccess::SetSource -- if we're already using
        // inline DO access and the source type matches, just swap the source pointer.
        if (IsUsingInlineDOAccess())
        {
            IFC(MetadataAPI::GetClassInfoFromObject_SkipWinRTPropertyOtherType(spInsp.Get(), &pSourceType));
            if (m_inlineDO.m_pSourceType == pSourceType)
            {
                // Same type -- reconnect in place.
                IFC(InlineDOSafeRemovePropertyChangedHandler());
                SetPtrValue(m_inlineDO.m_tpSource, spSourceForDP.Get());
                if (fListenToChanges)
                {
                    IFC(InlineDOAddPropertyChangedHandler());
                }
                *pbConnected = TRUE;
                goto Cleanup;
            }

            // Type changed -- disconnect inline path, will fall through to create new accessor.
            InlineDODisconnect();
        }
        else if (m_tpPropertyAccess != nullptr)
        {
            IFC(m_tpPropertyAccess->TryReconnect(spInsp.Get(), !!fListenToChanges, *pbConnected, pSourceType));
            if (*pbConnected)
            {
                goto Cleanup;
            }
        }
    }
    else
    {
        if (m_tpPropertyAccess != nullptr)
        {
            IFC(m_tpPropertyAccess->TryReconnect(spInsp.Get(), !!fListenToChanges, *pbConnected, pSourceType));
            if (*pbConnected)
            {
                goto Cleanup;
            }
        }
    }

    // If this is the first time we connect, or re-connect failed and we didn't resolve a source type yet,
    // resolve it now.
    if (!pSourceType)
    {
        IFC(MetadataAPI::GetClassInfoFromObject_SkipWinRTPropertyOtherType(spInsp.Get(), &pSourceType));
    }

    // See if we can create one of the known types of property accessors
    if (IsInlineDOAccessEnabled())
    {
        if (ctl::is<xaml::IDependencyObject>(spInsp))
        {
            // Inline DO accessor: store state directly in step,
            // avoiding a heap-allocated DependencyObjectPropertyAccess.
            IFC(InlineDOConnect(spInsp.Get(), spSourceForDP.Get(), pSourceType, fListenToChanges));
            if (IsUsingInlineDOAccess())
            {
                *pbConnected = TRUE;
                goto Cleanup;
            }
            // InlineDOConnect couldn't resolve DP -- fall through to heap-allocated DOPA.
            if (m_pDP)
            {
                IFC(DependencyObjectPropertyAccess::CreateInstance(this, spSourceForDP.Get(), pSourceType, m_pDP, fListenToChanges, &spResult));
            }
            else
            {
                IFC(DependencyObjectPropertyAccess::CreateInstance(this, spSourceForDP.Get(), pSourceType, fListenToChanges, &spResult));
            }
        }
    }
    else
    {
        if (ctl::is<xaml::IDependencyObject>(spInsp))
        {
            // The dependency object property access gets the original source because it
            // can be a weak reference wrapper
            if (m_pDP)
            {
                IFC(DependencyObjectPropertyAccess::CreateInstance(this, spSourceForDP.Get(), pSourceType, m_pDP, fListenToChanges, &spResult));
            }
            else
            {
                IFC(DependencyObjectPropertyAccess::CreateInstance(this, spSourceForDP.Get(), pSourceType, fListenToChanges, &spResult));
            }
        }
    }

    // If we havent resolved the property yet and we were not asked to bind to a DP
    // keep looking
    if (!spResult && !m_pDP)
    {
        // First try to acquire a property access through application metadata
        IFC(PropertyInfoPropertyAccess::CreateInstance(this, spInsp.Get(), pSourceType, fListenToChanges, &spResult));

        // If the metadata doesn't contain information about this property then try
        // the object itself for metadata
        if (!spResult)
        {
            // The object might also be a POCO, try to resolve the property there
            if ((spMap = spInsp.AsOrNull<wfc::IMap<HSTRING, IInspectable *>>()))
            {
                IFC(MapPropertyAccess::CreateInstance(this, spMap.Get(), fListenToChanges, &spResult));
            }
            else
            {
                // Check for ICustomPropertyProvider
                spPropertyProvider = spInsp.AsOrNull<xaml_data::ICustomPropertyProvider>();
                if (!spPropertyProvider)
                {
                    // If we couldn't get an ICPP directly, and this is a CLR application, get an
                    // ICPP wrapper from there.
                    ctl::ComPtr<IInspectable> spWrapper;
                    spWrapper.Attach(ReferenceTrackerManager::GetTrackerTarget(spInsp.Get()));
                    if (spWrapper)
                    {
                        spPropertyProvider = spWrapper.AsOrNull<xaml_data::ICustomPropertyProvider>();
                    }
                }

                if (spPropertyProvider)
                {
                    IFC(PropertyProviderPropertyAccess::CreateInstance(this, spPropertyProvider.Get(), fListenToChanges, &spResult));
                }
            }
        }
    }

    // If we haven't found the property log about it
    if (!spResult)
    {
        TraceConnectionError(spInsp.Get());
    }
    else
    {
        // Remember the property connection
        SetPtrValue(m_tpPropertyAccess, spResult);
        *pbConnected = TRUE;
    }

Cleanup:

    RRETURN(hr);
}

void PropertyAccessPathStep::TraceConnectionError(_In_ IInspectable *pSource)
{
    HRESULT hr = S_OK;
    xstring_ptr strClassName;
    const WCHAR *szTraceString = NULL;
    wrl_wrappers::HString strErrorString;
    ctl::ComPtr<IPropertyPathListener> spListener;

    if (!DebugOutput::IsLoggingForBindingEnabled())
    {
        goto Cleanup;
    }

    IFC(MetadataAPI::GetFriendlyRuntimeClassName(pSource, &strClassName));

    IFC(m_spListener.As(&spListener));
    IFCEXPECT_ASSERT(spListener.Get());
    IFC(spListener.Cast<PropertyPathListener>()->GetTraceString(&szTraceString));

    IFC(DXamlCore::GetCurrentNoCreate()->GetNonLocalizedErrorString(TEXT_BINDINGTRACE_PROPERTY_CONNECTION_FAILED, strErrorString.GetAddressOf()));

    DebugOutput::LogBindingErrorMessage(StringCchPrintfWWrapper(
        const_cast<WCHAR*>(strErrorString.GetRawBuffer(nullptr)),
        GetPropertyName(),
        const_cast<WCHAR*>(strClassName.GetBuffer()),
        szTraceString));

Cleanup:
    return;
}

_Check_return_
HRESULT
PropertyAccessPathStep::CollectionViewCurrentChanged()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spInsp;
    BOOLEAN bConnected = FALSE;

    // We do not care about the property access anymore
    // we need to create a new one based on the
    // new current item
    DisconnectCurrentItem();

    IFC(m_tpSourceAsCV->get_CurrentItem(&spInsp));

    IFC(ConnectPropertyAccessForObject(spInsp.Get(), m_fListenToChanges, &bConnected));

    // Notify that the value of the source has changed
    IFC(RaiseSourceChanged());

Cleanup:

    RRETURN(hr);
}


_Check_return_
HRESULT
PropertyAccessPathStep::GetType(_Outptr_ const CClassInfo **ppType)
{
    HRESULT hr = S_OK;

    if (IsInlineDOAccessEnabled())
    {
        if (IsUsingInlineDOAccess())
        {
            IFCEXPECT(m_inlineDO.m_pProperty);
            *ppType = m_inlineDO.m_pProperty->GetPropertyType();
        }
        else
        {
            IFCEXPECT(m_tpPropertyAccess);
            IFC(m_tpPropertyAccess->GetType(ppType));
        }
    }
    else
    {
        IFCEXPECT(m_tpPropertyAccess);
        IFC(m_tpPropertyAccess->GetType(ppType));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
PropertyAccessPathStep::SourceChanged()
{
    return RaiseSourceChanged();
}

_Check_return_
HRESULT
PropertyAccessPathStep::GetSourceType(_Outptr_ const CClassInfo **ppType)
{
    HRESULT hr = S_OK;

    if (IsInlineDOAccessEnabled())
    {
        if (IsUsingInlineDOAccess())
        {
            *ppType = m_inlineDO.m_pSourceType;
        }
        else
        {
            IFCEXPECT(m_tpPropertyAccess);
            IFC(m_tpPropertyAccess->GetSourceType(ppType));
        }
    }
    else
    {
        IFCEXPECT(m_tpPropertyAccess);
        IFC(m_tpPropertyAccess->GetSourceType(ppType));
    }

Cleanup:
    return hr;
}

// ============================================================================
// Inline DependencyObject accessor helpers
//
// For the common case where the source is a DependencyObject, we store the
// accessor state (source, event handler, source type) directly in this step
// instead of allocating a separate DependencyObjectPropertyAccess object.
// ============================================================================

// Parallel of DependencyObjectPropertyAccess::CreateInstance
// resolves DP, stores state inline, optionally listens.
_Check_return_
HRESULT
PropertyAccessPathStep::InlineDOConnect(
    _In_ IInspectable *pSource,
    _In_ IInspectable *pSourceForDP,
    _In_ const CClassInfo *pSourceType,
    _In_ bool fListenToChanges)
{
    ASSERT(IsInlineDOAccessEnabled());
    const CDependencyProperty* pDP = m_pDP;

    // If we don't have a pre-resolved DP, resolve by name for the current source type.
    if (!pDP && m_szProperty)
    {
        ctl::ComPtr<DependencyObject> spSource;
        IFC_RETURN(ResolveDependencyObject(pSource, &spSource));
        if (spSource)
        {
            IFC_RETURN(MetadataAPI::TryGetDependencyPropertyByName(
                pSourceType,
                XSTRING_PTR_EPHEMERAL2(m_szProperty, xstrlen(m_szProperty)),
                &pDP));
        }
    }

    // If we couldn't resolve a DP, bail out -- caller will fall through to non-DO accessors. 
    if (!pDP)
    {
        return S_OK;
    }

    // Store state inline in the step.  The DP goes in the InlineDOAccessor
    // (not in step-level m_pDP) so it gets cleaned up by Clear() on type change.
    m_inlineDO.m_pProperty = pDP;
    m_inlineDO.m_pSourceType = pSourceType;
    SetPtrValue(m_inlineDO.m_tpSource, pSourceForDP);

    if (fListenToChanges)
    {
        IFC_RETURN(InlineDOAddPropertyChangedHandler());
    }

    return S_OK;
}

// Parallel of DependencyObjectPropertyAccess::AddPropertyChangedHandler.
_Check_return_
HRESULT
PropertyAccessPathStep::InlineDOAddPropertyChangedHandler()
{
    ASSERT(IsInlineDOAccessEnabled());
    ctl::ComPtr<DependencyObject> spSource;

    IFC_RETURN(InlineDOGetDependencyObject(&spSource));
    if (!spSource)
    {
        return S_OK;
    }

    IFC_RETURN(m_inlineDO.m_epSyncHandler.AttachEventHandler(
        spSource.Get(),
        [this](_In_ xaml::IDependencyObject *sender, _In_ const CDependencyProperty* pDP)
        {
            return this->InlineDOPropertyChanged(pDP);
        }));

    return S_OK;
}

// Parallel of DependencyObjectPropertyAccess::SafeRemovePropertyChangedHandler.
_Check_return_
HRESULT
PropertyAccessPathStep::InlineDOSafeRemovePropertyChangedHandler()
{
    ASSERT(IsInlineDOAccessEnabled());

    if (m_inlineDO.m_epSyncHandler)
    {
        ctl::ComPtr<DependencyObject> spSource;
        IFC_RETURN(InlineDOSafeGetDependencyObject(&spSource));

        if (spSource)
        {
            IFC_RETURN(m_inlineDO.m_epSyncHandler.DetachEventHandler(ctl::iinspectable_cast(spSource.Get())));
        }
    }

    return S_OK;
}

// Parallel of DependencyObjectPropertyAccess::SetSource(nullptr, FALSE) + destructor cleanup.
void
PropertyAccessPathStep::InlineDODisconnect()
{
    ASSERT(IsInlineDOAccessEnabled());
    IGNOREHR(InlineDOSafeRemovePropertyChangedHandler());
    m_inlineDO.Clear();
}

// Parallel of DependencyObjectPropertyAccess::GetSource.
_Check_return_
HRESULT
PropertyAccessPathStep::InlineDOGetDependencyObject(_Outptr_ DependencyObject **ppSource)
{
    ASSERT(IsInlineDOAccessEnabled());
    return ResolveDependencyObject(m_inlineDO.m_tpSource.Get(), ppSource);
}

// Parallel of DependencyObjectPropertyAccess::SafeGetSource.
_Check_return_
HRESULT
PropertyAccessPathStep::InlineDOSafeGetDependencyObject(_Outptr_ DependencyObject **ppSource)
{
    ASSERT(IsInlineDOAccessEnabled());
    auto spSource = m_inlineDO.m_tpSource.GetSafeReference();

    *ppSource = NULL;

    if (spSource)
    {
        IFC_RETURN(ResolveDependencyObject(spSource.Get(), ppSource));
    }

    return S_OK;
}

// Shared helper for InlineDOGetDependencyObject / InlineDOSafeGetDependencyObject.
// Parallel of the IInspectable-to-DependencyObject conversion in DOPA::GetSource.
/* static */
_Check_return_
HRESULT
PropertyAccessPathStep::ResolveDependencyObject(
    _In_ IInspectable *pSource,
    _Outptr_ DependencyObject **ppSource)
{
    ASSERT(IsInlineDOAccessEnabled());
    ctl::ComPtr<xaml::IDependencyObject> spObj;

    // ValueWeakReference::get_value_as handles both direct DOs and weak-reference wrappers.
    spObj.Attach(ValueWeakReference::get_value_as<xaml::IDependencyObject>(pSource));
    *ppSource = static_cast<DependencyObject*>(spObj.Detach());

    return S_OK;
}

// Parallel of DependencyObjectPropertyAccess::PropertyAccessPathStepDPChanged +
// SourceDPChanged -- filters by property index, then raises source changed.
_Check_return_
HRESULT
PropertyAccessPathStep::InlineDOPropertyChanged(_In_ const CDependencyProperty* pDP)
{
    ASSERT(IsInlineDOAccessEnabled());

    if (IsUsingInlineDOAccess())
    {
        // m_pProperty can be null if the handler fires after InlineDODisconnect
        // cleared our state (e.g. the detach from the source failed).
        if (m_inlineDO.m_pProperty && pDP->GetIndex() == m_inlineDO.m_pProperty->GetIndex())
        {
            IFC_RETURN(RaiseSourceChanged());
        }
    }

    return S_OK;
}
