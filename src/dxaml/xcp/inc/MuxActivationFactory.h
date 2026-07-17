// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// Fast activation factory lookup for Microsoft.UI.* types.
//
// MuxGetActivationFactory is a drop-in replacement for wf::GetActivationFactory
// (or RoGetActivationFactory). For known Microsoft.UI.* namespaces it bypasses
// combase and calls DllGetActivationFactory directly on the target DLL, which
// is significantly faster. Unknown namespaces and any errors fall back to the
// normal RoGetActivationFactory path.
//
// This is gated behind WinAppSDK containment -- when disabled, it goes
// straight to RoGetActivationFactory.
//
// Usage:
//   #include <MuxActivationFactory.h>
//
//   // Instead of:
//   //   wf::GetActivationFactory(hstring, &comPtr);
//   // Use:
//   MuxGetActivationFactory(hstring, &comPtr);
//

#pragma once

#include <roapi.h>
#include <wrl/client.h>

// Non-template core -- calls into ActivationFactoryCache singleton.
HRESULT MuxGetActivationFactoryImpl(
    _In_ HSTRING activatableClassId,
    _In_ REFIID iid,
    _COM_Outptr_ void** factory);

// Template overload for raw pointer output (T**).
template <typename T>
inline HRESULT MuxGetActivationFactory(_In_ HSTRING activatableClassId, _COM_Outptr_ T** factory)
{
    return MuxGetActivationFactoryImpl(activatableClassId, __uuidof(T), reinterpret_cast<void**>(factory));
}

// Template overload for ComPtr output (matches wf::GetActivationFactory signature).
template <typename T>
inline HRESULT MuxGetActivationFactory(
    _In_ HSTRING activatableClassId,
    Microsoft::WRL::Details::ComPtrRef<T> factory)
{
    return MuxGetActivationFactoryImpl(
        activatableClassId,
        __uuidof(typename T::InterfaceType),
        reinterpret_cast<void**>(factory.ReleaseAndGetAddressOf()));
}
