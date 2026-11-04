using Microsoft.UI.Xaml;

namespace ExceptionContextLab;

public partial class App : Application
{
    internal const string TestExceptionMessage = "Issue 11253: intentional async void test exception.";
    private readonly SessionLog _log = new();
    private MainWindow? _window;

    public App()
    {
        InitializeComponent();
        UnhandledException += OnUnhandledException;
        _log.Write("SESSION_START");
    }

    protected override void OnLaunched(LaunchActivatedEventArgs args)
    {
        _window = new MainWindow(_log);
        _window.Activate();
    }

    private void OnUnhandledException(object sender, UnhandledExceptionEventArgs args)
    {
        var isTestException = args.Exception.Message == TestExceptionMessage;
        if (isTestException && _window?.MarkHandled == true)
        {
            args.Handled = true;
        }

        _log.Write($"UNHANDLED test={isTestException} handled={args.Handled} "
            + $"type={args.Exception.GetType().Name} hr=0x{args.Exception.HResult:X8} message={args.Message}");
        if (isTestException)
        {
            _window?.ObserveTestException(args.Handled);
        }
    }
}
