// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.ComponentModel;
using System.Globalization;
using System.Runtime.CompilerServices;
// Tabular aliases keep the sample code concise.
using TableView = Microsoft.UI.Xaml.Controls.Tabular.TableView;
using TableViewTemplateColumn = Microsoft.UI.Xaml.Controls.Tabular.TableViewTemplateColumn;
using TableViewTextColumn = Microsoft.UI.Xaml.Controls.Tabular.TableViewTextColumn;

namespace TableViewSampleApp.Models;

/// <summary>
/// The person row used by most gallery pages. Implements INotifyPropertyChanged so in-cell
/// editors (two-way bound) and page actions that mutate a row are reflected by the table.
/// Shared cell templates for this model live in Templates\PersonCellTemplates.xaml.
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
    private string? _avatarPath;
    private int _employeeId;
    private string _office = string.Empty;

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
    /// Single-character initial. Falls back to '?' when neither name is set. The shared
    /// </summary>
    public string Initial => _firstName.Length > 0
        ? _firstName.Substring(0, 1)
        : (_lastName.Length > 0 ? _lastName.Substring(0, 1) : "?");

    // AvatarTemplate binds this: PersonPicture derives no initials from an Arabic or Hebrew DisplayName.
    public string Initials => FirstTextElement(_firstName) + FirstTextElement(_lastName);

    private static string FirstTextElement(string value)
    {
        var trimmed = value.Trim();
        return trimmed.Length == 0
            ? string.Empty
            : StringInfo.GetNextTextElement(trimmed, 0).ToUpper(CultureInfo.CurrentCulture);
    }

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
    /// <see cref="JoinDate"/> as a nullable value, for <c>CalendarDatePicker.Date</c> two-way
    /// binding (x:Bind does not convert DateTimeOffset to DateTimeOffset?). Clearing the picker
    /// (null) is ignored, so the row always keeps a join date.
    /// </summary>
    public DateTimeOffset? JoinDateOrNull
    {
        get => _joinDate;
        set
        {
            if (value is { } date)
            {
                JoinDate = date;
            }
        }
    }

    /// <summary>
    /// Start of the daily work shift; edited by the TimePicker cell templates.
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

    /// <summary>
    /// Optional photo as an <c>ms-appx:///</c> URI string, or null. Bind it only through
    /// AvatarImageConverter; the AvatarTemplate shows initials when it is null.
    /// </summary>
    public string? AvatarPath
    {
        get => _avatarPath;
        set => Set(ref _avatarPath, value);
    }

    /// <summary>Stable employee number (1001, 1002, ...).</summary>
    public int EmployeeId
    {
        get => _employeeId;
        set => Set(ref _employeeId, value);
    }

    /// <summary>Office location; one of <c>PersonData.Offices</c>.</summary>
    public string Office
    {
        get => _office;
        set => Set(ref _office, value);
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
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(Initials)));
        }
        if (propertyName is nameof(JoinDate))
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(JoinDateText)));
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(JoinDateOrNull)));
        }
    }
}
