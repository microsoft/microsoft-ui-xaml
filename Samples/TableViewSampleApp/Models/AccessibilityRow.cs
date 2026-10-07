// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#nullable enable
using System.ComponentModel;
using System.Globalization;
using System.Runtime.CompilerServices;

namespace TableViewSampleApp.Models;

/// <summary>
/// A fixture-only record: an immutable ID and a deliberately distinct accessible label
/// let reviewers distinguish object identity, displayed text, and explicit template names.
/// No ToString override supplies a fallback row name.
/// </summary>
public sealed class AccessibilityRow : INotifyPropertyChanged
{
    private string _displayText = string.Empty;
    private string _department = string.Empty;

    public AccessibilityRow(int id) => Id = id;

    public int Id { get; }

    public string ExplicitName => $"Explicit label for record {Id.ToString("00", CultureInfo.InvariantCulture)}";

    public string DisplayText
    {
        get => _displayText;
        set => Set(ref _displayText, value);
    }

    public string Department
    {
        get => _department;
        set => Set(ref _department, value);
    }

    public event PropertyChangedEventHandler? PropertyChanged;

    private void Set(ref string field, string value, [CallerMemberName] string? propertyName = null)
    {
        if (field == value) return;
        field = value;
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
    }
}
