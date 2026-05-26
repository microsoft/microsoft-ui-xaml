// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Xaml.WidgetSpinner.Metadata;

namespace Microsoft.Xaml.WidgetSpinner.Model
{
    public class XamlPredicateAndArgs
    {
        public XamlType PredicateType { get; }
        public string Arguments { get; }

        public XamlPredicateAndArgs(XamlType predicateType, string arguments)
        {
            PredicateType = predicateType;
            Arguments = arguments;
        }
    }
}
