// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Tests.Common;
using Private.Infrastructure.Hosting.WPF;
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using Windows.Graphics;
using HostInterop = Private.Infrastructure.Hosting.WPF.Interop;

namespace Microsoft.UI.Xaml.Tests.Hosting.Win32.WPF
{
    [TestClass]
    public class WindowPlacementExplicitCascadeTests : XamlTestsBase
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
        public void EnabledCascadesExplicitPlacementOnlyByPosition()
        {
            RunCascadeScenario(
                WindowCascadeBehavior.Enabled,
                WindowShowReason.Default,
                includeTargetPeer: true,
                includeOffMonitorPeer: false,
                raiseOffMonitorPeer: false,
                expectCascade: true);
        }

        [TestMethod]
        public void AutomaticDoesNotCascadeExplicitPlacement()
        {
            RunCascadeScenario(
                WindowCascadeBehavior.Automatic,
                WindowShowReason.Default,
                includeTargetPeer: true,
                includeOffMonitorPeer: false,
                raiseOffMonitorPeer: false,
                expectCascade: false);
        }

        [TestMethod]
        public void DisabledDoesNotCascadeExplicitPlacement()
        {
            RunCascadeScenario(
                WindowCascadeBehavior.Disabled,
                WindowShowReason.Default,
                includeTargetPeer: true,
                includeOffMonitorPeer: false,
                raiseOffMonitorPeer: false,
                expectCascade: false);
        }

        [TestMethod]
        public void ApplicationRestartDoesNotCascadeExplicitPlacement()
        {
            RunCascadeScenario(
                WindowCascadeBehavior.Enabled,
                WindowShowReason.ApplicationRestart,
                includeTargetPeer: true,
                includeOffMonitorPeer: false,
                raiseOffMonitorPeer: false,
                expectCascade: false);
        }

        [TestMethod]
        public void OffMonitorPeerIsSkipped()
        {
            // The only peer in the group lives on another monitor, so nothing qualifies.
            RunCascadeScenario(
                WindowCascadeBehavior.Enabled,
                WindowShowReason.Default,
                includeTargetPeer: false,
                includeOffMonitorPeer: true,
                raiseOffMonitorPeer: false,
                expectCascade: false);
        }

        [TestMethod]
        public void LowerZOrderTargetMonitorPeerIsFoundAfterOffMonitorPeer()
        {
            // The off-monitor peer sits above the target-monitor peer in Z order. The
            // walk must skip it and keep going rather than stop at the first peer.
            RunCascadeScenario(
                WindowCascadeBehavior.Enabled,
                WindowShowReason.Default,
                includeTargetPeer: true,
                includeOffMonitorPeer: true,
                raiseOffMonitorPeer: true,
                expectCascade: true);
        }

        private static void RunCascadeScenario(
            WindowCascadeBehavior cascadeBehavior,
            WindowShowReason showReason,
            bool includeTargetPeer,
            bool includeOffMonitorPeer,
            bool raiseOffMonitorPeer,
            bool expectCascade)
        {
            var monitors = GetMonitors();
            if (includeOffMonitorPeer && monitors.Count < 2)
            {
                Log.Result(TestResult.Skipped, "This scenario requires at least two displays.");
                return;
            }

            UIExecutor.Execute(() =>
            {
                var target = monitors[0];
                var other = monitors.Count > 1 ? monitors[1] : monitors[0];
                var placementId = "WindowPlacementExplicitCascadeTests." + Guid.NewGuid().ToString("N");

                var requested = CreatePlacement(target);
                requested.State = WindowPlacementState.Maximized;
                var requestedSnapshot = ClonePlacement(requested);

                // The platform applies its own position fitting to an explicit placement, so
                // the uncascaded result is not always the literal requested rect. Measure the
                // uncascaded result first and compare against that instead of the raw request.
                var baseline = MeasureUncascadedPosition(requested);
                Log.Comment($"Uncascaded baseline position: {baseline.X},{baseline.Y}");

                var targetPeer = includeTargetPeer ? CreateWindow() : null;
                var offMonitorPeer = includeOffMonitorPeer ? CreateWindow() : null;
                var caller = CreateWindow();

                try
                {
                    if (targetPeer != null)
                    {
                        ShowPeer(targetPeer, placementId, CreatePlacement(target));
                    }

                    if (offMonitorPeer != null)
                    {
                        ShowPeer(offMonitorPeer, placementId, CreatePlacement(other));
                    }

                    if (raiseOffMonitorPeer && offMonitorPeer != null)
                    {
                        // Put the off-monitor peer above the target-monitor peer in Z order.
                        HostInterop.SetWindowPos(
                            HostInterop.GetWindowFromWindowId(offMonitorPeer.AppWindow.Id),
                            HWND_TOP,
                            0,
                            0,
                            0,
                            0,
                            SetWindowPosFlags.SWP_NOMOVE | SetWindowPosFlags.SWP_NOSIZE | SetWindowPosFlags.SWP_NOACTIVATE);
                    }

                    caller.PersistPlacementId = placementId;
                    caller.Show(new WindowShowOptions
                    {
                        Placement = requested,
                        Reason = showReason,
                        CascadeBehavior = cascadeBehavior,
                        DoNotActivate = true
                    });

                    Verify.IsTrue(caller.TryGetPlacement(out var actual));
                    Log.Comment($"Resulting position: {actual.NormalRect.X},{actual.NormalRect.Y}");

                    // Cascading only moves the window. Everything else must survive.
                    VerifyPlacementPreserved(requestedSnapshot, actual, includePosition: false, includeDpi: false);

                    // The caller's own object must never be mutated.
                    VerifyPlacementPreserved(requestedSnapshot, requested, includePosition: true, includeDpi: true);

                    if (expectCascade)
                    {
                        Verify.IsTrue(
                            actual.NormalRect.X != baseline.X || actual.NormalRect.Y != baseline.Y,
                            "Expected the explicit placement to be cascaded away from the uncascaded position.");
                        Verify.IsTrue(
                            actual.NormalRect.X > baseline.X && actual.NormalRect.Y > baseline.Y,
                            "A cascade offsets down and to the right.");
                    }
                    else
                    {
                        Verify.AreEqual(baseline.X, actual.NormalRect.X);
                        Verify.AreEqual(baseline.Y, actual.NormalRect.Y);
                    }
                }
                finally
                {
                    caller.Close();
                    offMonitorPeer?.Close();
                    targetPeer?.Close();
                }
            });
        }

        // Shows a window with the requested placement in a placement group that has no
        // other members, so no cascade can happen. The result is the uncascaded position.
        private static PointInt32 MeasureUncascadedPosition(WindowPlacement requested)
        {
            var control = CreateWindow();
            try
            {
                control.PersistPlacementId = "WindowPlacementExplicitCascadeTests.Baseline." + Guid.NewGuid().ToString("N");
                control.Show(new WindowShowOptions
                {
                    Placement = ClonePlacement(requested),
                    CascadeBehavior = WindowCascadeBehavior.Disabled,
                    DoNotActivate = true
                });

                Verify.IsTrue(control.TryGetPlacement(out var placement));
                return new PointInt32(placement.NormalRect.X, placement.NormalRect.Y);
            }
            finally
            {
                control.Close();
            }
        }

        private static void ShowPeer(Window window, string placementId, WindowPlacement placement)
        {
            window.PersistPlacementId = placementId;
            window.Show(new WindowShowOptions
            {
                Placement = placement,
                CascadeBehavior = WindowCascadeBehavior.Disabled,
                DoNotActivate = true
            });
        }

        private static Window CreateWindow()
        {
            return new Window
            {
                Content = new Microsoft.UI.Xaml.Controls.StackPanel()
            };
        }

        private static WindowPlacement CreatePlacement(MonitorInfo monitor)
        {
            return new WindowPlacement(
                new RectInt32(monitor.WorkArea.X + 80, monitor.WorkArea.Y + 80, 500, 400),
                monitor.WorkArea,
                monitor.Dpi)
            {
                DisplayDeviceName = monitor.DeviceName
            };
        }

        private static WindowPlacement ClonePlacement(WindowPlacement placement)
        {
            return new WindowPlacement(placement.NormalRect, placement.WorkArea, placement.Dpi)
            {
                State = placement.State,
                SnapRect = placement.SnapRect,
                DisplayDeviceName = placement.DisplayDeviceName,
                VirtualDesktopId = placement.VirtualDesktopId
            };
        }

        private static void VerifyPlacementPreserved(
            WindowPlacement expected,
            WindowPlacement actual,
            bool includePosition,
            bool includeDpi)
        {
            if (includePosition)
            {
                Verify.AreEqual(expected.NormalRect.X, actual.NormalRect.X);
                Verify.AreEqual(expected.NormalRect.Y, actual.NormalRect.Y);
            }

            if (includeDpi)
            {
                Verify.AreEqual(expected.Dpi, actual.Dpi);
            }

            Verify.AreEqual(expected.NormalRect.Width, actual.NormalRect.Width);
            Verify.AreEqual(expected.NormalRect.Height, actual.NormalRect.Height);
            Verify.AreEqual(expected.WorkArea.X, actual.WorkArea.X);
            Verify.AreEqual(expected.WorkArea.Y, actual.WorkArea.Y);
            Verify.AreEqual(expected.WorkArea.Width, actual.WorkArea.Width);
            Verify.AreEqual(expected.WorkArea.Height, actual.WorkArea.Height);
            Verify.AreEqual(expected.DisplayDeviceName, actual.DisplayDeviceName);
            Verify.AreEqual(expected.State, actual.State);
            Verify.AreEqual(expected.SnapRect, actual.SnapRect);
            Verify.AreEqual(expected.VirtualDesktopId, actual.VirtualDesktopId);
        }

        private static List<MonitorInfo> GetMonitors()
        {
            var monitors = new List<MonitorInfo>();
            MonitorEnumProc callback = (IntPtr monitor, IntPtr hdc, ref RECT rect, IntPtr data) =>
            {
                var info = new MONITORINFOEX { CbSize = Marshal.SizeOf<MONITORINFOEX>() };
                if (GetMonitorInfo(monitor, ref info))
                {
                    monitors.Add(new MonitorInfo
                    {
                        DeviceName = info.SzDevice,
                        Dpi = GetEffectiveDpi(monitor),
                        WorkArea = new RectInt32(
                            info.RcWork.Left,
                            info.RcWork.Top,
                            info.RcWork.Right - info.RcWork.Left,
                            info.RcWork.Bottom - info.RcWork.Top)
                    });
                }
                return true;
            };

            EnumDisplayMonitors(IntPtr.Zero, IntPtr.Zero, callback, IntPtr.Zero);
            return monitors;
        }

        private static int GetEffectiveDpi(IntPtr monitor)
        {
            if (GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, out uint dpiX, out uint _) == 0)
            {
                return (int)dpiX;
            }

            return 96;
        }

        private sealed class MonitorInfo
        {
            public string DeviceName;
            public RectInt32 WorkArea;
            public int Dpi;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct RECT
        {
            public int Left;
            public int Top;
            public int Right;
            public int Bottom;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        private struct MONITORINFOEX
        {
            public int CbSize;
            public RECT RcMonitor;
            public RECT RcWork;
            public int DwFlags;

            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 32)]
            public string SzDevice;
        }

        private delegate bool MonitorEnumProc(IntPtr monitor, IntPtr hdc, ref RECT rect, IntPtr data);

        [DllImport("user32.dll")]
        private static extern bool EnumDisplayMonitors(
            IntPtr hdc,
            IntPtr clip,
            MonitorEnumProc callback,
            IntPtr data);

        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        private static extern bool GetMonitorInfo(IntPtr monitor, ref MONITORINFOEX info);

        [DllImport("Shcore.dll")]
        private static extern int GetDpiForMonitor(IntPtr monitor, int dpiType, out uint dpiX, out uint dpiY);

        private const int MDT_EFFECTIVE_DPI = 0;

        private static readonly IntPtr HWND_TOP = IntPtr.Zero;

        private static class SetWindowPosFlags
        {
            public const uint SWP_NOMOVE = 0x0002u;
            public const uint SWP_NOSIZE = 0x0001u;
            public const uint SWP_NOACTIVATE = 0x0010u;
        }
    }
}
