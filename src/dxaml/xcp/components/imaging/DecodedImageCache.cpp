// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <DecodedImageCache.h>
#include <ImageDecodeParams.h>
#include <ImageProviderInterfaces.h>
#include <OfferableSoftwareBitmap.h>

DecodedImageCache::DecodedImageCache(
    _In_ IInvalidateDecodedImageCacheCallback* callback,
    _In_ xref_ptr<ImageDecodeParams> decodeParams,
    _In_ OfferableSoftwareBitmap* softwareBitmapNoRef
    )
    : m_callback(callback)
    , m_decodeParams(std::move(decodeParams))
    , m_softwareBitmapNoRef(softwareBitmapNoRef)
{
    CNotifyOnDelete* notification = nullptr;
    IFCFAILFAST(m_softwareBitmapNoRef->GetNotifyOnDelete(&notification));

    notification->AddOnDeleteCallback(this, xstring_ptr::NullString());
}

DecodedImageCache::~DecodedImageCache()
{
    if (m_softwareBitmapNoRef)
    {
        CNotifyOnDelete* notification = NULL;
        IFCFAILFAST(m_softwareBitmapNoRef->GetNotifyOnDelete(&notification));

        notification->RemoveOnDeleteCallback(this);
    }
}

// Handler for the image surface becoming invalid.
void DecodedImageCache::OnDelete(_In_ const xstring_ptr& token)
{
    m_softwareBitmapNoRef = nullptr;
    IFCFAILFAST(m_callback->OnCacheInvalidated(this));
}
