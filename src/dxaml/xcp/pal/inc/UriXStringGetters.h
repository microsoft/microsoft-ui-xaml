// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Declares helper getters for IPALUri fields that return xstring_ptrs instead of raw buffers.

#pragma once

#ifndef URI_XSTRING_GETTERS_H
#define URI_XSTRING_GETTERS_H

namespace UriXStringGetters
{
    _Check_return_ HRESULT GetScheme(
        _In_ const IPALUri *pUri,
        _Out_ xstring_ptr* pstrScheme
        );

    _Check_return_ HRESULT GetHost(
        _In_ const IPALUri *pUri,
        _Out_ xstring_ptr* pstrHost
        );

    _Check_return_ HRESULT GetPath(
        _In_ const IPALUri *pUri,
        _Out_ xstring_ptr* pstrPath
        );

    _Check_return_ HRESULT GetFilePath(
        _In_ const IPALUri *pUri,
        _Out_ xstring_ptr* pstrPath
        );
}

#endif
