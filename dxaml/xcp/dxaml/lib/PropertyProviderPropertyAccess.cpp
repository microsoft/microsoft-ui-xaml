// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "PropertyProviderPropertyAccess.h"

using namespace DirectUI;
using namespace DirectUISynonyms;
using namespace xaml_data;

void
PropertyProviderPropertyAccess::Initialize(
    _In_ IPropertyAccessHost *pOwner,
    _In_ xaml_data::ICustomPropertyProvider *pSource,
    _In_ xaml_data::ICustomProperty *pProperty,
    _In_ const CClassInfo *pPropertyType)
{
    m_pOwnerNoRef = pOwner;
    
    SetPtrValue(m_tpSource, pSource);
    SetPtrValue(m_tpProperty, pProperty);

    m_pPropertyType = pPropertyType;
}


PropertyProviderPropertyAccess::~PropertyProviderPropertyAccess()
{
    auto spSource = m_tpSource.GetSafeReference();
    if (spSource)
    {
        IGNOREHR(DisconnectPropertyChangedHandler(spSource.Get()));
    }
}

_Check_return_ 
HRESULT 
PropertyProviderPropertyAccess::CreateInstance(
    _In_ IPropertyAccessHost *pOwner,
    _In_ xaml_data::ICustomPropertyProvider *pSource,
    _In_ bool fListenToChanges,
    _Outptr_ PropertyAccess **ppPropertyAccess)
{
    // By default there's no property access
    *ppPropertyAccess = nullptr;

    // First try to resolve the property
    ctl::ComPtr<xaml_data::ICustomProperty> spProperty;
    IFC_RETURN(pSource->GetCustomProperty(
        wrl_wrappers::HStringReference(pOwner->GetPropertyName()).Get(),
        spProperty.ReleaseAndGetAddressOf()));

    // If no property with that name exists then we're done
    if (!spProperty)
    {
        return S_OK;
    }
    
    TypeNamePtr typeName;
    IFC_RETURN(spProperty->get_Type(typeName.ReleaseAndGetAddressOf()));

    const CClassInfo *pPropertyType = nullptr;
    {
        auto hr = MetadataAPI::GetClassInfoByTypeName(typeName.Get(), &pPropertyType);

        // We really want to fail, but unfortunately because we didn't in Windows 8.1, we can't do so now either.
        // TODO: [Add quirk.] TFS#677191: Quirk: GetClassInfoFromObject and PropertyProviderPropertyAccess should not swallow errors from GetClassInfoByTypeName anymore in Threshold and beyond.
        ASSERTSUCCEEDED(hr);
        if (FAILED(hr))
        {
            // Fall back on Object for compatibility with Windows 8.1.
            pPropertyType = MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::Object);
        }
    }

    // Translate to a primitive WinRT type (int, string, ..., object).
    IFC_RETURN(MetadataAPI::GetPrimitiveClassInfo(pPropertyType, &pPropertyType))

    ctl::ComPtr<PropertyProviderPropertyAccess> spResult;
    IFC_RETURN(ctl::make<PropertyProviderPropertyAccess>(&spResult));
    spResult->Initialize(
        pOwner,
        pSource,
        spProperty.Get(),
        pPropertyType);
    
    if (fListenToChanges)
    {
        IFC_RETURN(spResult->AddPropertyChangedHandler(pSource));
    }

    *ppPropertyAccess = spResult.Detach();

    return S_OK;
}

_Check_return_ 
HRESULT 
PropertyProviderPropertyAccess::GetValue(_COM_Outptr_result_maybenull_ IInspectable **ppValue)
{
    if (IsConnected())
    {
        IFC_RETURN(m_tpProperty->GetValue(m_tpSource.Get(), ppValue));
    }
    else
    {
        *ppValue = nullptr;
    }
    return S_OK;
}

_Check_return_ 
HRESULT 
PropertyProviderPropertyAccess::SetValue(_In_ IInspectable *pValue)
{
    IFCEXPECT_RETURN(IsConnected());
    IFC_RETURN(m_tpProperty->SetValue(m_tpSource.Get(), pValue));
    return S_OK;
}

_Check_return_ HRESULT
PropertyProviderPropertyAccess::SetSource(_In_opt_ IInspectable *pSource, _In_ BOOLEAN fListenToChanges)
{
    // If we're clearing out the source, it's possible we by-pass the code in TryReconnect. Make sure we 
    // get the source's type while we still can, so we have an opportunity in the future to re-use this path.
    if (!m_sourceType && m_tpSource)
    {
        TypeNamePtr sourceType;
        if (SUCCEEDED(m_tpSource->get_Type(sourceType.ReleaseAndGetAddressOf())))
        {
            m_sourceType = std::move(sourceType);
        }
    }

    if (fListenToChanges)
    {
        // Disconnect the handler on the previous source, and attach to the new source. Both 
        // old and new source are allowed to be NULL.
        IFC_RETURN(UpdatePropertyChangedHandler(m_tpSource.Get(), pSource));
    }
    else
    {
        IFC_RETURN(DisconnectEventHandlers());
    }
    SetPtrValue(m_tpSource, pSource);

    return S_OK;
}

_Check_return_ HRESULT
PropertyProviderPropertyAccess::TryReconnect(_In_ IInspectable* pSource, _In_ BOOLEAN fListenToChanges, _Inout_ BOOLEAN& bConnected, _Inout_ const CClassInfo*& pResolvedType)
{
    bConnected = FALSE;

    auto spCPP = ctl::query_interface_cast<xaml_data::ICustomPropertyProvider>(pSource);
    if (spCPP)
    {
        // Make sure we've resolved the type of the previous source.
        if (!m_sourceType && m_tpSource)
        {
            TypeNamePtr sourceType;
            if (SUCCEEDED(m_tpSource->get_Type(sourceType.ReleaseAndGetAddressOf())))
            {
                m_sourceType = std::move(sourceType);
            }
        }

        TypeNamePtr newSourceType;
        if (m_sourceType.Get().Name && SUCCEEDED(spCPP->get_Type(newSourceType.ReleaseAndGetAddressOf())) && m_sourceType.Get().Kind == newSourceType.Get().Kind)
        {
            INT nResult = 0;
            IFC_RETURN(WindowsCompareStringOrdinal(m_sourceType.Get().Name, newSourceType.Get().Name, &nResult));
            if (nResult == 0)
            {
                IFC_RETURN(SetSource(pSource, fListenToChanges));
                bConnected = TRUE;
            }
        }
    }

    return S_OK;
}

_Check_return_ 
HRESULT 
PropertyProviderPropertyAccess::GetSource(_COM_Outptr_result_maybenull_ IInspectable **ppSource)
{
    IFCEXPECT_RETURN(IsConnected());
    IFC_RETURN(m_tpSource.CopyTo(ppSource));
    return S_OK;
}

_Check_return_ 
HRESULT 
PropertyProviderPropertyAccess::DisconnectEventHandlers()
{
    if (IsConnected())
    {
        IFC_RETURN(DisconnectPropertyChangedHandler(m_tpSource.Get()));
    }
    return S_OK;
}

_Check_return_ 
HRESULT 
PropertyProviderPropertyAccess::OnPropertyChanged()
{
    IFC_RETURN(m_pOwnerNoRef->SourceChanged());
    return S_OK;
}

_Ret_notnull_
const wchar_t*
PropertyProviderPropertyAccess::GetPropertyName()
{
    return m_pOwnerNoRef->GetPropertyName();
}

_Check_return_
HRESULT
PropertyProviderPropertyAccess::GetSourceType(_Outptr_ const CClassInfo** ppSourceType)
{
    IFC_RETURN(MetadataAPI::GetClassInfoFromObject_SkipWinRTPropertyOtherType(m_tpSource.Get(), ppSourceType));
    return S_OK;
}
