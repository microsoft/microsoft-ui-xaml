// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Reflection;
using System.Xaml;

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    public class PropertyStep : BindPathStep
    {
        public string PropertyName { get; }
        public PropertyInfo PropertyInfo { get; }

        public PropertyStep(string name, XamlType valueType, BindPathStep parent, ApiInformation apiInformation)
            : this(name, valueType, parent, apiInformation, null)
        {
        }

        public PropertyStep(string name, XamlType valueType, BindPathStep parent, ApiInformation apiInformation, PropertyInfo propertyInfo)
            : base(valueType, parent, apiInformation)
        {
            PropertyName = name;
            PropertyInfo = propertyInfo;
        }

        public override string UniqueName => PropertyName;

        public override bool IsValueRequired
        {
            get
            {
                if (Parent.ImplementsINDEI)
                {
                    var propertyInfo = this.Parent.ValueType.UnderlyingType.GetProperty(PropertyName);
                    if (propertyInfo != null)
                    {
                        return DirectUI.ReflectionHelper.FindAttributeByShortTypeName(propertyInfo, KnownTypes.RequiredAttribute) != null;
                    }
                }
                return false;
            }
        }
    }
}
