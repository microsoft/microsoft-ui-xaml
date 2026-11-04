// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Data.Sqlite;

namespace WinUIProfiler.Engine.Storage;

internal static class Sql
{
    // A command on a connection with an open transaction must be given that transaction.
    internal static void Execute(SqliteConnection connection, string text, SqliteTransaction? transaction = null,
        params (string Name, object? Value)[] parameters)
    {
        using var command = Create(connection, text, transaction, parameters);
        command.ExecuteNonQuery();
    }

    internal static long Scalar(SqliteConnection connection, string text, SqliteTransaction? transaction = null,
        params (string Name, object? Value)[] parameters)
    {
        using var command = Create(connection, text, transaction, parameters);
        return command.ExecuteScalar() is long value ? value : 0;
    }

    // A command run many times: set its parameters' values, and its transaction, before each run.
    internal static SqliteCommand Prepare(SqliteConnection connection, string text, params (string Name, SqliteType Type)[] parameters)
    {
        var command = connection.CreateCommand();
        command.CommandText = text;
        foreach (var (name, type) in parameters)
        {
            command.Parameters.Add(name, type);
        }

        return command;
    }

    private static SqliteCommand Create(SqliteConnection connection, string text, SqliteTransaction? transaction,
        (string Name, object? Value)[] parameters)
    {
        var command = connection.CreateCommand();
        command.CommandText = text;
        command.Transaction = transaction;
        foreach (var (name, value) in parameters)
        {
            command.Parameters.AddWithValue(name, value ?? DBNull.Value);
        }

        return command;
    }
}
