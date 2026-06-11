// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Reflection;

namespace XamlOM
{
    /// <summary>
    /// Prevents an enum value from being generated in the framework.
    /// </summary>
    [AttributeUsage(AttributeTargets.Field, Inherited = false)]
    public class CoreEnumValueAttribute : Attribute, NewBuilders.IEnumValueBuilder
    {
        public void BuildNewEnumValue(OM.EnumValueDefinition definition, FieldInfo source)
        {
            definition.IdlMemberInfo.IsExcluded = true;
        }
    }
}
