// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "StringIndexerPathStep.h"
#include "MapPropertyAccess.h"
#include "IndexerPropertyAccess.h"
#include "PropertyPath.h"
#include <XStringUtils.h>

#pragma warning(disable:4267) //'var' : conversion from 'size_t' to 'type', possible loss of data

using namespace DirectUI;
using namespace DirectUISynonyms;
using namespace xaml_data;

StringIndexerPathStep::~StringIndexerPathStep()
{
    delete[] m_szIndex;
    delete[] m_szIndexerName;
    m_szIndex = NULL;
}

void StringIndexerPathStep::DisconnectCurrentItem()
{
    if (m_tpPropertyAccess)
    {
        VERIFYHR(m_tpPropertyAccess->DisconnectEventHandlers());
        m_tpPropertyAccess.Clear();
    }
}

_Check_return_
HRESULT
StringIndexerPathStep::CollectionViewCurrentChanged()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spItem;

    // Disconnect from the vector source
    DisconnectCurrentItem();

    // Now get the new current value and
    // try to connect again to it
    IFC(m_tpSourceAsCV->get_CurrentItem(&spItem));

    IFC(InitializeFromSource(spItem.Get()));

    // Notify the source of changes
    IFC(RaiseSourceChanged());

Cleanup:

    RRETURN(hr);
}

_Check_return_
HRESULT
StringIndexerPathStep::ReConnect(_In_ IInspectable *pSource)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spInsp;
    ctl::ComPtr<xaml_data::ICollectionView> spSourceAsCV;

    // First cleanup ourselves
    Disconnect();

    // If the value is null or empty nothing to do
    if (PropertyValue::IsNullOrEmpty(pSource))
    {
        goto Cleanup;
    }

    // Get the property map out, if it is not one then nothing to do
    IFC(InitializeFromSource(pSource));
    if (!m_tpPropertyAccess)
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

        IFC(InitializeFromSource(spInsp.Get()));
    }

Cleanup:

    if (pSource != NULL && !IsConnected())
    {
        TraceConnectionFailure(pSource);
    }

    RRETURN(hr);
}

_Check_return_
HRESULT
StringIndexerPathStep::InitializeFromSource(_In_ IInspectable *pRawSource)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IMap<HSTRING, IInspectable *>> spMap;
    ctl::ComPtr<xaml_data::ICustomPropertyProvider> spPropertyProvider;
    wxaml_interop::TypeName sTypeName = {0};
    wrl_wrappers::HString strTypeName;
    ctl::ComPtr<IInspectable> spIndex;
    ctl::ComPtr<PropertyAccess> spPropertyAccess;
    ctl::ComPtr<IInspectable> spSource;
    spSource.Attach(ValueWeakReference::get_value_as<IInspectable>(pRawSource));

    spMap = spSource.AsOrNull<wfc::IMap<HSTRING, IInspectable *>>();
    if (spMap)
    {
        IFC(MapPropertyAccess::CreateInstance(this, spMap.Get(), m_fListenToChanges, &spPropertyAccess ));
        SetPtrValue(m_tpPropertyAccess, spPropertyAccess);
    }
    else
    {
        spPropertyProvider = spSource.AsOrNull<xaml_data::ICustomPropertyProvider>();
        if (spPropertyProvider)
        {
            IFC(strTypeName.Set(STR_LEN_PAIR(L"String")));

            sTypeName.Name = strTypeName.Get();
            sTypeName.Kind = wxaml_interop::TypeKind_Primitive;

            IFC(PropertyValue::CreateFromString(wrl_wrappers::HStringReference(m_szIndex, wcslen(m_szIndex)).Get(), &spIndex));

            IFC(IndexerPropertyAccess::CreateInstance(
                this,
                spPropertyProvider.Get(),
                sTypeName,
                spIndex.Get(),
                m_fListenToChanges,
                &spPropertyAccess));
            SetPtrValue(m_tpPropertyAccess, spPropertyAccess);
        }
    }

Cleanup:
    RRETURN(hr);
}

void StringIndexerPathStep::TraceConnectionFailure(_In_ IInspectable *pSource)
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

    IFC(DXamlCore::GetCurrentNoCreate()->GetNonLocalizedErrorString(TEXT_BINDINGTRACE_STR_INDEXER_CONNECTION_FAILED, strErrorString.GetAddressOf()));

    DebugOutput::LogBindingErrorMessage(StringCchPrintfWWrapper(
        const_cast<WCHAR*>(strErrorString.GetRawBuffer(nullptr)),
        m_szIndex,
        const_cast<WCHAR*>(strClassName.GetBuffer()),
        szTraceString));

Cleanup:
    return;
}


_Check_return_
HRESULT
StringIndexerPathStep::GetValue(_Out_ IInspectable **ppValue)
{
    HRESULT hr = S_OK;

    if (!IsConnected())
    {
        *ppValue = NULL;
        goto Cleanup;
    }

    IFC(m_tpPropertyAccess->GetValue(ppValue));

Cleanup:

    if (FAILED(hr))
    {
        TraceGetterFailure();
    }

    RRETURN(hr);
}

void StringIndexerPathStep::TraceGetterFailure()
{
    HRESULT hr = S_OK;
    const CClassInfo *pTypeInfo = NULL;
    const WCHAR *szTraceString = NULL;
    xstring_ptr strSourceClassName;
    ctl::ComPtr<IInspectable> spSource;
    wrl_wrappers::HString strErrorString;
    ctl::ComPtr<IPropertyPathListener> spListener;

    if (!DebugOutput::IsLoggingForBindingEnabled())
    {
        goto Cleanup;
    }

    IFC(m_tpPropertyAccess->GetSource(&spSource));
    IFC(MetadataAPI::GetFriendlyRuntimeClassName(spSource.Get(), &strSourceClassName));

    IFC(m_tpPropertyAccess->GetType(&pTypeInfo));

    IFC(m_spListener.As(&spListener));
    IFCEXPECT_ASSERT(spListener.Get());
    IFC(spListener.Cast<PropertyPathListener>()->GetTraceString(&szTraceString));

    IFC(DXamlCore::GetCurrentNoCreate()->GetNonLocalizedErrorString(TEXT_BINDINGTRACE_STR_INDEXER_FAILED, strErrorString.GetAddressOf()));

    DebugOutput::LogBindingErrorMessage(StringCchPrintfWWrapper(
        const_cast<WCHAR*>(strErrorString.GetRawBuffer(nullptr)),
        m_szIndex,
        pTypeInfo->GetName().GetBuffer(),
        const_cast<WCHAR*>(strSourceClassName.GetBuffer()),
        szTraceString));

Cleanup:
    return;
}


_Check_return_
HRESULT
StringIndexerPathStep::SetValue(_In_  IInspectable *pValue)
{
    HRESULT hr = S_OK;

    IFCEXPECT(IsConnected());

    IFC(m_tpPropertyAccess->SetValue(pValue));

Cleanup:

    RRETURN(hr);
}

_Check_return_
HRESULT
StringIndexerPathStep::GetType(_Outptr_ const CClassInfo **ppType)
{
    HRESULT hr = S_OK;

    IFCEXPECT(m_tpPropertyAccess);

    // Use the type of the property access instead of assuming
    // that everything is of type object. We can have indexers
    // that accept a string but are of different types.
    IFC(m_tpPropertyAccess->GetType(ppType));

Cleanup:

    RRETURN(hr);
}

_Check_return_
HRESULT
StringIndexerPathStep::SourceChanged()
{
    return RaiseSourceChanged();
}

_Check_return_
HRESULT
StringIndexerPathStep::GetIndexedPropertyName(_Outptr_result_z_ WCHAR **pszPropertyName)
{
    HRESULT hr = S_OK;

    if (m_szIndexerName == NULL)
    {
        // The size of the string will be
        // wcslen(Item[) + wcslen(m_szIndex) + wcslen(]) + 1
        size_t stringSize = wcslen(L"Item[]") + wcslen(m_szIndex) + 1;

        m_szIndexerName = new WCHAR[stringSize];

        IFCEXPECT(swprintf_s(m_szIndexerName, stringSize, L"Item[%s]", m_szIndex) >= 0);
    }

    *pszPropertyName = m_szIndexerName;

Cleanup:

    RRETURN(hr);
}

_Check_return_
HRESULT
StringIndexerPathStep::GetSourceType(_Outptr_ const CClassInfo **ppType)
{
    HRESULT hr = S_OK;
    IFCEXPECT(m_tpPropertyAccess);
    IFC(m_tpPropertyAccess->GetSourceType(ppType));
Cleanup:
    return hr;
}
