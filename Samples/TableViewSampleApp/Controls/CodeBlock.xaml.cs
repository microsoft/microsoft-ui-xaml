// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.IO;
using System.Reflection;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using Windows.ApplicationModel.DataTransfer;
using Windows.System;

namespace TableViewSampleApp.Controls;

/// <summary>
/// Reusable code-preview block. Authoritative snippet text is loaded from
/// embedded .txt resources under Snippets/, keyed by the SnippetName
/// dependency property. This avoids drift between hardcoded strings in
/// the gallery XAML and the live page implementation, while keeping the
/// snippet pipeline trivial (no file I/O, no source-reflection plumbing).
/// </summary>
public sealed partial class CodeBlock : UserControl
{
    private const string CopyButtonText = "Copy";
    private const string CopyButtonAutomationName = "Copy code";
    private const string CopyIconGlyph = "\uE8C8";
    private const string CopiedIconGlyph = "\uE73E";

    private readonly DispatcherTimer _copyFeedbackTimer;

    public CodeBlock()
    {
        InitializeComponent();
        _copyFeedbackTimer = new DispatcherTimer { Interval = TimeSpan.FromSeconds(2) };
        _copyFeedbackTimer.Tick += OnCopyFeedbackTimerTick;
        Unloaded += OnUnloaded;
        CodeText.PointerWheelChanged += OnCodeWheelChanged;
    }

    // Without this, a ScrollViewer that cannot scroll vertically turns a vertical wheel into horizontal
    // scrolling; hand the wheel to the nearest vertically scrollable ancestor instead.
    private void OnCodeWheelChanged(object sender, PointerRoutedEventArgs e)
    {
        if (e.Handled || CodeScroller.ScrollableHeight > 0.5
            || (e.KeyModifiers & (VirtualKeyModifiers.Shift | VirtualKeyModifiers.Control)) != 0)
        {
            return;
        }

        var properties = e.GetCurrentPoint(CodeScroller).Properties;
        if (properties.IsHorizontalMouseWheel || properties.MouseWheelDelta == 0)
        {
            return;
        }

        e.Handled = true;
        for (var parent = VisualTreeHelper.GetParent(CodeScroller); parent is not null; parent = VisualTreeHelper.GetParent(parent))
        {
            if (parent is ScrollViewer outer
                && outer.VerticalScrollMode != ScrollMode.Disabled
                && outer.ScrollableHeight > 0.5)
            {
                var offset = Math.Clamp(outer.VerticalOffset - properties.MouseWheelDelta, 0, outer.ScrollableHeight);
                outer.ChangeView(null, offset, null, disableAnimation: true);
                return;
            }
        }
    }

    public string Caption
    {
        get => (string)GetValue(CaptionProperty);
        set => SetValue(CaptionProperty, value);
    }

    public static readonly DependencyProperty CaptionProperty =
        DependencyProperty.Register(nameof(Caption), typeof(string), typeof(CodeBlock),
            new PropertyMetadata("Source", OnContentChanged));

    public string Code
    {
        get => (string)GetValue(CodeProperty);
        set => SetValue(CodeProperty, value);
    }

    public static readonly DependencyProperty CodeProperty =
        DependencyProperty.Register(nameof(Code), typeof(string), typeof(CodeBlock),
            new PropertyMetadata(string.Empty, OnContentChanged));

    public double CodeMaxHeight
    {
        get => (double)GetValue(CodeMaxHeightProperty);
        set => SetValue(CodeMaxHeightProperty, value);
    }

    public static readonly DependencyProperty CodeMaxHeightProperty =
        DependencyProperty.Register(nameof(CodeMaxHeight), typeof(double), typeof(CodeBlock),
            new PropertyMetadata(360.0, OnCodeMaxHeightChanged));

    private static void OnCodeMaxHeightChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        if (d is CodeBlock block)
        {
            block.ApplyCodeMaxHeight();
        }
    }

    private void ApplyCodeMaxHeight()
    {
        if (CodeScroller is null)
        {
            return;
        }

        var bounded = !double.IsInfinity(CodeMaxHeight);
        CodeScroller.MaxHeight = CodeMaxHeight;
        CodeScroller.VerticalScrollMode = bounded ? ScrollMode.Auto : ScrollMode.Disabled;
        CodeScroller.VerticalScrollBarVisibility = bounded ? ScrollBarVisibility.Auto : ScrollBarVisibility.Disabled;
    }

    /// <summary>
    /// Sets <see cref="Code"/> to the contents of an embedded snippet file.
    /// Pass the file's logical name (e.g. "Showcase.xaml.txt"). The file
    /// must be present under Snippets/ and built as an EmbeddedResource (the
    /// .csproj wildcard already handles this).
    /// </summary>
    public string SnippetName
    {
        get => (string)GetValue(SnippetNameProperty);
        set => SetValue(SnippetNameProperty, value);
    }

    public static readonly DependencyProperty SnippetNameProperty =
        DependencyProperty.Register(nameof(SnippetName), typeof(string), typeof(CodeBlock),
            new PropertyMetadata(string.Empty, OnSnippetNameChanged));

    private static void OnSnippetNameChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        if (d is CodeBlock block && e.NewValue is string name && !string.IsNullOrEmpty(name))
        {
            block.Code = LoadSnippet(name);
        }
    }

    protected override void OnApplyTemplate()
    {
        base.OnApplyTemplate();
        SyncContent();
    }

    private static void OnContentChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        if (d is CodeBlock block)
        {
            block.SyncContent();
        }
    }

    private void SyncContent()
    {
        if (CaptionText is not null)
        {
            CaptionText.Text = Caption ?? string.Empty;
        }

        if (CodeText is not null)
        {
            CodeText.Text = Code ?? string.Empty;
        }

        ApplyCodeMaxHeight();

        if (CopyButton is not null && !_copyFeedbackTimer.IsEnabled)
        {
            AutomationProperties.SetName(CopyButton, ResolvedCopyAutomationName);
        }
    }

    // A page can host more than one CodeBlock (XAML plus code-behind), so the copy
    // buttons are qualified by caption — otherwise they are indistinguishable to a
    // screen reader and read as the same button twice.
    private string ResolvedCopyAutomationName =>
        string.IsNullOrWhiteSpace(Caption) ? CopyButtonAutomationName : $"Copy {Caption}";

    private static string LoadSnippet(string snippetName)
    {
        var assembly = typeof(CodeBlock).GetTypeInfo().Assembly;
        // Embedded-resource names follow <DefaultNamespace>.Snippets.<File>
        // The MSBuild EmbeddedResource item with default LogicalName conventions
        // generates "TableViewSampleApp.Snippets.<file>" given the project's
        // RootNamespace. Match by ".<file>" suffix only, so "Sort.cs.txt" never matches
        // "XSort.cs.txt".
        foreach (var resource in assembly.GetManifestResourceNames())
        {
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

    private void OnCopyClick(object sender, RoutedEventArgs e)
    {
        try
        {
            var package = new DataPackage();
            package.SetText(Code ?? string.Empty);
            Clipboard.SetContent(package);
            ShowCopyFeedback("Copied", "Copied to clipboard", CopiedIconGlyph,
                AutomationNotificationKind.ActionCompleted, "Code copied to clipboard");
        }
        catch (Exception)
        {
            ShowCopyFeedback("Copy failed", "Copy failed", CopyIconGlyph,
                AutomationNotificationKind.ActionAborted, "Code could not be copied to clipboard");
        }
    }

    private void ShowCopyFeedback(
        string buttonText,
        string automationName,
        string iconGlyph,
        AutomationNotificationKind notificationKind,
        string notificationText)
    {
        CopyText.Text = buttonText;
        CopyIcon.Glyph = iconGlyph;
        AutomationProperties.SetName(CopyButton, automationName);

        var peer = FrameworkElementAutomationPeer.FromElement(CopyButton)
                   ?? FrameworkElementAutomationPeer.CreatePeerForElement(CopyButton);
        peer?.RaiseNotificationEvent(
            notificationKind,
            AutomationNotificationProcessing.MostRecent,
            notificationText,
            "CodeBlockCopy");

        _copyFeedbackTimer.Stop();
        _copyFeedbackTimer.Start();
    }

    private void OnCopyFeedbackTimerTick(object? sender, object e)
    {
        _copyFeedbackTimer.Stop();
        CopyText.Text = CopyButtonText;
        CopyIcon.Glyph = CopyIconGlyph;
        AutomationProperties.SetName(CopyButton, ResolvedCopyAutomationName);
    }

    private void OnUnloaded(object sender, RoutedEventArgs e)
    {
        _copyFeedbackTimer.Stop();
    }
}
