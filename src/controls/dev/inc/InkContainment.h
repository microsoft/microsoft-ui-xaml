// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "FrameworkUdk/Containment.h"

// Bug 63503009: [2.0-experimental Servicing] InkCanvas/InkToolbar inking support (RCC: InkCanvas_InkingSupport)
#ifndef WINAPPSDK_CHANGEID_63503009
#define WINAPPSDK_CHANGEID_63503009 63503009
#endif

// Inking (InkCanvas, InkToolbar and their family) is entirely new public surface shipped via
// servicing, so an app pinned to an earlier patch level must not be able to construct any of it.
// Gating DllGetActivationFactory alone is not enough: XAML markup instantiates types through the
// generated metadata provider, which calls WINRT_GetActivationFactory directly and so never passes
// through that export. Calling this from every activatable constructor makes construction fail with
// E_NOTIMPL when the change is disabled. Base constructors run before derived ones, so gating a base
// (e.g. InkToolbarToolButton) also contains every type derived from it.
inline void ThrowIfInkingContainmentDisabled()
{
    if (!WinAppSdk::Containment::IsChangeEnabled<WINAPPSDK_CHANGEID_63503009>())
    {
        throw winrt::hresult_not_implemented();
    }
}
