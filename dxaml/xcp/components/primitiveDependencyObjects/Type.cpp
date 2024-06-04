// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <Type.h>
#include <CreateParameters.h>
#include <TypeTableStructs.h>
#include <MetadataAPI.h>

CType::CType(_In_opt_ CCoreServices* core) : 
    CNoParentShareableDependencyObject(core),
    m_nTypeIndex(KnownTypeIndex::UnknownType)
{
}

_Check_return_ HRESULT CType::GetClassName(_Out_ xstring_ptr* fullName) const
{
    *fullName = DirectUI::MetadataAPI::GetClassInfoByIndex(m_nTypeIndex)->GetFullName();
    return S_OK;
}

bool CType::IsCoreType() const
{
    return DirectUI::MetadataAPI::IsKnownIndex(m_nTypeIndex);
}
