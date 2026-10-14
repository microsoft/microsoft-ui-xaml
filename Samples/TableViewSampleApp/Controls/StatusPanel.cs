// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Generic;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Markup;

namespace TableViewSampleApp.Controls;

// Only Last action is a live region: every action rewrites all readouts, so making each one live
// would queue several announcements per click.
[ContentProperty(Name = nameof(Readouts))]
public partial class StatusPanel : RailSection
{
    private readonly Grid _grid;
    private readonly List<(TextBlock Label, FrameworkElement Value)> _rows = new();
    private readonly TextBlock _rowsLabel;
    private readonly TextBlock _rowsValue;
    private readonly TextBlock _shapingLabel;
    private readonly TextBlock _shapingValue;
    private readonly TextBlock _lastActionLabel;
    private readonly TextBlock _lastActionValue;

    public StatusPanel()
    {
        Header = "Status";

        _grid = new Grid { Style = (Style)Application.Current.Resources["StatusGridStyle"] };
        _grid.ColumnDefinitions.Add(new ColumnDefinition { Width = GridLength.Auto });
        _grid.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(1, GridUnitType.Star) });
        Children.Add(_grid);

        (_rowsLabel, _rowsValue) = (CreateLabel("Rows", top: false), CreateValue("0"));
        (_shapingLabel, _shapingValue) = (CreateLabel("Shaping", top: false), CreateValue("Flat"));
        (_lastActionLabel, _lastActionValue) = (CreateLabel("Last action", top: true), CreateValue("(none)"));
        AutomationProperties.SetLiveSetting(_lastActionValue, AutomationLiveSetting.Polite);

        Readouts = new CallbackCollection<UIElement>(InsertReadout, RemoveReadout);
        Loading += (_, _) => DeriveAutomationIds();
        Rebuild();
    }

    public IList<UIElement> Readouts { get; }

    public string? AutomationIdPrefix
    {
        get => (string?)GetValue(AutomationIdPrefixProperty);
        set => SetValue(AutomationIdPrefixProperty, value);
    }

    public static readonly DependencyProperty AutomationIdPrefixProperty =
        DependencyProperty.Register(nameof(AutomationIdPrefix), typeof(string), typeof(StatusPanel),
            new PropertyMetadata(null, (d, e) => ((StatusPanel)d).DeriveAutomationIds()));

    public string Rows
    {
        get => (string)GetValue(RowsProperty);
        set => SetValue(RowsProperty, value);
    }

    public static readonly DependencyProperty RowsProperty =
        DependencyProperty.Register(nameof(Rows), typeof(string), typeof(StatusPanel),
            new PropertyMetadata("0", (d, e) => ((StatusPanel)d)._rowsValue.Text = (string?)e.NewValue ?? string.Empty));

    public string ShapingText
    {
        get => (string)GetValue(ShapingTextProperty);
        set => SetValue(ShapingTextProperty, value);
    }

    public static readonly DependencyProperty ShapingTextProperty =
        DependencyProperty.Register(nameof(ShapingText), typeof(string), typeof(StatusPanel),
            new PropertyMetadata("Flat", (d, e) => ((StatusPanel)d)._shapingValue.Text = (string?)e.NewValue ?? string.Empty));

    public string LastAction
    {
        get => (string)GetValue(LastActionProperty);
        set => SetValue(LastActionProperty, value);
    }

    public static readonly DependencyProperty LastActionProperty =
        DependencyProperty.Register(nameof(LastAction), typeof(string), typeof(StatusPanel),
            new PropertyMetadata("(none)", (d, e) => ((StatusPanel)d).OnLastActionChanged((string?)e.NewValue ?? string.Empty)));

    public bool ShowRows
    {
        get => (bool)GetValue(ShowRowsProperty);
        set => SetValue(ShowRowsProperty, value);
    }

    public static readonly DependencyProperty ShowRowsProperty =
        DependencyProperty.Register(nameof(ShowRows), typeof(bool), typeof(StatusPanel),
            new PropertyMetadata(true, (d, e) => ((StatusPanel)d).Rebuild()));

    public bool ShowShaping
    {
        get => (bool)GetValue(ShowShapingProperty);
        set => SetValue(ShowShapingProperty, value);
    }

    public static readonly DependencyProperty ShowShapingProperty =
        DependencyProperty.Register(nameof(ShowShaping), typeof(bool), typeof(StatusPanel),
            new PropertyMetadata(true, (d, e) => ((StatusPanel)d).Rebuild()));

    public bool ShowLastAction
    {
        get => (bool)GetValue(ShowLastActionProperty);
        set => SetValue(ShowLastActionProperty, value);
    }

    public static readonly DependencyProperty ShowLastActionProperty =
        DependencyProperty.Register(nameof(ShowLastAction), typeof(bool), typeof(StatusPanel),
            new PropertyMetadata(true, (d, e) => ((StatusPanel)d).Rebuild()));

    public static string GetLabel(DependencyObject element) => (string)element.GetValue(LabelProperty);

    public static void SetLabel(DependencyObject element, string value) => element.SetValue(LabelProperty, value);

    public static readonly DependencyProperty LabelProperty =
        DependencyProperty.RegisterAttached("Label", typeof(string), typeof(StatusPanel),
            new PropertyMetadata(string.Empty, OnLabelChanged));

    // A label can be set before or after XAML adds its readout to the panel; keep the row in sync.
    private static readonly DependencyProperty LabelBlockProperty =
        DependencyProperty.RegisterAttached("LabelBlock", typeof(TextBlock), typeof(StatusPanel), new PropertyMetadata(null));

    private static void OnLabelChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        if (d.GetValue(LabelBlockProperty) is TextBlock label)
        {
            label.Text = (string?)e.NewValue ?? string.Empty;
        }
    }

    private void InsertReadout(int index, UIElement element)
    {
        if (element is not FrameworkElement value)
        {
            return;
        }

        var label = CreateLabel(GetLabel(value), top: true);
        value.SetValue(LabelBlockProperty, label);
        if (value is TextBlock text)
        {
            text.Style = (Style)Application.Current.Resources["StatusReadoutValueStyle"];
            text.TextWrapping = TextWrapping.Wrap;
        }

        _rows.Insert(index, (label, value));
        DeriveAutomationId(value);
        Rebuild();
    }

    private void RemoveReadout(int index)
    {
        _rows[index].Value.ClearValue(LabelBlockProperty);
        _rows.RemoveAt(index);
        Rebuild();
    }

    private void Rebuild()
    {
        if (_grid is null)
        {
            return;
        }

        var all = new List<(TextBlock Label, FrameworkElement Value)>(_rows);
        if (ShowRows) all.Add((_rowsLabel, _rowsValue));
        if (ShowShaping) all.Add((_shapingLabel, _shapingValue));
        if (ShowLastAction) all.Add((_lastActionLabel, _lastActionValue));

        _grid.Children.Clear();
        _grid.RowDefinitions.Clear();
        for (var i = 0; i < all.Count; i++)
        {
            _grid.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });
            var (label, value) = all[i];
            Grid.SetRow(label, i);
            Grid.SetColumn(label, 0);
            Grid.SetRow(value, i);
            Grid.SetColumn(value, 1);
            _grid.Children.Add(label);
            _grid.Children.Add(value);
        }
    }

    private void OnLastActionChanged(string text)
    {
        var changed = !string.Equals(_lastActionValue.Text, text, System.StringComparison.Ordinal);
        _lastActionValue.Text = text;

        // TextBlock does not raise LiveRegionChanged on a text change; raise it once per new message.
        if (changed && _lastActionValue.IsLoaded)
        {
            (FrameworkElementAutomationPeer.FromElement(_lastActionValue)
                ?? FrameworkElementAutomationPeer.CreatePeerForElement(_lastActionValue))
                ?.RaiseAutomationEvent(AutomationEvents.LiveRegionChanged);
        }
    }

    private void DeriveAutomationIds()
    {
        var prefix = AutomationIdPrefix ?? string.Empty;
        AutomationProperties.SetAutomationId(_rowsValue, prefix + "RowsText");
        AutomationProperties.SetAutomationId(_shapingValue, prefix + "ShapingModeText");
        AutomationProperties.SetAutomationId(_lastActionValue, prefix + "LastActionText");
        foreach (var (_, value) in _rows)
        {
            DeriveAutomationId(value);
        }
    }

    // Re-run on Loading in case XAML set the Name after it added the element.
    private void DeriveAutomationId(FrameworkElement value)
    {
        if (string.IsNullOrEmpty(value.Name) || !string.IsNullOrEmpty(AutomationProperties.GetAutomationId(value)))
        {
            return;
        }

        AutomationProperties.SetAutomationId(value, (AutomationIdPrefix ?? string.Empty) + value.Name);
    }

    private static TextBlock CreateLabel(string text, bool top)
    {
        var label = new TextBlock
        {
            Text = text,
            Style = (Style)Application.Current.Resources["StatusReadoutLabelStyle"],
        };
        if (top)
        {
            label.VerticalAlignment = VerticalAlignment.Top;
        }

        return label;
    }

    private static TextBlock CreateValue(string text) => new()
    {
        Text = text,
        Style = (Style)Application.Current.Resources["StatusReadoutValueStyle"],
    };
}
