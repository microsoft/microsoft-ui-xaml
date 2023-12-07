// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using System;

namespace XamlOM
{
    /// <summary>
    /// Specifies in which .g.h file the enum needs to be generated.
    /// </summary>
    public class NativeCategoryAttribute : Attribute, NewBuilders.IEnumBuilder
    {
        public EnumCategory Category
        {
            get;
            private set;
        }

        public NativeCategoryAttribute(EnumCategory category)
        {
            Category = category;
        }

        public void BuildNewEnum(OM.EnumDefinition definition, Type source)
        {
            definition.IsAutomationEnum = Category == EnumCategory.AutomationEnum;
            definition.IsAutomationCoreEnum = Category == EnumCategory.AutomationCoreEnum;
        }
    }
}
