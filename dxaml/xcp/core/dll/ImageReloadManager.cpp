// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

//------------------------------------------------------------------------
//
//  Synopsis:
//      Adds the image to this reload manager. When a resource invalidation
//      occurs, ReloadOnResourceInvalidation() will be invoked on the image.
//
//------------------------------------------------------------------------
void CImageReloadManager::AddImage(_In_ CImageSource* image)
{
    m_images.emplace_back(xref::get_weakref(image));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Removes the image from this reload manager. This is supported
//      in the middle of a ReloadImages() call but won't affect the operation
//      of that call.
//
//------------------------------------------------------------------------
void CImageReloadManager::RemoveImage(_In_ CImageSource* image)
{
    m_images.erase(
        std::remove_if(
            m_images.begin(),
            m_images.end(),
            [image](xref::weakref_ptr<CImageSource>& element) { return (element == image); }),
        m_images.end()
        );
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Clears all images from this reload manager. This is supported
//      in the middle of a ReloadImages() call but won't affect the operation
//      of that call.
//
//------------------------------------------------------------------------
void CImageReloadManager::ClearImages()
{
    m_images.clear();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Calls ReloadOnResourceInvalidation() for each image tracked by
//      this ImageReloadManager.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CImageReloadManager::ReloadImages(ResourceInvalidationReason reason)
{
    // Remove all the dead CImageSources first. A CImageSource will fail to unregister itself if it attempts to
    // unregister during its dtor - it's already been removed from the tree and can't walk up to the island that
    // it originally registered with.
    m_images.erase(
        std::remove_if(
            m_images.begin(),
            m_images.end(),
            [](xref::weakref_ptr<CImageSource>& i) { return i.expired(); }),
        m_images.end()
        );

    // Make a temporary copy of the image list. This allows images to synchronously call back
    // into this reload manager to remove themselves from inside CBitmapImage::ReloadOnResourceInvalidation(),
    // if they need to.
    std::vector<xref::weakref_ptr<CImageSource>> copy(m_images);

    // Call ReloadOnResourceInvalidation() for each image.
    for (const auto& element : copy)
    {
        const auto& image = element.lock();
        if (image)
        {
            IFC_RETURN(image->ReloadOnResourceInvalidation(reason));
        }
        // If the image is a dead pointer, it means it was added after we made the copy and while we were calling
        // out to the remaining CImageSources. Leave it in - we'll remove it the next time we reload the images.
    }

    return S_OK;
}


