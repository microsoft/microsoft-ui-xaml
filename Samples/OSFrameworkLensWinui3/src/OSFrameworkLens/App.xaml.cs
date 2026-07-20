using System.Diagnostics;
using Microsoft.UI.Xaml;
using OSFrameworkLens.UI;

namespace OSFrameworkLens;

/// <summary>WinUI 3 application entry — no StartupUri; the inspector window is created explicitly.</summary>
public partial class App : Application
{
    private Window? _mainWindow;

    public App()
    {
        InitializeComponent();
        // Last-chance net: a leaked exception out of any UI-thread callback would silently terminate the app.
        UnhandledException += (_, e) =>
        {
            Debug.WriteLine($"App.UnhandledException: {e.Exception}");
            e.Handled = true;
        };
    }

    protected override void OnLaunched(LaunchActivatedEventArgs args)
    {
        _mainWindow = new InspectorWindow();
        _mainWindow.Activate();
    }
}
