// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <ThemeWalkResourceCache.h>
#include <CDependencyObject.h>
#include "wil\result.h"

#include <FrameworkUdk/Containment.h>

// Bug 46076120: Explorer first frame - resource lookups don't use the lookup cache during layout
#define WINAPPSDK_CHANGEID_46076120 46076120

wil::details::lambda_call<std::function<void()>> ThemeWalkResourceCache::BeginThemeWalk()
{
    // This method should used idf the KIR for 46076120 is rolled back (i.e. the fix has been disabled).
    ASSERT(!WinAppSdk::Containment::IsChangeEnabled<WINAPPSDK_CHANGEID_46076120>());
    ASSERT(!m_isCachingThemeResources);

    m_isInThemeWalk = true;
    return wil::scope_exit(std::function<void()>([this]() {
        m_isInThemeWalk = false;

        // The cache only persists for the duration of the theme walk.
        m_resourceCache.clear();

#if XCP_MONITOR
        m_resourceCache.shrink_to_fit();
#endif
    }));
}

wil::details::lambda_call<std::function<void()>> ThemeWalkResourceCache::BeginCachingThemeResources()
{
    // This method should used iff the fix for 46076120 is enabled.
    ASSERT(WinAppSdk::Containment::IsChangeEnabled<WINAPPSDK_CHANGEID_46076120>());
    ASSERT(!m_isInThemeWalk);

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
        m_resourceCache.erase(
            std::remove_if(m_resourceCache.begin(), m_resourceCache.end(),
                [&](const CacheItemType& item)
                {
                    return std::get<2>(item).Equals(resourceKey);
                }),
            m_resourceCache.end());
    }
}

void ThemeWalkResourceCache::SetSubTreeTheme(Theming::Theme theme)
{
    m_subTreeTheme = theme;
}

_Check_return_ CDependencyObject*
ThemeWalkResourceCache::TryGetCachedResource(
    _In_ CResourceDictionary* targetDictionary,
    _In_ const xstring_ptr& resourceKey
    )
{
    CDependencyObject* resource = nullptr;

    if (m_isInThemeWalk || m_isCachingThemeResources)
    {
        auto subTreeTheme = m_subTreeTheme;
        auto iter = std::find_if(m_resourceCache.begin(), m_resourceCache.end(),
            [&](const CacheItemType& item)
            {
                return (std::get<0>(item) == targetDictionary && std::get<1>(item) == subTreeTheme && std::get<2>(item).Equals(resourceKey));
            });

        if (iter != m_resourceCache.end())
        {
            resource = std::get<3>(*iter).lock_noref();
        }
    }

    return resource;
}

void
ThemeWalkResourceCache::AddCachedResource(
    _In_ CResourceDictionary* targetDictionary,
    _In_ const xstring_ptr& resourceKey,
    _In_ CDependencyObject* resource
    )
{
    if (m_isInThemeWalk || m_isCachingThemeResources)
    {
        auto subTreeTheme = m_subTreeTheme;
        auto iter = std::find_if(m_resourceCache.begin(), m_resourceCache.end(),
            [&](const CacheItemType& item)
            {
                return (std::get<0>(item) == targetDictionary && std::get<1>(item) == subTreeTheme && std::get<2>(item).Equals(resourceKey));
            });

        // Only add an entry if one isn't already in there.
        if (iter == m_resourceCache.end())
        {
            m_resourceCache.push_back(std::make_tuple(targetDictionary, subTreeTheme, resourceKey, xref::get_weakref(resource)));
        }
    }
}
