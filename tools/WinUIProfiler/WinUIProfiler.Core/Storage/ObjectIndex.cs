// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Data.Sqlite;
using WinUIProfiler.Engine.Events;

namespace WinUIProfiler.Engine.Storage;

// The UI elements and composition visuals the events mention, and for each object the events that mention it. Runs on
// the session writer's thread inside its transactions, so a mention is saved and trimmed together with its event.
//
// There are no create or destroy events. An address is known to be a UI element once it appears in a field that only
// ever holds a CUIElement* (tree, child, popup, layout, template, visual state, render and composition-link events).
// DependencyObject_Enter, PeerAssociated and ContentChanged's content also carry other DependencyObjects, so their
// mentions of an address are held until the address is known to be a UI element. DependencyObject_Enter reports the
// type each time an object enters the tree: a different type at a known address means the memory was reused by a new
// object, so a new element starts there. The new object's parent, peer and layout events can come before that, so
// the old element's mentions since it (or its last child) left the tree move to the new one. Reuse by an object of the
// same type can't be detected.
//
// The fields that hold a visual (ParentVisualId, ChildVisualId, VisualId) only ever hold an IVisual pointer, so a visual
// is known from its first mention. Its insertion (WucVisualChildInserted, WucVisualRootSet) reports its type and owner
// comp node, and the owner's CompPeerLinked label becomes its name. An insertion with a different type or owner than the
// visual's last means a new visual reused the pointer, so a new visual starts there. Visuals get their children before
// they are inserted, so as with elements the old visual's mentions since it left the tree (removed, or its parent's
// children cleared) move to the new one. Reuse by a visual of the same type and owner can't be detected. A hand-in or
// hand-off visual that its element's re-created comp node inserts again also gets a new owner, so its history splits.
internal sealed class ObjectIndex : IDisposable
{
    // Mentions held in memory per address: a UI element is confirmed within a few, other objects never are. Later ones
    // wait in the temporary table HeldMentions, so confirming an address still saves all of its mentions.
    private const int MaxHeldMentions = 8;

    // Objects whose rows were all trimmed are remembered, so a later mention keeps their ID, type and name.
    private const int MaxTrimmedObjects = 100_000;

    private enum Role : byte
    {
        UIElement,
        Shared,
        Visual
    }

    // Where an event type mentions objects, and for DependencyObject_Enter where its type and name are.
    private sealed record EventRoles((int Field, Role Role)[] Fields, int ObjectField, int TypeField, int NameField, bool LeavesTree,
        TreeRoles Tree);

    // Where an event that changes the visual tree or a comp node's label has the fields that matter; -1 where it has none.
    // Inserted is the inserted visual; Removed the removed one; Cleared the parent whose children were all removed.
    private sealed record TreeRoles(int Inserted = -1, int Parent = -1, int Type = -1, int Owner = -1, int Removed = -1,
        int Cleared = -1, int CompNode = -1, int Label = -1, bool Unlinks = false);

    private static readonly TreeRoles NoTreeRoles = new();

    private sealed class TrackedObject(long id, ObjectKind kind, long address, string? type, string? name)
    {
        public long Id { get; } = id;
        public ObjectKind Kind { get; } = kind;
        public long Address { get; } = address;
        public string? Type { get; set; } = type;
        public string? Name { get; set; } = name;
        public long LastSequence { get; set; }
        // When it last left the tree: an element's last ElementLeftTree that named it, as the element leaving or as a
        // leaving child's parent; a visual's removal since its last insertion. 0 if none.
        public long LeftSequence { get; set; }
        // A visual's owner comp node, from its last insertion (kept after removal, to tell a reused pointer apart), and its
        // parent visual while it is in the tree; 0 when unknown.
        public long Owner { get; set; }
        public long Parent { get; set; }
        // True while it waits in ObjectIndex.unlabeled for its owner's label.
        public bool AwaitingLabel { get; set; }
        // False once trimming deleted its row; the next mention writes it again.
        public bool Stored { get; set; }
    }

    private sealed class Held
    {
        public string? Type { get; set; }
        public string? Name { get; set; }
        public List<(long Sequence, int Field)> Mentions { get; } = new(2);
        // True once mentions past MaxHeldMentions went to HeldMentions.
        public bool Overflow { get; set; }
    }

    private readonly SqliteConnection connection;
    private readonly SqliteCommand insertObject;
    private readonly SqliteCommand updateObject;
    private readonly SqliteCommand insertMention;
    private readonly SqliteCommand insertHeld;
    private readonly SqliteCommand saveHeld;
    private readonly SqliteCommand deleteHeld;
    private readonly Dictionary<long, EventRoles> rolesByType = [];
    private readonly Dictionary<long, TrackedObject> elements = [];
    private readonly Dictionary<long, Held> held = [];
    private readonly Dictionary<long, TrackedObject> visuals = [];
    // The visuals in the tree under each parent visual's address, so clearing a parent's children knows them.
    private readonly Dictionary<long, HashSet<long>> children = [];
    // Each linked comp node's element label (x:Name, or the class name).
    private readonly Dictionary<long, string> labels = [];
    // Visuals whose owner comp node was not linked yet when they were inserted, by owner; they get its label once it is.
    private readonly Dictionary<long, List<TrackedObject>> unlabeled = [];
    private readonly Dictionary<string, string> typeNames = [];
    private long objectCount;

    internal ObjectIndex(SqliteConnection connection)
    {
        this.connection = connection;
        insertObject = Sql.Prepare(connection, """
            INSERT INTO Objects (ObjectId, Kind, Address, Type, Name) VALUES ($id, $kind, $address, $type, $name)
            """, ("$id", SqliteType.Integer), ("$kind", SqliteType.Text), ("$address", SqliteType.Integer), ("$type", SqliteType.Text),
            ("$name", SqliteType.Text));
        updateObject = Sql.Prepare(connection, "UPDATE Objects SET Type = $type, Name = $name WHERE ObjectId = $id",
            ("$id", SqliteType.Integer), ("$type", SqliteType.Text), ("$name", SqliteType.Text));
        insertMention = Sql.Prepare(connection, "INSERT INTO ObjectEvents (Seq, ObjectId, Field) VALUES ($seq, $object, $field)",
            ("$seq", SqliteType.Integer), ("$object", SqliteType.Integer), ("$field", SqliteType.Integer));
        // Only the writer's connection sees a TEMP table, and it is not part of the session file.
        Sql.Execute(connection, """
            CREATE TEMP TABLE HeldMentions (
                Address INTEGER NOT NULL,
                Seq INTEGER NOT NULL,
                Field INTEGER NOT NULL,
                PRIMARY KEY (Address, Seq, Field)) WITHOUT ROWID
            """);
        insertHeld = Sql.Prepare(connection, "INSERT INTO temp.HeldMentions (Address, Seq, Field) VALUES ($address, $seq, $field)",
            ("$address", SqliteType.Integer), ("$seq", SqliteType.Integer), ("$field", SqliteType.Integer));
        saveHeld = Sql.Prepare(connection, """
            INSERT INTO ObjectEvents (Seq, ObjectId, Field) SELECT Seq, $object, Field FROM temp.HeldMentions WHERE Address = $address
            """, ("$address", SqliteType.Integer), ("$object", SqliteType.Integer));
        deleteHeld = Sql.Prepare(connection, "DELETE FROM temp.HeldMentions WHERE Address = $address", ("$address", SqliteType.Integer));
    }

    // Records the objects the event mentions. Call right after inserting the event, in the same transaction.
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
        insertHeld.Transaction = transaction;
        saveHeld.Transaction = transaction;
        deleteHeld.Transaction = transaction;
        var fields = profilerEvent.Fields;
        if (roles.TypeField >= 0 && Address(fields[roles.ObjectField].Value) is var entered && entered != 0)
        {
            ObserveType(entered, fields[roles.TypeField].Value as string, roles.NameField >= 0 ? fields[roles.NameField].Value as string : null,
                transaction);
        }

        var tree = roles.Tree;
        if (tree.CompNode >= 0 && Address(fields[tree.CompNode].Value) is var compNode && compNode != 0)
        {
            if (tree.Unlinks)
            {
                // A comp node is unlinked when its element leaves; a later link at the address is a new comp node.
                labels.Remove(compNode);
                if (unlabeled.Remove(compNode, out var waiting))
                {
                    waiting.ForEach(visual => visual.AwaitingLabel = false);
                }
            }
            else if (fields[tree.Label].Value is string { Length: > 0 } label)
            {
                Link(compNode, label);
            }
        }

        if (tree.Inserted >= 0 && Address(fields[tree.Inserted].Value) is var inserted && inserted != 0)
        {
            ObserveInsertion(inserted, tree.Parent >= 0 ? Address(fields[tree.Parent].Value) : 0, fields[tree.Type].Value as string,
                Address(fields[tree.Owner].Value), transaction);
        }

        foreach (var (field, role) in roles.Fields)
        {
            var address = Address(fields[field].Value);
            if (address == 0)
            {
                continue;
            }

            Record(address, sequence, field, role);
            if (roles.LeavesTree)
            {
                // Both of ElementLeftTree's fields are UI-element-only, so the element is known now.
                elements[address].LeftSequence = sequence;
            }
        }

        if (tree.Removed >= 0 && Address(fields[tree.Removed].Value) is var removed && removed != 0)
        {
            // Recorded above, so the visual is known.
            Leave(visuals[removed], sequence);
        }

        if (tree.Cleared >= 0 && Address(fields[tree.Cleared].Value) is var parent && parent != 0 &&
            children.Remove(parent, out var cleared))
        {
            // The event names only the parent, so each child's history gets it through the parent's field.
            foreach (var child in cleared)
            {
                if (visuals.TryGetValue(child, out var visual))
                {
                    Mention(sequence, visual, -1 - tree.Cleared);
                    visual.LeftSequence = sequence;
                    visual.Parent = 0;
                }
            }
        }
    }

    // Removes the mentions of trimmed events and the objects' rows left without any. Call in the trimming transaction.
    internal void Trim(long cutoff, SqliteTransaction transaction)
    {
        Sql.Execute(connection, "DELETE FROM ObjectEvents WHERE Seq < $cutoff", transaction, ("$cutoff", cutoff));
        Sql.Execute(connection, """
            DELETE FROM Objects
            WHERE NOT EXISTS (SELECT 1 FROM ObjectEvents WHERE ObjectEvents.ObjectId = Objects.ObjectId)
            """, transaction);
        Forget(elements, cutoff);
        foreach (var visual in Forget(visuals, cutoff))
        {
            // Its children set stays: the visuals in it are still in the tree under the address.
            Detach(visual);
        }

        foreach (var (owner, waiting) in unlabeled)
        {
            waiting.RemoveAll(visual => !visuals.TryGetValue(visual.Address, out var current) || current != visual);
            if (waiting.Count == 0)
            {
                unlabeled.Remove(owner);
            }
        }

        Sql.Execute(connection, "DELETE FROM temp.HeldMentions WHERE Seq < $cutoff", transaction, ("$cutoff", cutoff));
        foreach (var (address, entry) in held)
        {
            entry.Mentions.RemoveAll(mention => mention.Sequence < cutoff);
            if (entry.Mentions.Count == 0 && !entry.Overflow)
            {
                held.Remove(address);
            }
        }
    }

    // Marks the objects whose rows were all trimmed, and forgets the oldest of them past MaxTrimmedObjects.
    private static List<TrackedObject> Forget(Dictionary<long, TrackedObject> objects, long cutoff)
    {
        foreach (var tracked in objects.Values)
        {
            tracked.Stored &= tracked.LastSequence >= cutoff;
        }

        var trimmed = objects.Values.Where(tracked => !tracked.Stored).ToList();
        var forgotten = trimmed.OrderBy(tracked => tracked.LastSequence).Take(trimmed.Count - MaxTrimmedObjects).ToList();
        foreach (var tracked in forgotten)
        {
            objects.Remove(tracked.Address);
        }

        return forgotten;
    }

    // Which fields hold a UI element's address, from the runtime call sites (uielement.cpp, ContentControl.cpp,
    // DXamlCore.cpp, depends.cpp, Popup.cpp) and checked on recorded sessions. Shared fields also hold other DependencyObjects.
    // The visual fields are those of the WUC visual and visual surface events (XamlProfilerTracing.h).
    private static Role? RoleOf(string eventName, string fieldName) => fieldName switch
    {
        "ElementId" => eventName == "PeerAssociated" ? Role.Shared : Role.UIElement,
        "ParentId" or "ChildId" or "UIElementId" or "ElementPointer" or "PopupId" => Role.UIElement,
        "OldContentId" or "NewContentId" => Role.Shared,
        "ObjectPointer" => eventName switch
        {
            "DependencyObject_Enter" => Role.Shared,
            "FrameworkElement_ApplyTemplate" or "Control_RefreshTemplateBindings" or
                "VisualStateManager_GoToStateOptimized" or "EventManager_RaiseLoadedEvent" => Role.UIElement,
            // In other events it is a window, timer, clock or framework-layer object.
            _ => null
        },
        "ParentVisualId" or "ChildVisualId" or "VisualId" => Role.Visual,
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
        int At(string name) => Array.IndexOf(names, name);
        var tree = profilerEvent.Name switch
        {
            "WucVisualChildInserted" => new TreeRoles(Inserted: At("ChildVisualId"), Parent: At("ParentVisualId"), Type: At("VisualType"),
                Owner: At("OwnerCompNodeId")),
            "WucVisualRootSet" => new TreeRoles(Inserted: At("VisualId"), Type: At("VisualType"), Owner: At("OwnerCompNodeId")),
            "WucVisualChildRemoved" => new TreeRoles(Removed: At("ChildVisualId"), Parent: At("ParentVisualId")),
            "WucVisualChildrenCleared" => new TreeRoles(Cleared: At("ParentVisualId")),
            "CompPeerLinked" => new TreeRoles(CompNode: At("CompNodeId"), Label: At("ElementLabel")),
            "CompPeerUnlinked" => new TreeRoles(CompNode: At("CompNodeId"), Unlinks: true),
            _ => NoTreeRoles
        };
        roles = new(fields.ToArray(), At("ObjectPointer"), entered ? At("Type") : -1, entered ? At("Name") : -1,
            profilerEvent.Name == "ElementLeftTree", tree);
        rolesByType[typeId] = roles;
        return roles;
    }

    private string InternType(string type) => typeNames.TryGetValue(type, out var known) ? known : typeNames[type] = type;

    // A visual's insertion: its parent visual (0 for a root), type and owner comp node.
    private void ObserveInsertion(long address, long parent, string? type, long owner, SqliteTransaction transaction)
    {
        type = string.IsNullOrEmpty(type) ? null : InternType(type);
        var label = owner != 0 && labels.TryGetValue(owner, out var known) ? known : null;
        if (visuals.TryGetValue(address, out var visual) &&
            (visual.Type is not null && type is not null && visual.Type != type || visual.Owner != 0 && owner != 0 && visual.Owner != owner))
        {
            // A new visual reused the pointer: the old one's history ends when it left the tree.
            visuals.Remove(address);
            Detach(visual);
            visuals[address] = new(++objectCount, ObjectKind.Visual, address, type, label) { Owner = owner };
            if (visual.LeftSequence > 0)
            {
                MoveToNewObject(visual, transaction);
            }

            visual = visuals[address];
        }
        else if (visual is not null)
        {
            visual.LeftSequence = 0;
            if (type is not null && visual.Type != type || owner != 0 && visual.Owner != owner || label is not null && visual.Name != label)
            {
                visual.Type = type ?? visual.Type;
                visual.Owner = owner != 0 ? owner : visual.Owner;
                visual.Name = label ?? visual.Name;
                Update(visual);
            }
        }
        else
        {
            visuals[address] = visual = new(++objectCount, ObjectKind.Visual, address, type, label) { Owner = owner };
        }

        if (visual.Name is null && visual.Owner != 0 && !visual.AwaitingLabel)
        {
            (unlabeled.TryGetValue(visual.Owner, out var waiting) ? waiting : unlabeled[visual.Owner] = []).Add(visual);
            visual.AwaitingLabel = true;
        }

        if (visual.Parent != parent)
        {
            Detach(visual);
            if (parent != 0)
            {
                (children.TryGetValue(parent, out var set) ? set : children[parent] = []).Add(address);
                visual.Parent = parent;
            }
        }
    }

    // A comp node was linked to its element. A comp node usually creates its visuals before that, so those get the label now.
    private void Link(long compNode, string label)
    {
        labels[compNode] = label;
        if (!unlabeled.Remove(compNode, out var waiting))
        {
            return;
        }

        foreach (var visual in waiting)
        {
            visual.AwaitingLabel = false;
            if (visual.Name is null && visual.Owner == compNode && visuals.TryGetValue(visual.Address, out var current) && current == visual)
            {
                visual.Name = label;
                Update(visual);
            }
        }
    }

    private void Leave(TrackedObject visual, long sequence)
    {
        visual.LeftSequence = sequence;
        Detach(visual);
    }

    private void Detach(TrackedObject visual)
    {
        if (visual.Parent != 0 && children.TryGetValue(visual.Parent, out var set))
        {
            set.Remove(visual.Address);
            if (set.Count == 0)
            {
                children.Remove(visual.Parent);
            }
        }

        visual.Parent = 0;
    }

    private void Update(TrackedObject tracked)
    {
        if (tracked.Stored)
        {
            updateObject.Parameters["$id"].Value = tracked.Id;
            updateObject.Parameters["$type"].Value = (object?)tracked.Type ?? DBNull.Value;
            updateObject.Parameters["$name"].Value = (object?)tracked.Name ?? DBNull.Value;
            updateObject.ExecuteNonQuery();
        }
    }

    private void ObserveType(long address, string? type, string? name, SqliteTransaction transaction)
    {
        if (string.IsNullOrEmpty(type))
        {
            return;
        }

        type = InternType(type);
        name = string.IsNullOrEmpty(name) ? null : name;
        if (elements.TryGetValue(address, out var element))
        {
            if (element.Type is not null && element.Type != type)
            {
                // The memory was reused: the element's history ends when it left the tree and a new object starts.
                elements.Remove(address);
                StartHolding(address, type, name);
                if (element.LeftSequence > 0)
                {
                    MoveToNewObject(element, transaction);
                }

                return;
            }

            if (element.Type != type || name is not null && element.Name != name)
            {
                element.Type = type;
                element.Name = name ?? element.Name;
                Update(element);
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
        StartHolding(address, type, name);
    }

    // Holds the mentions of a new object at the address; those held for the object there before are dropped.
    private void StartHolding(long address, string type, string? name)
    {
        if (held.TryGetValue(address, out var previous) && previous.Overflow)
        {
            deleteHeld.Parameters["$address"].Value = address;
            deleteHeld.ExecuteNonQuery();
        }

        held[address] = new Held { Type = type, Name = name };
    }

    // Takes the stored mentions after the object last left the tree from it and records them again for the new object at
    // its address. Rare, so they are read back rather than kept in memory.
    private void MoveToNewObject(TrackedObject tracked, SqliteTransaction transaction)
    {
        var moved = new List<(long Sequence, int Field, long TypeId)>();
        using (var select = connection.CreateCommand())
        {
            select.Transaction = transaction;
            select.CommandText = """
                SELECT r.Seq, r.Field, e.TypeId FROM ObjectEvents r JOIN Events e ON e.Seq = r.Seq
                WHERE r.ObjectId = $id AND r.Seq > $left
                ORDER BY r.Seq, r.Field
                """;
            select.Parameters.AddWithValue("$id", tracked.Id);
            select.Parameters.AddWithValue("$left", tracked.LeftSequence);
            using var reader = select.ExecuteReader();
            while (reader.Read())
            {
                moved.Add((reader.GetInt64(0), reader.GetInt32(1), reader.GetInt64(2)));
            }
        }

        if (moved.Count == 0)
        {
            return;
        }

        Sql.Execute(connection, "DELETE FROM ObjectEvents WHERE ObjectId = $id AND Seq > $left", transaction,
            ("$id", tracked.Id), ("$left", tracked.LeftSequence));
        // Its leaving event may already be trimmed.
        Sql.Execute(connection, "DELETE FROM Objects WHERE ObjectId = $id AND NOT EXISTS (SELECT 1 FROM ObjectEvents WHERE ObjectId = $id)",
            transaction, ("$id", tracked.Id));
        foreach (var (sequence, field, typeId) in moved)
        {
            // A negative field is a cleared child's mention, which only visuals have.
            Record(tracked.Address, sequence, field,
                field < 0 ? Role.Visual : Array.Find(rolesByType[typeId].Fields, item => item.Field == field).Role);
        }
    }

    private void Record(long address, long sequence, int field, Role role)
    {
        if (role == Role.Visual)
        {
            if (!visuals.TryGetValue(address, out var visual))
            {
                visuals[address] = visual = new(++objectCount, ObjectKind.Visual, address, null, null);
            }

            Mention(sequence, visual, field);
        }
        else if (elements.TryGetValue(address, out var element))
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

    private void Confirm(long address, long sequence, int field)
    {
        held.Remove(address, out var entry);
        var element = new TrackedObject(++objectCount, ObjectKind.Element, address, entry?.Type, entry?.Name);
        elements[address] = element;
        if (entry is not null)
        {
            foreach (var (heldSequence, heldField) in entry.Mentions)
            {
                Mention(heldSequence, element, heldField);
            }
        }

        Mention(sequence, element, field);
        if (entry?.Overflow == true)
        {
            // Earlier than this mention, so the element's last sequence stays this one.
            saveHeld.Parameters["$address"].Value = address;
            saveHeld.Parameters["$object"].Value = element.Id;
            saveHeld.ExecuteNonQuery();
            deleteHeld.Parameters["$address"].Value = address;
            deleteHeld.ExecuteNonQuery();
        }
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
            return;
        }

        insertHeld.Parameters["$address"].Value = address;
        insertHeld.Parameters["$seq"].Value = sequence;
        insertHeld.Parameters["$field"].Value = field;
        insertHeld.ExecuteNonQuery();
        entry.Overflow = true;
    }

    // Mentions arrive in Seq order.
    private void Mention(long sequence, TrackedObject tracked, int field)
    {
        if (!tracked.Stored)
        {
            insertObject.Parameters["$id"].Value = tracked.Id;
            insertObject.Parameters["$kind"].Value = tracked.Kind.ToString();
            insertObject.Parameters["$address"].Value = tracked.Address;
            insertObject.Parameters["$type"].Value = (object?)tracked.Type ?? DBNull.Value;
            insertObject.Parameters["$name"].Value = (object?)tracked.Name ?? DBNull.Value;
            insertObject.ExecuteNonQuery();
            tracked.Stored = true;
        }

        insertMention.Parameters["$seq"].Value = sequence;
        insertMention.Parameters["$object"].Value = tracked.Id;
        insertMention.Parameters["$field"].Value = field;
        insertMention.ExecuteNonQuery();
        tracked.LastSequence = sequence;
    }

    public void Dispose()
    {
        insertObject.Dispose();
        updateObject.Dispose();
        insertMention.Dispose();
        insertHeld.Dispose();
        saveHeld.Dispose();
        deleteHeld.Dispose();
    }
}