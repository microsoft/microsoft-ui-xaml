using System.Text.Json;
using WinUIProfiler.Engine.Snapshots;

namespace WinUIProfiler.App.ViewModels;

internal static class SnapshotFiles
{
    internal static Task<SnapshotViewerData> LoadAsync(SnapshotHistoryEntry entry, CancellationToken token) =>
        SnapshotViewerData.LoadAsync(entry.Saved.Path, token, entry.Process, entry.Saved.CaptureId);

    internal static async Task<SnapshotDiffData> LoadAsync(SnapshotHistoryEntry before, SnapshotHistoryEntry after,
        CancellationToken token) =>
        new(await LoadAsync(before, token).ConfigureAwait(false), await LoadAsync(after, token).ConfigureAwait(false), token);
}

internal static class SnapshotPresentation
{
    private static readonly JsonSerializerOptions DetailOptions = new() { WriteIndented = true };

    internal static string Label(this SnapshotTreeNode node) => node.Type[(node.Type.LastIndexOf('.') + 1)..] +
        (string.IsNullOrEmpty(node.Name) ? "" : $" ({node.Name})") + $" ({node.VisualAddress ?? node.ObjectAddress})";

    internal static string Details(this SnapshotTreeNode node) => JsonSerializer.Serialize(new
    {
        id = node.Id,
        type = node.Type,
        name = node.Name,
        objectAddress = node.ObjectAddress,
        visualAddress = node.VisualAddress,
        diagnosticsHandle = node.DiagnosticsHandle,
        properties = node.Properties.EnumerateObject()
            .Where(property => !(node.VisualAddress is null ? IsDefaultXamlProperty(property) : IsDefaultCompositionProperty(property)))
            .ToDictionary(property => property.Name, property => property.Value)
    }, DetailOptions);

    private static bool IsDefaultXamlProperty(JsonProperty property) => property.Name switch
    {
        "opacity" => MatchesNumber(property.Value, 1),
        "visibility" => MatchesString(property.Value, "Visible"),
        "horizontalAlignment" or "verticalAlignment" => MatchesString(property.Value, "Stretch"),
        "name" or "text" => MatchesString(property.Value, ""),
        "background" => property.Value.ValueKind == JsonValueKind.Null,
        "margin" => IsZeroObject(property.Value, ["left", "top", "right", "bottom"]),
        "renderTransformOrigin" => IsZeroObject(property.Value, ["x", "y"]),
        "renderTransform" => IsDefaultXamlTransform(property.Value),
        _ => false
    };

    private static bool IsDefaultXamlTransform(JsonElement value) =>
        value.ValueKind == JsonValueKind.Null ||
        (value.ValueKind == JsonValueKind.Object &&
            value.EnumerateObject().Count() == 2 &&
            value.TryGetProperty("type", out var type) &&
            MatchesString(type, "Microsoft.UI.Xaml.Media.MatrixTransform") &&
            value.TryGetProperty("matrix", out var matrix) &&
            MatchesNumbers(matrix, [1, 0, 0, 1, 0, 0]));

    private static bool IsZeroObject(JsonElement value, ReadOnlySpan<string> names)
    {
        if (value.ValueKind != JsonValueKind.Object || value.EnumerateObject().Count() != names.Length) return false;
        foreach (var name in names)
            if (!value.TryGetProperty(name, out var component) || !MatchesNumber(component, 0)) return false;
        return true;
    }

    private static bool MatchesString(JsonElement value, string expected) =>
        value.ValueKind == JsonValueKind.String && value.GetString() == expected;

    private static bool IsDefaultCompositionProperty(JsonProperty property) => property.Name switch
    {
        "offset" or "centerPoint" => MatchesNumbers(property.Value, [0, 0, 0]),
        "size" => MatchesNumbers(property.Value, [0, 0]),
        "transformMatrix" => MatchesNumbers(property.Value, [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1]),
        "opacity" => MatchesNumber(property.Value, 1),
        "isVisible" => property.Value.ValueKind == JsonValueKind.True,
        "scale" => MatchesNumbers(property.Value, [1, 1, 1]),
        "rotationAngle" => MatchesNumber(property.Value, 0),
        _ => false
    };

    private static bool MatchesNumber(JsonElement value, double expected) =>
        value.ValueKind == JsonValueKind.Number && value.TryGetDouble(out var number) && number == expected;

    private static bool MatchesNumbers(JsonElement value, ReadOnlySpan<double> expected)
    {
        if (value.ValueKind != JsonValueKind.Array || value.GetArrayLength() != expected.Length) return false;
        var index = 0;
        foreach (var component in value.EnumerateArray())
            if (!MatchesNumber(component, expected[index++])) return false;
        return true;
    }
}
