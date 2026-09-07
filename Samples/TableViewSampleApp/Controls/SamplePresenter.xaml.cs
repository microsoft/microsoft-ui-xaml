// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.IO;
using System.Reflection;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;

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
    public SamplePresenter()
    {
        InitializeComponent();
        SizeChanged += OnSizeChanged;
        Loaded += OnLoaded;
    }

    private void OnLoaded(object sender, RoutedEventArgs e)
    {
        SyncContent();
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
            new PropertyMetadata(null, OnContentChanged));

    public string? Description
    {
        get => (string?)GetValue(DescriptionProperty);
        set => SetValue(DescriptionProperty, value);
    }

    public static readonly DependencyProperty DescriptionProperty =
        DependencyProperty.Register(nameof(Description), typeof(string), typeof(SamplePresenter),
            new PropertyMetadata(null, OnContentChanged));

    public object? Example
    {
        get => GetValue(ExampleProperty);
        set => SetValue(ExampleProperty, value);
    }

    public static readonly DependencyProperty ExampleProperty =
        DependencyProperty.Register(nameof(Example), typeof(object), typeof(SamplePresenter),
            new PropertyMetadata(null, OnContentChanged));

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
        (d as SamplePresenter)?.SyncContent();
    }

    private static void OnContentChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        (d as SamplePresenter)?.SyncContent();
    }

    // x:Bind does not project reliably in this split-binary CsWinRT sample (Header /
    // Description / Example / Options all came through blank), so the scaffold pushes
    // its content slots from code-behind instead of XAML bindings. Idempotent: the DP
    // setters no-op when the value is unchanged, so repeated syncs don't re-parent.
    private void SyncContent()
    {
        if (HeaderText is not null) HeaderText.Text = Header ?? string.Empty;
        if (DescriptionText is not null) DescriptionText.Text = Description ?? string.Empty;
        if (ExampleHost is not null) ExampleHost.Content = Example;
        if (OptionsHost is not null) OptionsHost.Content = Options;
        if (SourceCodeBlock is not null)
        {
            SourceCodeBlock.Caption = SourceCaption;
            SourceCodeBlock.Code = SourceXaml;
        }
        if (AdditionalSourceCodeBlock is not null)
        {
            AdditionalSourceCodeBlock.Caption = AdditionalSnippetCaption;
            AdditionalSourceCodeBlock.Code = AdditionalSourceCode;
            AdditionalSourceCodeBlock.Visibility = AdditionalSourceVisibility;
        }
        UpdateOptionsVisibility();
        UpdateSourceVisibility();
    }

    private void UpdateOptionsVisibility()
    {
        var hasOptions = Options is not null;
        OptionsBorder.Visibility = hasOptions ? Visibility.Visible : Visibility.Collapsed;
        // FIXED 320 px (matching the rail's natural max) so the table doesn't
        // re-flow as you navigate between pages — pane-shake fix.
        OptionsColumn.Width = hasOptions ? new GridLength(320) : new GridLength(0);
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
            new PropertyMetadata("XAML", OnContentChanged));

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
            new PropertyMetadata("C#", OnContentChanged));

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
        (d as SamplePresenter)?.SyncContent();
    }

    // Exposed for x:Bind in XAML — collapses the second CodeBlock when no additional source.
    public Visibility AdditionalSourceVisibility =>
        string.IsNullOrWhiteSpace(AdditionalSourceCode) ? Visibility.Collapsed : Visibility.Visible;

    private static void OnSourceChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        (d as SamplePresenter)?.SyncContent();
    }

    private void UpdateSourceVisibility()
    {
        var hasSource = !string.IsNullOrWhiteSpace(SourceXaml)
                        || !string.IsNullOrWhiteSpace(AdditionalSourceCode);
        SourceExpander.Visibility = hasSource ? Visibility.Visible : Visibility.Collapsed;
    }

    private void OnSizeChanged(object sender, SizeChangedEventArgs e)
    {
        ApplyResponsiveLayout(e.NewSize.Width);
        ApplyStretchSizing(e.NewSize.Height);
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
            sp.ExampleRow.Height = sp.StretchExample
                ? new GridLength(1, GridUnitType.Star)
                : GridLength.Auto;
            sp.ApplyStretchSizing(sp.ActualHeight);
        }
    }

    // Star sizing inside an infinite-height ScrollViewer collapses to 0 — so when
    // StretchExample is on, pin the inner Grid's Height to the viewport so `*` has
    // a finite range to divide. The outer ScrollViewer is still enabled, so a tiny
    // window where header+description+chrome exceed viewport still scrolls.
    //
    // We subtract the Grid's own vertical Margin so that Grid+Margin together fit
    // the viewport — without this the OuterScroller adds 40 px of scrollable
    // overflow (PageContentMargin top+bottom = 16+24), which clips the Source
    // expander header and the bottom of the right-rail Options card.
    private void ApplyStretchSizing(double availableHeight)
    {
        if (StretchExample && availableHeight > 0)
        {
            var verticalMargin = OuterGrid.Margin.Top + OuterGrid.Margin.Bottom;
            var target = availableHeight - verticalMargin;
            // Floor at 0 so a tiny window doesn't pass a negative Height; the outer
            // ScrollViewer takes over in that edge case.
            OuterGrid.Height = target > 0 ? target : 0;
        }
        else
        {
            OuterGrid.ClearValue(HeightProperty);
        }
    }

    private void ApplyResponsiveLayout(double availableWidth)
    {
        // Below 900 px, drop the Options rail under the Example so the demo isn't squeezed.
        // The Options rail then takes Row 4 (which was the source-expander row in wide
        // mode), so we also have to move SourceExpander down to Row 5 to avoid both
        // landing in the same cell. Skip the layout shuffle if there's no Options
        // rail — SourceExpander then keeps its default XAML Row=4 placement.
        if (Options is null) return;

        if (availableWidth < 900)
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
            OptionsColumn.Width = new GridLength(320);
            Grid.SetRow(SourceExpander, 4);
        }
    }

    private static string LoadSnippet(string snippetName)
    {
        var assembly = typeof(SamplePresenter).GetTypeInfo().Assembly;
        foreach (var resource in assembly.GetManifestResourceNames())
        {
            if (resource.EndsWith("." + snippetName, StringComparison.OrdinalIgnoreCase) ||
                resource.EndsWith(snippetName, StringComparison.OrdinalIgnoreCase))
            {
                using var stream = assembly.GetManifestResourceStream(resource);
                if (stream is null) continue;
                using var reader = new StreamReader(stream);
                return reader.ReadToEnd();
            }
        }
        return $"// Snippet '{snippetName}' not found.";
    }
}
