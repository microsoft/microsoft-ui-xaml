// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <RuntimeEnabledFeatures.h>

// Each of the below entries can be controlled by regkeys.
// To set a value for both native and wow64, run these as admin:
//      reg add HKLM\Software\Microsoft\WinUI\Xaml /v <key-name> /t REG_DWORD /d <value> /reg:32
//      reg add HKLM\Software\Microsoft\WinUI\Xaml /v <key-name> /t REG_DWORD /d <value> /reg:64

namespace RuntimeFeatureBehavior
{
    const RuntimeEnabledFeatureData s_runtimeFeatureData[s_runtimeFeatureLength] =
    {
        // The count and order of entries here must be the same as they are in the RuntimeEnabledFeature enum.
        // FeatureName, Feature, DefaultEnabledState, DisableValue, DefaultDwordValue
        { L"TestDisabledByDefaultFeature", RuntimeEnabledFeature::TestDisabledByDefaultFeature, false, 0, 0 },
        { L"TestEnabledByDefaultFeature", RuntimeEnabledFeature::TestEnabledByDefaultFeature, true, 0, 1 },
        { L"TestGetFeatureValue", RuntimeEnabledFeature::TestGetFeatureValue, false, -1, 0 },
        { L"ParserCustomWriter", RuntimeEnabledFeature::ParserCustomWriter, false, 0, 0 },
        { L"XamlDiagnostics", RuntimeEnabledFeature::XamlDiagnostics, false, 0, 0 },
        { L"EnforceXbfV2Stream", RuntimeEnabledFeature::EnforceXbfV2Stream, false, 0, 0 },
        { L"UsePrivateHeap", RuntimeEnabledFeature::UsePrivateHeap, false, 0, 0 },
        { L"DrawDWriteTextLayoutInGreen", RuntimeEnabledFeature::DrawDWriteTextLayoutInGreen, false, 0, 0 },
        { L"BackgroundThreadImageLoading", RuntimeEnabledFeature::BackgroundThreadImageLoading, true, 0, 1 },
        { L"DisableTransitionsForTest", RuntimeEnabledFeature::DisableTransitionsForTest, false, 0, 0 },
        { L"EnableStyleOptimization", RuntimeEnabledFeature::EnableStyleOptimization, false, 0, 0 },
        { L"DisableGlobalAnimations", RuntimeEnabledFeature::DisableGlobalAnimations, false, 0, 0 },
        { L"DisableTextBoxCaret", RuntimeEnabledFeature::DisableTextBoxCaret, false, 0, 0 },
        { L"EnableAnimatedListViewBaseScrolling", RuntimeEnabledFeature::EnableAnimatedListViewBaseScrolling, false, 0, 0 },
        { L"ShrinkApplicationViewVisibleBounds", RuntimeEnabledFeature::ShrinkApplicationViewVisibleBounds, false, 0, 0 },
        { L"EnableCulledFromRenderWalkOptimization", RuntimeEnabledFeature::EnableCulledFromRenderWalkOptimization, true, 0, 1 },
        { L"EnableFullCompNodeTree", RuntimeEnabledFeature::EnableFullCompNodeTree, false, 0, 0 },
        { L"EnableVisualDebugTags", RuntimeEnabledFeature::EnableVisualDebugTags, false, 0, 0 },
        { L"EnableTracingCookies", RuntimeEnabledFeature::EnableTracingCookies, false, 0, 0 },
        { L"ForceWarp", RuntimeEnabledFeature::ForceWarp, false, 0, 0 },
        { L"AllowFailFastOnAnyFailure", RuntimeEnabledFeature::AllowFailFastOnAnyFailure, false, 0, 0 },
        { L"ReleaseGraphicsDeviceOnSuspend", RuntimeEnabledFeature::ReleaseGraphicsDeviceOnSuspend, false, 0, 0 },
        { L"ReleaseGraphicsAndDCompDevicesOnSuspend", RuntimeEnabledFeature::ReleaseGraphicsAndDCompDevicesOnSuspend, false, 0, 0 },
        { L"EnableOnFocusMouseMode", RuntimeEnabledFeature::EnableOnFocusMouseMode, false, 0, 0 },
        { L"EnableApplicationMouseMode", RuntimeEnabledFeature::EnableApplicationMouseMode, true, 0, 0},
        { L"EnableAutoFocusOverride", RuntimeEnabledFeature::EnableAutoFocusOverride, 0, false },
        { L"AutoFocus_Shadow", RuntimeEnabledFeature::AutoFocus_Shadow, false, -1, 0 },
        { L"AutoFocus_ManifoldShadow", RuntimeEnabledFeature::AutoFocus_ManifoldShadow, false, -1, 0 },
        { L"AutoFocus_Distance", RuntimeEnabledFeature::AutoFocus_Distance, false, -1, 0 },
        { L"AutoFocus_SecondaryDistance", RuntimeEnabledFeature::AutoFocus_SecondaryDistance, false, -1, 0 },
        { L"SlowDownAnimations", RuntimeEnabledFeature::SlowDownAnimations, false, 0, 1 },
        { L"AnimationFailFast", RuntimeEnabledFeature::AnimationFailFast, false, 0, 0 },
        { L"EnableCoreShutdown", RuntimeEnabledFeature::EnableCoreShutdown, false, 0, 0},
        { L"TraceDCompSurfaces", RuntimeEnabledFeature::TraceDCompSurfaces, false, 0, 0 },
        { L"TraceDCompSurfaceUpdates", RuntimeEnabledFeature::TraceDCompSurfaceUpdates, false, 0, 0 },
        { L"RenderTargetBitmapUsingSpriteVisualsTestMode", RuntimeEnabledFeature::RenderTargetBitmapUsingSpriteVisualsTestMode, false, 0, 0 },
        { L"EnableGlobalAnimations", RuntimeEnabledFeature::EnableGlobalAnimations, false, 0, 0 },
        { L"InsertDManipHitTestVisual", RuntimeEnabledFeature::InsertDManipHitTestVisual, true, 0, 0 },
        { L"DumpMockDManipHitTestVisual", RuntimeEnabledFeature::DumpMockDManipHitTestVisual, false, 0, 0 },
        { L"DoNotSetRootScrollViewerBackground", RuntimeEnabledFeature::DoNotSetRootScrollViewerBackground, false, 0, 0 },
        { L"DisableDefaultConnectedAnimationConfiguration", RuntimeEnabledFeature::DisableDefaultConnectedAnimationConfiguration, false, 0, 0 },
        { L"EnableWUCShapes", RuntimeEnabledFeature::EnableWUCShapes, false, 0, 0 },
        { L"EnableDebugD3DDevice", RuntimeEnabledFeature::EnableDebugD3DDevice, false, 0, 0 },
        { L"SynchronousCompTreeUpdates", RuntimeEnabledFeature::SynchronousCompTreeUpdates, false, 0, 0 },
        { L"WindowedPopupActualBoundsMode", RuntimeEnabledFeature::WindowedPopupActualBoundsMode, false, 0, 0 },
        { L"EnableWindowedPopupDebugVisual", RuntimeEnabledFeature::EnableWindowedPopupDebugVisual, false, 0, 0 },
        { L"ForceRoundedCalendarViewBaseItemChrome", RuntimeEnabledFeature::ForceRoundedCalendarViewBaseItemChrome, false, 0, 0 },
        { L"DenyRoundedCalendarViewBaseItemChrome", RuntimeEnabledFeature::DenyRoundedCalendarViewBaseItemChrome, false, 0, 0 },
        { L"EnableUWPWindow", RuntimeEnabledFeature::EnableUWPWindow, false, 0, 0 },
        { L"EnableReentrancyChecks", RuntimeEnabledFeature::EnableReentrancyChecks, false, 0, 0 },
        #if DBG
        { L"CaptureTrackerPtrCallStack", RuntimeEnabledFeature::CaptureTrackerPtrCallStack, false, 0, 0 },
        #endif
        { L"ForceProjectedShadowsOnByDefault", RuntimeEnabledFeature::ForceProjectedShadowsOnByDefault, false, 0, 0 },
        { L"EnableDropShadowDebugVisual", RuntimeEnabledFeature::EnableDropShadowDebugVisual, false, 0, 0 },
        { L"ForceRoundedListViewBaseItemChrome", RuntimeEnabledFeature::ForceRoundedListViewBaseItemChrome, false, 0, 0 },
        { L"DenyRoundedListViewBaseItemChrome", RuntimeEnabledFeature::DenyRoundedListViewBaseItemChrome, false, 0, 0 },
        { L"ForceSelectionIndicatorVisualEnabled", RuntimeEnabledFeature::ForceSelectionIndicatorVisualEnabled, false, 0, 0 },
        { L"DenySelectionIndicatorVisualEnabled", RuntimeEnabledFeature::DenySelectionIndicatorVisualEnabled, false, 0, 0 },
        { L"ForceSelectionIndicatorModeInline", RuntimeEnabledFeature::ForceSelectionIndicatorModeInline, false, 0, 0 },
        { L"ForceSelectionIndicatorModeOverlay", RuntimeEnabledFeature::ForceSelectionIndicatorModeOverlay, false, 0, 0 },

        // Until the feature is complete, we will default the typographic model back to the legacy one.
        { L"DisableDWriteTypographicModel", RuntimeEnabledFeature::DisableDWriteTypographicModel, false, 0, 0 },
        { L"ForceDWriteTypographicModel", RuntimeEnabledFeature::ForceDWriteTypographicModel, false, 0, 0 },
    };
}
