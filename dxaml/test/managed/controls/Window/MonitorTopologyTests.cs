// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Tests.Common;
using Microsoft.Win32.SafeHandles;
using Private.Infrastructure;
using System;
using System.Buffers.Binary;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;

namespace Microsoft.UI.Xaml.Tests.Controls
{
    [TestClass]
    [TestProperty("Ignore", "True")]
    public class MonitorTopologyTests : XamlTestsBase
    {
        [ClassInitialize]
        [TestProperty("BinaryUnderTest", "Microsoft.UI.Xaml.dll")]
        [TestProperty("RunAs", "UAP")]
        [TestProperty("UAP:Praid", "XamlManagedTAEFTests")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("IgnoreInContinuousIntegration", "True")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        [TestProperty("TestPass:MinOSVer", WindowsOSVersion._24H2)]
        [TestProperty("IsolationLevel", "Class")]
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
        [WEX.TestExecution.Markup.Description("Moves the managed test window from the primary display to a 150% virtual display and verifies XamlRoot DPI.")]
        public void WindowUpdatesDpiAfterMonitorDrag()
        {
            Verify.IsTrue(Environment.Is64BitProcess, "The VM-only monitor driver currently supports x64.");

            Border content = null;
            UIExecutor.Execute(() =>
            {
                content = new Border();
                TestServices.WindowHelper.WindowContent = content;
            });
            TestServices.WindowHelper.WaitForIdle();
            TestServices.WindowHelper.RestoreForegroundWindow();
            Verify.IsTrue(TestServices.WindowHelper.IsForegroundWindow, "The managed test window must be in the foreground.");

            var hwnd = NativeMethods.GetForegroundWindow();
            Verify.AreNotEqual(IntPtr.Zero, hwnd, "GetForegroundWindow returned null.");
            var primary = VirtualDisplayTestSession.GetPrimaryDisplay();
            MoveWindow(hwnd, primary.Bounds.Left + 80, primary.Bounds.Top + 80, 800, 600);
            TestServices.WindowHelper.WaitForIdle();

            var initialDpi = NativeMethods.GetDpiForWindow(hwnd);
            Verify.AreNotEqual(144u, initialDpi, "The primary display must use a DPI other than 150% for this test.");
            WaitForRasterizationScale(content, initialDpi / 96.0);

            using (var topology = VirtualDisplayTestSession.Create(2560, 1440, 150))
            {
                try
                {
                    Log.Comment(
                        "Dragging the managed test window from primary bounds ({0},{1})-({2},{3}) to virtual bounds ({4},{5})-({6},{7}).",
                        primary.Bounds.Left,
                        primary.Bounds.Top,
                        primary.Bounds.Right,
                        primary.Bounds.Bottom,
                        topology.Display.Bounds.Left,
                        topology.Display.Bounds.Top,
                        topology.Display.Bounds.Right,
                        topology.Display.Bounds.Bottom);

                    Verify.IsTrue(NativeMethods.GetWindowRect(hwnd, out var sourceBounds), "GetWindowRect failed.");
                    var startX = sourceBounds.Left + sourceBounds.Width / 2;
                    var startY = sourceBounds.Top + 16;
                    var destinationX = topology.Display.Bounds.Left + topology.Display.Bounds.Width / 2;
                    var destinationY = topology.Display.Bounds.Top + 120;

                    DragPointer(startX, startY, destinationX, destinationY, 1500);
                    TestServices.WindowHelper.WaitForIdle();

                    WaitForWindowDpi(hwnd, 144);
                    WaitForRasterizationScale(content, 1.5);

                    var destinationMonitor = NativeMethods.MonitorFromWindow(hwnd, NativeMethods.MONITOR_DEFAULTTONEAREST);
                    Verify.AreEqual(topology.Display.Handle, destinationMonitor);
                    Verify.AreEqual(144u, NativeMethods.GetDpiForWindow(hwnd));
                }
                finally
                {
                    MoveWindow(hwnd, primary.Bounds.Left + 80, primary.Bounds.Top + 80, 800, 600);
                    TestServices.WindowHelper.WaitForIdle();
                    WaitForRasterizationScale(content, initialDpi / 96.0);
                }
            }
        }

        private static void MoveWindow(IntPtr hwnd, int left, int top, int width, int height)
        {
            Verify.IsTrue(
                NativeMethods.SetWindowPos(
                    hwnd,
                    IntPtr.Zero,
                    left,
                    top,
                    width,
                    height,
                    NativeMethods.SWP_NOZORDER | NativeMethods.SWP_NOACTIVATE | NativeMethods.SWP_NOOWNERZORDER),
                "SetWindowPos failed.");
        }

        private static void DragPointer(
            int startX,
            int startY,
            int destinationX,
            int destinationY,
            int durationMilliseconds)
        {
            Verify.IsTrue(NativeMethods.SetCursorPos(startX, startY), "SetCursorPos failed at the drag start.");
            Thread.Sleep(100);

            NativeMethods.mouse_event(NativeMethods.MOUSEEVENTF_LEFTDOWN, 0, 0, 0, UIntPtr.Zero);
            try
            {
                const int frameDurationMilliseconds = 16;
                var steps = Math.Max(1, durationMilliseconds / frameDurationMilliseconds);
                for (var step = 1; step <= steps; step++)
                {
                    var progress = (double)step / steps;
                    var x = startX + (int)Math.Round((destinationX - startX) * progress);
                    var y = startY + (int)Math.Round((destinationY - startY) * progress);
                    if (!NativeMethods.SetCursorPos(x, y))
                    {
                        throw new Win32Exception(Marshal.GetLastWin32Error(), "SetCursorPos failed during the drag.");
                    }

                    Thread.Sleep(frameDurationMilliseconds);
                }
            }
            finally
            {
                NativeMethods.mouse_event(NativeMethods.MOUSEEVENTF_LEFTUP, 0, 0, 0, UIntPtr.Zero);
            }
        }

        private static void WaitForWindowDpi(IntPtr hwnd, uint expectedDpi)
        {
            var timeout = Stopwatch.StartNew();
            uint actualDpi;
            do
            {
                actualDpi = NativeMethods.GetDpiForWindow(hwnd);
                if (actualDpi == expectedDpi)
                {
                    return;
                }

                Thread.Sleep(200);
            }
            while (timeout.Elapsed < TimeSpan.FromSeconds(15));

            Verify.AreEqual(expectedDpi, actualDpi, "The managed test window did not update to the destination monitor DPI.");
        }

        private static void WaitForRasterizationScale(FrameworkElement content, double expectedScale)
        {
            var timeout = Stopwatch.StartNew();
            double actualScale = 0;
            do
            {
                TestServices.WindowHelper.WaitForIdle();
                UIExecutor.Execute(() =>
                {
                    Verify.IsNotNull(content.XamlRoot, "The managed test content must have a XamlRoot.");
                    actualScale = content.XamlRoot.RasterizationScale;
                });

                if (Math.Abs(actualScale - expectedScale) < 0.001)
                {
                    return;
                }

                Thread.Sleep(200);
            }
            while (timeout.Elapsed < TimeSpan.FromSeconds(15));

            Verify.IsTrue(
                Math.Abs(actualScale - expectedScale) < 0.001,
                "XamlRoot.RasterizationScale did not update to the destination monitor DPI.");
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct WindowRect
        {
            public int Left;
            public int Top;
            public int Right;
            public int Bottom;

            public int Width => Right - Left;
            public int Height => Bottom - Top;
        }

        private static class NativeMethods
        {
            public const uint MONITOR_DEFAULTTONEAREST = 2;
            public const uint SWP_NOZORDER = 0x0004;
            public const uint SWP_NOACTIVATE = 0x0010;
            public const uint SWP_NOOWNERZORDER = 0x0200;
            public const uint MOUSEEVENTF_LEFTDOWN = 0x0002;
            public const uint MOUSEEVENTF_LEFTUP = 0x0004;

            [DllImport("user32.dll")]
            public static extern IntPtr GetForegroundWindow();

            [DllImport("user32.dll", SetLastError = true)]
            [return: MarshalAs(UnmanagedType.Bool)]
            public static extern bool GetWindowRect(IntPtr hwnd, out WindowRect rect);

            [DllImport("user32.dll", SetLastError = true)]
            [return: MarshalAs(UnmanagedType.Bool)]
            public static extern bool SetWindowPos(
                IntPtr hwnd,
                IntPtr hwndInsertAfter,
                int x,
                int y,
                int width,
                int height,
                uint flags);

            [DllImport("user32.dll")]
            public static extern uint GetDpiForWindow(IntPtr hwnd);

            [DllImport("user32.dll")]
            public static extern IntPtr MonitorFromWindow(IntPtr hwnd, uint flags);

            [DllImport("user32.dll", SetLastError = true)]
            [return: MarshalAs(UnmanagedType.Bool)]
            public static extern bool SetCursorPos(int x, int y);

            [DllImport("user32.dll")]
            public static extern void mouse_event(
                uint flags,
                uint dx,
                uint dy,
                uint data,
                UIntPtr extraInfo);
        }
    }

    internal sealed class VirtualDisplayTestSession : IDisposable
    {
        private const uint MonitorId = 0;
        private const uint ExpectedProtocolVersion = 1;
        private const int MaxMonitors = 4;
        private const uint GetStateIoctl = 0x8000e000;
        private const uint AddMonitorIoctl = 0x8000e004;
        private const uint ClearMonitorsIoctl = 0x8000e00c;
        private const int MonitorResponseSize = 32;
        private const int StateResponseSize = 152;
        private const int QualifiedDpiBuild = 26660;
        private const uint IndirectWiredOutputTechnology = 16;
        private static readonly Guid InterfaceId = new Guid("064CBEC3-A0A1-4765-BCF5-9B9F7EC02ACA");
        private static readonly uint[] DpiPercentages = { 100, 125, 150, 175, 200, 225, 250, 300, 350, 400, 450, 500 };

        private readonly SafeFileHandle _driver;
        private readonly DriverMonitor _monitor;
        private readonly uint _originalScale;
        private bool _scaleChanged;
        private bool _disposed;

        private VirtualDisplayTestSession(
            SafeFileHandle driver,
            DriverMonitor monitor,
            DisplayTarget display,
            uint originalScale,
            bool scaleChanged)
        {
            _driver = driver;
            _monitor = monitor;
            Display = display;
            _originalScale = originalScale;
            _scaleChanged = scaleChanged;
        }

        public DisplayTarget Display { get; private set; }

        public static VirtualDisplayTestSession Create(uint width, uint height, uint scalePercent)
        {
            if (Environment.OSVersion.Version.Build != QualifiedDpiBuild)
            {
                throw new PlatformNotSupportedException(
                    $"The mixed-DPI monitor test is currently qualified only on Windows build {QualifiedDpiBuild}.");
            }

            var driver = OpenDriver();
            DriverMonitor monitor = null;
            var monitorAdded = false;
            DisplayTarget display = null;
            uint originalScale = 0;
            var originalScaleKnown = false;
            var scaleChangeRequested = false;
            try
            {
                WaitForDriverReady(driver);
                if (ReadState(driver).Monitors.Count != 0)
                {
                    throw new InvalidOperationException(
                        "The virtual display driver already owns monitors. Recover or reset the VM before running this test.");
                }

                monitor = AddMonitor(driver, MonitorId, width, height);
                monitorAdded = true;
                display = WaitForDisplay(monitor, null);
                originalScale = QueryScalePercent(display.Source);
                originalScaleKnown = true;
                var session = new VirtualDisplayTestSession(driver, monitor, display, originalScale, false);
                scaleChangeRequested = originalScale != scalePercent;
                SetScale(display.Source, scalePercent);
                display = WaitForDisplay(monitor, scalePercent);
                session.Display = display;
                session._scaleChanged = scaleChangeRequested;

                Log.Comment(
                    "Created virtual display {0}x{1} at {2}% ({3} DPI), bounds ({4},{5})-({6},{7}).",
                    width,
                    height,
                    scalePercent,
                    display.EffectiveDpi,
                    display.Bounds.Left,
                    display.Bounds.Top,
                    display.Bounds.Right,
                    display.Bounds.Bottom);

                return session;
            }
            catch (Exception createError)
            {
                var failures = new List<Exception> { createError };
                if (scaleChangeRequested && originalScaleKnown)
                {
                    try
                    {
                        var currentDisplay = display ?? WaitForDisplay(monitor, null);
                        SetScale(currentDisplay.Source, originalScale);
                        WaitForDisplay(monitor, originalScale);
                    }
                    catch (Exception cleanupError) when (IsOperational(cleanupError))
                    {
                        failures.Add(new InvalidOperationException("Failed to restore the virtual display DPI after setup failed.", cleanupError));
                    }
                }

                if (monitorAdded)
                {
                    try
                    {
                        Send(driver, ClearMonitorsIoctl, CreateRequest(8), 0);
                        if (ReadState(driver).Monitors.Count != 0)
                        {
                            throw new InvalidDataException("The driver acknowledged setup cleanup but still reports virtual monitors.");
                        }
                    }
                    catch (Exception cleanupError) when (IsOperational(cleanupError))
                    {
                        failures.Add(new InvalidOperationException("Failed to remove the virtual display after setup failed.", cleanupError));
                    }
                }

                driver.Dispose();
                if (failures.Count == 1)
                {
                    throw;
                }

                throw new AggregateException(
                    "Virtual display setup and cleanup failed. Reset the VM before running more display tests.",
                    failures);
            }
        }

        public static DisplayTarget GetPrimaryDisplay()
        {
            var primary = EnumerateDesktopMonitors().SingleOrDefault(pair => (pair.Value.Info.Flags & 1) != 0);
            if (primary.Value.Info.Size == 0)
            {
                throw new InvalidOperationException("The interactive desktop does not have exactly one primary display.");
            }

            return new DisplayTarget(primary.Value.Handle, primary.Value.Info.Bounds, default, MeasureDpi(primary.Value.Handle, primary.Value.Info.Bounds));
        }

        public void Dispose()
        {
            if (_disposed)
            {
                return;
            }

            _disposed = true;
            var failures = new List<Exception>();
            if (_scaleChanged)
            {
                try
                {
                    var display = WaitForDisplay(_monitor, null);
                    SetScale(display.Source, _originalScale);
                    WaitForDisplay(_monitor, _originalScale);
                }
                catch (Exception error) when (IsOperational(error))
                {
                    failures.Add(new InvalidOperationException("Failed to restore the virtual display DPI.", error));
                }
            }

            try
            {
                Send(_driver, ClearMonitorsIoctl, CreateRequest(8), 0);
                if (ReadState(_driver).Monitors.Count != 0)
                {
                    throw new InvalidDataException("The driver acknowledged cleanup but still reports virtual monitors.");
                }
            }
            catch (Exception error) when (IsOperational(error))
            {
                failures.Add(new InvalidOperationException("Failed to remove the virtual display.", error));
            }
            finally
            {
                _driver.Dispose();
            }

            if (failures.Count != 0)
            {
                throw new AggregateException("Virtual display cleanup failed. Reset the VM before running more display tests.", failures);
            }
        }

        private static SafeFileHandle OpenDriver()
        {
            var paths = FindDriverPaths();
            if (paths.Count != 1)
            {
                throw new InvalidOperationException(
                    paths.Count == 0
                        ? "The MonitorTopologyTest driver is not installed on this VM."
                        : "Multiple MonitorTopologyTest adapters are installed. Reset the VM to one adapter.");
            }

            var driver = Native.CreateFile(paths[0], 0xc0000000, 0, IntPtr.Zero, 3, 0x80, IntPtr.Zero);
            if (driver.IsInvalid)
            {
                var error = Native.Error("Open MonitorTopologyTest driver");
                driver.Dispose();
                throw error;
            }

            return driver;
        }

        private static void WaitForDriverReady(SafeFileHandle driver)
        {
            var timeout = Stopwatch.StartNew();
            do
            {
                if (ReadState(driver).Ready)
                {
                    return;
                }

                Thread.Sleep(200);
            }
            while (timeout.Elapsed < TimeSpan.FromSeconds(20));

            throw new TimeoutException("The virtual display adapter did not become ready.");
        }

        private static DriverState ReadState(SafeFileHandle driver)
        {
            var response = Send(driver, GetStateIoctl, CreateRequest(8), StateResponseSize);
            if (ReadUInt32(response, 0) != StateResponseSize ||
                ReadUInt32(response, 4) != ExpectedProtocolVersion ||
                ReadUInt32(response, 8) > 1 ||
                ReadUInt32(response, 12) != MaxMonitors ||
                ReadUInt32(response, 16) > MaxMonitors)
            {
                throw new InvalidDataException("The virtual display driver returned an incompatible state response.");
            }

            var monitorCount = checked((int)ReadUInt32(response, 16));
            var monitors = new List<DriverMonitor>();
            for (var index = 0; index < monitorCount; index++)
            {
                monitors.Add(ReadMonitor(response.AsSpan(24 + index * MonitorResponseSize, MonitorResponseSize)));
            }

            return new DriverState(ReadUInt32(response, 8) == 1, monitors);
        }

        private static DriverMonitor AddMonitor(SafeFileHandle driver, uint id, uint width, uint height)
        {
            var request = CreateRequest(28);
            WriteUInt32(request, 8, id);
            WriteUInt32(request, 12, width);
            WriteUInt32(request, 16, height);
            WriteUInt32(request, 20, 60);
            WriteUInt32(request, 24, 1);
            return ReadMonitor(Send(driver, AddMonitorIoctl, request, MonitorResponseSize));
        }

        private static DriverMonitor ReadMonitor(ReadOnlySpan<byte> response)
        {
            return new DriverMonitor(
                ReadUInt32(response, 0),
                ReadUInt32(response, 4),
                ReadUInt32(response, 8),
                new Luid(ReadUInt32(response, 20), ReadInt32(response, 24)),
                ReadUInt32(response, 28));
        }

        private static DisplayTarget WaitForDisplay(DriverMonitor monitor, uint? expectedScale)
        {
            var timeout = Stopwatch.StartNew();
            DisplayTarget previous = null;
            var stableReadings = 0;

            do
            {
                var display = TryResolveDisplay(monitor);
                if (display != null)
                {
                    var scaleMatches = !expectedScale.HasValue ||
                        QueryScalePercent(display.Source) == expectedScale.Value;
                    var dpiMatches = !expectedScale.HasValue ||
                        display.EffectiveDpi == expectedScale.Value * 96 / 100;
                    stableReadings = scaleMatches && dpiMatches && display.Equals(previous)
                        ? stableReadings + 1
                        : scaleMatches && dpiMatches ? 1 : 0;
                    if (stableReadings >= 3)
                    {
                        return display;
                    }

                    previous = display;
                }
                else
                {
                    stableReadings = 0;
                    previous = null;
                }

                Thread.Sleep(200);
            }
            while (timeout.Elapsed < TimeSpan.FromSeconds(20));

            throw new TimeoutException(
                expectedScale.HasValue
                    ? $"The virtual display did not converge to {expectedScale.Value}%."
                    : "The virtual display did not become active on the interactive desktop.");
        }

        private static DisplayTarget TryResolveDisplay(DriverMonitor monitor)
        {
            var matches = GetActivePaths()
                .Where(path => path.Target.Adapter.Equals(monitor.Adapter) && path.Target.Id == monitor.TargetId)
                .ToArray();
            if (matches.Length == 0)
            {
                return null;
            }

            if (matches.Length != 1 || matches[0].Target.OutputTechnology != IndirectWiredOutputTechnology)
            {
                throw new NotSupportedException("The virtual monitor does not have one indirect-wired active path.");
            }

            var path = matches[0];
            var sourceName = new SourceDeviceName
            {
                Header = DeviceInfoHeader.Create<SourceDeviceName>(1, path.Source.Adapter, path.Source.Id)
            };
            Native.Check(Native.DisplayConfigGetSourceDeviceName(ref sourceName), "Read virtual display source name");

            var monitors = EnumerateDesktopMonitors();
            if (!monitors.TryGetValue(sourceName.ViewGdiDeviceName, out var desktopMonitor))
            {
                return null;
            }

            return new DisplayTarget(
                desktopMonitor.Handle,
                desktopMonitor.Info.Bounds,
                new DisplaySource(path.Source.Adapter, path.Source.Id),
                MeasureDpi(desktopMonitor.Handle, desktopMonitor.Info.Bounds));
        }

        private static uint QueryScalePercent(DisplaySource source)
        {
            var query = new DpiQuery
            {
                Header = DeviceInfoHeader.Create<DpiQuery>(unchecked((uint)-3), source.Adapter, source.Id)
            };
            Native.Check(Native.DisplayConfigGetDpi(ref query), "Read virtual display DPI");
            return GetScalePercent(query.MinimumRelative, query.CurrentRelative, query.MaximumRelative);
        }

        private static void SetScale(DisplaySource source, uint scalePercent)
        {
            var query = new DpiQuery
            {
                Header = DeviceInfoHeader.Create<DpiQuery>(unchecked((uint)-3), source.Adapter, source.Id)
            };
            Native.Check(Native.DisplayConfigGetDpi(ref query), "Read virtual display DPI range");

            var requestedIndex = Array.IndexOf(DpiPercentages, scalePercent);
            if (requestedIndex < 0)
            {
                throw new ArgumentOutOfRangeException(nameof(scalePercent));
            }

            ValidateDpiRange(query.MinimumRelative, query.CurrentRelative, query.MaximumRelative);
            var relativeScale = checked(requestedIndex + query.MinimumRelative);
            if (relativeScale < query.MinimumRelative || relativeScale > query.MaximumRelative)
            {
                throw new NotSupportedException($"The virtual display does not support {scalePercent}% scale.");
            }

            if (GetScalePercent(query.MinimumRelative, query.CurrentRelative, query.MaximumRelative) == scalePercent)
            {
                return;
            }

            var request = new DpiSet
            {
                Header = DeviceInfoHeader.Create<DpiSet>(unchecked((uint)-4), source.Adapter, source.Id),
                RelativeScale = relativeScale
            };
            Native.Check(Native.DisplayConfigSetDpi(ref request), "Set virtual display DPI");
        }

        private static uint GetScalePercent(int minimum, int current, int maximum)
        {
            ValidateDpiRange(minimum, current, maximum);
            return DpiPercentages[checked(current - minimum)];
        }

        private static void ValidateDpiRange(int minimum, int current, int maximum)
        {
            var recommendedIndex = -(long)minimum;
            var maximumIndex = (long)maximum - minimum;
            if (minimum > 0 ||
                maximum < 0 ||
                current < minimum ||
                current > maximum ||
                recommendedIndex >= DpiPercentages.Length ||
                maximumIndex >= DpiPercentages.Length)
            {
                throw new NotSupportedException($"Unsupported DPI scale range ({minimum}, {current}, {maximum}).");
            }
        }

        private static DisplayPath[] GetActivePaths()
        {
            const uint QueryOnlyActivePaths = 2;
            for (var attempt = 0; attempt < 5; attempt++)
            {
                Native.Check(
                    Native.GetDisplayConfigBufferSizes(QueryOnlyActivePaths, out var pathCount, out var modeCount),
                    "Get active display buffer sizes");
                var paths = new DisplayPath[Math.Max(pathCount, 1u)];
                var modes = new DisplayMode[Math.Max(modeCount, 1u)];
                var error = Native.QueryDisplayConfig(
                    QueryOnlyActivePaths,
                    ref pathCount,
                    paths,
                    ref modeCount,
                    modes,
                    IntPtr.Zero);
                if (error == 122)
                {
                    continue;
                }

                Native.Check(error, "Query active displays");
                return paths.Take(checked((int)pathCount)).ToArray();
            }

            throw new InvalidOperationException("The display topology kept changing during enumeration.");
        }

        private static Dictionary<string, DesktopMonitor> EnumerateDesktopMonitors()
        {
            var handles = new List<IntPtr>();
            Native.MonitorEnum callback = (IntPtr monitor, IntPtr deviceContext, ref NativeRect bounds, IntPtr data) =>
            {
                handles.Add(monitor);
                return true;
            };
            if (!Native.EnumDisplayMonitors(IntPtr.Zero, IntPtr.Zero, callback, IntPtr.Zero))
            {
                throw Native.Error("Enumerate desktop monitors");
            }

            var monitors = new Dictionary<string, DesktopMonitor>(StringComparer.OrdinalIgnoreCase);
            foreach (var handle in handles)
            {
                var info = new MonitorInfo { Size = (uint)Marshal.SizeOf<MonitorInfo>() };
                if (!Native.GetMonitorInfo(handle, ref info))
                {
                    throw Native.Error("Read desktop monitor bounds");
                }

                monitors.Add(info.Device, new DesktopMonitor(handle, info));
            }

            return monitors;
        }

        private static uint MeasureDpi(IntPtr monitor, NativeRect bounds)
        {
            var previousContext = Native.SetThreadDpiAwarenessContext(new IntPtr(-4));
            if (previousContext == IntPtr.Zero)
            {
                throw Native.Error("Set per-monitor DPI awareness");
            }

            try
            {
                var x = checked((int)(((long)bounds.Left + bounds.Right) / 2));
                var y = checked((int)(((long)bounds.Top + bounds.Bottom) / 2));
                var window = Native.CreateWindowEx(
                    0x08000080,
                    "STATIC",
                    string.Empty,
                    0x80000000,
                    x,
                    y,
                    1,
                    1,
                    IntPtr.Zero,
                    IntPtr.Zero,
                    IntPtr.Zero,
                    IntPtr.Zero);
                if (window == IntPtr.Zero)
                {
                    throw Native.Error("Create DPI probe window");
                }

                try
                {
                    if (Native.MonitorFromWindow(window, 0) != monitor)
                    {
                        throw new InvalidOperationException("The DPI probe window landed on a different display.");
                    }

                    var dpi = Native.GetDpiForWindow(window);
                    if (dpi == 0)
                    {
                        throw new InvalidOperationException("GetDpiForWindow returned zero for the DPI probe.");
                    }

                    return dpi;
                }
                finally
                {
                    if (!Native.DestroyWindow(window))
                    {
                        throw Native.Error("Destroy DPI probe window");
                    }
                }
            }
            finally
            {
                if (Native.SetThreadDpiAwarenessContext(previousContext) == IntPtr.Zero)
                {
                    throw Native.Error("Restore DPI awareness");
                }
            }
        }

        private static List<string> FindDriverPaths()
        {
            var interfaceId = InterfaceId;
            using (var devices = Native.SetupDiGetClassDevs(ref interfaceId, null, IntPtr.Zero, 0x12))
            {
                if (devices.IsInvalid)
                {
                    throw Native.Error("Enumerate MonitorTopologyTest interfaces");
                }

                var paths = new List<string>();
                for (uint index = 0; ; index++)
                {
                    var data = new DeviceInterfaceData { Size = (uint)Marshal.SizeOf<DeviceInterfaceData>() };
                    if (!Native.SetupDiEnumDeviceInterfaces(devices, IntPtr.Zero, ref interfaceId, index, ref data))
                    {
                        var error = Marshal.GetLastWin32Error();
                        if (error == 259)
                        {
                            break;
                        }

                        throw new Win32Exception(error, "Enumerate MonitorTopologyTest interfaces");
                    }

                    Native.SetupDiGetDeviceInterfaceDetail(devices, ref data, null, 0, out var required, IntPtr.Zero);
                    var sizeError = Marshal.GetLastWin32Error();
                    if (sizeError != 122)
                    {
                        throw new Win32Exception(sizeError, "Read MonitorTopologyTest interface path size");
                    }

                    var buffer = new byte[required];
                    WriteUInt32(buffer, 0, IntPtr.Size == 8 ? 8u : 6u);
                    if (!Native.SetupDiGetDeviceInterfaceDetail(devices, ref data, buffer, required, out _, IntPtr.Zero))
                    {
                        throw Native.Error("Read MonitorTopologyTest interface path");
                    }

                    var path = Encoding.Unicode.GetString(buffer, 4, buffer.Length - 4);
                    var terminator = path.IndexOf('\0');
                    if (terminator <= 0)
                    {
                        throw new InvalidDataException("Windows returned an invalid MonitorTopologyTest interface path.");
                    }

                    paths.Add(path.Substring(0, terminator));
                }

                return paths;
            }
        }

        private static byte[] Send(SafeFileHandle driver, uint ioctl, byte[] request, int responseSize)
        {
            var response = new byte[responseSize];
            if (!Native.DeviceIoControl(
                driver,
                ioctl,
                request,
                (uint)request.Length,
                response,
                (uint)response.Length,
                out var returned,
                IntPtr.Zero))
            {
                throw Native.Error($"MonitorTopologyTest IOCTL 0x{ioctl:X8}");
            }

            if (returned != responseSize)
            {
                throw new InvalidDataException($"Expected {responseSize} driver response bytes, received {returned}.");
            }

            return response;
        }

        private static byte[] CreateRequest(int size)
        {
            var request = new byte[size];
            WriteUInt32(request, 0, (uint)size);
            WriteUInt32(request, 4, ExpectedProtocolVersion);
            return request;
        }

        private static uint ReadUInt32(ReadOnlySpan<byte> bytes, int offset)
        {
            return BinaryPrimitives.ReadUInt32LittleEndian(bytes.Slice(offset));
        }

        private static int ReadInt32(ReadOnlySpan<byte> bytes, int offset)
        {
            return BinaryPrimitives.ReadInt32LittleEndian(bytes.Slice(offset));
        }

        private static void WriteUInt32(Span<byte> bytes, int offset, uint value)
        {
            BinaryPrimitives.WriteUInt32LittleEndian(bytes.Slice(offset), value);
        }

        private static bool IsOperational(Exception error)
        {
            return error is IOException ||
                error is UnauthorizedAccessException ||
                error is Win32Exception ||
                error is InvalidOperationException ||
                error is NotSupportedException ||
                error is TimeoutException ||
                error is ArgumentException;
        }

        internal sealed class DisplayTarget
        {
            internal DisplayTarget(IntPtr handle, NativeRect bounds, DisplaySource source, uint effectiveDpi)
            {
                Handle = handle;
                Bounds = bounds;
                Source = source;
                EffectiveDpi = effectiveDpi;
            }

            public IntPtr Handle { get; }
            public NativeRect Bounds { get; }
            internal DisplaySource Source { get; }
            public uint EffectiveDpi { get; }

            public override bool Equals(object obj)
            {
                var other = obj as DisplayTarget;
                return other != null &&
                    Handle == other.Handle &&
                    Bounds.Equals(other.Bounds) &&
                    Source.Equals(other.Source) &&
                    EffectiveDpi == other.EffectiveDpi;
            }

            public override int GetHashCode()
            {
                return HashCode.Combine(Handle, Bounds, Source, EffectiveDpi);
            }
        }

        private sealed class DriverState
        {
            public DriverState(bool ready, IReadOnlyList<DriverMonitor> monitors)
            {
                Ready = ready;
                Monitors = monitors;
            }

            public bool Ready { get; }
            public IReadOnlyList<DriverMonitor> Monitors { get; }
        }

        private sealed class DriverMonitor
        {
            public DriverMonitor(uint id, uint width, uint height, Luid adapter, uint targetId)
            {
                Id = id;
                Width = width;
                Height = height;
                Adapter = adapter;
                TargetId = targetId;
            }

            public uint Id { get; }
            public uint Width { get; }
            public uint Height { get; }
            public Luid Adapter { get; }
            public uint TargetId { get; }
        }

        internal readonly struct DisplaySource : IEquatable<DisplaySource>
        {
            public DisplaySource(Luid adapter, uint id)
            {
                Adapter = adapter;
                Id = id;
            }

            public Luid Adapter { get; }
            public uint Id { get; }

            public bool Equals(DisplaySource other)
            {
                return Adapter.Equals(other.Adapter) && Id == other.Id;
            }

            public override bool Equals(object obj)
            {
                return obj is DisplaySource other && Equals(other);
            }

            public override int GetHashCode()
            {
                return HashCode.Combine(Adapter, Id);
            }
        }

        [StructLayout(LayoutKind.Sequential)]
        internal struct NativeRect : IEquatable<NativeRect>
        {
            public int Left;
            public int Top;
            public int Right;
            public int Bottom;

            public int Width => Right - Left;
            public int Height => Bottom - Top;

            public bool Equals(NativeRect other)
            {
                return Left == other.Left && Top == other.Top && Right == other.Right && Bottom == other.Bottom;
            }

            public override bool Equals(object obj)
            {
                return obj is NativeRect other && Equals(other);
            }

            public override int GetHashCode()
            {
                return HashCode.Combine(Left, Top, Right, Bottom);
            }
        }

        [StructLayout(LayoutKind.Sequential)]
        internal struct Luid : IEquatable<Luid>
        {
            public Luid(uint low, int high)
            {
                Low = low;
                High = high;
            }

            public uint Low;
            public int High;

            public bool Equals(Luid other)
            {
                return Low == other.Low && High == other.High;
            }

            public override bool Equals(object obj)
            {
                return obj is Luid other && Equals(other);
            }

            public override int GetHashCode()
            {
                return HashCode.Combine(Low, High);
            }
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct Rational
        {
            public uint Numerator;
            public uint Denominator;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct DisplayPathSource
        {
            public Luid Adapter;
            public uint Id;
            public uint ModeIndex;
            public uint StatusFlags;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct DisplayPathTarget
        {
            public Luid Adapter;
            public uint Id;
            public uint ModeIndex;
            public uint OutputTechnology;
            public uint Rotation;
            public uint Scaling;
            public Rational RefreshRate;
            public uint ScanLineOrdering;
            public int TargetAvailable;
            public uint StatusFlags;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct DisplayPath
        {
            public DisplayPathSource Source;
            public DisplayPathTarget Target;
            public uint Flags;
        }

        [StructLayout(LayoutKind.Explicit, Size = 64)]
        private struct DisplayMode
        {
            [FieldOffset(0)]
            public uint InfoType;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct DeviceInfoHeader
        {
            public uint Type;
            public uint Size;
            public Luid Adapter;
            public uint Id;

            public static DeviceInfoHeader Create<T>(uint type, Luid adapter, uint id)
            {
                return new DeviceInfoHeader
                {
                    Type = type,
                    Size = (uint)Marshal.SizeOf<T>(),
                    Adapter = adapter,
                    Id = id
                };
            }
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        private struct SourceDeviceName
        {
            public DeviceInfoHeader Header;

            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 32)]
            public string ViewGdiDeviceName;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct DpiQuery
        {
            public DeviceInfoHeader Header;
            public int MinimumRelative;
            public int CurrentRelative;
            public int MaximumRelative;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct DpiSet
        {
            public DeviceInfoHeader Header;
            public int RelativeScale;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        private struct MonitorInfo
        {
            public uint Size;
            public NativeRect Bounds;
            public NativeRect WorkArea;
            public uint Flags;

            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 32)]
            public string Device;
        }

        private readonly struct DesktopMonitor
        {
            public DesktopMonitor(IntPtr handle, MonitorInfo info)
            {
                Handle = handle;
                Info = info;
            }

            public IntPtr Handle { get; }
            public MonitorInfo Info { get; }
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct DeviceInterfaceData
        {
            public uint Size;
            public Guid InterfaceClassGuid;
            public uint Flags;
            public UIntPtr Reserved;
        }

        private sealed class DeviceInfoHandle : SafeHandleZeroOrMinusOneIsInvalid
        {
            public DeviceInfoHandle()
                : base(true)
            {
            }

            protected override bool ReleaseHandle()
            {
                return Native.SetupDiDestroyDeviceInfoList(handle);
            }
        }

        private static class Native
        {
            internal delegate bool MonitorEnum(IntPtr monitor, IntPtr deviceContext, ref NativeRect bounds, IntPtr data);

            internal static Win32Exception Error(string operation)
            {
                var code = Marshal.GetLastWin32Error();
                return new Win32Exception(code, $"{operation}: {new Win32Exception(code).Message}");
            }

            internal static void Check(int error, string operation)
            {
                if (error != 0)
                {
                    throw new Win32Exception(error, $"{operation}: {new Win32Exception(error).Message}");
                }
            }

            [DllImport("setupapi.dll", EntryPoint = "SetupDiGetClassDevsW", CharSet = CharSet.Unicode, SetLastError = true)]
            internal static extern DeviceInfoHandle SetupDiGetClassDevs(
                ref Guid classGuid,
                string enumerator,
                IntPtr parent,
                uint flags);

            [DllImport("setupapi.dll", SetLastError = true)]
            [return: MarshalAs(UnmanagedType.Bool)]
            internal static extern bool SetupDiEnumDeviceInterfaces(
                DeviceInfoHandle deviceInfoSet,
                IntPtr deviceInfoData,
                ref Guid interfaceClassGuid,
                uint memberIndex,
                ref DeviceInterfaceData deviceInterfaceData);

            [DllImport("setupapi.dll", EntryPoint = "SetupDiGetDeviceInterfaceDetailW", SetLastError = true)]
            [return: MarshalAs(UnmanagedType.Bool)]
            internal static extern bool SetupDiGetDeviceInterfaceDetail(
                DeviceInfoHandle deviceInfoSet,
                ref DeviceInterfaceData deviceInterfaceData,
                byte[] deviceInterfaceDetailData,
                uint deviceInterfaceDetailDataSize,
                out uint requiredSize,
                IntPtr deviceInfoData);

            [DllImport("setupapi.dll", SetLastError = true)]
            [return: MarshalAs(UnmanagedType.Bool)]
            internal static extern bool SetupDiDestroyDeviceInfoList(IntPtr deviceInfoSet);

            [DllImport("kernel32.dll", EntryPoint = "CreateFileW", CharSet = CharSet.Unicode, SetLastError = true)]
            internal static extern SafeFileHandle CreateFile(
                string fileName,
                uint desiredAccess,
                uint shareMode,
                IntPtr securityAttributes,
                uint creationDisposition,
                uint flagsAndAttributes,
                IntPtr templateFile);

            [DllImport("kernel32.dll", SetLastError = true)]
            [return: MarshalAs(UnmanagedType.Bool)]
            internal static extern bool DeviceIoControl(
                SafeFileHandle device,
                uint controlCode,
                byte[] input,
                uint inputSize,
                byte[] output,
                uint outputSize,
                out uint bytesReturned,
                IntPtr overlapped);

            [DllImport("user32.dll")]
            internal static extern int GetDisplayConfigBufferSizes(
                uint flags,
                out uint pathCount,
                out uint modeCount);

            [DllImport("user32.dll")]
            internal static extern int QueryDisplayConfig(
                uint flags,
                ref uint pathCount,
                [Out] DisplayPath[] paths,
                ref uint modeCount,
                [Out] DisplayMode[] modes,
                IntPtr currentTopologyId);

            [DllImport("user32.dll", EntryPoint = "DisplayConfigGetDeviceInfo")]
            internal static extern int DisplayConfigGetSourceDeviceName(ref SourceDeviceName request);

            [DllImport("user32.dll", EntryPoint = "DisplayConfigGetDeviceInfo")]
            internal static extern int DisplayConfigGetDpi(ref DpiQuery request);

            [DllImport("user32.dll", EntryPoint = "DisplayConfigSetDeviceInfo")]
            internal static extern int DisplayConfigSetDpi(ref DpiSet request);

            [DllImport("user32.dll", SetLastError = true)]
            [return: MarshalAs(UnmanagedType.Bool)]
            internal static extern bool EnumDisplayMonitors(
                IntPtr deviceContext,
                IntPtr clipRect,
                MonitorEnum callback,
                IntPtr data);

            [DllImport("user32.dll", EntryPoint = "GetMonitorInfoW", CharSet = CharSet.Unicode, SetLastError = true)]
            [return: MarshalAs(UnmanagedType.Bool)]
            internal static extern bool GetMonitorInfo(IntPtr monitor, ref MonitorInfo info);

            [DllImport("user32.dll", SetLastError = true)]
            internal static extern IntPtr SetThreadDpiAwarenessContext(IntPtr dpiContext);

            [DllImport("user32.dll", EntryPoint = "CreateWindowExW", CharSet = CharSet.Unicode, SetLastError = true)]
            internal static extern IntPtr CreateWindowEx(
                uint extendedStyle,
                string className,
                string windowName,
                uint style,
                int x,
                int y,
                int width,
                int height,
                IntPtr parent,
                IntPtr menu,
                IntPtr instance,
                IntPtr parameter);

            [DllImport("user32.dll", SetLastError = true)]
            [return: MarshalAs(UnmanagedType.Bool)]
            internal static extern bool DestroyWindow(IntPtr window);

            [DllImport("user32.dll")]
            internal static extern IntPtr MonitorFromWindow(IntPtr window, uint flags);

            [DllImport("user32.dll")]
            internal static extern uint GetDpiForWindow(IntPtr window);
        }
    }
}
