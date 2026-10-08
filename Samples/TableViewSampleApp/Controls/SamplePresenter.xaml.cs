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

/// <summary>
/// Page-scaffold control inspired by w-ahmad/WinUI.TableView's SamplePresenter.
///
/// Layout: Header + Description at top, an Example slot for the live demo on the left,
/// an optional Options rail (fixed 320 px) on the right, and a collapsible Source
/// expander at the bottom. Pages should set Header, Description, Example, and
/// (optionally) Options + SourceSnippet or SourceXaml.
///
/// Per-page Theme toggle is intentionally absent — the shell's title-bar Theme button
/// is the single source of truth for theme switching. See SamplePresenter.xaml comment.
/// </summary>
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

    // An expanded Expander adds its content border and padding around the Source scroller.
    // Used until the expanded chrome has been measured once.
    private const double ExpandedSourceChromeEstimate = 26;

    // How long after an expand the Source block is kept in view while the layout settles.
    private const long RevealSourceWindowMs = 1500;

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

    // Everything an expanded Source expander adds above and around SourceContentScroller (top
    // margin, header, content border and padding). Measured once expanded; it does not depend on
    // the scroller's own height, so using it cannot feed back into the cap.
    private double _expandedSourceChrome = CollapsedSourceEstimate + ExpandedSourceChromeEstimate;

    // After Source is expanded, RevealSourceIfPending keeps the block in view until this
    // Environment.TickCount64 deadline (see RevealSourceWindowMs).
    private long _revealSourceUntil;
    private bool _revealSourceQueued;

    public SamplePresenter()
    {
        InitializeComponent();
        SizeChanged += OnSizeChanged;
        Loaded += OnLoaded;
        Unloaded += OnUnloaded;

        // The Source scroller is already bounded, so the code blocks inside it stop scrolling
        // vertically themselves: one vertical scroller, no trapped mouse wheel (D:S9).
        SourceCodeBlock.CodeMaxHeight = double.PositiveInfinity;
        AdditionalSourceCodeBlock.CodeMaxHeight = double.PositiveInfinity;
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

    // Expanding/collapsing the source expander changes how much vertical space the
    // Example row can claim, so re-run the stretch sizing against the current height. On
    // expand, the code is also scrolled into view once laid out (see RevealSourceIfPending):
    // when the page scrolls, the expanded block would otherwise open below the window.
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

            // Give the page grid the full content width (up to PageMaxContentWidth) instead of
            // its content's desired width: a Left-aligned grid otherwise sizes to whatever the
            // table and rail ask for, so the rail's right edge moved from page to page.
            var width = Math.Max(0, availableSize.Width - OuterGrid.Margin.Left - OuterGrid.Margin.Right);
            if (!OuterGrid.Width.Equals(width))
            {
                OuterGrid.Width = width;
            }
        }

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

    // Defensive guard (FIX-PLAN 0.6b, D:S1): if the Example row is smaller than what the Example
    // content needs (its Auto rows plus the table's MinHeight), remember the need and re-measure.
    // ApplyStretchSizing then unpins and lets the OuterScroller scroll, instead of a `*` table row
    // collapsing to 0 under tall Auto siblings or overflowing onto the Source expander.
    //
    // DesiredSize cannot show the overflow: it is clamped to the available size. A clipped child
    // can: it is arranged at its own (minimum) height inside a smaller layout slot. Checked after
    // every layout pass (cheap: three levels of the Example), because a resize can
    // turn a fitting page into an overflowing one without the Example's own size changing.
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

    /// <summary>
    /// When true, the Example row consumes all remaining vertical space (Row 3 = `*`)
    /// instead of sizing to its content's natural height. Useful for showcase pages
    /// where the demo IS the page — the table fills the viewport responsively on
    /// any resolution / DPI / window size, without per-page pixel heights.
    /// Default false preserves Auto + MinHeight=320 sizing for all existing pages.
    /// </summary>
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
            sp.ApplyStretchSizing(sp._availableHeight);
            sp.InvalidateMeasure();
        }
    }

    // Star sizing inside an infinite-height ScrollViewer collapses to 0, so when StretchExample
    // is on and the page fits, the inner Grid's Height is pinned to the height the parent
    // offered (minus the Grid's own margin), which gives `*` a finite range to divide.
    //
    // The page does NOT fit, and is left to scroll, when:
    //  * the layout is narrow (< 900 px): the rail sits under the Example (D:S2);
    //  * title + description + the Example's minimum + the Source block exceed the height (D:S1).
    //    An expanded Source block counts its chrome plus MinSourceHeight of code.
    // Then the grid is unpinned and the Example row gets a fixed height, so the table stays
    // bounded and virtualizes; it never sits in an unbounded scroller.
    //
    // Source rule (P1-1). Expanding Source never balloons the table and never leaves the code
    // below the window:
    //  * pinned page: the Example row shrinks towards its minimum and the code gets the rest,
    //    at least MinSourceHeight (the fit check reserves it);
    //  * scrolling page: the Example row drops to its minimum, the code is capped to what is
    //    left of the viewport under it (at least MinSourceHeight), and RevealSourceIfPending
    //    scrolls the page so the whole expanded block, and the table above it, are on screen.
    private void ApplyStretchSizing(double availableHeight)
    {
        if (OuterGrid is null)
        {
            return;
        }

        var hasHeight = !double.IsNaN(availableHeight) && availableHeight > 0;
        var target = hasHeight ? Math.Max(0, availableHeight - OuterGrid.Margin.Top - OuterGrid.Margin.Bottom) : 0;
        var sourceVisible = SourceExpander.Visibility == Visibility.Visible;
        var sourceExpanded = sourceVisible && SourceExpander.IsExpanded;
        // Right after a collapse ActualHeight is still the expanded height; fall back to the
        // estimate rather than treat the old code block as chrome.
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
                // The page scrolls; keep the code bounded to one viewport so its own scrollbar
                // stays reachable once the block is revealed.
                SetMaxHeight(SourceContentScroller, Math.Max(MinSourceHeight, target - sourceChrome));
            }
            else
            {
                ClearIfSet(SourceContentScroller, MaxHeightProperty);
            }

            return;
        }

        var headerBlock = HeaderBlockHeight();
        var reservedCode = sourceExpanded ? MinSourceHeight : 0;
        var fits = !_isNarrow && headerBlock + _exampleMinHeight + sourceChrome + reservedCode <= target;
        if (fits)
        {
            SetExampleRowHeight(new GridLength(1, GridUnitType.Star));
            if (!OuterGrid.Height.Equals(target))
            {
                OuterGrid.Height = target;
            }

            // Cap the source content to what the pinned grid can spare once the Example row has
            // its minimum and the title block is accounted for. Without the cap the expander's
            // Auto row asks for the full height of both code blocks, overflows the pin, and is
            // clipped with no way to scroll to it. The fit check guarantees MinSourceHeight.
            var spare = target - headerBlock - _exampleMinHeight - sourceChrome;
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
                // Wide: the Example (at its minimum) and the whole Source block share one
                // viewport. Narrow: the rail sits between them, so the block gets the viewport.
                codeCap = _isNarrow
                    ? target - sourceChrome
                    : target - _exampleMinHeight - sourceChrome - OuterGrid.RowSpacing;
            }
            else
            {
                exampleHeight = Math.Max(_exampleMinHeight, target - headerBlock - sourceChrome);
                codeCap = target - sourceChrome;
            }

            SetExampleRowHeight(new GridLength(Math.Floor(exampleHeight), GridUnitType.Pixel));
            SetMaxHeight(SourceContentScroller, Math.Max(MinSourceHeight, Math.Floor(codeCap)));
        }
    }

    // After Source expands: once the expanded block is laid out, scroll the page (if it scrolls)
    // so the bottom of the block meets the bottom of the viewport, without scrolling its header
    // off the top. With the cap from ApplyStretchSizing that leaves the table, at its minimum,
    // in view above the code. A pinned page cannot scroll, so this is a no-op there.
    //
    // The layout can still settle for a moment after the first pass (the measured chrome
    // replaces its estimate, or the overflow guard learns a larger Example minimum and the code
    // moves down), so the reveal re-runs on every layout pass for a short window after the
    // expand. It only ever scrolls forward, so it cannot fight a user who scrolls back up later.
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
            // First expand: replace the estimate with the measured chrome and size again.
            _expandedSourceChrome = chrome;
            InvalidateMeasure();
            return;
        }

        // Scroll after the pass settles, when the scroller's extent includes the expanded block.
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

    // Title + description + the spacer row + the Grid's row spacing (5 gaps between 6 rows).
    private double HeaderBlockHeight()
    {
        var header = HeaderText.ActualHeight > 0 ? HeaderText.ActualHeight : 36;
        var description = DescriptionText.ActualHeight > 0 ? DescriptionText.ActualHeight : 40;
        return header + description + SpacerRow.Height.Value + (OuterGrid.RowSpacing * 5);
    }

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

        // Below 900 px, drop the Options rail under the Example so the demo isn't squeezed.
        // The Options rail then takes Row 4 (which was the source-expander row in wide
        // mode), so we also have to move SourceExpander down to Row 5 to avoid both
        // landing in the same cell. Without Options, keep the default wide placement
        // with a collapsed Options column.
        if (isNarrow)
        {
            Grid.SetRow(OptionsBorder, 4);
            Grid.SetColumn(OptionsBorder, 0);
            Grid.SetColumnSpan(OptionsBorder, 2);
            OptionsBorder.Margin = new Thickness(0, 12, 0, 0);
            // Collapse the right column to 0 so it doesn't reserve 320 px of dead space.
            OptionsColumn.Width = new GridLength(0);
            // Push the source expander to Row 5 so it sits BELOW the reparented Options
            // rail. Restore in wide mode.
            Grid.SetRow(SourceExpander, 5);
        }
        else
        {
            Grid.SetRow(OptionsBorder, 3);
            Grid.SetColumn(OptionsBorder, 1);
            Grid.SetColumnSpan(OptionsBorder, 1);
            OptionsBorder.Margin = new Thickness(12, 0, 0, 0);
            // Restore the fixed-width column for the wide layout.
            OptionsColumn.Width = hasOptions ? new GridLength(OptionsColumnWidth) : new GridLength(0);
            Grid.SetRow(SourceExpander, 4);
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
                return reader.ReadToEnd();
            }
        }
        throw new InvalidOperationException($"Snippet '{snippetName}' was not found in embedded resources.");
    }
}
