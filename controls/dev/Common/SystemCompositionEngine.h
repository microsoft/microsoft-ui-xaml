// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// WinUI renders through the lifted compositor by default, but an app can opt the process into the system
// composition engine by calling CompositionEngine.TrySetProcessEngine(CompositionEngineType.System). It has to
// do that before creating any composition object, so the choice is fixed for the lifetime of the process.
//
// The engine matters wherever a feature is implemented against one compositor and has to take a different route
// on the other - see InkCanvas (visual splicing) and MicaBackdrop (window backdrops).
namespace SystemCompositionEngine
{
    // True when this process renders through the system composition engine rather than the lifted one.
    // Must be called on a thread with a Xaml compositor.
    bool IsEnabledForProcess();
}
