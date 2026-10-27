// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Data.Sqlite;
using WinUIProfiler.Engine.Events;

namespace WinUIProfiler.Engine.Storage;

// The UI elements the events mention, and for each element the events that mention it. Runs on the session writer's
// thread inside its transactions, so a mention is saved and trimmed together with its event.
//
// There are no create or destroy events. An address is known to be a UI element once it appears in a field that only
// ever holds a CUIElement* (tree, child, layout, template, visual state, render and composition-link events).
// DependencyObject_Enter, PeerAssociated and ContentChanged's content also carry other DependencyObjects, so their
// mentions of an address are held until the address is known to be a UI element. DependencyObject_Enter reports the
// type each time an object enters the tree: a different type at a known address means the memory was reused by a new
// object, so a new element starts there. Reuse by an object of the same type can't be detected.
internal sealed class ObjectIndex : IDisposable
{
    // Mentions held per address: a UI element is confirmed within a few, other objects never are.
    private const int MaxHeldMentions = 8;

    // Elements whose rows were all trimmed are remembered, so a later mention keeps their ID, type and name.
    private const int MaxTrimmedElements = 100_000;

    private enum Role : byte
    {
        UIElement,
        Shared
    }

    // Where an event type mentions UI elements, and for DependencyObject_Enter where its type and name are.
    private sealed record EventRoles((int Field, Role Role)[] Fields, int ObjectField, int TypeField, int NameField);

    private sealed class Element(long id, long address, string? type, string? name)
    {
        public long Id { get; } = id;
        public long Address { get; } = address;
        public string? Type { get; set; } = type;
        public string? Name { get; set; } = name;
        public long LastSequence { get; set; }
        // False once trimming deleted its row; the next mention writes it again.
        public bool Stored { get; set; }
    }

    private sealed class Held
    {
        public string? Type { get; set; }
        public string? Name { get; set; }
        public List<(long Sequence, int Field)> Mentions { get; } = new(2);
    }

    private readonly SqliteConnection connection;
    private readonly SqliteCommand insertObject;
    private readonly SqliteCommand updateObject;
    private readonly SqliteCommand insertMention;
    private readonly Dictionary<long, EventRoles> rolesByType = [];
    private readonly Dictionary<long, Element> elements = [];
    private readonly Dictionary<long, Held> held = [];
    private readonly Dictionary<string, string> typeNames = [];
    private long elementCount;

    internal ObjectIndex(SqliteConnection connection)
    {
        this.connection = connection;
        insertObject = Sql.Prepare(connection, "INSERT INTO Objects (ObjectId, Address, Type, Name) VALUES ($id, $address, $type, $name)",
            ("$id", SqliteType.Integer), ("$address", SqliteType.Integer), ("$type", SqliteType.Text), ("$name", SqliteType.Text));
        updateObject = Sql.Prepare(connection, "UPDATE Objects SET Type = $type, Name = $name WHERE ObjectId = $id",
            ("$id", SqliteType.Integer), ("$type", SqliteType.Text), ("$name", SqliteType.Text));
        insertMention = Sql.Prepare(connection, "INSERT INTO ObjectEvents (Seq, ObjectId, Field) VALUES ($seq, $object, $field)",
            ("$seq", SqliteType.Integer), ("$object", SqliteType.Integer), ("$field", SqliteType.Integer));
    }

    // Records the UI elements the event mentions. Call right after inserting the event, in the same transaction.
    internal void Add(long sequence, long typeId, ProfilerEvent profilerEvent, SqliteTransaction transaction)
    {
        var roles = RolesOf(typeId, profilerEvent);
        if (roles.Fields.Length == 0)
        {
            return;
        }

        insertObject.Transaction = transaction;
        updateObject.Transaction = transaction;
        insertMention.Transaction = transaction;
        var fields = profilerEvent.Fields;
        if (roles.TypeField >= 0 && Address(fields[roles.ObjectField].Value) is var entered && entered != 0)
        {
            ObserveType(entered, fields[roles.TypeField].Value as string, roles.NameField >= 0 ? fields[roles.NameField].Value as string : null);
        }

        foreach (var (field, role) in roles.Fields)
        {
            var address = Address(fields[field].Value);
            if (address == 0)
            {
                continue;
            }

            if (elements.TryGetValue(address, out var element))
            {
                Mention(sequence, element, field);
            }
            else if (role == Role.UIElement)
            {
                Confirm(address, sequence, field);
            }
            else
            {
                Hold(address, sequence, field);
            }
        }
    }

    // Removes the mentions of trimmed events and the elements' rows left without any. Call in the trimming transaction.
    internal void Trim(long cutoff, SqliteTransaction transaction)
    {
        Sql.Execute(connection, "DELETE FROM ObjectEvents WHERE Seq < $cutoff", transaction, ("$cutoff", cutoff));
        Sql.Execute(connection, """
            DELETE FROM Objects
            WHERE NOT EXISTS (SELECT 1 FROM ObjectEvents WHERE ObjectEvents.ObjectId = Objects.ObjectId)
            """, transaction);
        foreach (var element in elements.Values)
        {
            element.Stored &= element.LastSequence >= cutoff;
        }

        var trimmed = elements.Values.Where(element => !element.Stored).ToList();
        foreach (var element in trimmed.OrderBy(element => element.LastSequence).Take(trimmed.Count - MaxTrimmedElements))
        {
            elements.Remove(element.Address);
        }

        foreach (var (address, entry) in held)
        {
            entry.Mentions.RemoveAll(mention => mention.Sequence < cutoff);
            if (entry.Mentions.Count == 0)
            {
                held.Remove(address);
            }
        }
    }

    // Which fields hold a UI element's address, from the runtime call sites (uielement.cpp, ContentControl.cpp,
    // DXamlCore.cpp, depends.cpp) and checked on recorded sessions. Shared fields also hold other DependencyObjects.
    private static Role? RoleOf(string eventName, string fieldName) => fieldName switch
    {
        "ElementId" => eventName == "PeerAssociated" ? Role.Shared : Role.UIElement,
        "ParentId" or "ChildId" or "UIElementId" or "ElementPointer" => Role.UIElement,
        "OldContentId" or "NewContentId" => Role.Shared,
        "ObjectPointer" => eventName switch
        {
            "DependencyObject_Enter" => Role.Shared,
            "FrameworkElement_ApplyTemplate" or "Control_RefreshTemplateBindings" or
                "VisualStateManager_GoToStateOptimized" or "EventManager_RaiseLoadedEvent" => Role.UIElement,
            // In other events it is a window, timer, clock or framework-layer object.
            _ => null
        },
        _ => null
    };

    private static long Address(object? value) => value switch
    {
        long address => address,
        ulong address => unchecked((long)address),
        _ => 0
    };

    private EventRoles RolesOf(long typeId, ProfilerEvent profilerEvent)
    {
        if (rolesByType.TryGetValue(typeId, out var roles))
        {
            return roles;
        }

        var names = profilerEvent.Fields.Select(field => field.Name).ToArray();
        var fields = new List<(int, Role)>();
        for (var i = 0; i < names.Length; i++)
        {
            if (RoleOf(profilerEvent.Name, names[i]) is { } role)
            {
                fields.Add((i, role));
            }
        }

        // Only the start form of DependencyObject_Enter carries the type and name.
        var entered = profilerEvent.Name == "DependencyObject_Enter" && Array.IndexOf(names, "Type") >= 0;
        roles = new(fields.ToArray(), Array.IndexOf(names, "ObjectPointer"), entered ? Array.IndexOf(names, "Type") : -1,
            entered ? Array.IndexOf(names, "Name") : -1);
        rolesByType[typeId] = roles;
        return roles;
    }

    private void ObserveType(long address, string? type, string? name)
    {
        if (string.IsNullOrEmpty(type))
        {
            return;
        }

        type = typeNames.TryGetValue(type, out var known) ? known : typeNames[type] = type;
        name = string.IsNullOrEmpty(name) ? null : name;
        if (elements.TryGetValue(address, out var element))
        {
            if (element.Type is not null && element.Type != type)
            {
                // The memory was reused: the element's history ends here and a new object starts.
                elements.Remove(address);
                held[address] = new Held { Type = type, Name = name };
                return;
            }

            if (element.Type != type || name is not null && element.Name != name)
            {
                element.Type = type;
                element.Name = name ?? element.Name;
                if (element.Stored)
                {
                    updateObject.Parameters["$id"].Value = element.Id;
                    updateObject.Parameters["$type"].Value = element.Type;
                    updateObject.Parameters["$name"].Value = (object?)element.Name ?? DBNull.Value;
                    updateObject.ExecuteNonQuery();
                }
            }

            return;
        }

        if (held.TryGetValue(address, out var entry) && (entry.Type is null || entry.Type == type))
        {
            entry.Type = type;
            entry.Name = name ?? entry.Name;
            return;
        }

        // A first sighting, or a different type replacing an object that was never a UI element.
        held[address] = new Held { Type = type, Name = name };
    }

    private void Confirm(long address, long sequence, int field)
    {
        held.Remove(address, out var entry);
        var element = new Element(++elementCount, address, entry?.Type, entry?.Name);
        elements[address] = element;
        if (entry is not null)
        {
            foreach (var (heldSequence, heldField) in entry.Mentions)
            {
                Mention(heldSequence, element, heldField);
            }
        }

        Mention(sequence, element, field);
    }

    private void Hold(long address, long sequence, int field)
    {
        if (!held.TryGetValue(address, out var entry))
        {
            held[address] = entry = new Held();
        }

        if (entry.Mentions.Count < MaxHeldMentions)
        {
            entry.Mentions.Add((sequence, field));
        }
    }

    // Mentions arrive in Seq order.
    private void Mention(long sequence, Element element, int field)
    {
        if (!element.Stored)
        {
            insertObject.Parameters["$id"].Value = element.Id;
            insertObject.Parameters["$address"].Value = element.Address;
            insertObject.Parameters["$type"].Value = (object?)element.Type ?? DBNull.Value;
            insertObject.Parameters["$name"].Value = (object?)element.Name ?? DBNull.Value;
            insertObject.ExecuteNonQuery();
            element.Stored = true;
        }

        insertMention.Parameters["$seq"].Value = sequence;
        insertMention.Parameters["$object"].Value = element.Id;
        insertMention.Parameters["$field"].Value = field;
        insertMention.ExecuteNonQuery();
        element.LastSequence = sequence;
    }

    public void Dispose()
    {
        insertObject.Dispose();
        updateObject.Dispose();
        insertMention.Dispose();
    }
}