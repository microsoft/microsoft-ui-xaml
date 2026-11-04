// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using Microsoft.UI.Xaml;

namespace InkBugBash
{
    public partial class App : Application
    {
        // The system-composition switcher (CompositionEngine.TrySetProcessEngine) only exists in the
        // ADO lift repo, not here, so this app always runs on the lifted compositor.
        public static string CompositorStatus => "lifted";

        // Unpackaged app: the install folder may be read-only, so keep state under LocalAppData.
        private static string StateFolder
        {
            get
            {
                var dir = System.IO.Path.Combine(
                    System.Environment.GetFolderPath(System.Environment.SpecialFolder.LocalApplicationData),
                    "InkBugBash");
                System.IO.Directory.CreateDirectory(dir);
                return dir;
            }
        }

        // Custom drying has to be chosen before the canvas loads, so it is persisted and applied on start.
        private static string ModeFile => System.IO.Path.Combine(StateFolder, "ink_mode.txt");

        public static bool CustomDrying =>
            System.IO.File.Exists(ModeFile) && System.IO.File.ReadAllText(ModeFile).Trim() == "custom";

        public static void SetCustomDrying(bool on)
        {
            try { System.IO.File.WriteAllText(ModeFile, on ? "custom" : "default"); }
            catch (System.IO.IOException) { }
        }

        public App()
        {
            this.InitializeComponent();
            UnhandledException += (_, e) => Log($"UnhandledException: {e.Message}\n{e.Exception}");
            System.AppDomain.CurrentDomain.UnhandledException += (_, e) => Log($"AppDomain: {e.ExceptionObject}");

            // A missing StaticResource/ThemeResource surfaces only as a generic XamlParseException,
            // so ask XAML to name the key it could not resolve.
            DebugSettings.IsXamlResourceReferenceTracingEnabled = true;
            DebugSettings.XamlResourceReferenceFailed += (_, e) => Log($"ResourceRefFailed: {e.Message}");
        }

        protected override void OnLaunched(LaunchActivatedEventArgs args)
        {
            try
            {
                Log("OnLaunched: creating window");
                var w = new MainWindow();
                Log("OnLaunched: window constructed");
                w.Activate();
                Log("OnLaunched: activated");
            }
            catch (System.Exception ex)
            {
                Log($"OnLaunched threw: {ex}");
                throw;
            }
        }

        // Startup failures here surface only as a stowed exception with no window, so record them.
        internal static void Log(string message)
        {
            try
            {
                System.IO.File.AppendAllText(
                    System.IO.Path.Combine(StateFolder, "startup-error.txt"),
                    $"[{System.DateTime.Now:HH:mm:ss}] {message}\n\n");
            }
            catch (System.IO.IOException)
            {
            }
        }
    }
}
