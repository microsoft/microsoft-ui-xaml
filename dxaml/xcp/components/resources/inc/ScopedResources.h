// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <xref_ptr.h>
#include <memory>

class xstring_ptr;
class xstring_ptr_view;
class CDependencyObject;
class CResourceDictionary;
class XamlServiceProviderContext;
enum class KnownPropertyIndex : UINT16;

namespace Resources { namespace ScopedResources
{
    xstring_ptr GetOverrideKey(
        KnownPropertyIndex propertyIndex);

    _Check_return_ HRESULT MarkAsOverride(
        const xstring_ptr_view& keyName,
        const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext);

    _Check_return_ HRESULT TryCreateOverrideForContext(
        const xstring_ptr_view& referencingResourceKey,
        _In_ const CDependencyObject* const referencingResource,
        _In_ const CResourceDictionary* const referencingResourceDictionary,
        const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
        _Outptr_result_maybenull_ CDependencyObject** resultObj,
        _Out_opt_ xref_ptr<CResourceDictionary>* resultDict);

    _Check_return_ HRESULT GetOrCreateOverrideForVisualTree(
        _In_ const CDependencyObject* const object,
        const xstring_ptr_view& keyName,
        bool searchForOverrides,
        _Out_ CDependencyObject** resultObj,
        _Out_opt_ xref_ptr<CResourceDictionary>* resultDict);

    _Check_return_ HRESULT TryGetOrCreateOverrideForDictionary(
        _In_ CResourceDictionary* const thisDictionary,
        _In_opt_ const CDependencyObject* const foundValue,
        _In_opt_ const CResourceDictionary* const foundValueDictionary,
        const xstring_ptr_view& keyName,
        _Out_ CDependencyObject** resultObj,
        _Out_opt_ xref_ptr<CResourceDictionary>* resultDict);
} }
