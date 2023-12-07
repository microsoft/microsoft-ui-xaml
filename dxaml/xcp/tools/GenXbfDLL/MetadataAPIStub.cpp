// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DynamicMetadataStorage.h"
#include "CustomClassInfo.h"
#include "CustomDependencyProperty.h"

using namespace DirectUI;

// Tries to get a IXamlMetadataProvider.
_Check_return_ HRESULT MetadataAPI::TryGetMetadataProvider(_Outptr_result_maybenull_ xaml_markup::IXamlMetadataProvider** ppMetadataProvider)
{
    DynamicMetadataStorageInstanceWithLock storage;
    IFC_RETURN(storage->m_metadataProvider.CopyTo(ppMetadataProvider));
    return S_OK;
}

_Check_return_ HRESULT CCustomDependencyProperty::SetDefaultMetadata(_In_ xaml::IPropertyMetadata* pDefaultMetadata)
{
    return S_OK;
}

// NO-OP to help build
_Check_return_ HRESULT CClassInfo::RunClassConstructorIfNecessary()
{
    RRETURN(S_OK);
}