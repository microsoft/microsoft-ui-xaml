// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Xaml;

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    public class FieldStep : PropertyStep
    {
        public string FieldName { get; }

        public FieldStep(string name, XamlType valueType, BindPathStep parent, ApiInformation apiInformation)
            : base(name, valueType, parent, apiInformation)
        {
            FieldName = name;
        }

        public override string UniqueName => FieldName;
    }
}
