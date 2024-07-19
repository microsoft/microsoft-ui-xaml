﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Microsoft.UI.Xaml.Controls;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.RepeaterTests.Common
{
    public static class LayoutExtension
    {
        public static void SetOrientation(this VirtualizingLayout layout, ScrollOrientation scrollOrientation)
        {
            // Note:
            // The public properties of UniformGridLayout and FlowLayout interpret orientation the opposite to
            // how FlowLayoutAlgorithm interprets it. 
            // For simplicity, all of our test validation code is written in terms that match the implementation.
            // For this reason, we need to switch the orientation whenever we set UniformGridLayout.Orientation 
            // or StackLayout.Orientation.
            if (layout is StackLayout)
            {
                ((StackLayout)layout).Orientation = scrollOrientation.ToLayoutOrientation();
            }
            else if (layout is UniformGridLayout)
            {
                ((UniformGridLayout)layout).Orientation = scrollOrientation.ToOrthogonalLayoutOrientation();
            }
            else if (layout is FlowLayout)
            {
                ((FlowLayout)layout).Orientation = scrollOrientation.ToOrthogonalLayoutOrientation();
            }
            else
            {
                throw new InvalidOperationException("layout unknown");
            }
        }
    }
}
