// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Theme.h"
#include <functional>

class CResourceDictionary;
class CDependencyObject;

namespace wil { namespace details {
    template <typename T>
    class lambda_call;
}}

class ThemeWalkResourceCache
{
public:
    ThemeWalkResourceCache() = default;
    ThemeWalkResourceCache(const ThemeWalkResourceCache&) = delete;
    ThemeWalkResourceCache& operator=(const ThemeWalkResourceCache&) = delete;

    ~ThemeWalkResourceCache() = default;

    wil::details::lambda_call<std::function<void()>> BeginThemeWalk();

    wil::details::lambda_call<std::function<void()>> BeginCachingThemeResources();
    // Remove a cache entry because it has been invalidated - either a new resource by the same key was added or an
    // existing resource was removed.
    void RemoveThemeResourceCacheEntry(_In_ const xstring_ptr_view& resourceKey);

    void SetSubTreeTheme(Theming::Theme theme);

    // This does not add-ref the return value.
    _Check_return_ CDependencyObject* TryGetCachedResource(
        _In_ CResourceDictionary* targetDictionary,
        _In_ const xstring_ptr& resourceKey
        );

    void AddCachedResource(
        _In_ CResourceDictionary* targetDictionary,
        _In_ const xstring_ptr& resourceKey,
        _In_ CDependencyObject* resource
        );

    bool IsEmpty() const
    {
        return m_resourceCache.empty();
    }

private:
    // We cache our resource lookups for the duration of a theme walk and clear them
    // when the walk completes.  This is done to alleviate the perf cost of querying
    // the resource dictionary multiple times for the same resource.
    typedef std::tuple<CResourceDictionary*, Theming::Theme, xstring_ptr, xref::weakref_ptr<CDependencyObject>> CacheItemType;

    std::vector<CacheItemType> m_resourceCache;

    Theming::Theme m_subTreeTheme = Theming::Theme::None;

    bool m_isInThemeWalk = false;

    //
    // We cache theme resources for faster lookup in two main scenarios:
    //
    //  1. During the CCoreServices::NotifyThemeChange walk - the entire tree gets notified to look up their theme
    //     resources again
    //
    //  2. During UpdateLayout - a lot of elements can enter the tree due to applying templates, and they will do a
    //     theme resource lookup as part of entering the tree. This spares ~69k ResourceDictionary lookups.
    //
    // We also cache theme resources in some additional scenarios from profiling:
    //
    //  3. During AppBarButton's VSM update - spares ~1000 "AppBarButtonForegroundDisabled" lookups
    //
    // Note that scenario 1 (the NotifyThemeChange walk) is safe. It doesn't synchronously call out to app code and
    // doesn't mess with ResourceDictionaries in a way that invalidates caches. Note that it _does_ mess with
    // ResourceDictionaries - it builds the system color resources, but that's before anything tries to read back a
    // theme resource so the cache is still empty. Scenario 2 (UpdateLayout) is more complicated, because this can call
    // out synchronously to app code, which can invalidate entries in the cache. To account for this, if any resource is
    // added to or removed from any ResourceDictionary while we're caching theme resources, we'll clear the
    // corresponding entry from the cache.
    //
    bool m_isCachingThemeResources = false;
};

