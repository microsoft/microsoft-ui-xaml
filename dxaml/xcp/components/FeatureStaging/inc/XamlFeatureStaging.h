// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// Velocity feature gates for WinUI 3.
//
// This header intentionally exposes only plain functions. The generated Velocity header and the
// internal WIL headers it pulls in stay contained inside the Microsoft.UI.Xaml.FeatureStaging static
// library, so the rest of the product keeps building against the public
// Microsoft.Windows.ImplementationLibrary package.
//
// To add a feature: add it to FeatureStaging-Microsoft.UI.Xaml.featurestaging (with a feature ID
// assigned by the Velocity team), then add a matching accessor here and in XamlFeatureStaging.cpp.
//
// Do NOT confuse these with the compile-time constants in xcp\inc\FeatureFlags.h. Those are baked in
// when WinUI is built. These are resolved on the user's machine, at runtime, from the Velocity
// configuration, which is what makes cloud-controlled rollout possible.

namespace XamlVelocity
{
    // Pilot feature used to validate the Velocity end-to-end loop in Microsoft.UI.Xaml.dll.
    // Stage: DisabledByDefault.
    //
    // Safe to call from any thread. Never throws and never fails: if the Velocity feature store is
    // unavailable (for example on a downlevel OS), this returns the compiled-in default state.
    bool IsXamlVelocityPilotEnabled() noexcept;
}
