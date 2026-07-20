// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using Microsoft.UI.Xaml;
using System;
using System.IO;

namespace InkGridTestApp
{
    public partial class App : Application
    {
        // Persisted next to the executable so the compositor mode survives the restart that is
        // required to change it (InkCanvas reads "UseSystemVisualLink" once per process).
        public static readonly string CompositorModeFilePath =
            Path.Combine(AppContext.BaseDirectory, "compositor_mode.txt");

        public App()
        {
            this.InitializeComponent();
        }

        public static bool ReadUseSystemVisualLink()
        {
            try
            {
                if (File.Exists(CompositorModeFilePath))
                {
                    return string.Equals(
                        File.ReadAllText(CompositorModeFilePath).Trim(),
                        "system",
                        StringComparison.OrdinalIgnoreCase);
                }
            }
            catch
            {
                // Fall through to the default (lifted).
            }

            return false;
        }

        public static void WriteUseSystemVisualLink(bool useSystem)
        {
            try
            {
                File.WriteAllText(CompositorModeFilePath, useSystem ? "system" : "lifted");
            }
            catch
            {
                // Best-effort; the toggle simply won't persist if this fails.
            }
        }

        protected override void OnLaunched(Microsoft.UI.Xaml.LaunchActivatedEventArgs args)
        {
            // Apply the persisted compositor mode BEFORE the window (and thus the first InkCanvas)
            // is created, so the very first InkCanvas.OnLoaded observes the intended resource value.
            try
            {
                Resources["UseSystemVisualLink"] = ReadUseSystemVisualLink();
            }
            catch
            {
                // Leave the App.xaml default (false / lifted) if the resource cannot be set.
            }

            var window = new MainWindow
            {
                Title = "InkGridTestApp"
            };

            window.Activate();
        }
    }
}
