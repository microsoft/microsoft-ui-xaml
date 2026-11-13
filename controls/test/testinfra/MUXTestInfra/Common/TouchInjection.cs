// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;

using WEX.Logging.Interop;

using MitaLog = Microsoft.Windows.Apps.Test.Foundation.Utilities.Log;

namespace Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Common
{
    // Runs a MITA touch gesture, and retries it when Windows rejects one of its frames with ERROR_INVALID_PARAMETER
    // (bug 65248069). MITA schedules each frame at a fixed offset from the start of the gesture, so when the test
    // process falls about 100 ms behind (for example while the machine is busy starting processes), MITA injects all
    // the overdue frames back-to-back, and Windows rejects one partway through that burst. Every failed attempt is
    // logged with the frames MITA injected and some machine state.
    internal static class TouchInjection
    {
        private const int ErrorInvalidParameter = 87;
        private const int MaxAttempts = 3;
        private const int SM_CXSCREEN = 0;
        private const int SM_CYSCREEN = 1;

        public static void Run(string gesture, Action inject)
        {
            for (int attempt = 1; ; attempt++)
            {
                try
                {
                    RunAttempt(gesture, inject, attempt);
                    return;
                }
                catch (Win32Exception e) when (e.NativeErrorCode == ErrorInvalidParameter && attempt < MaxAttempts)
                {
                    Log.Warning($"Windows rejected a touch frame during {gesture}, so the gesture is being retried (attempt {attempt + 1} of {MaxAttempts}). See bug 65248069.");
                    Thread.Sleep(200);
                }
            }
        }

        private static void RunAttempt(string gesture, Action inject, int attempt)
        {
            var frames = new List<string>();
            var clock = Stopwatch.StartNew();
            DateTime startTime = DateTime.Now;
            string screenAtStart = DescribeScreen();
            IntPtr foregroundAtStart = GetForegroundWindow();

            Action<string, object[]> previousOut = MitaLog.OutImplementation;
            MitaLog.OutImplementation = (format, args) =>
            {
                frames.Add($"+{clock.Elapsed.TotalMilliseconds:F1} ms: {Format(format, args)}");
                previousOut?.Invoke(format, args);
            };

            try
            {
                inject();
            }
            catch (Win32Exception e)
            {
                LogFailure(gesture, attempt, e, clock.Elapsed, frames, startTime, screenAtStart, foregroundAtStart);
                throw;
            }
            finally
            {
                MitaLog.OutImplementation = previousOut;
            }
        }

        private static void LogFailure(string gesture, int attempt, Win32Exception e, TimeSpan elapsed, List<string> frames,
            DateTime startTime, string screenAtStart, IntPtr foregroundAtStart)
        {
            try
            {
                var report = new StringBuilder();
                report.AppendLine($"Touch injection failed {elapsed.TotalMilliseconds:F0} ms into {gesture} (attempt {attempt} of {MaxAttempts}): Win32 error {e.NativeErrorCode} ({e.Message}). Diagnostics for bug 65248069:");
                report.AppendLine("Frames MITA injected, timed from the start of the gesture:");
                foreach (string frame in frames)
                {
                    report.AppendLine("  " + frame);
                }
                report.AppendLine($"Screen: {screenAtStart} at the start, {DescribeScreen()} at the failure.");
                report.AppendLine($"Foreground window: {DescribeWindow(foregroundAtStart)} at the start, {DescribeWindow(GetForegroundWindow())} at the failure.");
                report.AppendLine("Processes started from 5 s before the gesture until the failure, timed from the start of the gesture:");
                foreach (var started in GetProcessesStartedSince(startTime.AddSeconds(-5)))
                {
                    report.AppendLine($"  {(started.Start - startTime).TotalMilliseconds:+0;-0} ms: {started.Name} ({started.Id})");
                }
                var textInputHosts = GetProcessesStartedSince(DateTime.MinValue).Where(p => p.Name == "TextInputHost").ToList();
                report.AppendLine(textInputHosts.Count == 0
                    ? "TextInputHost isn't running."
                    : "TextInputHost started " + string.Join(", ", textInputHosts.Select(p => $"{(p.Start - startTime).TotalMilliseconds:+0;-0} ms from the start of the gesture ({p.Id})")) + ".");
                Log.Comment(report.ToString());
            }
            catch (Exception diagnosticsException)
            {
                Log.Comment($"Couldn't collect touch injection diagnostics: {diagnosticsException.Message}");
            }
        }

        private static List<(string Name, int Id, DateTime Start)> GetProcessesStartedSince(DateTime since)
        {
            var started = new List<(string Name, int Id, DateTime Start)>();
            foreach (Process process in Process.GetProcesses())
            {
                using (process)
                {
                    try
                    {
                        if (process.StartTime >= since)
                        {
                            started.Add((process.ProcessName, process.Id, process.StartTime));
                        }
                    }
                    catch (Exception)
                    {
                        // Protected and exiting processes don't expose a start time.
                    }
                }
            }
            return started.OrderBy(p => p.Start).ToList();
        }

        private static string Format(string format, object[] args)
        {
            try
            {
                return args == null || args.Length == 0 ? format : string.Format(format, args);
            }
            catch (FormatException)
            {
                return format;
            }
        }

        private static string DescribeScreen()
        {
            return $"{GetSystemMetrics(SM_CXSCREEN)}x{GetSystemMetrics(SM_CYSCREEN)} at {GetDpiForSystem()} DPI";
        }

        private static string DescribeWindow(IntPtr window)
        {
            if (window == IntPtr.Zero)
            {
                return "none";
            }

            var className = new StringBuilder(256);
            GetClassName(window, className, className.Capacity);
            GetWindowThreadProcessId(window, out uint processId);
            string processName;
            try
            {
                using (Process process = Process.GetProcessById((int)processId))
                {
                    processName = process.ProcessName;
                }
            }
            catch (Exception)
            {
                processName = "an exited process";
            }
            return $"0x{window.ToInt64():x} ({className}) in {processName} ({processId})";
        }

        [DllImport("user32.dll")]
        private static extern int GetSystemMetrics(int index);

        [DllImport("user32.dll")]
        private static extern uint GetDpiForSystem();

        [DllImport("user32.dll")]
        private static extern IntPtr GetForegroundWindow();

        [DllImport("user32.dll")]
        private static extern uint GetWindowThreadProcessId(IntPtr window, out uint processId);

        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        private static extern int GetClassName(IntPtr window, StringBuilder className, int maxCount);
    }
}
