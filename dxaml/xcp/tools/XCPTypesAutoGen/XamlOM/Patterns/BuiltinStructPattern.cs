// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;

namespace XamlOM
{
    /// <summary>
    /// Specifies that a type is a built-in struct.
    /// </summary>
    [AttributeUsage(AttributeTargets.Struct, Inherited = false)]
    public class BuiltinStructAttribute : 
        Attribute, 
        NewBuilders.IPattern,
        NewBuilders.IClassBuilder
    {
        public string NativeName
        {
            get;
            private set;
        }

        public BuiltinStructAttribute(string nativeName)
        {
            NativeName = nativeName;
        }

        public void BuildNewClass(OM.ClassDefinition definition, Type source)
        {
            definition.BaseClass = (OM.ClassDefinition)NewBuilders.ModelFactory.GetOrCreateType(typeof(Microsoft.UI.Xaml.DependencyObject));
            definition.CoreName = NativeName;
            definition.GenerateInCore = false;
            definition.XamlClassFlags.HasTypeConverter = true;
        }
    }
}
