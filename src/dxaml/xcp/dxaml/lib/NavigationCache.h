// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Asynchronously loads frame content and caches it. Uses two caches:
//      one for permanent caching, and one for transient caching. The size
//      of the transient cache is bounded, and the size of the permanent
//      cache is not. Loading can be canceled and performed again
//      synchronously.

#pragma once

namespace DirectUI
{
    class PageStackEntry;

    class NavigationCache
    {
    private:
        typedef std::list<wrl_wrappers::HString> StringListType;

        xaml_controls::IFrame *m_pIFrame;

        // Transient cache size (Frame.CacheSize)
        UINT m_transientCacheSize;
        
        // List of items in the transient cache, sorted by Most Recently Used. MRU cached 
        // item is at the end. Items are flushed out when m_transientCacheSize is exceeded.
        StringListType m_transientCacheMruList;

        // Transient cache. Items are flushed out when m_transientCacheSize is exceeded.
        TrackerPropertySet<IInspectable> m_transientMap;

        // Permanent cache. Items are not flushed out.
        TrackerPropertySet<IInspectable> m_permanentMap;

        NavigationCache();

        static _Check_return_ HRESULT
        LoadContent(
            _In_ HSTRING descriptor,
            _Outptr_ IInspectable **ppIInspectable);

        _Check_return_ HRESULT
        GetCachedContent(
            _In_ HSTRING descriptor,
            _Outptr_ IInspectable **ppIInspectable,
            _Out_ BOOLEAN *pFound);

        _Check_return_ HRESULT
        CacheContent(
            _In_ xaml::Navigation::NavigationCacheMode navigationCacheMode,
            _In_ HSTRING descriptor,
            _In_ IInspectable *pIInspectable);

        _Check_return_ HRESULT
        UncacheContent();

    public:
        ~NavigationCache();

        static _Check_return_ HRESULT
        Create(
            xaml_controls::IFrame *pIFrame,
            _In_ UINT transientCacheSize,
            _Outptr_ NavigationCache **ppNavigationCache);

        _Check_return_ HRESULT
        GetContent(
            _In_ PageStackEntry *pPageStackEntry,
            _Outptr_ IInspectable **ppIInspectable);

        _Check_return_ HRESULT
        UncachePageContent(
            _In_ HSTRING descriptor);
        
        _Check_return_ HRESULT
        ChangeTransientCacheSize(
            _In_ UINT transientCacheSize);

    public:

        void ReferenceTrackerWalk( _In_ EReferenceTrackerWalkType walkType);
    };
}
