using Microsoft.UI.Xaml;
using Windows.Graphics;

namespace WinUIProfiler.App.Views;

public sealed partial class SnapshotViewerWindow : Window
{
    public SnapshotViewerWindow()
    {
        InitializeComponent();
        AppWindow.Resize(new SizeInt32(1100, 760));
    }
}
