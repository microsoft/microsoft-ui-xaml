// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Declares helper functions for ms-resource:// URIs and other ms-<scheme> URIs.

#pragma once

struct IPALUri;

namespace MsUriHelpers
{
    _Check_return_
    bool IsMsResourceUri(_In_ const xstring_ptr_view& strUri);

    _Check_return_
    HRESULT IsMsResourceUri(
        _In_ const IPALUri *pUri,
        _Out_ bool *pResult
        );

    _Check_return_
    HRESULT CrackMsResourceUri(
        _In_ const xstring_ptr_view& strUri,
        _Out_ xstring_ptr* pstrResourceMap,
        _Out_opt_ xstring_ptr* pstrResourcePath,
        _Out_opt_ xstring_ptr* pstrFilePath,
        _Out_opt_ bool* pHadFilePath
        );

    _Check_return_
    HRESULT CrackMsResourceUri(
        _In_ const IPALUri *pUri,
        _Out_ xstring_ptr* pstrResourceMap,
        _Out_opt_ xstring_ptr* pstrResourcePath,
        _Out_opt_ xstring_ptr* pstrFilePath,
        _Out_opt_ bool* pHadFilePath
        );

    xephemeral_string_ptr& GetAppxScheme();
    xephemeral_string_ptr& GetAppDataScheme();
    xephemeral_string_ptr& GetResourceScheme();
}
