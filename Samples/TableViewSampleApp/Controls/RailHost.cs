// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Windows.Foundation;

namespace TableViewSampleApp.Controls;

// Hosts the options rail. With FillsSlot the rail reports no height, so the Example alone sizes the
// row and the rail scrolls inside it instead of making the page taller.
public sealed partial class RailHost : Panel
{
    private bool _fillsSlot;
    private double _maxRailHeight = double.PositiveInfinity;
    private double _slotHeight = double.NaN;

    public bool FillsSlot
    {
        get => _fillsSlot;
        set
        {
            if (_fillsSlot != value)
            {
                _fillsSlot = value;
                _slotHeight = double.NaN;
                InvalidateMeasure();
            }
        }
    }

    public double MaxRailHeight
    {
        get => _maxRailHeight;
        set
        {
            if (!_maxRailHeight.Equals(value))
            {
                _maxRailHeight = value;
                InvalidateMeasure();
            }
        }
    }

    protected override Size MeasureOverride(Size availableSize)
    {
        var height = Math.Min(availableSize.Height, MaxRailHeight);
        if (FillsSlot && !double.IsNaN(_slotHeight))
        {
            height = Math.Min(height, _slotHeight);
        }

        double width = 0, desiredHeight = 0;
        foreach (var child in Children)
        {
            child.Measure(new Size(availableSize.Width, height));
            width = Math.Max(width, child.DesiredSize.Width);
            desiredHeight = Math.Max(desiredHeight, child.DesiredSize.Height);
        }

        return new Size(width, FillsSlot ? 0 : desiredHeight);
    }

    protected override Size ArrangeOverride(Size finalSize)
    {
        // The slot is only known here; measure the rail again against it on the next pass.
        if (FillsSlot && (double.IsNaN(_slotHeight) || Math.Abs(finalSize.Height - _slotHeight) > 0.5))
        {
            _slotHeight = finalSize.Height;
            InvalidateMeasure();
        }

        var height = Math.Min(finalSize.Height, MaxRailHeight);
        foreach (var child in Children)
        {
            child.Arrange(new Rect(0, 0, finalSize.Width, height));
        }

        return finalSize;
    }
}
