namespace WinUIProfiler.Cli;

internal static class Program
{
    private static async Task<int> Main(string[] args)
    {
        using var cancellation = new CancellationTokenSource();
        ConsoleCancelEventHandler handler = (_, e) => { e.Cancel = true; cancellation.Cancel(); };
        Console.CancelKeyPress += handler;
        try { return await CliApplication.RunAsync(args, Console.Out, Console.Error, cancellation.Token); }
        finally { Console.CancelKeyPress -= handler; }
    }
}
