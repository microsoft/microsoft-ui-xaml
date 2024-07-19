// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// Copyright (c) Microsoft Corporation.  All rights reserved.

#pragma once

namespace RuntimeFeatureBehavior
{
    // The set of possible RuntimeFeatures kept in sync with RuntimeFeatureData.cpp
    enum class RuntimeEnabledFeature
    {
        TestDisabledByDefaultFeature = 0,
        TestEnabledByDefaultFeature,
        TestGetFeatureValue,
        ParserCustomWriter,
        XamlDiagnostics,
        EnforceXbfV2Stream,
        UsePrivateHeap,
        DrawDWriteTextLayoutInGreen, // Turns on a debugging mode where all text layouted by IDWriteTextLayout are displayed in green color.
        BackgroundThreadImageLoading,
        DisableTransitionsForTest, // Disables state change transitions during testing to reduce execution time.
        EnableStyleOptimization,    // Turns on style flyweighting
        DisableGlobalAnimations, // Causes the framework to report that animations are globally disabled.
        DisableTextBoxCaret,
        EnableAnimatedListViewBaseScrolling,
        ShrinkApplicationViewVisibleBounds,
        EnableCulledFromRenderWalkOptimization,
        EnableFullCompNodeTree,     // Creates 1 CompNode for every UIElement in the core tree.
        EnableVisualDebugTags,   // Adds extra debugging info to WinRT visuals
        EnableTracingCookies,    // Adds ETW logging of Expressions/Property Sets in the DWM
        ForceWarp,
        AllowFailFastOnAnyFailure,  // If enabled, this allows some processes (like Shell) to immediately FailFast on any stowed exception failure.
        ReleaseGraphicsDeviceOnSuspend,
        ReleaseGraphicsAndDCompDevicesOnSuspend,
        EnableOnFocusMouseMode,        // Indicates that WhenFocused should be honored for controls.
        EnableApplicationMouseMode,    // Indicates that apps should be in mouse mode unless requested not to be.
        EnableAutoFocusOverride,
        AutoFocus_Shadow,
        AutoFocus_ManifoldShadow,
        AutoFocus_Distance,
        AutoFocus_SecondaryDistance,
        SlowDownAnimations,
        AnimationFailFast,
        EnableCoreShutdown,
        TraceDCompSurfaces,
        TraceDCompSurfaceUpdates,
        RenderTargetBitmapUsingSpriteVisualsTestMode,
        EnableGlobalAnimations, // Causes the framework to report that animations are enabled globally by the system
        InsertDManipHitTestVisual,
        DumpMockDManipHitTestVisual,
        DoNotSetRootScrollViewerBackground,
        DisableDefaultConnectedAnimationConfiguration,
        EnableWUCShapes,
        EnableDebugD3DDevice,
        SynchronousCompTreeUpdates,         // Skip CompositorTreeHost and update the comp node synchronously
        WindowedPopupActualBoundsMode,      // Use ActualBounds of Popup's child to determine size of windowed popup HWND
        EnableWindowedPopupDebugVisual,
        ForceRoundedCalendarViewBaseItemChrome,
        DenyRoundedCalendarViewBaseItemChrome,
        EnableUWPWindow,                   // if enabled, it will allow creating an UWP window and thus launching UWP apps // task 31614767 : UWP window creation has been disabled
        EnableReentrancyChecks, // If enabled, we will FailFast when reentrancy is detected
        // add entries before here to be safe as C# test code uses fixed numeric values instead of this enum so if added after DBG entry, the enum value can change
        #if DBG
        CaptureTrackerPtrCallStack, // Capture the stack that calls TrackerPtr.Set
        #endif
        ForceProjectedShadowsOnByDefault,
        EnableDropShadowDebugVisual,
        ForceRoundedListViewBaseItemChrome,
        DenyRoundedListViewBaseItemChrome,
        ForceSelectionIndicatorVisualEnabled,
        DenySelectionIndicatorVisualEnabled,
        ForceSelectionIndicatorModeInline,
        ForceSelectionIndicatorModeOverlay,

        // Insert new enum values before this one.
        // This is used to initialize the lengths of the
        // feature data and state arrays.
        LastValue
    };
}
