// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Declares feature areas (i.e. WPP flags) for debugging-centric 
//      ETW traces.

#pragma once

#define WPP_CONTROL_GUIDS \
    WPP_DEFINE_CONTROL_GUID(WindowsUIXaml, (CB18E7B3,F5B0,412f,9F18,5D87FEFCD663), \
        WPP_DEFINE_BIT(Common) \
        WPP_DEFINE_BIT(DManip_Compositor) \
        WPP_DEFINE_BIT(DManip_InputManager) \
        WPP_DEFINE_BIT(DManip_InputManagerViewport) \
        WPP_DEFINE_BIT(DManip_PALService) \
        WPP_DEFINE_BIT(DManip_PALServiceViewportEventHandler) \
        WPP_DEFINE_BIT(DManip_ScrollViewer) \
        WPP_DEFINE_BIT(DManip_ScrollContentPresenter) \
        WPP_DEFINE_BIT(ResourceLoading) \
        WPP_DEFINE_BIT(ListViewBaseItemChrome) \
        )

// The following lines are required to enable our custom log function DoTraceLevelMessage which takes both a level and flags.
#define WPP_LEVEL_FLAGS_LOGGER(lvl, flags) WPP_LEVEL_LOGGER(flags)  
#define WPP_LEVEL_FLAGS_ENABLED(lvl, flags) (WPP_LEVEL_ENABLED(flags) && WPP_CONTROL(WPP_BIT_ ## flags).Level >= lvl)  
