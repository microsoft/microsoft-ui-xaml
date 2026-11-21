using System.Text.Json;
using System.Text.Json.Serialization;

namespace RenderBench.Analysis;

public static class MetricsJson
{
    static readonly JsonSerializerOptions s_options = new()
    {
        WriteIndented = true,
        Converters = { new JsonStringEnumConverter() },
        NumberHandling = JsonNumberHandling.AllowNamedFloatingPointLiterals,
    };

    public static string Serialize(RunMetrics metrics) => JsonSerializer.Serialize(metrics, s_options);

    public static RunMetrics Deserialize(string json)
        => JsonSerializer.Deserialize<RunMetrics>(json, s_options) ?? throw new InvalidDataException("empty metrics json");
}
