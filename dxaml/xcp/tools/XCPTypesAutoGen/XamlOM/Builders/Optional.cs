// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Reflection;

namespace XamlOM
{
    /// <summary>
    /// Specifies a parameter is optional.
    /// </summary>
    [AttributeUsage(AttributeTargets.Parameter, Inherited = false)]
    public class OptionalAttribute : Attribute, NewBuilders.IParameterBuilder
    {
        public OptionalAttribute()
        {
        }

        public void BuildNewParameter(OM.ParameterDefinition definition, ParameterInfo source)
        {
            definition.ParameterType.IsOptional = true;
        }
    }
}
