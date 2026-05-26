// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

ApplicationDataProvider::ApplicationDataProvider()
{
    m_pLocalFolder = NULL;
    m_pTempFolder = NULL;
    m_pRoamingFolder = NULL;
}

ApplicationDataProvider::~ApplicationDataProvider()
{
    ReleaseInterface(m_pLocalFolder);
    ReleaseInterface(m_pTempFolder);
    ReleaseInterface(m_pRoamingFolder);
}

_Check_return_
HRESULT ApplicationDataProvider::Create(
    _Outptr_ ApplicationDataProvider **ppAppDataProvider
    )
{
    HRESULT hr = S_OK;
    ApplicationDataProvider *pAppDataProvider = NULL;

    pAppDataProvider = new ApplicationDataProvider();

    IFC(pAppDataProvider->Initialize());

    *ppAppDataProvider = pAppDataProvider;
    pAppDataProvider = NULL;

Cleanup:
    ReleaseInterface(pAppDataProvider);
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Maps the given ms-appdata URI to a local file path.
//
//---------------------------------------------------------------------------
_Check_return_
HRESULT ApplicationDataProvider::GetAppDataResource(
    _In_ IPALUri* pResourceUri,
    _Outptr_ IPALResource** ppResource
    )
{
    HRESULT hr = S_OK;
    xstring_ptr strScheme;
    xstring_ptr strUriPath;
    xephemeral_string_ptr strRelativeFilePath;
    xephemeral_string_ptr strSpecialFolderName;
    xstring_ptr strHost;
    XUINT32 startIndex = 0;
    XUINT32 offset = 0;
    IPALUri *pFolderUri = NULL;
    IPALUri *pCombinedUri = NULL;
    xstring_ptr strFilePath;

    IFC(UriXStringGetters::GetScheme(pResourceUri, &strScheme));

    if (!strScheme.Equals(MsUriHelpers::GetAppDataScheme(), xstrCompareCaseInsensitive))
    {
        IFC(E_INVALIDARG);
    }

    if (!m_strPackageIdentityName.IsNullOrEmpty())
    {
        IFC(UriXStringGetters::GetHost(pResourceUri, &strHost));
        if (!strHost.IsNullOrEmpty())
        {
            // Make sure the host matches the current package identity name
            if (!strHost.Equals(m_strPackageIdentityName, xstrCompareCaseInsensitive))
            {
                IFC(E_INVALIDARG);
            }
        }
    }

    // The URI looks like this:
    //      ms-appdata:///SpecialFolderName/Path/To/File
    //
    // We extract the special folder name from the first subdirectory.
    // We also extract the relative file path (everything after the first subdirectory).

    IFC(UriXStringGetters::GetPath(pResourceUri, &strUriPath));
    // Skip the leading forward slash in the URI path
    if (strUriPath.GetCount() && strUriPath.GetBuffer()[0] == L'/')
    {
        startIndex = 1;
    }

    offset = strUriPath.FindChar(L'/', startIndex);
    IFCCHECK(offset != xstring_ptr_view::npos);
    if (offset == startIndex)
    {
        // Can't have a blank special folder name.
        IFC(E_INVALIDARG);
    }

    strUriPath.SubString(startIndex, offset, &strSpecialFolderName);

    if (strSpecialFolderName.Equals(XSTRING_PTR_EPHEMERAL(L"local"), xstrCompareCaseInsensitive))
    {
        pFolderUri = m_pLocalFolder;
    }
    else if (strSpecialFolderName.Equals(XSTRING_PTR_EPHEMERAL(L"roaming"), xstrCompareCaseInsensitive))
    {
        pFolderUri = m_pRoamingFolder;
    }
    else if (strSpecialFolderName.Equals(XSTRING_PTR_EPHEMERAL(L"temp"), xstrCompareCaseInsensitive))
    {
        pFolderUri = m_pTempFolder;
    }
    else
    {
        IFC(E_INVALIDARG);
    }

    // This happens when we're not running immersively--we wouldn't have initialized any of the special
    // folder URIs
    if (!pFolderUri)
    {
        IFC(E_FAIL);
    }

    // +1 to skip the forward slash separating the special folder name from the rest of the path
    strUriPath.SubString(offset + 1, strUriPath.GetCount(), &strRelativeFilePath);

    IFC(pFolderUri->Combine(strRelativeFilePath.GetCount(), strRelativeFilePath.GetBuffer(), &pCombinedUri));

    IFC(UriXStringGetters::GetFilePath(pCombinedUri, &strFilePath));
    IFC(CFilePathResource::Create(pResourceUri, strFilePath, ppResource));

Cleanup:
    ReleaseInterface(pCombinedUri);
    RRETURN(hr);
}

_Check_return_
HRESULT ApplicationDataProvider::Initialize()
{
    HRESULT hr = S_OK;

    IFC(GetStateFolderUris(&m_pLocalFolder, &m_pRoamingFolder, &m_pTempFolder));
    IFC(GetPackageIdentityName(&m_strPackageIdentityName));

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT ApplicationDataProvider::PathAddBackslashHString(
    _Inout_  wrl_wrappers::HString& path
)
{
    const wchar_t* pathBuffer = path.GetRawBuffer(nullptr);
    if (path.Length() > 0 && pathBuffer[path.Length() - 1] != '\\')
    {
        IFC_RETURN(path.Concat(wrl_wrappers::HStringReference(L"\\"), path));
    }

    return S_OK;
}

_Check_return_
HRESULT ApplicationDataProvider::GetStateFolderUrisUsingApplicationData(
    const wrl::ComPtr<wst::IStorageFolder> &spStorageFolder,
    _Outptr_ IPALUri **folder
)
{
    wrl::ComPtr<wst::IStorageItem> spStorageItem;
    wrl_wrappers::HString path;
    UINT32 length = 0;

    IFC_RETURN(spStorageFolder.As(&spStorageItem));
    IFC_RETURN(spStorageItem->get_Path(path.GetAddressOf()));
    IFC_RETURN(PathAddBackslashHString(path));
    const wchar_t* pathBuffer = path.GetRawBuffer(&length);
    IFC_RETURN(gps->UriCreate(length, pathBuffer, folder));

    return S_OK;
}

_Check_return_
HRESULT ApplicationDataProvider::GetStateFolderUris(
    _Outptr_ IPALUri **ppLocalFolder,
    _Outptr_ IPALUri **ppRoamingFolder,
    _Outptr_ IPALUri **ppTempFolder
)
{
    wrl::ComPtr<wst::IApplicationDataStatics> spApplicationDataStatic;
    wrl::ComPtr<wst::IApplicationData> spApplicationData;

    IFC_RETURN(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Windows_Storage_ApplicationData).Get(),
        &spApplicationDataStatic));
    HRESULT hr = spApplicationDataStatic->get_Current(&spApplicationData);
    if (hr == HRESULT_FROM_WIN32(APPMODEL_ERROR_NO_PACKAGE))
    {
        // If we are not in a packaged app, then we cannot get the application data
        // Indicate success and exit.
        return S_OK;
    }

    xref_ptr<IPALUri> pLocalFolder;
    xref_ptr<IPALUri> pRoamingFolder;
    xref_ptr<IPALUri> pTempFolder;

    wrl::ComPtr<wst::IStorageFolder> spStorageFolder;
    IFC_RETURN(spApplicationData->get_LocalFolder(&spStorageFolder));
    IFC_RETURN(GetStateFolderUrisUsingApplicationData(spStorageFolder, pLocalFolder.ReleaseAndGetAddressOf()));

    IFC_RETURN(spApplicationData->get_RoamingFolder(&spStorageFolder));
    IFC_RETURN(GetStateFolderUrisUsingApplicationData(spStorageFolder, pRoamingFolder.ReleaseAndGetAddressOf()));

    IFC_RETURN(spApplicationData->get_TemporaryFolder(&spStorageFolder));
    IFC_RETURN(GetStateFolderUrisUsingApplicationData(spStorageFolder, pTempFolder.ReleaseAndGetAddressOf()));

    *ppLocalFolder = pLocalFolder.detach();
    *ppRoamingFolder = pRoamingFolder.detach();
    *ppTempFolder = pTempFolder.detach();

    return S_OK;
}

_Check_return_
HRESULT ApplicationDataProvider::GetPackageIdentityName(_Out_ xstring_ptr* pstrPackageIdentityName)
{
    HRESULT hr = S_OK;
    XBYTE *pBuffer = NULL;
    UINT32 bufferLength = 0;
    PACKAGE_ID *pPackageId = NULL;
    LONG result = 0;

    // Retrieve required buffer length.
    result = GetCurrentPackageId(&bufferLength, NULL);

    // If there's no package, early out.
    if (result == APPMODEL_ERROR_NO_PACKAGE)
    {
        pstrPackageIdentityName->Reset();
        goto Cleanup;
    }

    // If the failure wasn't insufficient buffer, something else went wrong
    if (result != ERROR_INSUFFICIENT_BUFFER)
    {
        IFC(HRESULT_FROM_WIN32(result));
    }

    pBuffer = new XBYTE[bufferLength];

    result = GetCurrentPackageId(&bufferLength, pBuffer);

    if (result != ERROR_SUCCESS)
    {
        IFC(HRESULT_FROM_WIN32(result));
    }

    pPackageId = reinterpret_cast<PACKAGE_ID*>(pBuffer);

    IFC(xstring_ptr::CloneBuffer(
        pPackageId->name,
        xstrlen(pPackageId->name),
        pstrPackageIdentityName));

Cleanup:
    delete[] pBuffer;
    RRETURN(hr);
}
