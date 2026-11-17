# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# Puts a time limit on a te.exe run. If te.exe is still running when the limit expires, the TAEF processes started
# since the watchdog was armed are dumped and then ended, so a TAEF host that never exits costs one work item instead
# of holding the whole pipeline job until its timeout.
#
# Dot-source this script for Start-TaefHangWatchdog and Stop-TaefHangWatchdog. The watchdog also runs it in a 32-bit
# PowerShell, with -DumpProcessId and -DumpPath, to dump 32-bit processes: a 64-bit process can only write 64-bit
# dumps of them, which lose their x86 context.

[CmdletBinding()]
param(
    [int]$DumpProcessId = 0,
    [string]$DumpPath
)

if (-not ('WinUITestInfra.TaefHangWatchdog' -as [type]))
{
    Add-Type -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Threading;
using System.Threading.Tasks;

namespace WinUITestInfra
{
    public static class TaefHangWatchdog
    {
        // Full memory plus handle, unloaded-module, memory-info and thread-info data.
        private const uint DumpType = 0x2 | 0x4 | 0x20 | 0x800 | 0x1000;

        private static readonly object s_lock = new object();
        private static readonly List<string> s_report = new List<string>();
        private static Timer s_timer;
        private static int s_generation;
        private static bool s_fired;
        private static DateTime s_armedAt;
        private static double s_timeoutMinutes;
        private static string s_dumpFolder;
        private static string[] s_processNames;
        private static string s_scriptPath;

        private sealed class Target
        {
            public Process Process;
            public string ProcessName;
            public int Id;
            public bool IsTe;

            public string Name
            {
                get { return string.Format("{0} ({1})", ProcessName, Id); }
            }
        }

        [DllImport("dbghelp.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool MiniDumpWriteDump(IntPtr process, uint processId, SafeHandle file, uint dumpType,
            IntPtr exceptionParam, IntPtr userStreamParam, IntPtr callbackParam);

        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool IsWow64Process(IntPtr process, [MarshalAs(UnmanagedType.Bool)] out bool wow64Process);

        public static void Arm(double timeoutMinutes, string dumpFolder, string[] processNames, string scriptPath)
        {
            lock (s_lock)
            {
                StopTimer();
                s_report.Clear();
                s_fired = false;
                s_armedAt = DateTime.Now;
                s_timeoutMinutes = timeoutMinutes;
                s_dumpFolder = dumpFolder;
                s_processNames = processNames;
                s_scriptPath = scriptPath;
                s_generation++;
                s_timer = new Timer(OnTimeout, s_generation, TimeSpan.FromMinutes(timeoutMinutes), Timeout.InfiniteTimeSpan);
            }
        }

        // Stops the watchdog. Returns what it did if it fired, otherwise null.
        public static string[] Disarm()
        {
            lock (s_lock)
            {
                StopTimer();
                return s_fired ? s_report.ToArray() : null;
            }
        }

        // Returns null on success, otherwise why the dump couldn't be written.
        public static string WriteDump(int processId, string path)
        {
            string error = null;
            try
            {
                using (Process process = Process.GetProcessById(processId))
                using (FileStream file = new FileStream(path, FileMode.Create, FileAccess.ReadWrite, FileShare.None))
                {
                    if (!MiniDumpWriteDump(process.Handle, (uint)processId, file.SafeFileHandle, DumpType, IntPtr.Zero, IntPtr.Zero, IntPtr.Zero))
                    {
                        // MiniDumpWriteDump reports an HRESULT through GetLastError.
                        error = string.Format("MiniDumpWriteDump failed with 0x{0:X8}.", Marshal.GetLastWin32Error());
                    }
                }
            }
            catch (Exception e)
            {
                error = e.Message;
            }

            if (error != null)
            {
                try
                {
                    File.Delete(path);
                }
                catch (Exception)
                {
                }
            }

            return error;
        }

        private static void OnTimeout(object generation)
        {
            lock (s_lock)
            {
                // Ignore a callback that was already queued when the watchdog was disarmed or re-armed.
                if (s_timer == null || (int)generation != s_generation)
                {
                    return;
                }

                s_fired = true;
                Report(string.Format(
                    "te.exe was still running after {0} minutes, so the TAEF processes started since {1:HH:mm:ss} are being dumped and ended.",
                    s_timeoutMinutes, s_armedAt));

                List<Target> targets = FindTargets();
                if (targets.Count == 0)
                {
                    Report("No running TAEF processes were found.");
                }

                // Dump everything while the hang is intact. Then end the test hosts before te.exe, so te.exe gets a
                // chance to notice, finish its log and exit on its own.
                foreach (Target target in targets)
                {
                    Dump(target);
                }

                foreach (Target target in targets)
                {
                    if (!target.IsTe)
                    {
                        End(target, 0);
                    }
                }

                foreach (Target target in targets)
                {
                    if (target.IsTe)
                    {
                        End(target, 30000);
                    }
                }

                foreach (Target target in targets)
                {
                    target.Process.Dispose();
                }
            }
        }

        private static List<Target> FindTargets()
        {
            int currentProcessId;
            using (Process current = Process.GetCurrentProcess())
            {
                currentProcessId = current.Id;
            }

            // A little slack for clock granularity.
            DateTime notBefore = s_armedAt.AddSeconds(-5);
            List<Target> targets = new List<Target>();
            foreach (Process process in Process.GetProcesses())
            {
                Target target = null;
                try
                {
                    if (process.Id != currentProcessId && IsWatched(process.ProcessName) && process.StartTime >= notBefore)
                    {
                        target = new Target();
                        target.Process = process;
                        target.ProcessName = process.ProcessName;
                        target.Id = process.Id;
                        target.IsTe = string.Equals(process.ProcessName, "te", StringComparison.OrdinalIgnoreCase);
                    }
                }
                catch (Exception)
                {
                    // The process exited or can't be inspected.
                }

                if (target != null)
                {
                    targets.Add(target);
                }
                else
                {
                    process.Dispose();
                }
            }

            return targets;
        }

        private static bool IsWatched(string processName)
        {
            foreach (string name in s_processNames)
            {
                bool matches = name.EndsWith("*")
                    ? processName.StartsWith(name.Substring(0, name.Length - 1), StringComparison.OrdinalIgnoreCase)
                    : string.Equals(processName, name, StringComparison.OrdinalIgnoreCase);
                if (matches)
                {
                    return true;
                }
            }

            return false;
        }

        private static void Dump(Target target)
        {
            string path = Path.Combine(s_dumpFolder, string.Format("{0}.{1}.hang.dmp", target.ProcessName, target.Id));
            string error;
            try
            {
                bool wow64;
                if (Environment.Is64BitProcess && IsWow64Process(target.Process.Handle, out wow64) && wow64)
                {
                    error = WriteDumpFrom32BitProcess(target.Id, path);
                }
                else
                {
                    error = WriteDump(target.Id, path);
                }
            }
            catch (Exception e)
            {
                error = e.Message;
            }

            Report(error == null ? "Wrote " + path : "Couldn't dump " + target.Name + ": " + error);
        }

        private static string WriteDumpFrom32BitProcess(int processId, string path)
        {
            ProcessStartInfo startInfo = new ProcessStartInfo(
                Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.SystemX86), @"WindowsPowerShell\v1.0\powershell.exe"),
                string.Format(
                    "-NoProfile -NonInteractive -ExecutionPolicy Bypass -File \"{0}\" -DumpProcessId {1} -DumpPath \"{2}\"",
                    s_scriptPath, processId, path));
            startInfo.UseShellExecute = false;
            startInfo.CreateNoWindow = true;
            startInfo.RedirectStandardOutput = true;
            startInfo.RedirectStandardError = true;

            using (Process helper = Process.Start(startInfo))
            {
                Task<string> output = helper.StandardOutput.ReadToEndAsync();
                Task<string> errors = helper.StandardError.ReadToEndAsync();
                if (!helper.WaitForExit(10 * 60 * 1000))
                {
                    helper.Kill();
                    return "The 32-bit dump helper didn't finish within 10 minutes.";
                }

                return helper.ExitCode == 0 ? null : (output.Result + errors.Result).Trim();
            }
        }

        private static void End(Target target, int graceMilliseconds)
        {
            try
            {
                if (graceMilliseconds > 0 && target.Process.WaitForExit(graceMilliseconds))
                {
                    Report(target.Name + " exited on its own.");
                    return;
                }

                if (target.Process.HasExited)
                {
                    Report(target.Name + " had already exited.");
                    return;
                }

                target.Process.Kill();
                Report(target.Process.WaitForExit(30000) ? "Ended " + target.Name + "." : target.Name + " didn't exit after being ended.");
            }
            catch (Exception e)
            {
                Report("Couldn't end " + target.Name + ": " + e.Message);
            }
        }

        private static void Report(string message)
        {
            s_report.Add(message);
            Console.WriteLine("TaefHangWatchdog: " + message);
        }

        private static void StopTimer()
        {
            if (s_timer != null)
            {
                s_timer.Dispose();
                s_timer = null;
            }
        }
    }
}
'@
}

if ($DumpProcessId -ne 0)
{
    $dumpError = [WinUITestInfra.TaefHangWatchdog]::WriteDump($DumpProcessId, $DumpPath)
    if ($dumpError)
    {
        Write-Output $dumpError
        exit 1
    }

    exit 0
}

# The TAEF processes a hung run is waiting on: te.exe and the test hosts it starts.
$TaefHangWatchdogProcessNames = @('te', 'TE.ProcessHost*', 'TaefHostApp*')
$TaefHangWatchdogScriptPath = $PSCommandPath

function Start-TaefHangWatchdog
{
    param(
        [Parameter(Mandatory = $true)]
        [double]$TimeoutMinutes,

        [Parameter(Mandatory = $true)]
        [string]$DumpFolder,

        [string[]]$ProcessNames = $TaefHangWatchdogProcessNames
    )

    [WinUITestInfra.TaefHangWatchdog]::Arm($TimeoutMinutes, $DumpFolder, $ProcessNames, $TaefHangWatchdogScriptPath)
}

# Stops the watchdog. Returns what it did if it fired; otherwise returns nothing.
function Stop-TaefHangWatchdog
{
    [WinUITestInfra.TaefHangWatchdog]::Disarm()
}
