// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Globalization;
using System.Linq;
using System.Runtime.CompilerServices;
using Microsoft.UI.Dispatching;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Controls.Tabular;
using Microsoft.UI.Xaml.Media;
using TableViewSampleApp.Data;
using TableViewSampleApp.Helpers;
using TableViewSampleApp.Models;

namespace TableViewSampleApp.Pages;

/// <summary>
/// Text wrap + row height: a wrapping TextBlock in a TableViewTemplateColumn, with TextWrapping,
/// MaxLines and the column width driven from the rail, and the resulting distinct row heights
/// measured from the realized TableViewRow elements.
/// </summary>
public sealed partial class TextWrapPage : Page
{
    // Sentences the "Lengthen selected bio" action appends, in turn.
    private static readonly string[] s_extraSentences =
    {
        "Recently took over the on-call rotation for the payments service and is rewriting its runbook.",
        "Runs a monthly brown-bag session on accessibility testing with screen readers.",
        "Is currently mentoring two new hires through their first release.",
        "Spends Fridays reviewing customer feedback and turning the patterns into backlog items.",
    };

    private readonly Dictionary<Person, string> _originalBios = new();
    private TableViewSource? _source;          // created ONCE; reshaped in place, never rebuilt
    private string _appliedMode = "flat";      // written only after GroupBy/ClearGroupBy returns
    private string _appliedKey = "Department";
    private int _maxLines;
    private int _nextSentence;
    private bool _measureQueued;

    public TextWrapPage()
    {
        // The cell template binds this static state; reset it so a new visit starts from the
        // defaults the rail shows.
        WrapState.TextWrapping = TextWrapping.Wrap;
        WrapState.TextTrimming = TextTrimming.None;
        WrapState.MaxLines = 0;

        foreach (var person in People)
        {
            _originalBios[person] = person.Bio;
        }

        _source = TableViewSource.From(People);
        InitializeComponent();
        Loaded += OnPageLoaded;
        RefreshReadouts();
    }

    /// <summary>Wrapping, trimming and line limit shared by every Bio cell.</summary>
    public static TextWrapState WrapState { get; } = new();

    public ObservableCollection<Person> People { get; } = PersonData.Take(40);

    public TableViewSource? Source => _source;

    private void OnPageLoaded(object sender, RoutedEventArgs e) => QueueRowMeasure();

    // ---- Wrapping, max lines and width --------------------------------------------------

    private void OnWrapToggled(object sender, RoutedEventArgs e)
    {
        // Toggled fires during InitializeComponent (IsOn="True"); the ctor already set WrapState.
        if (!IsLoaded || WrapToggle is null || MaxLinesSelector is null)
        {
            return;
        }

        ApplyTextState();
        SetLastAction(WrapToggle.IsOn ? "TextWrapping -> Wrap" : "TextWrapping -> NoWrap");
    }

    private void OnMaxLinesChanged(object sender, SelectionChangedEventArgs e)
    {
        // Fires during InitializeComponent (SelectedIndex="0").
        if (!IsLoaded || MaxLinesSelector is null || WrapToggle is null)
        {
            return;
        }

        _maxLines = int.TryParse(SampleShaping.SelectedTag(MaxLinesSelector, "0"), NumberStyles.Integer, CultureInfo.CurrentCulture, out var lines) ? lines : 0;
        ApplyTextState();
        SetLastAction(_maxLines == 0
            ? "MaxLines -> unlimited"
            : string.Format(CultureInfo.CurrentCulture, "MaxLines -> {0}", _maxLines));
    }

    // The one writer of WrapState, so the toggle and the max-lines picker never overwrite each
    // other. Every Bio cell binds WrapState, which is what makes the setting survive recycling.
    private void ApplyTextState()
    {
        var wrapOn = WrapToggle.IsOn;
        WrapState.TextWrapping = wrapOn ? TextWrapping.Wrap : TextWrapping.NoWrap;

        // TextBlock.MaxLines only clamps text that wraps, so it is not applied for NoWrap and the
        // picker is disabled to say so.
        WrapState.MaxLines = wrapOn ? _maxLines : 0;
        WrapState.TextTrimming = !wrapOn || _maxLines > 0 ? TextTrimming.CharacterEllipsis : TextTrimming.None;
        MaxLinesSelector.IsEnabled = wrapOn;
        QueueRowMeasure();
    }

    private void OnBioWidthChanged(object sender, RangeBaseValueChangedEventArgs e)
    {
        // Fires during InitializeComponent when Value is set; the XAML Width already matches.
        if (!IsLoaded || BioColumn is null || BioWidthText is null)
        {
            return;
        }

        BioColumn.Width = new GridLength(e.NewValue);
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Bio column Width -> {0:N0} px", e.NewValue));
        QueueRowMeasure();
    }

    // ---- Actions ------------------------------------------------------------------------

    private void OnLengthenBioClick(object sender, RoutedEventArgs e)
    {
        if (PeopleTable.SelectedItem is not Person person)
        {
            SetLastAction("No row selected.");
            return;
        }

        person.Bio = person.Bio + " " + s_extraSentences[_nextSentence++ % s_extraSentences.Length];
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Lengthened {0}'s bio to {1:N0} sentences", person.FullName, SentenceCount(person.Bio)));
    }

    private void OnShortenBioClick(object sender, RoutedEventArgs e)
    {
        if (PeopleTable.SelectedItem is not Person person)
        {
            SetLastAction("No row selected.");
            return;
        }

        var shorter = FirstSentence(person.Bio);
        if (shorter == person.Bio)
        {
            SetLastAction(string.Format(CultureInfo.CurrentCulture, "{0}'s bio is already one sentence", person.FullName));
            return;
        }

        person.Bio = shorter;
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Shortened {0}'s bio to one sentence", person.FullName));
    }

    private void OnOneLineBiosClick(object sender, RoutedEventArgs e)
    {
        foreach (var person in People)
        {
            person.Bio = FirstSentence(person.Bio);
        }

        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Cut all {0:N0} bios to their first sentence", People.Count));
    }

    private void OnRestoreBiosClick(object sender, RoutedEventArgs e)
    {
        foreach (var person in People)
        {
            if (_originalBios.TryGetValue(person, out var bio))
            {
                person.Bio = bio;
            }
        }

        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Restored all {0:N0} bios", People.Count));
    }

    private void OnScrollToLastClick(object sender, RoutedEventArgs e)
    {
        // TableView has no scroll-into-view API in this release, so scroll its body ScrollViewer.
        if (FindDescendant<ScrollViewer>(PeopleTable, "PART_BodyScroller") is not { } scroller)
        {
            SetLastAction("The table body is not realized yet.");
            return;
        }

        scroller.ChangeView(null, scroller.ScrollableHeight, null, disableAnimation: true);

        // Rows realized near the end can change the extent once they are measured, so scroll a
        // second time after that layout pass.
        DispatcherQueue.TryEnqueue(DispatcherQueuePriority.Low, () =>
        {
            if (IsLoaded)
            {
                scroller.ChangeView(null, scroller.ScrollableHeight, null, disableAnimation: true);
            }
        });
        SetLastAction("Scrolled the table body to the last row");
    }

    private static string FirstSentence(string text)
    {
        var end = text.IndexOf(". ", StringComparison.Ordinal);
        return end < 0 ? text : text.Substring(0, end + 1);
    }

    private static int SentenceCount(string text) =>
        text.Split(". ", StringSplitOptions.RemoveEmptyEntries).Length;

    // ---- Row height measurement ---------------------------------------------------------

    private void OnBioSizeChanged(object sender, SizeChangedEventArgs e) => QueueRowMeasure();

    // Coalesced: many cells change size in one layout pass, but the rows are measured once,
    // after layout, at low priority.
    private void QueueRowMeasure()
    {
        if (_measureQueued)
        {
            return;
        }

        _measureQueued = true;
        DispatcherQueue.TryEnqueue(DispatcherQueuePriority.Low, () =>
        {
            _measureQueued = false;
            if (IsLoaded)
            {
                RefreshRowHeights();
            }
        });
    }

    private void RefreshRowHeights()
    {
        if (RowHeightsText is null || PeopleTable is null)
        {
            return;
        }

        var heights = new SortedSet<double>();
        CollectRowHeights(PeopleTable, heights);
        RowHeightsText.Text = heights.Count == 0
            ? "(no rows realized)"
            : string.Format(CultureInfo.CurrentCulture, "{0:N0} distinct: {1} px", heights.Count,
                string.Join(", ", heights.Select(h => h.ToString("N0", CultureInfo.CurrentCulture))));
    }

    private static void CollectRowHeights(DependencyObject node, SortedSet<double> heights)
    {
        if (node is TableViewRow row)
        {
            if (row.ActualHeight > 0 && row.Visibility == Visibility.Visible)
            {
                heights.Add(Math.Round(row.ActualHeight));
            }

            return;
        }

        var count = VisualTreeHelper.GetChildrenCount(node);
        for (var i = 0; i < count; i++)
        {
            CollectRowHeights(VisualTreeHelper.GetChild(node, i), heights);
        }
    }

    private static T? FindDescendant<T>(DependencyObject node, string name) where T : FrameworkElement
    {
        var count = VisualTreeHelper.GetChildrenCount(node);
        for (var i = 0; i < count; i++)
        {
            var child = VisualTreeHelper.GetChild(node, i);
            if (child is T match && match.Name == name)
            {
                return match;
            }

            if (FindDescendant<T>(child, name) is { } found)
            {
                return found;
            }
        }

        return null;
    }

    private void RefreshReadouts()
    {
        if (RowsText is null || WrappingText is null || BioWidthText is null || MaxLinesText is null || BioColumn is null)
        {
            return;
        }

        RowsText.Text = SampleShaping.RowCountText(People.Count);
        WrappingText.Text = WrapState.TextWrapping.ToString();
        BioWidthText.Text = string.Format(CultureInfo.CurrentCulture, "{0:N0} px", BioColumn.Width.Value);
        MaxLinesText.Text = WrapState.TextWrapping != TextWrapping.Wrap
            ? "Not applied (NoWrap)"
            : _maxLines == 0 ? "Unlimited" : _maxLines.ToString(CultureInfo.CurrentCulture);
    }

    #region Sample scaffolding (generic; see FIX-PLAN §6)

    private void OnShapingModeChanged(object sender, SelectionChangedEventArgs e) => ApplyShaping(announce: true);

    private void OnGroupKeyChanged(object sender, SelectionChangedEventArgs e) => ApplyShaping(announce: true);

    private void ApplyShaping(bool announce)
    {
        // Fires during InitializeComponent (each selector's SelectedIndex="0"), before the
        // later-declared elements exist. Guard every element this path touches.
        if (_source is null || PeopleTable is null || ShapingModeSelector is null || GroupKeySelector is null
            || ExpandAllButton is null || CollapseAllButton is null || ShapingModeText is null)
        {
            return;
        }

        var mode = SampleShaping.SelectedTag(ShapingModeSelector, "flat");
        var key = SampleShaping.SelectedTag(GroupKeySelector, "Department");
        var selected = PeopleTable.SelectedItem;

        switch (mode)
        {
            case "grouped":
                // The key selector receives the ROW; the identity selector receives the KEY.
                _source.GroupBy(item => SampleShaping.KeyOf(item as Person, key), SampleShaping.GroupIdentity);
                break;
            // case "hierarchy":
            // case "groupedHierarchy":
            //     Hierarchical (tree) rows are not available in this release, so the two matching
            //     ComboBoxItems ship disabled. TableViewSource and TableView have no hierarchy
            //     member today. When hierarchy ships, apply it to this same source here, composed
            //     with the GroupBy stage above rather than replacing it, and set _appliedMode only
            //     after the call returns.
            default:
                _source.ClearGroupBy();
                mode = "flat";
                break;
        }

        _appliedMode = mode;
        _appliedKey = key;

        // Re-applying GroupBy can drop the selection when the selected row changed group.
        SampleShaping.Reselect(PeopleTable, selected, People.Count * 2, RefreshReadouts);
        UpdateShapingGating();
        QueueRowMeasure();
        if (announce)
        {
            SetLastAction(mode == "grouped"
                ? string.Format(CultureInfo.CurrentCulture, "Shaping -> Grouped by {0}", SampleShaping.Label(GroupKeySelector))
                : "Shaping -> Flat");
        }
    }

    // Call after ANY write to the grouped-on property: from an action or from an in-cell edit.
    // This page's actions only change Bio, which is never a group key.
    private void ReapplyIfGroupedOn(string? propertyName)
    {
        if (_appliedMode == "grouped" && propertyName == _appliedKey)
        {
            ApplyShaping(announce: false);
        }
    }

    private void UpdateShapingGating()
    {
        var grouped = _appliedMode == "grouped";
        GroupKeySelector.IsEnabled = grouped;
        ExpandAllButton.IsEnabled = grouped;
        CollapseAllButton.IsEnabled = grouped;
        ShapingModeText.Text = SampleShaping.ShapingText(grouped, GroupKeySelector);
    }

    private void OnExpandAllClick(object sender, RoutedEventArgs e)
    {
        PeopleTable.ExpandAllGroups();
        QueueRowMeasure();
        SetLastAction("Expanded all groups");
    }

    private void OnCollapseAllClick(object sender, RoutedEventArgs e)
    {
        PeopleTable.CollapseAllGroups();
        QueueRowMeasure();
        SetLastAction("Collapsed all groups");
    }

    // The only writer of LastActionText.
    private void SetLastAction(string message)
    {
        if (LastActionText is not null)
        {
            LastActionText.Text = message;
        }

        RefreshReadouts();
    }

    #endregion
}

/// <summary>
/// Wrapping state shared by every Bio cell through x:Bind. Raises PropertyChanged so realized
/// cells update in place when the rail changes.
/// </summary>
public sealed partial class TextWrapState : INotifyPropertyChanged
{
    private TextWrapping _textWrapping = TextWrapping.Wrap;
    private TextTrimming _textTrimming = TextTrimming.None;
    private int _maxLines;

    public TextWrapping TextWrapping
    {
        get => _textWrapping;
        set => Set(ref _textWrapping, value);
    }

    public TextTrimming TextTrimming
    {
        get => _textTrimming;
        set => Set(ref _textTrimming, value);
    }

    public int MaxLines
    {
        get => _maxLines;
        set => Set(ref _maxLines, value);
    }

    public event PropertyChangedEventHandler? PropertyChanged;

    private void Set<T>(ref T field, T value, [CallerMemberName] string? propertyName = null)
    {
        if (Equals(field, value))
        {
            return;
        }

        field = value;
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
    }
}
