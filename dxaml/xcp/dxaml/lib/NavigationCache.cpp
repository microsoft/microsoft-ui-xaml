// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Asynchronously loads frame content and caches it. Uses two caches:
//      one for permanent caching, and one for transient caching. The size
//      of the transient cache is bounded, and the size of the permanent
//      cache is not. Loading can be canceled and performed again
//      synchronously.
//  Notes:
//      Transient caching uses a least-recently-used algorithm to
//      determine which content to replace.

#include "precomp.h"
#include "NavigationCache.h"
#include "PageStackEntry.g.h"

#pragma warning(disable:4267) //'var' : conversion from 'size_t' to 'type', possible loss of data

using namespace DirectUI;
using namespace DirectUISynonyms;
using namespace xaml_markup;

namespace N = xaml::Navigation;

NavigationCache::NavigationCache() :
    m_pIFrame(NULL),
    m_transientCacheSize(0)
{
}

NavigationCache::~NavigationCache()
{
    m_pIFrame = NULL;
}

_Check_return_ HRESULT
NavigationCache::Create(
    xaml_controls::IFrame *pIFrame,
    _In_ UINT transientCacheSize,
    _Outptr_ NavigationCache **ppNavigationCache)
{
    HRESULT hr = S_OK;
    IInspectable *pIInspectable = NULL;
    NavigationCache *pNavigationCache = NULL;

    IFCPTR(ppNavigationCache);
    *ppNavigationCache = NULL;

    IFCPTR(pIFrame);

    pNavigationCache = new NavigationCache();

    pNavigationCache->m_transientCacheSize = transientCacheSize;
    pNavigationCache->m_pIFrame = pIFrame;

    *ppNavigationCache = pNavigationCache;

Cleanup:
    ReleaseInterface(pIInspectable);


    pNavigationCache = NULL;

    RRETURN(hr);
}

_Check_return_ HRESULT
NavigationCache::GetContent(
    _In_ PageStackEntry *pPageStackEntry,
    _Outptr_ IInspectable **ppIInspectable)
{
    HRESULT hr = S_OK;
    IPage *pIPage = NULL;
    BOOLEAN shouldCache = FALSE;
    BOOLEAN found = FALSE;
    wrl_wrappers::HString strDescriptor;
    IInspectable *pIInspectable = NULL;
    N::NavigationCacheMode navigationCacheMode = N::NavigationCacheMode_Disabled;

    IFCPTR(ppIInspectable);
    *ppIInspectable = NULL;

    IFCPTR(pPageStackEntry);

    IFC(pPageStackEntry->GetDescriptor(strDescriptor.GetAddressOf()));
    IFCPTR(strDescriptor.Get());
    IFC(GetCachedContent(strDescriptor.Get(), &pIInspectable, &found));

    if (!found)
    {
        IFC(LoadContent(strDescriptor.Get(), &pIInspectable));
        IFCPTR(pIInspectable);

        pIPage = ctl::query_interface<IPage>(pIInspectable);

        if (pIPage)
        {
            IFC(pIPage->get_NavigationCacheMode(&navigationCacheMode));
        }

        shouldCache = m_transientCacheSize >= 1 && navigationCacheMode != N::NavigationCacheMode_Disabled;

        if (shouldCache)
        {
            IFC(CacheContent(navigationCacheMode, strDescriptor.Get(), pIInspectable));
        }
    }

    IFC(pPageStackEntry->PrepareContent(pIInspectable));

    *ppIInspectable = pIInspectable;
    pIInspectable = NULL;

    TraceNavigationCacheGetContentInfo(WindowsGetStringRawBuffer(strDescriptor.Get(), NULL), found);

Cleanup:
    ReleaseInterface(pIPage);
    ReleaseInterface(pIInspectable);

    RRETURN(hr);
}

_Check_return_ HRESULT
NavigationCache::LoadContent(
    _In_ HSTRING descriptor,
    _Outptr_ IInspectable** ppInstance)
{
    HRESULT hr = S_OK;
    const CClassInfo* pType = nullptr;

    IFCPTR(descriptor);

    IFCPTR(ppInstance);
    *ppInstance = NULL;

    IFC(MetadataAPI::GetClassInfoByFullName(
        XSTRING_PTR_EPHEMERAL_FROM_HSTRING(descriptor),
        &pType));

    IFC(ActivationAPI::ActivateInstance(pType, ppInstance));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
NavigationCache::GetCachedContent(
    _In_ HSTRING descriptor,
    _Outptr_ IInspectable **ppIInspectable,
    _Out_ BOOLEAN *pFound)
{
    HRESULT hr = S_OK;
    BOOLEAN found = FALSE;
    IInspectable *pIInspectable = NULL;
    StringListType::iterator listIterator;

    *ppIInspectable = NULL;
    *pFound = FALSE;

    IFCPTR(descriptor);

    IFC(m_permanentMap.HasKey(descriptor, &found));

    if (found)
    {
        IFC(m_permanentMap.Lookup(descriptor, &pIInspectable));
    }
    else
    {
        IFC(m_transientMap.HasKey(descriptor, &found));

        if (found)
        {
            IFC(m_transientMap.Lookup(descriptor, &pIInspectable));

            // Make the descriptor the most recently used.
            for (listIterator = m_transientCacheMruList.begin(); listIterator != m_transientCacheMruList.end(); ++listIterator)
            {
                if (descriptor == *listIterator)
                {
                    m_transientCacheMruList.splice(m_transientCacheMruList.end(), m_transientCacheMruList, listIterator);
                    break;
                }
            }
        }
    }

    *ppIInspectable = pIInspectable;
    pIInspectable = NULL;

    *pFound = found;

Cleanup:
    ReleaseInterface(pIInspectable);

    RRETURN(hr);
}

_Check_return_ HRESULT
NavigationCache::CacheContent(
    _In_ xaml::Navigation::NavigationCacheMode navigationCacheMode,
    _In_ HSTRING descriptor,
    _In_ IInspectable *pIInspectable)
{
    HRESULT hr = S_OK;
    BOOLEAN wasReplaced = FALSE;

    IFCPTR(descriptor);
    IFCPTR(pIInspectable);
    IFCEXPECT(navigationCacheMode != N::NavigationCacheMode_Disabled);

    if (navigationCacheMode == N::NavigationCacheMode_Enabled)
    {
        wrl_wrappers::HString strDescriptor;

        IFCEXPECT(m_transientCacheSize >= 1);
        IFCEXPECT(m_transientCacheMruList.size() <= m_transientCacheSize);

        if (m_transientCacheMruList.size() == m_transientCacheSize)
        {
            IFC(UncacheContent());
        }

        IFC(strDescriptor.Set(descriptor));
        m_transientCacheMruList.emplace_back(std::move(strDescriptor));

        IFC(m_transientMap.Insert(descriptor, pIInspectable, &wasReplaced));
    }
    else if (navigationCacheMode == N::NavigationCacheMode_Required)
    {
        IFC(m_permanentMap.Insert(descriptor, pIInspectable, &wasReplaced));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
NavigationCache::UncacheContent()
{
    HRESULT hr = S_OK;
    UINT size = 0;
    wrl_wrappers::HString strDescriptor;
    IFCEXPECT(m_transientCacheMruList.size() >= 1);
    IFC(m_transientMap.Size(&size));
    IFCEXPECT(size >= 1);

    // Remove LRU item

    strDescriptor = std::move(m_transientCacheMruList.front());
    m_transientCacheMruList.erase(m_transientCacheMruList.begin());

    IFC(m_transientMap.Remove(strDescriptor.Get()));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
NavigationCache::UncachePageContent(
    _In_ HSTRING descriptor)
{
    HRESULT hr = S_OK;
    BOOLEAN found = FALSE;
    StringListType::iterator listIterator;

    IFCPTR(descriptor);

    IFC(m_permanentMap.HasKey(descriptor, &found));
    if (found)
    {
        IFC(m_permanentMap.Remove(descriptor));
    }
    else
    {
        for (listIterator = m_transientCacheMruList.begin(); listIterator != m_transientCacheMruList.end(); ++listIterator)
        {
            if (descriptor == *listIterator)
            {
                m_transientCacheMruList.erase(listIterator);
                break;
            }
        }
        IFC(m_transientMap.Remove(descriptor));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
NavigationCache::ChangeTransientCacheSize(
    _In_ UINT transientCacheSize)
{
    HRESULT hr = S_OK;
    INT cItemsToUncache = 0;
    UINT size = 0;

    // If the transient cache size has been reduced,
    // uncache if needed
    if (transientCacheSize < m_transientCacheSize)
    {
        size = m_transientCacheMruList.size();

        if (size > transientCacheSize)
        {
            cItemsToUncache = size - transientCacheSize;
        }

        for (INT i = 0; i < cItemsToUncache; ++i)
        {
            IFC(UncacheContent());
        }
    }

    m_transientCacheSize = transientCacheSize;

Cleanup:
    RRETURN(hr);
}

void NavigationCache::ReferenceTrackerWalk( _In_ EReferenceTrackerWalkType walkType)
{
    m_transientMap.ReferenceTrackerWalk(walkType);
    m_permanentMap.ReferenceTrackerWalk(walkType);
}

