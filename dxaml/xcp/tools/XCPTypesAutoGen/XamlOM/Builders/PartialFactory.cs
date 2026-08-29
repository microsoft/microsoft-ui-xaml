// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;

namespace XamlOM
{
    /// <summary>
    /// Causes the class factory to be only partially generated
    /// </summary>
    [AttributeUsage(AttributeTargets.Class, Inherited = false)]
    public class PartialFactoryAttribute : Attribute, NewBuilders.IClassBuilder
    {
        public PartialFactoryAttribute()
        {
        }

        public void BuildNewClass(OM.ClassDefinition definition, Type source)
        {
            definition.GeneratePartialFactory = true;
        }
    }
}