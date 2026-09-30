// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.IO;
using System.Reflection;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Windows.ApplicationModel.DataTransfer;

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
    public CodeBlock()
    {
        InitializeComponent();
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
    }

    private static string LoadSnippet(string snippetName)
    {
        var assembly = typeof(CodeBlock).GetTypeInfo().Assembly;
        // Embedded-resource names follow <DefaultNamespace>.Snippets.<File>
        // The MSBuild EmbeddedResource item with default LogicalName conventions
        // generates "TableViewSampleApp.Snippets.<file>" given the project's
        // RootNamespace. Match by suffix to stay tolerant to future moves.
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

    private void OnCopyClick(object sender, RoutedEventArgs e)
    {
        var package = new DataPackage();
        package.SetText(Code ?? string.Empty);
        Clipboard.SetContent(package);
    }
}
