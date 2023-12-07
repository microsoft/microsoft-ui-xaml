// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;

namespace XamlOM
{
    /// <summary>
    /// Specifies that the type is a framework-only type, and should not be included in the core type table.
    /// </summary>
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Interface | AttributeTargets.Struct | AttributeTargets.Enum, Inherited = false)]
    public class FrameworkTypePatternAttribute : Attribute,
        NewBuilders.IPattern, NewBuilders.IClassBuilder
    {
        public void BuildNewClass(OM.ClassDefinition definition, Type source)
        {
            if (source != typeof(Microsoft.UI.Xaml.Controls.Control) && source != typeof(Microsoft.UI.Xaml.Controls.Panel))
            {
                if (!typeof(Microsoft.UI.Xaml.DependencyObject).IsAssignableFrom(source))
                {
                    definition.CoreCreationMethodName = string.Empty;

                    if (typeof(Microsoft.UI.Xaml.EventArgs).IsAssignableFrom(source))
                    {
                        definition.IsFrameworkEventArgs = true;
                    }
                }
                else
                {
                    definition.HasPeerStateAndLogic = true;
                    if (typeof(Microsoft.UI.Xaml.Controls.Control).IsAssignableFrom(source))
                    {
                        definition.IsCustomControl = true;
                    }
                }
            }
        }
    }
}
