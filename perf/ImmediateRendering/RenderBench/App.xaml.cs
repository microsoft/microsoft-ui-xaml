using Microsoft.UI.Xaml;

namespace RenderBench;

public partial class App : Application
{
    BenchRunner? _runner;

    public App()
    {
        UnhandledException += (_, e) =>
        {
            BenchEventSource.Log.RunCompleted(2, "unhandled: " + e.Exception);
            Console.Error.WriteLine(e.Exception);
        };
        InitializeComponent();
    }

    protected override void OnLaunched(LaunchActivatedEventArgs args)
    {
        if (!BenchOptions.TryParse(Environment.GetCommandLineArgs()[1..], out var options, out var error))
        {
            Console.Error.WriteLine(error);
            Console.Error.WriteLine(BenchOptions.Usage);
            BenchEventSource.Log.RunCompleted(3, "bad arguments: " + error);
            Exit();
            return;
        }
        if (options.CoveragePage is not null)
        {
            new Coverage.CoverageRunner(options).Start(); // ---- coverage ----
            return;
        }
        _runner = new BenchRunner(options);
        _runner.Start();
    }
}
