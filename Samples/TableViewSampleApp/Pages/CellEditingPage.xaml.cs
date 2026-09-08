// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using TableViewSampleApp.Data;
using TableViewSampleApp.Models;

namespace TableViewSampleApp.Pages;

/// <summary>
/// Demonstrates the control's built-in in-place cell editing. When
/// <c>TableView.IsReadOnly</c> is <c>false</c>, a <c>TableViewTextColumn</c>
/// cell enters edit on double-tap (or F2 on the focused row); Enter / Tab
/// commit the edited value back to the bound model and Esc cancels.
///
/// Unlike <see cref="DensityReadOnlyPage"/> — which hosts editable TextBox
/// template cells and mirrors a read-only flag into them — this page uses plain
/// text columns and relies on the framework editing path. Because editing is a
/// model-first, two-way write, every committed edit lands directly on the
/// <see cref="Person"/> model (INotifyPropertyChanged) and is surfaced live in
/// the edit log.
/// </summary>
public sealed partial class CellEditingPage : Page, INotifyPropertyChanged
{
    // Property names that map to an editable text column on this page.
    private static readonly string[] s_editableProperties =
    {
        nameof(Person.FirstName),
        nameof(Person.LastName),
        nameof(Person.Role),
        nameof(Person.Department),
        nameof(Person.Email),
    };

    private string _statusText = string.Empty;

    public CellEditingPage()
    {
        InitializeComponent();

        foreach (var person in PersonData.Take(20))
        {
            person.PropertyChanged += OnPersonPropertyChanged;
            People.Add(person);
        }

        DemoTable.ItemsSource = People;

        // Start editable so double-tap / F2 works without first flipping the toggle.
        DemoTable.IsReadOnly = false;

        UpdateStatus();
    }

    public ObservableCollection<Person> People { get; } = new();

    /// <summary>Rolling log of committed edits, newest first (bound in XAML).</summary>
    public ObservableCollection<string> EditLog { get; } = new();

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

    private void OnReadOnlyToggled(object sender, RoutedEventArgs e)
    {
        if (DemoTable is null || ReadOnlyToggle is null)
        {
            return;
        }

        DemoTable.IsReadOnly = ReadOnlyToggle.IsOn;
        UpdateStatus();
    }

    private void OnPersonPropertyChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (sender is not Person person || e.PropertyName is null)
        {
            return;
        }

        // Only surface edits to the columns this page actually exposes; skip
        // derived-property notifications (FullName / Initial / JoinDateText).
        if (Array.IndexOf(s_editableProperties, e.PropertyName) < 0)
        {
            return;
        }

        var newValue = e.PropertyName switch
        {
            nameof(Person.FirstName) => person.FirstName,
            nameof(Person.LastName) => person.LastName,
            nameof(Person.Role) => person.Role,
            nameof(Person.Department) => person.Department,
            nameof(Person.Email) => person.Email,
            _ => string.Empty,
        };

        // Newest first; cap the log so it stays readable in the fixed-height rail.
        EditLog.Insert(0, $"{person.FullName} · {e.PropertyName} = {newValue}");
        while (EditLog.Count > 50)
        {
            EditLog.RemoveAt(EditLog.Count - 1);
        }

        UpdateStatus();
    }

    private void UpdateStatus()
    {
        var mode = ReadOnlyToggle is not null && ReadOnlyToggle.IsOn ? "read-only" : "editable";
        StatusText = $"Cells: {mode} · {People.Count:N0} rows · {EditLog.Count:N0} edits committed";
    }

    public event PropertyChangedEventHandler? PropertyChanged;

    private void OnPropertyChanged([CallerMemberName] string? name = null) =>
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
}
