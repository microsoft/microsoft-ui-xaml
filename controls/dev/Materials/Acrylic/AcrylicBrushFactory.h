// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "AcrylicBrush.h"

class AcrylicBrushFactory
    : public winrt::factory_implementation::AcrylicBrushT<AcrylicBrushFactory, AcrylicBrush>
{
public:
    AcrylicBrushFactory();
};

namespace winrt::Microsoft::UI::Xaml::Media
{
    namespace factory_implementation { using AcrylicBrush = ::AcrylicBrushFactory; };
    namespace implementation { using AcrylicBrush = ::AcrylicBrush; };
}
