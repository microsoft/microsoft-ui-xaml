// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Globalization;
using WinUIProfiler.Engine.Storage;

namespace WinUIProfiler.Cli;

internal static partial class CliApplication
{
    private static int Composition(string[] args, TextWriter output, TextWriter error, CancellationToken token)
    {
        if (args is [_, "--help"])
        {
            output.WriteLine(Usage);
            return 0;
        }

        CommandArguments options;
        string path;
        string view;
        long? resource;
        int offset;
        int count;
        try
        {
            options = CommandArguments.Parse(args.Skip(1), ["--view", "--resource", "--offset", "--count"], ["--json"]);
            options.RequirePositionals(1);
            path = Path.GetFullPath(options.Positionals[0]);
            if (!path.EndsWith(".sqlite", StringComparison.OrdinalIgnoreCase))
                throw new ArgumentException("composition reads a session file (.sqlite); use 'record' to import an ETL.");
            view = options.Get("--view") is { } selected
                ? Array.Find(["native", "winui"], value => value.Equals(selected, StringComparison.OrdinalIgnoreCase))
                    ?? throw new ArgumentException("--view must be native or winui.")
                : "winui";
            resource = PositiveLong(options, "--resource");
            offset = options.Integer("--offset", 0);
            count = options.Integer("--count", DefaultDumpLimit);
        }
        catch (ArgumentException exception)
        {
            error.WriteLine(exception.Message);
            error.WriteLine(Usage);
            return 2;
        }

        try
        {
            var recording = SessionFile.ReadComposition(path, token);
            var native = recording.NativeLifetimes.Where(row => resource is null || row.NativeResourceId == resource).ToArray();
            var winui = recording.WinUICreations.Where(row => resource is null || row.NativeResourceId == resource).ToArray();
            var total = view == "native" ? native.Length : winui.Length;
            var rows = (view == "native" ? native.Cast<object>() : winui.Cast<object>())
                .Skip(offset).Take(count == 0 ? int.MaxValue : count).ToArray();
            var summary = new
            {
                recording.NativeCreationObservations,
                recording.NativeReleaseObservations,
                recording.MatchedReleaseObservations,
                NativeLifetimeRows = recording.NativeLifetimes.Count,
                WinUICreationObservations = recording.WinUICreations.Count,
                InferredAssociations = recording.WinUICreations.Count(row => row.Correlation == "InferredSequence"),
                AmbiguousAssociations = recording.WinUICreations.Count(row => row.Correlation == "Ambiguous"),
                UnattributedCreations = recording.WinUICreations.Count(row => row.Correlation == "Unattributed")
            };
            string[] notes =
            [
                "NativeResourceId identifies one process-local resource/proxy generation, not a XAML or public COM object.",
                "WinUI attribution is inferred only for a single same-thread creation candidate within 1 ms, after the previous WinUI counter.",
                "EndNotObserved is not proof of survival or a leak. ReleaseObserved is not final COM destruction or GPU retirement.",
                "Counts describe retained observations; trimming and capture loss can remove evidence."
            ];
            if (options.Has("--json"))
            {
                WriteJson(new
                {
                    Session = recording.Session,
                    Summary = summary,
                    View = view,
                    NativeResourceId = resource,
                    Offset = offset,
                    Returned = rows.Length,
                    Total = total,
                    NextOffset = (long)offset + rows.Length < total ? (long?)offset + rows.Length : null,
                    Rows = rows,
                    Notes = notes
                }, output);
            }
            else
            {
                output.WriteLine(string.Create(CultureInfo.InvariantCulture,
                    $"Native creation observations: {summary.NativeCreationObservations}; release observations: {summary.NativeReleaseObservations}; " +
                    $"matched releases: {summary.MatchedReleaseObservations}."));
                output.WriteLine(string.Create(CultureInfo.InvariantCulture,
                    $"WinUI Composition creations: {summary.WinUICreationObservations}; inferred: {summary.InferredAssociations}; " +
                    $"ambiguous: {summary.AmbiguousAssociations}; unattributed: {summary.UnattributedCreations}."));
                foreach (var note in notes) output.WriteLine(note);
                output.WriteLine(view == "native"
                    ? "NativeResourceId  ResourcePointer     ResourceHandle      CreationSeq  ReleaseSeq  Status"
                    : "WinUISeq  Thread  ResourceName                              Candidates  NativeResourceId  ReleaseSeq  Correlation / NativeStatus");
                foreach (var row in rows)
                {
                    token.ThrowIfCancellationRequested();
                    output.WriteLine(row is NativeCompositionLifetime n
                        ? string.Create(CultureInfo.InvariantCulture,
                            $"{n.NativeResourceId,16} {n.ResourcePointer,18} {n.ResourceHandle,18} {n.CreationSeq,12} {n.ReleaseSeq,11}  {n.Status}")
                        : row is WinUICompositionLifetime w
                            ? string.Create(CultureInfo.InvariantCulture,
                                $"{w.WinUISeq,8} {w.ThreadId,7} {w.ResourceName,-41} {w.CandidateCount,10} {w.NativeResourceId,17} " +
                                $"{w.ReleaseSeq,11}  {w.Correlation} / {w.NativeStatus ?? "No native association"}")
                            : throw new InvalidOperationException("Unknown composition row."));
                }
                output.WriteLine(string.Create(CultureInfo.InvariantCulture, $"{rows.Length} of {total} {view} rows, from offset {offset}."));
            }
            return 0;
        }
        catch (OperationCanceledException)
        {
            error.WriteLine("Cancelled.");
            return 4;
        }
        catch (Exception exception) when (exception is IOException or InvalidDataException or UnauthorizedAccessException or
            Microsoft.Data.Sqlite.SqliteException or InvalidOperationException)
        {
            error.WriteLine($"Could not query '{path}': {exception.Message}");
            return 3;
        }
    }
}
