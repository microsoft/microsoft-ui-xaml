// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

enum PixelFormat;

//
// Generate a string to uniquely identify an image for the purposes of
// caching, based on the given parameters.
//
_Check_return_ HRESULT
GenerateCacheIdentifier(
    _In_ const xstring_ptr_view& canonicalUri,
    XUINT32 decodeWidth,
    XUINT32 decodeHeight,
    PixelFormat pixelFormat,
    bool includeInvalidationId,
    XUINT32 invalidationId,
    _Inout_ xstring_ptr* cacheIdentifier);
