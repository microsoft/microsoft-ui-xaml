// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <MarkupExtension.h>

class ThemeWalkResourceCache;
class CStyle;

class CThemeResourceExtension
    final : public CMarkupExtensionBase
{

private:
    CThemeResourceExtension(_In_ CCoreServices *pCore);

public:
    // Creation method
    DECLARE_CREATE(CThemeResourceExtension);

    void Reset() override;

    _Check_return_ HRESULT SetThemeResourceBinding(
        _In_ CDependencyObject* pDependencyObject,
        _In_ const CDependencyProperty* pDP,
        _In_opt_ CModifiedValue* pModifiedValue = nullptr,
        _In_ BaseValueSource baseValueSource = BaseValueSourceUnknown);

    // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override;

    // If ThemeResource is pre-resolved, it will be stored in the ambient dictionary.
    // Mark is to allow multiple association to prevent wrapping in CDependencyObjectWrapper
    // when that is done.
    bool DoesAllowMultipleAssociation() const override { return true; }

    _Check_return_ HRESULT ProvideValue(
                _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
                _Out_ std::shared_ptr<XamlQualifiedObject>& qoValue) override;

    _Check_return_ HRESULT GetValue(_Out_ CValue* pValue);

    _Check_return_ HRESULT GetLastResolvedThemeValue(_Out_ CValue* pValue);

    bool IsValueFromInitialTheme() { return m_isValueFromInitialTheme; }

    xref::weakref_ptr<CResourceDictionary> GetTargetDictionaryWeekRef() const {return m_pTargetDictionaryWeakRef;}

    xstring_ptr m_strResourceKey;
    ThemeWalkResourceCache* m_themeWalkResourceCache;

    static _Check_return_ HRESULT LookupResource(
        _In_ const xstring_ptr& strResourceKey,
        _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
        _In_ CCoreServices *pCore,
        _In_ bool bShouldCheckThemeResources,
        _Outptr_result_maybenull_ CDependencyObject** ppValue,
        _In_opt_ CStyle* optimizedStyleParent = nullptr,
        _In_ KnownPropertyIndex stylePropertyIndex = KnownPropertyIndex::UnknownType_UnknownProperty);

    static _Check_return_ HRESULT ResolveInitialValueAndTargetDictionary(
        _In_ const xstring_ptr& strResourceKey,
        _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
        _In_ CCoreServices *pCore,
        _In_ bool bShouldCheckThemeResources,
        _Outptr_result_maybenull_ CDependencyObject** ppObject,
        _Outptr_result_maybenull_ CResourceDictionary** ppTargetDictionary,
        _In_opt_ CStyle* optimizedStyleParent = nullptr,
        _In_ KnownPropertyIndex stylePropertyIndex = KnownPropertyIndex::UnknownType_UnknownProperty
        );

protected:
     _Check_return_ HRESULT NotifyThemeChangedCore(_In_ Theming::Theme theme, _In_ bool fForceRefresh = false) override;

public:
    _Check_return_ HRESULT SetInitialValueAndTargetDictionary(
        _In_ CDependencyObject* pValue,
        _In_ CResourceDictionary* pTargetDictionary);
private:
    _Check_return_ HRESULT SetLastResolvedValue(_In_ CDependencyObject* pValueDO);

    _Check_return_ HRESULT SetIsValueFromInitialTheme(_In_ CResourceDictionary* pTargetDictionary);

private:
    // ThemeDictionaries from which theme value can be obtained
    // Use WeakRef to avoid cycles between theme resources and their dictionary.
    xref::weakref_ptr<CResourceDictionary> m_pTargetDictionaryWeakRef;

    // Last resolved theme value. If no theme switch has happened, this will be the initial theme value resolved during parse.
    CValue m_lastResolvedThemeValue;

    // Protect against recursion when theme resources are loaded
    static bool s_isLoadingThemeResources;

    // Was the last resolved value from the app's initial theme?
    uint8_t m_isValueFromInitialTheme : 1;
};

