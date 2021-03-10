// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "IconSource.h"
#include "BitmapIconSource.g.h"
#include "BitmapIconSource.properties.h"

class BitmapIconSource :
    public ReferenceTracker<BitmapIconSource, winrt::implementation::BitmapIconSourceT, IconSource>,
    public BitmapIconSourceProperties
{
public:
    using BitmapIconSourceProperties::EnsureProperties;
    using BitmapIconSourceProperties::ClearProperties;

    winrt::IconElement CreateIconElementCore();
};
