using System.Text.Json.Serialization;

namespace WinUIProfiler.Contracts;

[method: JsonConstructor]
public readonly record struct ProcessInstanceKey(uint ProcessId, long? CreationTimeFileTime, Guid InstanceId)
{
    public uint ProcessId { get; } = Require.Positive(ProcessId);
    public long? CreationTimeFileTime
    {
        get;
    } = CreationTimeFileTime is null or > 0
        ? CreationTimeFileTime : throw new ArgumentOutOfRangeException(nameof(CreationTimeFileTime));
    public Guid InstanceId { get; } = Require.Id(InstanceId);
    public void Validate()
    {
        Require.Positive(ProcessId);
        Require.Id(InstanceId);

    }
}

public sealed record SessionScope(Guid SessionId, ProcessInstanceKey Process)
{
    public void Validate()
    {
        Require.Id(SessionId);
        Process.Validate();

    }
}

public static class Require
{
    public static Guid Id(Guid value) => value != Guid.Empty ? value : throw new ArgumentException("An identity cannot be empty.");
    public static string Text(string? value) => !string.IsNullOrWhiteSpace(value) ? value : throw new ArgumentException("Text is required.");
    public static T NotNull<T>(T? value) where T : class => value ?? throw new ArgumentNullException(nameof(value));
    public static T Enum<T>(T value) where T : struct, Enum => System.Enum.IsDefined(value) ? value : throw new ArgumentOutOfRangeException(nameof(value));
    public static uint Positive(uint value) => value > 0 ? value : throw new ArgumentOutOfRangeException(nameof(value));
    public static long Positive(long value) => value > 0 ? value : throw new ArgumentOutOfRangeException(nameof(value));
}

public static class ContractVersions
{
    public const int Schema = 1;
}
