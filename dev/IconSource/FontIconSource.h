// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "IconSource.h"
#include "FontIconSource.g.h"
#include "FontIconSource.properties.h"

static constexpr auto c_fontIconSourceDefaultFontFamily{ L"Segoe MDL2 Assets"sv };

class FontIconSource :
    public ReferenceTracker<FontIconSource, winrt::implementation::FontIconSourceT, IconSource>,
    public FontIconSourceProperties
{
public:
    using FontIconSourceProperties::EnsureProperties;
    using FontIconSourceProperties::ClearProperties;

    winrt::IconElement CreateIconElementCore();
};
