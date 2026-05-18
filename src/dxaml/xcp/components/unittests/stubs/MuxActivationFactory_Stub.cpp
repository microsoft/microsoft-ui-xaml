// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// Stub for unit tests: just forwards to RoGetActivationFactory.
// The real implementation in xcpcore.cpp has a fast-path for known types,
// but unit tests don't link the full DLL so they get this simple fallback.

#include "precomp.h"
#include <roapi.h>

HRESULT MuxGetActivationFactoryImpl(
    _In_ HSTRING activatableClassId,
    _In_ REFIID iid,
    _COM_Outptr_ void** factory)
{
    return RoGetActivationFactory(activatableClassId, iid, factory);
}
