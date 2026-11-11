// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "FrameworkUdk/Containment.h"

// Bug 63518980: [2.0-experimental Servicing] TableView split binary activation carve-out (RCC: TableView_TabularBinaryActivation)
#ifndef WINAPPSDK_CHANGEID_63518980
#define WINAPPSDK_CHANGEID_63518980 63518980
#endif

// Everything in this binary is new public surface, so apps pinned to an earlier patch level must
// not be able to construct any of it. Gating DllGetActivationFactory alone is not enough: XAML
// markup instantiates types through the generated metadata provider, which calls
// WINRT_GetActivationFactory directly and so never passes through that export.
inline void ThrowIfTabularContainmentDisabled()
{
    if (!WinAppSdk::Containment::IsChangeEnabled<WINAPPSDK_CHANGEID_63518980>())
    {
        throw winrt::hresult_not_implemented();
    }
}
