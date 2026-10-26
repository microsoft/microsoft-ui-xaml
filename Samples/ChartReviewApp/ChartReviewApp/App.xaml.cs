// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using System;
using Microsoft.UI.Xaml;

namespace ChartReviewApp
{
    public partial class App : Application
    {
        private readonly int _primaryThreadId = Environment.CurrentManagedThreadId;
        public App()
        {
            this.InitializeComponent();
            // The app's own contrast colors apply; no automatic backplates behind text.
            HighContrastAdjustment = ApplicationHighContrastAdjustment.None;
            UnhandledException += (_, e) =>
            {
                try
                {
                    string path = System.IO.Path.Combine(System.IO.Path.GetTempPath(), "ChartsReviewApp-crash.txt");
                    System.IO.File.AppendAllText(path, $"{DateTime.Now:O} {ChartsSample.MainWindow.BuildLabel}{Environment.NewLine}{e.Exception}{Environment.NewLine}{Environment.NewLine}");
                }
                catch (Exception)
                {
                }
            };
        }

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
