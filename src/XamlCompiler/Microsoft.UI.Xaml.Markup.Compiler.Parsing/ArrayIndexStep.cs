// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Xaml;

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    public class ArrayIndexStep : BindPathStep
    {
        public int Index { get; }

        public ArrayIndexStep(int index, XamlType valueType, BindPathStep parent, ApiInformation apiInformation)
            : base(valueType, parent, apiInformation)
        {
            Index = index;
        }

        public override string UniqueName => string.Format("I{0}", Index);
    }
}
