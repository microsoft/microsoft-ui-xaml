// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CCoreServices;
class ImageDecodeParams;
struct ImageMetadata;

namespace ImageDecoderFactory
{
    _Check_return_ HRESULT CreateDecoder(
        _In_ CCoreServices* core,
        _In_ const EncodedImageData& encodedImageData,
        _Out_ std::unique_ptr<IImageDecoder>& decoder);
}