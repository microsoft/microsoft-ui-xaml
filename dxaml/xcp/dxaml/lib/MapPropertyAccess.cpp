// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "MapPropertyAccess.h"

#pragma warning(disable:4267) //'var' : conversion from 'size_t' to 'type', possible loss of data

using namespace DirectUI;
using namespace DirectUISynonyms;
using namespace xaml_data;

void
MapPropertyAccess::Initialize(
   _In_ IPropertyAccessHost* const pOwner,
   _In_ wfc::IMap<HSTRING, IInspectable *>* const pSource)
{
    m_pOwner = pOwner;
    SetPtrValue(m_tpSource, pSource);
}

MapPropertyAccess::~MapPropertyAccess()
{
    VERIFYHR(SafeRemoveKeyChangedEventHandler());
}

_Check_return_
HRESULT
MapPropertyAccess::CreateInstance(
    _In_ IPropertyAccessHost *pOwner,
    _In_ wfc::IMap<HSTRING, IInspectable *> *pSource,
    _In_ bool fListenToChanges,
    _Outptr_ PropertyAccess **ppPropertyAccess)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<MapPropertyAccess> spResult;

    // By default there's no property access
    *ppPropertyAccess = NULL;

    IFC(ctl::make<MapPropertyAccess>(&spResult));
    spResult->Initialize(pOwner, pSource);

    IFC(spResult->m_strProperty.Set(
        spResult->m_pOwner->GetPropertyName(),
        wcslen(spResult->m_pOwner->GetPropertyName())));

    if (fListenToChanges)
    {
        IFC(spResult->AddKeyChangedEventHandler());
    }

    *ppPropertyAccess = spResult.Detach();

Cleanup:

    RRETURN(hr);
}


_Check_return_
HRESULT
MapPropertyAccess::GetValue(_COM_Outptr_result_maybenull_ IInspectable **ppValue)
{
    if (IsConnected())
    {
        IFC_RETURN(m_tpSource.Get()->Lookup(m_strProperty.Get(), ppValue));
    }
    else
    {
        *ppValue = nullptr;
        // This method will only be called if we're connected
        ASSERT(false);
    }

    return S_OK;
}


_Check_return_
HRESULT
MapPropertyAccess::SetValue(_In_ IInspectable *pValue)
{
    HRESULT hr = S_OK;
    boolean bReplaced = false;

    // This method will only be called if we're connected
    ASSERT(IsConnected());

    IFC(m_tpSource.Get()->Insert(m_strProperty.Get(), pValue, &bReplaced));

Cleanup:

    RRETURN(hr);
}

_Check_return_
HRESULT
MapPropertyAccess::MapKeyChanged()
{
    HRESULT hr = S_OK;

    IFC(m_pOwner->SourceChanged());

Cleanup:

    RRETURN(hr);
}

_Check_return_
HRESULT
MapPropertyAccess::GetType(_Outptr_ const CClassInfo **ppType)
{
    *ppType = MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::Object);
    RRETURN(S_OK);
}

bool MapPropertyAccess::IsConnected()
{
    HRESULT hr = S_OK;
    boolean bHasKey = false;

    if (m_tpSource)
    {
        // Check if the dictionary contains the key that we're looking for
        IFC(m_tpSource.Get()->HasKey(m_strProperty.Get(), &bHasKey));
    }

Cleanup:

    return bHasKey ? TRUE : FALSE;
}

_Check_return_ HRESULT
MapPropertyAccess::SetSource(_In_opt_ IInspectable *pSource, _In_ BOOLEAN fListenToChanges)
{
    HRESULT hr = S_OK;

    IFC(DisconnectEventHandlers());
    SetPtrValue(m_tpSource, pSource);

    if (fListenToChanges)
    {
        IFC(AddKeyChangedEventHandler());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
MapPropertyAccess::GetSource(_Outptr_ IInspectable **ppSource)
{
    HRESULT hr = S_OK;

    IFCEXPECT(IsConnected());

    *ppSource = m_tpSource.Get();
    AddRefInterface(*ppSource);

Cleanup:

    RRETURN(hr);
}

_Check_return_
HRESULT
MapPropertyAccess::DisconnectEventHandlers()
{
    HRESULT hr = S_OK;

    if (m_epMapChangedEventHandler && m_tpSource)
    {
        IFC(m_epMapChangedEventHandler.DetachEventHandler(m_tpSource.Get()));
    }

Cleanup:

    RRETURN(hr);
}


_Check_return_
HRESULT
MapPropertyAccess::AddKeyChangedEventHandler()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IObservableMap<HSTRING, IInspectable *>> spObservable;

    spObservable = m_tpSource.AsOrNull<wfc::IObservableMap<HSTRING, IInspectable *>>();
    if (spObservable)
    {
        IFC(m_epMapChangedEventHandler.AttachEventHandler(spObservable.Get(),
            [this](wfc::IObservableMap<HSTRING, IInspectable *> *pSender, wfc::IMapChangedEventArgs<HSTRING> *pArgs)
            {
                return OnMapChanged(pArgs);
            }));
    }

Cleanup:

    RRETURN(hr);
    return S_OK;
}

_Check_return_
HRESULT
MapPropertyAccess::SafeRemoveKeyChangedEventHandler()
{
    HRESULT hr = S_OK;

    if (m_epMapChangedEventHandler)
    {
        auto spSource = m_tpSource.GetSafeReference();
        if (spSource)
        {
            IFC(m_epMapChangedEventHandler.DetachEventHandler(spSource.Get()));
        }
    }

Cleanup:

    RRETURN(hr);
}

_Check_return_
HRESULT
MapPropertyAccess::GetSourceType(_Outptr_ const CClassInfo** ppSourceType)
{
    HRESULT hr = S_OK;

    IFC(MetadataAPI::GetClassInfoFromObject_SkipWinRTPropertyOtherType(m_tpSource.Get(), ppSourceType));
Cleanup:
    return hr;
}
