// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <PalNotify.h>

class OfferableSoftwareBitmap;
class ImageDecodeParams;
struct IInvalidateDecodedImageCacheCallback;

class DecodedImageCache : public CXcpObjectBase< INotifyOnDeleteCallback >
{
public:
    DecodedImageCache(
        _In_ IInvalidateDecodedImageCacheCallback* callback,
        _In_ xref_ptr<ImageDecodeParams> decodeParams,
        _In_ OfferableSoftwareBitmap* softwareBitmapNoRef
        );

    const xref_ptr<ImageDecodeParams>& GetDecodeParams() const { return m_decodeParams; }

    _Ret_notnull_ OfferableSoftwareBitmap* GetSurface() const { return m_softwareBitmapNoRef; }

    // INotifyOnDeleteCallback
    void OnDelete(_In_ const xstring_ptr& token) override;

private:
    ~DecodedImageCache() override;

    IInvalidateDecodedImageCacheCallback* m_callback;
    xref_ptr<ImageDecodeParams> m_decodeParams;
    OfferableSoftwareBitmap* m_softwareBitmapNoRef;
};
