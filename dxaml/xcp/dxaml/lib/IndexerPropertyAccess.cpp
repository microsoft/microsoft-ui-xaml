// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "IndexerPropertyAccess.h"

using namespace DirectUI;
using namespace DirectUISynonyms;
using namespace xaml_data;

_Check_return_ 
HRESULT 
IndexerPropertyAccess::CreateInstance(
    _In_ IIndexedPropertyAccessHost *pOwner,
    _In_ xaml_data::ICustomPropertyProvider *pSource,
    _In_ wxaml_interop::TypeName sTypeName,
    _In_ IInspectable *pIndex,
    _In_ bool fListenToChanges,
    _Outptr_ PropertyAccess **ppPropertyAccess)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_data::ICustomProperty> spIndexer;
    const CClassInfo* pIndexerType = nullptr;
    wxaml_interop::TypeName sIndexerType = {0};
    ctl::ComPtr<IndexerPropertyAccess> spResult;

    // By default no property access is created.
    *ppPropertyAccess = nullptr;

    // Resolve the indexer by the type of the parameter.
    IFC(pSource->GetIndexedProperty(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"Item")).Get(), sTypeName, spIndexer.ReleaseAndGetAddressOf()));
    if (!spIndexer)
    {
        // No indexer found we're done
        goto Cleanup;
    }

    // Get the type of the property.
    IFC(spIndexer->get_Type(&sIndexerType));
    IFC(MetadataAPI::GetClassInfoByTypeName(sIndexerType, &pIndexerType));

    // Finally create the property access
    IFC(ctl::make<IndexerPropertyAccess>(&spResult));
    spResult->Initialize(
        pOwner,
        pSource,
        pIndex,
        spIndexer.Get(),
        pIndexerType);

    if (fListenToChanges)
    {
        IFC(spResult->AddPropertyChangedHandler());
    }

    // And return the newly constructed property access
    *ppPropertyAccess = spResult.Detach();

Cleanup:
    DELETE_STRING(sIndexerType.Name);
    RRETURN(hr);
}

void
IndexerPropertyAccess::Initialize(
    _In_ IIndexedPropertyAccessHost *pOwner,
    _In_ xaml_data::ICustomPropertyProvider *pSource,
    _In_ IInspectable *pIndex, 
    _In_ xaml_data::ICustomProperty *pProperty,
    _In_ const CClassInfo *pPropertyType)
{
    m_pOwner = pOwner;

    SetPtrValue(m_tpIndexer, pProperty);
    SetPtrValue(m_tpSource, pSource);
    SetPtrValue(m_tpIndex, pIndex);

    m_pPropertyType = pPropertyType;
}

IndexerPropertyAccess::~IndexerPropertyAccess()
{
    IGNOREHR(SafeRemovePropertyChangedHandler());
}

_Check_return_ 
HRESULT 
IndexerPropertyAccess::GetValue(_COM_Outptr_result_maybenull_ IInspectable **ppValue)
{
    ctl::ComPtr<IInspectable> spIndex;

    // Workarround for CLR bug, we need to send a new IInspectable
    // everytime or the CLR will fail to do the unboxing on the other side
    IFC_RETURN(DuplicatePropertyValue(m_tpIndex.Get(), spIndex.ReleaseAndGetAddressOf()));
    
    IFC_RETURN(m_tpIndexer->GetIndexedValue(m_tpSource.Get(), spIndex.Get(), ppValue));

    return S_OK;
}

_Check_return_ 
HRESULT 
IndexerPropertyAccess::SetValue(_In_ IInspectable *pValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spIndex;

    // Workarround for CLR bug, we need to send a new IInspectable
    // everytime or the CLR will fail to do the unboxing on the other side
    IFC(DuplicatePropertyValue(m_tpIndex.Get(), spIndex.ReleaseAndGetAddressOf()));
    
    IFC(m_tpIndexer->SetIndexedValue(m_tpSource.Get(), pValue, spIndex.Get()));

Cleanup:

    RRETURN(hr);
}

_Check_return_ HRESULT
IndexerPropertyAccess::SetSource(_In_opt_ IInspectable *pSource, _In_ BOOLEAN fListenToChanges)
{
    HRESULT hr = S_OK;

    IFC(DisconnectEventHandlers());
    SetPtrValue(m_tpSource, pSource);
    
    if (fListenToChanges)
    {
        IFC(AddPropertyChangedHandler());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ 
HRESULT 
IndexerPropertyAccess::GetSource(_Outptr_ IInspectable **ppSource)
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
IndexerPropertyAccess::DisconnectEventHandlers()
{
    HRESULT hr = S_OK;

    if (m_epPropertyChangedHandler && m_tpSource)
    {
        IFC(m_epPropertyChangedHandler.DetachEventHandler(m_tpSource.Get()));
    }

Cleanup:

    RRETURN(hr);
}

_Check_return_ 
HRESULT
IndexerPropertyAccess::AddPropertyChangedHandler()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_data::INotifyPropertyChanged> spINPC;

    spINPC = m_tpSource.AsOrNull<xaml_data::INotifyPropertyChanged>();
    if (spINPC)
    {
        IFC(m_epPropertyChangedHandler.AttachEventHandler(spINPC.Get(),
            [this](IInspectable *pSource, xaml_data::IPropertyChangedEventArgs *pArgs)
            {
                return OnPropertyChanged(pArgs);
            }));
    }

Cleanup:

    RRETURN(hr);
}


_Check_return_ 
HRESULT 
IndexerPropertyAccess::SafeRemovePropertyChangedHandler()
{
    HRESULT hr = S_OK;

    if (m_epPropertyChangedHandler)
    {
        auto spSource = m_tpSource.GetSafeReference();
        if (spSource)
        {
            IFC(m_epPropertyChangedHandler.DetachEventHandler(spSource.Get()));
        }
    }

Cleanup:
    
    RRETURN(hr);
}

_Check_return_ 
HRESULT 
IndexerPropertyAccess::PropertyChanged()
{
    HRESULT hr = S_OK;

    IFC(m_pOwner->SourceChanged());

Cleanup:

    RRETURN(hr);
}

_Check_return_ 
HRESULT 
IndexerPropertyAccess::DuplicatePropertyValue(
    _In_ IInspectable *pValue, 
    _Outptr_ IInspectable **ppDupe)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wf::IPropertyValue> spPV;
    wf::PropertyType propertyType;

    spPV = ctl::query_interface_cast<wf::IPropertyValue>(pValue);
    if (!spPV)
    {
        *ppDupe = pValue;
        ctl::addref_interface(pValue);
        goto Cleanup;
    }
    
    IFC(spPV->get_Type(&propertyType));

    switch (propertyType)
    {
    case wf::PropertyType_String:
        {
            wrl_wrappers::HString strValue;
            IFC(spPV->GetString(strValue.GetAddressOf()));
            IFC(PropertyValue::CreateFromString(strValue.Get(), ppDupe));
        }
        break;

    case wf::PropertyType_Int32:
        {
            INT32 iValue = 0;
            IFC(spPV->GetInt32(&iValue));
            IFC(PropertyValue::CreateFromInt32(iValue, ppDupe));
        }
        break;
        
    default:  

        // We should not get here since the only supported
        // indexes are int and string
        IFC(E_UNEXPECTED);
        break;
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
IndexerPropertyAccess::GetSourceType(_Outptr_ const CClassInfo **ppSourceType)
{
    HRESULT hr = S_OK;

    IFCEXPECT(IsConnected()); 

    IFC(MetadataAPI::GetClassInfoFromObject_SkipWinRTPropertyOtherType(m_tpSource.Get(), ppSourceType));

Cleanup:

    RRETURN(hr);
}
