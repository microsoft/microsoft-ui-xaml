// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <BaseValueSource.h>
#include <CValue.h>
#include "weakref_ptr.h"

class CThemeResourceExtension;
class CDependencyProperty;
class CModifiedValue;
class CResourceDictionary;
class ThemeWalkResourceCache;
class CStyle;
class XamlServiceProviderContext;
class CCoreServices;

class CThemeResource final
{
public:

    XUINT32 AddRef()
    {
        return ++m_cRef;
    }

    XUINT32 Release()
    {
        XUINT32 cRef = --m_cRef;

        if (0 == cRef)
        {
            delete this;
        }

        return cRef;
    }

    CThemeResource(_In_opt_ ThemeWalkResourceCache* resourceCache);
    CThemeResource(const CThemeResource&) = delete;
    CThemeResource& operator=(const CThemeResource&) = delete;

    explicit CThemeResource(_In_ CThemeResourceExtension* pThemeResourceExtension);

    _Check_return_ HRESULT GetValue(_Out_ CValue* pValue);
    _Check_return_ HRESULT RefreshValue();

    _Check_return_ HRESULT GetLastResolvedThemeValue(_Out_ CValue* pValue) const;

    const xstring_ptr& GetResourceKey() const { return m_strResourceKey; }

    bool IsValueFromInitialTheme() const { return m_isValueFromInitialTheme; }

    _Check_return_ HRESULT SetThemeResourceBinding(
        _In_ CDependencyObject* pDependencyObject,
        _In_ const CDependencyProperty* pDP,
        _In_opt_ CModifiedValue* pModifiedValue = nullptr,
        _In_ BaseValueSource baseValueSource = BaseValueSourceUnknown);

    static _Check_return_ HRESULT LookupResource(
        _In_ const xstring_ptr& strResourceKey,
        _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
        _In_ CCoreServices *pCore,
        _In_ bool bShouldCheckThemeResources,
        _Outptr_result_maybenull_ CThemeResource** ppValue,
        _In_opt_ CStyle* optimizedStyleParent,
        _In_ KnownPropertyIndex stylePropertyIndex);

    _Check_return_ HRESULT SetLastResolvedValue(_In_ CDependencyObject* pValueDO);

private:
    uint32_t m_cRef;
    uint8_t m_isValueFromInitialTheme : 1;
    xstring_ptr m_strResourceKey;

    _Check_return_ HRESULT SetInitialValueAndTargetDictionary(
        _In_ CDependencyObject* pValue,
        _In_ CResourceDictionary* pTargetDictionary);

    // Last resolved theme value. If no theme switch has happened, this will be the initial theme value resolved during parse.
    CValue m_lastResolvedThemeValue;

    // ThemeDictionaries from which theme value can be obtained
    // Use WeakRef to avoid cycles between theme resources and their dictionary.
    xref::weakref_ptr<CResourceDictionary> m_pTargetDictionaryWeakRef;
    // Was the last resolved value from the app's initial theme?

    // Resources are cached during a theme walk to alleviate the cost
    // of repeatedly searching our large resource dictionaries multiple
    // times for the same resource keys.
    ThemeWalkResourceCache* m_themeWalkResourceCache;
};

