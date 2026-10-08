// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.ComponentModel;
using System.Runtime.CompilerServices;
using Microsoft.UI.Xaml;

namespace TableViewSampleApp.Models;

/// <summary>
/// Wrapping state shared by every Bio cell through x:Bind. Raises PropertyChanged so realized
/// cells update in place when the rail changes.
/// </summary>
public sealed partial class TextWrapState : INotifyPropertyChanged
{
    private TextWrapping _textWrapping = TextWrapping.Wrap;
    private TextTrimming _textTrimming = TextTrimming.None;
    private int _maxLines;

    public TextWrapping TextWrapping
    {
        get => _textWrapping;
        set => Set(ref _textWrapping, value);
    }

    public TextTrimming TextTrimming
    {
        get => _textTrimming;
        set => Set(ref _textTrimming, value);
    }

    public int MaxLines
    {
        get => _maxLines;
        set => Set(ref _maxLines, value);
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
    }
}
