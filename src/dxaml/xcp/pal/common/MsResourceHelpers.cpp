// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <EncodedPtr.h>
#include <pal.h>
#include "XStringBuilder.h"

namespace MsUriHelpers
{
    _Check_return_
    bool IsMsResourceUri(_In_ const xstring_ptr_view& strUri)
    {
        return strUri.StartsWith(XSTRING_PTR_EPHEMERAL(L"ms-resource"), xstrCompareCaseInsensitive);
    }

    _Check_return_
    HRESULT IsMsResourceUri(
        _In_ const IPALUri *pUri,
        _Out_ bool *pResult
        )
    {
        HRESULT hr = S_OK;
        XUINT32 length = 0;
        WCHAR *pScheme = NULL;

        *pResult = FALSE;

        IFC(pUri->GetScheme(&length, NULL));
        if (length != xstrlen(L"ms-resource"))
        {
            goto Cleanup;
        }

        pScheme = new WCHAR[++length];
        IFC(pUri->GetScheme(&length, pScheme));

        *pResult = xstrncmpi(pScheme, L"ms-resource", length) == 0;

    Cleanup:
        delete[] pScheme;
        RRETURN(hr);
    }

    //---------------------------------------------------------------------------
    //
    //  Synopsis:
    //      Extracts a resource map specifier, a file path and a resource path out of an ms-resource
    //      URI.
    //
    //  Examples:
    //      ms-resource:///Files/dir/foo.png -> empty resource map, "dir/foo.png" file path, "Files/dir/foo.png" resource path.
    //      ms-resource://MyControls/Files/foo.png -> "MyControls" resource map, "foo.png" path.
    //      ms-resource://MyApp/Resources/String1 -> "MyApp", NULL, "Resources/String1"
    //
    //---------------------------------------------------------------------------
    _Check_return_
    HRESULT CrackMsResourceUri(
        _In_ const IPALUri *pUri,
        _Out_ xstring_ptr* pstrResourceMap,
        _Out_opt_ xstring_ptr* pstrResourcePath,
        _Out_opt_ xstring_ptr* pstrFilePath,
        _Out_opt_ bool* pHadFilePath
        )
    {
        HRESULT hr = S_OK;
        XStringBuilder stringBuilder;
        XUINT32 bufferLength = 0;
        WCHAR *pBufferNoRef = NULL;
        xephemeral_string_ptr strPathPrefixes[2];

        XSTRING_PTR_EPHEMERAL(L"Files/").Demote(&strPathPrefixes[0]);
        XSTRING_PTR_EPHEMERAL(L"/Files/").Demote(&strPathPrefixes[1]);

        if (pstrResourcePath)
        {
            pstrResourcePath->Reset();
        }

        if (pstrFilePath)
        {
            pstrFilePath->Reset();
        }

        if (pHadFilePath)
        {
            *pHadFilePath = FALSE;
        }

        // Extract the host as the resource map
        IFC(pUri->GetHost(&bufferLength, NULL));
        ++bufferLength;
        IFC(stringBuilder.InitializeAndGetFixedBuffer(bufferLength, &pBufferNoRef));
        IFC(pUri->GetHost(&bufferLength, pBufferNoRef));
        stringBuilder.SetFixedBufferCount(bufferLength);
        IFC(stringBuilder.DetachString(pstrResourceMap));

        if (pstrResourcePath || pstrFilePath)
        {
            xstring_ptr strResourcePath;

            // Extract the resource path
            IFC(pUri->GetPath(&bufferLength, NULL));
            ++bufferLength;
            IFC(stringBuilder.InitializeAndGetFixedBuffer(bufferLength, &pBufferNoRef));
            IFC(pUri->GetPath(&bufferLength, pBufferNoRef));
            stringBuilder.SetFixedBufferCount(bufferLength);
            IFC(stringBuilder.DetachString(&strResourcePath));

            if (pstrResourcePath)
            {
                *pstrResourcePath = strResourcePath;
            }

            if (pstrFilePath)
            {
                // Try to interpret the resource path as a file path.
                // Remove the Files/ or /Files/ prefix.
                for (size_t i = 0; i < ARRAY_SIZE(strPathPrefixes); ++i)
                {
                    if (strResourcePath.StartsWith(strPathPrefixes[i], xstrCompareCaseInsensitive))
                    {
                        IFC(strResourcePath.SubString(
                            strPathPrefixes[i].GetCount(),
                            strResourcePath.GetCount(),
                            pstrFilePath));

                        if (pHadFilePath)
                        {
                            *pHadFilePath = TRUE;
                        }

                        goto Cleanup;
                    }
                }

                // If we reached this point, this is a string URI, which has no file path.
            }
        }

    Cleanup:
        RRETURN(hr);
    }

    _Check_return_
    HRESULT CrackMsResourceUri(
        _In_ const xstring_ptr_view &strUri,
        _Out_ xstring_ptr* pstrResourceMap,
        _Out_opt_ xstring_ptr* pstrResourcePath,
        _Out_opt_ xstring_ptr* pstrFilePath,
        _Out_opt_ bool* pHadFilePath
        )
    {
        HRESULT hr = S_OK;
        IPALUri *pUri = NULL;

        IFC(gps->UriCreate(strUri.GetCount(), strUri.GetBuffer(), &pUri));
        IFC(CrackMsResourceUri(pUri, pstrResourceMap, pstrResourcePath, pstrFilePath, pHadFilePath));

    Cleanup:
        ReleaseInterface(pUri);
        RRETURN(hr);
    }

    xephemeral_string_ptr& GetAppxScheme()
    {
        static auto scheme = XSTRING_PTR_EPHEMERAL(L"ms-appx");

        return scheme;
    }

    xephemeral_string_ptr& GetAppDataScheme()
    {
        static auto scheme = XSTRING_PTR_EPHEMERAL(L"ms-appdata");

        return scheme;
    }

    xephemeral_string_ptr& GetResourceScheme()
    {
        static auto scheme = XSTRING_PTR_EPHEMERAL(L"ms-resource");

        return scheme;
    }
}
