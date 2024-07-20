// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "corep.h"

_Check_return_
HRESULT ResourceManager::Create(
    _In_ CCoreServices *pCore,
    _Outptr_ IPALResourceManager **ppResourceManager
    )
{
    HRESULT hr = S_OK;
    IPALResourceProvider *pResourceProvider = NULL;
    ResourceManager *pResourceManager = NULL;

    IFC(gps->CreateResourceProvider(pCore, &pResourceProvider));

    pResourceManager = new ResourceManager(pCore, pResourceProvider);

    *ppResourceManager = pResourceManager;
    pResourceManager = NULL;

Cleanup:
    ReleaseInterface(pResourceManager);
    ReleaseInterface(pResourceProvider);
    RRETURN(hr);
}

ResourceManager::ResourceManager(
    _In_ CCoreServices *pCore,
    _In_ IPALResourceProvider *pResourceProvider
) :
    m_pCore(pCore),
    m_pResourceProvider(pResourceProvider),
    m_pAppDataProvider(nullptr),
    m_resourceInvalidationId(0)
{
    // Weak reference to core
    XCP_WEAK(&m_pCore);

    AddRefInterface(pResourceProvider);
}

ResourceManager::~ResourceManager()
{
    ReleaseInterface(m_pResourceProvider);
    ReleaseInterface(m_pAppDataProvider);
}

_Check_return_
HRESULT ResourceManager::GetAppDataProviderNoRef(_Outptr_ IPALApplicationDataProvider** provider)
{
    if (!m_pAppDataProvider)
    {
        IFC_RETURN(gps->CreateApplicationDataProvider(&m_pAppDataProvider));
    }

    *provider = m_pAppDataProvider;
    return S_OK;
}

_Check_return_
HRESULT ResourceManager::IsLocalResourceUri(
    _In_ IPALUri* pUri,
    _Out_ bool* pIsLocal
    )
{
    return CBasePALResource::IsLocalResourceUri(pUri, pIsLocal);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      See IPALResourceManager::CanResourceBeInvalidated
//
//---------------------------------------------------------------------------
_Check_return_
HRESULT ResourceManager::CanResourceBeInvalidated(
    _In_ IPALUri* resourceUri,
    _Out_ bool* canBeInvalidated
    )
{
    HRESULT hr = S_OK;
    xstring_ptr strScheme;

    IFC(UriXStringGetters::GetScheme(resourceUri, &strScheme));

    // Note: currently we treat any resource that could be resolved through MRT as
    // able to be invalidated for any reason.
    //
    // In the future, if we have better support for resource invalidation, this
    // check will need to be more sophisticated.

    *canBeInvalidated =
        strScheme.Equals(MsUriHelpers::GetAppxScheme(), xstrCompareCaseInsensitive) ||
        strScheme.Equals(MsUriHelpers::GetResourceScheme(), xstrCompareCaseInsensitive);

Cleanup:
    RRETURN(hr);
}

XUINT32 ResourceManager::GetResourceInvalidationId(
    )
{
    return m_resourceInvalidationId;
}

_Check_return_ HRESULT ResourceManager::TryGetLocalResource(
    _In_ IPALUri* pResourceUri,
    _Outptr_result_maybenull_ IPALResource** ppResource
    )
{
    HRESULT hr = S_OK;
    IPALUri* pMsResourceUri = NULL;

    *ppResource = NULL;

    xstring_ptr strScheme;

    IFC(UriXStringGetters::GetScheme(pResourceUri, &strScheme));

    if (strScheme.Equals(MsUriHelpers::GetAppDataScheme(), xstrCompareCaseInsensitive))
    {
        IPALApplicationDataProvider* provider = nullptr;

        IFC(GetAppDataProviderNoRef(&provider));

        IFC(provider->GetAppDataResource(pResourceUri, ppResource));
        goto Cleanup;
    }

    if (strScheme.Equals(MsUriHelpers::GetAppxScheme(), xstrCompareCaseInsensitive))
    {
        IFC(pResourceUri->TransformToMsResourceUri(&pMsResourceUri));
    }
    else
    {
        // Note that we assume we're being called with one of the 3 schemes, so at this
        // point it's ms-resource. If it isn't, the IPALResourceProvider::GetFilePath call below
        // will catch it and fail with E_INVALIDARG.
        pMsResourceUri = pResourceUri;
        pResourceUri->AddRef();
    }

    IFC(m_pResourceProvider->TryGetLocalResource(pMsResourceUri, ppResource));

Cleanup:
    ReleaseInterface(pMsResourceUri);

    RRETURN(hr);
}


_Check_return_
HRESULT ResourceManager::GetPropertyBag(
       _In_ const xstring_ptr_view& xUid,
       _In_ const IPALUri *pBaseUri,
       _Out_ PropertyBag& propertyBag
    )
{
    xref_ptr<IPALUri> spResourceUri;
    xref_ptr<IPALUri> spBaseUri(const_cast<IPALUri*> (pBaseUri));

    // If the xUid is an absolute URI, pass it in as is. This is the trivial case.
    if (SUCCEEDED(gps->UriCreate(xUid.GetCount(), const_cast<WCHAR*>(xUid.GetBuffer()), spResourceUri.ReleaseAndGetAddressOf())))
    {
        IFC_RETURN(m_pResourceProvider->GetPropertyBag(spResourceUri.get(), propertyBag));
        return S_OK;
    }

    IFC_RETURN(GetUriForPropertyBagLookup(xUid, spBaseUri, spResourceUri));
    IFC_RETURN(m_pResourceProvider->GetPropertyBag(spResourceUri.get(), propertyBag));

    return S_OK;
}

_Check_return_ HRESULT ResourceManager::CombineResourceUri(
    _In_ IPALUri *pBaseUri,
    _In_ const xstring_ptr_view& strFragment,
    _Outptr_ IPALUri **ppCombinedUri
    )
{
    xref_ptr<IPALUri> combinedUri;
    bool isMsResourceUri = false;
    xstring_ptr strAdjustedFragment;

    // If the fragment is an absolute URI already, do not attempt to combine it with the base URI.
    // Return it directly, to preserve it as is.
    if (SUCCEEDED(gps->UriCreate(strFragment.GetCount(), strFragment.GetBuffer(), combinedUri.ReleaseAndGetAddressOf())))
    {
        *ppCombinedUri = combinedUri.detach();
        return S_OK;
    }

    // If the fragment starts with a '/' and it's an ms-resource URI, the combination loses the
    // "Files" part of the path, so we add it to the fragment before combining
    if (strFragment.GetCount() && strFragment.GetBuffer()[0] == L'/')
    {
        IFC_RETURN(MsUriHelpers::IsMsResourceUri(pBaseUri, &isMsResourceUri));

        if (isMsResourceUri)
        {
            XStringBuilder adjustedFragmentBuilder;

            IFC_RETURN(adjustedFragmentBuilder.Initialize(strFragment.GetCount() + xstrlen(L"/Files")));
            IFC_RETURN(adjustedFragmentBuilder.Append(L"/Files", xstrlen(L"/Files")));
            IFC_RETURN(adjustedFragmentBuilder.Append(strFragment));
            IFC_RETURN(adjustedFragmentBuilder.DetachString(&strAdjustedFragment));
        }
    }

    const xstring_ptr_view& strFragmentToUse = isMsResourceUri? strAdjustedFragment : strFragment;
    IFC_RETURN(pBaseUri->Combine(strFragmentToUse.GetCount(), const_cast<WCHAR*>(strFragmentToUse.GetBuffer()), combinedUri.ReleaseAndGetAddressOf()));

    *ppCombinedUri = combinedUri.detach();

    return S_OK;
}

_Check_return_
HRESULT ResourceManager::IsAmbiguousUriFragment(
        _In_ const xstring_ptr_view& strUriFragment,
        _Out_ bool *pIsAmbiguous
        )
{
    HRESULT hr = S_OK;
    IPALUri *pCombinedUri = NULL;

    // If the fragment is an absolute URI, it's not ambiguous
    if (SUCCEEDED(gps->UriCreate(strUriFragment.GetCount(), strUriFragment.GetBuffer(), &pCombinedUri)))
    {
        *pIsAmbiguous = FALSE;
        goto Cleanup;
    }

    // If the fragment starts with a '/', it refers to the package root, and is not
    // ambiguous
    if (strUriFragment.GetCount() && strUriFragment.GetBuffer()[0] == L'/')
    {
        *pIsAmbiguous = FALSE;
        goto Cleanup;
    }

    *pIsAmbiguous = TRUE;

Cleanup:
    ReleaseInterface(pCombinedUri);
    RRETURN(hr);
}

_Check_return_
HRESULT ResourceManager::CanCacheResource(
        _In_ const IPALUri *pUri,
        _Out_ bool *pCanCache
        )
{
    HRESULT hr = S_OK;
    xstring_ptr strScheme;

    IFC(UriXStringGetters::GetScheme(pUri, &strScheme));
    *pCanCache = !strScheme.Equals(MsUriHelpers::GetAppDataScheme(), xstrCompareCaseInsensitive);

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ResourceManager::TryResolveUri(
    _In_ const xstring_ptr_view& strUri,
    _In_opt_ IPALUri* pBaseUri,
    _Outptr_result_maybenull_ IPALResource** ppResource)
{
    HRESULT hr = S_OK;
    IPALUri* pResourceUri = NULL;
    bool fLocalResource;

    *ppResource = NULL;

    // Use default base URI from core if none is provided.
    if (!pBaseUri)
    {
        pBaseUri = m_pCore->GetBaseUriNoRef();
    }

    // Combine the passed-in URI string (which may be a simple
    // fragment or an already an absolute URI) with the base URI.
    // This will be the final resource URI we use for the rest of resolution.
    IFC(CombineResourceUri(pBaseUri, strUri, &pResourceUri));

    // First check for a local resource.
    IFC(IsLocalResourceUri(pResourceUri, &fLocalResource));
    if (fLocalResource)
    {
        TryGetLocalResource(pResourceUri, ppResource);
        goto Cleanup;
    }

    // Not a local resource.
    IFC(CBasePALResource::Create(pResourceUri, ppResource));

Cleanup:
    ReleaseInterface(pResourceUri);

    RRETURN(hr);
}



_Check_return_
HRESULT ResourceManager::SetScaleFactor(
    XUINT32 ulScaleFactor
    )
{
    HRESULT hr = S_OK;

    IFC(m_pResourceProvider->SetScaleFactor(ulScaleFactor));

    ++m_resourceInvalidationId;

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ResourceManager::NotifyThemeChanged()
{
    IFC_RETURN(m_pResourceProvider->NotifyThemeChanged());

    return S_OK;
}

//---------------------------------------------------------------------------
//
// See IPALResourceManager::SetProcessMUILanguages.
//
//---------------------------------------------------------------------------
_Check_return_
HRESULT ResourceManager::SetProcessMUILanguages(
    )
{
    return m_pResourceProvider->SetProcessMUILanguages();
}

void ResourceManager::DetachEvents()
{
    if (m_pResourceProvider != nullptr)
    {
        m_pResourceProvider->DetachEvents();
    }
}

_Check_return_
HRESULT ResourceManager::GetUriForPropertyBagLookup(
    _In_ const xstring_ptr_view& strXUid,
    _In_ const xref_ptr<IPALUri>& spBaseUri,
    _Out_ xref_ptr<IPALUri>& spPropertyBagUri
)
{
    bool isMsResourceUri = false;
    bool hadFilePath = false;
    xstring_ptr strResourceMap;
    xstring_ptr strFilePath;
    xref_ptr<IPALUri> spMsResourceBaseUri;
    xref_ptr<IPALUri> spBaseResourceUri;
    XStringBuilder baseResourceUriStrBuilder;
    xephemeral_string_ptr strComponentName;

    // Make sure the base URI we have is ms-resource
    IFC_RETURN(MsUriHelpers::IsMsResourceUri(spBaseUri.get(), &isMsResourceUri));
    if (isMsResourceUri)
    {
        IFC_RETURN(spBaseUri->Clone(spMsResourceBaseUri.ReleaseAndGetAddressOf()));
    }
    else
    {
        IFC_RETURN(spBaseUri->TransformToMsResourceUri(spMsResourceBaseUri.ReleaseAndGetAddressOf()));
    }

    // Construct Resource Uri from x:Uid
    //
    // The base resource URI is defined as follows:
    //      ms-resource://<ResourceMap>/<Component>/Resources
    // Where:
    //      <ResourceMap> is an optional resource map coming from the base URI
    //      <Component> is a subdirectory coming from the file path in the base URI if its
    //                         resource scope is Component
    IFC_RETURN(MsUriHelpers::CrackMsResourceUri(spMsResourceBaseUri.get(), &strResourceMap, NULL, &strFilePath, &hadFilePath));
    if (!hadFilePath)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    if (spBaseUri->GetComponentResourceLocation() == ::ComponentResourceLocation::Nested)
    {
        // Extract the component name out of the file path. The component name is the first subdirectory.
        // e.g. given "VendorLib/MainPage.xaml", the component is "VendorLib".
        // So, the component name is everything up to the first forward slash.
        // It is invalid to not have a forward slash, or have a forward slash at position 0.

        auto offset = strFilePath.FindChar(L'/');
        IFC_RETURN(offset != xstring_ptr_view::npos ? S_OK : E_INVALIDARG);
        if (offset == 0)
        {
            // Note: this error can be caused by a bug calculating PriIndexName.
            // See https://task.ms/36925415
            IFC_RETURN(E_INVALIDARG);   
        }

        strFilePath.SubString(0, offset, &strComponentName);
    }

    XUINT32 baseResourceUriLength = xstrlen(L"ms-resource://") + strResourceMap.GetCount() + 1 + strComponentName.GetCount() + xstrlen(L"/Resources/") + strXUid.GetCount();

    IFC_RETURN(baseResourceUriStrBuilder.Initialize(baseResourceUriLength));
    IFC_RETURN(baseResourceUriStrBuilder.Append(STR_LEN_PAIR(L"ms-resource://")));
    IFC_RETURN(baseResourceUriStrBuilder.Append(strResourceMap));
    if (!strComponentName.IsNullOrEmpty())
    {
        IFC_RETURN(baseResourceUriStrBuilder.AppendChar(L'/'));
        IFC_RETURN(baseResourceUriStrBuilder.Append(strComponentName));
    }
    IFC_RETURN(baseResourceUriStrBuilder.Append(STR_LEN_PAIR(L"/Resources/")));

    IFC_RETURN(gps->UriCreate(baseResourceUriStrBuilder.GetCount(), const_cast<WCHAR*> (baseResourceUriStrBuilder.GetBuffer()), spBaseResourceUri.ReleaseAndGetAddressOf()));
    IFC_RETURN(spBaseResourceUri->Combine(strXUid.GetCount(), const_cast<WCHAR*>(strXUid.GetBuffer()), spPropertyBagUri.ReleaseAndGetAddressOf()));

    return S_OK;
}
