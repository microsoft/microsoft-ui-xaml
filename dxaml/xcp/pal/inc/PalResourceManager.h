// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Defines the PAL resource manager interface.

#pragma once

#include "paltypes.h"
#include "refcounting.h"
#include "xcontainers.h"

typedef std::vector<std::pair<xstring_ptr, xstring_ptr>> PropertyBag;

namespace PALResources
{
    enum class RawStreamType
    {
        IStream,
    };
}

enum class ResourceInvalidationReason
{
    Any,
    ScaleChanged,
    HdrChanged
};

//-----------------------------------------------------------------------------
//
// Represents a resource used by the XAML runtime.
//
//-----------------------------------------------------------------------------
struct IPALResource : public IObject
{
    //-----------------------------------------------------------------------------
    //
    // Attempts to load the resource into memory.
    //
    // Not all resource types currently implement support for loading.
    // In this case E_NOTIMPL is returned.
    //
    //-----------------------------------------------------------------------------
    virtual _Check_return_ HRESULT Load(_Outptr_ IPALMemory** ppMemory) = 0;

    //-----------------------------------------------------------------------------
    //
    // Returns a printable string buffer for debugging / tracing purposes.
    // This buffer is owned by the IPALResource object - caller must not free.
    //
    //-----------------------------------------------------------------------------
    virtual const WCHAR* ToString() = 0;

    //-----------------------------------------------------------------------------
    //
    // Get the URI the identifies this resource.
    // The returned IPALUri is not addref'd.
    //
    //-----------------------------------------------------------------------------
    virtual IPALUri* GetResourceUriNoRef() = 0;

    //-----------------------------------------------------------------------------
    //
    // Get the URI that uniquely identifies this physical resource, for use as a cache key.
    // The returned IPALUri is not addref'd.
    //
    //-----------------------------------------------------------------------------
    virtual IPALUri* GetPhysicalResourceUriNoRef() = 0;

    //-----------------------------------------------------------------------------
    //
    // A local resource has an identifying URI scheme of
    // ms-resource, ms-appx, or ms-data.
    //
    // Note that a resource with an identifying URI scheme of
    // file:// MAY be local, but in general we can't determine
    // this, due to mapped network shares, etc. IsLocal() returns
    // FALSE for file:// resource URIs and we generally should
    // treat these as remote resources.
    //
    //-----------------------------------------------------------------------------
    virtual _Check_return_ HRESULT IsLocal(_Out_ bool* pfLocal) = 0;

    //-----------------------------------------------------------------------------
    //
    // Check whether this resource actually exists.
    //
    // For example, a resource could represent a file path, and this
    // function would check if the file actually exists (at the time
    // this function is called).
    //
    // Not all resource types implement an existence check. In this case
    // E_NOTIMPL is returned.
    //
    //-----------------------------------------------------------------------------
    virtual _Check_return_ HRESULT Exists(_Out_ bool* pfExists) = 0;

    //-----------------------------------------------------------------------------
    //
    // Gets the file path associated with this resource, if any.
    //
    // Not all resource types have an associated file path. For example,
    // a remote resource (e.g. one identified by an http URI) or a resource
    // that represents a stream will not have an associated file path.
    //
    // If this resource has no associated file path, this function succeeds and
    // sets the out parameter to null.
    //
    //-----------------------------------------------------------------------------
    virtual _Check_return_ HRESULT TryGetFilePath(_Out_ xstring_ptr* pstrFilePath) = 0;

    //-----------------------------------------------------------------------------
    //
    // Gets a "raw" (platform-specific) stream associated with this resource, if any.
    // Not all resource types are able to supply a stream.
    //
    // If this resource can't supply a stream, this function succeeds and
    // sets the out parameter to null.
    //
    // The caller requests a specific type of stream by passing in a well-known stream
    // type name. If this implementation doesn't recognize the stream type name, this
    // function succeeds and sets the out parameter to null.
    //
    //-----------------------------------------------------------------------------
    virtual _Check_return_ HRESULT TryGetRawStream(const PALResources::RawStreamType streamType, _Outptr_result_maybenull_ void** ppStream) = 0;

    //-----------------------------------------------------------------------------
    //
    // Returns the scale percentage associated with this resource, if any.
    //
    // If this resource isn't associated with any particular scale, or if its
    // associated scale couldn't be determined, returns 0.
    //
    // The scale percentage is a number like 100, 140, 180, etc.
    //
    //-----------------------------------------------------------------------------
    virtual XUINT32 GetScalePercentage() = 0;
};

struct IPALResourceProvider : public IObject
{
    virtual _Check_return_ HRESULT TryGetLocalResource(
        _In_ IPALUri* pResourceUri,
        _Outptr_result_maybenull_ IPALResource** ppResource
        ) = 0;

    virtual _Check_return_ HRESULT GetString(
        _In_ const xstring_ptr_view &strKey,
        _Out_ xstring_ptr* pstrString
        ) = 0;

    virtual _Check_return_ HRESULT GetPropertyBag(
        _In_ const IPALUri *pUri,
        _Out_ PropertyBag& propertyBag
        ) noexcept = 0;

    // Releases references on the apartment to prevent it from leaking.
    virtual void DetachEvents() = 0;

    virtual _Check_return_ HRESULT SetScaleFactor(
        XUINT32 ulScaleFactor
        ) = 0;

    virtual _Check_return_ HRESULT NotifyThemeChanged(
        ) = 0;

    //-----------------------------------------------------------------------------
    //
    // See IPALResourceManager::SetProcessMUILanguages.
    //
    //-----------------------------------------------------------------------------
    virtual _Check_return_ HRESULT SetProcessMUILanguages(
        ) = 0;
};

struct IPALApplicationDataProvider : public IObject
{
    virtual _Check_return_ HRESULT GetAppDataResource(
        _In_ IPALUri* pResourceUri,
        _Outptr_ IPALResource** ppResource
        ) = 0;
};

struct IPALResourceManager : public IObject
{
    //-----------------------------------------------------------------------------
    //
    // See IPALResource::IsLocal().
    //
    //-----------------------------------------------------------------------------
    virtual _Check_return_ HRESULT IsLocalResourceUri(
        _In_ IPALUri* pUri,
        _Out_ bool* pfIsLocal
        ) = 0;

    //-----------------------------------------------------------------------------
    //
    // Attempts to resolve a resource URI into an IPALResource object that
    // represents the resource.
    //
    // The resource URI must be local - see IPALResource::IsLocal().
    //
    // If the resource can't be resolved, this function succeeds and sets the
    // out param to NULL. In some cases an existence check is performed - if the
    // resource can easily/cheaply be existence-checked and proved to not exist
    // at the time this function is called, the function succeeds and sets the
    // out param to NULL.
    //
    // However, because existence is a temporal property, even if this function
    // returns a non-NULL resource object, that doesn't guarantee that the resource
    // will actually be in existence at a later time when the resource object
    // is used to access the resource data.
    //
    //-----------------------------------------------------------------------------
    virtual _Check_return_ HRESULT TryGetLocalResource(
        _In_ IPALUri* pResourceUri,
        _Outptr_result_maybenull_ IPALResource** ppResource
        ) = 0;

    //-----------------------------------------------------------------------------
    //
    // pstrUri is either a simple fragment (e.g. "foo.xaml"), a relative URI,
    // or an absolute URI.
    //
    // pBaseUri is optional. If not specified, and a base URI is needed, the
    // core base URI will be used as the default.
    //
    // Performs the following sequence:
    //     - combines pstrUri with the base URI to form an absolute resource URI
    //     - if the resource URI is local, calls
    //       IPALResourceManager::TryGetLocalResource() to return an IPALResource
    //       object to represent the resource
    //     - if the resource URI is not local, returns an IPALResource object
    //       that represents the remote resource
    //
    //-----------------------------------------------------------------------------
    virtual _Check_return_ HRESULT TryResolveUri(
        _In_ const xstring_ptr_view& strUri,
        _In_opt_ IPALUri* pBaseUri,
        _Outptr_result_maybenull_ IPALResource** ppResource
        ) = 0;

    //-----------------------------------------------------------------------------
    //
    // Check whether a resource can be invalidated.
    //
    // A resource invalidation occurs when an event occurs that changes the resource
    // loading context. After such an event, loading the same resource URI may
    // result in a different resource being resolved.
    //
    // For example, a resource URI that represents a file would return FALSE. (Even
    // though the contents of a file may change over time, the file itself is
    // always considered to be the same physical resource). Likewise, a resource URI
    // that represents an http[s] resource would return FALSE.
    //
    // Currently only resource URIs that represent an MRT resource will return TRUE.
    // MRT may resolve URIs as different resources if the MRT context changes.
    //
    //-----------------------------------------------------------------------------
    virtual _Check_return_ HRESULT CanResourceBeInvalidated(
        _In_ IPALUri* resourceUri,
        _Out_ bool* canBeInvalidated
        ) = 0;

    //-----------------------------------------------------------------------------
    //
    // The resource invalidation ID can be used to check whether a resource that
    // can be invalidated (see CanResourceBeInvalidated()) should be reloaded.
    //
    // This is a very coarse grained approach to resource invalidation. Currently
    // any time an invalidation event occurs we increment the invalidation ID.
    //
    // Future work to better support resource invalidation will likely do away
    // with the invalidation ID and use a finer-grained mechanism.
    //
    //-----------------------------------------------------------------------------
    virtual XUINT32 GetResourceInvalidationId(
        ) = 0;

    virtual _Check_return_ HRESULT GetPropertyBag(
           _In_ const xstring_ptr_view& strUid,
           _In_ const IPALUri *pBaseUri,
           _Out_ PropertyBag& propertyBag
        ) = 0;

    virtual _Check_return_ HRESULT CombineResourceUri(
        _In_ IPALUri *pBaseUri,
        _In_ const xstring_ptr_view& strFragment,
        _Outptr_ IPALUri **ppCombinedUri
        ) = 0;

    virtual _Check_return_ HRESULT IsAmbiguousUriFragment(
        _In_ const xstring_ptr_view& strUriFragment,
        _Out_ bool *pIsAmbiguous
        ) = 0;

    virtual _Check_return_ HRESULT CanCacheResource(
        _In_ const IPALUri *pUri,
        _Out_ bool *pCanCache
        ) = 0;

    virtual _Check_return_ HRESULT SetScaleFactor(
        XUINT32 ulScaleFactor
        ) = 0;

    virtual _Check_return_ HRESULT NotifyThemeChanged(
        ) = 0;

    //-----------------------------------------------------------------------------
    //
    // Sets the MUI language(s) for this process. Should be called before
    // using MUI functions to load resources.
    //
    // NOTE: eventually we should move all MUI functionality inside the ResourceManager
    // and this would be handled internally. Right now we do MUI loading separately
    // so this is needed.
    //
    //-----------------------------------------------------------------------------
    virtual _Check_return_ HRESULT SetProcessMUILanguages(
        ) = 0;

    // Releases references on the apartment to prevent it from leaking.
    virtual void DetachEvents() = 0;

    virtual _Check_return_ HRESULT GetUriForPropertyBagLookup(
        _In_ const xstring_ptr_view& strXUid,
        _In_ const xref_ptr<IPALUri>& spBaseUri,
        _Out_ xref_ptr<IPALUri>& spPropertyBagUri
        ) = 0;

};
