using System.Security.Cryptography;
using System.Text.Json;
using System.Text.Json.Serialization;
using Microsoft.UI.Dispatching;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Windows.Graphics;

namespace WinUIProfilerHarness.ControlDemo;

internal sealed class ScenarioPlan
{
    [JsonPropertyName("schema")]
    public required string Schema { get; init; }
    [JsonPropertyName("status")]
    public required string Status { get; init; }
    [JsonPropertyName("request")]
    public required string Request { get; init; }
    [JsonPropertyName("name")]
    public required string Name { get; init; }
    [JsonPropertyName("initialButtonCount")]
    public required int InitialButtonCount { get; init; }
    [JsonPropertyName("steps")]
    public required List<ScenarioStep> Steps { get; init; }
    [JsonIgnore]
    internal string Sha256 { get; private set; } = "";
    [JsonIgnore]
    internal int FinalButtonCount { get; private set; }

    internal static ScenarioPlan Load(string path)
    {
        byte[] bytes = File.ReadAllBytes(path);
        if (bytes.Length > 16384)
        {
            throw new ArgumentException("The scenario plan exceeds 16 KiB.");
        }
        string sha256 = Convert.ToHexString(SHA256.HashData(bytes)).ToLowerInvariant();
        if (bytes.Length >= 3 && bytes[0] == 0xef && bytes[1] == 0xbb && bytes[2] == 0xbf)
        {
            bytes = bytes[3..];
        }
        var options = new JsonSerializerOptions { UnmappedMemberHandling = JsonUnmappedMemberHandling.Disallow };
        ScenarioPlan plan = JsonSerializer.Deserialize<ScenarioPlan>(bytes, options)
            ?? throw new ArgumentException("The scenario plan is empty.");
        using JsonDocument catalog = JsonDocument.Parse(
            File.ReadAllBytes(Path.Combine(AppContext.BaseDirectory, "scenario-capabilities.json")));
        JsonElement limits = catalog.RootElement.GetProperty("limits");
        if (plan.Schema != catalog.RootElement.GetProperty("planSchema").GetString() ||
            plan.Status != "Ready" || string.IsNullOrWhiteSpace(plan.Request) ||
            string.IsNullOrWhiteSpace(plan.Name) || plan.Name.EnumerateRunes().Count() > 120 ||
            plan.InitialButtonCount < 0 || plan.InitialButtonCount > limits.GetProperty("maxButtons").GetInt32() ||
            plan.Steps is null || plan.Steps.Count < 1 || plan.Steps.Count > limits.GetProperty("maxSteps").GetInt32())
        {
            throw new ArgumentException("The scenario does not satisfy the supported plan contract.");
        }
        using JsonDocument document = JsonDocument.Parse(bytes);
        if (document.RootElement.EnumerateObject().Count() != 6)
        {
            throw new ArgumentException("The scenario plan has duplicate or unexpected fields.");
        }
        int count = plan.InitialButtonCount;
        int index = 0;
        foreach (JsonElement element in document.RootElement.GetProperty("steps").EnumerateArray())
        {
            ScenarioStep step = plan.Steps[index++];
            if (step is null)
            {
                throw new ArgumentException($"Step {index} must be an action object.");
            }
            string[] keys = element.EnumerateObject().Select(property => property.Name).Order().ToArray();
            switch (step.Action)
            {
                case "populate" when keys.SequenceEqual(new[] { "action", "count" }) &&
                    step.Count >= 1 && step.Count <= limits.GetProperty("maxButtons").GetInt32():
                    count = step.Count.Value;
                    break;
                case "clear" when keys.SequenceEqual(new[] { "action" }):
                    count = 0;
                    break;
                case "resize" when keys.SequenceEqual(new[] { "action", "height", "width" }) &&
                    step.Width >= limits.GetProperty("minWidth").GetInt32() &&
                    step.Width <= limits.GetProperty("maxWidth").GetInt32() &&
                    step.Height >= limits.GetProperty("minHeight").GetInt32() &&
                    step.Height <= limits.GetProperty("maxHeight").GetInt32():
                    break;
                case "scroll" when keys.SequenceEqual(new[] { "action", "position" }) &&
                    step.Position is "start" or "end" && count > 0:
                    break;
                default:
                    throw new ArgumentException($"Unsupported action or parameters at step {index}.");
            }
        }
        plan.FinalButtonCount = count;
        plan.Sha256 = sha256;
        return plan;
    }

    internal async Task<List<ScenarioStepResult>> ExecuteAsync(Window window, StackPanel panel, ScrollViewer viewer)
    {
        var results = new List<ScenarioStepResult>();
        int expected = InitialButtonCount;
        foreach (ScenarioStep step in Steps)
        {
            int index = results.Count + 1;
            ScenarioEvents.Log.StepStart(index, step.Action);
            switch (step.Action)
            {
                case "populate":
                    panel.Children.Clear();
                    expected = step.Count!.Value;
                    ScenarioWorkload.Populate(panel, expected);
                    break;
                case "clear":
                    panel.Children.Clear();
                    expected = 0;
                    break;
                case "resize":
                    window.AppWindow.Resize(new SizeInt32(step.Width!.Value, step.Height!.Value));
                    break;
                case "scroll":
                    panel.UpdateLayout();
                    if (viewer.ScrollableHeight <= 0)
                    {
                        throw new InvalidOperationException($"Step {index} cannot scroll: the content fits the viewport.");
                    }
                    double target = step.Position == "end" ? viewer.ScrollableHeight : 0;
                    viewer.ChangeView(null, target, null, true);
                    DateTime deadline = DateTime.UtcNow.AddSeconds(5);
                    while (Math.Abs(viewer.VerticalOffset - target) > 0.5 && DateTime.UtcNow < deadline)
                    {
                        await Task.Delay(16);
                    }
                    if (Math.Abs(viewer.VerticalOffset - target) > 0.5)
                    {
                        throw new InvalidOperationException($"Step {index} did not reach the requested scroll position.");
                    }
                    break;
                default:
                    throw new InvalidOperationException("The loaded plan contains an unsupported action.");
            }
            panel.UpdateLayout();
            await SettleAsync(panel.DispatcherQueue);
            panel.UpdateLayout();
            bool correct = HasExpectedButtons(panel, expected) &&
                (step.Action != "resize" ||
                 (window.AppWindow.Size.Width == step.Width && window.AppWindow.Size.Height == step.Height));
            ScenarioEvents.Log.StepStop(index, panel.Children.Count, correct);
            if (!correct)
            {
                throw new InvalidOperationException($"Step {index} failed its action or button-tree correctness check.");
            }
            results.Add(new ScenarioStepResult(index, step.Action, panel.Children.Count,
                window.AppWindow.Size.Width, window.AppWindow.Size.Height, viewer.VerticalOffset));
        }
        return results;
    }

    internal static bool HasExpectedButtons(StackPanel panel, int expected) =>
        panel.Children.Count == expected && panel.Children.Cast<UIElement>().Select((child, index) =>
            child is Button button && Equals(button.Content, $"Button {index}")).All(value => value);

    internal static Task SettleAsync(DispatcherQueue queue)
    {
        var completion = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
        if (!queue.TryEnqueue(DispatcherQueuePriority.Low, () => completion.SetResult()))
        {
            throw new InvalidOperationException("The scenario could not settle on its dispatcher.");
        }
        return completion.Task;
    }
}

internal sealed record ScenarioStep(
    [property: JsonPropertyName("action")] string Action,
    [property: JsonPropertyName("count")] int? Count,
    [property: JsonPropertyName("width")] int? Width,
    [property: JsonPropertyName("height")] int? Height,
    [property: JsonPropertyName("position")] string? Position);

internal sealed record ScenarioStepResult(int Index, string Action, int ControlCount,
    int WindowWidth, int WindowHeight, double VerticalOffset);
