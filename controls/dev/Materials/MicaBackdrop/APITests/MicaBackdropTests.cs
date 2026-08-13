// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Runtime.InteropServices;
using Common;
using Microsoft.UI;
using Microsoft.UI.Composition.SystemBackdrops;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;
using MUXControlsTestApp.Utilities;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class MicaBackdropTests : ApiTestBase
    {
        // DWMWINDOWATTRIBUTE.DWMWA_SYSTEMBACKDROP_TYPE and the DWM_SYSTEMBACKDROP_TYPE values, neither of which
        // is projected into managed code. DWMWA_USE_IMMERSIVE_DARK_MODE selects the light or dark variant of
        // the material DWM draws.
        private const int DWMWA_SYSTEMBACKDROP_TYPE = 38;
        private const int DWMWA_USE_IMMERSIVE_DARK_MODE = 20;
        private const int DWMSBT_AUTO = 0;
        private const int DWMSBT_TRANSIENTWINDOW = 3;

        [DllImport("dwmapi.dll")]
        private static extern int DwmGetWindowAttribute(IntPtr hwnd, int attribute, out int value, int size);

        [DllImport("dwmapi.dll")]
        private static extern int DwmSetWindowAttribute(IntPtr hwnd, int attribute, ref int value, int size);

        [TestMethod]
        public void VerifyMicaBackdropAttachesToAndDetachesFromAWindow()
        {
            RunOnUIThread.Execute(() =>
            {
                UsingWindow(window =>
                {
                    var backdrop = new MicaBackdrop();

                    window.SystemBackdrop = backdrop;
                    Verify.AreSame(backdrop, window.SystemBackdrop, "The Window should report the backdrop that was set on it");

                    window.SystemBackdrop = null;
                    Verify.IsNull(window.SystemBackdrop, "Clearing the Window's SystemBackdrop should detach the backdrop");

                    // Re-attaching the same instance exercises the connect/disconnect/connect cycle of the
                    // per-target bookkeeping MicaBackdrop keeps.
                    window.SystemBackdrop = backdrop;
                    Verify.AreSame(backdrop, window.SystemBackdrop, "The backdrop should be able to re-attach to the same Window");
                });
            });
        }

        [TestMethod]
        public void VerifyKindCanBeChangedWhileAttachedToAWindow()
        {
            RunOnUIThread.Execute(() =>
            {
                UsingWindow(window =>
                {
                    var backdrop = new MicaBackdrop();
                    Verify.AreEqual(MicaKind.Base, backdrop.Kind, "MicaKind.Base is the default kind");

                    backdrop.Kind = MicaKind.BaseAlt;
                    window.SystemBackdrop = backdrop;
                    Verify.AreEqual(MicaKind.BaseAlt, backdrop.Kind, "A kind set before attaching should be kept");

                    backdrop.Kind = MicaKind.Base;
                    Verify.AreEqual(MicaKind.Base, backdrop.Kind, "The kind should be changeable while attached");
                });
            });
        }

        [TestMethod]
        public void VerifyMicaBackdropCanBeSharedBetweenWindows()
        {
            RunOnUIThread.Execute(() =>
            {
                var backdrop = new MicaBackdrop();

                UsingWindow(firstWindow =>
                {
                    UsingWindow(secondWindow =>
                    {
                        firstWindow.SystemBackdrop = backdrop;
                        secondWindow.SystemBackdrop = backdrop;

                        Verify.AreSame(backdrop, firstWindow.SystemBackdrop);
                        Verify.AreSame(backdrop, secondWindow.SystemBackdrop);

                        // Each Window keeps its own state for the shared backdrop, so detaching one has to leave
                        // the other one attached.
                        firstWindow.SystemBackdrop = null;
                        Verify.IsNull(firstWindow.SystemBackdrop);
                        Verify.AreSame(backdrop, secondWindow.SystemBackdrop);

                        backdrop.Kind = MicaKind.BaseAlt;
                        Verify.AreEqual(MicaKind.BaseAlt, backdrop.Kind, "The kind should still be changeable for the remaining Window");
                    });
                });
            });
        }

        // On the lifted composition engine MicaController draws the material into the Window's content island,
        // so Xaml has no reason to ask DWM for a window backdrop. It only does that on the system composition
        // engine, which this test process doesn't use.
        [TestMethod]
        public void VerifyNoDwmSystemBackdropIsConfiguredOnTheLiftedCompositionEngine()
        {
            RunOnUIThread.Execute(() =>
            {
                UsingWindow(window =>
                {
                    if (!TryGetDwmSystemBackdropType(window, out int backdropTypeBefore))
                    {
                        Log.Comment("DWMWA_SYSTEMBACKDROP_TYPE is not supported on this OS, skipping.");
                        return;
                    }

                    Verify.AreEqual(DWMSBT_AUTO, backdropTypeBefore, "A Xaml Window starts out without a DWM system backdrop");

                    window.SystemBackdrop = new MicaBackdrop();

                    Verify.IsTrue(TryGetDwmSystemBackdropType(window, out int backdropTypeAfter));
                    Verify.AreEqual(DWMSBT_AUTO, backdropTypeAfter, "Attaching a MicaBackdrop shouldn't configure a DWM system backdrop on the lifted composition engine");
                });
            });
        }

        // Apps can configure DWMWA_SYSTEMBACKDROP_TYPE on a Xaml Window themselves. Whatever Xaml does with that
        // attribute, it has to give the app's choice back.
        [TestMethod]
        public void VerifyAppConfiguredDwmSystemBackdropSurvivesAMicaBackdrop()
        {
            RunOnUIThread.Execute(() =>
            {
                UsingWindow(window =>
                {
                    if (!TryGetDwmSystemBackdropType(window, out _))
                    {
                        Log.Comment("DWMWA_SYSTEMBACKDROP_TYPE is not supported on this OS, skipping.");
                        return;
                    }

                    int appBackdropType = DWMSBT_TRANSIENTWINDOW;
                    IntPtr windowHandle = Win32Interop.GetWindowFromWindowId(window.AppWindow.Id);
                    Verify.AreEqual(0, DwmSetWindowAttribute(windowHandle, DWMWA_SYSTEMBACKDROP_TYPE, ref appBackdropType, sizeof(int)));

                    var backdrop = new MicaBackdrop();
                    window.SystemBackdrop = backdrop;
                    backdrop.Kind = MicaKind.BaseAlt;
                    window.SystemBackdrop = null;

                    Verify.IsTrue(TryGetDwmSystemBackdropType(window, out int backdropTypeAfter));
                    Verify.AreEqual(DWMSBT_TRANSIENTWINDOW, backdropTypeAfter, "The app's DWM system backdrop should survive attaching and detaching a MicaBackdrop");
                });
            });
        }

        // DWM draws the light variant of a material unless it is told the window is dark, so Xaml forwards the
        // theme to it along with the material. That attribute is the app's to set otherwise, and an app that
        // set it has to get its choice back - the same rule the material itself follows.
        [TestMethod]
        public void VerifyAppConfiguredDarkModeSurvivesAMicaBackdrop()
        {
            RunOnUIThread.Execute(() =>
            {
                UsingWindow(window =>
                {
                    IntPtr windowHandle = Win32Interop.GetWindowFromWindowId(window.AppWindow.Id);
                    if (DwmGetWindowAttribute(windowHandle, DWMWA_USE_IMMERSIVE_DARK_MODE, out _, sizeof(int)) != 0)
                    {
                        Log.Comment("DWMWA_USE_IMMERSIVE_DARK_MODE is not supported on this OS, skipping.");
                        return;
                    }

                    int appDarkMode = 1;
                    Verify.AreEqual(0, DwmSetWindowAttribute(windowHandle, DWMWA_USE_IMMERSIVE_DARK_MODE, ref appDarkMode, sizeof(int)));

                    var backdrop = new MicaBackdrop();
                    window.SystemBackdrop = backdrop;
                    backdrop.Kind = MicaKind.BaseAlt;
                    window.SystemBackdrop = null;

                    Verify.AreEqual(0, DwmGetWindowAttribute(windowHandle, DWMWA_USE_IMMERSIVE_DARK_MODE, out int darkModeAfter, sizeof(int)));
                    Verify.AreEqual(appDarkMode, darkModeAfter, "The app's dark mode choice should survive attaching and detaching a MicaBackdrop");
                });
            });
        }

        private static bool TryGetDwmSystemBackdropType(Window window, out int backdropType)
        {
            IntPtr windowHandle = Win32Interop.GetWindowFromWindowId(window.AppWindow.Id);
            return DwmGetWindowAttribute(windowHandle, DWMWA_SYSTEMBACKDROP_TYPE, out backdropType, sizeof(int)) == 0;
        }

        // Runs 'test' against a Window of its own, so that a backdrop or a DWM attribute a test leaves behind
        // can't leak into the app's window and the tests that run after it.
        private static void UsingWindow(Action<Window> test)
        {
            var window = new Window { Content = new Grid() };

            try
            {
                test(window);
            }
            finally
            {
                window.SystemBackdrop = null;
                window.Close();
            }
        }
    }
}
