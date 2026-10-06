// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Windows.Foundation;

namespace TableViewSampleApp.Controls;

/// <summary>
/// Lays children out left to right and starts a new line when the next child does not fit, so a row
/// of options stays on one line in a wide window and wraps instead of being cut off in a narrow one.
/// Children keep their order, so tab order still follows reading order.
/// </summary>
public sealed partial class WrapPanel : Panel
{
    public double HorizontalSpacing { get; set; } = 8;

    public double VerticalSpacing { get; set; } = 8;

    protected override Size MeasureOverride(Size availableSize)
    {
        double lineWidth = 0, lineHeight = 0, width = 0, height = 0;
        var lineIsEmpty = true;
        foreach (var child in Children)
        {
            if (child.Visibility == Visibility.Collapsed)
            {
                continue;
            }

            child.Measure(new Size(availableSize.Width, double.PositiveInfinity));
            var size = child.DesiredSize;
            if (!lineIsEmpty && lineWidth + HorizontalSpacing + size.Width > availableSize.Width)
            {
                width = Math.Max(width, lineWidth);
                height += lineHeight + VerticalSpacing;
                lineWidth = 0;
                lineHeight = 0;
                lineIsEmpty = true;
            }

            lineWidth += (lineIsEmpty ? 0 : HorizontalSpacing) + size.Width;
            lineHeight = Math.Max(lineHeight, size.Height);
            lineIsEmpty = false;
        }

        return new Size(Math.Max(width, lineWidth), height + lineHeight);
    }

    protected override Size ArrangeOverride(Size finalSize)
    {
        var children = Children;
        double y = 0;
        var lineStart = 0;
        while (lineStart < children.Count)
        {
            // Same line breaks as MeasureOverride; children are centred vertically within their line.
            double lineWidth = 0, lineHeight = 0;
            var lineEnd = lineStart;
            var lineIsEmpty = true;
            while (lineEnd < children.Count)
            {
                var child = children[lineEnd];
                if (child.Visibility != Visibility.Collapsed)
                {
                    var size = child.DesiredSize;
                    var next = lineWidth + (lineIsEmpty ? 0 : HorizontalSpacing) + size.Width;
                    if (!lineIsEmpty && next > finalSize.Width)
                    {
                        break;
                    }

                    lineWidth = next;
                    lineHeight = Math.Max(lineHeight, size.Height);
                    lineIsEmpty = false;
                }

                lineEnd++;
            }

            double x = 0;
            for (var i = lineStart; i < lineEnd; i++)
            {
                var child = children[i];
                if (child.Visibility == Visibility.Collapsed)
                {
                    continue;
                }

                var size = child.DesiredSize;
                child.Arrange(new Rect(x, y + ((lineHeight - size.Height) / 2), size.Width, size.Height));
                x += size.Width + HorizontalSpacing;
            }

            y += lineHeight + VerticalSpacing;
            lineStart = lineEnd;
        }

        return finalSize;
    }
}
