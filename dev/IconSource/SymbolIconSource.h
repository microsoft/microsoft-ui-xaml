// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "IconSource.h"
#include "SymbolIconSource.g.h"
#include "SymbolIconSource.properties.h"

class SymbolIconSource : public ReferenceTracker<SymbolIconSource, winrt::implementation::SymbolIconSourceT, IconSource>, public SymbolIconSourceProperties
{
public:
    using SymbolIconSourceProperties::ClearProperties;
    using SymbolIconSourceProperties::EnsureProperties;

    winrt::DependencyProperty GetIconElementPropertyCore(winrt::DependencyProperty sourceProperty);
    winrt::IconElement CreateIconElementCore();
};
