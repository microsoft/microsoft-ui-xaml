// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "IconSource.g.h"
#include "IconSource.properties.h"

class IconSource : 
    public winrt::implementation::IconSourceT<IconSource, winrt::composable>,
    public IconSourceProperties
{
public:
    winrt::IconElement CreateIconElement();
    virtual winrt::IconElement CreateIconElementCore() = 0;
};
