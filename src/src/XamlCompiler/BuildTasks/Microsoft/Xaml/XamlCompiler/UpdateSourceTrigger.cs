// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    public enum UpdateSourceTrigger
    {
        Default,            // LostFocus for TextBox.Text, PropertyChanged for anything else
        PropertyChanged,    // Only works if property is DP (supports notifications)
        LostFocus,          // Only works if types supports LostFocus
        Explicit,           // Not supported
    }
}