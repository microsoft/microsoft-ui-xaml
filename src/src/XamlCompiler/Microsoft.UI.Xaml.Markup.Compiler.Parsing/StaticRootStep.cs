// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Xaml;

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    public class StaticRootStep : BindPathStep
    {
        public StaticRootStep(XamlType staticType, ApiInformation apiInformation)
            : base(staticType, null, apiInformation)
        {
        }

        public override bool IsIncludedInUpdate => false;

        public override string UniqueName => ValueType.UnderlyingType.FullName.GetMemberFriendlyName();

        public override bool NeedsCheckForNull => false;
    }
}
