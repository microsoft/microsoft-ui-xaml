// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ThemeWalkResourceCacheUnitTests.h"

#include <XamlLogging.h>

#include <ThemeWalkResourceCache.h>
#include <CDependencyObject.h>
#include "Theme.h"

using namespace WEX::Common;
using namespace Theming;

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace Theming {

    void ThemeWalkResourceCacheUnitTests::DoesCacheResourceInThemeWalk()
    {
        ThemeWalkResourceCache cache;

        auto endWalkOnExit = cache.BeginCachingThemeResources();

        xref_ptr<CDependencyObject> resource;
        resource.attach(new CDependencyObject());

        auto targetDictionary = reinterpret_cast<CResourceDictionary*>(1234);
        DECLARE_CONST_XSTRING_PTR_STORAGE(key, L"ResourceKey");

        // We haven't cached the resource yet, so it should return null.
        auto cachedResource = cache.TryGetCachedResource(targetDictionary, xstring_ptr(key));
        VERIFY_IS_NULL(cachedResource);

        cache.AddCachedResource(targetDictionary, xstring_ptr(key), resource.get());

        // The resource should have been cached and so querying it should return
        // something.
        cachedResource = cache.TryGetCachedResource(targetDictionary, xstring_ptr(key));
        VERIFY_ARE_EQUAL(cachedResource, resource.get());
    }

    void ThemeWalkResourceCacheUnitTests::DoesNotCacheResourceOutsideThemeWalk()
    {
        ThemeWalkResourceCache cache;

        xref_ptr<CDependencyObject> resource;
        resource.attach(new CDependencyObject());

        auto targetDictionary = reinterpret_cast<CResourceDictionary*>(1234);
        DECLARE_CONST_XSTRING_PTR_STORAGE(key, L"ResourceKey");

        cache.AddCachedResource(targetDictionary, xstring_ptr(key), resource.get());

        // We're not in a theme walk, so the cache should be empty.
        VERIFY_IS_TRUE(cache.IsEmpty());

        // Make sure trying to query the resource returns null as well.
        auto cachedResource = cache.TryGetCachedResource(targetDictionary, xstring_ptr(key));
        VERIFY_IS_NULL(cachedResource);
    }

    void ThemeWalkResourceCacheUnitTests::DoesCacheResourcePerDictionary()
    {
        ThemeWalkResourceCache cache;

        auto endWalkOnExit = cache.BeginCachingThemeResources();

        xref_ptr<CDependencyObject> resource1;
        resource1.attach(new CDependencyObject());

        auto targetDictionary1 = reinterpret_cast<CResourceDictionary*>(1234);

        xref_ptr<CDependencyObject> resource2;
        resource2.attach(new CDependencyObject());
        auto targetDictionary2 = reinterpret_cast<CResourceDictionary*>(5678);

        DECLARE_CONST_XSTRING_PTR_STORAGE(key, L"ResourceKey");

        cache.AddCachedResource(targetDictionary1, xstring_ptr(key), resource1.get());
        cache.AddCachedResource(targetDictionary2, xstring_ptr(key), resource2.get());

        // Make sure querying the cache with a given dictionary returns the correct
        // resource.
        auto cachedResource = cache.TryGetCachedResource(targetDictionary1, xstring_ptr(key));
        VERIFY_ARE_EQUAL(cachedResource, resource1.get());

        cachedResource = cache.TryGetCachedResource(targetDictionary2, xstring_ptr(key));
        VERIFY_ARE_EQUAL(cachedResource, resource2.get());

    }

    void ThemeWalkResourceCacheUnitTests::DoesCacheResourcePerSubTreeTheme()
    {
        ThemeWalkResourceCache cache;

        auto endWalkOnExit = cache.BeginCachingThemeResources();

        xref_ptr<CDependencyObject> resource1;
        resource1.attach(new CDependencyObject());

        xref_ptr<CDependencyObject> resource2;
        resource2.attach(new CDependencyObject());

        auto targetDictionary = reinterpret_cast<CResourceDictionary*>(1234);

        DECLARE_CONST_XSTRING_PTR_STORAGE(key, L"ResourceKey");

        cache.SetSubTreeTheme(Theme::Dark);
        cache.AddCachedResource(targetDictionary, xstring_ptr(key), resource1.get());

        cache.SetSubTreeTheme(Theme::Light);
        cache.AddCachedResource(targetDictionary, xstring_ptr(key), resource2.get());

        // Make sure querying the cache with a given sub-tree theme set returns the correct
        // resource.
        cache.SetSubTreeTheme(Theme::Dark);
        auto cachedResource = cache.TryGetCachedResource(targetDictionary, xstring_ptr(key));
        VERIFY_ARE_EQUAL(cachedResource, resource1.get());

        cache.SetSubTreeTheme(Theme::Light);
        cachedResource = cache.TryGetCachedResource(targetDictionary, xstring_ptr(key));
        VERIFY_ARE_EQUAL(cachedResource, resource2.get());
    }

    void ThemeWalkResourceCacheUnitTests::DoesClearCacheAfterThemeWalk()
    {
        ThemeWalkResourceCache cache;
        {
            auto endWalkOnExit = cache.BeginCachingThemeResources();

            xref_ptr<CDependencyObject> resource;
            resource.attach(new CDependencyObject());

            auto targetDictionary = reinterpret_cast<CResourceDictionary*>(1234);
            DECLARE_CONST_XSTRING_PTR_STORAGE(key, L"ResourceKey");

            cache.AddCachedResource(targetDictionary, xstring_ptr(key), resource.get());

            VERIFY_IS_FALSE(cache.IsEmpty());
        }

        // The cache should get cleared after we end the theme walk.
        VERIFY_IS_TRUE(cache.IsEmpty());
    }

} } } } } } // namespace ::Windows::UI::Xaml::Tests::Controls::Theming
