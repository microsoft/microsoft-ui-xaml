// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "RecyclePool.h"

// For MuxFinal builds we don't include RecyclePool in the WinMD
// and we use custom definition of the dependency properties
#ifdef MUX_PRERELEASE
#include "RecyclePool.properties.cpp"
#else
namespace winrt::Microsoft::UI::Xaml::Controls
{
    CppWinRTActivatableClassWithDPFactory(RecyclePool)
}
#include "RecyclePool.g.cpp"
#endif
