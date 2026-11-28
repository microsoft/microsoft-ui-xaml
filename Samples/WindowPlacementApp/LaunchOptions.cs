// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using Windows.Storage;

namespace WindowPlacementApp
{
    // How a window sets up its placement before the first Activate().
    public enum PlacementSetup
    {
        // PlacementId + Activate(). PlacementMode is RestoreAndSave (the default), so WinUI
        // cascades or loads saved placement, applies it, and saves on close. WinUI passes false, so STARTUPINFO
        // (for example, a shortcut set to "Run: Minimized") is ignored.
        RestoreAndSave,
        // PlacementMode = SaveOnClose. TryLoadPlacementDetails(), then
        // AppWindow.SetCurrentPlacement(details, isFirstWindow), then Activate().
        // WinUI still saves on close. The main window passes isFirstWindow = true, so it
        // honors STARTUPINFO. This saved-data path does not discover live cascade sources.
        SaveOnClose,
        // Spec example 3. Like SaveOnClose, but center the saved rect on its work area before
        // applying it. With nothing saved, Resize to 1200x800.
        SaveOnCloseCentered,
        // PlacementMode = SaveOnClose. Ignore the saved data and MoveAndResize to a fixed rect.
        // WinUI still saves on close.
        SaveOnCloseFixedRect,
        // Spec example 7. PlacementMode = Manual. Loads and applies like SaveOnClose, but WinUI doesn't save
        // on close. The app saves only when it calls SavePlacementDetails() (the Save button).
        Manual,
        // No PlacementId. WinUI does nothing.
        None,
        // Spec example 5. No PlacementId. The app loads its own saved details (MyStore) and calls
        // AppWindow.SetCurrentPlacement before Activate. It saves GetCurrentPlacement in Closed.
        AppStorage,
        // Overview "Manual placement" sample. Like Manual, plus the sample's display check
        // (TryGetDisplayArea, then MoveToDisplayArea(DisplayArea.Primary) if the display is gone),
        // and SavePlacementDetails in Closed. The display check needs a build with
        // IxpProjectionPath set (see the csproj). Without it, this setup skips the check.
        ManualSaveInClosed,
    }

    public sealed class LaunchOptions
    {
        public const string DefaultId = "Main";
        private const string SettingsContainer = "WindowPlacementApp.Startup";

        public string Id { get; set; } = DefaultId;
        public PlacementSetup Setup { get; set; } = PlacementSetup.RestoreAndSave;


        // Restart behavior: add AllowShowMinimized to AppWindow.PlacementRestorationBehavior,
        // so a saved minimized window stays minimized.
        public bool KeepMinimized { get; set; }

        // This process was started by a "Restart app" button. Spec example 2: sets
        // AppWindow.PlacementRestorationBehavior to All, so a saved minimized window stays
        // minimized. Only for this process. It isn't passed on to later relaunches.
        public bool Restart { get; set; }

        // OverlappedPresenter.IsResizable = false before the first Activate. WinUI still lets
        // AppWindow shrink a saved rect that doesn't fit the screen.
        public bool FixedSize { get; set; }

        // Spec example 4. Uses SaveOnClose, whatever Setup says. Saves whether the window was full
        // screen when it closed. If it was, adds PlacementInfo.FullScreen to the saved details
        // before Activate, so the window first appears full screen.
        public bool LaunchFullScreen { get; set; }

        // ManualSaveInClosed only. Before the overview sample's display check, replace the loaded
        // details with a copy that names a display that doesn't exist ("\\.\FAKEDISPLAY9") and
        // sits one work-area width to the right. Tests the TryGetDisplayArea() == null path.
        public bool FakeMissingDisplay { get; set; }

        public LaunchOptions Clone() => (LaunchOptions)MemberwiseClone();

        public override string ToString() => $"id=\"{Id}\" setup={Setup} keepMinimized={KeepMinimized} restart={Restart} fixedSize={FixedSize} launchFullScreen={LaunchFullScreen} fakeMissingDisplay={FakeMissingDisplay}";

        // Command line: [--id=X] [--setup=RestoreAndSave|SaveOnClose|SaveOnCloseCentered|SaveOnCloseFixedRect|Manual|None|AppStorage|ManualSaveInClosed]
        //               [--keep-minimized] [--restart] [--fixed-size] [--launch-fullscreen] [--fake-missing-display]
        // --restart sets PlacementRestorationBehavior to All (spec example 2), so a saved minimized
        // window stays minimized. It isn't passed on to later relaunches. The "Restart app"
        // buttons pass it.
        // Anything not on the command line comes from the saved startup settings.
        public static LaunchOptions FromCommandLine(string[] args)
        {
            LaunchOptions options = LoadStartupDefaults();
            foreach (string arg in args)
            {
                if (arg == "--restart")
                {
                    options.Restart = true;
                }
                else if (arg == "--keep-minimized")
                {
                    options.KeepMinimized = true;
                }

                else if (arg == "--fixed-size")
                {
                    options.FixedSize = true;
                }
                else if (arg == "--launch-fullscreen")
                {
                    options.LaunchFullScreen = true;
                }
                else if (arg == "--fake-missing-display")
                {
                    options.FakeMissingDisplay = true;
                }
                else if (arg.StartsWith("--id="))
                {
                    options.Id = arg.Substring(5);
                }
                else if (arg.StartsWith("--setup=") && Enum.TryParse(arg.Substring(8), out PlacementSetup setup))
                {
                    options.Setup = setup;
                }
            }
            return options;
        }

        public string ToCommandLine(bool restart)
        {
            var parts = new List<string> { $"--id={Id}", $"--setup={Setup}" };
            if (restart)
            {
                parts.Add("--restart");
            }
            if (KeepMinimized)
            {
                parts.Add("--keep-minimized");
            }

            if (FixedSize)
            {
                parts.Add("--fixed-size");
            }
            if (LaunchFullScreen)
            {
                parts.Add("--launch-fullscreen");
            }
            if (FakeMissingDisplay)
            {
                parts.Add("--fake-missing-display");
            }
            return string.Join(" ", parts);
        }

        public static LaunchOptions LoadStartupDefaults()
        {
            var options = new LaunchOptions();
            if (ApplicationData.Current.LocalSettings.Containers.TryGetValue(SettingsContainer, out ApplicationDataContainer container))
            {
                var values = container.Values;
                if (values.TryGetValue("Id", out object id)) options.Id = (string)id;
                if (values.TryGetValue("Setup", out object setup)) options.Setup = (PlacementSetup)(int)setup;
                if (values.TryGetValue("KeepMinimized", out object keepMinimized)) options.KeepMinimized = (bool)keepMinimized;
                if (values.TryGetValue("FixedSize", out object fixedSize)) options.FixedSize = (bool)fixedSize;
                if (values.TryGetValue("LaunchFullScreen", out object launchFullScreen)) options.LaunchFullScreen = (bool)launchFullScreen;
                if (values.TryGetValue("FakeMissingDisplay", out object fakeMissingDisplay)) options.FakeMissingDisplay = (bool)fakeMissingDisplay;
            }
            return options;
        }

        public void SaveAsStartupDefaults()
        {
            var values = ApplicationData.Current.LocalSettings.CreateContainer(SettingsContainer, ApplicationDataCreateDisposition.Always).Values;
            values.Clear();
            values["Id"] = Id;
            values["Setup"] = (int)Setup;
            values["KeepMinimized"] = KeepMinimized;
            values["FixedSize"] = FixedSize;
            values["LaunchFullScreen"] = LaunchFullScreen;
            values["FakeMissingDisplay"] = FakeMissingDisplay;
        }

        public static void ClearStartupDefaults() => ApplicationData.Current.LocalSettings.DeleteContainer(SettingsContainer);
    }

    // App-wide log. Shown in every window and appended to LocalState\placement-log.txt.
    public static class Log
    {
        private static readonly List<string> s_lines = new();
        public static event Action Changed;

        public static IReadOnlyList<string> Lines => s_lines;

        public static void Write(string message)
        {
            string line = $"{DateTime.Now:HH:mm:ss.fff} [{Environment.ProcessId}] {message}";
            s_lines.Add(line);
            Debug.WriteLine(line);
            try
            {
                File.AppendAllText(Path.Combine(ApplicationData.Current.LocalFolder.Path, "placement-log.txt"), line + Environment.NewLine);
            }
            catch (Exception)
            {
                // Another process may have the file open. The in-memory log is enough.
            }
            Changed?.Invoke();
        }

        public static string PackageIdentity()
        {
            try
            {
                return Windows.ApplicationModel.Package.Current.Id.FamilyName;
            }
            catch (Exception)
            {
                return "(none)";
            }
        }
    }
}
