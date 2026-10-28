// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Linq;
using Microsoft.UI.Windowing;
using Microsoft.UI.Xaml;

namespace WinUICsDesktopSampleApp.WindowPlacement
{
    public static class Log
    {
        public static ObservableCollection<string> Items { get; } = new ObservableCollection<string>();

        public static void Write(string message)
        {
            string line = $"{DateTime.Now:HH:mm:ss.fff}  {message}";
            Debug.WriteLine(line);
            Items.Insert(0, line);
            if (Items.Count > 500)
            {
                Items.RemoveAt(Items.Count - 1);
            }
        }

        public static void Error(string action, Exception ex) =>
            Write($"ERROR {action}: 0x{ex.HResult:X8} {ex.Message.Trim()}");
    }

    public static class PlacementDiagnostics
    {
        private const string PlacementContainerName = "Microsoft.UI.Xaml.WindowPlacement";

        public static string Describe(Window window)
        {
            var appWindow = window.AppWindow;
            string state = appWindow.Presenter is OverlappedPresenter overlapped
                ? overlapped.State.ToString()
                : appWindow.Presenter.Kind.ToString();

            return $"PlacementId: \"{window.PlacementId}\"\n" +
                $"Presenter/state: {state}\n" +
                $"Position (pixels): {appWindow.Position.X}, {appWindow.Position.Y}\n" +
                $"Outer size (pixels): {appWindow.Size.Width} x {appWindow.Size.Height}\n" +
                $"Window Width/Height (DIPs): {window.Width:0.#} x {window.Height:0.#}";
        }

        public static string ReadSavedRecords()
        {
            var settings = Microsoft.Windows.Storage.ApplicationData.GetDefault().LocalSettings;
            if (!settings.Containers.TryGetValue(PlacementContainerName, out var container) ||
                container.Values.Count == 0)
            {
                return "No saved placements. Close a test window with a PlacementId, then refresh.";
            }

            // Diagnostic access to the private format, not a public placement API.
            var records = container.Values.OrderBy(pair => pair.Key).Select(pair =>
            {
                if (pair.Value is Windows.Storage.ApplicationDataCompositeValue fields)
                {
                    return $"{pair.Key}\n" + string.Join("\n", fields.OrderBy(field => field.Key)
                        .Select(field => $"  {field.Key}: {field.Value}"));
                }
                return $"{pair.Key}: {pair.Value}";
            });
            return string.Join("\n\n", records);
        }

        public static void DeleteSavedRecords()
        {
            var settings = Microsoft.Windows.Storage.ApplicationData.GetDefault().LocalSettings;
            if (settings.Containers.TryGetValue(PlacementContainerName, out var container))
            {
                container.Values.Clear();
            }
        }
    }
}
