// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.IO;
using System.Reflection;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Media;

namespace TableViewSampleApp.Controls;

public sealed partial class SamplePresenter : UserControl
{
    private const double OptionsColumnWidth = 320;
    private const double WideLayoutMinWidth = 900;

    // The Example row's own MinHeight. Until a page's Example is seen to need more (see
    // _exampleMinHeight), this is what the Example row is assumed to need.
    private const double ExampleRowMinHeight = 320;

    // Never squeeze the source content below this; if the window is too short for even
    // one code block, the scroller inside it takes over.
    private const double MinSourceHeight = 160;

    // Height of a collapsed Source expander plus its top margin, used before it is measured.
    private const double CollapsedSourceEstimate = 44;

    private const double ExpandedSourceChromeEstimate = 26;

    private const long RevealSourceWindowMs = 1500;

    private const double HeaderGap = 12;
    private const double OuterGridTopMargin = 4;

    private double _headerHeight;

    // The smallest height the Example content has been seen to need (its Auto rows plus the
    // table's MinHeight). Learned when a child of the Example turns out taller than its slot;
    // it only grows for a given width, so the pin/scroll decision cannot oscillate.
    private double _exampleMinHeight = ExampleRowMinHeight;

    // The last finite height the presenter was offered by its parent in Measure. The pin is
    // derived from THIS, not from the presenter's own ActualHeight, so the pinned grid can never
    // feed its own size back into the pin.
    private double _availableHeight = double.NaN;
    private bool _isNarrow;
    private double _measuredWidth = double.NaN;
    private bool _overflowCheckPending;

    private double _expandedSourceChrome = CollapsedSourceEstimate + ExpandedSourceChromeEstimate;

    private long _revealSourceUntil;
    private bool _revealSourceQueued;

    public SamplePresenter()
    {
        InitializeComponent();
        SizeChanged += OnSizeChanged;
        Loaded += OnLoaded;
        Unloaded += OnUnloaded;

        // One vertical scroller: the Source scroller is already bounded.
        SourceCodeBlock.CodeMaxHeight = double.PositiveInfinity;
        AdditionalSourceCodeBlock.CodeMaxHeight = double.PositiveInfinity;

        var margin = OuterGrid.Margin;
        HeaderPanel.Margin = new Thickness(margin.Left, margin.Top, margin.Right, HeaderGap);
        OuterGrid.Margin = new Thickness(margin.Left, OuterGridTopMargin, margin.Right, margin.Bottom);
    }

    private void OnLoaded(object sender, RoutedEventArgs e)
    {
        LayoutUpdated -= OnLayoutUpdated;
        LayoutUpdated += OnLayoutUpdated;
        Bindings.Update();
        UpdateOptionsVisibility();
        UpdateSourceVisibility();
        ApplyResponsiveLayout(ActualWidth);
    }

    public string? Header
    {
        get => (string?)GetValue(HeaderProperty);
        set => SetValue(HeaderProperty, value);
    }

    public static readonly DependencyProperty HeaderProperty =
        DependencyProperty.Register(nameof(Header), typeof(string), typeof(SamplePresenter),
            new PropertyMetadata(null, OnBindingsChanged));

    public string? Description
    {
        get => (string?)GetValue(DescriptionProperty);
        set => SetValue(DescriptionProperty, value);
    }

    public static readonly DependencyProperty DescriptionProperty =
        DependencyProperty.Register(nameof(Description), typeof(string), typeof(SamplePresenter),
            new PropertyMetadata(null, OnBindingsChanged));

    public object? Example
    {
        get => GetValue(ExampleProperty);
        set => SetValue(ExampleProperty, value);
    }

    public static readonly DependencyProperty ExampleProperty =
        DependencyProperty.Register(nameof(Example), typeof(object), typeof(SamplePresenter),
            new PropertyMetadata(null, OnBindingsChanged));

    public object? Options
    {
        get => GetValue(OptionsProperty);
        set => SetValue(OptionsProperty, value);
    }

    public static readonly DependencyProperty OptionsProperty =
        DependencyProperty.Register(nameof(Options), typeof(object), typeof(SamplePresenter),
            new PropertyMetadata(null, OnOptionsChanged));

    private static void OnOptionsChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        if (d is SamplePresenter sp)
        {
            sp.Bindings.Update();
            sp.UpdateOptionsVisibility();
            sp.ApplyResponsiveLayout(sp.ActualWidth);
        }
    }

    private static void OnBindingsChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        (d as SamplePresenter)?.Bindings.Update();
    }

    private void UpdateOptionsVisibility()
    {
        var hasOptions = Options is not null;
        OptionsBorder.Visibility = hasOptions ? Visibility.Visible : Visibility.Collapsed;
        ApplyResponsiveLayout(ActualWidth);
    }

    public string? SourceXaml
    {
        get => (string?)GetValue(SourceXamlProperty);
        set => SetValue(SourceXamlProperty, value);
    }

    public static readonly DependencyProperty SourceXamlProperty =
        DependencyProperty.Register(nameof(SourceXaml), typeof(string), typeof(SamplePresenter),
            new PropertyMetadata(null, OnSourceChanged));

    public string? SourceCaption
    {
        get => (string?)GetValue(SourceCaptionProperty);
        set => SetValue(SourceCaptionProperty, value);
    }

    public static readonly DependencyProperty SourceCaptionProperty =
        DependencyProperty.Register(nameof(SourceCaption), typeof(string), typeof(SamplePresenter),
            new PropertyMetadata("XAML", OnBindingsChanged));

    /// <summary>
    /// Path under the project's Snippets/ folder of an EmbeddedResource snippet to display
    /// in the Source expander (e.g. "BasicTable.xaml.txt"). Loaded once at set-time.
    /// </summary>
    public string? SourceSnippet
    {
        get => (string?)GetValue(SourceSnippetProperty);
        set => SetValue(SourceSnippetProperty, value);
    }

    public static readonly DependencyProperty SourceSnippetProperty =
        DependencyProperty.Register(nameof(SourceSnippet), typeof(string), typeof(SamplePresenter),
            new PropertyMetadata(null, OnSourceSnippetChanged));

    private static void OnSourceSnippetChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        if (d is SamplePresenter sp && e.NewValue is string name && !string.IsNullOrEmpty(name))
        {
            sp.SourceXaml = LoadSnippet(name);
        }
    }

    /// <summary>
    /// Optional second snippet shown below SourceSnippet inside the same expander
    /// (e.g. "Filter.cs.txt" paired with "Filter.xaml.txt"). Loaded the same way
    /// as <see cref="SourceSnippet"/>.
    /// </summary>
    public string? AdditionalSnippet
    {
        get => (string?)GetValue(AdditionalSnippetProperty);
        set => SetValue(AdditionalSnippetProperty, value);
    }

    public static readonly DependencyProperty AdditionalSnippetProperty =
        DependencyProperty.Register(nameof(AdditionalSnippet), typeof(string), typeof(SamplePresenter),
            new PropertyMetadata(null, OnAdditionalSnippetChanged));

    private static void OnAdditionalSnippetChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        if (d is SamplePresenter sp && e.NewValue is string name && !string.IsNullOrEmpty(name))
        {
            sp.AdditionalSourceCode = LoadSnippet(name);
        }
        else if (d is SamplePresenter sp2)
        {
            sp2.AdditionalSourceCode = null;
        }
    }

    /// <summary>Caption rendered above the second CodeBlock. Defaults to "C#".</summary>
    public string? AdditionalSnippetCaption
    {
        get => (string?)GetValue(AdditionalSnippetCaptionProperty);
        set => SetValue(AdditionalSnippetCaptionProperty, value);
    }

    public static readonly DependencyProperty AdditionalSnippetCaptionProperty =
        DependencyProperty.Register(nameof(AdditionalSnippetCaption), typeof(string), typeof(SamplePresenter),
            new PropertyMetadata("C#", OnBindingsChanged));

    /// <summary>
    /// Bound to the second CodeBlock's Code property. Set by <see cref="AdditionalSnippet"/>
    /// loading, but may also be assigned directly for inline strings.
    /// </summary>
    public string? AdditionalSourceCode
    {
        get => (string?)GetValue(AdditionalSourceCodeProperty);
        set => SetValue(AdditionalSourceCodeProperty, value);
    }

    public static readonly DependencyProperty AdditionalSourceCodeProperty =
        DependencyProperty.Register(nameof(AdditionalSourceCode), typeof(string), typeof(SamplePresenter),
            new PropertyMetadata(null, OnAdditionalSourceCodeChanged));

    private static void OnAdditionalSourceCodeChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        if (d is SamplePresenter sp)
        {
            sp.Bindings.Update();
            sp.UpdateSourceVisibility();
        }
    }

    // Exposed for x:Bind in XAML — collapses the second CodeBlock when no additional source.
    public Visibility AdditionalSourceVisibility =>
        string.IsNullOrWhiteSpace(AdditionalSourceCode) ? Visibility.Collapsed : Visibility.Visible;

    private static void OnSourceChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        if (d is SamplePresenter sp)
        {
            sp.Bindings.Update();
            sp.UpdateSourceVisibility();
        }
    }

    private void UpdateSourceVisibility()
    {
        var hasSource = !string.IsNullOrWhiteSpace(SourceXaml)
                        || !string.IsNullOrWhiteSpace(AdditionalSourceCode);
        SourceExpander.Visibility = hasSource ? Visibility.Visible : Visibility.Collapsed;
    }

    private void OnSourceExpanderExpanding(Expander sender, ExpanderExpandingEventArgs args)
    {
        _revealSourceUntil = Environment.TickCount64 + RevealSourceWindowMs;
        InvalidateMeasure();
    }

    private void OnSourceExpanderCollapsed(Expander sender, ExpanderCollapsedEventArgs args)
    {
        _revealSourceUntil = 0;
        InvalidateMeasure();
    }

    private void OnSizeChanged(object sender, SizeChangedEventArgs e)
    {
        ApplyResponsiveLayout(e.NewSize.Width);
    }

    protected override Windows.Foundation.Size MeasureOverride(Windows.Foundation.Size availableSize)
    {
        if (!double.IsInfinity(availableSize.Width) && availableSize.Width > 0)
        {
            // A new width re-wraps the title, description and Try it text, so what the Example
            // needs is re-learned from scratch.
            if (double.IsNaN(_measuredWidth) || Math.Abs(availableSize.Width - _measuredWidth) > 1)
            {
                _measuredWidth = availableSize.Width;
                _exampleMinHeight = ExampleRowMinHeight;
            }

            ApplyResponsiveLayout(availableSize.Width);

            // Full content width; a Left-aligned grid would size to its content and move the rail per page.
            var width = Math.Max(0, availableSize.Width - OuterGrid.Margin.Left - OuterGrid.Margin.Right);
            if (!OuterGrid.Width.Equals(width))
            {
                OuterGrid.Width = width;
            }

            if (!HeaderPanel.Width.Equals(width))
            {
                HeaderPanel.Width = width;
            }
        }

        HeaderPanel.Measure(new Windows.Foundation.Size(availableSize.Width, double.PositiveInfinity));
        _headerHeight = HeaderPanel.DesiredSize.Height;

        _availableHeight = double.IsInfinity(availableSize.Height) ? double.NaN : availableSize.Height;
        ApplyStretchSizing(_availableHeight);

        var desired = base.MeasureOverride(availableSize);

        // Never report more than the parent offered. The OuterScroller scrolls whatever does
        // not fit; a larger desired size would make the parent arrange the whole page taller
        // than the window and clip it with nothing able to scroll.
        return double.IsInfinity(availableSize.Height)
            ? desired
            : new Windows.Foundation.Size(desired.Width, Math.Min(desired.Height, availableSize.Height));
    }

    private void OnUnloaded(object sender, RoutedEventArgs e) => LayoutUpdated -= OnLayoutUpdated;

    // DesiredSize is clamped to the slot, so detect overflow from children arranged taller than their
    // slot; ApplyStretchSizing then unpins and scrolls instead of collapsing the `*` table row.
    private void OnLayoutUpdated(object? sender, object e)
    {
        RevealSourceIfPending();

        if (!StretchExample || ExampleRow.Height.GridUnitType == GridUnitType.Auto || ExampleRow.ActualHeight <= 0)
        {
            return;
        }

        if (_overflowCheckPending || ExampleDeficit() <= 1)
        {
            return;
        }

        // Confirm after the pass settles: text re-wrapping during a resize briefly reports a
        // child taller than its slot, and learning from that would scroll a page that fits.
        _overflowCheckPending = true;
        DispatcherQueue.TryEnqueue(Microsoft.UI.Dispatching.DispatcherQueuePriority.Low, () =>
        {
            _overflowCheckPending = false;
            if (!IsLoaded || ExampleRow.ActualHeight <= 0)
            {
                return;
            }

            var deficit = ExampleDeficit();
            var needed = ExampleRow.ActualHeight + deficit;
            if (deficit > 1 && needed > _exampleMinHeight + 1)
            {
                _exampleMinHeight = needed;
                InvalidateMeasure();
            }
        });
    }

    // The largest amount by which an element near the top of the Example (the root, its
    // children and grandchildren, e.g. Grid > Border > TableView) is taller than its slot.
    private double ExampleDeficit() =>
        ExampleHost.Content is FrameworkElement root ? MaxDeficit(root, depth: 0) : 0;

    private static double MaxDeficit(FrameworkElement element, int depth)
    {
        var deficit = Deficit(element);
        if (depth >= 2)
        {
            return deficit;
        }

        var count = VisualTreeHelper.GetChildrenCount(element);
        for (var i = 0; i < count; i++)
        {
            if (VisualTreeHelper.GetChild(element, i) is FrameworkElement child && child.Visibility == Visibility.Visible)
            {
                deficit = Math.Max(deficit, MaxDeficit(child, depth + 1));
            }
        }

        return deficit;
    }

    private static double Deficit(FrameworkElement element)
    {
        var slot = LayoutInformation.GetLayoutSlot(element);
        return element.ActualHeight + element.Margin.Top + element.Margin.Bottom - slot.Height;
    }

    public bool StretchExample
    {
        get => (bool)GetValue(StretchExampleProperty);
        set => SetValue(StretchExampleProperty, value);
    }

    public static readonly DependencyProperty StretchExampleProperty =
        DependencyProperty.Register(nameof(StretchExample), typeof(bool), typeof(SamplePresenter),
            new PropertyMetadata(false, OnStretchExampleChanged));

    private static void OnStretchExampleChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        if (d is SamplePresenter sp)
        {
            sp.UpdateTryIt();
            sp.ApplyStretchSizing(sp._availableHeight);
            sp.InvalidateMeasure();
        }
    }

    public string? TryIt
    {
        get => (string?)GetValue(TryItProperty);
        set => SetValue(TryItProperty, value);
    }

    public static readonly DependencyProperty TryItProperty =
        DependencyProperty.Register(nameof(TryIt), typeof(string), typeof(SamplePresenter),
            new PropertyMetadata(null, (d, e) => ((SamplePresenter)d).UpdateTryIt()));

    public string? WhatToLookFor
    {
        get => (string?)GetValue(WhatToLookForProperty);
        set => SetValue(WhatToLookForProperty, value);
    }

    public static readonly DependencyProperty WhatToLookForProperty =
        DependencyProperty.Register(nameof(WhatToLookFor), typeof(string), typeof(SamplePresenter),
            new PropertyMetadata(null, (d, e) =>
            {
                var sp = (SamplePresenter)d;
                var text = (string?)e.NewValue;
                sp.WhatToLookForInfoBar.Message = text ?? string.Empty;
                sp.WhatToLookForInfoBar.Visibility = string.IsNullOrEmpty(text) ? Visibility.Collapsed : Visibility.Visible;
            }));

    public string? Snippet
    {
        get => (string?)GetValue(SnippetProperty);
        set => SetValue(SnippetProperty, value);
    }

    public static readonly DependencyProperty SnippetProperty =
        DependencyProperty.Register(nameof(Snippet), typeof(string), typeof(SamplePresenter),
            new PropertyMetadata(null, OnSnippetChanged));

    private static void OnSnippetChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        if (d is not SamplePresenter sp || e.NewValue is not string name || string.IsNullOrEmpty(name))
        {
            return;
        }

        if (sp.ReadLocalValue(SourceCaptionProperty) == DependencyProperty.UnsetValue)
        {
            sp.SourceCaption = $"{name}Page.xaml (excerpt)";
        }

        if (sp.ReadLocalValue(AdditionalSnippetCaptionProperty) == DependencyProperty.UnsetValue)
        {
            sp.AdditionalSnippetCaption = $"{name}Page.xaml.cs (excerpt)";
        }

        if (sp.ReadLocalValue(SourceSnippetProperty) == DependencyProperty.UnsetValue)
        {
            sp.SourceSnippet = $"{name}.xaml.txt";
        }

        if (sp.ReadLocalValue(AdditionalSnippetProperty) == DependencyProperty.UnsetValue)
        {
            sp.AdditionalSnippet = $"{name}.cs.txt";
        }
    }

    // Unnamed on purpose (see SamplePresenter.xaml): WinUI would surface an x:Name as the AutomationId.
    private InfoBar TryItInfoBar => (InfoBar)ExampleGrid.Children[0];

    private InfoBar WhatToLookForInfoBar => (InfoBar)RailPanel.Children[2];

    private void UpdateTryIt()
    {
        var text = TryIt;
        var hasTryIt = !string.IsNullOrEmpty(text);
        TryItInfoBar.Message = text ?? string.Empty;
        TryItInfoBar.Visibility = hasTryIt ? Visibility.Visible : Visibility.Collapsed;
        ExampleGrid.RowSpacing = hasTryIt ? 12 : 0;
        ExampleHostRow.Height = StretchExample ? new GridLength(1, GridUnitType.Star) : GridLength.Auto;
    }

    // `*` collapses to 0 in an infinite-height ScrollViewer, so a page that fits pins the grid's Height.
    // Otherwise the grid is unpinned and the Example row gets a fixed height so the table still virtualizes.
    private void ApplyStretchSizing(double availableHeight)
    {
        if (OuterGrid is null)
        {
            return;
        }

        var hasHeight = !double.IsNaN(availableHeight) && availableHeight > 0;
        var target = hasHeight ? Math.Max(0, availableHeight - _headerHeight - OuterGrid.Margin.Top - OuterGrid.Margin.Bottom) : 0;
        var sourceVisible = SourceExpander.Visibility == Visibility.Visible;
        var sourceExpanded = sourceVisible && SourceExpander.IsExpanded;
        // Right after a collapse ActualHeight is still the expanded height.
        var collapsedMeasured = SourceExpander.ActualHeight > 0 && SourceExpander.ActualHeight < CollapsedSourceEstimate * 2;
        var sourceChrome = !sourceVisible ? 0
            : sourceExpanded ? _expandedSourceChrome
            : (collapsedMeasured ? SourceExpander.ActualHeight + SourceExpander.Margin.Top : CollapsedSourceEstimate);

        if (!StretchExample || !hasHeight)
        {
            SetExampleRowHeight(StretchExample && hasHeight
                ? new GridLength(1, GridUnitType.Star)
                : GridLength.Auto);
            ClearIfSet(OuterGrid, HeightProperty);
            if (hasHeight)
            {
                SetMaxHeight(SourceContentScroller, Math.Max(MinSourceHeight, target - sourceChrome));
            }
            else
            {
                ClearIfSet(SourceContentScroller, MaxHeightProperty);
            }

            return;
        }

        var gaps = RowGapsHeight();
        var reservedCode = sourceExpanded ? MinSourceHeight : 0;
        var fits = !_isNarrow && gaps + _exampleMinHeight + sourceChrome + reservedCode <= target;
        if (fits)
        {
            SetExampleRowHeight(new GridLength(1, GridUnitType.Star));
            if (!OuterGrid.Height.Equals(target))
            {
                OuterGrid.Height = target;
            }

            // Without the cap the expander's Auto row overflows the pin and is clipped with no way to scroll.
            var spare = target - gaps - _exampleMinHeight - sourceChrome;
            SetMaxHeight(SourceContentScroller, Math.Max(MinSourceHeight, Math.Floor(spare)));
        }
        else
        {
            ClearIfSet(OuterGrid, HeightProperty);
            double exampleHeight;
            double codeCap;
            if (sourceExpanded)
            {
                exampleHeight = _exampleMinHeight;
                codeCap = _isNarrow
                    ? target - sourceChrome
                    : target - _exampleMinHeight - sourceChrome - OuterGrid.RowSpacing;
            }
            else
            {
                exampleHeight = Math.Max(_exampleMinHeight, target - gaps - sourceChrome);
                codeCap = target - sourceChrome;
            }

            SetExampleRowHeight(new GridLength(Math.Floor(exampleHeight), GridUnitType.Pixel));
            SetMaxHeight(SourceContentScroller, Math.Max(MinSourceHeight, Math.Floor(codeCap)));
        }
    }

    // Scrolls the expanded block into view. Layout can settle for a moment after the first pass, so this
    // re-runs for a short window; it only scrolls forward, so it never fights the user.
    private void RevealSourceIfPending()
    {
        if (_revealSourceQueued || Environment.TickCount64 > _revealSourceUntil)
        {
            return;
        }

        if (!SourceExpander.IsExpanded || SourceContentScroller.ActualHeight <= 0)
        {
            return;
        }

        var chrome = SourceExpander.ActualHeight + SourceExpander.Margin.Top - SourceContentScroller.ActualHeight;
        if (chrome > 0 && Math.Abs(chrome - _expandedSourceChrome) > 1)
        {
            _expandedSourceChrome = chrome;
            InvalidateMeasure();
            return;
        }

        // The scroller's extent includes the expanded block only after this pass.
        _revealSourceQueued = true;
        DispatcherQueue.TryEnqueue(Microsoft.UI.Dispatching.DispatcherQueuePriority.Low, () =>
        {
            _revealSourceQueued = false;
            if (Environment.TickCount64 > _revealSourceUntil || !IsLoaded || !SourceExpander.IsExpanded)
            {
                return;
            }

            var top = SourceExpander.TransformToVisual(OuterGrid).TransformPoint(new Windows.Foundation.Point(0, 0)).Y
                      + OuterGrid.Margin.Top;
            var bottom = top + SourceExpander.ActualHeight + OuterGrid.Margin.Bottom;
            var desired = Math.Min(bottom - OuterScroller.ViewportHeight, top - SourceExpander.Margin.Top);
            if (desired > OuterScroller.VerticalOffset + 1)
            {
                OuterScroller.ChangeView(null, desired, null, disableAnimation: true);
            }
        });
    }

    private double RowGapsHeight() => OuterGrid.RowSpacing * 2;

    private void SetExampleRowHeight(GridLength height)
    {
        if (!ExampleRow.Height.Equals(height))
        {
            ExampleRow.Height = height;
        }
    }

    private static void SetMaxHeight(FrameworkElement? element, double value)
    {
        if (element is not null && !element.MaxHeight.Equals(value))
        {
            element.MaxHeight = value;
        }
    }

    private static void ClearIfSet(FrameworkElement? element, DependencyProperty property)
    {
        if (element is not null && element.ReadLocalValue(property) != DependencyProperty.UnsetValue)
        {
            element.ClearValue(property);
        }
    }

    private void ApplyResponsiveLayout(double availableWidth)
    {
        var hasOptions = Options is not null;
        var isNarrow = hasOptions && availableWidth > 0 && availableWidth < WideLayoutMinWidth;
        if (isNarrow != _isNarrow)
        {
            _isNarrow = isNarrow;
            InvalidateMeasure();
        }

        if (isNarrow)
        {
            Grid.SetRow(OptionsBorder, 1);
            Grid.SetColumn(OptionsBorder, 0);
            Grid.SetColumnSpan(OptionsBorder, 2);
            OptionsBorder.Margin = new Thickness(0, 12, 0, 0);
            // Collapse the right column to 0 so it doesn't reserve 320 px of dead space.
            OptionsColumn.Width = new GridLength(0);
            Grid.SetRow(SourceExpander, 2);
        }
        else
        {
            Grid.SetRow(OptionsBorder, 0);
            Grid.SetColumn(OptionsBorder, 1);
            Grid.SetColumnSpan(OptionsBorder, 1);
            OptionsBorder.Margin = new Thickness(12, 0, 0, 0);
            // Restore the fixed-width column for the wide layout.
            OptionsColumn.Width = hasOptions ? new GridLength(OptionsColumnWidth) : new GridLength(0);
            Grid.SetRow(SourceExpander, 1);
        }
    }

    private static string LoadSnippet(string snippetName)
    {
        var assembly = typeof(SamplePresenter).GetTypeInfo().Assembly;
        foreach (var resource in assembly.GetManifestResourceNames())
        {
            // Match on ".<name>" only, so "Sort.cs.txt" never resolves to "XSort.cs.txt".
            if (resource.EndsWith("." + snippetName, StringComparison.OrdinalIgnoreCase))
            {
                using var stream = assembly.GetManifestResourceStream(resource);
                if (stream is null) continue;
                using var reader = new StreamReader(stream);
                return WithoutManualMarker(reader.ReadToEnd());
            }
        }
        throw new InvalidOperationException($"Snippet '{snippetName}' was not found in embedded resources.");
    }

    // The "snippet:manual" opt-out marker line is not part of the excerpt.
    private static string WithoutManualMarker(string text)
    {
        var end = text.IndexOf('\n');
        var first = (end < 0 ? text : text.Substring(0, end)).Trim();
        if (first is not ("// snippet:manual" or "<!-- snippet:manual -->"))
        {
            return text;
        }

        var rest = end < 0 ? string.Empty : text.Substring(end + 1);
        return rest.StartsWith("\r\n", StringComparison.Ordinal) ? rest.Substring(2)
            : rest.StartsWith('\n') ? rest.Substring(1)
            : rest;
    }
}
