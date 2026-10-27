using Microsoft.UI.Xaml;

namespace WinUIProfiler.App;

public partial class App : Application
{
    private MainWindow? window;
    public App()
    {
        UnhandledException += (_, error) =>
        {
            System.Diagnostics.Trace.TraceError(error.Exception.ToString());
            Console.Error.WriteLine(error.Message + Environment.NewLine + error.Exception);
        };
        if (Environment.GetEnvironmentVariable("WINUI_PROFILER_TRACE_XAML") == "1")
        {
            DebugSettings.IsXamlResourceReferenceTracingEnabled = true;
            DebugSettings.XamlResourceReferenceFailed += (_, error) => Console.Error.WriteLine(error.Message);
            DebugSettings.BindingFailed += (_, error) => Console.Error.WriteLine(error.Message);
        }
        InitializeComponent();
    }
    protected override void OnLaunched(LaunchActivatedEventArgs args)
    {
        window = new MainWindow();
        window.Activate();
    }
}
