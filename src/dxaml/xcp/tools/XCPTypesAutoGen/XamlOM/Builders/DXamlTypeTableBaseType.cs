// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using SilverlightXasd;

namespace XamlOM
{
    /// <summary>
    /// Specifies what the base type is of this type in the type table.
    /// </summary>
    [AttributeUsage(AttributeTargets.Class, Inherited = false)]
    public class DXamlTypeTableBaseTypeAttribute : Attribute, IClassBuilder
    {
        public Type BaseType
        {
            get;
            private set;
        }

        public DXamlTypeTableBaseTypeAttribute(Type type)
        {
            BaseType = type;
        }

        public void BuildClass(ClassDefinition definition, Type source)
        {
            definition.DXamlTypeTableBaseIndexName = Helper.GetIndexName(BaseType);
        }
    }
}
