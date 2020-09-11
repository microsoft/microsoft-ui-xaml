// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Design.ControlProvider
{
    class NumberBoxPropertyProvider : ControlPropertyProvider
    {
        public override string ControlName => "NumberBox";

        public override void AddProperties()
        {
            RegisterCommonProperty("Minimum");
            RegisterCommonProperty("Maximum");
            RegisterCommonProperty("Value");
            RegisterCommonProperty("Text");
            RegisterCommonProperty("SmallChange");
            RegisterCommonProperty("LargeChange");
            RegisterCommonProperty("SpinButtonPlacementMode");
            RegisterCommonProperty("ValidationMode");
            RegisterCommonProperty("AcceptsExpression");
        }
    }
}
