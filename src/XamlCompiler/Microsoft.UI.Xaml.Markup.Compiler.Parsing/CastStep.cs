// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Xaml;

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    public class CastStep : BindPathStep
    {
        public CastStep(XamlType valueType, BindPathStep parent, ApiInformation apiInformation)
            : base(valueType, parent, apiInformation)
        {
        }

        public override string UniqueName
        {
            get
            {
                var toCastName = Parent is RootStep ? "Root" : Parent.UniqueName;
                return string.Format("Cast_{0}_To_{1}", toCastName, ValueType.Name);
            }
        }
    }
}
