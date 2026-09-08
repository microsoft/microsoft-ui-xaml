// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Diagnostics;
using System.Runtime.CompilerServices;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using TableViewSampleApp.Data;
using TableViewSampleApp.Models;
using TableViewDensity = Microsoft.UI.Xaml.Controls.Tabular.TableViewDensity;

namespace TableViewSampleApp.Pages;

/// <summary>
/// Demonstrates two table-level appearance / behavior knobs:
/// <list type="bullet">
///   <item><description><c>TableView.Density</c> — Compact / Standard / Comfortable
///     row-height + cell-padding presets, switched live.</description></item>
///   <item><description><c>TableView.IsReadOnly</c> — table-level read-only.
///     This page keeps its template-cell editors so the lock/unlock state is
///     visually obvious while the Cell Editing page covers the framework edit APIs.</description></item>
/// </list>
/// </summary>
public sealed partial class DensityReadOnlyPage : Page, INotifyPropertyChanged
{
    private TableViewDensity _density = TableViewDensity.Standard;
    private string _statusText = string.Empty;

    public DensityReadOnlyPage()
    {
        InitializeComponent();

        foreach (var person in PersonData.Take(20))
        {
            People.Add(person);
        }

        DemoTable.ItemsSource = People;

        // Re-apply the XAML defaults now that DemoTable exists (the Standard radio's
        // Checked handler ran during InitializeComponent while DemoTable was still null).
        DemoTable.Density = _density;

        // Start editable so the read-only toggle has something visible to lock.
        DemoTable.IsReadOnly = false;
        GetReadOnlyState().IsReadOnly = false;

        UpdateStatus();
    }

    public ObservableCollection<Person> People { get; } = new();

    public string StatusText
    {
        get => _statusText;
        private set
        {
            if (_statusText != value)
            {
                _statusText = value;
                OnPropertyChanged();
            }
        }
    }

    private void OnDensityChecked(object sender, RoutedEventArgs e)
    {
        if (DemoTable is null || sender is not FrameworkElement { Tag: string tag })
        {
            return;
        }

        if (!Enum.TryParse<TableViewDensity>(tag, ignoreCase: false, out var density))
        {
            Debug.Fail($"DensityReadOnlyPage: unrecognised density Tag '{tag}'.");
            return;
        }

        _density = density;
        DemoTable.Density = density;
        UpdateStatus();
    }

    private void OnReadOnlyToggled(object sender, RoutedEventArgs e)
    {
        if (DemoTable is null || ReadOnlyToggle is null)
        {
            return;
        }

        var isReadOnly = ReadOnlyToggle.IsOn;
        DemoTable.IsReadOnly = isReadOnly;
        // Mirror into the shared flag the editable template cells bind to.
        GetReadOnlyState().IsReadOnly = isReadOnly;
        UpdateStatus();
    }

    private ReadOnlyState GetReadOnlyState() => (ReadOnlyState)Resources["ReadOnlyState"];

    private void UpdateStatus()
    {
        var readOnly = ReadOnlyToggle is not null && ReadOnlyToggle.IsOn ? "read-only" : "editable";
        StatusText = $"Density: {_density} · Cells: {readOnly} · {People.Count:N0} rows";
    }

    public event PropertyChangedEventHandler? PropertyChanged;

    private void OnPropertyChanged([CallerMemberName] string? name = null) =>
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
}

/// <summary>
/// Tiny observable flag shared (as a XAML resource) between the page and its
/// editable cell templates. Each template TextBox binds its IsReadOnly to
/// <see cref="IsReadOnly"/>; the page updates it whenever TableView.IsReadOnly
/// changes so the table-level read-only state is made visible.
/// </summary>
public sealed class ReadOnlyState : INotifyPropertyChanged
{
    private bool _isReadOnly;

    public bool IsReadOnly
    {
        get => _isReadOnly;
        set
        {
            if (_isReadOnly != value)
            {
                _isReadOnly = value;
                PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(IsReadOnly)));
            }
        }
    }

    public event PropertyChangedEventHandler? PropertyChanged;
}
