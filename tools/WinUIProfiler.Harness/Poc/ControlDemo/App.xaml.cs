using System.Diagnostics;
using System.Diagnostics.Tracing;
using System.Runtime.InteropServices;
using System.Text.Json;
using Microsoft.UI.Dispatching;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;

namespace WinUIProfilerHarness.ControlDemo;

public partial class App : Application
{
    private const int ControlCount = 40;
    private readonly string _resultPath;
    private Window? _window;
    private bool _started;

    public App()
    {
        string[] arguments = Environment.GetCommandLineArgs();
        if (arguments.Length != 3 || arguments[1] != "--result" ||
            !Path.IsPathFullyQualified(arguments[2]) || File.Exists(arguments[2]))
        {
            throw new ArgumentException("Expected --result <new absolute JSON path>.");
        }

        _resultPath = arguments[2];
        InitializeComponent();
    }

    protected override void OnLaunched(LaunchActivatedEventArgs args)
    {
        var panel = new StackPanel();
        _window = new Window
        {
            Title = "WinUI Profiler Harness - Layout Demo",
            Content = new ScrollViewer { Content = panel }
        };
        panel.Loaded += (_, _) =>
        {
            if (_started)
            {
                return;
            }

            _started = true;
            panel.UpdateLayout();
            if (!panel.DispatcherQueue.TryEnqueue(DispatcherQueuePriority.Low, () => RunScenario(panel)))
            {
                throw new InvalidOperationException("The scenario could not be queued.");
            }
        };
        _window.Activate();
    }

    private void RunScenario(StackPanel panel)
    {
        try
        {
            if (!ScenarioEvents.Log.IsEnabled())
            {
                throw new InvalidOperationException("The scenario ETW provider is not enabled.");
            }

            uint threadId = GetCurrentThreadId();
            ScenarioEvents.Log.ScenarioStart(ControlCount);
            ScenarioWorkload.Populate(panel, ControlCount);
            panel.UpdateLayout();
            bool correct = panel.Children.Count == ControlCount &&
                panel.Children.Cast<UIElement>().Select((child, index) =>
                    child is Button button && Equals(button.Content, $"Button {index}")).All(value => value);
            ScenarioEvents.Log.ScenarioStop(panel.Children.Count, correct);
            if (!correct)
            {
                throw new InvalidOperationException("The generated button tree is incorrect.");
            }

            using var process = Process.GetCurrentProcess();
            ProcessModule framework = process.Modules.Cast<ProcessModule>().Single(module =>
                string.Equals(module.ModuleName, "Microsoft.UI.Xaml.dll", StringComparison.OrdinalIgnoreCase));
            WriteResult(new
            {
                passed = true,
                processId = Environment.ProcessId,
                processCreatedUtc = process.StartTime.ToUniversalTime(),
                uiThreadId = threadId,
                controlCount = panel.Children.Count,
                frameworkPath = framework.FileName,
                frameworkVersion = framework.FileVersionInfo.FileVersion
            });
        }
        catch (Exception error)
        {
            WriteResult(new { passed = false, error = error.ToString() });
            Environment.ExitCode = 1;
        }
        finally
        {
            _window?.Close();
        }
    }

    private void WriteResult(object result)
    {
        Directory.CreateDirectory(Path.GetDirectoryName(_resultPath)!);
        File.WriteAllText(_resultPath, JsonSerializer.Serialize(result, new JsonSerializerOptions { WriteIndented = true }));
    }

    [DllImport("kernel32.dll")]
    private static extern uint GetCurrentThreadId();
}

[EventSource(Name = "WinUIProfilerHarness-ControlDemo", Guid = "ce339c72-348e-4e51-95ed-8db7f7d8fbc2")]
internal sealed class ScenarioEvents : EventSource
{
    internal static readonly ScenarioEvents Log = new();

    [Event(1, Level = EventLevel.Informational, Opcode = EventOpcode.Start, Task = (EventTask)1)]
    public void ScenarioStart(int expectedControls) => WriteEvent(1, expectedControls);

    [Event(2, Level = EventLevel.Informational, Opcode = EventOpcode.Stop, Task = (EventTask)1)]
    public void ScenarioStop(int actualControls, bool correct) => WriteEvent(2, actualControls, correct);
}
