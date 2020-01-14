// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Windows.UI.Xaml.Controls;

using Layout = Microsoft.UI.Xaml.Controls.Layout;
using FlowLayout = Microsoft.UI.Xaml.Controls.FlowLayout;
using FlowLayoutLineAlignment = Microsoft.UI.Xaml.Controls.FlowLayoutLineAlignment;
using UniformGridLayout = Microsoft.UI.Xaml.Controls.UniformGridLayout;
using UniformGridLayoutItemsJustification = Microsoft.UI.Xaml.Controls.UniformGridLayoutItemsJustification;
using UniformGridLayoutItemsStretch = Microsoft.UI.Xaml.Controls.UniformGridLayoutItemsStretch;
using StackLayout = Microsoft.UI.Xaml.Controls.StackLayout;

namespace MUXControlsTestApp.Samples
{
    public static class LayoutHelper
    {
        public static void SetMaxRowsOrColumns(Layout layout, int value)
        {
            if (layout is UniformGridLayout)
            {
                ((UniformGridLayout)layout).MaximumRowsOrColumns = value;
            }
        }

        public static void SetMinRowSpacing(Layout layout, double value)
        {
            if (layout is UniformGridLayout)
            {
                ((UniformGridLayout)layout).MinRowSpacing = value;
            }
            else if (layout is FlowLayout)
            {
                ((FlowLayout)layout).MinRowSpacing = value;
            }
            else if (layout is StackLayout)
            {
                // no-op
            }
        }

        public static void SetMinColumnSpacing(Layout layout, double value)
        {
            if (layout is UniformGridLayout)
            {
                ((UniformGridLayout)layout).MinColumnSpacing = value;
            }
            else if (layout is FlowLayout)
            {
                ((FlowLayout)layout).MinColumnSpacing = value;
            }
            else if (layout is StackLayout)
            {
                ((StackLayout)layout).Spacing = value;
            }
        }

        public static void SetLineAlignment(Layout layout, string value)
        {
            if (layout is UniformGridLayout)
            {
                var alignment = (UniformGridLayoutItemsJustification)Enum.Parse(typeof(UniformGridLayoutItemsJustification), value);
                ((UniformGridLayout)layout).ItemsJustification = alignment;
            }
            else if (layout is FlowLayout)
            {
                var alignment = (FlowLayoutLineAlignment)Enum.Parse(typeof(FlowLayoutLineAlignment), value);
                ((FlowLayout)layout).LineAlignment = alignment;
            }
            else if (layout is StackLayout)
            {
                // no-op
            }
        }

        public static void SetItemsStretch(Layout layout, string value)
        {
            if (layout is UniformGridLayout)
            {
                var stretch = (UniformGridLayoutItemsStretch)Enum.Parse(typeof(UniformGridLayoutItemsStretch), value);
                ((UniformGridLayout)layout).ItemsStretch = stretch;
            }
            else
            {
                // no-op
            }
        }
    }
}
