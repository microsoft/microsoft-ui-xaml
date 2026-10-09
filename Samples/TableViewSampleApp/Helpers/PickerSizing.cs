// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;

namespace TableViewSampleApp.Helpers;

// MinWidth="0" alone is not enough: the template's inner FlyoutButton keeps its themed MinWidth
// (242 TimePicker, 296 DatePicker) and is clipped. FitToWidth makes it follow the picker's width.
public static class PickerSizing
{
    private const string FlyoutButtonPartName = "FlyoutButton";

    public static bool GetFitToWidth(DependencyObject element) => (bool)element.GetValue(FitToWidthProperty);

    public static void SetFitToWidth(DependencyObject element, bool value) => element.SetValue(FitToWidthProperty, value);

    public static readonly DependencyProperty FitToWidthProperty =
        DependencyProperty.RegisterAttached("FitToWidth", typeof(bool), typeof(PickerSizing),
            new PropertyMetadata(false, OnFitToWidthChanged));

    private static void OnFitToWidthChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        if (d is not Control picker)
        {
            return;
        }

        picker.Loaded -= OnPickerLoaded;
        if (e.NewValue is true)
        {
            picker.Loaded += OnPickerLoaded;
            if (picker.IsLoaded)
            {
                Apply(picker);
            }
        }
    }

    // Loaded runs after the template is applied, and again when a recycled cell is re-realized.
    private static void OnPickerLoaded(object sender, RoutedEventArgs e)
    {
        if (sender is Control picker)
        {
            Apply(picker);
        }
    }

    private static void Apply(Control picker)
    {
        if (FindFlyoutButton(picker) is { } button && button.MinWidth > picker.MinWidth)
        {
            button.MinWidth = picker.MinWidth;
        }
    }

    private static Button? FindFlyoutButton(DependencyObject root)
    {
        var count = VisualTreeHelper.GetChildrenCount(root);
        for (var i = 0; i < count; i++)
        {
            var child = VisualTreeHelper.GetChild(root, i);
            if (child is Button { Name: FlyoutButtonPartName } button)
            {
                return button;
            }

            if (child is not Control && FindFlyoutButton(child) is { } nested)
            {
                return nested;
            }
        }

        return null;
    }
}
