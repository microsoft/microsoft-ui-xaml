// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Runtime.CompilerServices;
using Microsoft.UI.Dispatching;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;
using TableViewSampleApp.Data;
using TableViewSampleApp.Models;

namespace TableViewSampleApp.Pages;

/// <summary>
/// Exercises wrapped content inside <c>TableViewTemplateColumn</c> cells so the
/// sample can verify whether realized rows grow to match variable cell height.
/// </summary>
public sealed partial class TextWrapPage : Page, INotifyPropertyChanged
{
    private readonly List<WeakReference<TextBlock>> _bioCells = new();
    private double _columnWidth = 360;
    private string _columnWidthText = string.Empty;
    private string _statusText = string.Empty;

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

    public TextWrapPage()
    {
        InitializeComponent();

        foreach (var person in PersonData.Take(24))
        {
            People.Add(person);
        }

        DemoTable.ItemsSource = People;
        BioColumn.Width = new GridLength(_columnWidth);
        GetTextWrapState().TextWrapping = TextWrapping.Wrap;
        GetTextWrapState().TextTrimming = TextTrimming.None;
        UpdateStatus();

        Loaded += (_, _) => QueueStatusUpdate();
    }

    public ObservableCollection<Person> People { get; } = new();

    public string ColumnWidthText
    {
        get => _columnWidthText;
        private set
        {
            if (_columnWidthText != value)
            {
                _columnWidthText = value;
                OnPropertyChanged();
            }
        }
    }

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

    private void OnWrapToggled(object sender, RoutedEventArgs e)
    {
        if (WrapToggle is null)
        {
            return;
        }

        var state = GetTextWrapState();
        state.TextWrapping = WrapToggle.IsOn ? TextWrapping.Wrap : TextWrapping.NoWrap;
        state.TextTrimming = WrapToggle.IsOn ? TextTrimming.None : TextTrimming.CharacterEllipsis;
        QueueStatusUpdate();
    }

    private void OnColumnWidthChanged(object sender, Microsoft.UI.Xaml.Controls.Primitives.RangeBaseValueChangedEventArgs e)
    {
        _columnWidth = e.NewValue;
        if (BioColumn is not null)
        {
            BioColumn.Width = new GridLength(_columnWidth);
        }
        QueueStatusUpdate();
    }

    private void OnMaxLinesSelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        if (MaxLinesComboBox?.SelectedItem is not ComboBoxItem item ||
            item.Tag is not string tag ||
            !int.TryParse(tag, out var maxLines))
        {
            return;
        }

        var state = GetTextWrapState();
        state.MaxLines = maxLines;
        state.TextTrimming = maxLines == 0 && state.TextWrapping == TextWrapping.Wrap
            ? TextTrimming.None
            : TextTrimming.CharacterEllipsis;
        QueueStatusUpdate();
    }

    private void OnBioTextBlockLoaded(object sender, RoutedEventArgs e)
    {
        if (sender is TextBlock textBlock)
        {
            _bioCells.Add(new WeakReference<TextBlock>(textBlock));
            QueueStatusUpdate();
        }
    }

    private void OnBioTextBlockUnloaded(object sender, RoutedEventArgs e)
    {
        PruneBioCells();
        QueueStatusUpdate();
    }

    private void OnBioTextBlockSizeChanged(object sender, SizeChangedEventArgs e) => QueueStatusUpdate();

    private TextWrapState GetTextWrapState() => (TextWrapState)Resources["TextWrapState"];

    private void QueueStatusUpdate()
    {
        DispatcherQueue.TryEnqueue(DispatcherQueuePriority.Low, () =>
        {
            DemoTable?.UpdateLayout();
            UpdateStatus();
        });
    }

    private void UpdateStatus()
    {
        var state = GetTextWrapState();
        ColumnWidthText = $"{_columnWidth:N0} px";

        var cellHeights = GetBioCellHeights();
        var rowHeights = FindRealizedElementHeights(DemoTable, "Row");
        var maxLines = state.MaxLines == 0 ? "unlimited" : state.MaxLines.ToString(System.Globalization.CultureInfo.InvariantCulture);

        StatusText =
            $"Wrapping: {state.TextWrapping} · Bio width: {_columnWidth:N0}px · MaxLines: {maxLines}\n" +
            $"Realized Bio TextBlock heights: {FormatHeights(cellHeights)}\n" +
            $"Realized row-like element heights: {FormatHeights(rowHeights)}";
    }

    private IReadOnlyList<double> GetBioCellHeights()
    {
        PruneBioCells();
        return _bioCells
            .Select(reference => reference.TryGetTarget(out var textBlock) ? textBlock.ActualHeight : 0)
            .Where(height => height > 0)
            .Select(height => Math.Round(height, 1))
            .Distinct()
            .OrderBy(height => height)
            .ToArray();
    }

    private void PruneBioCells()
    {
        _bioCells.RemoveAll(reference => !reference.TryGetTarget(out _));
    }

    private static IReadOnlyList<double> FindRealizedElementHeights(DependencyObject? root, string typeNamePart)
    {
        var heights = new List<double>();
        if (root is null)
        {
            return heights;
        }

        void Visit(DependencyObject node)
        {
            if (node is FrameworkElement element &&
                element.ActualHeight > 0 &&
                element.GetType().Name.Contains(typeNamePart, StringComparison.OrdinalIgnoreCase))
            {
                heights.Add(Math.Round(element.ActualHeight, 1));
            }

            var childCount = VisualTreeHelper.GetChildrenCount(node);
            for (var i = 0; i < childCount; i++)
            {
                Visit(VisualTreeHelper.GetChild(node, i));
            }
        }

        Visit(root);
        return heights.Distinct().OrderBy(height => height).Take(8).ToArray();
    }

    private static string FormatHeights(IReadOnlyList<double> heights) =>
        heights.Count == 0
            ? "(none realized yet)"
            : $"{heights.Count} distinct: {string.Join(", ", heights.Select(height => $"{height:N1}px"))}";

    public event PropertyChangedEventHandler? PropertyChanged;

    private void OnPropertyChanged([CallerMemberName] string? name = null) =>
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
}

public sealed class TextWrapState : INotifyPropertyChanged
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
