// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Text.Json;
using System.Text.Json.Nodes;
using WinUIProfiler.Engine.Storage;

namespace WinUIProfiler.Mcp.Sessions;

/// <summary>
/// A view's result; what the view computed from the rows its statements read, in a sentence or two; and the paths of
/// the session files the result lists, when it lists some, so that its statements don't name them again.
/// </summary>
internal sealed record ToolAnswer(object Result, string Computed, IReadOnlyList<string>? Listed = null);

/// <summary>
/// The SQL statements one tool call's reads of session files run, which its result lists so the client can say how the
/// call got its answer. Start it synchronously in the flow that reads: the tasks and threads that flow starts report to it
/// too, and other calls, even those running at the same time, don't.
/// </summary>
internal sealed class QueryTrail : IDisposable
{
    private readonly Lock gate = new();
    private readonly List<Statement> statements = [];
    private readonly IDisposable observation;
    private bool closed;

    private QueryTrail() => observation = SessionQueries.Observe(Add);

    /// <summary>Starts the trail of a tool call.</summary>
    internal static QueryTrail Start() => new();

    /// <summary>
    /// Closes the trail, and returns the view's result as the client gets it: the result's properties, then computed,
    /// what the view computed, and queries, the statements its reads ran, each once, in the order they first ran. A
    /// statement has its SQL, ended with a semicolon; the rows it read, in all its runs; runs, when it ran more than once;
    /// and files, the files it ran on, unless the result gives their paths: it ran only on the session file the result
    /// describes, or on session files it lists (<see cref="ToolAnswer.Listed"/>).
    /// </summary>
    internal JsonObject Finish(ToolAnswer answer)
    {
        ArgumentNullException.ThrowIfNull(answer);
        var ran = Close();
        var result = JsonSerializer.SerializeToNode(answer.Result, Program.ToolSerializerOptions) as JsonObject
            ?? throw new InvalidOperationException("A tool's result is a JSON object.");
        var session = result["session"]?["sessionPath"]?.GetValue<string>();
        var given = (answer.Listed ?? []).Append(session).OfType<string>().Where(file => file.Length > 0)
            .Select(Path.GetFullPath).ToHashSet(StringComparer.OrdinalIgnoreCase);
        var queries = new JsonArray();
        foreach (var statement in ran)
        {
            var query = new JsonObject
            {
                ["sql"] = Ended(statement.Sql),
                ["rows"] = statement.Rows,
            };
            if (statement.Runs > 1)
            {
                query["runs"] = statement.Runs;
            }

            // Named only when the result doesn't give their paths.
            if (!statement.Files.All(file => file.Length > 0 && given.Contains(Path.GetFullPath(file))))
            {
                query["files"] = new JsonArray([.. statement.Files.Select(file => JsonValue.Create(file))]);
            }

            queries.Add(query);
        }

        result.Add("computed", answer.Computed);
        result.Add("queries", queries);
        return result;
    }

    public void Dispose()
    {
        Close();
        observation.Dispose();
    }

    private void Add(SessionQuery query)
    {
        lock (gate)
        {
            // A task the call started can still be reading after the call ended.
            if (closed)
            {
                return;
            }

            var statement = statements.Find(item => string.Equals(item.Sql, query.Sql, StringComparison.Ordinal));
            if (statement is null)
            {
                statement = new(query.Sql);
                statements.Add(statement);
            }

            statement.Runs++;
            statement.Rows += query.Rows;
            if (!statement.Files.Contains(query.Path, StringComparer.OrdinalIgnoreCase))
            {
                statement.Files.Add(query.Path);
            }
        }
    }

    private List<Statement> Close()
    {
        lock (gate)
        {
            closed = true;
            return [.. statements];
        }
    }

    // A statement as the sqlite3 shell runs it, ended with a semicolon.
    private static string Ended(string sql)
    {
        var text = sql.TrimEnd();
        return text.EndsWith(';') ? text : text + ";";
    }

    private sealed class Statement(string sql)
    {
        public string Sql { get; } = sql;

        public long Rows { get; set; }

        public int Runs { get; set; }

        public List<string> Files { get; } = [];
    }
}
