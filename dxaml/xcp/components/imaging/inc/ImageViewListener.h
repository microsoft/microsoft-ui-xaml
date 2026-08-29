// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class ImageViewBase;

struct IImageViewListener
{
    // Called on the UI thread to notify about the new data or error status available from the view.
    virtual _Check_return_ HRESULT OnImageViewUpdated(ImageViewBase& sender) = 0;
};
