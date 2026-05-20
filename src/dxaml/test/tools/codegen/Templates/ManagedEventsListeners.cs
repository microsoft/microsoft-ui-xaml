// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using Microsoft.UI.Xaml;

namespace Codegen.Templates
{
    public partial class ManagedEventsListeners
    {
        public IEnumerable<Type> GetTargetedTypes()
        {
            string[] targetNamespaces =
            {
                "Microsoft.UI.Xaml",
                "Microsoft.UI.Xaml.Controls",
                "Microsoft.UI.Xaml.Controls.Primitives"
            };
            
            Assembly a = typeof(UIElement).Assembly;
            return a.GetTypes().Where(t => t.IsClass && targetNamespaces.Contains(t.Namespace));
        }

        public IEnumerable<EventInfo> GetTargetedEvents(Type type)
        {
            return type.GetEvents(BindingFlags.Instance | BindingFlags.Public | BindingFlags.DeclaredOnly);
        }
    }
}
