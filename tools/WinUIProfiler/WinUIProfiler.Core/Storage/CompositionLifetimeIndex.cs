// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Data.Sqlite;
using WinUIProfiler.Engine.Events;

namespace WinUIProfiler.Engine.Storage;

// Stores evidence, not an address-keyed live-object cache. The views pair GUIDs and resolve references by event time,
// so out-of-order delivery and rolling retention don't leave stale lifetime or pointer associations.
internal sealed class CompositionLifetimeIndex : IDisposable
{
    internal static readonly Guid VisualInterfaceId = new("c0eeab6c-c897-5ac6-a1c9-63abd5055b9b");
    internal static readonly Guid SurfaceInterfaceId = new("bb8a4953-2c99-4f5a-96f5-4819027fa3ac");

    private readonly SqliteConnection connection;
    private readonly SqliteCommand insertLifetime;
    private readonly SqliteCommand insertReference;

    internal CompositionLifetimeIndex(SqliteConnection connection)
    {
        this.connection = connection;
        insertLifetime = Sql.Prepare(connection, """
            INSERT INTO CompositionObjectEvents
                (Seq, ObjectId, Created, TypeName, ObjectPointer, PublicInterfaceId, PublicInterfacePointer)
            VALUES ($seq, $id, $created, $type, $object, $iid, $pointer)
            """, ("$seq", SqliteType.Integer), ("$id", SqliteType.Text), ("$created", SqliteType.Integer),
            ("$type", SqliteType.Text), ("$object", SqliteType.Integer), ("$iid", SqliteType.Text), ("$pointer", SqliteType.Integer));
        insertReference = Sql.Prepare(connection, """
            INSERT INTO CompositionReferences (Seq, Field, InterfaceId, Address, SurfaceId)
            VALUES ($seq, $field, $iid, $address, $surface)
            """, ("$seq", SqliteType.Integer), ("$field", SqliteType.Text), ("$iid", SqliteType.Text),
            ("$address", SqliteType.Integer), ("$surface", SqliteType.Integer));
    }

    internal void Add(long sequence, ProfilerEvent profilerEvent, SqliteTransaction transaction)
    {
        if (!profilerEvent.TraceLogging)
        {
            return;
        }

        var fields = profilerEvent.Fields;
        if (profilerEvent.Provider == XamlProvider.DComp && profilerEvent.ProviderId == XamlProviders.DComp)
        {
            if (profilerEvent.Name is not ("CompositionObjectCreated" or "CompositionObjectDestroyed") ||
                GuidValue(Value(fields, "ObjectId")) is not { } id)
            {
                return;
            }

            var created = profilerEvent.Name == "CompositionObjectCreated";
            var iid = created ? GuidValue(Value(fields, "PublicInterfaceId")) : null;
            var pointer = created ? Address(Value(fields, "PublicInterfacePointer")) : null;
            insertLifetime.Transaction = transaction;
            insertLifetime.Parameters["$seq"].Value = sequence;
            insertLifetime.Parameters["$id"].Value = id.ToString();
            insertLifetime.Parameters["$created"].Value = created;
            insertLifetime.Parameters["$type"].Value = created ? Value(fields, "TypeName") as string ?? (object)DBNull.Value : DBNull.Value;
            insertLifetime.Parameters["$object"].Value = (object?)(created ? Address(Value(fields, "ObjectPointer")) : null) ?? DBNull.Value;
            insertLifetime.Parameters["$iid"].Value = iid is not null && pointer is not null ? iid.Value.ToString() : DBNull.Value;
            insertLifetime.Parameters["$pointer"].Value = iid is not null && pointer is not null ? pointer.Value : DBNull.Value;
            insertLifetime.ExecuteNonQuery();
            return;
        }

        if (profilerEvent.Provider is not (XamlProvider.Xaml or XamlProvider.XamlProfiler))
        {
            return;
        }

        var surface = profilerEvent.Name is "DCompSurfaceInit" or "VisualSurfaceChanged"
            ? Address(Value(fields, "SurfaceId")) : null;
        foreach (var field in fields)
        {
            if (field.Name is "VisualId" or "ParentVisualId" or "ChildVisualId")
            {
                Reference(field.Name, VisualInterfaceId, Address(Value(fields, field.Name)));
            }
        }

        if (profilerEvent.Provider == XamlProvider.XamlProfiler && profilerEvent.Name == "DCompSurfaceInit")
        {
            Reference("NativeSurfaceId", SurfaceInterfaceId, Address(Value(fields, "NativeSurfaceId")));
        }

        void Reference(string field, Guid iid, long? address)
        {
            if (address is null)
            {
                return;
            }

            insertReference.Transaction = transaction;
            insertReference.Parameters["$seq"].Value = sequence;
            insertReference.Parameters["$field"].Value = field;
            insertReference.Parameters["$iid"].Value = iid.ToString();
            insertReference.Parameters["$address"].Value = address;
            insertReference.Parameters["$surface"].Value = (object?)surface ?? DBNull.Value;
            insertReference.ExecuteNonQuery();
        }
    }

    internal void Trim(long cutoff, SqliteTransaction transaction)
    {
        Sql.Execute(connection, "DELETE FROM CompositionReferences WHERE Seq < $cutoff", transaction, ("$cutoff", cutoff));
        Sql.Execute(connection, "DELETE FROM CompositionObjectEvents WHERE Seq < $cutoff", transaction, ("$cutoff", cutoff));
    }

    // Include the additional evidence rows and B-trees in the writer's conservative batch-size estimate.
    internal static long EstimatedBytes(ProfilerEvent profilerEvent, int payloadBytes) =>
        !profilerEvent.TraceLogging ? 0
        : profilerEvent.Provider == XamlProvider.DComp ? payloadBytes + 256L
        : profilerEvent.Provider is XamlProvider.Xaml or XamlProvider.XamlProfiler
            ? profilerEvent.Fields.Count(field => field.Name is "VisualId" or "ParentVisualId" or "ChildVisualId" or "NativeSurfaceId") * 128L
            : 0;

    // Duplicate names don't establish an identity.
    private static object? Value(IReadOnlyList<EventField> fields, string name)
    {
        object? value = null;
        var found = false;
        foreach (var field in fields)
        {
            if (field.Name == name)
            {
                if (found) return null;
                value = field.Value;
                found = true;
            }
        }

        return value;
    }

    private static Guid? GuidValue(object? value)
    {
        var guid = value is Guid id ? id : value is string text && Guid.TryParse(text, out id) ? id : Guid.Empty;
        return guid == Guid.Empty ? null : guid;
    }

    // SQLite's signed integer preserves all 64 pointer bits; Int32 values represent 32-bit pointer bits.
    private static long? Address(object? value) => (value switch
    {
        ulong address => unchecked((long)address),
        long address => address,
        uint address => address,
        int address => unchecked((uint)address),
        _ => (long?)null
    }) is { } result and not 0 ? result : null;

    public void Dispose()
    {
        insertLifetime.Dispose();
        insertReference.Dispose();
    }
}
