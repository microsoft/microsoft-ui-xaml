// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Reflection;

namespace MUXControlsTestApp
{
    class TestInventory
    {
        static TestInventory()
        {
            Tests = new List<TestDeclaration>();
            foreach(Type type in typeof(TestInventory).GetTypeInfo().Assembly.GetTypes())
            {
                var attribute = type.GetTypeInfo().GetCustomAttribute<TopLevelTestPageAttribute>();
                if(attribute != null)
                {
                    Tests.Add(new TestDeclaration() 
                    {
                        PageType = type,
                        Name = attribute.Name,
                        AutomationName = attribute.Name + " Tests",
                        Icon = "ms-appx:///Assets/" + attribute.Icon,
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