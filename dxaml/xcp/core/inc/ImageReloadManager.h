// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CImageSource;
enum class ResourceInvalidationReason;

//------------------------------------------------------------------------
//
// CImageReloadManager tracks a set of images that are eligible for 
// reloading when a resource invalidation occurs.
//
//------------------------------------------------------------------------
class CImageReloadManager
{
public:
    void AddImage(_In_ CImageSource* image);
    void RemoveImage(_In_ CImageSource* image);
    void ClearImages();

    _Check_return_ HRESULT ReloadImages(ResourceInvalidationReason reason);

private:
    //
    // Images may not be unregistered properly. Use weak refs to check for dangling pointers.
    //
    // ImageSource unregister themselves in the destructor (among other places). At destruction time, the ImageSource
    // will have been detached from the island that they were originally registered with, and can't walk back up to
    // find the correct ContentRoot/RootScale objects associated with that island to unregister. So instead, we take
    // weak refs here so that we can check whether an ImageSource is still alive before we update it. If not, we just
    // remove it from the registered list.
    //
    // The alternative is for ImageSource to keep a pointer back to the CImageReloadManager that it originally
    // registered with, so that it can find it even when it's detached from the island. That can potentially create
    // lifetime problems if the ImageSource outlives the CImageReloadManager, though. The simpler and safer fix is
    // for the CImageReloadManager to check before calling in.
    //
    std::vector<xref::weakref_ptr<CImageSource>> m_images;
};


