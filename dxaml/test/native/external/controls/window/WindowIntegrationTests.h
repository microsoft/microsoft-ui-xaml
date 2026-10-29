// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace Window {

    class WindowIntegrationTests : public WEX::TestClass<WindowIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(WindowIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"a62e3c8d-69d4-44de-95b5-a62be5062286")
            TEST_CLASS_PROPERTY(L"IsolationLevel", L"Class")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        //
        // Platform:Any
        //
        BEGIN_TEST_METHOD(CanHideAndShowWindow)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can hide and show the window.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanMoveWindow)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can move the window.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"OneCore, WindowsCore") // This scenario is not supported on OneCore.
            TEST_METHOD_PROPERTY(L"Ignore", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanGetSetTitle)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can get and set the window title.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        //
        // Platform:Desktop
        //

        BEGIN_TEST_METHOD(SetTitleInMarkup)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that window title set in markup is correctly handled.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(SetSystemBackdropInMarkup)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that window SystemBackdrop set in markup is correctly handled.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowTopBorderPainting)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the Windows 10 Window ECITB border erase without changing either entry point's geometry.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

#ifdef MUX_PRERELEASE
        // Window placement

        BEGIN_TEST_METHOD(WindowPlacementRestoreNormal)
            TEST_METHOD_PROPERTY(L"Description", L"restore-normal: Reopens a normal window at its saved position and size instead of its fallback dimensions.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowPlacementPersistMovedBounds)
            TEST_METHOD_PROPERTY(L"Description", L"persist-moved-bounds: Reopens at the user's updated position and size, not the initial bounds.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowPlacementRestoreMaximized)
            TEST_METHOD_PROPERTY(L"Description", L"restore-maximized: Reopens maximized and preserves the normal restore bounds.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowPlacementMaximizedRestoreTracksNormalSize)
            TEST_METHOD_PROPERTY(L"Description", L"Width and Height keep the restored client size when switching directly from restored maximized placement to FullScreen or CompactOverlay.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowPlacementRestoreMinimizedNormal)
            TEST_METHOD_PROPERTY(L"Description", L"restore-minimized-normal: Reopens a minimized normal window as normal, preserving its bounds.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowPlacementRestoreMinimizedMaximized)
            TEST_METHOD_PROPERTY(L"Description", L"restore-minimized-maximized: Reopens maximized, not minimized, and preserves normal restore bounds.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowPlacementRestoresOnlyOnFirstActivation)
            TEST_METHOD_PROPERTY(L"Description", L"Repeated Activate calls do not undo a move or resize after the first restore.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowPlacementDoesNotCascadeForSharedId)
            TEST_METHOD_PROPERTY(L"Description", L"A second live window with the same id restores the saved placement without cascading.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowPlacementIdCanBeSetInMarkup)
            TEST_METHOD_PROPERTY(L"Description", L"PlacementId set in markup selects the record used when saving.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowPlacementSavesUsingCurrentId)
            TEST_METHOD_PROPERTY(L"Description", L"Changing PlacementId after activation saves under the current id, not the original id.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowPlacementIdRemainsUsableAfterClose)
            TEST_METHOD_PROPERTY(L"Description", L"PlacementId can still be read, changed, and cleared after the window closes.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowPlacementSettingIdAfterActivationDoesNotRestore)
            TEST_METHOD_PROPERTY(L"Description", L"Setting PlacementId after activation does not restore, even on a later Activate call.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowPlacementClearingIdDisablesSaving)
            TEST_METHOD_PROPERTY(L"Description", L"Clearing PlacementId normalizes it to empty and prevents overwriting the earlier record.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowPlacementIdsAreCaseSensitive)
            TEST_METHOD_PROPERTY(L"Description", L"Ids differing only by case keep independent records and do not restore each other's bounds.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowPlacementFallbackWithoutSavedPlacement)
            TEST_METHOD_PROPERTY(L"Description", L"fallback-without-saved-placement: Uses requested dimensions for a new id, then restores the placement saved on close.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowPlacementRequiresActivate)
            TEST_METHOD_PROPERTY(L"Description", L"Unshown and AppWindow-only windows do not overwrite saved placements.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowPlacementActivateRestoresAfterAppWindowShow)
            TEST_METHOD_PROPERTY(L"Description", L"Window.Activate restores saved bounds even when AppWindow.Show has already shown the window.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowPlacementSavesOnlyConfirmedSessionEnd)
            TEST_METHOD_PROPERTY(L"Description", L"A canceled session end saves nothing; a confirmed session end saves the current bounds.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowPlacementSavesFinalCloseHandlerChanges)
            TEST_METHOD_PROPERTY(L"Description", L"A canceled close saves nothing; the final close saves the id and bounds chosen by the close handler.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowPlacementRejectsInvalidRecords)
            TEST_METHOD_PROPERTY(L"Description", L"Rejects malformed, mismatched and future-version records and falls back to requested dimensions.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowPlacementRecordCompatibility)
            TEST_METHOD_PROPERTY(L"Description", L"Ignores unknown fields and replaces malformed or future-version records only when saving.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowPlacementSkipsUnsupportedPresenters)
            TEST_METHOD_PROPERTY(L"Description", L"Does not restore or save while FullScreen or CompactOverlay is active.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowPlacementCloseDuringActivation)
            TEST_METHOD_PROPERTY(L"Description", L"Closing during initial placement or activation, including nested Activate, does not overwrite saved data.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowPlacementReentrantActivate)
            TEST_METHOD_PROPERTY(L"Description", L"Nested Activate preserves restored bounds and state and allows saving after activation completes.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowPlacementSizeChangedCanOverrideRestoredSize)
            TEST_METHOD_PROPERTY(L"Description", L"Width and Height set during a restore callback override saved size without reapplying old fallback dimensions.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowPlacementSavesDuringSentMessages)
            TEST_METHOD_PROPERTY(L"Description", L"Cross-thread session-end and close messages save final bounds without changing the earlier id's record.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowPlacementSavesOnDestroy)
            TEST_METHOD_PROPERTY(L"Description", L"Window.Close and WM_CLOSE save during WM_DESTROY, not before destruction starts.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowPlacementSavesDuringNativeDestruction)
            TEST_METHOD_PROPERTY(L"Description", L"DestroyWindow, AppWindow.Destroy, and owner destruction save final placement before Window.Close cleans up XAML.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowPlacementNativeDestructionSkipsUnsupportedPresenters)
            TEST_METHOD_PROPERTY(L"Description", L"Native destruction after switching to FullScreen or CompactOverlay preserves the earlier Overlapped placement.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowPlacementAdaptToReplacementDisplay)
            TEST_METHOD_PROPERTY(L"Description", L"adapt-to-replacement-display: Simulates a missing display by editing saved data; restores usable bounds and size without changing the VM's display setup.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowPlacementPreferSameNameDisplay)
            TEST_METHOD_PROPERTY(L"Description", L"prefer-same-name-display: Edits saved coordinates while retaining the display name, then restores usable bounds; does not test competing displays.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowPlacementFitOffscreenBounds)
            TEST_METHOD_PROPERTY(L"Description", L"fit-offscreen-bounds: Edits saved bounds to be partly off-screen, then restores them inside the current work area, allowing native border tolerance.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowPlacementRestoreSnapped)
            TEST_METHOD_PROPERTY(L"Description", L"restore-snapped: Captures and restores left, right and corner snap bounds independently of normal bounds, including custom chrome.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowPlacementCapturesMinimizedSnappedBounds)
            TEST_METHOD_PROPERTY(L"Description", L"restore-minimized-snapped (capture limitation): Documents native capture gaps and preserves earlier records when capture is rejected.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowPlacementRestoreMinimizedSnapped)
            TEST_METHOD_PROPERTY(L"Description", L"restore-minimized-snapped: Edits saved data to represent a minimized snapped window, then reopens snapped, not minimized; native capture gaps are tested separately.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowPlacementMigratesChangedWorkArea)
            TEST_METHOD_PROPERTY(L"Description", L"Migrates normal, maximized and snapped placements when the work area grows or shrinks.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowPlacementRespectsFixedSizing)
            TEST_METHOD_PROPERTY(L"Description", L"Checks native sizing policy for oversized resizable and fixed-size windows without changing IsResizable.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowPlacementPreserveLogicalSize)
            TEST_METHOD_PROPERTY(L"Description", L"preserve-logical-size: Doubles the recorded DPI and restores at the target display's actual DPI without changing display settings.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowPlacementClearsStaleSnapFields)
            TEST_METHOD_PROPERTY(L"Description", L"Repeated session-end saves followed by close replace snapped records without leaving stale snap fields.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowPlacementRespectsSnappingSetting)
            TEST_METHOD_PROPERTY(L"Description", L"Checks the available native path's snap-setting behavior and restores the original setting after testing.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()
        
        // Width/Height tests

        BEGIN_TEST_METHOD(CanGetSetWindowWidthHeight)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the experimental Width/Height properties on Window can be read and written in DIPs.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowWidthHeightBeforeActivationUsesFinalConfiguration)
            TEST_METHOD_PROPERTY(L"Description", L"Validates exact pending sizes before activation, application with the final title-bar configuration and a sizing-capable presenter, and later native resizing.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowWidthHeightSettersPreserveOtherAxis)
            TEST_METHOD_PROPERTY(L"Description", L"Validates single-axis sizing, position preservation, pixel rounding, and unchanged live geometry in non-restored states.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(SetWindowWidthHeightInMarkup)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that Width/Height set in XAML markup on the <Window> element size the window's client area.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowWidthHeightRejectsInvalidValues)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that put_Width/put_Height return E_INVALIDARG for negative/NaN/infinite values, with a helpful specific error message.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowWidthHeightClampsLargeFiniteValues)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the 65,535-pixel outer-size clamp for finite Width/Height requests, both title-bar modes, and realized restored sizes.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowWidthHeightAfterClose)
            TEST_METHOD_PROPERTY(L"Description", L"Validates preserved Width/Height and property-value round-tripping after close across activation, restored, minimized, maximized, and non-default presenter states.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowWidthHeightAfterCloseValidatesArguments)
            TEST_METHOD_PROPERTY(L"Description", L"Validates preserved pending requests, invalid arguments, and exact finite Width/Height round-tripping after close, including the largest finite double.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowWidthHeightClosesDuringResize)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that closing from SizeChanged preserves resolved and pending sizes after the interrupted resize returns.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowWidthHeightSizeChangedReentrancy)
            TEST_METHOD_PROPERTY(L"Description", L"Validates restored sizes when SizeChanged switches presenters or toggles the title bar during a Width setter.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowWidthHeightAfterCloseChecksThread)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that Width/Height getters and setters enforce thread affinity both before and after close.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowWidthHeightDuringClose)
            TEST_METHOD_PROPERTY(L"Description", L"Validates pending and live sizing during Closed handlers, canceled close, and preservation of the final handler's native resize.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowWidthHeightRejectsBindingInMarkup)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a classic {Binding} on Window.Width/Height fails to parse with a helpful error, rather than silently collapsing the window to 0.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(SetWindowWidthHeightWhileMinimizedUpdatesRestoreSize)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting Width/Height while minimized updates the restore size (WPF parity) and leaves the window minimized.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanGetSetWindowWidthHeightBothChromeModes)
            TEST_METHOD_PROPERTY(L"Description", L"Validates Width/Height round-trip with ExtendsContentIntoTitleBar on and off.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(SetWindowWidthWhileMaximizedUpdatesRestoreSize)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting Width while maximized updates only the restore width without clobbering the restore height, in both ECITB modes.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(SetWindowWidthHeightWhileMaximizedWithECITBPreservesOuterClientOffset)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting Width/Height while maximized with ECITB enabled restores to the requested client size and preserves the outer-to-client chrome offset.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(GetWindowWidthHeightWhileMaximizedReportsRestoreSize)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the Width/Height getters return the restore size (not the live maximized size) while maximized, both when never set and after setting a value.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(GetWindowWidthHeightAfterSettingWhileMaximizedThenFullScreenReportsSetSize)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting Width/Height while maximized then switching straight into FullScreen reports the value just set (tracked restore size stays in sync).")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(SetWindowWidthHeightInNonDefaultPresenterIsDeferred)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that Width/Height set in the FullScreen and CompactOverlay presenters is remembered (returned by the getter) and applied when returning to the Default presenter.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(GetWindowWidthHeightInNonDefaultPresenterWithoutSetReturnsRestoreSize)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that in FullScreen/CompactOverlay the getter returns the restore size even when Width/Height were never set, and the window round-trips through the presenter untouched.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ToggleECITBPreservesSetClientSize)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting Height preserves the client size across ExtendsContentIntoTitleBar toggles - order-independent and across an on/off round-trip.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ToggleECITBWithoutSetLeavesWindowSizeUnchanged)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that toggling ExtendsContentIntoTitleBar without ever setting Width/Height leaves the outer window size unchanged.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ToggleECITBPreservesClientHeightOnlyAfterHeightSet)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that only setting Height opts into preserving the current client height across title-bar toggles, including after a native resize.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ToggleECITBInNonRestoredStateDoesNotPreserveClientHeight)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that title-bar toggles while minimized, maximized, full screen, or compact overlay do not resize the restored window to preserve client height.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(GetWindowWidthHeightInPresenterHonorsValueSetBeforeSwitching)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a Width/Height set before switching to FullScreen/CompactOverlay is still honored by the getter (not replaced by the live presenter size).")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(GetWindowWidthHeightInPresenterReflectsResizeBeforeSwitching)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a user resize after setting Width/Height (but before switching presenters) is reflected by the getter in FullScreen/CompactOverlay, not the original set value.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(RestoreFromMaximizedWithECITBIsExact)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that restoring from maximized with ExtendsContentIntoTitleBar lands at the exact requested client size, using the measured (WM_NCCALCSIZE-aware) chrome.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(GetWindowWidthHeightHonorsValueSetSameTurnAsPresenterSwitch)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that Width/Height set and a presenter switch in the same turn (no intervening idle) still reports the set value in the non-default presenter.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(SimulatedUserDragResizeIsTrackedForPresenter)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a resize bracketed by WM_ENTERSIZEMOVE/WM_EXITSIZEMOVE (simulated user drag) is captured at drag end and reflected by the getter in a non-sizing presenter.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        // The Window Min/Max size properties (MinWidth/MinHeight/MaxWidth/MaxHeight) only exist in prerelease.
        BEGIN_TEST_METHOD(MinMaxSizeGetSet)
            TEST_METHOD_PROPERTY(L"Description", L"Validates programmatic get/set and defaults of the Window Min/Max size properties.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(MinMaxSizeInvalidValuesThrow)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that invalid Window Min/Max size values are rejected.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(SetMinMaxSizeInMarkup)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that Window Min/Max size set in markup is correctly handled.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(MinMaxSizeSurvivesPresenterSwap)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that Window Min/Max size constraints are re-applied as the AppWindow moves between presenters (overlapped, compact overlay, full screen) in multiple directions.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(MinMaxSizeClampsWindowSize)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that resizing the window past its Min/Max size constraints is clamped by the presenter.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(MinMaxSizeLeavesUnsetConstraintsAlone)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that Window Min/Max size only writes presenter constraints the app actually set, leaving presenter properties the app manages directly untouched.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(MinMaxSizeTracksTitleBarToggle)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that Window Min/Max size constraints re-apply when ExtendsContentIntoTitleBar toggles, so they track the changed chrome instead of drifting by the caption height.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()
#endif // MUX_PRERELEASE

        BEGIN_TEST_METHOD(ContentSetInMarkupIsReleasedWhenCleared)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the markup-loaded Window content tree is released (not held for the Window's lifetime) once Window.Content is cleared.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

    };

} } } } } }
