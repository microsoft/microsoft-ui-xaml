// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Globalization;
using System.Security;
using System.Text;
using System.Text.Json.Nodes;

namespace WinUIProfiler.Mcp.Sessions;

/// <summary>
/// The file the server appends each tool call's SQL to, as the call's result lists it: a script whose statements run as is
/// in the sqlite3 shell. Each call's statements come under a comment that says when the call ended, its tool and view, and
/// the session file it read, and over one that says what it computed. A server writes one file, created by its first call
/// and named for when and for its process ID. Creating it deletes the folder's query logs not written to for 30 days.
/// </summary>
internal sealed class QueryLog(string folder)
{
    private const string Prefix = "queries-";
    private const string Extension = ".sql";
    private static readonly TimeSpan Kept = TimeSpan.FromDays(30);
    private static readonly UTF8Encoding Utf8 = new(encoderShouldEmitUTF8Identifier: false);

    private readonly Lock gate = new();
    private string? file;

    /// <summary>%LOCALAPPDATA%\WinUIProfiler\Mcp\logs, which also holds the logs of the server's builds.</summary>
    internal static string DefaultFolder { get; } = Path.Combine(
        Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "WinUIProfiler", "Mcp", "logs");

    /// <summary>The log the tools' calls write to. Only the server sets it, so the self-tests write none unless they set it.</summary>
    internal static QueryLog? Current { get; set; }

    internal string Folder { get; } = folder;

    /// <summary>
    /// A call's part of the log. Its statements end with a semicolon and their line breaks are \n, so the log's are too.
    /// Comments never span lines, so no statement is commented out.
    /// </summary>
    internal static string Format(DateTimeOffset time, string tool, string? view, string? session, IReadOnlyList<ListedQuery> queries,
        string computed)
    {
        var text = new StringBuilder();
        Comment(text, string.Create(CultureInfo.InvariantCulture, $"{time:yyyy-MM-dd HH:mm:ss.fff zzz}  {tool}") +
            (view is null ? "" : $", view {view}"));
        if (session is not null)
        {
            Comment(text, $"File: {session}");
        }

        text.Append('\n');
        if (queries.Count == 0)
        {
            Comment(text, "No SQL ran.");
            text.Append('\n');
        }

        foreach (var query in queries)
        {
            text.Append(query.Sql).Append('\n');
            Comment(text, EventJson.Count(query.Rows, "row", "rows") +
                (query.Runs > 1 ? string.Create(CultureInfo.InvariantCulture, $", in {query.Runs} runs") : ""));
            foreach (var on in query.Files ?? [])
            {
                Comment(text, $"On: {on}");
            }

            text.Append('\n');
        }

        Comment(text, $"Then: {computed}");
        return text.Append('\n').ToString();
    }

    /// <summary>
    /// Appends a call's part of the log, and adds to the call's result queryLog, the log's path, or, when it couldn't be
    /// written, queryLogError, why. The call doesn't fail when the log can't be written.
    /// </summary>
    internal void Write(JsonObject result, string text)
    {
        try
        {
            result.Add("queryLog", Append(text));
        }
        catch (Exception error) when (error is IOException or UnauthorizedAccessException or NotSupportedException or SecurityException)
        {
            result.Add("queryLogError", $"The SQL couldn't be written to the query log in {Folder}: {error.Message}");
        }
    }

    private static void Comment(StringBuilder text, string comment) =>
        text.Append("-- ").Append(comment.ReplaceLineEndings(" ")).Append('\n');

    private string Append(string text)
    {
        lock (gate)
        {
            if (file is null)
            {
                Directory.CreateDirectory(Folder);
                DeleteOld();
                file = Path.Combine(Folder, string.Create(CultureInfo.InvariantCulture,
                    $"{Prefix}{DateTime.Now:yyyyMMdd-HHmmss}-{Environment.ProcessId}{Extension}"));
            }

            File.AppendAllText(file, text, Utf8);
            return file;
        }
    }

    private void DeleteOld()
    {
        var oldest = DateTime.UtcNow - Kept;
        foreach (var old in Directory.EnumerateFiles(Folder, Prefix + "*" + Extension))
        {
            try
            {
                if (File.GetLastWriteTimeUtc(old) < oldest)
                {
                    File.Delete(old);
                }
            }
            catch (Exception error) when (error is IOException or UnauthorizedAccessException)
            {
                // Another server's log, still open, or one already deleted: the next server tries again.
            }
        }
    }
}
