// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Globalization;
using System.Text;
using Microsoft.Data.Sqlite;

namespace WinUIProfiler.Engine.Storage;

// A statement SessionFile ran to read a session file: the file, the statement's SQL with its parameters' values written in
// (see SessionQueries.Inline), and how many rows it read.
public sealed record SessionQuery(string Path, string Sql, long Rows);

// Shows a caller the statements SessionFile's reads run for it, so it can say what it read. Not the statements that check
// a file's version and views each time SessionFile opens one, nor statements that fail.
public static class SessionQueries
{
    private static readonly AsyncLocal<Observer?> Current = new();

    // Calls observe with each statement SessionFile runs once its rows are read, until the result is disposed: the
    // statements of this async flow, including the tasks and threads it starts meanwhile. observe may be called on
    // several threads at once, and an exception it throws fails the read. Observers nest: one observed inside another
    // doesn't hide the statements from it.
    public static IDisposable Observe(Action<SessionQuery> observe)
    {
        ArgumentNullException.ThrowIfNull(observe);
        var observer = new Observer(observe, Current.Value);
        Current.Value = observer;
        return observer;
    }

    // A command's SQL with its parameters' values written in as SQLite literals, as SqliteParameterCollection.AddWithValue
    // binds them, so the statement runs as is, in the sqlite3 shell for one. A parameter is replaced wherever the SQL
    // names it outside string literals, quoted names and comments; one the command doesn't have is left as it is. Line
    // breaks outside string literals and quoted names become \n.
    public static string Inline(SqliteCommand command)
    {
        ArgumentNullException.ThrowIfNull(command);
        var values = new Dictionary<string, object?>(StringComparer.Ordinal);
        foreach (SqliteParameter parameter in command.Parameters)
        {
            values[parameter.ParameterName] = parameter.Value;
        }

        var sql = command.CommandText ?? "";
        var text = new StringBuilder(sql.Length);
        var at = 0;
        while (at < sql.Length)
        {
            var start = at;
            var c = sql[at];
            if (c is '\'' or '"' or '`' or '[')
            {
                // A string literal or a quoted name, kept as it is. A doubled quote inside one is the quote itself; a name
                // in brackets has no escape.
                var close = c == '[' ? ']' : c;
                at++;
                while (at < sql.Length)
                {
                    if (sql[at++] == close)
                    {
                        if (close == ']' || at == sql.Length || sql[at] != close)
                        {
                            break;
                        }

                        at++;
                    }
                }

                text.Append(sql, start, at - start);
            }
            else if (c == '-' && Next(sql, at) == '-')
            {
                var end = sql.IndexOf('\n', at);
                end = end < 0 ? sql.Length : end > at && sql[end - 1] == '\r' ? end - 1 : end;
                text.Append(sql, start, end - start);
                at = end;
            }
            else if (c == '/' && Next(sql, at) == '*')
            {
                var end = sql.IndexOf("*/", at + 2, StringComparison.Ordinal);
                at = end < 0 ? sql.Length : end + 2;
                text.Append(sql[start..at].Replace("\r\n", "\n", StringComparison.Ordinal));
            }
            else if (c is '$' or ':' or '@' && IsNameChar(Next(sql, at)))
            {
                at++;
                while (at < sql.Length && IsNameChar(sql[at]))
                {
                    at++;
                }

                var name = sql[start..at];
                if (values.TryGetValue(name, out var value) || values.TryGetValue(name[1..], out value))
                {
                    var literal = Literal(value, name);

                    // A negative number right after a minus sign would start a comment: 1 -$a with -5 is 1 - -5.
                    if (literal[0] == '-' && text.Length > 0 && text[^1] == '-')
                    {
                        text.Append(' ');
                    }

                    text.Append(literal);
                }
                else
                {
                    text.Append(name);
                }
            }
            else if (IsNameChar(c))
            {
                // A name or a number, whole, so a $ inside a name isn't taken for a parameter.
                while (at < sql.Length && IsNameChar(sql[at]))
                {
                    at++;
                }

                text.Append(sql, start, at - start);
            }
            else if (c == '\r' && Next(sql, at) == '\n')
            {
                text.Append('\n');
                at += 2;
            }
            else
            {
                text.Append(c);
                at++;
            }
        }

        return text.ToString();
    }

    // Tells the observers of this flow, if any, about a statement a read ran and the rows it read.
    internal static void Report(SqliteCommand command, long rows)
    {
        var observer = Live(Current.Value);
        if (observer is null)
        {
            return;
        }

        var query = new SessionQuery(command.Connection?.DataSource ?? "", Inline(command), rows);
        for (; observer is not null; observer = Live(observer.Parent))
        {
            observer.Notify(query);
        }
    }

    // The observer, or the nearest it was observed inside, that isn't disposed yet.
    private static Observer? Live(Observer? observer)
    {
        while (observer is { IsLive: false })
        {
            observer = observer.Parent;
        }

        return observer;
    }

    private static char Next(string text, int index) => index + 1 < text.Length ? text[index + 1] : '\0';

    // The characters SQLite allows in names and parameter names after their first.
    private static bool IsNameChar(char c) => char.IsAsciiLetterOrDigit(c) || c is '_' or '$' || c >= '\u0080';

    private static string Literal(object? value, string parameter) => value switch
    {
        null or DBNull => "NULL",
        bool flag => flag ? "1" : "0",
        sbyte or byte or short or ushort or int or uint or long => Convert.ToString(value, CultureInfo.InvariantCulture)!,

        // Bound as the long with the same bits.
        ulong number => unchecked((long)number).ToString(CultureInfo.InvariantCulture),
        float number => Real(number, parameter),
        double number => Real(number, parameter),
        string text => Text(text),
        byte[] bytes => $"X'{Convert.ToHexString(bytes)}'",
        _ => throw new NotSupportedException(
            $"SessionQueries can't write {parameter}'s value, of type {value.GetType()}, into SQL."),
    };

    // The shortest text that reads back as the same double, kept a REAL: 1 is written 1.0.
    private static string Real(double value, string parameter)
    {
        if (!double.IsFinite(value))
        {
            throw new NotSupportedException($"SessionQueries can't write {parameter}'s value, {value}, into SQL.");
        }

        var text = value.ToString("R", CultureInfo.InvariantCulture);
        return text.Contains('.', StringComparison.Ordinal) || text.Contains('E', StringComparison.Ordinal) ? text : text + ".0";
    }

    // A text literal. SQLite would end the statement at a NUL character, so text with one is written as its UTF-8 bytes.
    private static string Text(string text) => text.Contains('\0', StringComparison.Ordinal)
        ? $"CAST(X'{Convert.ToHexString(Encoding.UTF8.GetBytes(text))}' AS TEXT)"
        : $"'{text.Replace("'", "''", StringComparison.Ordinal)}'";

    private sealed class Observer : IDisposable
    {
        // Null once disposed, so a task that outlives the observer, and still has it as its flow's observer, doesn't keep
        // what the callback holds.
        private Action<SessionQuery>? observe;

        public Observer(Action<SessionQuery> observe, Observer? parent)
        {
            this.observe = observe;
            Parent = parent;
        }

        public Observer? Parent { get; }

        public bool IsLive => Volatile.Read(ref observe) is not null;

        public void Notify(SessionQuery query) => Volatile.Read(ref observe)?.Invoke(query);

        public void Dispose()
        {
            Volatile.Write(ref observe, null);
            if (ReferenceEquals(Current.Value, this))
            {
                Current.Value = Live(Parent);
            }
        }
    }
}
