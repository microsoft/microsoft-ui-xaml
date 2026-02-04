// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <Microsoft.UI.Xaml.h>
#include <MinPal.h>
#include <Indexes.g.h>
#include <TypeTableStructs.h>
#include <MetadataAPI.h>
#include <DynamicMetadataStorage.h>
#include <CustomDependencyProperty.h>

using namespace DirectUI;

_Check_return_ HRESULT MetadataAPI::TryGetMetadataProvider(_Outptr_result_maybenull_ xaml_markup::IXamlMetadataProvider** ppProvider)
{
    DynamicMetadataStorageInstanceWithLock storage;

    // First check for a cached metadata provider.
    if (storage->m_metadataProvider != nullptr)
    {
        IFC_RETURN(storage->m_metadataProvider.CopyTo(ppProvider));
        return S_OK;
    }

    return E_NOTIMPL;
}

const CObjectDependencyProperty* MetadataAPI::GetNullObjectProperty()
{
    return nullptr;
}

const CObjectDependencyProperty* CObjectDependencyProperty::GetNextProperty() const
{
    return nullptr;
}

_Check_return_ HRESULT CCustomDependencyProperty::SetDefaultMetadata(_In_ xaml::IPropertyMetadata* pDefaultMetadata)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT MetadataAPI::GetClassInfoFromObject_SkipWinRTPropertyOtherType(_In_opt_ IInspectable* pInstance, _Outptr_ const CClassInfo** ppType)
{
    return S_OK;
}

_Check_return_ HRESULT MetadataAPI::GetClassInfoFromWinRTPropertyType(_In_ wf::IPropertyValue* pValue, _In_ wf::PropertyType ePropertyType, _Outptr_ const CClassInfo** ppType)
{
    *ppType = nullptr;
    return S_OK;
}