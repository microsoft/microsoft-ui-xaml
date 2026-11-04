// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Globalization;
using System.Text.Json;
using WinUIProfiler.Engine.Events;

namespace WinUIProfiler.Engine.Storage;

// The .NET type of a field value. The names are stored in EventTypes.FieldTypes, so renaming one breaks existing files.
internal enum FieldType
{
    Null,
    Boolean,
    Byte,
    SByte,
    Int16,
    UInt16,
    Int32,
    UInt32,
    Int64,
    UInt64,
    Single,
    Double,
    String,
    Char,
    Guid,
    DateTime,
    Bytes,
    Object // Any other type, stored as its text.
}

// An event's field values as a JSON array, in field order. The names and types are stored once per event type, so
// values read back with their original .NET type. JSON has no Infinity or NaN, so non-finite floating-point values are
// written as the strings "Infinity", "-Infinity" and "NaN".
internal static class EventPayload
{
    internal static FieldType TypeOf(object? value) => value switch
    {
        null => FieldType.Null,
        bool => FieldType.Boolean,
        byte => FieldType.Byte,
        sbyte => FieldType.SByte,
        short => FieldType.Int16,
        ushort => FieldType.UInt16,
        int => FieldType.Int32,
        uint => FieldType.UInt32,
        long => FieldType.Int64,
        ulong => FieldType.UInt64,
        float => FieldType.Single,
        double => FieldType.Double,
        string => FieldType.String,
        char => FieldType.Char,
        Guid => FieldType.Guid,
        DateTime => FieldType.DateTime,
        byte[] => FieldType.Bytes,
        _ => FieldType.Object
    };

    internal static FieldType ParseType(string name) =>
        Enum.TryParse<FieldType>(name, out var type) && Enum.IsDefined(type) && name == type.ToString()
            ? type
            : throw new InvalidDataException($"Unknown field type '{name}'.");

    internal static void Write(Utf8JsonWriter writer, IReadOnlyList<EventField> fields)
    {
        writer.WriteStartArray();
        foreach (var field in fields)
        {
            switch (field.Value)
            {
                case null: writer.WriteNullValue(); break;
                case bool value: writer.WriteBooleanValue(value); break;
                case byte value: writer.WriteNumberValue(value); break;
                case sbyte value: writer.WriteNumberValue(value); break;
                case short value: writer.WriteNumberValue(value); break;
                case ushort value: writer.WriteNumberValue(value); break;
                case int value: writer.WriteNumberValue(value); break;
                case uint value: writer.WriteNumberValue(value); break;
                case long value: writer.WriteNumberValue(value); break;
                case ulong value: writer.WriteNumberValue(value); break;
                case float value when float.IsFinite(value): writer.WriteNumberValue(value); break;
                case float value: writer.WriteStringValue(value.ToString(CultureInfo.InvariantCulture)); break;
                case double value when double.IsFinite(value): writer.WriteNumberValue(value); break;
                case double value: writer.WriteStringValue(value.ToString(CultureInfo.InvariantCulture)); break;
                case string value: writer.WriteStringValue(value); break;
                case char value: writer.WriteStringValue(value.ToString()); break;
                case Guid value: writer.WriteStringValue(value); break;
                case DateTime value: writer.WriteStringValue(value); break;
                case byte[] value: writer.WriteBase64StringValue(value); break;
                default: writer.WriteStringValue(Convert.ToString(field.Value, CultureInfo.InvariantCulture)); break;
            }
        }

        writer.WriteEndArray();
    }

    // Reads the UTF-8 JSON directly, without building a document per event.
    internal static EventField[] Read(ReadOnlySpan<byte> payload, string[] names, FieldType[] types)
    {
        var reader = new Utf8JsonReader(payload);
        if (!reader.Read() || reader.TokenType != JsonTokenType.StartArray)
        {
            throw Mismatch();
        }

        var fields = new EventField[names.Length];
        for (var i = 0; i < fields.Length; i++)
        {
            if (!reader.Read() || reader.TokenType is JsonTokenType.EndArray or JsonTokenType.StartArray or JsonTokenType.StartObject)
            {
                throw Mismatch();
            }

            fields[i] = new(names[i], Read(ref reader, types[i]));
        }

        if (!reader.Read() || reader.TokenType != JsonTokenType.EndArray)
        {
            throw Mismatch();
        }

        return fields;
    }

    private static InvalidDataException Mismatch() => new("An event's payload does not match its event type.");

    private static object? Read(ref Utf8JsonReader reader, FieldType type) => type switch
    {
        FieldType.Null => reader.TokenType == JsonTokenType.Null ? null : throw Mismatch(),
        FieldType.Boolean => reader.GetBoolean(),
        FieldType.Byte => reader.GetByte(),
        FieldType.SByte => reader.GetSByte(),
        FieldType.Int16 => reader.GetInt16(),
        FieldType.UInt16 => reader.GetUInt16(),
        FieldType.Int32 => reader.GetInt32(),
        FieldType.UInt32 => reader.GetUInt32(),
        FieldType.Int64 => reader.GetInt64(),
        FieldType.UInt64 => reader.GetUInt64(),
        FieldType.Single => reader.TokenType == JsonTokenType.String
            ? float.Parse(reader.GetString()!, CultureInfo.InvariantCulture)
            : reader.GetSingle(),
        FieldType.Double => reader.TokenType == JsonTokenType.String
            ? double.Parse(reader.GetString()!, CultureInfo.InvariantCulture)
            : reader.GetDouble(),
        FieldType.Char => reader.GetString() is [var c] ? c : throw Mismatch(),
        FieldType.Guid => reader.GetGuid(),
        FieldType.DateTime => reader.GetDateTime(),
        FieldType.Bytes => reader.GetBytesFromBase64(),
        _ => reader.GetString()
    };
}