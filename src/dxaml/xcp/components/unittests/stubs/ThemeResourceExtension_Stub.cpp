// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <TypeTableStructs.h>
#include <ThemeResourceExtension.h>
#include <ThemeResource.h>


CThemeResourceExtension::CThemeResourceExtension(_In_ CCoreServices* core)
    : CMarkupExtensionBase(core)
{}

_Check_return_ HRESULT
CThemeResourceExtension::GetLastResolvedThemeValue(_Out_ CValue* pValue)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT
CThemeResourceExtension::ResolveInitialValueAndTargetDictionary(
    _In_ const xstring_ptr& strResourceKey,
    _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
    _In_ CCoreServices *pCore,
    _In_ bool bShouldCheckThemeResources,
    _Outptr_result_maybenull_ CDependencyObject** ppObject,
    _Outptr_result_maybenull_ CResourceDictionary** ppTargetDictionary,
    _In_opt_ CStyle* optimizedStyleParent,
    _In_ KnownPropertyIndex stylePropertyIndex)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CThemeResourceExtension::SetThemeResourceBinding(
    _In_ CDependencyObject* pDependencyObject,
    _In_ const CDependencyProperty* pDP,
    _In_opt_ CModifiedValue* pModifiedValue,
    _In_ BaseValueSource baseValueSource)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CThemeResourceExtension::SetInitialValueAndTargetDictionary(
    _In_ CDependencyObject* pValue,
    _In_ CResourceDictionary* dictionaryReadFrom)
{
    return E_NOTIMPL;
}

void CThemeResourceExtension::Reset()
{
}

KnownTypeIndex CThemeResourceExtension::GetTypeIndex() const
{
    return static_cast<KnownTypeIndex>(0);
}

_Check_return_ HRESULT CThemeResourceExtension::ProvideValue(
    _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
    _Out_ std::shared_ptr<XamlQualifiedObject>& qoValue)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CThemeResourceExtension::NotifyThemeChangedCore(_In_ Theming::Theme theme, _In_ bool fForceRefresh)
{
    return E_NOTIMPL;
}
