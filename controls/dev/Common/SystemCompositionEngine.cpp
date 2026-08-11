// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "SystemCompositionEngine.h"

bool SystemCompositionEngine::IsEnabledForProcess()
{
    // The engine is picked once, before the first composition object exists, so this can't change afterwards.
    // Every caller on every thread agrees for the lifetime of the process.
    static const bool isEnabled = []()
    {
        auto compositor = winrt::CompositionTarget::GetCompositorForCurrentThread();

        // GetForSystemEngine takes any composition object (IInspectable) and returns null unless that object
        // belongs to the system engine, so pass the compositor directly rather than allocating a throwaway
        // visual just to probe the engine. CompositionEngine lives in the Microsoft.UI.Composition namespace
        // (it was promoted out of the Experimental namespace in the InteractiveExperiences transport).
        return winrt::Microsoft::UI::Composition::CompositionEngine::GetForSystemEngine(compositor) != nullptr;
    }();

    return isEnabled;
}
