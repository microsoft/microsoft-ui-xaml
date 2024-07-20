// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

enum class InitializationType
{
    //
    //  This initialization type  is used when
    //  none of the other explict types
    //  make sense. In a practical sense this is
    //  for initialization of non-main view in
    //  foreground apps.
    //
    Normal,
    //
    //  This initialization type is used when
    //  the initialization is for main view in
    //  the foreground app.
    //
    MainView,
    //
    //  This initialization type is used when
    //  the initialization is for xbf..
    //
    Xbf,
    //
    //  This initialization type is used when
    //  the initialization is for background tasks.
    //
    BackgroundTask,
    //
    // Xaml core is started only to host islands (StartOnCurrentThread API).
    // This is also the mode used by WinUI Desktop apps.
    //
    IslandsOnly,
    //
    //  This Initialization type is used when
    //  we shutdown the core in between test runs
    //  and we need to get the core back to an
    //  initialized state from an idle state
    //
    FromIdle,
};

enum class DeinitializationType
{
    //
    //  This deinitialization type is used in all
    //  cases where we aren't keeping the core
    //  in a state that we can recover from and is
    //  the default shutdown path.
    //
    Default,
    //
    //  This deinitialization type is used when
    //  we shutdown the core in between test runs
    //  and we want to get the core to an "idle"
    //  state
    //
    ToIdle,
};
