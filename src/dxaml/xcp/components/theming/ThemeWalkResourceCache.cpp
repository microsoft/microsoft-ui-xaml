// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <ThemeWalkResourceCache.h>
#include <CDependencyObject.h>
#include "wil\result.h"
#include "FrameworkUdk/Containment.h"

// Bug 62645877: [2.0 servicing] Reduce number of xstring_ptr_view::Equals calls
#define WINAPPSDK_CHANGEID_62645877 62645877

wil::details::lambda_call<std::function<void()>> ThemeWalkResourceCache::BeginCachingThemeResources()
{
    if (!m_isCachingThemeResources)
    {
        m_isCachingThemeResources = true;
        return wil::scope_exit(std::function<void()>([this]()
            {
                m_isCachingThemeResources = false;
                m_resourceCache.clear();

#if XCP_MONITOR
                m_resourceCache.shrink_to_fit();
#endif
            }));
    }
    else
    {
        return wil::scope_exit(std::function<void()>([](){}));
    }
}

void ThemeWalkResourceCache::RemoveThemeResourceCacheEntry(_In_ const xstring_ptr_view& resourceKey)
{
    if (m_isCachingThemeResources && m_resourceCache.size() > 0)
    {
        if (WinAppSdk::Containment::IsChangeEnabled<WINAPPSDK_CHANGEID_62645877>())
        {
            auto keyToRemove = ResourceKey(resourceKey, false);

            m_resourceCache.erase(
                std::remove_if(m_resourceCache.begin(), m_resourceCache.end(),
                    [&](const CacheItemType& item)
                    {
                        return std::get<2>(item) == keyToRemove;
                    }),
                m_resourceCache.end());
        }
        else
        {
            m_resourceCache.erase(
                std::remove_if(m_resourceCache.begin(), m_resourceCache.end(),
                    [&](const CacheItemType& item)
                    {
                        return std::get<2>(item).GetKey().Equals(resourceKey);
                    }),
                m_resourceCache.end());
        }
    }
}

void ThemeWalkResourceCache::SetSubTreeTheme(Theming::Theme theme)
{
    m_subTreeTheme = theme;
}

_Check_return_ CDependencyObject*
ThemeWalkResourceCache::TryGetCachedResource(
    _In_ CResourceDictionary* targetDictionary,
    _In_ const ResourceKey& resourceKey
    )
{
    CDependencyObject* resource = nullptr;

    if (m_isCachingThemeResources)
    {
        auto subTreeTheme = m_subTreeTheme;

        if (WinAppSdk::Containment::IsChangeEnabled<WINAPPSDK_CHANGEID_62645877>())
        {
            auto iter = std::find_if(m_resourceCache.begin(), m_resourceCache.end(),
                [&](const CacheItemType& item)
                {
                    return (std::get<0>(item) == targetDictionary && std::get<1>(item) == subTreeTheme && std::get<2>(item) == resourceKey);
                });

            if (iter != m_resourceCache.end())
            {
                resource = std::get<3>(*iter).lock_noref();
            }
        }
        else
        {
            auto iter = std::find_if(m_resourceCache.begin(), m_resourceCache.end(),
                [&](const CacheItemType& item)
                {
                    return (std::get<0>(item) == targetDictionary && std::get<1>(item) == subTreeTheme && std::get<2>(item).GetKey().Equals(resourceKey.GetKey()));
                });

            if (iter != m_resourceCache.end())
            {
                resource = std::get<3>(*iter).lock_noref();
            }
        }
    }

    return resource;
}

void
ThemeWalkResourceCache::AddCachedResource(
    _In_ CResourceDictionary* targetDictionary,
    _In_ const ResourceKey& resourceKey,
    _In_ CDependencyObject* resource
    )
{
    if (m_isCachingThemeResources)
    {
        auto subTreeTheme = m_subTreeTheme;

        if (WinAppSdk::Containment::IsChangeEnabled<WINAPPSDK_CHANGEID_62645877>())
        {
            auto iter = std::find_if(m_resourceCache.begin(), m_resourceCache.end(),
                [&](const CacheItemType& item)
                {
                    return (std::get<0>(item) == targetDictionary && std::get<1>(item) == subTreeTheme && std::get<2>(item) == resourceKey);
                });

            // Only add an entry if one isn't already in there.
            if (iter == m_resourceCache.end())
            {
                m_resourceCache.push_back(std::make_tuple(targetDictionary, subTreeTheme, resourceKey.ToStorage(), xref::get_weakref(resource)));
            }
        }
        else
        {
            auto iter = std::find_if(m_resourceCache.begin(), m_resourceCache.end(),
                [&](const CacheItemType& item)
                {
                    return (std::get<0>(item) == targetDictionary && std::get<1>(item) == subTreeTheme && std::get<2>(item).GetKey().Equals(resourceKey.GetKey()));
                });

            // Only add an entry if one isn't already in there.
            if (iter == m_resourceCache.end())
            {
                m_resourceCache.push_back(std::make_tuple(targetDictionary, subTreeTheme, resourceKey.ToStorageNoHash(), xref::get_weakref(resource)));
            }
        }
    }
}
