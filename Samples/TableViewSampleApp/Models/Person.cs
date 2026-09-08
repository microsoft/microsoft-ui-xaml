// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.ComponentModel;
using System.Runtime.CompilerServices;
// Tabular-namespace type aliases: disambiguate from the stale-mock base Microsoft.UI.Xaml.Controls TableView projection
using TableView = Microsoft.UI.Xaml.Controls.Tabular.TableView;
using TableViewTemplateColumn = Microsoft.UI.Xaml.Controls.Tabular.TableViewTemplateColumn;
using TableViewTextColumn = Microsoft.UI.Xaml.Controls.Tabular.TableViewTextColumn;

namespace TableViewSampleApp.Models;

/// <summary>
/// Realistic POCO used by the gallery's "directory of people" sample.
/// Implements INotifyPropertyChanged so future capability pages (live updates,
/// live updates, sort/filter) can mutate properties and have the table
/// reflect changes once the corresponding TableView phase ships.
/// </summary>
public sealed class Person : INotifyPropertyChanged
{
    private string _firstName = string.Empty;
    private string _lastName = string.Empty;
    private string _email = string.Empty;
    private string _department = string.Empty;
    private string _role = string.Empty;
    private string _bio = string.Empty;
    private DateTimeOffset _joinDate;
    private TimeSpan _shiftStart;
    private double _salary;
    private bool _isActive;

    public string FirstName
    {
        get => _firstName;
        set => Set(ref _firstName, value);
    }

    public string LastName
    {
        get => _lastName;
        set => Set(ref _lastName, value);
    }

    public string FullName => $"{_firstName} {_lastName}";

    /// <summary>
    /// Single-character initial used by the avatar visual in the
    /// row-template sample. Falls back to '?' when neither name is set.
    /// </summary>
    public string Initial => _firstName.Length > 0
        ? _firstName.Substring(0, 1)
        : (_lastName.Length > 0 ? _lastName.Substring(0, 1) : "?");

    public string Email
    {
        get => _email;
        set => Set(ref _email, value);
    }

    public string Department
    {
        get => _department;
        set => Set(ref _department, value);
    }

    public string Role
    {
        get => _role;
        set => Set(ref _role, value);
    }

    public string Bio
    {
        get => _bio;
        set => Set(ref _bio, value);
    }

    public DateTimeOffset JoinDate
    {
        get => _joinDate;
        set => Set(ref _joinDate, value);
    }

    /// <summary>
    /// Formatted JoinDate suitable for a TableViewTextColumn Binding.
    /// SortPage binds a plain text column to this string (display) while
    /// sorting on the actual <see cref="JoinDate"/> DateTimeOffset via
    /// SortMemberPath="JoinDate" so the order is correct chronologically.
    /// </summary>
    public string JoinDateText => _joinDate.ToString("yyyy-MM-dd", System.Globalization.CultureInfo.InvariantCulture);

    /// <summary>
    /// Start of the daily work shift. Used by the MixedControlsPage sample
    /// to demonstrate a TimePicker hosted directly inside a
    /// TableViewTemplateColumn cell.
    /// </summary>
    public TimeSpan ShiftStart
    {
        get => _shiftStart;
        set => Set(ref _shiftStart, value);
    }

    public double Salary
    {
        get => _salary;
        set => Set(ref _salary, value);
    }

    public bool IsActive
    {
        get => _isActive;
        set => Set(ref _isActive, value);
    }

    public event PropertyChangedEventHandler? PropertyChanged;

    private void Set<T>(ref T field, T value, [CallerMemberName] string? propertyName = null)
    {
        if (Equals(field, value))
        {
            return;
        }
        field = value;
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        if (propertyName is nameof(FirstName) or nameof(LastName))
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(FullName)));
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(Initial)));
        }
        if (propertyName is nameof(JoinDate))
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(JoinDateText)));
        }
    }
}
