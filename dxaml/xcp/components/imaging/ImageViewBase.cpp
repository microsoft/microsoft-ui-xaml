// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <ImageViewBase.h>
#include <ImageViewListener.h>

void ImageViewBase::AddImageViewListener(IImageViewListener& imageViewListener)
{
    ASSERT(std::find(m_imageViewListeners.begin(), m_imageViewListeners.end(), &imageViewListener) == m_imageViewListeners.end());
    m_imageViewListeners.push_back(&imageViewListener);
}

void ImageViewBase::RemoveImageViewListener(IImageViewListener& imageViewListener)
{
    auto it = std::find(m_imageViewListeners.begin(), m_imageViewListeners.end(), &imageViewListener);
    ASSERT(it != m_imageViewListeners.end());
    m_imageViewListeners.erase(it);
}

void ImageViewBase::TriggerViewUpdated()
{
    for (auto listener: m_imageViewListeners)
    {
        VERIFYHR(listener->OnImageViewUpdated(*this));
    }
}
