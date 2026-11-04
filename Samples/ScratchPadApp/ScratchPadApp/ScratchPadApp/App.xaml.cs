// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using Microsoft.UI.Xaml;
using System;
using System.IO;

namespace ScratchPadApp
{
    // WinUI3 Inking Perf Harness.
    // - Captures inking sampling rate / jitter / frame FPS / startup latency / working set.
    // - Configurable compositor path (lifted vs system) via compositor_mode.txt, surfaced to the
    //   framework InkCanvas through the "ForceCompositorMode" application resource (read once per
    //   process by InkCanvas::IsSystemCompositor in the local instrumented build).
    public partial class App : Application
    {
        // Writable data folder that works BOTH unpackaged and packaged (MSIX install dir is read-only,
        // so we must NOT use AppContext.BaseDirectory for anything we write).
        public static readonly string DataFolder =
            Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "WinUIInkPerf");

        // Persisted so the mode survives the restart required to change it.
        public static readonly string CompositorModeFilePath = Path.Combine(DataFolder, "compositor_mode.txt");

        // App-level harness log. The framework writes the compositor decision to
        // %TEMP%\InkPerfCompositor.log (same virtualized temp within this process).
        public static readonly string HarnessLogPath = Path.Combine(DataFolder, "InkPerfHarness.log");

        public App()
        {
            try { Directory.CreateDirectory(DataFolder); } catch { }

            // Opt THIS process into the system composition engine BEFORE InitializeComponent creates
            // any composition object. The switcher InkCanvas only DETECTS the engine
            // (CompositionEngine.GetForSystemEngine); it does not enable it. On a system-composition
            // device (or with the per-exe switcher override) this makes GetForSystemEngine non-null so
            // the InkCanvas takes the system splice. "lifted" is the safe escape hatch (skips the call).
            string startupMode = ReadCompositorMode();
            if (startupMode == "system" || startupMode == "auto")
            {
                try
                {
                    bool ok = Microsoft.UI.Composition.CompositionEngine.TrySetProcessEngine(
                        Microsoft.UI.Composition.CompositionEngineType.System);
                    Log($"TrySetProcessEngine(System) => {ok} (mode={startupMode})");
                }
                catch (Exception ex)
                {
                    Log($"TrySetProcessEngine(System) threw: 0x{ex.HResult:X8} {ex.Message}");
                }
            }
            else
            {
                Log($"mode={startupMode}; skipping TrySetProcessEngine (lifted)");
            }

            this.InitializeComponent();
            this.UnhandledException += (s, e) =>
            {
                Log("UNHANDLED: " + e.Message + "\r\n" + e.Exception);
            };
        }

        // "system" | "lifted" | "auto" (default). Controls how InkCanvas selects the compositor path.
        public static string ReadCompositorMode()
        {
            try
            {
                if (File.Exists(CompositorModeFilePath))
                {
                    string v = File.ReadAllText(CompositorModeFilePath).Trim().ToLowerInvariant();
                    if (v == "system" || v == "lifted" || v == "auto")
                    {
                        return v;
                    }
                }
            }
            catch { }
            return "auto";
        }

        public static void WriteCompositorMode(string mode)
        {
            try { File.WriteAllText(CompositorModeFilePath, mode); } catch { }
        }

        public static void Log(string message)
        {
            try
            {
                File.AppendAllText(HarnessLogPath,
                    DateTime.Now.ToString("HH:mm:ss.fff") + " " + message + "\r\n");
            }
            catch { }
        }

        protected override void OnLaunched(Microsoft.UI.Xaml.LaunchActivatedEventArgs args)
        {
            // Surface the requested compositor mode to the InkCanvas BEFORE the first InkCanvas loads.
            string mode = ReadCompositorMode();
            try { Resources["ForceCompositorMode"] = mode; } catch { }
            Log($"OnLaunched: ForceCompositorMode='{mode}'");

            var window = new MainWindow { Title = "WinUI3 Inking Perf Harness" };
            window.Activate();
        }
    }
}
