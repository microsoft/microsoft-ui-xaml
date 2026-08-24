using System;

namespace XamlProfiler.Services;

/// <summary>
/// A snapshotted XamlProfilerTracing event: the event name plus the already-copied
/// payload values. Instances are built on the UI thread from primitives captured on
/// the ETW pump thread, so the recycled-buffer hazard documented in
/// <see cref="EtwListenerService.OnEvent"/> does not apply once the data is in here.
///
/// The typed accessors tolerate missing/out-of-range slots and type mismatches so a
/// single malformed event can never throw its way out of a handler. Note in
/// particular that TraceLogging <c>uint64_t</c> arrives as <see cref="long"/>, which
/// <see cref="U64"/> unwraps.
/// </summary>
public readonly struct EtwEvent
{
    private readonly object?[] _payload;

    public EtwEvent(string name, object?[]? payload)
    {
        Name = name;
        _payload = payload ?? Array.Empty<object?>();
    }

    public string Name { get; }

    /// <summary>Read a 64-bit handle/id slot (TraceLogging uint64_t marshals as Int64).</summary>
    public ulong U64(int index)
    {
        if (index < 0 || index >= _payload.Length) return 0;
        var v = _payload[index];
        if (v == null) return 0;
        try { return unchecked((ulong)Convert.ToInt64(v)); }
        catch { return 0; }
    }

    /// <summary>Read a boolean payload slot.</summary>
    public bool Bool(int index)
    {
        if (index < 0 || index >= _payload.Length) return false;
        var v = _payload[index];
        if (v == null) return false;
        try { return Convert.ToBoolean(v); }
        catch { return false; }
    }

    /// <summary>Read a signed 32-bit payload slot (e.g. an insertion Index).</summary>
    public int I32(int index)
    {
        if (index < 0 || index >= _payload.Length) return 0;
        var v = _payload[index];
        if (v == null) return 0;
        try { return Convert.ToInt32(v); }
        catch { return 0; }
    }

    /// <summary>
    /// Read a (possibly wide) string payload slot. The TraceEvent parser already
    /// materialises PCWSTR / PCSTR into a managed <see cref="string"/>.
    /// </summary>
    public string Str(int index)
    {
        if (index < 0 || index >= _payload.Length) return string.Empty;
        return _payload[index]?.ToString() ?? string.Empty;
    }
}
