﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using Windows.Foundation;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.RepeaterTests.Common.Mocks
{
    partial class MockNonVirtualizingLayout : NonVirtualizingLayout
    {
        public Func<Size, NonVirtualizingLayoutContext, Size> MeasureLayoutFunc { get; set; }
        public Func<Size, NonVirtualizingLayoutContext, Size> ArrangeLayoutFunc { get; set; }

        public new void InvalidateMeasure()
        {
            base.InvalidateMeasure();
        }

        protected override Size MeasureOverride(NonVirtualizingLayoutContext context, Size availableSize)
        {
            return MeasureLayoutFunc != null ? MeasureLayoutFunc(availableSize, context) : default(Size);
        }

        protected override Size ArrangeOverride(NonVirtualizingLayoutContext context, Size finalSize)
        {
            return ArrangeLayoutFunc != null ? ArrangeLayoutFunc(finalSize, context) : default(Size);
        }
    }
}
