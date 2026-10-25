using Microsoft.UI.Xaml;
using Windows.Graphics;

namespace WinUIProfiler.App;

public sealed partial class MainWindow : Window
{
    private bool closing;
    private bool shutdownPending;
    public MainWindow()
    {
        InitializeComponent();
        Session.SetOwnerWindow(this);
        AppWindow.Resize(new SizeInt32(1280, 860));
        AppWindow.Closing += async (_, args) =>
        {
            if (closing) return;
            args.Cancel = true;
            if (shutdownPending) return;
            shutdownPending = true;
            await Session.StopLiveConnectionAsync();
            closing = true;
            Close();
        };
    }
}
