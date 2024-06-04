// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <CustomWriterRuntimeData.h>
#include <CustomWriterRuntimeContext.h>
#include <ResourceDictionaryCustomRuntimeData.h>
#include <ResourceDictionary2.h>
#include <ParserAPI.h>
#include <MetadataAPI.h>
#include <limits>
#include <wil\result.h>
#include <FrameworkTheming.h>
#include <Theme.h>
#include "MUX-ETWEvents.h"
#include "resources\inc\ResourceResolver.h"
#include "resources\inc\ScopedResources.h"
#include "resources\inc\ResourceLookupLogger.h"
#include <UriXStringGetters.h>

#include "XamlNativeRuntime.h"
#include "XamlNode.h"
#include "XamlNodeType.h"
#include "XamlOptimizedNodeList.h"
#include "XamlReader.h"
#include "XamlSchemaContext.h"
#include "XamlType.h"
#include <DesignMode.h>
#include "CColor.h"
#include "ThemeWalkResourceCache.h"

// Imports from DXAML to support NullKeyedResource
#include <DependencyObject.h>
#include <NullKeyedResource.g.h>
#include <DXamlCore.h>

#pragma warning(disable:4267) // 'var' : conversion from 'size_t' to 'type', possible loss of data
#pragma warning(disable:4244) // 'argument' : conversion from 'type1' to 'type2', possible loss of data

using namespace Theming;
using namespace Resources;

CResourceDictionary::CResourceDictionary(_In_ CCoreServices* pCore)
    : CDOCollection(pCore)
    , m_nImplicitStylesCount(0)
    , m_bHasKey(false)
    , m_pMergedDictionaries(nullptr)
    , m_bAllowItems(true)
    , m_pResourceOwner(nullptr)
    , m_pActiveThemeDictionary(nullptr)
    , m_pThemeDictionaries(nullptr)
    , m_bIsThemeDictionaries(false)
    , m_isThemeDictionary(false)
    , m_activeTheme(Theme::None)
    , m_undeferredKeyCount(0)
    , m_isSystemColorsDictionary(false)
    , m_isHighContrast(false)
    , m_useAppResourcesForThemeRef(false)
    , m_isGlobal(false)
#if DBG
    , m_processingBulkUndeferral(false)
#endif
{}

HRESULT CResourceDictionary::CreateSystemColors(
    _In_ CCoreServices* coreServices,
    _Outptr_ CResourceDictionary** systemDictionary)
{
    CREATEPARAMETERS cp(coreServices);
    IFC_RETURN(CResourceDictionary::Create(reinterpret_cast<CDependencyObject**>(systemDictionary), &cp));
    (*systemDictionary)->m_isSystemColorsDictionary = true;
    (*systemDictionary)->m_isGlobal = true;
    return S_OK;
}

KnownTypeIndex CResourceDictionary::GetTypeIndex() const
{
    return DependencyObjectTraits<CResourceDictionary>::Index;
}

CResourceDictionary::~CResourceDictionary()
{
    ReleaseInterface(m_pMergedDictionaries);

    m_pActiveThemeDictionary = NULL;
    ReleaseInterface(m_pThemeDictionaries);
}

XUINT32 CResourceDictionary::GetCount() const
{
    XUINT32 count = CDOCollection::GetCount();

    if (m_pDeferredResources)
    {
        count += static_cast<XUINT32>((m_pDeferredResources->size() - m_undeferredKeyCount));
    }

    return count;
}

//------------------------------------------------------------------------
//
//  Method:     SetValue
//
//  Synopsis: Overloads SetValue to set Source correctly and propagate the
//            resource owner to Merged dictionaries.
//------------------------------------------------------------------------
_Check_return_ HRESULT
CResourceDictionary::SetValue(_In_ const SetValueParams& args)
{
    IFC_RETURN(CDOCollection::SetValue(args));

    if (args.m_pDP != nullptr)
    {
        switch (args.m_pDP->GetIndex())
        {
        case KnownPropertyIndex::ResourceDictionary_Source:
            {
                IFC_RETURN(Clear());
                if (m_pMergedDictionaries)
                {
                    IFC_RETURN(m_pMergedDictionaries->Clear());
                }

                IFC_RETURN(LoadFromSource());
                break;
            }
        case KnownPropertyIndex::ResourceDictionary_MergedDictionaries:
            {
                if (m_pMergedDictionaries)
                {
                    m_pMergedDictionaries->SetResourceOwner(m_pResourceOwner);
                }
                break;
            }
        case KnownPropertyIndex::ResourceDictionary_ThemeDictionaries:
            {
                if (m_pThemeDictionaries)
                {
                    m_pThemeDictionaries->SetResourceOwner(m_pResourceOwner);
                }
                break;
            }
        }
    }

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Process Enter on the Child and specify that its parent is
//      a resource dictionary
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CResourceDictionary::ChildEnter(
    _In_ CDependencyObject *pChild,
    _In_ CDependencyObject *pNamescopeOwner,
    EnterParams params,
    bool fCanProcessEnterLeave)
{
    EnterParams enterParams(params);
    enterParams.pParentResourceDictionary = this;

    IFC_RETURN(CDOCollection::ChildEnter(pChild, pNamescopeOwner, enterParams, fCanProcessEnterLeave));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Process Leave on the Child and specify that its parent is
//      a resource dictionary
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CResourceDictionary::ChildLeave(
    _In_ CDependencyObject *pChild,
    _In_ CDependencyObject *pNamescopeOwner,
    LeaveParams params,
    bool fCanProcessEnterLeave)
{
    LeaveParams leaveParams(params);
    leaveParams.pParentResourceDictionary = nullptr;

    IFC_RETURN(CDOCollection::ChildLeave(pChild, pNamescopeOwner, leaveParams, fCanProcessEnterLeave));

    return S_OK;
}

// Helper for CResourceDictionary::Add(), for adding an item to the valuestore of the dictionary
_Check_return_ HRESULT
CResourceDictionary::AddKey(
    const ResourceKey& key,
    _In_ CDependencyObject *pValue,
    bool keyIsOverride,
    bool undeferringKey)
{
    const WCHAR* pKey = key.GetKey().GetBuffer();

    GetContext()->GetThemeWalkResourceCache()->RemoveThemeResourceCacheEntry(key.GetKey());

    // Check that the key store size is 1 less than the collection size.
    // The value should have been added right before the call to AddKey.
    ASSERT(m_keyByIndex.size() == CDOCollection::GetCount() - 1);

    TraceResourceDictionaryAddInfo(
        reinterpret_cast<UINT64>(this),
        reinterpret_cast<UINT64>(pValue),
        pKey,
        key.IsKeyType());

    if (EventEnabledResourceDictionaryAddWithSourceInfo())
    {
        CValue line, column, uri, hash;

        IGNOREHR(pValue->GetValueByIndex(KnownPropertyIndex::DependencyObject_Line, &line));
        IGNOREHR(pValue->GetValueByIndex(KnownPropertyIndex::DependencyObject_Column, &column));
        IGNOREHR(pValue->GetValueByIndex(KnownPropertyIndex::DependencyObject_ParseUri, &uri));
        IGNOREHR(pValue->GetValueByIndex(KnownPropertyIndex::DependencyObject_XbfHash, &hash));

        TraceResourceDictionaryAddWithSourceInfo(
            reinterpret_cast<UINT64>(this),
            reinterpret_cast<UINT64>(pValue),
            uri.AsString().GetBuffer(),
            static_cast<UINT32>(line.AsSigned()),
            static_cast<UINT32>(column.AsSigned()),
            hash.AsString().GetBuffer());
    }

    CResourceDictionary* pResource = do_pointer_cast<CResourceDictionary>(pValue);

    if (m_bIsThemeDictionaries)
    {
        if (!pResource)
        {
            IFC_RETURN(CErrorService::OriginateInvalidOperationError(
                GetContext(),
                AG_E_RESOURCE_THEME_NOT_A_DICTIONARY));
        }
        else
        {
            pResource->MarkIsThemeDictionary();
        }
    }

    if (pResource &&
        m_isGlobal)
    {
        pResource->MarkIsGlobal();
    }

    ResourceKeyStorage keyStorage = key.ToStorage();

    // This method, when invoked through the programmatic API, will only be called when
    // the key has already been removed from the Dictionary. The public API surface and
    // DXAML wrapper will remove the key if present. The only other way we can call add
    // is when we're in a parsing scenario. In XBFv2 all the keys are deferred, contained
    // in the CResourceDictionary2 dictionary, and this code path is never executed. In
    // XBFv1 and text parsing scenarios we will indeed invoke the collection method directly.
    //
    // When we're parsing XBFv1 or V2 we should never have a deferred resource dictionary
    // so we don't need to check the collection at runtime. If we did that would in fact
    // be invalidating an assumption about how this class should be used in the framework.
#if DBG
    // ASSERT visible to Prefast fre build but references DBG-only variables
    ASSERT(undeferringKey ||
        m_processingBulkUndeferral ||
        !m_pDeferredResources ||
        !m_pDeferredResources->ContainsKey(keyStorage.GetKey(), keyStorage.IsKeyType()));
#endif

    // Insert the map entry if the caller didn't do it already.
    auto pair = m_resourceMap.emplace(keyStorage, nullptr);

    if (!pair.second) // second component is false if the key was already mapped
    {
        IFC_RETURN(static_cast<HRESULT>(E_DO_RESOURCE_KEYCONFLICT));
    }

    auto mapEntry = pair.first;

    // Store the key, and update the map entry's value to the resource value.
    ASSERT(mapEntry != m_resourceMap.end());
    m_keyByIndex.emplace_back(std::make_pair(keyStorage.GetKey(), KeyInfo { keyStorage.IsKeyType(), keyIsOverride }));
    mapEntry->second = pValue;

    if (undeferringKey)
    {
        m_undeferredKeyCount++;
    }

    // When we're adding a key from a deferred source it has just been
    // created. Implicit styles should have been handled though an alternative
    // code path and taken into account, and the ImplicitStyle
    // counts already reflect the entries in the deferred dictionary.
    if (keyStorage.IsKeyType() && pValue && !undeferringKey)
    {
        CStyle* style = nullptr;
        if(SUCCEEDED(DoPointerCast(style, pValue)))
        {
            ++m_nImplicitStylesCount;
            if (!IsParsing())
            {
                IFC_RETURN(NotifyImplicitStyleChanged(style, StyleUpdateType::Added));
            }
        }
    }

    if (!undeferringKey)
    {
        // Adding a key should invalidate all not-found caches on the path.
        InvalidateNotFoundCache(true, key);
    }

    return S_OK;
}

// Removes the item with the matching key from the valuestore
_Check_return_ HRESULT
CResourceDictionary::RemoveKey(
    const ResourceKeyStorage& key,
    unsigned int index)
{
    CDependencyObject* pValue = nullptr;

    GetContext()->GetThemeWalkResourceCache()->RemoveThemeResourceCacheEntry(key.GetKey());

    // This method is called from Remove/RemoveAt and by now all the remaining keys should
    // have been faulted in and the deferred resource dictionary released.
    ASSERT(!m_pDeferredResources);

    TraceResourceDictionaryRemoveInfo(reinterpret_cast<UINT64>(this), key.GetKey().GetBuffer(), key.IsKeyType());

    auto iter = m_resourceMap.find(key);
    ASSERT(iter != m_resourceMap.end());

    pValue = iter->second;

    if (index < m_keyByIndex.size())
    {
        m_resourceMap.erase(iter);
        m_keyByIndex.erase(m_keyByIndex.begin() + index); // remove the stored key
    }
    else
    {
        // The index is invalid.
        IFC_RETURN(E_FAIL);
    }

    // The value is still in the collection because we're called to remove
    // the key/index info only before removing the actual value.
    ASSERT(pValue == (*this)[index]);

    if (key.IsKeyType())
    {
        CStyle* pStyle = nullptr;
        if (SUCCEEDED(DoPointerCast(pStyle, pValue)))
        {
            --m_nImplicitStylesCount;
            IFC_RETURN(NotifyImplicitStyleChanged(pStyle, StyleUpdateType::Removed));
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   GetKeyNoRefImpl
//
//  Synopsis:  Retrieves an item from the valuestore.
//
//  keyNoRef -- Value corresponding to key. This returned value is not AddRef'd.
//
//  dictionaryReadFrom -- returns the dictionary the key was found in
//
// The scope passed in defines the scope to search for the key if not first
// found in the current dictionary.
//------------------------------------------------------------------------
_Check_return_ HRESULT
CResourceDictionary::GetKeyNoRefImpl(
    const ResourceKey& key,
    Resources::LookupScope scope,
    _Outptr_ CDependencyObject** keyNoRef,
    _Out_opt_ xref_ptr<CResourceDictionary>* dictionaryReadFrom)
{
    Diagnostics::ResourceLookupLogger* loggerNoRef = GetContext()->GetResourceLookupLogger();

    auto resourceLookupGuard = wil::scope_exit([&]
    {
        TRACE_HR_NORETURN(loggerNoRef->OnLeaveDictionary(this));
    });
    IFC_RETURN(loggerNoRef->OnEnterDictionary(this, key.GetKey()));

    ResourceKey modifiedKey(key);

    // We can only use key-not-found cache optimizations if we can guarantee that the cache is valid.
    // In current implementation, it means this and child dictionaries can use a cache (e.g. LocalOnly).
    // Any additive change in either structure or contents of dictionaries in sub-tree will be propagated
    // and will invalidate all caches on a path.
    // In order to avoid duplication of checking caches for positive lookups, they will only be created
    // and checked on edges between client code and resource lookup code.  This is controlled by a
    // flag returned by key.ShouldFilter().  The flag is initially set to true by callers of GetKeyNoRefImpl.
    // The first dictionary which is eligible to be cached (e.g. scope == LocalOnly), will reset the flag
    // on key to prevent further checking within dictionary sub-tree.
    // We will also skip the cache if a resource lookup trace is being recorded as we want to ensure we
    // follow the same path that led to the key being deemed "not found" in the first place.

    const bool useKeysNotFoundCache =
        (scope == Resources::LookupScope::LocalOnly) &&
        key.ShouldFilter() &&
        !loggerNoRef->IsLogging();

    if (useKeysNotFoundCache)
    {
        if (m_keysNotFoundCache &&
            m_keysNotFoundCache->Exists(key))
        {
            // We checked this key before and it wasn't found, no need to check again.
            return S_OK;
        }

        // The key is either here or not, but we are trying to retrieve it for the first time.
        modifiedKey.ClearShouldFilter();
    }

    auto value = FindResourceByKey(modifiedKey);
    bool undeferring = false;

    if (!value)
    {
        IFC_RETURN(FindDeferredResource(
            modifiedKey,
            undeferring,
            &value));
    }

    // If we got the value, set the out parameter
    if (value)
    {
        if (dictionaryReadFrom)
        {
            dictionaryReadFrom->reset(this);
        }
    }

    if (!value && Resources::DoesScopeMatch(scope, Resources::LookupScope::Merged) && m_pMergedDictionaries)
    {
        // NOTE: This looks like a buggy implementation of precedence rules as global resources would take immediate
        // precedence when looked up through MergedDictionary skipping a lookup in ThemeDictionaries associated with this
        // resource dictionary.
        // For sake of compatibility, I'm not changing this behavior. The check for global resources through GetKeyNoRef()
        // [when bShouldCheckThemeResources is True] in the Merged Dictionary should only occur once and in the rest of
        // this code path.
        // So reset the GlobalTheme flag to indicate we can skip over it.
        for (XINT32 i = m_pMergedDictionaries->GetCount() - 1; i >= 0 && !value; i--)
        {
            auto mergedDictionaryLookupGuard = wil::scope_exit([&]
            {
                TRACE_HR_NORETURN(loggerNoRef->OnLeaveMergedDictionary(i));
            });
            IFC_RETURN(loggerNoRef->OnEnterMergedDictionary(i, key.GetKey()));

            xref_ptr<CResourceDictionary> currentDictionary;
            currentDictionary.attach(static_cast<CResourceDictionary*>(m_pMergedDictionaries->GetItemWithAddRef(i)));
            IFC_RETURN(currentDictionary->GetKeyNoRefImpl(modifiedKey, scope, &value, dictionaryReadFrom));
            scope &= ~Resources::LookupScope::GlobalTheme;
        }
    }

    if (!value && Resources::DoesScopeMatch(scope, Resources::LookupScope::LocalTheme))
    {
        IFC_RETURN(GetKeyFromThemeDictionariesNoRef(
            modifiedKey,
            &value,
            dictionaryReadFrom));
    }

    if (!value && Resources::DoesScopeMatch(scope,Resources::LookupScope::GlobalTheme))
    {
        IFC_RETURN(GetKeyFromGlobalThemeResourceNoRef(
            GetContext(),
            this,
            modifiedKey,
            &value,
            dictionaryReadFrom));
    }

    if (!value &&
        useKeysNotFoundCache &&
        !undeferring)
    {
        // Key was not found, and we are allowing caching.  Cache it!

        if (!m_keysNotFoundCache)
        {
            m_keysNotFoundCache = std::make_unique<Resources::details::ResourceKeyCache>();
        }

        m_keysNotFoundCache->Add(key);

        return S_OK;
    }

    *keyNoRef = value;
    return S_OK;
}

_Check_return_ HRESULT CResourceDictionary::FindDeferredResource(
    const ResourceKey& key,
    _Out_ bool& undeferring,
    _Outptr_ CDependencyObject** result)
{
    *result = nullptr;

    if (m_pDeferredResources)
    {
        // Check first if there's XBFv2-style deferred keys
        IFC_RETURN(TryLoadDeferredResource(
            key,
            undeferring,
            result));
    }
    else if (m_pRuntimeDeferredKeys &&
             !m_pRuntimeDeferredKeys->IsExpanding() &&
             !key.IsKeyType())
    {
        // The old method of deferring keys at runtime did not support implicit keys
        bool bFound = false;

        IFC_RETURN(m_pRuntimeDeferredKeys->Load(key.GetKey(), &bFound));

        if (m_pRuntimeDeferredKeys->IsEmpty())
        {
            m_pRuntimeDeferredKeys.reset();
        }

        if (!!bFound)
        {
            // try again now that we expanded the deferred node
            *result = FindResourceByKey(key);
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   GetKeyNoRef
//
//  Synopsis:  Retrieves an item from the valuestore.
//
//  keyNoRef -- Value corresponding to key. This returned value is not AddRef'd.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CResourceDictionary::GetKeyNoRef(
    const xstring_ptr_view& strKey,
    Resources::LookupScope scope,
    _Outptr_ CDependencyObject** keyNoRef)
{
    *keyNoRef = nullptr;
    return GetKeyNoRefImpl(ResourceKey(strKey, false), scope, keyNoRef);
}

_Check_return_ HRESULT
CResourceDictionary::GetKeyNoRef(
    const xstring_ptr_view& strKey,
    _Outptr_ CDependencyObject** keyNoRef)
{
    *keyNoRef = nullptr;
    return GetKeyNoRefImpl(ResourceKey(strKey, false), Resources::LookupScope::All, keyNoRef);
}

_Check_return_ HRESULT
CResourceDictionary::GetImplicitStyleKeyNoRef(
    const xstring_ptr_view& strKey,
    Resources::LookupScope resourceLookupScope,
    _Outptr_ CDependencyObject** keyNoRef,
    _Out_opt_ xref_ptr<CResourceDictionary>* dictionaryReadFrom)
{
    *keyNoRef = nullptr;

    if (dictionaryReadFrom)
    {
        dictionaryReadFrom->reset();
    }

    return GetKeyNoRefImpl(ResourceKey(strKey, true), resourceLookupScope, keyNoRef, dictionaryReadFrom);
}

_Check_return_ HRESULT CResourceDictionary::GetKeyForResourceResolutionNoRef(
    const xstring_ptr_view& resourceKey,
    Resources::LookupScope resourceLookupScope,
    _Outptr_ CDependencyObject** keyNoRef,
    _Out_opt_ xref_ptr<CResourceDictionary>* dictionaryReadFrom)
{
    *keyNoRef = nullptr;

    if (dictionaryReadFrom)
    {
        dictionaryReadFrom->reset();
    }

    // when resolving resources, the key will never be a type.
    return GetKeyNoRefImpl(ResourceKey(resourceKey, false), resourceLookupScope, keyNoRef, dictionaryReadFrom);
}

_Check_return_ HRESULT CResourceDictionary::GetKeyForResourceResolutionNoRef(
    const ResourceKey& key,
    Resources::LookupScope resourceLookupScope,
    _Outptr_ CDependencyObject** keyNoRef,
    _Out_opt_ xref_ptr<CResourceDictionary>* dictionaryReadFrom)
{
    *keyNoRef = nullptr;

    if (dictionaryReadFrom)
    {
        dictionaryReadFrom->reset();
    }

    // when resolving resources, the key will never be a type.
    return GetKeyNoRefImpl(ResourceKey(key, false), resourceLookupScope, keyNoRef, dictionaryReadFrom);
}

//------------------------------------------------------------------------
//
//  Method:   GetKeyFromThemeDictionariesNoRef
//
//  Synopsis:  Lookup key in theme dictionaries. Also remembers active
//  theme dictionary to avoid finding it the next time, if theme has not
//  changed.
//
//  ppDO -- Returns value corresponding to key. NULL if not found.
//       The returned value is not AddRef'd.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CResourceDictionary::GetKeyFromThemeDictionariesNoRef(
    const ResourceKey& key,
    _Outptr_ CDependencyObject** ppDO,
    _Out_opt_ xref_ptr<CResourceDictionary>* themeResourcesReadFrom)
{
    *ppDO = nullptr;

    IFC_RETURN(EnsureActiveThemeDictionary());

    // Lookup key in active theme dictionary

    if (m_pActiveThemeDictionary)
    {
        Diagnostics::ResourceLookupLogger* loggerNoRef = GetContext()->GetResourceLookupLogger();

        auto themeDictionaryLookupGuard = wil::scope_exit([&]
        {
            TRACE_HR_NORETURN(loggerNoRef->OnLeaveThemeDictionary(m_activeTheme));
        });
        IFC_RETURN(loggerNoRef->OnEnterThemeDictionary(m_activeTheme, key.GetKey()));

        auto core = GetContext();

        IFC_RETURN(m_pActiveThemeDictionary->GetKeyNoRefImpl(key, LookupScope::LocalOnly, ppDO, themeResourcesReadFrom));

        if (*ppDO != nullptr)
        {
            // Always allow Application.Resources to override values found in the global ThemeDictionaries.
            if (m_pThemeDictionaries->IsGlobalThemeDictionaries())
            {
                IFC_RETURN(GetKeyOverrideFromApplicationResourcesNoRef(
                    core,
                    key,
                    ppDO,
                    themeResourcesReadFrom
                ));
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CResourceDictionary::EnsureActiveThemeDictionary()
{
    if (!m_pThemeDictionaries)
    {
        return S_OK;
    }

    auto core = GetContext();

    // Update active theme dictionary if theme has changed.
    // Don't update for old apps, for which theme change was not supported.

    const bool baseThemeChanged = !core->IsThemeRequestedForSubTree() && (Theming::GetBaseValue(m_activeTheme) != core->GetFrameworkTheming()->GetBaseTheme());
    const bool requestedThemeChanged = core->IsThemeRequestedForSubTree() && (Theming::GetBaseValue(m_activeTheme) != core->GetRequestedThemeForSubTree());
    const bool highContrastChanged  = (m_activeTheme & Theme::HighContrastMask) != core->GetFrameworkTheming()->GetHighContrastTheme();

    if (!m_pActiveThemeDictionary ||                                            // No active theme dictionary
        (baseThemeChanged || requestedThemeChanged || highContrastChanged))    // and theme changed
    {
        CDependencyObject* pDOResources = nullptr;
        bool isHighContrast = false;

        // If we already have an active theme dictionary, that means we're trying to
        // resolve a different one because of a theme switch.
        const bool fThemeSwitchOccurred = (m_pActiveThemeDictionary != nullptr);

        // Find theme dictionary corresponding to current theme

        // A high contrast theme was requested:
        // - If a subtree requested a theme, we will use the high contrast version of that theme.
        // - Otherwise, we will pick the high contrast for the app-wide theme.
        if (core->GetFrameworkTheming()->HasHighContrastTheme())
        {
            if (core->IsThemeRequestedForSubTree())
            {
                switch (core->GetRequestedThemeForSubTree())
                {
                    case Theme::Light:
                        IFC_RETURN(m_pThemeDictionaries->GetKeyNoRef(XSTRING_PTR_EPHEMERAL(L"HighContrastWhite"), LookupScope::LocalOnly, &pDOResources));
                        break;

                    case Theme::Dark:
                        IFC_RETURN(m_pThemeDictionaries->GetKeyNoRef(XSTRING_PTR_EPHEMERAL(L"HighContrastBlack"), LookupScope::LocalOnly, &pDOResources));
                        break;

                    default:
                        // We should never get here because IsThemeRequestedForSubTree() returned true.
                        __assume(0);
                        break;
                }
            }
            else
            {
                switch (core->GetFrameworkTheming()->GetHighContrastTheme())
                {
                    case Theme::HighContrastBlack:
                        IFC_RETURN(m_pThemeDictionaries->GetKeyNoRef(XSTRING_PTR_EPHEMERAL(L"HighContrastBlack"), LookupScope::LocalOnly, &pDOResources));
                        break;

                    case Theme::HighContrastWhite:
                        IFC_RETURN(m_pThemeDictionaries->GetKeyNoRef(XSTRING_PTR_EPHEMERAL(L"HighContrastWhite"), LookupScope::LocalOnly, &pDOResources));
                        break;

                    case Theme::HighContrastCustom:
                        IFC_RETURN(m_pThemeDictionaries->GetKeyNoRef(XSTRING_PTR_EPHEMERAL(L"HighContrastCustom"), LookupScope::LocalOnly, &pDOResources));
                        break;
                }
            }

            if (!pDOResources)
            {
                IFC_RETURN(m_pThemeDictionaries->GetKeyNoRef(XSTRING_PTR_EPHEMERAL(L"HighContrast"), LookupScope::LocalOnly | Resources::LookupScope::GlobalTheme, &pDOResources));
            }

            isHighContrast = pDOResources != nullptr;
        }

        if (!pDOResources)
        {
            // If a subtree requested a theme, get it. Otherwise get the app-wide theme.
            auto requestedTheme = core->IsThemeRequestedForSubTree() ? core->GetRequestedThemeForSubTree() : core->GetFrameworkTheming()->GetBaseTheme();

            if (requestedTheme != Theme::None)
            {
                switch (requestedTheme)
                {
                    case Theme::Light:
                        IFC_RETURN(m_pThemeDictionaries->GetKeyNoRef(XSTRING_PTR_EPHEMERAL(L"Light"), LookupScope::LocalOnly, &pDOResources));
                        break;

                    case Theme::Dark:
                        IFC_RETURN(m_pThemeDictionaries->GetKeyNoRef(XSTRING_PTR_EPHEMERAL(L"Dark"), LookupScope::LocalOnly, &pDOResources));
                        break;

                    default:
                        IFC_RETURN(E_FAIL);
                        break;
                }
            }

            if (!pDOResources)
            {
                IFC_RETURN(m_pThemeDictionaries->GetKeyNoRef(XSTRING_PTR_EPHEMERAL(L"Default"), &pDOResources));
            }
        }

        if (pDOResources)
        {
            // Remember active theme dictionary, to avoid finding it next
            // time if theme hasn't changed.
            // Not Addref'd because m_pThemeDictionaries keeps it alive
            IFC_RETURN(DoPointerCast(m_pActiveThemeDictionary, pDOResources));

            if (isHighContrast)
            {
                m_pActiveThemeDictionary->MarkIsHighContrast();
            }

            // Remember active theme dictionary's theme
            m_activeTheme = core->IsThemeRequestedForSubTree() ? core->GetRequestedThemeForSubTree() : core->GetFrameworkTheming()->GetBaseTheme();
            m_activeTheme = m_activeTheme | core->GetFrameworkTheming()->GetHighContrastTheme();

            // Make sure we re-evaluate all ThemeResource expressions inside this
            // theme dictionary first if a theme switch occurred.
            if (fThemeSwitchOccurred)
            {
                IFC_RETURN(m_pActiveThemeDictionary->NotifyThemeChanged(m_activeTheme, highContrastChanged));
            }
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   GetKeyFromGlobalThemeResourceNoRef
//
//  Synopsis:  Lookup key in glocal system colors and theme resources
//
//  ppDO -- Returns value corresponding to key. NULL if not found.
//       The returned value is not AddRef'd.
//
//  themeResourcesReadFrom -- If Value was from global system colors,
//     global theme resources or Application's theme resources, returns that
//     dictionary.
//
//------------------------------------------------------------------------
/*static*/ _Check_return_ HRESULT
CResourceDictionary::GetKeyFromGlobalThemeResourceNoRef(
    _In_ CCoreServices *pCore,
    _In_ CResourceDictionary *pSkipDictionary,
    const ResourceKey& key,
    _Outptr_ CDependencyObject** ppDO,
    _Out_opt_ xref_ptr<CResourceDictionary>* themeResourcesReadFrom
    )
{
    // We currently have a bug where we'll end up with a StackOverflow if we ever define (and attempt to resolve)
    // a system color or theme resource named after a theme dictionary (e.g. "Default"). Once we properly guard
    // against this, we could actually first look in Application.Resources, and only look in the system colors or
    // theme resource dictionary if we didn't find the resource.

    // Look in System Colors
    CResourceDictionary* pSystemColorsResourcesNoRef = NULL;
    IFC_RETURN(pCore->GetSystemColorsResources(&pSystemColorsResourcesNoRef));

    if (pSystemColorsResourcesNoRef != nullptr && pSystemColorsResourcesNoRef != pSkipDictionary)
    {
        IFC_RETURN(pSystemColorsResourcesNoRef->GetKeyNoRefImpl(key, LookupScope::LocalOnly, ppDO, themeResourcesReadFrom));

        if (*ppDO)
        {
            // Allow Application.Resources to override system colors.
            IFC_RETURN(GetKeyOverrideFromApplicationResourcesNoRef(
                pCore,
                key,
                ppDO,
                themeResourcesReadFrom
                ));
        }
    }

    // Look in Global Theme Resources
    if (!(*ppDO))
    {
        CResourceDictionary* pThemeResourcesNoRef = pCore->GetThemeResources();

        if (pThemeResourcesNoRef != nullptr && pThemeResourcesNoRef != pSkipDictionary)
        {
            IFC_RETURN(pThemeResourcesNoRef->GetKeyNoRefImpl(key, LookupScope::LocalOnly, ppDO, themeResourcesReadFrom));

            // If this is a theme resource key, allow Application.Resources to override it.
            if (*ppDO)
            {
                IFC_RETURN(GetKeyOverrideFromApplicationResourcesNoRef(
                    pCore,
                    key,
                    ppDO,
                    themeResourcesReadFrom));
            }
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Allows Application.Resources to override global ThemeResource keys.
//
//  ppDO -- Returns value corresponding to key. NULL if not found.
//       The returned value is not AddRef'd.
//
//  dictionaryReadFrom -- If Value was from global system colors,
//     global theme resources or Application's theme resources, returns that
//     dictionary.
//
//------------------------------------------------------------------------
/*static*/ _Check_return_ HRESULT
CResourceDictionary::GetKeyOverrideFromApplicationResourcesNoRef(
    _In_ CCoreServices *pCore,
    const ResourceKey& key,
    _Inout_ CDependencyObject** ppDO,
    _Out_opt_ xref_ptr<CResourceDictionary>* dictionaryReadFrom
    )
{
    CDependencyObject* pApplicationResourceNoRef = nullptr;
    xref_ptr<CResourceDictionary> appDictionaryReadFrom;

    auto appDictionary = pCore->GetApplicationResourceDictionary();

    if (appDictionary)
    {
        IFC_RETURN(appDictionary->GetKeyForResourceResolutionNoRef(
            key,
            Resources::LookupScope::LocalOnly,
            &pApplicationResourceNoRef,
            &appDictionaryReadFrom));
    }

    if (pApplicationResourceNoRef)
    {
        *ppDO = pApplicationResourceNoRef;
        appDictionaryReadFrom->SetUseAppResourcesForThemeRef(true);
        if (dictionaryReadFrom)
        {
            *dictionaryReadFrom = std::move(appDictionaryReadFrom);
        }
    }

    return S_OK;
}

//  Adds an item to the valuestore of the dictionary
_Check_return_ HRESULT
CResourceDictionary::Add(
    const ResourceKey& key,
    _In_ CValue *pValue,
    _Out_opt_ CValue *pResult,
    bool keyIsOverride,
    bool fIgnoreAllowItemsFlag,
    bool undeferringKey)
{
    // Don't allow elements from XAML if Source is set during parse
    // and we're not deliberately ignoring m_bAllowItems (as would be the case when
    // adding expanded deferred resources)
    if (!m_bAllowItems && !fIgnoreAllowItemsFlag && !DesignerInterop::GetDesignerMode(DesignerMode::V2Only))
    {
        // VisualStudio Designer uses XAML ResourceDictionary instances as a key store for its own values:
        //  ex: WindowsUIXamlViewNodeManagerDesignTimeProperties.InstantiatedElementViewNodeProperty
        // XAML's new WinUI controls require an instance of DEPControls ThemeResource be placed in the App or Page
        // merged dictionaries:  this control sets its Source property.  Due to this, we need to add an
        // exception for ResourceDictionaries when Source has been set, only for the Designer scenario.
        IFC_RETURN(SetAndOriginateError(E_NER_INVALID_OPERATION, ParserError, AG_E_RESOURCE_LOCAL_VALUES_NOT_ALLOWED));
    }

    // All resources must have a key, either implicit or explicit.
    // CCollection::Add calls CResourceDictionary::Append, and we check
    // m_bHasKey there to validate that the resource was added with a key.
    m_bHasKey = true;
    auto cleanupGuard = wil::scope_exit([&]
    {
        m_bHasKey = false;
    });

    // In case we got a naked null value, repackage it into a CNullKeyedResource to keep CCollection happy
    // Necessary because for resource values provided by markup extension, the parser will call directly into this
    // method, so this is the only logical place to wrap them if necessary
    if (pValue->IsNull())
    {
        pValue->SetAddRef<valueObject>(DirectUI::DXamlCore::GetCurrent()->GetCachedNullKeyedResource()->GetHandle());  // add-ref to keep alive after ComPtr goes out of scope
    }

    IFC_RETURN(CCollection::Add(this, 1, pValue, pResult));

    // This looks like an incorrect implementation: We've already added the key to the
    // underlying collection which means it's a child of the DO and will start to behave
    // differently. If this fails we're in SpookieVille, USA: Home of Undefined Framework Behavior.
    if (pValue->GetType() != valueObject)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    HRESULT xr = AddKey(key, pValue->AsObject(), keyIsOverride, undeferringKey);

    if (FAILED(xr))
    {
        // This copy is to avoid modifying the incoming CValue struct
        // which will be needed later for future cleanup
        CDependencyObject *pObj = pValue->AsObject();
        CDOCollection::Remove(pObj);
        ReleaseInterface(pObj);
        IFC_RETURN(xr);
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   Append
//
//  Synopsis:
//      Append a reference to an object to the collection.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
CResourceDictionary::Append(_In_ CDependencyObject *pObject, _Out_ XUINT32 *pnIndex)
{
    IFCEXPECTRC_RETURN(m_bHasKey, E_FAIL);

    // If this item has a managed peer then we might have to do work to keep it alive
    // (prevent it from being GC-ed).  For example, if this is a managed class that
    // derives directly from DependencyObject.
    //
    // Note: It's important that we do this before the item's parent gets set to avoid
    // a possible extra UpdateTreeParent call in SetParticipatesInManagedTreeDefault.
    if (pObject && pObject->HasManagedPeer())
    {
        IFC_RETURN(pObject->SetParticipatesInManagedTreeDefault());
    }

    // Add to the collection.  Whether the resource has an x:Key or an x:Name,
    // we always add to the collection before adding the key.
    IFC_RETURN(CDOCollection::Append(pObject, pnIndex));

    return S_OK;
}



//------------------------------------------------------------------------
//
//  Method:   OnAddToCollection
//
//  Synopsis:
//      Process new resource items
//
//------------------------------------------------------------------------

 _Check_return_ HRESULT
CResourceDictionary::OnAddToCollection(_In_ CDependencyObject *pDO)
{
    // When we get a new resource item, that item doesn't need to keep a ref
    // count on the core any longer.

    pDO->ContextRelease();

    if (pDO->OfTypeByIndex<KnownTypeIndex::ResourceDictionary>())
    {
        // Adding a new dictionary, invalidate all caches.
        InvalidateNotFoundCache(true);
    }

    if (m_bIsThemeDictionaries)
    {
        IFC_RETURN(NotifyThemeDictionariesChanged());
    }

    IFC_RETURN(CDOCollection::OnAddToCollection(pDO));

    return S_OK;

}



//------------------------------------------------------------------------
//
//  Method:   OnRemoveFromCollection
//
//  Synopsis:
//      Process an item that's been removed from the dictionary.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CResourceDictionary::OnRemoveFromCollection(_Inout_opt_ CDependencyObject *pDO, _In_ XINT32 iPreviousIndex)
{
    // Since we are being pulled out of the tree grab a ref to the core, so that the core
    // will not go away on us.  This will be released when we are added to the tree or we are
    // deleted.

    pDO->ContextAddRef();

    if (m_bIsThemeDictionaries)
    {
        IFC_RETURN(NotifyThemeDictionariesChanged());
    }

    IFC_RETURN(CDOCollection::OnRemoveFromCollection(pDO, iPreviousIndex));

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method:   clear
//
//  Synopsis:
//        Remove all the objects in a collection.
//------------------------------------------------------------------------
_Check_return_
HRESULT
CResourceDictionary::Clear()
{
    HRESULT hr = S_OK;
    HRESULT xhr = S_OK;

    TraceResourceDictionaryClearInfo((XUINT64)this);

    xhr = CCollection::Clear();
    m_resourceMap.clear();
    m_keyByIndex.clear();
    IFC(InvalidateImplicitStyles(NULL));
    m_pDeferredResources.reset();

    if (m_bIsThemeDictionaries)
    {
        IFC(NotifyThemeDictionariesChanged());
    }

Cleanup:
    if (FAILED(xhr))
    {
        hr = xhr;
    }

    return hr;
}

_Check_return_
HRESULT
CResourceDictionary::Remove(
        _In_ const xstring_ptr& strKey,
        _In_ bool fKeyIsType
    )
{
    HRESULT hr = S_OK;
    CDependencyObject *pDO = NULL;

    if (m_pDeferredResources && m_pDeferredResources->ContainsKey(strKey, !!fKeyIsType))
    {
        IFC(LoadAllDeferredResources());
    }

    IGNOREHR(GetKeyNoRefImpl(ResourceKey(strKey, !!fKeyIsType), Resources::LookupScope::SelfOnly, &pDO));
    if (pDO)
    {
        CDependencyObject* pReturnedObject = CDOCollection::Remove(pDO);
        if (!pReturnedObject)
        {
            hr = S_FALSE;
        }
        ReleaseInterface(pReturnedObject);
    }
    else
    {
        IFC(S_FALSE);
    }
Cleanup:
    return hr;
}

_Check_return_ void *
CResourceDictionary::RemoveAt(_In_ XUINT32 nIndex)
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES

#if DBG
    // ASSERT visible to Prefast fre build but references DBG-only variables
    ASSERT(!m_processingBulkUndeferral);
#endif

    if (m_pDeferredResources)
    {
        IFC(LoadAllDeferredResources());
    }

    IFCEXPECTRC(nIndex < m_keyByIndex.size(), E_FAIL);

    {
        const auto& keyValuePair = m_keyByIndex[nIndex];

#if DBG
        CDependencyObject* dbgValue = FindResourceByKey(ResourceKey(keyValuePair.first, keyValuePair.second.m_isType));
        CDependencyObject *dbgDOToRemove = static_cast<CDependencyObject*>(CDOCollection::GetItemWithAddRef(nIndex));
        ASSERT(dbgValue && dbgValue == dbgDOToRemove);
        ReleaseInterfaceNoNULL(dbgDOToRemove);
#endif

        IFC(RemoveKey(ResourceKeyStorage(keyValuePair.first, keyValuePair.second.m_isType), nIndex));
    }

Cleanup:
    return CDOCollection::RemoveAt(nIndex);
}

_Check_return_ HRESULT
CResourceDictionary::GetUndeferredImplicitStyles(Jupiter::stack_vector<CStyle*, 8>* styles)
{
    IFCEXPECT_RETURN(styles);

    for (auto& entry : m_resourceMap)
    {
        const auto& resourceKey = entry.first;

        if (!resourceKey.IsKeyType()) // if not implicit
            continue;

        CDependencyObject* pValue = entry.second;

        if (!pValue || !pValue->OfTypeByIndex<KnownTypeIndex::Style>())
            continue;

        CStyle* pStyle = nullptr;
        IFC_RETURN(DoPointerCast(pStyle, pValue));
        styles->m_vector.push_back(pStyle);
    }

    return S_OK;
}


//----------------------------------------------------------
//
// Method: LoadFromSource
//
// Sets this ResourceDictionary instance to have the contents specified
// by m_strSource
//
//-----------------------------------------------------------
_Check_return_ HRESULT
CResourceDictionary::LoadFromSource()
{
    XUINT32 cSource;
    const WCHAR* pSource = m_strSource.GetBufferAndCount(&cSource);

    if (nullptr == pSource || 0 == cSource || L'\0' == pSource[0])
    {
        return S_OK;
    }

    auto core = GetContext();

    xstringmap<bool>& uriCache = core->GetResourceDictionaryUriCache();
    bool currentlyParsing = false;
    if (uriCache.Find(pSource, cSource, currentlyParsing) && currentlyParsing)
    {
        IFC_RETURN(SetAndOriginateError(E_NER_INVALID_OPERATION, ParserError, AG_E_RESOURCE_CYCLE_DETECTED));
    }

    IFC_RETURN(uriCache.Add(pSource, cSource, TRUE));
    auto uriCacheGuard = wil::scope_exit([&]()
    {
        ASSERT(uriCache.ContainsKey(pSource, cSource));
        VERIFYHR(uriCache.Remove(pSource, cSource));
    });

    xref_ptr<IPALResourceManager> spResourceManager;
    IFC_RETURN(core->GetResourceManager(spResourceManager.ReleaseAndGetAddressOf()));
    bool isAmbiguousUri = false;
    IFC_RETURN(spResourceManager->IsAmbiguousUriFragment(m_strSource, &isAmbiguousUri));

    ComponentResourceLocation resourceLocation = ComponentResourceLocation::Application;
    IPALUri* preferredBaseURI = GetPreferredBaseUri();

    if (!preferredBaseURI || !isAmbiguousUri)
    {
        // If the uri is unambiguous, we want our own base URI instead of our parent's
        xref_ptr<IPALUri> spBaseUri;
        IFC_RETURN(CApplication::CreateBaseUri(cSource, pSource, spBaseUri.ReleaseAndGetAddressOf()));
        SetBaseUri(spBaseUri);
        preferredBaseURI = spBaseUri.get();
    }
    else if (preferredBaseURI && isAmbiguousUri)
    {
        // If we have a base URI (either from ourselves or our parent), and the specified Source
        // URI is ambiguous, then the resulting Source URI should inherit our base URI's
        // component location; otherwise leave it at the default location of "Application".
        // This ensures that the correct URI is constructed when looking up an x:Uid specified in
        // markup loaded via ResourceDictionary.Source.
        resourceLocation = preferredBaseURI->GetComponentResourceLocation();
    }

    xref_ptr<IPALUri> spResourceUri;
    IFC_RETURN(spResourceManager->CombineResourceUri(preferredBaseURI, m_strSource, spResourceUri.ReleaseAndGetAddressOf()));
    spResourceUri->SetComponentResourceLocation(resourceLocation);

    xref_ptr<IPALMemory> spResourceMem;
    bool isBinaryXaml = false;
    Parser::XamlBuffer buffer;
    IFC_RETURN(core->LoadXamlResource(spResourceUri, &isBinaryXaml, spResourceMem.ReleaseAndGetAddressOf(), &buffer));

    // Now that we're all loaded, try to make a relative URI into an absolute one
    if (isAmbiguousUri)
    {
        SetBaseUri(spResourceUri);
    }

    m_bAllowItems = true;

    // Prepare an event payload for an upcoming ParseXaml trace event only if tracing is enabled.
    xstring_ptr strUri;
    if (EventEnabledParseXamlBegin())
    {
        IFC_RETURN(spResourceUri->GetCanonical(&strUri));
    }
    xref_ptr<CDependencyObject> spDO;

    // Allow the designer to perform template validation when calling Application::LoadComponent
    auto expandTemplatesDuringParse = DesignerInterop::GetDesignerMode(DesignerMode::V2Only);

    IFC_RETURN(core->ParseXamlWithExistingFrameworkRoot(
            buffer,
            this,
            xstring_ptr(),
            strUri,
            expandTemplatesDuringParse,
            spDO.ReleaseAndGetAddressOf()));
    m_bAllowItems = false;

    return S_OK;
}

//---------------------------------------------------
//
// Method: InvalidateImplicitStyles
//
// Invalidate implicit styles in the dictionary
//
//---------------------------------------------------
_Check_return_ HRESULT
CResourceDictionary::InvalidateImplicitStyles(_In_opt_ CResourceDictionary *pOldResources)
{
    CUIElement * pVisualOwner = do_pointer_cast<CUIElement>(m_pResourceOwner);

    if (pVisualOwner)
    {
        IFC_RETURN(CFrameworkElement::InvalidateImplicitStyles(pVisualOwner, pOldResources));
    }
    else
    {
        // if owner is not CUIElement try to see whether is an Application
        CApplication * pApplication = do_pointer_cast<CApplication>(m_pResourceOwner);
        auto core = GetContext();

        if (pApplication && core)
        {
            // If the owner is the Application, invalidate all implicit styles
            IFC_RETURN(core->InvalidateImplicitStylesOnRoots(pOldResources));
        }
    }
    return S_OK;
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//      The virtual method which does the tree walk to clean up all
//      the device related resources like brushes, textures,
//      primitive composition data etc. in this subgraph.
//
//-----------------------------------------------------------------------------
void CResourceDictionary::CleanupDeviceRelatedResourcesRecursive(_In_ bool cleanupDComp)
{
    CDOCollection::CleanupDeviceRelatedResourcesRecursive(cleanupDComp);

    // Cleanup the dictionary entries
    for (auto& value : m_resourceMap)
    {
        value.second->CleanupDeviceRelatedResourcesRecursive(cleanupDComp);
    }
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//      A theme dictionary has been added or removed from the ThemeDictionaries
//  collection.
//
//-----------------------------------------------------------------------------

_Check_return_ HRESULT
CResourceDictionary::OnThemeDictionariesChanged()
{
    // Remove active theme dictionary, because the theme dictionaries collection
    // has changed, and a better candidate for the active theme dictionary
    // may be available.
    if (m_pActiveThemeDictionary)
    {
        m_pActiveThemeDictionary = nullptr;
    }
    m_activeTheme = Theming::Theme::None;

    RRETURN(S_OK);
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//     Notify parent resource dictionary that ThemeDictionaries collection
//  has changed.
//
//-----------------------------------------------------------------------------

_Check_return_ HRESULT
CResourceDictionary::NotifyThemeDictionariesChanged()
{
    CDependencyObject *pParent = GetParentInternal(false);
    CResourceDictionary *pParentResourceDictionary = NULL;

    ASSERT(m_bIsThemeDictionaries);

    IFC_RETURN(DoPointerCast(pParentResourceDictionary, pParent));
    if (pParentResourceDictionary)
    {
        IFC_RETURN(pParentResourceDictionary->OnThemeDictionariesChanged());
    }

    return S_OK;
}

KnownTypeIndex CStaticResourceExtension::GetTypeIndex() const
{
    return DependencyObjectTraits<CStaticResourceExtension>::Index;
}

_Check_return_ HRESULT
CStaticResourceExtension::ProvideValue(
    _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
    _Out_ std::shared_ptr<XamlQualifiedObject>& qoValue)
{
    CDependencyObject* pObjectNoRef = NULL;
    auto core = GetContext();

    IFC_RETURN(LookupResourceNoRef(m_strResourceKey, spServiceProviderContext, core, &pObjectNoRef, TRUE /* bShouldCheckThemeResources */));

    // If we can't find an object that corresponds to the key, raise an error
    // like "Cannot find a Resource with the Name/Key %0".
    if (!pObjectNoRef)
    {
        // Rerun the search with logging enabled
        xstring_ptr traceMessage;
        {
            Diagnostics::ResourceLookupLogger* loggerNoRef = core->GetResourceLookupLogger();

            auto cleanupGuard = wil::scope_exit([&]
            {
                TRACE_HR_NORETURN(loggerNoRef->Stop(m_strResourceKey, traceMessage));
            });

            xstring_ptr currentUri;
            if (const auto baseUri = spServiceProviderContext->GetBaseUri())
            {
                IGNOREHR(UriXStringGetters::GetPath(baseUri, &currentUri));
            }

            IFC_RETURN(loggerNoRef->Start(m_strResourceKey, currentUri));
            IFC_RETURN(LookupResourceNoRef(m_strResourceKey, spServiceProviderContext, core, &pObjectNoRef, TRUE /* bShouldCheckThemeResources */));
        }

        // Record the message in an error context
        std::vector<std::wstring> extraInfo;
        extraInfo.push_back(std::wstring(traceMessage.GetBuffer()));

        IFC_RETURN_EXTRA_INFO(CErrorService::OriginateInvalidOperationError(
                core,
                AG_E_PARSER_FAILED_RESOURCE_FIND,
                m_strResourceKey),
            &extraInfo);
    }

    {
        std::shared_ptr<XamlQualifiedObject> qo;
        if (!do_pointer_cast<CNullKeyedResource>(pObjectNoRef))
        {
            XamlTypeToken typeToken;

            IFC_RETURN(XamlNativeRuntime::GetTypeTokenForDO(pObjectNoRef, typeToken));
            // TODO: move this setting of the TypeToken some where central.

            qo = std::make_shared<XamlQualifiedObject>(typeToken);
            IFC_RETURN(qo->SetDependencyObject(pObjectNoRef));

            // Resources are special, in that Poco objects are actually stored on the managed side. If this
            // is a Poco object, it will consequently be a ManagedObjectReference and pegged.  So we need
            // to set the corresponding flag in the XQO.

            if (pObjectNoRef->OfTypeByIndex<KnownTypeIndex::ExternalObjectReference>())
            {
                qo->SetHasPeggedManagedPeer();
            }
        }
        else
        {
            // The resource key resolving to a CNullKeyedResource indicates that the resource is
            // null-valued. Create a XamlQualifiedObject representing 'null'
            std::shared_ptr<XamlSchemaContext> schemaContext;
            IFC_RETURN(spServiceProviderContext->GetSchemaContext(schemaContext));
            qo = schemaContext->get_NullKeyedResourceXQO();
        }

        qoValue = std::move(qo);
    }

    return S_OK;
}

_Check_return_ HRESULT
CStaticResourceExtension::LookupResourceNoRef(
    _In_ const xstring_ptr& strKey,
    _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
    _In_ CCoreServices *pCore,
    _Outptr_result_maybenull_ CDependencyObject** ppObject,
    _In_ bool bShouldCheckThemeResources,
    _In_opt_ CStyle* optimizedStyleParent,
    _In_ KnownPropertyIndex stylePropertyIndex
    )
{
    Resources::ResolvedResource resource;
    IFC_RETURN(ResourceResolver::ResolveStaticResource(
        strKey,
        spServiceProviderContext,
        pCore,
        !!bShouldCheckThemeResources,
        resource,
        optimizedStyleParent,
        stylePropertyIndex));

    *ppObject = resource.Value.get();

    return S_OK;
}

void CStaticResourceExtension::Reset()
{
    m_strResourceKey.Reset();
}

CustomResourceExtension::CustomResourceExtension(_In_ CCoreServices *pCore) : CMarkupExtensionBase(pCore)
{
}

CustomResourceExtension::~CustomResourceExtension()
{
}

KnownTypeIndex CustomResourceExtension::GetTypeIndex() const
{
    return DependencyObjectTraits<CustomResourceExtension>::Index;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Provides the property value by calling out to an implementation of ICustomResourceLoader.
//
//  Notes:
//      If no custom resource loader is set, fails with AG_E_PARSER_NO_CUSTOM_RESOURCE_LOADER.
//
//---------------------------------------------------------------------------
_Check_return_
HRESULT CustomResourceExtension::ProvideValue(
    _In_ const std::shared_ptr<XamlServiceProviderContext> &spServiceProviderContext,
    _Out_ std::shared_ptr<XamlQualifiedObject> &qoValue
    )
{
    CValue value;
    XamlTypeToken typeToken;
    auto core = GetContext();
    ICustomResourceLoader *pResourceLoader = core->GetCustomResourceLoader();
    std::shared_ptr<XamlSchemaContext> spSchemaContext;
    std::shared_ptr<XamlProperty> spXamlProperty;
    std::shared_ptr<XamlQualifiedObject> spTargetObject;
    std::shared_ptr<XamlType> spObjectType;
    std::shared_ptr<XamlType> spPropertyType;
    std::shared_ptr<XamlQualifiedObject> qo;
    xstring_ptr spPropertyName;
    xstring_ptr spPropertyTypeName;
    xstring_ptr spTypeName;

    if (!pResourceLoader)
    {
        IFC_RETURN(CErrorService::OriginateInvalidOperationError(
                core,
                AG_E_PARSER_NO_CUSTOM_RESOURCE_LOADER,
                m_strResourceKey));
    }

    spServiceProviderContext->GetMarkupExtensionTargetObject(spTargetObject);
    IFCEXPECT_RETURN(spTargetObject);
    typeToken = spTargetObject->GetTypeToken();

    IFC_RETURN(spServiceProviderContext->GetSchemaContext(spSchemaContext));
    IFC_RETURN(spSchemaContext->GetXamlType(typeToken, spObjectType));
    IFC_RETURN(spObjectType->get_FullName(&spTypeName));

    spServiceProviderContext->GetMarkupExtensionTargetProperty(spXamlProperty);
    IFCEXPECT_RETURN(spXamlProperty);

    IFC_RETURN(spXamlProperty->get_Name(&spPropertyName));
    IFC_RETURN(spXamlProperty->get_Type(spPropertyType));
    IFC_RETURN(spPropertyType->get_FullName(&spPropertyTypeName));

    IFC_RETURN(pResourceLoader->GetResource(m_strResourceKey, spTypeName, spPropertyName, spPropertyTypeName, &value));

    qo = std::make_shared<XamlQualifiedObject>(spPropertyType->get_TypeToken());
    IFC_RETURN(qo->SetValue(value));

    qoValue = std::move(qo);

    return S_OK;
}

_Check_return_
HRESULT CustomResourceExtension::ProvideValueByPropertyIndex(
    const KnownPropertyIndex propIndex,
    _Out_ CValue& value
) const
{
    auto core = GetContext();
    ICustomResourceLoader *pResourceLoader = core->GetCustomResourceLoader();

    if (!pResourceLoader)
    {
        IFC_RETURN(CErrorService::OriginateInvalidOperationError(
            core,
            AG_E_PARSER_NO_CUSTOM_RESOURCE_LOADER,
            m_strResourceKey));
    }

    auto depProp = DirectUI::MetadataAPI::GetDependencyPropertyByIndex(propIndex);
    xstring_ptr spPropertyName = depProp->GetName();
    xstring_ptr spPropertyTypeName = depProp->GetPropertyType()->GetFullName();
    xstring_ptr spTypeName = depProp->GetTargetType()->GetFullName();

    IFC_RETURN(pResourceLoader->GetResource(m_strResourceKey, spTypeName, spPropertyName, spPropertyTypeName, &value));

    return S_OK;
}


//---------------------------------------------------------------------------
//
//  Synopsis:
//      Expands any non deferrable keys if the ResourceDictionary
//      is being deferred.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT
CResourceDictionary::Ensure(_In_ ObjectWriter *pOriginalWriter)
{
    if (m_pRuntimeDeferredKeys)
    {
        IFC_RETURN(m_pRuntimeDeferredKeys->PlayNonDeferredContent(pOriginalWriter));
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Expands all items and stops any delay loading of keys.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT
CResourceDictionary::EnsureAll()
{
    if (m_pRuntimeDeferredKeys)
    {
        IFC_RETURN(m_pRuntimeDeferredKeys->EnsureAll());
        ASSERT(m_pRuntimeDeferredKeys->IsEmpty());
        m_pRuntimeDeferredKeys.reset();
    }

    return S_OK;
}

_Check_return_ HRESULT
CResourceDictionary::ShouldDeferNodeStream(
    _In_ const std::shared_ptr<XamlOptimizedNodeList>& spNodeList,
    _Out_ bool *pfShouldDefer,
    _Out_ bool *pfHasNonDeferrableContent
    )
{
    HRESULT hr = S_OK;

    XUINT32 uiNodeDepth        = 0;
    XUINT32 totalItems         = 0;
    XUINT32 totalKeyedItems    = 0;
    bool fKeyPropertyFound = false;
    bool fNamePropertyFound = false;

    std::shared_ptr<XamlReader>           spReader;
    std::shared_ptr<XamlSchemaContext>    spSchemaContext;
    std::shared_ptr<DirectiveProperty>         spKeyProperty;
    std::shared_ptr<DirectiveProperty>         spNameProperty;

    *pfShouldDefer = FALSE;
    *pfHasNonDeferrableContent = FALSE;

    // if the NodeList is less than 128 nodelist elements, there is not much use in deferring, so skip it.
    if (spNodeList->get_CountForOldDictionaryDeferral() < 128)
    {
        goto Cleanup;
    }

    IFC(spNodeList->get_Reader(spReader));
    IFC(spReader->GetSchemaContext(spSchemaContext));
    IFC(spSchemaContext->get_X_KeyProperty(spKeyProperty));
    IFC(spSchemaContext->get_X_NameProperty(spNameProperty));

    // find the count of keyed items in the resource dictionary
    //
    // we don't consider implicit template items or items with x:Name as they
    // have side effects on the structure of the ResourceDcitionary
    // that will break lookup logic if we choose to defer
    while ((hr = spReader->Read()) == S_OK)
    {
        switch (spReader->CurrentNode().get_NodeType())
        {
            case XamlNodeType::xntStartObject:
                {
                    if (uiNodeDepth == 0)
                    {
                        totalItems++;
                        fKeyPropertyFound = FALSE;
                        fNamePropertyFound = FALSE;
                    }

                    uiNodeDepth++;
                }
                break;

            case XamlNodeType::xntEndObject:
                {
                    uiNodeDepth--;
                    if (uiNodeDepth == 0)
                    {

                        if (fKeyPropertyFound && !fNamePropertyFound)
                        {
                            totalKeyedItems++;
                        }
                    }
                }
                break;

            case XamlNodeType::xntStartProperty:
                {
                    if (uiNodeDepth == 1)
                    {
                        XamlNode currentXamlNode = spReader->CurrentNode();

                        if (XamlProperty::AreEqual(spKeyProperty, currentXamlNode.get_Property()))
                        {
                            fKeyPropertyFound = TRUE;
                        }
                        else if (XamlProperty::AreEqual(spNameProperty, currentXamlNode.get_Property()))
                        {
                            fNamePropertyFound = TRUE;
                        }
                    }
                }
                break;

            default:
                break;
        }
    }

    ASSERT( totalItems >= totalKeyedItems );
    // defer only if at least 50% are items by key
    if ( totalKeyedItems >= (totalItems-totalKeyedItems) )
    {
        *pfShouldDefer = TRUE;
    }

    if (totalItems-totalKeyedItems > 0)
    {
        *pfHasNonDeferrableContent = TRUE;
    }

Cleanup:
    return hr;
}

_Check_return_ HRESULT
CResourceDictionary::DeferKeysAsXaml(
    _In_ const bool fIsResourceDictionaryWithKeyProperty,
    _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
    _In_ const std::shared_ptr<XamlOptimizedNodeList>& spNodeList,
    _In_ const std::shared_ptr<XamlSavedContext>& spContext
    )
{
    ASSERT(!m_pRuntimeDeferredKeys);
    m_pRuntimeDeferredKeys = std::unique_ptr<CDeferredKeys>(new CDeferredKeys(this));

    IFC_RETURN(m_pRuntimeDeferredKeys->RecordXaml(spServiceProviderContext, spNodeList, spContext, fIsResourceDictionaryWithKeyProperty));

    return S_OK;
}

_Check_return_ HRESULT
CResourceDictionary::SetCustomWriterRuntimeData(
    _In_ std::shared_ptr<CustomWriterRuntimeData> data,
    _In_ std::unique_ptr<CustomWriterRuntimeContext> context)
{
    ASSERT(!m_pDeferredResources);

    m_pDeferredResources = std::unique_ptr<CResourceDictionary2>(new CResourceDictionary2());

    IFC_RETURN(m_pDeferredResources->SetCustomWriterRuntimeData(std::static_pointer_cast<ResourceDictionaryCustomRuntimeData>(data), std::move(context)));

    m_nImplicitStylesCount += m_pDeferredResources->GetInitialImplicitStyleKeyCount();

    // We need to load deferred resources with an x:Name in order to make sure FindName() can
    // discover them. Some apps (like in-box Calculator) create named resources that are then accessed
    // as via the code-genned fields in code-behind.
    for (const auto& key : m_pDeferredResources->GetInitialResourcesWithXNames())
    {
        CDependencyObject* pResource = nullptr;

        ResourceKey resourceKey(key, false);

        // Check that the resource isn't already undeferred. It could've
        // been added as a side effect of undeferring another resource,
        // e.g. when we pre-resolve references inside templates.
        if (FindResourceByKey(resourceKey) == nullptr)
        {
            bool undeferring = false;

            // Resources with an x:Name can never be implicitly keyed
            IFC_RETURN(TryLoadDeferredResource(resourceKey, undeferring, &pResource));

            if (EventEnabledResourceUsingXNameInfo() && pResource)
            {
                TraceResourceUsingXNameInfo(
                    reinterpret_cast<UINT64>(pResource),
                    reinterpret_cast<UINT64>(this),
                    key.GetBuffer(),
                    pResource->GetClassName().GetBuffer()
                    );
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
CResourceDictionary::TryLoadDeferredResource(
    const ResourceKey& key,
    _Out_ bool& undeferring,
    _Out_ CDependencyObject** ppResource)
{
    *ppResource = nullptr;
    undeferring = false;

    // Presumably, we're only here because we tried to look up this key and failed. Let's avoid
    // searching for the key again in fre, but validate that this case is true in chk
    ASSERT(FindResourceByKey(key) == nullptr);

    // Check that we're not already in the process of loading a resource with the same key.
    // Just return if we are, so the lookup continues up the chain - we support cases
    // where a style is based on another style with the same key in inherited resources.
    // There will be a "cannot find resource" error if the lookup ultimately fails.

    auto found = std::find_if(
        m_undeferringResources.begin(),
        m_undeferringResources.end(),
        [&](const auto& entry)
        {
            return key == entry;
        });

    if (found != m_undeferringResources.end())
    {
        undeferring = true;
        return S_OK;
    }

    ResourceKeyStorage keyStorage = key.ToStorage();

    // Load the resource. Temporarily add the key with a dummy value in our store,
    // so we can check above for reentrancy that attempts to lookup the same key,
    // e.g. a style with BasedOn set to its own key.
    m_undeferringResources.push_back(keyStorage);

    auto guard = wil::scope_exit([this, &keyStorage]
    {
        auto newEnd = std::remove(m_undeferringResources.begin(), m_undeferringResources.end(), keyStorage);
        m_undeferringResources.erase(newEnd, m_undeferringResources.end());
    });

    bool keyFound = false;
    std::shared_ptr<CDependencyObject> resource;

    IFC_RETURN(m_pDeferredResources->LoadValueIfExists(keyStorage.GetKey(), keyStorage.IsKeyType(), keyFound, resource));

    // Notice how this method behaves- if the key fails to parse it throws a bad HRESULT, if
    // the key simply doesn't exist it returns S_OK with the 'keyFound' parameter set to false.
    if (!keyFound)
    {
        return S_OK;
    }

    // We support null-valued resources, so wrap the returned null in a NullKeyedResource
    CValue value;

    if (resource)
    {
        *ppResource = resource.get();
        value.Wrap<valueObject>(resource.get());
    }
    else
    {
        *ppResource = DirectUI::DXamlCore::GetCurrent()->GetCachedNullKeyedResource()->GetHandle();
        value.SetAddRef<valueObject>(*ppResource);    // add-ref to keep alive after ComPtr goes out of scope
    }

    IFC_RETURN(Add(key, &value, nullptr, false, true, true));

    return S_OK;
}

_Check_return_ HRESULT
CResourceDictionary::LoadAllDeferredResources()
{
    if (m_pDeferredResources)
    {
        TraceFaultInBehaviorBegin(L"ResourceDictionary");

#if DBG
        // ASSERT visible to Prefast fre build but references DBG-only variables
        ASSERT(!m_processingBulkUndeferral);
        m_processingBulkUndeferral = true;
        auto resetBulkUndeferral = wil::scope_exit([&] {
            m_processingBulkUndeferral = false;
        });
#endif

        std::vector<std::pair<xstring_ptr, std::shared_ptr<CDependencyObject>>> implicitResources;
        std::vector<std::pair<xstring_ptr, std::shared_ptr<CDependencyObject>>> explicitResources;

        m_keyByIndex.reserve(m_pDeferredResources->size() - m_undeferredKeyCount);
        m_resourceMap.reserve(m_pDeferredResources->size() - m_undeferredKeyCount);

        // DeferredResource loading is a retryable process in Jupiter. If either:
        // - One of the keys fail to load due to runtime object instantiation failure
        // - One of the key adds fails due to a duplicate runtime key or other CCollection::Add
        //   failure.
        // we allow the process to be retried.
        //
        // In practice because we do appropriate checks for collision at key-add there
        // should NEVER be a bad HRESULT thrown from an Add operation, I'd go as
        // far as making it FAIL_FAST here, but I don't claim to know all the ways DO Enter
        // can fail and we could end up doing some nontrivial amount of work there.
        IFC_RETURN(m_pDeferredResources->LoadAllRemainingDeferredResources(
            m_resourceMap,
            implicitResources, explicitResources));

        // Imagine the following XAML. If both these resources have not been undeferred at this point. Then the undeferal of 'Icon' will
        // actually result in the current dictionary undefering IconConverter and adding it to the current dictionary. We don't necessarily
        // want to prevent the undefering that happens here because the TemplateContent needs the value and dictionary found in for pre-resolving
        // resources.
        /*
            <local:IconConverter x:Key="IconConverter"></local:IconConverter>
            <Style x:Key="Icon" TargetType="ContentControl">
                <Setter Property="Template">
                    <Setter.Value>
                        <ControlTemplate TargetType="ContentControl">
                            <Viewbox Stretch="Uniform">
                                <Grid>
                                    <TextBlock FontFamily="Segoe MDL2 Assets" Text="{Binding Content, RelativeSource={RelativeSource TemplatedParent}, Converter={StaticResource IconConverter}}" />
                                </Grid>
                            </Viewbox>
                        </ControlTemplate>
                    </Setter.Value>
                </Setter>
            </Style>
        */
        for (const auto& kvp : implicitResources)
        {
            ResourceKey key(kvp.first, true);

            // Make sure key is not in map
            if (FindResourceByKey(key) == nullptr)
            {
                CValue value;
                value.SetObjectAddRef(kvp.second.get());
                IFC_RETURN(Add(key, &value, nullptr, false, true, false));
            }
        }

        for (const auto& kvp : explicitResources)
        {
            ResourceKey key(kvp.first, false);

            // Make sure key is not in map
            if (FindResourceByKey(key) == nullptr)
            {
                CValue value;
                value.SetObjectAddRef(kvp.second.get());
                IFC_RETURN(Add(key, &value, nullptr, false, true, false));
            }
        }

        TraceFaultInBehaviorEnd();
    }

    m_pDeferredResources.reset();
    return S_OK;
}

bool CResourceDictionary::AllowsResourceOverrides() const
{
    return OfTypeByIndex<KnownTypeIndex::ColorPaletteResources>();
}

bool CResourceDictionary::HasPotentialOverrides() const
{
    if (AllowsResourceOverrides())
    {
        return true;
    }

    if (m_pThemeDictionaries)
    {
        for (auto dictionary : *m_pThemeDictionaries)
        {
            if (checked_cast<CResourceDictionary>(dictionary)->HasPotentialOverrides())
            {
                return true;
            }
        }
    }

    if (m_pMergedDictionaries)
    {
        for (auto dictionary : *m_pMergedDictionaries)
        {
            if (checked_cast<CResourceDictionary>(dictionary)->HasPotentialOverrides())
            {
                return true;
            }
        }
    }

    return false;
}

_Check_return_ HRESULT CResourceDictionary::GetLocalOverrideNoRef(
    const xstring_ptr_view& strKey,
    _Outptr_result_maybenull_ CDependencyObject** resultDO,
    _Outptr_result_maybenull_ CResourceDictionary** resultDict)
{
    IFC_RETURN(EnsureActiveThemeDictionary());

    // If in HC, override only if it's a HC theme dictionary

    if (AllowsResourceOverrides() &&
        (IsHighContrast() || GetContext()->GetFrameworkTheming()->GetHighContrastTheme() == Theme::None))
    {
        ResourceKey key(strKey, false);

        *resultDO = FindResourceByKey(key);

        if (!*resultDO)
        {
            bool unused = false;

            IFC_RETURN(FindDeferredResource(
                key,
                unused,
                resultDO));
        }

        if (*resultDO)
        {
            *resultDict = this;
            return S_OK;
        }
    }

    if (m_pMergedDictionaries)
    {
        xref_ptr<CResourceDictionary> dictionary;

        for (XINT32 i = m_pMergedDictionaries->GetCount() - 1; i >= 0; --i)
        {
            dictionary.attach(static_cast<CResourceDictionary*>(m_pMergedDictionaries->GetItemWithAddRef(i)));

            IFC_RETURN(dictionary->GetLocalOverrideNoRef(
                strKey,
                resultDO,
                resultDict));

            if (*resultDO)
            {
                return S_OK;
            }
        }
    }

    if (m_pActiveThemeDictionary)
    {
        IFC_RETURN(m_pActiveThemeDictionary->GetLocalOverrideNoRef(
            strKey,
            resultDO,
            resultDict));
    }

    return S_OK;
}

_Check_return_ HRESULT CResourceDictionary::GetKeyWithOverrideNoRef(
    const xstring_ptr_view& keyName,
    Resources::LookupScope resourceLookupScope,
    _Outptr_ CDependencyObject **ppDO)
{
    xref_ptr<CResourceDictionary> dictionaryResourcesReadFrom;

    IFC_RETURN(GetKeyForResourceResolutionNoRef(
        keyName,
        resourceLookupScope,
        ppDO,
        &dictionaryResourcesReadFrom));

    if (AllowsResourceOverrides())
    {
        CDependencyObject* overrideValue = nullptr;

        IFC_RETURN(ScopedResources::TryGetOrCreateOverrideForDictionary(
            this,
            *ppDO,
            dictionaryResourcesReadFrom.get(),
            keyName,
            &overrideValue,
            nullptr));

        if (overrideValue)
        {
            *ppDO = overrideValue;
        }
    }

    return S_OK;
}

void CResourceDictionary::MarkAllIsGlobal()
{
    MarkIsGlobal();

    if (m_pThemeDictionaries)
    {
        m_pThemeDictionaries->MarkAllIsGlobal();

        for (auto dictionary : *m_pThemeDictionaries)
        {
            static_cast<CResourceDictionary*>(dictionary)->MarkAllIsGlobal();
        }
    }

    if (m_pMergedDictionaries)
    {
        for (auto dictionary : *m_pMergedDictionaries)
        {
            static_cast<CResourceDictionary*>(dictionary)->MarkAllIsGlobal();
        }
    }
}

_Check_return_ HRESULT CResourceDictionary::NotifyThemeChangedCore(
    Theming::Theme theme,
    bool forceRefresh)
{
    if (AllowsResourceOverrides() &&
        !IsThemeDictionary() &&
        !m_keyByIndex.empty())
    {
        // Purge generated instances in non-theme override dictionary.
        // Only non-theme dictionaries need to be purged, since theme
        // dictionaries will contain resources for the right theme, while
        // plain resource dictionary can contain theme-dependent resources.

        for (int index = m_keyByIndex.size() - 1; index >= 0; --index)
        {
            if (m_keyByIndex[index].second.m_isOverride)
            {
                xref_ptr<CDependencyObject> removed;
                removed.attach(static_cast<CDependencyObject*>(RemoveAt(index)));
            }
        }
    }

    InvalidateNotFoundCache(false);

    IFC_RETURN(__super::NotifyThemeChangedCore(theme, forceRefresh));

    return S_OK;
}

CResourceDictionary* GetParentDictionaryHelper(_In_ CResourceDictionary* current)
{
    // Get immediate parent dictionary of the current one.  If it's contained in a collection,
    // we need to skip over it.

    if (CDependencyObject* parent = current->GetParentInternal(false))
    {
        if (parent->OfTypeByIndex<KnownTypeIndex::ResourceDictionaryCollection>())
        {
            parent = parent->GetParentInternal(false);
        }

        if (CResourceDictionary* parentAsResourceDictionary = do_pointer_cast<CResourceDictionary>(parent))
        {
            return parentAsResourceDictionary;
        }
    }

    return nullptr;
}

void CResourceDictionary::InvalidateNotFoundCache(bool propagate)
{
    if (propagate)
    {
        // Traverse dictionary sub-tree iteratively as it has less overhead.

        CResourceDictionary* current = this;

        while (current)
        {
            if (current->m_keysNotFoundCache)
            {
                current->m_keysNotFoundCache->Clear();
            }

            current = GetParentDictionaryHelper(current);
        }
    }
    else
    {
        if (m_keysNotFoundCache)
        {
            m_keysNotFoundCache->Clear();
        }
    }
}

void CResourceDictionary::InvalidateNotFoundCache(bool propagate, const ResourceKey& key)
{
    if (propagate)
    {
        // Traverse dictionary sub-tree iteratively as it has less overhead.

        CResourceDictionary* current = this;

        while (current)
        {
            if (current->m_keysNotFoundCache)
            {
                current->m_keysNotFoundCache->Remove(key);
            }

            current = GetParentDictionaryHelper(current);
        }
    }
    else
    {
        if (m_keysNotFoundCache)
        {
            m_keysNotFoundCache->Remove(key);
        }
    }
}

KnownTypeIndex CColorPaletteResources::GetTypeIndex() const
{
    return DependencyObjectTraits<CColorPaletteResources>::Index;
}

_Check_return_ HRESULT CColorPaletteResources::GetColor(
    KnownPropertyIndex propertyIndex,
    _Out_ CValue& result)
{
    xstring_ptr overrideKey = ScopedResources::GetOverrideKey(propertyIndex);
    ASSERT(!overrideKey.IsNullOrEmpty());

    CDependencyObject* found = nullptr;

    IFC_RETURN(GetKeyNoRef(
        overrideKey,
        Resources::LookupScope::LocalOnly,
        &found));

    CManagedObjectReference* mor = do_pointer_cast<CManagedObjectReference>(found);

    if (mor && !mor->m_nativeValue.IsNull())
    {
        IFC_RETURN(result.CopyConverted(mor->m_nativeValue));
    }
    else if (found && !mor)
    {
        result.SetAddRef<valueObject>(found);
    }
    else
    {
        result.Set<valueNull>(nullptr);
    }

    return S_OK;
}

_Check_return_ HRESULT CColorPaletteResources::SetColor(
    KnownPropertyIndex propertyIndex,
    const CValue& value)
{
    xstring_ptr overrideKey = ScopedResources::GetOverrideKey(propertyIndex);
    ASSERT(!overrideKey.IsNullOrEmpty());

    // Key might not exist, but we don't want to check and then remove to reduce number of lookups.
    // Therefore, ignore failures.
    IGNOREHR(Remove(overrideKey));

    CREATEPARAMETERS cp(GetContext(), value);
    xref_ptr<CDependencyObject> color;
    IFC_RETURN(CColor::Create(color.ReleaseAndGetAddressOf(), &cp));

    CValue colorValue;
    colorValue.Wrap<valueObject>(color.get());

    IFC_RETURN(Add(overrideKey, &colorValue, nullptr, false));

    return S_OK;
}

bool CResourceDictionary::HasKey(const xstring_ptr& key, bool isImplicitKey) const
{
    bool hasKey = false;
    if (m_pDeferredResources)
    {
        hasKey = m_pDeferredResources->ContainsKey(key, isImplicitKey);
    }

    if (!hasKey)
    {
        size_t index = 0;
        while (!hasKey && index < m_keyByIndex.size())
        {
            if (m_keyByIndex[index].second.m_isType == isImplicitKey)
            {
                hasKey = key.Equals(m_keyByIndex[index].first);
            }
            ++index;
        }
    }

    return hasKey;
}

namespace Resources::details
{
    bool ResourceKeyCache::Exists(const ResourceKey& key)
    {
        auto end = m_cache.begin() + m_count;

        auto found = std::find_if(
            m_cache.begin(),
            end,
            [&key](const ResourceKeyStorage& entry)
            {
                return key == entry;
            });

        if (found != end)
        {
            // If element was found, promote it to the most-recently-used position.
            ShiftRight(found - m_cache.begin());
            return true;
        }
        else
        {
            return false;
        }
    }

    void ResourceKeyCache::Add(const ResourceKey& key)
    {
        ResourceKeyStorage keyStorage = key.ToStorage();

        ShiftRight();
        m_cache[0] = std::move(keyStorage);

        if (m_count < c_cacheSize)
        {
            ++m_count;
        }
    }

    void ResourceKeyCache::Remove(const ResourceKey& key)
    {
        auto end = std::remove_if(
            m_cache.begin(),
            m_cache.begin() + m_count,
            [&key](const ResourceKeyStorage& entry)
            {
                return key == entry;
            });

        m_count = end - m_cache.begin();
    }

    void ResourceKeyCache::Clear()
    {
        m_count = 0;
    }

    void ResourceKeyCache::ShiftRight(uint32_t index)
    {
        // Since we want to minimize incurred penalty from using a cache,
        // these loops are hand-unrolled to avoid conditionals.

        static_assert(c_cacheSize == 8, "Update this method to match cache size.");
        ASSERT(index < c_cacheSize);

        ResourceKeyStorage first = std::move(m_cache[index]);

        switch (7 - index)
        {
            case 0: m_cache[7] = std::move(m_cache[6]);
            case 1: m_cache[6] = std::move(m_cache[5]);
            case 2: m_cache[5] = std::move(m_cache[4]);
            case 3: m_cache[4] = std::move(m_cache[3]);
            case 4: m_cache[3] = std::move(m_cache[2]);
            case 5: m_cache[2] = std::move(m_cache[1]);
            case 6: m_cache[1] = std::move(m_cache[0]);
            case 7: m_cache[0] = std::move(first);
        }
    }

    void ResourceKeyCache::ShiftRight()
    {
        static_assert(c_cacheSize == 8, "Update this method to match cache size.");

        m_cache[7] = std::move(m_cache[6]);
        m_cache[6] = std::move(m_cache[5]);
        m_cache[5] = std::move(m_cache[4]);
        m_cache[4] = std::move(m_cache[3]);
        m_cache[3] = std::move(m_cache[2]);
        m_cache[2] = std::move(m_cache[1]);
        m_cache[1] = std::move(m_cache[0]);
    }
}