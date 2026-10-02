// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Dispatching;
using Microsoft.UI.Windowing;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Tests.Common;
using Private.Infrastructure.Hosting.WPF;
using System;
using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;

namespace Microsoft.UI.Xaml.Tests.Hosting.Win32.WPF
{
    [TestClass]
    public class WindowPlacementPersistenceTests : XamlTestsBase
    {
        [ClassInitialize]
        [TestProperty("BinaryUnderTest", "Microsoft.UI.Xaml.dll")]
        [TestProperty("RunAs", "UAP")]
        [TestProperty("UAP:Praid", "XamlManagedTAEFTests")]
        [TestProperty("Hosting:Mode", "WPF")]
        [TestProperty("Classification", "Integration")]
        public static void Setup(TestContext context)
        {
            AssemblySetup.CommonTestClassSetup();
        }

        [ClassCleanup]
        public void ClassCleanup()
        {
            base.CommonClassCleanup();
        }

        [TestMethod]
        public void HiddenPreparationDoesNotDisplayWindow()
        {
            UIExecutor.Execute(() =>
            {
                var window = CreateWindow();
                try
                {
                    var options = new WindowShowOptions
                    {
                        Placement = CreatePlacement(),
                        DoNotActivate = true
                    };

                    window.TryApplyInitialPlacement(options);
                    Verify.IsFalse(window.Visible);

                    window.Show(new WindowShowOptions
                    {
                        SkipInitialPlacement = true,
                        DoNotActivate = true
                    });
                    Verify.IsTrue(window.Visible);
                }
                finally
                {
                    window.Close();
                }
            });
        }

        [TestMethod]
        public void FirstShowUsesPersistenceConfigurationAndCapturesBounds()
        {
            UIExecutor.Execute(() =>
            {
                var window = CreateWindow();
                try
                {
                    window.PersistPlacementId = "WindowPlacementPersistenceTests.FirstShow";
                    window.UseAutomaticPlacementPersistence = true;
                    window.Show(new WindowShowOptions { DoNotActivate = true });

                    Verify.IsTrue(window.Visible);
                    Verify.IsTrue(window.Bounds.Width > 0);
                    Verify.IsTrue(window.Bounds.Height > 0);
                }
                finally
                {
                    window.Close();
                }
            });
        }

        [TestMethod]
        public void FirstActivateUsesPersistenceConfiguration()
        {
            UIExecutor.Execute(() =>
            {
                var window = CreateWindow();
                try
                {
                    window.PersistPlacementId = "WindowPlacementPersistenceTests.FirstActivate";
                    window.UseAutomaticPlacementPersistence = true;
                    window.Activate();

                    Verify.IsTrue(window.Visible);
                }
                finally
                {
                    window.Close();
                }
            });
        }

        [TestMethod]
        public void SavesVirtualDesktopIdAfterPostedRefresh()
        {
            UIExecutor.Execute(() =>
            {
                var placementId = "WindowPlacementPersistenceTests." + Guid.NewGuid().ToString("N");
                var window = CreateWindow();
                try
                {
                    window.PersistPlacementId = placementId;
                    window.UseAutomaticPlacementPersistence = true;
                    window.Show(new WindowShowOptions { DoNotActivate = true });

                    WindowPlacement observedPlacement = null;
                    for (var attempt = 0; attempt < 20; attempt++)
                    {
                        DispatcherQueue.GetForCurrentThread().DoEvents();
                        Verify.IsTrue(window.TryGetPlacement(out observedPlacement));
                        if (observedPlacement.VirtualDesktopId.HasValue &&
                            observedPlacement.VirtualDesktopId.Value != Guid.Empty)
                        {
                            break;
                        }

                        // The shell can reject the first query before it registers the window.
                        // Re-display it to post another safe-point refresh rather than accepting
                        // an empty id.
                        window.Hide();
                        window.Show(new WindowShowOptions
                        {
                            SkipInitialPlacement = true,
                            DoNotActivate = true
                        });
                    }

                    Verify.IsNotNull(observedPlacement);
                    Verify.IsTrue(observedPlacement.VirtualDesktopId.HasValue);
                    Verify.AreNotEqual(Guid.Empty, observedPlacement.VirtualDesktopId.Value);
                }
                finally
                {
                    window.Close();
                }

                var savedPlacement = WindowPlacement.LoadForPersistPlacementId(placementId);
                Verify.IsNotNull(savedPlacement);
                Verify.IsTrue(savedPlacement.VirtualDesktopId.HasValue);
                Verify.AreNotEqual(Guid.Empty, savedPlacement.VirtualDesktopId.Value);
            });
        }

        [TestMethod]
        public void TryGetPlacementReturnsCurrentPlacement()
        {
            UIExecutor.Execute(() =>
            {
                var window = CreateWindow();
                try
                {
                    window.Show(new WindowShowOptions { DoNotActivate = true });

                    Verify.IsTrue(window.TryGetPlacement(out var placement));
                    Verify.IsNotNull(placement);
                    Verify.IsTrue(placement.NormalRect.Width > 0);
                    Verify.IsTrue(placement.NormalRect.Height > 0);
                    Verify.IsTrue(placement.WorkArea.Width > 0);
                    Verify.IsTrue(placement.WorkArea.Height > 0);
                    Verify.IsTrue(placement.Dpi > 0);
                }
                finally
                {
                    window.Close();
                }
            });
        }

        [TestMethod]
        public void InvalidInitialRequestDoesNotDisplayWindow()
        {
            UIExecutor.Execute(() =>
            {
                var window = CreateWindow();
                try
                {
                    var options = new WindowShowOptions
                    {
                        Placement = CreatePlacement(),
                        SkipInitialPlacement = true
                    };

                    try
                    {
                        window.Show(options);
                        Verify.Fail("Expected invalid initial placement options to be rejected.");
                    }
                    catch (global::System.ArgumentException)
                    {
                    }

                    Verify.IsFalse(window.Visible);
                }
                finally
                {
                    window.Close();
                }
            });
        }

        [TestMethod]
        public void NullOptionsAreRejectedWithoutChangingTheWindow()
        {
            UIExecutor.Execute(() =>
            {
                var window = CreateWindow();
                try
                {
                    AssertOptionsRejected(() => window.Show((WindowShowOptions)null));
                    Verify.IsFalse(window.Visible);

                    AssertOptionsRejected(() => window.TryApplyInitialPlacement(null));
                    Verify.IsFalse(window.Visible);

                    window.Show(new WindowShowOptions { DoNotActivate = true });
                    Verify.IsTrue(window.Visible);
                }
                finally
                {
                    window.Close();
                }
            });
        }

        [TestMethod]
        public void LateRequestsRejectNullButIgnoreFieldsInNonNullOptions()
        {
            UIExecutor.Execute(() =>
            {
                var window = CreateWindow();
                try
                {
                    window.Show(new WindowShowOptions { DoNotActivate = true });
                    window.Hide();

                    AssertOptionsRejected(() => window.Show((WindowShowOptions)null));
                    AssertOptionsRejected(() => window.TryApplyInitialPlacement(null));
                    Verify.IsFalse(window.Visible);

                    var invalidPlacement = CreatePlacement();
                    invalidPlacement.NormalRect = new global::Windows.Graphics.RectInt32(0, 0, -1, 0);
                    var ignoredOptions = new WindowShowOptions
                    {
                        Placement = invalidPlacement,
                        SkipInitialPlacement = true,
                        Reason = (WindowShowReason)999,
                        CascadeBehavior = (WindowCascadeBehavior)999
                    };

                    window.Show(ignoredOptions);
                    Verify.IsTrue(window.Visible);
                    window.Hide();
                    Verify.IsFalse(window.TryApplyInitialPlacement(ignoredOptions));
                }
                finally
                {
                    window.Close();
                }
            });
        }

        [TestMethod]
        public void HiddenApplicationRestoresSavedPlacementWhenCascadingIsDisabled()
        {
            UIExecutor.Execute(() =>
            {
                var placementId = "WindowPlacementPersistenceTests." + Guid.NewGuid().ToString("N");
                var firstWindow = CreateWindow();
                WindowPlacement savedPlacement = null;
                try
                {
                    firstWindow.PersistPlacementId = placementId;
                    firstWindow.UseAutomaticPlacementPersistence = true;
                    firstWindow.Show(new WindowShowOptions
                    {
                        Placement = CreatePlacement(),
                        CascadeBehavior = WindowCascadeBehavior.Disabled,
                        DoNotActivate = true
                    });
                    Verify.IsTrue(firstWindow.TryGetPlacement(out savedPlacement));
                }
                finally
                {
                    firstWindow.Close();
                }

                var secondWindow = CreateWindow();
                try
                {
                    secondWindow.PersistPlacementId = placementId;
                    secondWindow.UseAutomaticPlacementPersistence = true;
                    Verify.IsTrue(secondWindow.TryApplyInitialPlacement(new WindowShowOptions
                    {
                        CascadeBehavior = WindowCascadeBehavior.Disabled,
                        DoNotActivate = true
                    }));
                    Verify.IsFalse(secondWindow.Visible);
                    Verify.IsTrue(secondWindow.TryGetPlacement(out var restoredPlacement));
                    Verify.AreEqual(savedPlacement.NormalRect.Width, restoredPlacement.NormalRect.Width);
                    Verify.AreEqual(savedPlacement.NormalRect.Height, restoredPlacement.NormalRect.Height);
                }
                finally
                {
                    secondWindow.Close();
                }
            });
        }

        [TestMethod]
        public void DetachedLoadReturnsPlacementSavedByAClosedWindow()
        {
            UIExecutor.Execute(() =>
            {
                var placementId = "WindowPlacementPersistenceTests." + Guid.NewGuid().ToString("N");
                Verify.IsNull(WindowPlacement.LoadForPersistPlacementId(placementId));

                var window = CreateWindow();
                WindowPlacement savedPlacement = null;
                try
                {
                    window.PersistPlacementId = placementId;
                    window.UseAutomaticPlacementPersistence = true;
                    window.Show(new WindowShowOptions
                    {
                        Placement = CreatePlacement(),
                        CascadeBehavior = WindowCascadeBehavior.Disabled,
                        DoNotActivate = true
                    });
                    Verify.IsTrue(window.TryGetPlacement(out savedPlacement));
                }
                finally
                {
                    window.Close();
                }

                var loadedPlacement = WindowPlacement.LoadForPersistPlacementId(placementId);
                Verify.IsNotNull(loadedPlacement);
                Verify.AreEqual(savedPlacement.NormalRect.X, loadedPlacement.NormalRect.X);
                Verify.AreEqual(savedPlacement.NormalRect.Y, loadedPlacement.NormalRect.Y);
                Verify.AreEqual(savedPlacement.NormalRect.Width, loadedPlacement.NormalRect.Width);
                Verify.AreEqual(savedPlacement.NormalRect.Height, loadedPlacement.NormalRect.Height);
                Verify.AreEqual(savedPlacement.Dpi, loadedPlacement.Dpi);
            });
        }

        [TestMethod]
        public void FullScreenPresenterKeepsAndSavesThePreFullScreenPlacement()
        {
            string skipReason = null;

            UIExecutor.Execute(() =>
            {
                var placementId = "WindowPlacementPersistenceTests." + Guid.NewGuid().ToString("N");
                var window = CreateWindow();
                WindowPlacement overlappedPlacement = null;
                try
                {
                    window.PersistPlacementId = placementId;
                    window.UseAutomaticPlacementPersistence = true;
                    window.Show(new WindowShowOptions
                    {
                        Placement = CreatePlacement(200, 150, 640, 480),
                        CascadeBehavior = WindowCascadeBehavior.Disabled,
                        DoNotActivate = true
                    });

                    Verify.IsTrue(window.TryGetPlacement(out overlappedPlacement));
                    Verify.AreEqual(WindowPlacementState.Normal, overlappedPlacement.State);
                    Log.Comment("Overlapped placement: " + Describe(overlappedPlacement));

                    var appWindow = window.AppWindow;
                    Verify.IsNotNull(appWindow);
                    var overlappedSize = appWindow.Size;

                    appWindow.SetPresenter(AppWindowPresenterKind.FullScreen);
                    if (appWindow.Presenter.Kind != AppWindowPresenterKind.FullScreen)
                    {
                        skipReason = "This environment refused the FullScreen presenter. The AppWindow reports " +
                            appWindow.Presenter.Kind + ".";
                        return;
                    }

                    var fullScreenSize = appWindow.Size;
                    Log.Comment("Overlapped size " + overlappedSize.Width + "x" + overlappedSize.Height +
                        "; full screen size " + fullScreenSize.Width + "x" + fullScreenSize.Height + ".");
                    if (fullScreenSize.Width == overlappedSize.Width &&
                        fullScreenSize.Height == overlappedSize.Height)
                    {
                        // Without a real geometry change the assertions below would pass even if
                        // capture had overwritten the snapshot with live full-screen bounds.
                        skipReason = "This environment did not resize the window for the FullScreen presenter.";
                        return;
                    }

                    // A non-overlapped presenter has no overlapped geometry to capture, so the cache
                    // must still report the placement the window had before the transition.
                    Verify.IsTrue(window.TryGetPlacement(out var whileFullScreen));
                    Log.Comment("Placement while full screen: " + Describe(whileFullScreen));
                    Verify.AreEqual(WindowPlacementState.Normal, whileFullScreen.State);
                    VerifyPlacementsMatch(overlappedPlacement, whileFullScreen);
                }
                finally
                {
                    window.Close();
                }

                if (skipReason != null)
                {
                    return;
                }

                var loadedPlacement = WindowPlacement.LoadForPersistPlacementId(placementId);
                Verify.IsNotNull(loadedPlacement);
                Log.Comment("Saved placement: " + Describe(loadedPlacement));
                Verify.AreEqual(WindowPlacementState.Normal, loadedPlacement.State);
                VerifyPlacementsMatch(overlappedPlacement, loadedPlacement);
            });

            if (skipReason != null)
            {
                Log.Result(TestResult.Skipped, skipReason);
            }
        }

        [TestMethod]
        public void MaximizedPlacementSurvivesCloseAndRestore()
        {
            string skipReason = null;

            UIExecutor.Execute(() =>
            {
                var placementId = "WindowPlacementPersistenceTests." + Guid.NewGuid().ToString("N");
                Verify.IsNull(WindowPlacement.LoadForPersistPlacementId(placementId));

                var window = CreateWindow();
                WindowPlacement savedPlacement = null;
                try
                {
                    window.PersistPlacementId = placementId;
                    window.UseAutomaticPlacementPersistence = true;
                    window.Show(new WindowShowOptions
                    {
                        Placement = CreatePlacement(200, 150, 640, 480),
                        CascadeBehavior = WindowCascadeBehavior.Disabled,
                        DoNotActivate = true
                    });

                    Verify.IsTrue(window.TryGetPlacement(out var beforeMaximize));
                    Log.Comment("Before maximize: normal " +
                        beforeMaximize.NormalRect.X + "," + beforeMaximize.NormalRect.Y + " " +
                        beforeMaximize.NormalRect.Width + "x" + beforeMaximize.NormalRect.Height +
                        " work area " + beforeMaximize.WorkArea.X + "," + beforeMaximize.WorkArea.Y + " " +
                        beforeMaximize.WorkArea.Width + "x" + beforeMaximize.WorkArea.Height +
                        " at " + beforeMaximize.Dpi + " DPI.");

                    var presenter = window.AppWindow.Presenter as OverlappedPresenter;
                    Verify.IsNotNull(presenter);
                    presenter.Maximize();
                    if (presenter.State != OverlappedPresenterState.Maximized)
                    {
                        skipReason = "This environment refused maximized application. The presenter reports " +
                            presenter.State + " after Maximize().";
                        return;
                    }

                    Verify.IsTrue(window.TryGetPlacement(out savedPlacement));
                    if (savedPlacement.State != WindowPlacementState.Maximized)
                    {
                        skipReason = "This environment refused maximized application. Capture reports " +
                            savedPlacement.State + " while the presenter reports Maximized.";
                        return;
                    }

                    Log.Comment("Saved maximized restore bounds: " +
                        savedPlacement.NormalRect.X + "," + savedPlacement.NormalRect.Y + " " +
                        savedPlacement.NormalRect.Width + "x" + savedPlacement.NormalRect.Height +
                        " work area " + savedPlacement.WorkArea.X + "," + savedPlacement.WorkArea.Y + " " +
                        savedPlacement.WorkArea.Width + "x" + savedPlacement.WorkArea.Height +
                        " at " + savedPlacement.Dpi + " DPI.");
                }
                finally
                {
                    window.Close();
                }

                if (skipReason != null)
                {
                    return;
                }

                var loadedPlacement = WindowPlacement.LoadForPersistPlacementId(placementId);
                Verify.IsNotNull(loadedPlacement);
                Verify.AreEqual(WindowPlacementState.Maximized, loadedPlacement.State);
                VerifyPlacementsMatch(savedPlacement, loadedPlacement);

                var restoredWindow = CreateWindow();
                try
                {
                    restoredWindow.PersistPlacementId = placementId;
                    restoredWindow.UseAutomaticPlacementPersistence = true;
                    restoredWindow.Show(new WindowShowOptions
                    {
                        CascadeBehavior = WindowCascadeBehavior.Disabled,
                        DoNotActivate = true
                    });

                    var restoredPresenter = restoredWindow.AppWindow.Presenter as OverlappedPresenter;
                    Verify.IsNotNull(restoredPresenter);
                    Verify.AreEqual(OverlappedPresenterState.Maximized, restoredPresenter.State);

                    Verify.IsTrue(restoredWindow.TryGetPlacement(out var restoredPlacement));
                    Log.Comment("Restored maximized restore bounds: " +
                        restoredPlacement.NormalRect.X + "," + restoredPlacement.NormalRect.Y + " " +
                        restoredPlacement.NormalRect.Width + "x" + restoredPlacement.NormalRect.Height +
                        " work area " + restoredPlacement.WorkArea.X + "," + restoredPlacement.WorkArea.Y + " " +
                        restoredPlacement.WorkArea.Width + "x" + restoredPlacement.WorkArea.Height +
                        " at " + restoredPlacement.Dpi + " DPI.");
                    Verify.AreEqual(WindowPlacementState.Maximized, restoredPlacement.State);
                    VerifyPlacementsMatch(savedPlacement, restoredPlacement);
                }
                finally
                {
                    restoredWindow.Close();
                }
            });

            if (skipReason != null)
            {
                Log.Result(TestResult.Skipped, skipReason);
            }
        }

        [TestMethod]
        public void MinimizedPlacementSurvivesCloseAndRestore()
        {
            string skipReason = null;

            UIExecutor.Execute(() =>
            {
                var placementId = "WindowPlacementPersistenceTests." + Guid.NewGuid().ToString("N");
                Verify.IsNull(WindowPlacement.LoadForPersistPlacementId(placementId));

                var window = CreateWindow();
                WindowPlacement savedPlacement = null;
                try
                {
                    window.PersistPlacementId = placementId;
                    window.UseAutomaticPlacementPersistence = true;
                    window.Show(new WindowShowOptions
                    {
                        Placement = CreatePlacement(200, 150, 640, 480),
                        CascadeBehavior = WindowCascadeBehavior.Disabled,
                        DoNotActivate = true
                    });

                    Verify.IsTrue(window.TryGetPlacement(out var beforeMinimize));
                    Log.Comment("Before minimize: normal " +
                        beforeMinimize.NormalRect.X + "," + beforeMinimize.NormalRect.Y + " " +
                        beforeMinimize.NormalRect.Width + "x" + beforeMinimize.NormalRect.Height +
                        " work area " + beforeMinimize.WorkArea.X + "," + beforeMinimize.WorkArea.Y + " " +
                        beforeMinimize.WorkArea.Width + "x" + beforeMinimize.WorkArea.Height +
                        " at " + beforeMinimize.Dpi + " DPI.");

                    var presenter = window.AppWindow.Presenter as OverlappedPresenter;
                    Verify.IsNotNull(presenter);
                    presenter.Minimize();
                    if (presenter.State != OverlappedPresenterState.Minimized)
                    {
                        skipReason = "This environment refused minimized application. The presenter reports " +
                            presenter.State + " after Minimize().";
                        return;
                    }

                    Verify.IsTrue(window.TryGetPlacement(out savedPlacement));
                    if (savedPlacement.State != WindowPlacementState.Minimized)
                    {
                        skipReason = "This environment refused minimized application. Capture reports " +
                            savedPlacement.State + " while the presenter reports Minimized.";
                        return;
                    }

                    Log.Comment("Saved minimized restore bounds: " +
                        savedPlacement.NormalRect.X + "," + savedPlacement.NormalRect.Y + " " +
                        savedPlacement.NormalRect.Width + "x" + savedPlacement.NormalRect.Height +
                        " work area " + savedPlacement.WorkArea.X + "," + savedPlacement.WorkArea.Y + " " +
                        savedPlacement.WorkArea.Width + "x" + savedPlacement.WorkArea.Height +
                        " at " + savedPlacement.Dpi + " DPI.");
                }
                finally
                {
                    window.Close();
                }

                if (skipReason != null)
                {
                    return;
                }

                var loadedPlacement = WindowPlacement.LoadForPersistPlacementId(placementId);
                Verify.IsNotNull(loadedPlacement);
                Verify.AreEqual(WindowPlacementState.Minimized, loadedPlacement.State);
                VerifyPlacementsMatch(savedPlacement, loadedPlacement);

                var restoredWindow = CreateWindow();
                try
                {
                    restoredWindow.PersistPlacementId = placementId;
                    restoredWindow.UseAutomaticPlacementPersistence = true;
                    restoredWindow.Show(new WindowShowOptions
                    {
                        // Default restores to normal; restart preserves saved minimization.
                        Reason = WindowShowReason.ApplicationRestart,
                        CascadeBehavior = WindowCascadeBehavior.Disabled,
                        DoNotActivate = true
                    });

                    var restoredPresenter = restoredWindow.AppWindow.Presenter as OverlappedPresenter;
                    Verify.IsNotNull(restoredPresenter);
                    Verify.AreEqual(OverlappedPresenterState.Minimized, restoredPresenter.State);

                    Verify.IsTrue(restoredWindow.TryGetPlacement(out var restoredPlacement));
                    Log.Comment("Restored minimized restore bounds: " +
                        restoredPlacement.NormalRect.X + "," + restoredPlacement.NormalRect.Y + " " +
                        restoredPlacement.NormalRect.Width + "x" + restoredPlacement.NormalRect.Height +
                        " work area " + restoredPlacement.WorkArea.X + "," + restoredPlacement.WorkArea.Y + " " +
                        restoredPlacement.WorkArea.Width + "x" + restoredPlacement.WorkArea.Height +
                        " at " + restoredPlacement.Dpi + " DPI.");
                    Verify.AreEqual(WindowPlacementState.Minimized, restoredPlacement.State);
                    VerifyPlacementsMatch(savedPlacement, restoredPlacement);
                }
                finally
                {
                    restoredWindow.Close();
                }
            });

            if (skipReason != null)
            {
                Log.Result(TestResult.Skipped, skipReason);
            }
        }

        [TestMethod]
        public void DetachedLoadIsIsolatedPerPersistPlacementId()
        {
            UIExecutor.Execute(() =>
            {
                var firstId = "WindowPlacementPersistenceTests." + Guid.NewGuid().ToString("N");
                var secondId = "WindowPlacementPersistenceTests." + Guid.NewGuid().ToString("N");
                Verify.IsNull(WindowPlacement.LoadForPersistPlacementId(firstId));
                Verify.IsNull(WindowPlacement.LoadForPersistPlacementId(secondId));

                var firstSaved = SaveThroughClosedWindow(firstId, CreatePlacement(100, 100, 500, 400));
                var secondSaved = SaveThroughClosedWindow(secondId, CreatePlacement(200, 150, 640, 480));

                // The two ids must describe different geometry, otherwise the isolation check below
                // would pass even if the store ignored the id.
                Verify.AreNotEqual(firstSaved.NormalRect.Width, secondSaved.NormalRect.Width);
                Verify.AreNotEqual(firstSaved.NormalRect.Height, secondSaved.NormalRect.Height);

                var firstLoaded = WindowPlacement.LoadForPersistPlacementId(firstId);
                var secondLoaded = WindowPlacement.LoadForPersistPlacementId(secondId);
                Verify.IsNotNull(firstLoaded);
                Verify.IsNotNull(secondLoaded);

                VerifyPlacementsMatch(firstSaved, firstLoaded);
                VerifyPlacementsMatch(secondSaved, secondLoaded);
            });
        }

        [TestMethod]
        public void DetachedLoadReturnsTheMostRecentSaveForAPersistPlacementId()
        {
            UIExecutor.Execute(() =>
            {
                var placementId = "WindowPlacementPersistenceTests." + Guid.NewGuid().ToString("N");
                Verify.IsNull(WindowPlacement.LoadForPersistPlacementId(placementId));

                var firstSaved = SaveThroughClosedWindow(placementId, CreatePlacement(100, 100, 500, 400));
                var secondSaved = SaveThroughClosedWindow(placementId, CreatePlacement(200, 150, 640, 480));

                // The two saves must describe different geometry, otherwise the check below would
                // pass even if the second save never replaced the first.
                Verify.AreNotEqual(firstSaved.NormalRect.Width, secondSaved.NormalRect.Width);
                Verify.AreNotEqual(firstSaved.NormalRect.Height, secondSaved.NormalRect.Height);

                var loadedPlacement = WindowPlacement.LoadForPersistPlacementId(placementId);
                Verify.IsNotNull(loadedPlacement);
                VerifyPlacementsMatch(secondSaved, loadedPlacement);
                Verify.AreNotEqual(firstSaved.NormalRect.Width, loadedPlacement.NormalRect.Width);
                Verify.AreNotEqual(firstSaved.NormalRect.Height, loadedPlacement.NormalRect.Height);
            });
        }

        [TestMethod]
        public void AutomaticPersistenceOptOutDoesNotSave()
        {
            UIExecutor.Execute(() =>
            {
                var placementId = "WindowPlacementPersistenceTests." + Guid.NewGuid().ToString("N");
                Verify.IsNull(WindowPlacement.LoadForPersistPlacementId(placementId));

                var window = CreateWindow();
                try
                {
                    window.PersistPlacementId = placementId;
                    window.UseAutomaticPlacementPersistence = false;
                    window.Show(new WindowShowOptions
                    {
                        Placement = CreatePlacement(),
                        CascadeBehavior = WindowCascadeBehavior.Disabled,
                        DoNotActivate = true
                    });
                    Verify.IsTrue(window.Visible);
                }
                finally
                {
                    window.Close();
                }

                Verify.IsNull(WindowPlacement.LoadForPersistPlacementId(placementId));

                // The same id must be saveable with the opt-in, otherwise the check above would
                // pass even if the id were misspelled or the store were broken for every id.
                var savedPlacement = SaveThroughClosedWindow(placementId, CreatePlacement(200, 150, 640, 480));
                var loadedPlacement = WindowPlacement.LoadForPersistPlacementId(placementId);
                Verify.IsNotNull(loadedPlacement);
                VerifyPlacementsMatch(savedPlacement, loadedPlacement);
            });
        }

        [TestMethod]
        public void SaveUsesThePersistIdSetAtCloseTime()
        {
            UIExecutor.Execute(() =>
            {
                var enrolledId = "WindowPlacementPersistenceTests." + Guid.NewGuid().ToString("N");
                var closeTimeId = "WindowPlacementPersistenceTests." + Guid.NewGuid().ToString("N");
                Verify.IsNull(WindowPlacement.LoadForPersistPlacementId(enrolledId));
                Verify.IsNull(WindowPlacement.LoadForPersistPlacementId(closeTimeId));

                var window = CreateWindow();
                WindowPlacement savedPlacement = null;
                try
                {
                    window.PersistPlacementId = enrolledId;
                    window.UseAutomaticPlacementPersistence = true;
                    window.Show(new WindowShowOptions
                    {
                        Placement = CreatePlacement(200, 150, 640, 480),
                        CascadeBehavior = WindowCascadeBehavior.Disabled,
                        DoNotActivate = true
                    });
                    Verify.IsTrue(window.TryGetPlacement(out savedPlacement));

                    // The save must snapshot the id that is current when the window closes,
                    // not the id the window enrolled with on first display.
                    window.PersistPlacementId = closeTimeId;
                }
                finally
                {
                    window.Close();
                }

                Verify.IsNull(WindowPlacement.LoadForPersistPlacementId(enrolledId));

                var loadedPlacement = WindowPlacement.LoadForPersistPlacementId(closeTimeId);
                Verify.IsNotNull(loadedPlacement);
                VerifyPlacementsMatch(savedPlacement, loadedPlacement);
            });
        }

        [TestMethod]
        public void LateAutomaticPersistenceOptOutCancelsTheSave()
        {
            UIExecutor.Execute(() =>
            {
                var placementId = "WindowPlacementPersistenceTests." + Guid.NewGuid().ToString("N");
                Verify.IsNull(WindowPlacement.LoadForPersistPlacementId(placementId));

                var window = CreateWindow();
                try
                {
                    window.PersistPlacementId = placementId;
                    window.UseAutomaticPlacementPersistence = true;
                    window.Show(new WindowShowOptions
                    {
                        Placement = CreatePlacement(),
                        CascadeBehavior = WindowCascadeBehavior.Disabled,
                        DoNotActivate = true
                    });
                    Verify.IsTrue(window.Visible);

                    // The window enrolled with the opt-in, so the save must honor the Boolean
                    // that is current when the window closes, not the one it enrolled with.
                    window.UseAutomaticPlacementPersistence = false;
                }
                finally
                {
                    window.Close();
                }

                Verify.IsNull(WindowPlacement.LoadForPersistPlacementId(placementId));

                // The same id must be saveable with the opt-in left on, otherwise the check
                // above would pass even if the id were misspelled or the store were broken.
                var savedPlacement = SaveThroughClosedWindow(placementId, CreatePlacement(200, 150, 640, 480));
                var loadedPlacement = WindowPlacement.LoadForPersistPlacementId(placementId);
                Verify.IsNotNull(loadedPlacement);
                VerifyPlacementsMatch(savedPlacement, loadedPlacement);
            });
        }

        [TestMethod]
        public void SaveUsesTheGeometrySetByACloseHandler()
        {
            UIExecutor.Execute(() =>
            {
                var placementId = "WindowPlacementPersistenceTests." + Guid.NewGuid().ToString("N");
                Verify.IsNull(WindowPlacement.LoadForPersistPlacementId(placementId));

                var window = CreateWindow();
                WindowPlacement firstDisplayPlacement = null;
                WindowPlacement closeTimePlacement = null;
                var capturedAtCloseTime = false;

                try
                {
                    window.PersistPlacementId = placementId;
                    window.UseAutomaticPlacementPersistence = true;

                    // The save snapshot runs after the Closed handlers return, so a move made
                    // here must be the geometry that reaches storage.
                    window.Closed += (sender, args) =>
                    {
                        window.AppWindow.MoveAndResize(new global::Windows.Graphics.RectInt32(320, 240, 720, 560));
                        capturedAtCloseTime = window.TryGetPlacement(out closeTimePlacement);
                    };

                    window.Show(new WindowShowOptions
                    {
                        Placement = CreatePlacement(200, 150, 640, 480),
                        CascadeBehavior = WindowCascadeBehavior.Disabled,
                        DoNotActivate = true
                    });
                    Verify.IsTrue(window.TryGetPlacement(out firstDisplayPlacement));
                }
                finally
                {
                    window.Close();
                }

                Verify.IsTrue(capturedAtCloseTime);
                Verify.IsNotNull(closeTimePlacement);
                Verify.IsNotNull(firstDisplayPlacement);

                Log.Comment($"First display {Describe(firstDisplayPlacement)}; close time {Describe(closeTimePlacement)}");

                // Without a real change the rest of this test would pass even if the save
                // captured the window before the handler ran.
                Verify.IsFalse(RectanglesMatch(firstDisplayPlacement.NormalRect, closeTimePlacement.NormalRect));

                var loadedPlacement = WindowPlacement.LoadForPersistPlacementId(placementId);
                Verify.IsNotNull(loadedPlacement);
                VerifyPlacementsMatch(closeTimePlacement, loadedPlacement);
            });
        }

        [TestMethod]
        public void EmptyPersistIdAtFirstDisplayNeverEnrollsSaving()
        {
            UIExecutor.Execute(() =>
            {
                var placementId = "WindowPlacementPersistenceTests." + Guid.NewGuid().ToString("N");
                Verify.IsNull(WindowPlacement.LoadForPersistPlacementId(placementId));

                var window = CreateWindow();
                try
                {
                    // No persist id at first display, so the window never enrolls. A later id
                    // gates saves on an enrolled window; it cannot create enrollment.
                    window.UseAutomaticPlacementPersistence = true;
                    window.Show(new WindowShowOptions
                    {
                        Placement = CreatePlacement(),
                        CascadeBehavior = WindowCascadeBehavior.Disabled,
                        DoNotActivate = true
                    });
                    Verify.IsTrue(window.Visible);

                    window.PersistPlacementId = placementId;
                }
                finally
                {
                    window.Close();
                }

                Verify.IsNull(WindowPlacement.LoadForPersistPlacementId(placementId));

                // The same id must be saveable from a window that enrolled with it, otherwise the
                // check above would pass even if the id were misspelled or the store were broken.
                var savedPlacement = SaveThroughClosedWindow(placementId, CreatePlacement(200, 150, 640, 480));
                var loadedPlacement = WindowPlacement.LoadForPersistPlacementId(placementId);
                Verify.IsNotNull(loadedPlacement);
                VerifyPlacementsMatch(savedPlacement, loadedPlacement);
            });
        }

        [TestMethod]
        public void RestartShowRemainsNonActivatingWhenPlacementIsSkipped()
        {
            UIExecutor.Execute(() =>
            {
                var window = CreateWindow();
                try
                {
                    window.Show(new WindowShowOptions
                    {
                        Reason = WindowShowReason.ApplicationRestart,
                        SkipInitialPlacement = true
                    });

                    Verify.IsTrue(window.Visible);
                }
                finally
                {
                    window.Close();
                }
            });
        }

        [TestMethod]
        public void LegacyShowDoesNotMoveOrResizeTheWindow()
        {
            VerifyLegacyDisplayPreservesGeometry(window => window.Show());
        }

        [TestMethod]
        public void LegacyActivateDoesNotMoveOrResizeTheWindow()
        {
            VerifyLegacyDisplayPreservesGeometry(window => window.Activate());
        }

        // A window that never touches the placement API must display exactly where the
        // application put it. The automatic pass still runs, but with no explicit placement,
        // no persist id, no opt-in, and no launch hint there is no source, so it must not
        // move, resize, or cascade the window.
        private static void VerifyLegacyDisplayPreservesGeometry(Action<Window> display)
        {
            UIExecutor.Execute(() =>
            {
                var window = CreateWindow();
                try
                {
                    var appWindow = window.AppWindow;
                    Verify.IsNotNull(appWindow);

                    appWindow.MoveAndResize(new global::Windows.Graphics.RectInt32(120, 90, 560, 420));

                    // Read back what the OS actually accepted while hidden. That, not the
                    // requested rectangle, is the geometry the display path must preserve.
                    var expectedPosition = appWindow.Position;
                    var expectedSize = appWindow.Size;
                    Verify.IsFalse(window.Visible);

                    display(window);

                    Verify.IsTrue(window.Visible);

                    var actualPosition = appWindow.Position;
                    var actualSize = appWindow.Size;
                    Log.Comment($"Expected {expectedPosition.X},{expectedPosition.Y} {expectedSize.Width}x{expectedSize.Height}; " +
                        $"actual {actualPosition.X},{actualPosition.Y} {actualSize.Width}x{actualSize.Height}");

                    Verify.AreEqual(expectedPosition.X, actualPosition.X);
                    Verify.AreEqual(expectedPosition.Y, actualPosition.Y);
                    Verify.AreEqual(expectedSize.Width, actualSize.Width);
                    Verify.AreEqual(expectedSize.Height, actualSize.Height);
                }
                finally
                {
                    window.Close();
                }
            });
        }

        internal static Window CreateWindow()
        {
            var window = new Window
            {
                Content = new Microsoft.UI.Xaml.Controls.StackPanel()
            };
            return window;
        }

        private static WindowPlacement CreatePlacement()
        {
            return CreatePlacement(100, 100, 500, 400);
        }

        internal static WindowPlacement CreatePlacement(int x, int y, int width, int height)
        {
            return new WindowPlacement(
                new global::Windows.Graphics.RectInt32(x, y, width, height),
                new global::Windows.Graphics.RectInt32(0, 0, 1920, 1080),
                96);
        }

        internal static WindowPlacement SaveThroughClosedWindow(string placementId, WindowPlacement placement)
        {
            var window = CreateWindow();
            WindowPlacement savedPlacement = null;
            try
            {
                window.PersistPlacementId = placementId;
                window.UseAutomaticPlacementPersistence = true;
                window.Show(new WindowShowOptions
                {
                    Placement = placement,
                    CascadeBehavior = WindowCascadeBehavior.Disabled,
                    DoNotActivate = true
                });
                Verify.IsTrue(window.TryGetPlacement(out savedPlacement));
            }
            finally
            {
                window.Close();
            }

            return savedPlacement;
        }

        private static void VerifyPlacementsMatch(WindowPlacement expected, WindowPlacement actual)
        {
            Verify.AreEqual(expected.NormalRect.X, actual.NormalRect.X);
            Verify.AreEqual(expected.NormalRect.Y, actual.NormalRect.Y);
            Verify.AreEqual(expected.NormalRect.Width, actual.NormalRect.Width);
            Verify.AreEqual(expected.NormalRect.Height, actual.NormalRect.Height);
            Verify.AreEqual(expected.Dpi, actual.Dpi);
        }

        private static bool RectanglesMatch(global::Windows.Graphics.RectInt32 left, global::Windows.Graphics.RectInt32 right)
        {
            return left.X == right.X
                && left.Y == right.Y
                && left.Width == right.Width
                && left.Height == right.Height;
        }

        private static string Describe(WindowPlacement placement)
        {
            var rect = placement.NormalRect;
            return $"{rect.X},{rect.Y} {rect.Width}x{rect.Height} at {placement.Dpi} dpi";
        }

        private static void AssertOptionsRejected(Action action)
        {
            try
            {
                action();
                Verify.Fail("Expected null options to be rejected.");
            }
            catch (ArgumentException error)
            {
                Verify.AreEqual(unchecked((int)0x80070057), error.HResult);
            }
        }
    }
}
