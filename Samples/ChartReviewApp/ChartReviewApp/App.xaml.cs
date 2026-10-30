// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using System;
using System.IO;
using System.Runtime.InteropServices;
using Microsoft.UI.Xaml;

namespace ChartReviewApp
{
    public partial class App : Application
    {
        private const string ChartsDll = "Microsoft.UI.Xaml.Controls.Charts.dll";
        private const string ChartsPri = "Microsoft.UI.Xaml.Controls.Charts.pri";
        private readonly int _primaryThreadId = Environment.CurrentManagedThreadId;

        public App()
        {
            // Log from the start: App.xaml loads the Chart's resources, so a missing or broken Chart
            // component fails inside InitializeComponent, before Application.UnhandledException exists.
            AppDomain.CurrentDomain.UnhandledException += (_, e) => WriteCrash(e.ExceptionObject?.ToString());
            CheckChartComponent();
            try
            {
                this.InitializeComponent();
            }
            catch (Exception ex)
            {
                WriteCrash($"Loading App.xaml failed while merging the Chart control's resources (XamlChartsResources).{Environment.NewLine}{ex}");
                throw;
            }
            // The app's own contrast colors apply; no automatic backplates behind text.
            HighContrastAdjustment = ApplicationHighContrastAdjustment.None;
            UnhandledException += (_, e) => WriteCrash(e.Exception?.ToString());
        }

        // A missing Chart DLL or .pri ends the process with a fail-fast in XAML and no message; name the
        // missing file instead, in the crash file that SETUP.txt points to and in a message box.
        private static void CheckChartComponent()
        {
            string folder = AppContext.BaseDirectory;
            foreach (string file in new[] { ChartsDll, ChartsPri })
            {
                if (File.Exists(Path.Combine(folder, file))) continue;
                string message = $"The Chart control is missing: {file} was not found in {folder}. " +
                    "Extract the whole Charts Review App zip again, then run launch.cmd.";
                WriteCrash(message);
                MessageBoxW(IntPtr.Zero, message, "Charts Review App", 0x10 /* MB_ICONERROR */);
                Environment.Exit(1);
            }
        }

        internal static void WriteCrash(string details)
        {
            try
            {
                string path = Path.Combine(Path.GetTempPath(), "ChartsReviewApp-crash.txt");
                File.AppendAllText(path, $"{DateTime.Now:O} {ChartsSample.MainWindow.BuildLabel}{Environment.NewLine}{details}{Environment.NewLine}{Environment.NewLine}");
            }
            catch (Exception)
            {
            }
        }

        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        private static extern int MessageBoxW(IntPtr owner, string text, string caption, uint type);

        protected override void OnLaunched(Microsoft.UI.Xaml.LaunchActivatedEventArgs args)
        {
            if (Environment.CurrentManagedThreadId != _primaryThreadId)
            {
                return;
            }

            m_window ??= new ChartsSample.MainWindow(ChartsSample.MainWindow.BuildLabel + " | C# | Unpackaged");
            m_window.Activate();
        }

        private Window m_window;
    }
}
