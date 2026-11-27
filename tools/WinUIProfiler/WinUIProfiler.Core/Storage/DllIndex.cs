// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Globalization;
using Microsoft.Data.Sqlite;
using WinUIProfiler.Engine.Events;

namespace WinUIProfiler.Engine.Storage;

// The target's image loads and unloads and its hard page faults, as plain integer columns with indexes, so the DllLoads
// view counts each mapping's faults with an index range search instead of decoding every event's JSON. Runs on the
// session writer's thread inside its transactions, like FrameIndex, so a row is saved and trimmed with its event.
internal sealed class DllIndex : IDisposable
{
    // Kernel-Process ImageLoad and ImageUnload; the classic kernel's Image Load (10) and Unload (2); MemoryHardFault (32).
    private const int ManifestLoad = 5;
    private const int ManifestUnload = 6;
    private const int ClassicLoad = 10;
    private const int ClassicUnload = 2;
    private const int HardFault = 32;

    private readonly SqliteConnection connection;
    private readonly SqliteCommand insertImage;
    private readonly SqliteCommand insertFault;

    internal DllIndex(SqliteConnection connection)
    {
        this.connection = connection;
        insertImage = Sql.Prepare(connection, """
            INSERT INTO DllImageEvents (Seq, Qpc, IsLoad, Source, ImageBase, ImageSize, DllPath)
            VALUES ($seq, $qpc, $load, $source, $base, $size, $path)
            """,
            ("$seq", SqliteType.Integer), ("$qpc", SqliteType.Integer), ("$load", SqliteType.Integer), ("$source", SqliteType.Text),
            ("$base", SqliteType.Integer), ("$size", SqliteType.Integer), ("$path", SqliteType.Text));
        insertFault = Sql.Prepare(connection, "INSERT INTO HardFaults (Seq, Qpc, Address) VALUES ($seq, $qpc, $address)",
            ("$seq", SqliteType.Integer), ("$qpc", SqliteType.Integer), ("$address", SqliteType.Integer));
    }

    // Records the target's image or hard fault event. Call right after inserting the event, in the same transaction.
    internal void Add(long sequence, ProfilerEvent profilerEvent, SqliteTransaction transaction)
    {
        switch (profilerEvent.Provider)
        {
            case XamlProvider.KernelMemory when profilerEvent.Opcode == HardFault:
                // A kernel address doesn't fit a signed 64-bit integer; no user-mode DLL is mapped there.
                if (Integer(profilerEvent, "VirtualAddress") is { } address)
                {
                    insertFault.Transaction = transaction;
                    insertFault.Parameters["$seq"].Value = sequence;
                    insertFault.Parameters["$qpc"].Value = profilerEvent.TimestampQpc;
                    insertFault.Parameters["$address"].Value = address;
                    insertFault.ExecuteNonQuery();
                }

                break;
            case XamlProvider.KernelImage when profilerEvent.Opcode is ClassicLoad or ClassicUnload:
                AddImage(sequence, profilerEvent, profilerEvent.Opcode == ClassicLoad, transaction);
                break;
            case XamlProvider.KernelProcess when profilerEvent.EventId is ManifestLoad or ManifestUnload:
                AddImage(sequence, profilerEvent, profilerEvent.EventId == ManifestLoad, transaction);
                break;
        }
    }

    private void AddImage(long sequence, ProfilerEvent profilerEvent, bool load, SqliteTransaction transaction)
    {
        if (Integer(profilerEvent, "ImageBase") is not { } imageBase || Integer(profilerEvent, "ImageSize") is not { } imageSize)
        {
            return;
        }

        insertImage.Transaction = transaction;
        insertImage.Parameters["$seq"].Value = sequence;
        insertImage.Parameters["$qpc"].Value = profilerEvent.TimestampQpc;
        insertImage.Parameters["$load"].Value = load;
        insertImage.Parameters["$source"].Value = profilerEvent.Provider.ToString();
        insertImage.Parameters["$base"].Value = imageBase;
        insertImage.Parameters["$size"].Value = imageSize;
        insertImage.Parameters["$path"].Value =
            profilerEvent.Fields.FirstOrDefault(field => field.Name == "ImageName").Value?.ToString() ?? (object)DBNull.Value;
        insertImage.ExecuteNonQuery();
    }

    internal static long? Integer(ProfilerEvent profilerEvent, string name)
    {
        foreach (var field in profilerEvent.Fields)
        {
            if (field.Name == name)
            {
                return field.Value switch
                {
                    long value => value,
                    int value => value,
                    uint value => value,
                    ulong value when value <= long.MaxValue => (long)value,
                    string text when text.StartsWith("0x", StringComparison.OrdinalIgnoreCase) &&
                        long.TryParse(text.AsSpan(2), NumberStyles.AllowHexSpecifier, CultureInfo.InvariantCulture, out var hex) &&
                        hex >= 0 => hex,
                    string text when long.TryParse(text, NumberStyles.Integer, CultureInfo.InvariantCulture, out var number) => number,
                    _ => null
                };
            }
        }

        return null;
    }

    // Removes the rows of trimmed events. Call in the trimming transaction.
    internal void Trim(long cutoff, SqliteTransaction transaction)
    {
        Sql.Execute(connection, "DELETE FROM DllImageEvents WHERE Seq < $cutoff", transaction, ("$cutoff", cutoff));
        Sql.Execute(connection, "DELETE FROM HardFaults WHERE Seq < $cutoff", transaction, ("$cutoff", cutoff));
    }

    public void Dispose()
    {
        insertImage.Dispose();
        insertFault.Dispose();
    }
}
