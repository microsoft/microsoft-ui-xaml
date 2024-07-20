// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Surface cache for primitive composition.
//
// Surfaces can be implemented several different ways under the covers:  as single HWTextures, as tiled surfaces
// using D2D bitmaps and doubtless other ways in the future.  The representation is the responsibility of
// ImageSurfaceWrapper.  This class (SurfaceCache) does not look inside the ImageHardwareResource pack
// it is given, it simply stores it with an associated key so that it can be shared between Images.
//
// Note this class does not reference the underlying surfaces, since if it did it would hold them alive
// and they would never get deleted, hence never fire their OnDelete notifications and never get
// removed from the cache.  It does RefCount the ImageHardwareResource objects it stores, as these
// are just convenience wrappers for hiding the details and making it easy to store in a ValueStore.

#pragma once

class ImageHardwareResources;

class SurfaceCache : public CXcpObjectBase< INotifyOnDeleteCallback >
{
public:
    static _Check_return_ HRESULT Create(
        _In_opt_ CCoreServices* pCore,
        _Outptr_ SurfaceCache** ppCache
        );

    _Check_return_ HRESULT AddResources(
        _In_ const xstring_ptr& strKey,
        _In_ ImageHardwareResources *pResources
        );

    _Check_return_ HRESULT GetResources(
        _In_ const xstring_ptr_view& strKey,
        _Outptr_result_maybenull_ ImageHardwareResources **ppSurface
        );

    _Check_return_ HRESULT Remove(
        _In_ const xstring_ptr& strKey
        );

    _Check_return_ HRESULT Clear();

    static void CleanupSurfaceCacheStoreValue(
        _In_ const xstring_ptr& strKey,
        XHANDLE value,
        XHANDLE extraData
        );

protected:
    SurfaceCache(
        );

    ~SurfaceCache(
        ) override;

    _Check_return_ HRESULT Initialize(
        _In_ CCoreServices* pCore
        );

    void OnDelete(
        _In_ const xstring_ptr& token
        ) override;

    CValueStore* m_pStore;
};


