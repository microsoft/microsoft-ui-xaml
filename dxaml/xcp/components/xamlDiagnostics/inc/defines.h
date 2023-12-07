// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "minerror.h"


typedef HRESULT(__stdcall *GetClassObjectMethod)(_In_ REFCLSID rclsid, _In_ REFIID riid, _Outptr_ LPVOID* ppv);
typedef std::function<HRESULT()> DispatcherMethod;

#define CHECKCOLOR(name)                                       \
    if (SUCCEEDED(spColorsStatics->get_##name(&knownColor)) && \
        knownColor.A == color.A &&                             \
        knownColor.R == color.R &&                             \
        knownColor.G == color.G &&                             \
        knownColor.B == color.B)                               \
        {                                                          \
        return L#name;                                         \
        }                                                          \

#define CHECKFONTWEIGHT(name) \
    if (SUCCEEDED(spFontWeightsStatics->get_##name(&knownWeight)) && \
        knownWeight.Weight == fontWeight.Weight)                     \
        {                                                                \
        return L#name;                                               \
        }                                                                \

