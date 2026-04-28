// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

enum class IslandSceneKind
{
    Blank,
    WithButton,
    WithNumberBox,
    ManyControls
};

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Hosting {

//
// XamlIslandTests is a raw TAEF test - it doesn't use any of the Xaml hosting infrastructures. Instead, this test
// manually spins up threads, creates hwnds, and creates DesktopWindowXamlSources to initialize Xaml. This is close
// to how customers in Shell consume DWXS.
//
// This bare-metal approach also gives us enough flexibility to test many different Xaml island setups:
// - Multiple islands per thread
// - Multiple windows, each with islands
// - Tearing down and reinitializing Xaml on the same thread
//
// In the future, this barebones Win32 hosting can be refactored out to a different type of test host. It has the
// benefit of not loading up WPF/.NET. This test stays purely native.
//
// In exchange, XamlIslandTests can't use a lot of the test functionality available to other tests, namely anything
// on TestServices. This is because initializing TestServices will automatically spin up a WPF host for Xaml, and
// that's something we're avoiding. In the future we can refactor TestServices to be consumable piecemeal, rather than
// requiring a WPF host.
//
// Tests in this class follow the same pattern. There are (at least) two threads involved.
//
//      TAEF thread:
//       1. Create UI thread[s] with test-specific ThreadProcs.
//       2. Sleep() to let the UI threads render (there's no WaitForIdle equivalent yet).
//       3. Get the hwnd[s] that the UI thread[s] created via GetHwnd().
//       4. Call SetFlagAndPokeHwnd to kick the UI thread[s] to do the next thing. Repeat as needed.
//       5. TerminateThread the UI thread[s].
//
//      UI thread[s]:
//       1. Call CreateHwnd to create, show, and register a new hwnd.
//       2. Call CreateDesktopWindowXamlSource to create a DesktopWindowXamlSource, attach it to an hwnd, and populate
//          it with a simple Xaml tree.
//       3. Call PumpMessagesWhileWaitingForFlag to wait for the signal from the TAEF thread to do the next thing.
//       4. Do the next thing (tear down the DWXS, create a new one, etc).
//       5. Repeat from step 3 as needed.
//
class XamlIslandTests : public WEX::TestClass<XamlIslandTests>
{
public:
    BEGIN_TEST_CLASS(XamlIslandTests)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        // Even though we're not a UAP, this is needed to load an AppxManifest and Xaml types. Without it, DWXS gives
        // "class not registered" errors.
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"Description", L"Various Xaml island configurations.")
        TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
        // When running this test, make sure to run with -hostingMode set to "Win32Explicit"
        TEST_CLASS_PROPERTY(L"Hosting:Mode", L"Win32Explicit")
    END_TEST_CLASS()

    TEST_METHOD_SETUP(TestSetup)
    TEST_METHOD_CLEANUP(TestCleanup)

    TEST_METHOD(IslandsRequireDispatcherQueueController)
    TEST_METHOD(WindowsXamlManagerCreationScenarios)
    TEST_METHOD(ValidateXamlShutdownCompletedOnThread)
    TEST_METHOD(ValidateXamlShutdownCompletedOnThreadWithDeferral)
    TEST_METHOD(XamlUnloadsAutomatically)
    TEST_METHOD(WindowsXamlManagerKeptAlive)    

    BEGIN_TEST_METHOD(XamlIslandCreationScenarios)
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()
    
    TEST_METHOD(TwoWindowsOnSameThread)

    TEST_METHOD(TwoWindowsOnDifferentThreads)

    BEGIN_TEST_METHOD(WindowsXamlManagerOnDifferentThreadsStress)
        TEST_METHOD_PROPERTY(L"IsolationLevel", L"Method")  // Don't cause failures in other tests if this leaves Xaml in a weird state.
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(IslandsOnDifferentThreadsStress)
        TEST_METHOD_PROPERTY(L"IsolationLevel", L"Method")  // Don't cause failures in other tests if this leaves Xaml in a weird state.
    END_TEST_METHOD()

    // Recommended command line:
    //  runtests.cmd -win32explicit ApplicationLikeFileExplorerStress -runIgnoredTests /testmode:loop /looptest:1 /loop:10
    // It may take a few tries.
    // Race condition in XamlControlsResources creation during Xaml startup (affects FileExplorer)
    BEGIN_TEST_METHOD(ApplicationLikeFileExplorerStress)
        TEST_METHOD_PROPERTY(
            L"Description",
            L"Runs Xaml startup on multiple threads at the same time to flush out race conditions that might affect "
                L"FileExplorer.")
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")    // Currently hits a crash and a hang.
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(IslandsOnDifferentThreadsWithButtonStress)
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")    // Test unstable due to bug
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(IslandsOnDifferentThreadsWithManyControlsStress)
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")    // Test unstable due to bug
    END_TEST_METHOD()

    TEST_METHOD(TwoDesktopIslandsInSameWindow)

    BEGIN_TEST_METHOD(TwoXamlIslandsInSameWindow)
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ReinitializeXamlIslandOnUIThread_DefaultApplication_NoReferenceToApplication)
       TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(ReinitializeXamlIslandOnUIThread_DefaultApplication_KeepApplicationAlive)
       TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()
    TEST_METHOD(ReinitializeXamlIslandOnUIThread_CustomApplication_NoReferenceToApplication_v1Shutdown)
    BEGIN_TEST_METHOD(ReinitializeXamlIslandOnUIThread_CustomApplication_NoReferenceToApplication)
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()
    TEST_METHOD(ReinitializeXamlIslandOnUIThread_CustomApplication_KeepApplicationAlive_v1Shutdown)
    TEST_METHOD(ReinitializeXamlIslandOnUIThread_CustomApplication_KeepApplicationAlive)

    TEST_METHOD(IslandSystemBackdrop)

    TEST_METHOD(IslandWithMuxcDoesntPoisonThread)

    BEGIN_TEST_METHOD(IslandsWithSuddenShutdown)
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ShutdownWithLeakDetection)
        TEST_METHOD_PROPERTY(L"IsolationLevel", L"Method")  // Uses PerformProcessWideLeakDetection, which detaches the DLL
    END_TEST_METHOD()

    TEST_METHOD(ShowTeachingTipWhileIslandClosing)

    BEGIN_TEST_METHOD(MultipleApplicationObjects)
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")    // Apps crash when creating multiple Application objects. Should just throw an exception.
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(FailFastWhenXamlNotShutdown)
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")    // Deliberately disabled because we don't have a test hook for failfasts.
                                                    // Run it manually by using the -RunIgnoredTests flag
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(FailFastWhenMessagesNotPumpedAfterClosing)
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")    // Deliberately disabled because we don't have a test hook for failfasts.
                                                    // Run it manually by using the -RunIgnoredTests flag
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(PointerInput)
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()

    TEST_METHOD(LoadedImageSurface_DecodeOnShutdownCrash)

    TEST_METHOD(ToggleAccessKeyDisplayModeOnMultipleIslands)

    TEST_METHOD(FindXamlRootTopLevelWindow)

    TEST_METHOD(ValidateContentIsland)

    TEST_METHOD(ReparentScrollViewerAcrossIslands)

    // Tests that we can support multiple UI threads (specifically around RunOnUIThread calls)
    TEST_METHOD(TwoUIThreads)

    TEST_METHOD(DeviceLostTest)

    BEGIN_TEST_METHOD(EnsureDwxsEventOrderDuringTabTraversal)
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CommandBarFlyoutTest)
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // Investigate failure
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(EnsureWin32SetOnProgrammaticFocusToFocusedElement)
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()
    
    TEST_METHOD(StagedFlyoutTest)

    TEST_METHOD(CloseWindowSuddenly)

    TEST_METHOD(UnrootedAutoSuggestBox)

    TEST_METHOD(CloseIslandWhileAsbPopupOpening)
    TEST_METHOD(ValidateUiaAfterVisualTreeReset)

    TEST_METHOD(ValidateDispatcherShutdownModeInIslandsApp)
    TEST_METHOD(ValidateDispatcherShutdownModeInDesktopApp)

    BEGIN_TEST_METHOD(ValidateCallbackErrorPropagatesInDesktopApp)
        TEST_METHOD_PROPERTY(L"Description", L"Verify that an app callback failure in Application.Start propagates as an error return instead of fail-fasting inside WinUI.")
    END_TEST_METHOD()

    TEST_METHOD(EffectiveViewportChangedInWindowedPopup)

    BEGIN_TEST_METHOD(ValidateUiaFindAllWithWindowedPopup)
        TEST_METHOD_PROPERTY(L"Description", L"Regression test to make sure that running UIA FindAll on an island with a windowed popup still finds the content in the island.")
    END_TEST_METHOD()

    TEST_METHOD(ValidateNavigationView);

    TEST_METHOD(BasicHwndlessIslandTest);

    BEGIN_TEST_METHOD(WinUI3CohabitationWithWinUI2)
        TEST_METHOD_PROPERTY(L"Description", L"Verify WinUI3 rendering works fine despite the WinUI2 module loaded in same process.")
    END_TEST_METHOD()

    TEST_METHOD(ContentRendersInHwndlessIslands);

    TEST_METHOD(KeyboardInputWorksInHwndlessIslands);

    TEST_METHOD(PointerInputWorksInHwndlessIslands);

    BEGIN_TEST_METHOD(PopupsWorkInHwndlessIslands)
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // Windowless popups do not currently work in HWND-less islands.
    END_TEST_METHOD()

    // Called on the TAEF thread to control UI thread
    void SetFlagAndPokeHwnd(int id, HWND hwnd);
    // Called on the UI threads to wait for signals from TAEF thread
    void PumpMessagesWhileWaitingForFlag(int id);

    // Called on the new UI thread to create new hwnds.
    HWND CreateHwnd(int id, HWND parent = NULL);
    HWND CreateHwnd();  // Convenience method for when there's only one hwnd
    // Called on the TAEF thread to retrieve hwnds.
    HWND GetHwnd(int id);
    HWND GetHwnd();     // Convenience method for when there's only one hwnd

    ATOM GetWindowClassAtom() { return m_windowClassAtom; }

private:
    void ReinitializeXamlIslandOnUIThread_Common(wil::unique_handle& uiThread);

    void IslandStressWorker(IslandSceneKind sceneKind);

    void CommandBarFlyoutTest_VerifyCBF(Private::Infrastructure::IslandHelper^ ih);
    void CommandBarFlyoutTest_VerifyCBFOverflowPopup(Microsoft::UI::Content::ContentIsland^ secondaryItemsContentIsland);
    void CommandBarFlyoutTest_VerifyCBFPopup(Microsoft::UI::Content::ContentIsland^ primaryMenuContentIsland);

private:
    bool GetFlag(int id);
    void ClearFlag(int id);
    void ClearFlags();

    // One of the consequences of rolling our own islands is cross-thread communication. We'll spin up new threads to
    // create hwnds and initialize DesktopWindowXamlSources. Those threads will be new UI threads and will run message
    // pumps. This CS is used to talk with those UI threads.
    wil::critical_section m_cs;

    ATOM m_windowClassAtom;

    // A list of hwnds created by various UI threads. Keyed by an arbitrary number. It's up to the test to define what
    // id means what.
    std::map<int, HWND> m_hwndMap;

    // A list of flags used to communicate with various UI threads. Keyed by an arbitrary number. It's up to the test
    // to define what flag means what.
    std::map<int, bool> m_flagMap;
};

} } } } } }
