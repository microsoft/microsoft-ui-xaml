// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// It is list of feature flags for XAML features
// The values are same as default values for shipped 19h1 release

#pragma once

#ifdef __midlrt
namespace features
{
    // This section is processed by MIDL.  When a feature is marked "DisabledByDefault",
    // MIDL will include these in the WinMD and mark them with the "Experimental" attribute.
    feature_name Feature_XamlMotionSystemHoldbacks = { AlwaysDisabled, FALSE };
    feature_name Feature_Xaml2018 = { AlwaysDisabled, FALSE };
    feature_name Feature_CommandingImprovements = { AlwaysDisabled, FALSE };
    feature_name Feature_WUXCPreviewTypes = { AlwaysDisabled, FALSE };
    feature_name Feature_UwpSupportApi = { AlwaysDisabled, FALSE };

#ifdef MUX_PRERELEASE
    // In prerelease, the API is "experimental"
    feature_name Feature_InputValidation = { DisabledByDefault, FALSE };
    feature_name Feature_UiaEndpointsForContentIslands = { DisabledByDefault, FALSE };
    feature_name Feature_ExperimentalApi = { DisabledByDefault, FALSE };
#else
    // In release, the experimental feature is disabled, removing it from the final WinMD,
    // but preserving it in the private WinMD.
    feature_name Feature_InputValidation = { AlwaysDisabled, FALSE };
    feature_name Feature_UiaEndpointsForContentIslands = { AlwaysDisabled, FALSE };
    feature_name Feature_ExperimentalApi = { AlwaysDisabled, FALSE };
#endif

    feature_name Feature_HeaderPlacement = { AlwaysDisabled, TRUE };
}
#else

#define WI_IS_FEATURE_PRESENT(FeatureName) (__INTERNAL_FEATURE_PRESENT_##FeatureName == 1)

namespace Feature_XamlMotionSystemHoldbacks
{
    #define __INTERNAL_FEATURE_PRESENT_Feature_XamlMotionSystemHoldbacks 0
    inline bool __cdecl IsEnabled()
    {
        return false;
    }
}

namespace Feature_Xaml2018
{
    #define __INTERNAL_FEATURE_PRESENT_Feature_Xaml2018 0
    inline bool __cdecl IsEnabled()
    {
        return false;
    }
}

namespace Feature_CommandingImprovements
{
    #define __INTERNAL_FEATURE_PRESENT_Feature_CommandingImprovements 0
    inline bool __cdecl IsEnabled()
    {
        return false;
    }
}

namespace Feature_WUXCPreviewTypes
{
    #define __INTERNAL_FEATURE_PRESENT_Feature_WUXCPreviewTypes 0
    inline bool __cdecl IsEnabled()
    {
        return false;
    }
}

namespace Feature_WUCShapes
{
    #define __INTERNAL_FEATURE_PRESENT_Feature_WUCShapes 0
    inline bool __cdecl IsEnabled()
    {
        return false;
    }
}

namespace Feature_HeaderPlacement
{
    #define __INTERNAL_FEATURE_PRESENT_Feature_HeaderPlacement 0
    inline bool __cdecl IsEnabled()
    {
        return false;
    }
}

namespace Feature_RunawayTextureList
{
    #define __INTERNAL_FEATURE_PRESENT_Feature_RunawayTextureList 0
    inline bool __cdecl IsEnabled()
    {
        return false;
    }
}

// Below apis are not release ready, hence we have marked them as experimental in winmd in preview build and removed from winmd in final release of WinUI. 
// This feature is always enabled, so that we can use these features in preview builds.

namespace Feature_InputValidation
{
    #define __INTERNAL_FEATURE_PRESENT_Feature_InputValidation 1
    inline bool __cdecl IsEnabled()
    {
        return true;
    }
}

namespace Feature_UwpSupportApi
{
    #define __INTERNAL_FEATURE_PRESENT_Feature_UwpSupportApi 1
    inline bool __cdecl IsEnabled()
    {
        return !!__INTERNAL_FEATURE_PRESENT_Feature_UwpSupportApi;
    }
}

namespace Feature_UiaEndpointsForContentIslands
{
    #define __INTERNAL_FEATURE_PRESENT_Feature_UiaEndpointsForContentIslands 1
    inline bool __cdecl IsEnabled()
    {
        return !!__INTERNAL_FEATURE_PRESENT_Feature_UiaEndpointsForContentIslands;
    }
}

namespace Feature_ExperimentalApi
{
    #define __INTERNAL_FEATURE_PRESENT_Feature_ExperimentalApi 1
    inline bool __cdecl IsEnabled()
    {
        return !!__INTERNAL_FEATURE_PRESENT_Feature_ExperimentalApi;
    }
}

#endif  // __midlrt
