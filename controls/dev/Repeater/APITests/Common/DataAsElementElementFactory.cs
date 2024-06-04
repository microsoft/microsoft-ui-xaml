// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Microsoft.UI.Xaml.Controls;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.RepeaterTests.Common
{
    class DataAsElementElementFactory : ElementFactory
    {
        protected override UIElement GetElementCore(Microsoft.UI.Xaml.ElementFactoryGetArgs args)
        {
            return args.Data as UIElement;
        }

        protected override void RecycleElementCore(Microsoft.UI.Xaml.ElementFactoryRecycleArgs args)
        {
        }
    }
}
