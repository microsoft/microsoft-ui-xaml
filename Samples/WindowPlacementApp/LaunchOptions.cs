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
        // loads, applies, and saves. WinUI always passes isFirstWindow = false, so STARTUPINFO
        // (for example, a shortcut set to "Run: Minimized") is ignored.
        RestoreAndSave,
        // PlacementMode = SaveOnClose. TryLoadPlacementDetails(), then
        // AppWindow.SetCurrentPlacement(details, isFirstWindow), then Activate().
        // WinUI still saves on close. The main window passes isFirstWindow = true, so it
        // honors STARTUPINFO. Otherwise the same result as RestoreAndSave, written by the app.
        SaveOnClose,
        // Like SaveOnClose, but center the saved rect on its work area before applying it.
        SaveOnCloseCentered,
        // PlacementMode = SaveOnClose. Ignore the saved data and MoveAndResize to a fixed rect.
        // WinUI still saves on close.
        SaveOnCloseFixedRect,
        // PlacementMode = Manual. Loads and applies like SaveOnClose, but WinUI doesn't save
        // on close. The app saves only when it calls SavePlacementDetails() (the Save button).
        Manual,
        // No PlacementId. WinUI does nothing.
        None,
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

        // This process was started by a "Restart app" button. Same effect as KeepMinimized,
        // but only for this process. It isn't passed on to later relaunches.
        public bool Restart { get; set; }

        // OverlappedPresenter.IsResizable = false before the first Activate. SetCurrentPlacement
        // uses the live window's resizability, so a saved rect that doesn't fit is moved, not shrunk.
        public bool FixedSize { get; set; }

        public LaunchOptions Clone() => (LaunchOptions)MemberwiseClone();

        public override string ToString() => $"id=\"{Id}\" setup={Setup} keepMinimized={KeepMinimized} restart={Restart} fixedSize={FixedSize}";

        // Command line: [--id=X] [--setup=RestoreAndSave|SaveOnClose|SaveOnCloseCentered|SaveOnCloseFixedRect|Manual|None]
        //               [--keep-minimized] [--restart] [--fixed-size]
        // --restart has the same effect as --keep-minimized, but isn't passed on to later
        // relaunches. The "Restart app" buttons pass it.
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
