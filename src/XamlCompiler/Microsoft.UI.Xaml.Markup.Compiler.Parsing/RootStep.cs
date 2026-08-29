// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Xaml;

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    public class RootStep : BindPathStep
    {
        public RootStep(XamlType valueType, bool isElementRoot = false)
            : base(valueType, null, null)
        {
            IsElementRoot = isElementRoot;
        }

        public override bool IsIncludedInUpdate => true;

        public override string UniqueName => "";

        public override bool NeedsCheckForNull => IsElementRoot ? false : base.NeedsCheckForNull;

        public bool IsElementRoot;
    }
}
