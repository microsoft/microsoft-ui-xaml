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
    private readonly int _displaySeconds;
    private readonly ScenarioPlan? _scenarioPlan;
    private readonly bool _explore;
    private Window? _window;
    private DispatcherQueueTimer? _closeTimer;
    private bool _started;

    public App()
    {
        string[] arguments = Environment.GetCommandLineArgs();
        if (arguments.Length == 2 && arguments[1] == "--explore")
        {
            _explore = true;
            _resultPath = "";
            InitializeComponent();
            return;
        }
        if (arguments.Length < 3 || arguments.Length > 7 || arguments.Length % 2 != 1 || arguments[1] != "--result" ||
            !Path.IsPathFullyQualified(arguments[2]) || File.Exists(arguments[2]))
        {
            throw new ArgumentException("Expected --explore, or --result <new absolute JSON path> [--scenario <plan>] [--display-seconds <1-300>].");
        }
        var seen = new HashSet<string>();
        for (int index = 3; index < arguments.Length; index += 2)
        {
            if (!seen.Add(arguments[index]))
            {
                throw new ArgumentException("App arguments must not be repeated.");
            }
            if (arguments[index] == "--display-seconds" &&
                int.TryParse(arguments[index + 1], out _displaySeconds) && _displaySeconds is >= 1 and <= 300)
            {
                continue;
            }
            if (arguments[index] == "--scenario" && Path.IsPathFullyQualified(arguments[index + 1]))
            {
                _scenarioPlan = ScenarioPlan.Load(arguments[index + 1]);
                continue;
            }
            throw new ArgumentException("Invalid scenario or display-duration argument.");
        }

        _resultPath = arguments[2];
        InitializeComponent();
    }

    protected override void OnLaunched(LaunchActivatedEventArgs args)
    {
        var panel = new StackPanel();
        var viewer = new ScrollViewer { Content = panel };
        _window = new Window
        {
            Title = "WinUI Profiler Harness - Layout Demo",
            Content = viewer
        };
        panel.Loaded += (_, _) =>
        {
            if (_started)
            {
                return;
            }

            _started = true;
            panel.UpdateLayout();
            if (!panel.DispatcherQueue.TryEnqueue(DispatcherQueuePriority.Low, () =>
            {
                if (_explore)
                {
                    ScenarioWorkload.Populate(panel, ControlCount);
                    _window!.Title = $"ControlDemo - Explore - {ControlCount} buttons - no capture";
                }
                else
                {
                    _ = RunScenarioAsync(panel, viewer);
                }
            }))
            {
                throw new InvalidOperationException("The scenario could not be queued.");
            }
        };
        _window.Activate();
    }

    private async Task RunScenarioAsync(StackPanel panel, ScrollViewer viewer)
    {
        bool succeeded = false;
        try
        {
            if (!ScenarioEvents.Log.IsEnabled())
            {
                throw new InvalidOperationException("The scenario ETW provider is not enabled.");
            }

            uint threadId = GetCurrentThreadId();
            int expected = _scenarioPlan?.FinalButtonCount ?? ControlCount;
            var steps = new List<ScenarioStepResult>();
            if (_scenarioPlan is not null)
            {
                ScenarioWorkload.Populate(panel, _scenarioPlan.InitialButtonCount);
                panel.UpdateLayout();
                await ScenarioPlan.SettleAsync(panel.DispatcherQueue);
                if (!ScenarioPlan.HasExpectedButtons(panel, _scenarioPlan.InitialButtonCount))
                {
                    throw new InvalidOperationException("The initial button tree is incorrect.");
                }
            }
            ScenarioEvents.Log.ScenarioStart(expected);
            if (_scenarioPlan is null)
            {
                ScenarioWorkload.Populate(panel, ControlCount);
            }
            else
            {
                steps = await _scenarioPlan.ExecuteAsync(_window!, panel, viewer);
            }
            panel.UpdateLayout();
            bool correct = GetCurrentThreadId() == threadId && ScenarioPlan.HasExpectedButtons(panel, expected);
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
                scenarioPlanSha256 = _scenarioPlan?.Sha256,
                scenarioStepCount = steps.Count,
                executedSteps = steps,
                frameworkPath = framework.FileName,
                frameworkVersion = framework.FileVersionInfo.FileVersion
            });
            succeeded = true;
        }
        catch (Exception error)
        {
            WriteResult(new { passed = false, error = error.ToString() });
            Environment.ExitCode = 1;
        }
        finally
        {
            if (succeeded && _displaySeconds > 0)
            {
                _window!.Title = $"ControlDemo - {panel.Children.Count} buttons verified - closes in {_displaySeconds}s";
                _closeTimer = panel.DispatcherQueue.CreateTimer();
                _closeTimer.Interval = TimeSpan.FromSeconds(_displaySeconds);
                _closeTimer.IsRepeating = false;
                _closeTimer.Tick += (_, _) => _window.Close();
                _closeTimer.Start();
            }
            else
            {
                _window?.Close();
            }
        }
    }

    private void WriteResult(object result)
    {
        Directory.CreateDirectory(Path.GetDirectoryName(_resultPath)!);
        File.WriteAllText(_resultPath, JsonSerializer.Serialize(result, new JsonSerializerOptions
        {
            WriteIndented = true,
            PropertyNamingPolicy = JsonNamingPolicy.CamelCase
        }));
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

    [Event(3, Level = EventLevel.Informational, Opcode = EventOpcode.Start, Task = (EventTask)2)]
    public void StepStart(int index, string action) => WriteEvent(3, index, action);

    [Event(4, Level = EventLevel.Informational, Opcode = EventOpcode.Stop, Task = (EventTask)2)]
    public void StepStop(int index, int actualControls, bool correct) => WriteEvent(4, index, actualControls, correct);
}
