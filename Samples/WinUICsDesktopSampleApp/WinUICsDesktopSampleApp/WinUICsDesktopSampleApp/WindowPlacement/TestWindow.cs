// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Threading.Tasks;
using Microsoft.UI.Windowing;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;

namespace WinUICsDesktopSampleApp.WindowPlacement
{
    public sealed class TestWindow : Window
    {
        private readonly TextBlock m_status;
        private bool m_closed;
        private bool m_closePending;

        public TestWindow(int number, string placementId)
        {
            Title = $"Placement test #{number} (id=\"{placementId}\")";
            var root = new StackPanel { Spacing = 12, Padding = new Thickness(16) };
            root.Children.Add(new TextBlock
            {
                Text = "Move or resize this window, close it, then reopen the same PlacementId.",
                TextWrapping = TextWrapping.Wrap,
            });

            var buttons = new VariableSizedWrapGrid { Orientation = Orientation.Horizontal, ItemWidth = 144, ItemHeight = 44 };
            AddButton(buttons, "Refresh", Refresh);
            AddButton(buttons, "Maximize", () => Overlapped().Maximize());
            AddButton(buttons, "Minimize", () => Overlapped().Minimize());
            AddButton(buttons, "Restore", () => Overlapped().Restore());
            AddButton(buttons, "Full screen", () => AppWindow.SetPresenter(AppWindowPresenterKind.FullScreen));
            AddButton(buttons, "Compact overlay", () => AppWindow.SetPresenter(AppWindowPresenterKind.CompactOverlay));
            AddButton(buttons, "Overlapped", () => AppWindow.SetPresenter(AppWindowPresenterKind.Overlapped));
            AddButton(buttons, "Clear PlacementId", () => { PlacementId = ""; Refresh(); });
            AddButton(buttons, "Close", Close);
            AddButton(buttons, "Close in 5s", CloseLater);
            root.Children.Add(buttons);
            root.Children.Add(new TextBlock
            {
                Text = "Use Close in 5s, then minimize or snap with Win+arrows to test saving that state.",
                TextWrapping = TextWrapping.Wrap,
            });
            m_status = new TextBlock
            {
                FontFamily = new FontFamily("Consolas"),
                IsTextSelectionEnabled = true,
                TextWrapping = TextWrapping.Wrap,
            };
            root.Children.Add(m_status);
            Content = new ScrollViewer { Content = root, HorizontalScrollBarVisibility = ScrollBarVisibility.Disabled };

            AppWindow.Changed += (sender, args) =>
            {
                if (args.DidPositionChange || args.DidSizeChange || args.DidPresenterChange)
                {
                    Refresh();
                }
            };
            Activated += (sender, args) =>
            {
                Log.Write($"{Title}: Activated {args.WindowActivationState}");
                Refresh();
            };
            Closed += (sender, args) => m_closed = true;
        }

        private OverlappedPresenter Overlapped()
        {
            if (AppWindow.Presenter is not OverlappedPresenter)
            {
                AppWindow.SetPresenter(AppWindowPresenterKind.Overlapped);
            }
            return (OverlappedPresenter)AppWindow.Presenter;
        }

        private void Refresh()
        {
            if (!m_closed)
            {
                m_status.Text = PlacementDiagnostics.Describe(this);
                Log.Write($"{Title}: {m_status.Text.Replace("\n", "; ")}");
            }
        }

        private async void CloseLater()
        {
            if (m_closePending)
            {
                return;
            }
            m_closePending = true;
            Log.Write($"{Title}: closing in 5 seconds");
            await Task.Delay(5000);
            if (!m_closed)
            {
                Close();
            }
        }

        private void AddButton(Panel panel, string text, Action action)
        {
            var button = new Button { Content = text, HorizontalAlignment = HorizontalAlignment.Stretch };
            button.Click += (sender, args) =>
            {
                try
                {
                    action();
                }
                catch (Exception ex)
                {
                    m_status.Text = $"{text}: 0x{ex.HResult:X8} {ex.Message}";
                    Log.Error($"{Title}: {text}", ex);
                }
            };
            panel.Children.Add(button);
        }
    }
}
