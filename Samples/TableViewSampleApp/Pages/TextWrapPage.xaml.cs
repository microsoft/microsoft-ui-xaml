// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Globalization;
using Microsoft.UI.Dispatching;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Controls.Tabular;
using Microsoft.UI.Xaml.Media;
using TableViewSampleApp.Controls;
using TableViewSampleApp.Data;
using TableViewSampleApp.Helpers;
using TableViewSampleApp.Models;

namespace TableViewSampleApp.Pages;

/// <summary>
/// Text wrap + row height: a wrapping TextBlock in a TableViewTemplateColumn, with TextWrapping,
/// MaxLines and the column width driven from the rail, and the resulting distinct row heights
/// measured from the realized TableViewRow elements.
/// </summary>
public sealed partial class TextWrapPage : SamplePageBase
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
    private int _maxLines;
    private int _nextSentence;
    private bool _measureQueued;

    public TextWrapPage()
    {
        // <snippet>
        // The cell template binds this static state; reset it so a new visit starts from the
        // defaults the rail shows.
        WrapState.TextWrapping = TextWrapping.Wrap;
        WrapState.TextTrimming = TextTrimming.None;
        WrapState.MaxLines = 0;
        // </snippet>

        foreach (var person in People)
        {
            _originalBios[person] = person.Bio;
        }

        // <snippet>
        Source = TableViewSource.From(People);     // created once; reshaped in place, never rebuilt
        InitializeComponent();
        // </snippet>

        InitializeSample(Status, Shaping.Attach(PeopleTable, Source));
        TrackLifetime(QueueRowMeasure);
    }

    // <snippet>
    /// <summary>Wrapping, trimming and line limit shared by every Bio cell.</summary>
    public static TextWrapState WrapState { get; } = new();
    // </snippet>

    public ObservableCollection<Person> People { get; } = PersonData.Take(40);

    public TableViewSource Source { get; }

    // <snippet>
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
    // </snippet>

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
        EnqueueIfLoaded(() => scroller.ChangeView(null, scroller.ScrollableHeight, null, disableAnimation: true), DispatcherQueuePriority.Low);
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

    // Grouping changes which rows are realized and where: measure them again.
    protected override void OnShapingApplied(ShapingAppliedEventArgs e) => QueueRowMeasure();

    // Expand all / Collapse all change which rows are realized: measure them again.
    protected override void OnShapingAction(ShapingActionEventArgs e) => QueueRowMeasure();

    private void OnBioSizeChanged(object sender, SizeChangedEventArgs e) => QueueRowMeasure();

    // <snippet>
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
    // </snippet>

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
}
