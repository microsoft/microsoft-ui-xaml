// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

//------------------------------------------------------------------------
//
//  Synopsis:
//      Create a new surface cache.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
SurfaceCache::Create(
    _In_opt_ CCoreServices* pCore,
    _Outptr_ SurfaceCache** ppCache
    )
{
    HRESULT hr = S_OK;
    SurfaceCache* pNewCache = NULL;

    pNewCache = new SurfaceCache();
 
    IFC(pNewCache->Initialize(pCore));

    *ppCache = pNewCache;
    pNewCache = NULL;

Cleanup:
    ReleaseInterface(pNewCache);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      (ctor) - Create a new surface cache.
//
//------------------------------------------------------------------------
SurfaceCache::SurfaceCache(
    )
    : m_pStore(NULL)
{
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      (dtor) - Clean up surface cache.
//
//------------------------------------------------------------------------
SurfaceCache::~SurfaceCache(
    )
{
    if (m_pStore != NULL)
    {
        VERIFYHR(m_pStore->Traverse(&CleanupSurfaceCacheStoreValue, this));
    }
    
    ReleaseInterface(m_pStore);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Initialize surface cache and create value store.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
SurfaceCache::Initialize(
    _In_ CCoreServices* pCore
    )
{
    m_pStore = new CValueStore(false);
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Get a surface from the cache if it exists.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
SurfaceCache::GetResources(
    _In_ const xstring_ptr_view& strKey,
    _Outptr_result_maybenull_ ImageHardwareResources **ppResources
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(m_pStore->GetValue(strKey, reinterpret_cast< XHANDLE* >(ppResources))))
    {
        AddRefInterface(*ppResources);
    }

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Add a surface to the cache.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
SurfaceCache::AddResources(
    _In_ const xstring_ptr& strKey,
    _In_ ImageHardwareResources *pResources
    )
{
    CNotifyOnDelete* pNotifyOnDelete = NULL;

    IFC_RETURN(pResources->GetNotifyOnDelete(&pNotifyOnDelete));

    pNotifyOnDelete->AddOnDeleteCallback(this, strKey);

    IFC_RETURN(m_pStore->PutValue(strKey, static_cast< XHANDLE >(pResources)));
    AddRefInterface(pResources);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Remove a surface from the cache.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
SurfaceCache::Remove(
    _In_ const xstring_ptr& strKey
    )
{
    HRESULT hr = S_OK;
    ImageHardwareResources *pResources = NULL;

    if (SUCCEEDED(m_pStore->GetValue(strKey, reinterpret_cast< XHANDLE* >(&pResources))))
    {
        //
        // CValueStore doesn't seem to have a way of removing values. We simulate a remove by
        // associating NULL with the key, which is different from actually removing the value.
        // GetValue will return S_OK with a NULL value rather than return a failing HR, so we
        // have to check that the resources aren't NULL here even if the lookup succeeds.
        //
        if (pResources != NULL)
        {
            //
            // Remove the key before proceeding with deletion of the notification -- the key
            // (passed in to this method by reference) will be invalidated during the call
            // to RemoveOnDeleteCallback.
            //
            IFC(m_pStore->PutValue(strKey, NULL));

            CNotifyOnDelete* pNotifyOnDeleteNoRef = NULL;

            IFC(pResources->GetNotifyOnDelete(&pNotifyOnDeleteNoRef));

            pNotifyOnDeleteNoRef->RemoveOnDeleteCallback(this);
        }
    }

Cleanup:
    ReleaseInterface(pResources);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Callback method for cleaning up hw image caches from Uri store.
//
//------------------------------------------------------------------------
void
SurfaceCache::CleanupSurfaceCacheStoreValue(
    _In_ const xstring_ptr& strKey,
    XHANDLE value,
    XHANDLE extraData
    )
{
    HRESULT hr = S_OK;

    ImageHardwareResources* pResources = static_cast< ImageHardwareResources* >(value);
    CNotifyOnDelete* pNotifyOnDelete = NULL;

    if (pResources != NULL)
    {
        IFC(pResources->GetNotifyOnDelete(&pNotifyOnDelete));
        pNotifyOnDelete->RemoveOnDeleteCallback(reinterpret_cast<SurfaceCache*>(extraData));
        ReleaseInterface(pResources);
    }

Cleanup:
    XCP_FAULT_ON_FAILURE(SUCCEEDED(hr));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Handler for a surface being deleted and needing to be removed
//      from the cache.
//
//------------------------------------------------------------------------
void
SurfaceCache::OnDelete(
    _In_ const xstring_ptr& token
    )
{
    VERIFYHR(Remove(token));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Clear the cache completely
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
SurfaceCache::Clear()
{
    if (m_pStore != NULL)
    {
        IFC_RETURN(m_pStore->Traverse(&CleanupSurfaceCacheStoreValue, this));
    }

    // All elements have been removed, now recreate the store completely
    ReleaseInterface(m_pStore);
    m_pStore = new CValueStore(FALSE);
    
    return S_OK;
}


