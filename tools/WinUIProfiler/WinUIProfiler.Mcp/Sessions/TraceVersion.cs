// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Security.Cryptography;
using System.Text.Json;
using WinUIProfiler.Engine.Storage;

namespace WinUIProfiler.Mcp.Sessions;

/// <summary>
/// A version of a trace (.etl): its size, and a SHA-256 of its first and last MiB, which hold an ETL's header, with when
/// the recording started and stopped, and its last buffers. Hashing all of a trace of gigabytes would take about as long
/// as reading it again.
/// </summary>
/// <remarks>
/// record_trace keeps the version it read beside each session file it reads to the trace's end, in
/// &lt;session file&gt;.trace.json, with the marker providers it read (see <see cref="TraceMarkers"/>), and reuses that
/// session only while the trace is still that version and the providers are the same. The trace's time can't tell: a
/// trace copied over another keeps its own time, which can be older than the session file. A record without the
/// providers, kept before they were read, doesn't match, so its session, which holds no marker events, is read again.
/// </remarks>
internal sealed record TraceVersion(long Bytes, string Sha256)
{
    internal const string RecordSuffix = ".trace.json";

    private const int SampleBytes = 1024 * 1024;

    private static readonly JsonSerializerOptions RecordOptions = new(JsonSerializerDefaults.Web) { WriteIndented = true };

    /// <summary>The trace's version; null when it can't be read.</summary>
    internal static TraceVersion? Read(string trace)
    {
        try
        {
            using var stream = new FileStream(trace, FileMode.Open, FileAccess.Read, FileShare.ReadWrite | FileShare.Delete);
            var bytes = stream.Length;
            var head = (int)Math.Min(bytes, SampleBytes);
            var tail = (int)Math.Min(bytes - head, SampleBytes);
            var sample = new byte[head + tail];
            stream.ReadExactly(sample, 0, head);
            stream.Position = bytes - tail;
            stream.ReadExactly(sample, head, tail);
            return new(bytes, Convert.ToHexString(SHA256.HashData(sample)));
        }
        catch (Exception error) when (error is IOException or UnauthorizedAccessException)
        {
            return null;
        }
    }

    /// <summary>
    /// The version of the trace a session file was read from, as the record beside it says; null when there's no record
    /// of that session and trace, or the session was read with other marker providers.
    /// </summary>
    internal static TraceVersion? Recorded(string sessionPath, SessionFileInfo info, string trace, TraceMarkers markers)
    {
        Record? record;
        try
        {
            record = JsonSerializer.Deserialize<Record>(File.ReadAllText(sessionPath + RecordSuffix), RecordOptions);
        }
        catch (Exception error) when (error is IOException or UnauthorizedAccessException or JsonException)
        {
            return null;
        }

        return record is { Sha256OfFirstAndLastMiB: { } sha256, MarkerProviders: { } providers } &&
            record.SessionStartedUtc == info.StartedUtc && string.Equals(record.Trace, trace, StringComparison.OrdinalIgnoreCase) &&
            providers.SequenceEqual(markers.Key, StringComparer.Ordinal)
            ? new(record.Bytes, sha256)
            : null;
    }

    /// <summary>
    /// Records beside a session file that it was read from this version of the trace, with these marker providers. The
    /// record names the session by when it started, so a record left beside an earlier session file of the same name isn't
    /// taken for the new one's. A record that can't be written only means the session isn't reused.
    /// </summary>
    internal void Keep(string sessionPath, SessionFileInfo info, string trace, TraceMarkers markers)
    {
        try
        {
            File.WriteAllText(sessionPath + RecordSuffix,
                JsonSerializer.Serialize(new Record(trace, Bytes, Sha256, info.StartedUtc, markers.Key), RecordOptions));
        }
        catch (Exception error) when (error is IOException or UnauthorizedAccessException)
        {
            // Not reused, then.
        }
    }

    // What <session file>.trace.json holds. MarkerProviders is TraceMarkers.Key.
    private sealed record Record(string? Trace, long Bytes, string? Sha256OfFirstAndLastMiB, DateTime SessionStartedUtc,
        IReadOnlyList<string>? MarkerProviders);
}
