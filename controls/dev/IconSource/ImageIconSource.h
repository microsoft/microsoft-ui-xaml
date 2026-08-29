// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ImageIconSource.g.h"
#include "ImageIconSource.properties.h"

class ImageIconSource :
    public ReferenceTracker<ImageIconSource, DeriveFromBitmapIconSourceHelper_base, winrt::ImageIconSource>,
    public ImageIconSourceProperties
{
public:
    using ImageIconSourceProperties::EnsureProperties;
    using ImageIconSourceProperties::ClearProperties;

    winrt::DependencyProperty GetIconElementPropertyCore(winrt::DependencyProperty sourceProperty);
    winrt::IconElement CreateIconElementCore();
};
