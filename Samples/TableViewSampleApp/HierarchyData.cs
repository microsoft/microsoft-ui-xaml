using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;

namespace TableViewSampleApp;

// Row model for the hierarchy page: a file-system-shaped tree, which is the scenario the feature
// is shaped around (arbitrary depth, wildly uneven fan-out, and folders whose contents are not
// worth enumerating until someone opens them).
//
// Note what this model does NOT have: a Level, an IsExpanded, or a parent pointer. The tree is
// described purely by "what are this node's children" and "does it have any" - the two callbacks
// TableViewSource.WithChildren takes. Expansion state lives in the control, so the same model can
// be shown in two TableViews expanded differently.
public sealed class Node : INotifyPropertyChanged
{
    private string _name = "";
    private ObservableCollection<Node>? _children;

    public string Name { get => _name; set { if (_name != value) { _name = value; OnPropertyChanged(nameof(Name)); } } }

    public string Kind { get; init; } = "";
    public string Owner { get; init; } = "";
    public long SizeBytes { get; init; }
    public DateTimeOffset Modified { get; init; }

    // Formatted for display. A folder shows the rolled-up size of everything beneath it, which is
    // computed eagerly here only because the sample's tree is tiny; a real app would not walk a
    // lazily-loaded subtree to render a column.
    public string Size => SizeBytes <= 0 ? "" : FormatSize(SizeBytes);

    public string ModifiedText => Modified == default ? "" : Modified.ToString("yyyy-MM-dd");

    // True when this node can be expanded, answerable WITHOUT materializing Children. That is the
    // whole point of the HasChildren predicate: a folder reports true from its own metadata, so a
    // collapsed tree never pays to enumerate anything.
    public bool IsFolder { get; init; }

    // Deliberately lazy. A node that has never been expanded has never allocated its child
    // collection, and LoadCount below proves the projection really is only descending into
    // expanded nodes rather than walking the whole tree.
    public ObservableCollection<Node> Children
    {
        get
        {
            if (_children is null)
            {
                _children = new ObservableCollection<Node>(_childFactory?.Invoke() ?? new List<Node>());
                LoadCount++;
            }
            return _children;
        }
    }

    // How many nodes have materialized their children so far, across the whole tree. The page
    // shows this so the lazy walk is observable rather than merely claimed.
    public static int LoadCount { get; private set; }

    public static void ResetLoadCount() => LoadCount = 0;

    private readonly Func<List<Node>>? _childFactory;

    public Node(string name, string kind, string owner, long sizeBytes, DateTimeOffset modified, bool isFolder = false, Func<List<Node>>? children = null)
    {
        _name = name;
        Kind = kind;
        Owner = owner;
        SizeBytes = sizeBytes;
        Modified = modified;
        IsFolder = isFolder;
        _childFactory = children;
    }

    public event PropertyChangedEventHandler? PropertyChanged;

    private void OnPropertyChanged(string propertyName)
        => PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));

    private static string FormatSize(long bytes)
    {
        string[] units = { "B", "KB", "MB", "GB" };
        double value = bytes;
        int unit = 0;
        while (value >= 1024 && unit < units.Length - 1)
        {
            value /= 1024;
            unit++;
        }
        return unit == 0 ? $"{bytes} B" : $"{value:0.#} {units[unit]}";
    }
}

internal static class HierarchyData
{
    private static readonly DateTimeOffset s_base = new(2025, 3, 1, 0, 0, 0, TimeSpan.Zero);

    private static Node Folder(string name, string owner, int dayOffset, Func<List<Node>> children)
        => new(name, "Folder", owner, 0, s_base.AddDays(dayOffset), isFolder: true, children: children);

    private static Node File(string name, string owner, long size, int dayOffset)
        => new(name, System.IO.Path.GetExtension(name).TrimStart('.').ToUpperInvariant(), owner, size, s_base.AddDays(dayOffset));

    // Roots. Deliberately uneven: one deep branch (src), one wide-but-shallow branch (assets), one
    // empty folder, and loose files at the root so leaves and branches interleave at every level.
    public static List<Node> Make()
    {
        Node.ResetLoadCount();

        return new List<Node>
        {
            Folder("src", "core-team", 2, () => new List<Node>
            {
                Folder("controls", "core-team", 4, () => new List<Node>
                {
                    Folder("TableView", "tabular-team", 6, () => new List<Node>
                    {
                        File("TableView.cpp", "tabular-team", 412_880, 7),
                        File("TableView.h", "tabular-team", 38_112, 7),
                        File("TableView.idl", "tabular-team", 41_006, 5),
                        File("TableView.xaml", "tabular-team", 27_540, 8),
                        Folder("Automation", "a11y-team", 9, () => new List<Node>
                        {
                            File("TableViewAutomationPeer.cpp", "a11y-team", 22_014, 9),
                            File("TableViewRowAutomationPeer.cpp", "a11y-team", 5_536, 9),
                        }),
                    }),
                    Folder("TreeView", "core-team", 3, () => new List<Node>
                    {
                        File("TreeView.cpp", "core-team", 96_220, 3),
                        File("TreeViewItem.cpp", "core-team", 61_004, 3),
                    }),
                    // An empty folder: expandable per IsFolder, but yields nothing when opened.
                    // The chevron disappears on expand, which is the correct behaviour for a
                    // source that only learns the truth by enumerating.
                    Folder("Experimental", "core-team", 1, () => new List<Node>()),
                }),
                File("pch.h", "core-team", 1_204, 1),
            }),

            Folder("assets", "design-team", 10, () => new List<Node>
            {
                File("icon-16.png", "design-team", 1_022, 10),
                File("icon-32.png", "design-team", 2_310, 10),
                File("icon-48.png", "design-team", 4_180, 10),
                File("icon-256.png", "design-team", 38_902, 11),
                File("splash.png", "design-team", 1_204_880, 12),
            }),

            Folder("docs", "docs-team", 14, () => new List<Node>
            {
                File("README.md", "docs-team", 8_140, 14),
                Folder("design-notes", "docs-team", 15, () => new List<Node>
                {
                    File("Hierarchical-Data-Rows-Design.md", "tabular-team", 51_220, 16),
                    File("Shaping-Design.md", "tabular-team", 44_980, 13),
                }),
            }),

            File("LICENSE", "legal", 1_090, 0),
            File("version.txt", "build", 24, 17),
        };
    }
}
