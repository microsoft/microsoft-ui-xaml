// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using TableViewSampleApp.Data;
using TableViewSampleApp.Models;

namespace TableViewSampleApp.Pages;

/// <summary>
/// Hosts fully interactive controls inside <c>TableViewTemplateColumn</c> cells
/// so picker / menu / flyout placement can be verified in the sample app.
/// </summary>
public sealed partial class CellFlyoutsPage : Page, INotifyPropertyChanged
{
    private string _statusText = string.Empty;
    private Person? _watchedRow;

    protected override Windows.Foundation.Size MeasureOverride(Windows.Foundation.Size availableSize)
    {
        if (Content is FrameworkElement child)
        {
            child.Measure(availableSize);
            return child.DesiredSize;
        }
        return new Windows.Foundation.Size(0, 0);
    }

    protected override Windows.Foundation.Size ArrangeOverride(Windows.Foundation.Size finalSize)
    {
        if (Content is FrameworkElement child)
        {
            child.Arrange(new Windows.Foundation.Rect(0, 0, finalSize.Width, finalSize.Height));
        }
        return finalSize;
    }

    public CellFlyoutsPage()
    {
        InitializeComponent();

        foreach (var person in PersonData.Take(18))
        {
            People.Add(person);
        }

        PeopleTable.ItemsSource = People;
        Loaded += OnPageLoaded;
        Unloaded += OnPageUnloaded;
        UpdateStatus("Ready: open any picker, drop-down, or flyout from a cell.");
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

    private void OnPageLoaded(object sender, RoutedEventArgs e)
    {
        if (People.Count > 0)
        {
            _watchedRow = People[0];
            _watchedRow.PropertyChanged += OnWatchedRowChanged;
            RefreshReadout();
        }
    }

    private void OnPageUnloaded(object sender, RoutedEventArgs e)
    {
        if (_watchedRow is not null)
        {
            _watchedRow.PropertyChanged -= OnWatchedRowChanged;
            _watchedRow = null;
        }
    }

    private void OnWatchedRowChanged(object? sender, PropertyChangedEventArgs e)
    {
        RefreshReadout();
        UpdateStatus($"{e.PropertyName} changed for first row.");
    }

    private void OnDatePickerLoaded(object sender, RoutedEventArgs e)
    {
        if (sender is DatePicker datePicker)
        {
            datePicker.MinYear = new DateTimeOffset(new DateTime(2010, 1, 1));
            datePicker.MaxYear = new DateTimeOffset(DateTime.Now.AddYears(2));
        }
    }

    private void OnDepartmentComboBoxLoaded(object sender, RoutedEventArgs e)
    {
        if (sender is ComboBox comboBox)
        {
            comboBox.ItemsSource = PersonData.Departments;
        }
    }

    private void OnDetailsButtonClick(object sender, RoutedEventArgs e)
    {
        if (sender is not FrameworkElement element)
        {
            return;
        }

        // FlyoutBase.ShowAttachedFlyout alone is not reliable for a Flyout declared inside a
        // DataTemplate on WinUI 3 desktop: the template-created Flyout has no XamlRoot, and
        // showing it silently no-ops (the Click handler still runs, so the failure is invisible).
        // Framework-owned pickers - DatePicker, ComboBox - are unaffected because they set the
        // XamlRoot on their own flyouts. Resolve the flyout, give it the element's XamlRoot, and
        // ShowAt the element so it anchors to this row's cell rather than the page.
        var flyout = FlyoutBase.GetAttachedFlyout(element);
        if (flyout is null)
        {
            UpdateStatus("Details button has no attached flyout.");
            return;
        }

        flyout.XamlRoot = element.XamlRoot;
        flyout.ShowAt(element);
        UpdateStatus("Details button opened its attached flyout.");
    }

    private void OnShiftMenuItemClick(object sender, RoutedEventArgs e)
    {
        if (sender is not MenuFlyoutItem { Tag: string tag } ||
            !TimeSpan.TryParse(tag, System.Globalization.CultureInfo.InvariantCulture, out var shift))
        {
            return;
        }

        if (sender is FrameworkElement { DataContext: Person person })
        {
            person.ShiftStart = shift;
            UpdateStatus($"Shift menu committed {shift:hh\\:mm} for {person.FullName}.");
        }
    }

    private void RefreshReadout()
    {
        if (_watchedRow is null)
        {
            LiveReadout.Text = "(no rows)";
            return;
        }

        LiveReadout.Text =
            $"Name        : {_watchedRow.FullName}\n" +
            $"JoinDate    : {_watchedRow.JoinDate:yyyy-MM-dd}\n" +
            $"ShiftStart  : {_watchedRow.ShiftStart:hh\\:mm}\n" +
            $"Department  : {_watchedRow.Department}\n" +
            $"Role        : {_watchedRow.Role}";
    }

    private void UpdateStatus(string message)
    {
        StatusText = $"{message} Controls are hit-testable and tab stops.";
    }

    public event PropertyChangedEventHandler? PropertyChanged;

    private void OnPropertyChanged([CallerMemberName] string? name = null) =>
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
}
