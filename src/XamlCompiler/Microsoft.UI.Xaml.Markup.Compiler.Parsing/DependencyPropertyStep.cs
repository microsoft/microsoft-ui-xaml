// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Xaml;

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    public class DependencyPropertyStep : PropertyStep
    {
        public XamlType OwnerType { get; }

        public DependencyPropertyStep(string name, XamlType valueType, BindPathStep parent, ApiInformation apiInformation)
            : base(name, valueType, parent, apiInformation)
        {
            // Determie OwnerType based on who declares the property
            // (it may be a base class, so can't just take parent.ValueType)
            // However sometimes we can't find the member on parent type,
            // so we should just fall back to the parent's valuetype itself

            var property = parent.ValueType.GetMember(name);

            if (property?.DeclaringType != null)
            {
                OwnerType = property.DeclaringType;
            }
            else
            {
                OwnerType = parent.ValueType;
            }
        }

        public DependencyPropertyStep(string name, XamlType valueType, XamlType ownerType, BindPathStep parent, ApiInformation apiInformation)
            : base(name, valueType, parent, apiInformation)
        {
            OwnerType = ownerType;
        }

        public override string UniqueName => PropertyName;
    }
}
