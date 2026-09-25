// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Windowing;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Tests.Common;
using Private.Infrastructure.Hosting.WPF;
using WEX.TestExecution;
using WEX.TestExecution.Markup;

namespace Microsoft.UI.Xaml.Tests.Hosting.Win32.WPF
{
    // Covers the public Window.Show() and Window.Hide() display surface described in
    // "Control display and activation" in the window placement persistence spec.
    [TestClass]
    public class WindowShowHideTests : XamlTestsBase
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
        public void ParameterlessShowDisplaysTheWindow()
        {
            UIExecutor.Execute(() =>
            {
                var window = CreateWindow();
                try
                {
                    Verify.IsFalse(window.Visible);

                    window.Show();

                    Verify.IsTrue(window.Visible);
                }
                finally
                {
                    window.Close();
                }
            });
        }

        [TestMethod]
        public void HideHidesTheWindowWithoutClosingIt()
        {
            UIExecutor.Execute(() =>
            {
                var window = CreateWindow();
                try
                {
                    window.Show(new WindowShowOptions { DoNotActivate = true });
                    Verify.IsTrue(window.Visible);

                    window.Hide();

                    Verify.IsFalse(window.Visible);

                    // The window and its content are still alive after hiding.
                    Verify.IsNotNull(window.Content);
                    Verify.IsNotNull(window.AppWindow);
                }
                finally
                {
                    window.Close();
                }
            });
        }

        [TestMethod]
        public void ShowAfterHideRevealsTheWindowAgain()
        {
            UIExecutor.Execute(() =>
            {
                var window = CreateWindow();
                try
                {
                    window.Show(new WindowShowOptions { DoNotActivate = true });
                    window.Hide();
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
        public void HideOnAHiddenWindowIsANoOp()
        {
            UIExecutor.Execute(() =>
            {
                var window = CreateWindow();
                try
                {
                    // Never displayed.
                    window.Hide();
                    Verify.IsFalse(window.Visible);

                    window.Show(new WindowShowOptions { DoNotActivate = true });
                    window.Hide();
                    window.Hide();

                    Verify.IsFalse(window.Visible);
                }
                finally
                {
                    window.Close();
                }
            });
        }

        [TestMethod]
        public void HideDoesNotEndTheInitialPlacementPhase()
        {
            UIExecutor.Execute(() =>
            {
                var window = CreateWindow();
                try
                {
                    window.Hide();

                    // The phase is still open, so hidden application still runs.
                    Verify.IsTrue(window.TryApplyInitialPlacement(new WindowShowOptions
                    {
                        Placement = CreatePlacement(),
                        DoNotActivate = true
                    }));
                    Verify.IsFalse(window.Visible);
                }
                finally
                {
                    window.Close();
                }
            });
        }

        [TestMethod]
        public void HideDoesNotReopenTheInitialPlacementPhase()
        {
            UIExecutor.Execute(() =>
            {
                var window = CreateWindow();
                try
                {
                    window.Show(new WindowShowOptions { DoNotActivate = true });
                    window.Hide();

                    // First display already ended the phase; hiding does not bring it back.
                    Verify.IsFalse(window.TryApplyInitialPlacement(new WindowShowOptions
                    {
                        Placement = CreatePlacement(),
                        DoNotActivate = true
                    }));
                }
                finally
                {
                    window.Close();
                }
            });
        }

        [TestMethod]
        public void HidePreservesCapturedPlacement()
        {
            UIExecutor.Execute(() =>
            {
                var window = CreateWindow();
                try
                {
                    window.Show(new WindowShowOptions { DoNotActivate = true });
                    Verify.IsTrue(window.TryGetPlacement(out var shownPlacement));

                    window.Hide();

                    Verify.IsTrue(window.TryGetPlacement(out var hiddenPlacement));
                    Verify.AreEqual(shownPlacement.NormalRect.X, hiddenPlacement.NormalRect.X);
                    Verify.AreEqual(shownPlacement.NormalRect.Y, hiddenPlacement.NormalRect.Y);
                    Verify.AreEqual(shownPlacement.NormalRect.Width, hiddenPlacement.NormalRect.Width);
                    Verify.AreEqual(shownPlacement.NormalRect.Height, hiddenPlacement.NormalRect.Height);
                }
                finally
                {
                    window.Close();
                }
            });
        }

        [TestMethod]
        public void ShowDoesNotUnminimizeButActivateDoes()
        {
            UIExecutor.Execute(() =>
            {
                var window = CreateWindow();
                try
                {
                    window.Show(new WindowShowOptions { DoNotActivate = true });

                    var presenter = window.AppWindow.Presenter as OverlappedPresenter;
                    Verify.IsNotNull(presenter);

                    presenter.Minimize();
                    Verify.AreEqual(OverlappedPresenterState.Minimized, presenter.State);

                    window.Show();
                    Verify.AreEqual(OverlappedPresenterState.Minimized, presenter.State);

                    window.Activate();
                    Verify.AreNotEqual(OverlappedPresenterState.Minimized, presenter.State);
                }
                finally
                {
                    window.Close();
                }
            });
        }

        [TestMethod]
        public void HideAfterCloseFails()
        {
            UIExecutor.Execute(() =>
            {
                var window = CreateWindow();
                window.Show(new WindowShowOptions { DoNotActivate = true });
                window.Close();

                try
                {
                    window.Hide();
                    Verify.Fail("Expected Hide() on a closed window to fail.");
                }
                catch (global::System.Exception)
                {
                }
            });
        }

        private static Window CreateWindow()
        {
            return new Window
            {
                Content = new Microsoft.UI.Xaml.Controls.StackPanel()
            };
        }

        private static WindowPlacement CreatePlacement()
        {
            return new WindowPlacement(
                new global::Windows.Graphics.RectInt32(100, 100, 500, 400),
                new global::Windows.Graphics.RectInt32(0, 0, 1920, 1080),
                96);
        }
    }
}
