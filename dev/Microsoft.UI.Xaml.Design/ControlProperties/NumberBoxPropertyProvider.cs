// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml.Controls;
using Microsoft.Windows.Design.Metadata;
using System;

namespace Microsoft.UI.Xaml.Design.ControlProvider
{
    class NumberBoxPropertyProvider : ControlPropertyProvider
    {
        public NumberBoxPropertyProvider(AttributeTableBuilder builder) : base(builder) { }

        public override Type controlType => typeof(NumberBox);

        public override void AddProperties()
        {
            RegisterCommonProperty("Minimum");
            RegisterCommonProperty("Maximum");
            RegisterCommonProperty("Value");
            RegisterCommonProperty("SmallChange");
            RegisterCommonProperty("LargeChange");
            RegisterCommonProperty("SpinButtonPlacementMode");
            RegisterCommonProperty("ValidationMode");
            RegisterCommonProperty("AcceptsExpression");
        }
    }
}
