// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <HostingModeTestClass.h>

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
            TEST_CLASS_HOSTING_MODE_DEFAULT()
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        //
        // Platform:Any
        //

        //
        // Platform:Desktop
        //

        BEGIN_TEST_METHOD(CanMoveWindow)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can move the window.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"OneCore, WindowsCore") // This scenario is not supported on OneCore. Also excluded on WindowsCore.
            TEST_METHOD_PROPERTY(L"Ignore", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(SetTitleInMarkup)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that window title set in markup is correctly handled.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(SetSystemBackdropInMarkup)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that window SystemBackdrop set in markup is correctly handled.")
        END_TEST_METHOD()

#ifdef MUX_PRERELEASE
        // Experimental Width/Height properties on Window are only present in prerelease builds.
        BEGIN_TEST_METHOD(CanGetSetWindowWidthHeight)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the experimental Width/Height properties on Window can be read and written in DIPs.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(SetWindowWidthHeightInMarkup)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that Width/Height set in XAML markup on the <Window> element size the window's client area.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowWidthHeightRejectsInvalidValues)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that put_Width/put_Height return E_INVALIDARG for negative/NaN/infinite values, with a helpful specific error message.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowWidthHeightRejectsBindingInMarkup)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a classic {Binding} on Window.Width/Height fails to parse with a helpful error, rather than silently collapsing the window to 0.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(SetWindowWidthHeightWhileMinimizedUpdatesRestoreSize)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting Width/Height while minimized updates the restore size (WPF parity) and leaves the window minimized.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanGetSetWindowWidthHeightBothChromeModes)
            TEST_METHOD_PROPERTY(L"Description", L"Validates Width/Height round-trip with ExtendsContentIntoTitleBar on and off.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(SetWindowWidthWhileMaximizedUpdatesRestoreSize)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting Width while maximized updates only the restore width without clobbering the restore height, in both ECITB modes.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(SetWindowWidthHeightWhileMaximizedWithECITBPreservesOuterClientOffset)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting Width/Height while maximized with ECITB enabled restores to the requested client size and preserves the outer-to-client chrome offset.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(GetWindowWidthHeightWhileMaximizedReportsRestoreSize)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the Width/Height getters return the restore size (not the live maximized size) while maximized, both when never set and after setting a value.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(GetWindowWidthHeightAfterSettingWhileMaximizedThenFullScreenReportsSetSize)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting Width/Height while maximized then switching straight into FullScreen reports the value just set (tracked restore size stays in sync).")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(SetWindowWidthHeightInNonDefaultPresenterIsDeferred)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that Width/Height set in the FullScreen and CompactOverlay presenters is remembered (returned by the getter) and applied when returning to the Default presenter.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(GetWindowWidthHeightInNonDefaultPresenterWithoutSetReturnsRestoreSize)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that in FullScreen/CompactOverlay the getter returns the restore size even when Width/Height were never set, and the window round-trips through the presenter untouched.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ToggleECITBPreservesSetClientSize)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a set Width/Height is preserved across ExtendsContentIntoTitleBar toggles - order-independent and across an on/off round-trip.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ToggleECITBWithoutSetLeavesWindowSizeUnchanged)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that toggling ExtendsContentIntoTitleBar without ever setting Width/Height leaves the outer window size unchanged.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(GetWindowWidthHeightInPresenterHonorsValueSetBeforeSwitching)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a Width/Height set before switching to FullScreen/CompactOverlay is still honored by the getter (not replaced by the live presenter size).")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(GetWindowWidthHeightInPresenterReflectsResizeBeforeSwitching)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a user resize after setting Width/Height (but before switching presenters) is reflected by the getter in FullScreen/CompactOverlay, not the original set value.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(RestoreFromMaximizedWithECITBIsExact)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that restoring from maximized with ExtendsContentIntoTitleBar lands at the exact requested client size, using the measured (WM_NCCALCSIZE-aware) chrome.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(GetWindowWidthHeightHonorsValueSetSameTurnAsPresenterSwitch)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that Width/Height set and a presenter switch in the same turn (no intervening idle) still reports the set value in the non-default presenter.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(SimulatedUserDragResizeIsTrackedForPresenter)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a resize bracketed by WM_ENTERSIZEMOVE/WM_EXITSIZEMOVE (simulated user drag) is captured at drag end and reflected by the getter in a non-sizing presenter.")
        END_TEST_METHOD()

        // The Window Min/Max size properties (MinWidth/MinHeight/MaxWidth/MaxHeight) only exist in prerelease.
        BEGIN_TEST_METHOD(MinMaxSizeGetSet)
            TEST_METHOD_PROPERTY(L"Description", L"Validates programmatic get/set and defaults of the Window Min/Max size properties.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(MinMaxSizeInvalidValuesThrow)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that invalid Window Min/Max size values are rejected.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(SetMinMaxSizeInMarkup)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that Window Min/Max size set in markup is correctly handled.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(MinMaxSizeSurvivesPresenterSwap)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that Window Min/Max size constraints are re-applied as the AppWindow moves between presenters (overlapped, compact overlay, full screen) in multiple directions.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(MinMaxSizeClampsWindowSize)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that resizing the window past its Min/Max size constraints is clamped by the presenter.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(MinMaxSizeLeavesUnsetConstraintsAlone)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that Window Min/Max size only writes presenter constraints the app actually set, leaving presenter properties the app manages directly untouched.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(MinMaxSizeTracksTitleBarToggle)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that Window Min/Max size constraints re-apply when ExtendsContentIntoTitleBar toggles, so they track the changed chrome instead of drifting by the caption height.")
        END_TEST_METHOD()
#endif // MUX_PRERELEASE

        BEGIN_TEST_METHOD(ContentSetInMarkupIsReleasedWhenCleared)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the markup-loaded Window content tree is released (not held for the Window's lifetime) once Window.Content is cleared.")
        END_TEST_METHOD()

    };

    class WindowIntegrationTestsUap : public WEX::TestClass<WindowIntegrationTestsUap>
    {
    public:
        BEGIN_TEST_CLASS(WindowIntegrationTestsUap)
            TEST_CLASS_PROPERTY(L"MasterFile:ClassName", L"WindowIntegrationTests")
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"a62e3c8d-69d4-44de-95b5-a62be5062286")
            TEST_CLASS_PROPERTY(L"IsolationLevel", L"Class")
            TEST_CLASS_HOSTING_MODE(UAP)
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(CanHideAndShowWindow)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can hide and show the window.")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanGetSetTitle)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can get and set the window title.")
        END_TEST_METHOD()
    };

} } } } } }
