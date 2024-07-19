// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace ImageUtils
{
    HRESULT SaveWICBitmapAsPNG(_In_ IWICBitmapSource *pWICBitmapSource, _In_ LPCWSTR filename);
}

