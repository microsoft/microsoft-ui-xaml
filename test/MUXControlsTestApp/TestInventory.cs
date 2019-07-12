// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Reflection;

namespace MUXControlsTestApp
{
    static class Extensions
    {
        public static void Add(this List<TestDeclaration> list, string name, Type pageType)
        {
            list.Add(new TestDeclaration() {
                Name = name,
                AutomationName = name + " Tests",
                PageType = pageType
            });
        }

        public static void Add(this List<TestDeclaration> list, string name, Type pageType, string icon)
        {
            list.Add(new TestDeclaration() {
                Name = name,
                AutomationName = name + " Tests",
                PageType = pageType,
                Icon = "ms-appx:///Assets/" + icon
            });
        }
    }

    class TestInventory
    {
        static TestInventory()
        {
            Tests = new List<TestDeclaration>();
            foreach(Type type in typeof(TestInventory).GetTypeInfo().Assembly.GetTypes())
            {
                var attribute = type.GetTypeInfo().GetCustomAttribute<AddToTestInventoryAttribute>();
                if(attribute != null)
                {
                    Tests.Add(new TestDeclaration() 
                    {
                        PageType = type,
                        Name = attribute.Name,
                        Icon = attribute.Icon,
                    });
                }
            }

            Tests.Sort((a, b) =>
            {
                return a.Name.CompareTo(b.Name);
            });
        }

        public static List<TestDeclaration> Tests { get; private set; }
    }
}