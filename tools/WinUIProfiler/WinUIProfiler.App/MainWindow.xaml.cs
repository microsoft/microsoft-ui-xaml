using Microsoft.UI.Xaml;
using Windows.Graphics;

namespace WinUIProfiler.App;

public sealed partial class MainWindow : Window
{
    public MainWindow()
    {
        InitializeComponent();
        Session.SetOwnerWindow(this);
        AppWindow.Resize(new SizeInt32(1280, 860));
    }
}
