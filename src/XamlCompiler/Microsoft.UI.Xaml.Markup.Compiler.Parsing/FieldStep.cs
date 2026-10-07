// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Reflection;
using System.Xaml;

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    public class FieldStep : PropertyStep
    {
        public string FieldName { get; }
        public FieldInfo FieldInfo { get; }

        public FieldStep(string name, XamlType valueType, BindPathStep parent, ApiInformation apiInformation)
            : this(name, valueType, parent, apiInformation, null)
        {
        }

        public FieldStep(string name, XamlType valueType, BindPathStep parent, ApiInformation apiInformation, FieldInfo fieldInfo)
            : base(name, valueType, parent, apiInformation, null)
        {
            FieldName = name;
            FieldInfo = fieldInfo;
        }

        public override string UniqueName => FieldName;
    }
}
