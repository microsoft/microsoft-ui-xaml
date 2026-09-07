// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using TableViewSampleApp.Models;
using TableViewGridLinesVisibility = Microsoft.UI.Xaml.Controls.Tabular.TableViewGridLinesVisibility;
using TableViewDensity = Microsoft.UI.Xaml.Controls.Tabular.TableViewDensity;

namespace TableViewSampleApp.Pages;

public sealed partial class FilePropertiesPage : Page
{
    public FilePropertiesPage()
    {
        InitializeComponent();

        if (PropTable is not null)
        {
            PropTable.HeadersVisibility = TableViewHeadersVisibility.Column;
            PropTable.GridLinesVisibility = TableViewGridLinesVisibility.Horizontal;
            PropTable.Density = TableViewDensity.Compact;
        }

        Loaded += OnLoaded;
    }

    private void OnLoaded(object sender, RoutedEventArgs e)
    {
        PopulateFiles();
    }

    private void PopulateFiles()
    {
        if (FileList is null)
        {
            return;
        }

        var files = EnumerateFiles(AppContext.BaseDirectory);
        if (files.Count < 1)
        {
            var userProfile = Environment.GetFolderPath(Environment.SpecialFolder.UserProfile);
            if (!string.IsNullOrWhiteSpace(userProfile))
            {
                files = EnumerateFiles(userProfile);
            }
        }

        FileList.ItemsSource = files;

        if (files.Count > 0)
        {
            FileList.SelectedIndex = 0;
        }
        else
        {
            if (SelectedFileText is not null)
            {
                SelectedFileText.Text = "No readable files found";
            }

            if (StatusText is not null)
            {
                StatusText.Text = "0 properties across 0 sections";
            }
        }
    }

    private static List<FileInfo> EnumerateFiles(string folder)
    {
        try
        {
            if (string.IsNullOrWhiteSpace(folder) || !Directory.Exists(folder))
            {
                return new List<FileInfo>();
            }

            return new DirectoryInfo(folder)
                .EnumerateFiles()
                .OrderBy(file => file.Name, StringComparer.OrdinalIgnoreCase)
                .Take(100)
                .ToList();
        }
        catch
        {
            return new List<FileInfo>();
        }
    }

    private void OnFileSelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        if (FileList?.SelectedItem is FileInfo file)
        {
            ShowFileProperties(file);
        }
    }

    private void ShowFileProperties(FileInfo file)
    {
        if (PropTable is null)
        {
            return;
        }

        var entries = BuildEntries(file);
        // Source keying and grouping are not in this release: the source stays flat.
        PropTable.ItemsSource = TableViewSource.From(entries);

        if (SelectedFileText is not null)
        {
            SelectedFileText.Text = file.Name;
        }

        if (StatusText is not null)
        {
            var sectionCount = entries.Select(entry => entry.Section).Distinct(StringComparer.Ordinal).Count();
            StatusText.Text = string.Format(CultureInfo.CurrentCulture, "{0} properties across {1} sections", entries.Count, sectionCount);
        }
    }

    private static List<FilePropertyEntry> BuildEntries(FileInfo file)
    {
        return new List<FilePropertyEntry>
        {
            new("Description", "Title"),
            new("Description", "Subject"),
            new("Description", "Tags"),
            new("Description", "Categories"),
            new("Description", "Comments"),

            new("Origin", "Authors"),
            new("Origin", "Last saved by"),
            new("Origin", "Revision number"),
            new("Origin", "Version number"),
            new("Origin", "Program name"),
            new("Origin", "Company"),
            new("Origin", "Manager"),
            new("Origin", "Content created", SafeRead(() => file.CreationTime.ToString("g", CultureInfo.CurrentCulture))),
            new("Origin", "Date last saved", SafeRead(() => file.LastWriteTime.ToString("g", CultureInfo.CurrentCulture))),
            new("Origin", "Last printed"),

            new("File", "Name", SafeRead(() => file.Name)),
            new("File", "Item type", SafeRead(() => GetFileType(file.Extension))),
            new("File", "Folder path", SafeRead(() => file.DirectoryName ?? string.Empty)),
            new("File", "Size", SafeRead(() => FormatSize(file.Length))),
            new("File", "Date created", SafeRead(() => file.CreationTime.ToString("g", CultureInfo.CurrentCulture))),
            new("File", "Date modified", SafeRead(() => file.LastWriteTime.ToString("g", CultureInfo.CurrentCulture))),
            new("File", "Attributes", SafeRead(() => file.Attributes.ToString())),
        };
    }

    private static string GroupIdentity(object key) => key?.ToString() ?? string.Empty;

    private static string SafeRead(Func<string> read)
    {
        try
        {
            return read() ?? string.Empty;
        }
        catch
        {
            return string.Empty;
        }
    }

    private static string GetFileType(string extension)
    {
        if (string.IsNullOrWhiteSpace(extension))
        {
            return "File";
        }

        return extension.ToLowerInvariant() switch
        {
            ".cs" => "C# Source File",
            ".dll" => "Application extension",
            ".exe" => "Application",
            ".json" => "JSON File",
            ".pdb" => "Program Debug Database",
            ".pri" => "PRI File",
            ".txt" => "Text Document",
            ".winmd" => "Windows Metadata File",
            ".xaml" => "XAML File",
            ".xml" => "XML Document",
            _ => string.Format(CultureInfo.InvariantCulture, "{0} File", extension.TrimStart('.').ToUpperInvariant()),
        };
    }

    private static string FormatSize(long bytes)
    {
        string[] units = { "bytes", "KB", "MB", "GB" };
        var size = (double)bytes;
        var unit = 0;

        while (size >= 1024 && unit < units.Length - 1)
        {
            size /= 1024;
            unit++;
        }

        if (unit == 0)
        {
            return string.Format(CultureInfo.CurrentCulture, "{0:N0} {1}", bytes, units[unit]);
        }

        return string.Format(CultureInfo.CurrentCulture, "{0:N1} {1}", size, units[unit]);
    }
}