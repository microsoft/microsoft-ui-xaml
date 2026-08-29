// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <wincodec.h>
#include <ImageProvider.h>
#include "ImageAvailableCallback.h"
#include "ImageCopyParams.h"
#include "SoftwareBitmapSource.h"

// NOTE: Functions that intialize/use core need to be in core/core/elements

_Check_return_ HRESULT CSoftwareBitmapSource::ReloadSource(bool forceCopyToSoftwareSurface)
{
    HRESULT hr = S_OK;
    auto core = GetContext();
    xref_ptr<ImageCopyParams> imageCopyParams;

    auto imageCopyCallback = make_xref<ImageAvailableCallback<CSoftwareBitmapSource>>(this, &CSoftwareBitmapSource::OnSoftwareBitmapImageAvailable);

    IFC(PrepareCopyParams(forceCopyToSoftwareSurface, imageCopyParams));

    // There may be an existing get operation in the case that there is a hardware copy and
    // the need for a software copy comes in.  In this case, just release (but don't abort)
    // the get operation and start the software operation.  The existing operation will
    // still complete (unless the hardware resources were released which controls the early
    // exit condition).  Ideally this should track multiple m_pImageGetOperation's, but ImageSource
    // and SoftwareBitmapSource generally just care about the current in-flight get operation.
    // TODO: This should be cleaned up a bit to allow tracking multiple get operations
    //       or it will be made obsolete based on the todo in SoftwareBitmapSource::ReloadReleasedSoftwareImage
    m_spAbortableImageOperation.reset();

    IFC(core->GetImageProvider()->CopyImage(
        imageCopyParams,
        imageCopyCallback,
        core,
        m_spAbortableImageOperation));

Cleanup:
    return hr;
}