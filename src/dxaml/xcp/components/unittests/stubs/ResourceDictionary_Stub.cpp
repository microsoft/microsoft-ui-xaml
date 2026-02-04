// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <DependencyObjectTraits.h>
#include <XamlOptimizedNodeList.h>
#include <SavedContext.h>
#include <ObjectWriter.h>

class CStyle;

#include <Resources.h>

_Check_return_ HRESULT
CResourceDictionary::GetKeyNoRef(
    const xstring_ptr_view&,
    _Outptr_ CDependencyObject**)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CResourceDictionary::GetImplicitStyleKeyNoRef(
    const xstring_ptr_view&,
    Resources::LookupScope,
    _Outptr_ CDependencyObject**,
    _Out_opt_ xref_ptr<CResourceDictionary>*)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CResourceDictionary::GetKeyForResourceResolutionNoRef(
    const xstring_ptr_view& ,
    Resources::LookupScope,
    _Outptr_ CDependencyObject**,
    _Out_opt_ xref_ptr<CResourceDictionary>*)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CResourceDictionary::GetKeyForResourceResolutionNoRef(
    const ResourceKey&,
    Resources::LookupScope,
    _Outptr_ CDependencyObject**,
    _Out_opt_ xref_ptr<CResourceDictionary>*)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CResourceDictionary::GetKeyFromGlobalThemeResourceNoRef(
    _In_ CCoreServices*,
    _In_ CResourceDictionary*,
    const ResourceKey&,
    _Outptr_ CDependencyObject**,
    _Out_opt_ xref_ptr<CResourceDictionary>*)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CResourceDictionary::InvalidateImplicitStyles(_In_opt_ CResourceDictionary *)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CResourceDictionary::LoadAllDeferredResources()
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CResourceDictionary::Add(
    const ResourceKey&,
    _In_ CValue *,
    _Out_opt_ CValue *,
    bool,
    bool,
    bool)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CResourceDictionary::GetKeyNoRef(
    const xstring_ptr_view& strKey,
    Resources::LookupScope scope,
    _Outptr_ CDependencyObject** keyNoRef)
{
    return E_NOTIMPL;
}

bool CResourceDictionary::AllowsResourceOverrides() const
{
    return false;
}

bool CResourceDictionary::HasPotentialOverrides() const
{
    return false;
}

_Check_return_ HRESULT CResourceDictionary::GetLocalOverrideNoRef(
    const xstring_ptr_view& strKey,
    _Outptr_result_maybenull_ CDependencyObject** resultDO,
    _Outptr_result_maybenull_ CResourceDictionary** resultDict)
{
    return E_NOTIMPL;
}

void CResourceDictionary::InvalidateNotFoundCache(bool) {}
