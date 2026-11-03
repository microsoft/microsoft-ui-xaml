using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Text.Json;
using System.Text.Json.Serialization;
using System.Windows.Automation;
using System.Windows.Forms;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Storage;
using WinUIProfiler.Engine.Target;

namespace ProfilerHarness.Driver;

internal static class Program
{
    private static readonly JsonSerializerOptions JsonOptions = new(JsonSerializerDefaults.Web)
    {
        WriteIndented = true,
        PropertyNameCaseInsensitive = true,
        Converters = { new JsonStringEnumConverter(JsonNamingPolicy.CamelCase) }
    };

    [STAThread]
    private static int Main(string[] args)
    {
        try
        {
            return RunAsync(args).GetAwaiter().GetResult();
        }
        catch (Exception error)
        {
            Console.Error.WriteLine(error);
            return 1;
        }
    }

    private static async Task<int> RunAsync(string[] args)
    {
        if (args.Length != 3 ||
            !args[0].Equals("capture", StringComparison.OrdinalIgnoreCase) ||
            !args[1].Equals("--request", StringComparison.OrdinalIgnoreCase))
        {
            Console.Error.WriteLine("Usage: ProfilerHarness.Driver capture --request <capture-request.json>");
            return 2;
        }

        var requestPath = Path.GetFullPath(args[2]);
        var request = await ReadJsonAsync<CaptureRequest>(requestPath).ConfigureAwait(false);
        request.Validate();
        Directory.CreateDirectory(Path.GetDirectoryName(request.ResultPath)!);

        CaptureResult result;
        TargetConnection? connection = null;
        Exception? disposalError = null;
        var stopwatch = Stopwatch.StartNew();
        try
        {
            WriteProgress("connecting", request.Launch.Target);
            var progress = new Progress<ConnectionProgress>(value => WriteProgress(value.State, value.Detail));
            connection = await TargetConnection.ConnectAsync(
                new(
                    ParseLaunchKind(request.Launch.Kind),
                    request.Launch.Target,
                    request.Launch.Arguments ?? "",
                    request.Launch.WorkingDirectory,
                    request.PayloadDirectory,
                    TrustLocalBinaries: true,
                    SessionDirectory: request.SessionDirectory),
                progress,
                CancellationToken.None).ConfigureAwait(false);

            var processId = checked((int)(connection.Scope?.Process.ProcessId ??
                throw new InvalidOperationException("The profiler connection did not expose a target process.")));
            var recordingPath = await WaitForFirstDrawnFrameAsync(
                connection,
                TimeSpan.FromSeconds(request.FirstFrameTimeoutSeconds),
                CancellationToken.None).ConfigureAwait(false);

            var baselineFrames = SessionFile.ReadFrames(recordingPath)
                .Select(frame => frame.FrameId)
                .ToHashSet();
            WriteProgress("snapshot-before", request.SnapshotDirectory);
            var before = await connection.TakeSnapshotAsync(request.SnapshotDirectory).ConfigureAwait(false);

            var scenario = await ReadJsonAsync<AutomationScenario>(request.ScenarioPath).ConfigureAwait(false);
            scenario.Validate();
            WriteProgress("scenario", scenario.Name);
            var stepResults = await UiAutomationRunner.RunAsync(
                processId,
                scenario,
                TimeSpan.FromSeconds(request.ScenarioTimeoutSeconds)).ConfigureAwait(false);

            if (request.SettleMilliseconds > 0)
            {
                await Task.Delay(request.SettleMilliseconds).ConfigureAwait(false);
            }

            WriteProgress("snapshot-after", request.SnapshotDirectory);
            var after = await connection.TakeSnapshotAsync(request.SnapshotDirectory).ConfigureAwait(false);
            var allFrames = SessionFile.ReadFrames(recordingPath);
            var scenarioFrames = allFrames
                .Where(frame => frame.Drawn && !baselineFrames.Contains(frame.FrameId))
                .ToArray();
            var metrics = CaptureMetrics.From(scenarioFrames, request.FrameBudgetMilliseconds, before, after);
            var etw = connection.Etw;
            result = new(
                SchemaVersion: 1,
                Status: "succeeded",
                Error: null,
                Target: new(processId, connection.Scope!.Process.CreationTimeFileTime, connection.RuntimePath),
                SessionPath: recordingPath,
                BeforeSnapshot: SnapshotResult.From(before),
                AfterSnapshot: SnapshotResult.From(after),
                Metrics: metrics,
                CaptureValidity: new(
                    EtwAvailable: etw is not null,
                    LostEvents: etw?.LostEvents ?? 0,
                    DroppedEvents: etw?.DroppedEvents ?? 0,
                    EtwError: connection.EtwError,
                    RecordingError: connection.RecordingError),
                Steps: stepResults,
                ElapsedMilliseconds: stopwatch.Elapsed.TotalMilliseconds,
                CompletedUtc: DateTimeOffset.UtcNow);
        }
        catch (Exception error)
        {
            result = new(
                SchemaVersion: 1,
                Status: "failed",
                Error: error.ToString(),
                Target: connection?.Scope is { } scope
                    ? new(checked((int)scope.Process.ProcessId), scope.Process.CreationTimeFileTime, connection.RuntimePath)
                    : null,
                SessionPath: connection?.Recording?.Path,
                BeforeSnapshot: null,
                AfterSnapshot: null,
                Metrics: CaptureMetrics.Empty,
                CaptureValidity: new(
                    EtwAvailable: connection?.Etw is not null,
                    LostEvents: connection?.Etw?.LostEvents ?? 0,
                    DroppedEvents: connection?.Etw?.DroppedEvents ?? 0,
                    EtwError: connection?.EtwError,
                    RecordingError: connection?.RecordingError),
                Steps: [],
                ElapsedMilliseconds: stopwatch.Elapsed.TotalMilliseconds,
                CompletedUtc: DateTimeOffset.UtcNow);
        }
        finally
        {
            if (connection is not null)
            {
                try
                {
                    await connection.DisposeAsync().ConfigureAwait(false);
                }
                catch (Exception error)
                {
                    disposalError = error;
                }
            }
        }

        if (disposalError is not null)
        {
            result = result with
            {
                Status = "failed",
                Error = string.Join(
                    Environment.NewLine,
                    new[] { result.Error, $"Profiler cleanup failed: {disposalError}" }.Where(value => value is not null))
            };
        }
        await WriteJsonAsync(request.ResultPath, result).ConfigureAwait(false);
        WriteProgress("complete", request.ResultPath);
        return result.Status == "succeeded" ? 0 : 1;
    }

    private static TargetLaunchKind ParseLaunchKind(string value) =>
        value.ToLowerInvariant() switch
        {
            "packaged" => TargetLaunchKind.Packaged,
            "unpackaged" => TargetLaunchKind.Unpackaged,
            "attach" => TargetLaunchKind.Attach,
            _ => throw new InvalidDataException($"Unsupported launch kind '{value}'.")
        };

    private static async Task<string> WaitForFirstDrawnFrameAsync(
        TargetConnection connection,
        TimeSpan timeout,
        CancellationToken cancellationToken)
    {
        var deadline = Stopwatch.StartNew();
        long? sequence = null;
        string? path = null;
        while (deadline.Elapsed < timeout)
        {
            cancellationToken.ThrowIfCancellationRequested();
            if (connection.Failure is { } failure)
            {
                throw new IOException("The profiler connection failed while waiting for the first frame.", failure);
            }
            if (connection.Completion.IsCompleted)
            {
                throw new IOException("The target exited before the first completed frame was recorded.");
            }

            path ??= connection.Recording?.Path;
            if (path is not null && File.Exists(path))
            {
                var (frames, lastSequence) = SessionFile.ReadChangedFrames(path!, sequence);
                sequence = lastSequence;
                if (frames.Any(frame => frame.Drawn))
                {
                    WriteProgress("first-frame", path);
                    return path;
                }
            }
            await Task.Delay(100, cancellationToken).ConfigureAwait(false);
        }
        throw new TimeoutException($"No completed frame was recorded within {timeout.TotalSeconds:F0} seconds.");
    }

    private static async Task<T> ReadJsonAsync<T>(string path)
    {
        await using var stream = File.OpenRead(path);
        return await JsonSerializer.DeserializeAsync<T>(stream, JsonOptions).ConfigureAwait(false)
            ?? throw new InvalidDataException($"'{path}' is empty.");
    }

    private static async Task WriteJsonAsync<T>(string path, T value)
    {
        Directory.CreateDirectory(Path.GetDirectoryName(path)!);
        var temporary = path + ".partial";
        await using (var stream = new FileStream(
            temporary,
            FileMode.Create,
            FileAccess.Write,
            FileShare.None,
            64 * 1024,
            FileOptions.Asynchronous))
        {
            await JsonSerializer.SerializeAsync(stream, value, JsonOptions).ConfigureAwait(false);
        }
        File.Move(temporary, path, overwrite: true);
    }

    private static void WriteProgress(string state, string detail)
    {
        Console.Error.WriteLine(JsonSerializer.Serialize(
            new { timestampUtc = DateTimeOffset.UtcNow, state, detail },
            JsonOptions));
    }
}

internal sealed record CaptureRequest(
    int SchemaVersion,
    LaunchRequest Launch,
    string PayloadDirectory,
    string SessionDirectory,
    string SnapshotDirectory,
    string ScenarioPath,
    string ResultPath,
    int FirstFrameTimeoutSeconds,
    int ScenarioTimeoutSeconds,
    int SettleMilliseconds,
    double FrameBudgetMilliseconds)
{
    public void Validate()
    {
        if (SchemaVersion != 1)
        {
            throw new InvalidDataException("Only capture request schemaVersion 1 is supported.");
        }
        Launch.Validate();
        RequireAbsoluteDirectory(PayloadDirectory, nameof(PayloadDirectory), mustExist: true);
        RequireAbsoluteDirectory(SessionDirectory, nameof(SessionDirectory), mustExist: false);
        RequireAbsoluteDirectory(SnapshotDirectory, nameof(SnapshotDirectory), mustExist: false);
        RequireAbsoluteFile(ScenarioPath, nameof(ScenarioPath), mustExist: true);
        RequireAbsoluteFile(ResultPath, nameof(ResultPath), mustExist: false);
        if (FirstFrameTimeoutSeconds is < 1 or > 600 ||
            ScenarioTimeoutSeconds is < 1 or > 3600 ||
            SettleMilliseconds is < 0 or > 300_000 ||
            !double.IsFinite(FrameBudgetMilliseconds) ||
            FrameBudgetMilliseconds <= 0)
        {
            throw new InvalidDataException("Capture timing values are outside their supported bounds.");
        }
    }

    private static void RequireAbsoluteDirectory(string path, string name, bool mustExist)
    {
        if (!Path.IsPathFullyQualified(path))
        {
            throw new InvalidDataException($"{name} must be absolute.");
        }
        if (mustExist && !Directory.Exists(path))
        {
            throw new DirectoryNotFoundException($"{name} does not exist: '{path}'.");
        }
        if (!mustExist)
        {
            Directory.CreateDirectory(path);
        }
    }

    private static void RequireAbsoluteFile(string path, string name, bool mustExist)
    {
        if (!Path.IsPathFullyQualified(path))
        {
            throw new InvalidDataException($"{name} must be absolute.");
        }
        if (mustExist && !File.Exists(path))
        {
            throw new FileNotFoundException($"{name} does not exist.", path);
        }
    }
}

internal sealed record LaunchRequest(string Kind, string Target, string? Arguments, string? WorkingDirectory)
{
    public void Validate()
    {
        if (Kind is not ("packaged" or "unpackaged" or "attach"))
        {
            throw new InvalidDataException($"Unsupported launch kind '{Kind}'.");
        }
        if (string.IsNullOrWhiteSpace(Target))
        {
            throw new InvalidDataException("A launch target is required.");
        }
        if (Kind == "unpackaged" && !Path.IsPathFullyQualified(Target))
        {
            throw new InvalidDataException("An unpackaged executable path must be absolute.");
        }
        if (WorkingDirectory is not null && !Path.IsPathFullyQualified(WorkingDirectory))
        {
            throw new InvalidDataException("The launch working directory must be absolute.");
        }
    }
}

internal sealed record AutomationScenario(
    int SchemaVersion,
    string Name,
    string? Description,
    IReadOnlyList<AutomationStep> Steps)
{
    private static readonly HashSet<string> SupportedActions = new(StringComparer.Ordinal)
    {
        "waitForWindow", "waitForElement", "invoke", "click", "setText", "sendKeys",
        "select", "toggle", "wait", "assertExists", "assertNotExists"
    };

    public void Validate()
    {
        if (SchemaVersion != 1 || string.IsNullOrWhiteSpace(Name))
        {
            throw new InvalidDataException("The automation scenario identity is invalid.");
        }
        if (Steps is null || Steps.Count is < 1 or > 200)
        {
            throw new InvalidDataException("An automation scenario requires 1 to 200 steps.");
        }
        for (var index = 0; index < Steps.Count; index++)
        {
            var step = Steps[index] ?? throw new InvalidDataException($"Automation step {index} is null.");
            if (!SupportedActions.Contains(step.Action))
            {
                throw new InvalidDataException($"Automation step {index} uses unsupported action '{step.Action}'.");
            }
            step.Validate(index);
        }
    }
}

internal sealed record AutomationStep(
    string Action,
    AutomationSelector? Selector,
    string? Text,
    string? Keys,
    int? Milliseconds,
    int? TimeoutSeconds,
    string? ExpectedToggleState)
{
    public void Validate(int index)
    {
        if (Action == "wait")
        {
            if (Milliseconds is null or < 0 or > 300_000)
            {
                throw new InvalidDataException($"Automation step {index} needs milliseconds between 0 and 300000.");
            }
        }
        else if (Action != "sendKeys" || Selector is not null)
        {
            (Selector ?? throw new InvalidDataException($"Automation step {index} requires a selector.")).Validate(index);
        }

        if (Action == "setText" && Text is null)
        {
            throw new InvalidDataException($"Automation step {index} requires text.");
        }
        if (Action == "sendKeys" && string.IsNullOrWhiteSpace(Keys))
        {
            throw new InvalidDataException($"Automation step {index} requires keys.");
        }
        if (TimeoutSeconds is < 1 or > 300)
        {
            throw new InvalidDataException($"Automation step {index} timeout is outside 1 to 300 seconds.");
        }
        if (ExpectedToggleState is not null &&
            ExpectedToggleState is not ("on" or "off" or "indeterminate"))
        {
            throw new InvalidDataException($"Automation step {index} has an invalid toggle state.");
        }
    }
}

internal sealed record AutomationSelector(
    string? AutomationId,
    string? Name,
    string? ClassName,
    string? ControlType,
    string? Match,
    int? Index)
{
    public void Validate(int step)
    {
        if (AutomationId is null && Name is null && ClassName is null && ControlType is null)
        {
            throw new InvalidDataException($"Automation step {step} selector is empty.");
        }
        if (Match is not null && Match is not ("exact" or "contains" or "startsWith"))
        {
            throw new InvalidDataException($"Automation step {step} selector has invalid match mode '{Match}'.");
        }
        if (Index is < 0)
        {
            throw new InvalidDataException($"Automation step {step} selector index cannot be negative.");
        }
        if (ControlType is not null)
        {
            _ = UiAutomationRunner.ResolveControlType(ControlType);
        }
    }
}

internal static class UiAutomationRunner
{
    public static Task<IReadOnlyList<AutomationStepResult>> RunAsync(
        int processId,
        AutomationScenario scenario,
        TimeSpan scenarioTimeout)
    {
        var completion = new TaskCompletionSource<IReadOnlyList<AutomationStepResult>>(
            TaskCreationOptions.RunContinuationsAsynchronously);
        var thread = new Thread(() =>
        {
            try
            {
                completion.SetResult(Run(processId, scenario, scenarioTimeout));
            }
            catch (Exception error)
            {
                completion.SetException(error);
            }
        })
        {
            IsBackground = true,
            Name = "Profiler harness UI Automation"
        };
        thread.SetApartmentState(ApartmentState.STA);
        thread.Start();
        return completion.Task;
    }

    private static IReadOnlyList<AutomationStepResult> Run(
        int processId,
        AutomationScenario scenario,
        TimeSpan scenarioTimeout)
    {
        var deadline = Stopwatch.StartNew();
        var results = new List<AutomationStepResult>(scenario.Steps.Count);
        for (var index = 0; index < scenario.Steps.Count; index++)
        {
            if (deadline.Elapsed >= scenarioTimeout)
            {
                throw new TimeoutException($"Scenario '{scenario.Name}' exceeded {scenarioTimeout.TotalSeconds:F0} seconds.");
            }

            var step = scenario.Steps[index];
            var stepWatch = Stopwatch.StartNew();
            ExecuteStep(processId, step, scenarioTimeout - deadline.Elapsed);
            results.Add(new(index, step.Action, stepWatch.Elapsed.TotalMilliseconds, "succeeded"));
        }
        return results;
    }

    private static void ExecuteStep(int processId, AutomationStep step, TimeSpan remaining)
    {
        var timeout = TimeSpan.FromSeconds(step.TimeoutSeconds ?? 30);
        if (timeout > remaining)
        {
            timeout = remaining;
        }

        switch (step.Action)
        {
            case "wait":
                Thread.Sleep(step.Milliseconds!.Value);
                return;
            case "waitForWindow":
            case "waitForElement":
                _ = WaitForElement(processId, step.Selector!, timeout);
                return;
            case "assertExists":
                _ = FindElement(processId, step.Selector!) ??
                    throw new InvalidOperationException($"Expected element was not found: {Describe(step.Selector!)}.");
                return;
            case "assertNotExists":
                if (FindElement(processId, step.Selector!) is not null)
                {
                    throw new InvalidOperationException($"Unexpected element was found: {Describe(step.Selector!)}.");
                }
                return;
            case "sendKeys":
                if (step.Selector is not null)
                {
                    WaitForElement(processId, step.Selector, timeout).SetFocus();
                }
                SendKeys.SendWait(step.Keys!);
                return;
        }

        var element = WaitForElement(processId, step.Selector!, timeout);
        switch (step.Action)
        {
            case "invoke":
                Invoke(element);
                break;
            case "click":
                Click(element);
                break;
            case "setText":
                SetText(element, step.Text!);
                break;
            case "select":
                Select(element);
                break;
            case "toggle":
                Toggle(element, step.ExpectedToggleState);
                break;
            default:
                throw new InvalidOperationException($"Unsupported action '{step.Action}'.");
        }
    }

    private static AutomationElement WaitForElement(
        int processId,
        AutomationSelector selector,
        TimeSpan timeout)
    {
        var stopwatch = Stopwatch.StartNew();
        while (stopwatch.Elapsed < timeout)
        {
            var element = FindElement(processId, selector);
            if (element is not null)
            {
                return element;
            }
            Thread.Sleep(100);
        }
        throw new TimeoutException($"Element was not found within {timeout.TotalSeconds:F0} seconds: {Describe(selector)}.");
    }

    private static AutomationElement? FindElement(int processId, AutomationSelector selector)
    {
        var conditions = new List<Condition>
        {
            new PropertyCondition(AutomationElement.ProcessIdProperty, processId)
        };
        var match = selector.Match ?? "exact";
        if (selector.AutomationId is not null && match == "exact")
        {
            conditions.Add(new PropertyCondition(AutomationElement.AutomationIdProperty, selector.AutomationId));
        }
        if (selector.Name is not null && match == "exact")
        {
            conditions.Add(new PropertyCondition(AutomationElement.NameProperty, selector.Name));
        }
        if (selector.ClassName is not null && match == "exact")
        {
            conditions.Add(new PropertyCondition(AutomationElement.ClassNameProperty, selector.ClassName));
        }
        if (selector.ControlType is not null)
        {
            conditions.Add(new PropertyCondition(
                AutomationElement.ControlTypeProperty,
                ResolveControlType(selector.ControlType)));
        }

        var condition = conditions.Count == 1 ? conditions[0] : new AndCondition([.. conditions]);
        AutomationElementCollection candidates;
        try
        {
            candidates = AutomationElement.RootElement.FindAll(TreeScope.Descendants, condition);
        }
        catch (ElementNotAvailableException)
        {
            return null;
        }

        var matches = candidates.Cast<AutomationElement>()
            .Where(element => Matches(element, selector, match))
            .ToArray();
        var index = selector.Index ?? 0;
        return index < matches.Length ? matches[index] : null;
    }

    private static bool Matches(AutomationElement element, AutomationSelector selector, string match)
    {
        try
        {
            return Matches(element.Current.AutomationId, selector.AutomationId, match) &&
                Matches(element.Current.Name, selector.Name, match) &&
                Matches(element.Current.ClassName, selector.ClassName, match);
        }
        catch (ElementNotAvailableException)
        {
            return false;
        }
    }

    private static bool Matches(string actual, string? expected, string match)
    {
        if (expected is null)
        {
            return true;
        }
        return match switch
        {
            "contains" => actual.Contains(expected, StringComparison.OrdinalIgnoreCase),
            "startsWith" => actual.StartsWith(expected, StringComparison.OrdinalIgnoreCase),
            _ => actual.Equals(expected, StringComparison.Ordinal)
        };
    }

    private static void Invoke(AutomationElement element)
    {
        if (element.TryGetCurrentPattern(InvokePattern.Pattern, out var value))
        {
            ((InvokePattern)value).Invoke();
            return;
        }
        throw new InvalidOperationException("The selected element does not support InvokePattern.");
    }

    private static void Click(AutomationElement element)
    {
        if (element.TryGetCurrentPattern(InvokePattern.Pattern, out var value))
        {
            ((InvokePattern)value).Invoke();
            return;
        }

        var rectangle = element.Current.BoundingRectangle;
        if (rectangle.IsEmpty || !double.IsFinite(rectangle.Left) || !double.IsFinite(rectangle.Top))
        {
            throw new InvalidOperationException("The selected element has no clickable bounds.");
        }
        var x = checked((int)Math.Round(rectangle.Left + rectangle.Width / 2));
        var y = checked((int)Math.Round(rectangle.Top + rectangle.Height / 2));
        if (!SetCursorPos(x, y))
        {
            throw new InvalidOperationException("SetCursorPos failed.");
        }
        mouse_event(MouseEventLeftDown, 0, 0, 0, UIntPtr.Zero);
        mouse_event(MouseEventLeftUp, 0, 0, 0, UIntPtr.Zero);
    }

    private static void SetText(AutomationElement element, string text)
    {
        if (element.TryGetCurrentPattern(ValuePattern.Pattern, out var value) &&
            !((ValuePattern)value).Current.IsReadOnly)
        {
            ((ValuePattern)value).SetValue(text);
            return;
        }

        element.SetFocus();
        SendKeys.SendWait("^a");
        System.Windows.Clipboard.SetText(text);
        SendKeys.SendWait("^v");
    }

    private static void Select(AutomationElement element)
    {
        if (element.TryGetCurrentPattern(SelectionItemPattern.Pattern, out var value))
        {
            ((SelectionItemPattern)value).Select();
            return;
        }
        throw new InvalidOperationException("The selected element does not support SelectionItemPattern.");
    }

    private static void Toggle(AutomationElement element, string? expected)
    {
        if (!element.TryGetCurrentPattern(TogglePattern.Pattern, out var value))
        {
            throw new InvalidOperationException("The selected element does not support TogglePattern.");
        }
        var pattern = (TogglePattern)value;
        if (expected is null)
        {
            pattern.Toggle();
            return;
        }

        var desired = expected switch
        {
            "on" => ToggleState.On,
            "off" => ToggleState.Off,
            "indeterminate" => ToggleState.Indeterminate,
            _ => throw new InvalidOperationException($"Unsupported toggle state '{expected}'.")
        };
        for (var attempt = 0; attempt < 3 && pattern.Current.ToggleState != desired; attempt++)
        {
            pattern.Toggle();
        }
        if (pattern.Current.ToggleState != desired)
        {
            throw new InvalidOperationException($"The element did not reach toggle state '{expected}'.");
        }
    }

    internal static ControlType ResolveControlType(string value) =>
        value switch
        {
            "Button" => ControlType.Button,
            "Calendar" => ControlType.Calendar,
            "CheckBox" => ControlType.CheckBox,
            "ComboBox" => ControlType.ComboBox,
            "Custom" => ControlType.Custom,
            "DataGrid" => ControlType.DataGrid,
            "DataItem" => ControlType.DataItem,
            "Document" => ControlType.Document,
            "Edit" => ControlType.Edit,
            "Group" => ControlType.Group,
            "Header" => ControlType.Header,
            "HeaderItem" => ControlType.HeaderItem,
            "Hyperlink" => ControlType.Hyperlink,
            "Image" => ControlType.Image,
            "List" => ControlType.List,
            "ListItem" => ControlType.ListItem,
            "Menu" => ControlType.Menu,
            "MenuBar" => ControlType.MenuBar,
            "MenuItem" => ControlType.MenuItem,
            "Pane" => ControlType.Pane,
            "ProgressBar" => ControlType.ProgressBar,
            "RadioButton" => ControlType.RadioButton,
            "ScrollBar" => ControlType.ScrollBar,
            "Separator" => ControlType.Separator,
            "Slider" => ControlType.Slider,
            "Spinner" => ControlType.Spinner,
            "SplitButton" => ControlType.SplitButton,
            "StatusBar" => ControlType.StatusBar,
            "Tab" => ControlType.Tab,
            "TabItem" => ControlType.TabItem,
            "Table" => ControlType.Table,
            "Text" => ControlType.Text,
            "Thumb" => ControlType.Thumb,
            "TitleBar" => ControlType.TitleBar,
            "ToolBar" => ControlType.ToolBar,
            "ToolTip" => ControlType.ToolTip,
            "Tree" => ControlType.Tree,
            "TreeItem" => ControlType.TreeItem,
            "Window" => ControlType.Window,
            _ => throw new InvalidDataException($"Unsupported control type '{value}'.")
        };

    private static string Describe(AutomationSelector selector) =>
        JsonSerializer.Serialize(selector);

    private const uint MouseEventLeftDown = 0x0002;
    private const uint MouseEventLeftUp = 0x0004;

    [DllImport("user32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool SetCursorPos(int x, int y);

    [DllImport("user32.dll")]
    private static extern void mouse_event(uint flags, uint dx, uint dy, uint data, UIntPtr extraInfo);
}

internal sealed record AutomationStepResult(
    int Index,
    string Action,
    double ElapsedMilliseconds,
    string Status);

internal sealed record CaptureResult(
    int SchemaVersion,
    string Status,
    string? Error,
    TargetResult? Target,
    string? SessionPath,
    SnapshotResult? BeforeSnapshot,
    SnapshotResult? AfterSnapshot,
    CaptureMetrics Metrics,
    CaptureValidity CaptureValidity,
    IReadOnlyList<AutomationStepResult> Steps,
    double ElapsedMilliseconds,
    DateTimeOffset CompletedUtc);

internal sealed record TargetResult(int ProcessId, long? CreationTimeFileTime, string RuntimePath);

internal sealed record SnapshotResult(
    string Path,
    Guid CaptureId,
    int Roots,
    int XamlNodes,
    int CompositionNodes,
    int Correlations)
{
    public static SnapshotResult From(SavedTreeSnapshot snapshot) =>
        new(
            snapshot.Path,
            snapshot.CaptureId,
            snapshot.Roots,
            snapshot.XamlNodes,
            snapshot.CompositionNodes,
            snapshot.Correlations);
}

internal sealed record CaptureMetrics(
    int DrawnFrames,
    int SlowFrames,
    double? MedianDrawMilliseconds,
    double? P95DrawMilliseconds,
    double? MaxDrawMilliseconds,
    double? MedianSpanMilliseconds,
    double? P95SpanMilliseconds,
    long XamlEvents,
    long CompositionEvents,
    long OtherEvents,
    int BeforeXamlNodes,
    int AfterXamlNodes,
    int XamlNodeDelta,
    int BeforeCompositionNodes,
    int AfterCompositionNodes,
    int CompositionNodeDelta)
{
    public static CaptureMetrics Empty { get; } =
        new(0, 0, null, null, null, null, null, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    public static CaptureMetrics From(
        IReadOnlyList<SessionFrame> frames,
        double frameBudgetMilliseconds,
        SavedTreeSnapshot before,
        SavedTreeSnapshot after)
    {
        var drawTimes = frames
            .Select(frame => frame.EndMilliseconds - frame.LastTickStartMilliseconds)
            .OfType<double>()
            .Where(double.IsFinite)
            .Order()
            .ToArray();
        var spans = frames
            .Select(frame => frame.EndMilliseconds - frame.StartMilliseconds)
            .OfType<double>()
            .Where(double.IsFinite)
            .Order()
            .ToArray();
        return new(
            DrawnFrames: frames.Count,
            SlowFrames: drawTimes.Count(value => value > frameBudgetMilliseconds),
            MedianDrawMilliseconds: Percentile(drawTimes, 0.5),
            P95DrawMilliseconds: Percentile(drawTimes, 0.95),
            MaxDrawMilliseconds: drawTimes.Length == 0 ? null : drawTimes[^1],
            MedianSpanMilliseconds: Percentile(spans, 0.5),
            P95SpanMilliseconds: Percentile(spans, 0.95),
            XamlEvents: frames.Sum(frame => frame.Xaml),
            CompositionEvents: frames.Sum(frame => frame.Composition),
            OtherEvents: frames.Sum(frame => frame.Other),
            BeforeXamlNodes: before.XamlNodes,
            AfterXamlNodes: after.XamlNodes,
            XamlNodeDelta: after.XamlNodes - before.XamlNodes,
            BeforeCompositionNodes: before.CompositionNodes,
            AfterCompositionNodes: after.CompositionNodes,
            CompositionNodeDelta: after.CompositionNodes - before.CompositionNodes);
    }

    private static double? Percentile(IReadOnlyList<double> sorted, double percentile)
    {
        if (sorted.Count == 0)
        {
            return null;
        }
        if (sorted.Count == 1)
        {
            return sorted[0];
        }
        var position = percentile * (sorted.Count - 1);
        var lower = (int)Math.Floor(position);
        var upper = (int)Math.Ceiling(position);
        if (lower == upper)
        {
            return sorted[lower];
        }
        var fraction = position - lower;
        return sorted[lower] + (sorted[upper] - sorted[lower]) * fraction;
    }
}

internal sealed record CaptureValidity(
    bool EtwAvailable,
    int LostEvents,
    long DroppedEvents,
    string? EtwError,
    string? RecordingError);
