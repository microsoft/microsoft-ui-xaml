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
            AxeTests = new List<TestDeclaration>();
            foreach (Type type in typeof(TestInventory).GetTypeInfo().Assembly.GetTypes())
            {
                var attributeTest = type.GetTypeInfo().GetCustomAttribute<TopLevelTestPageAttribute>();
                if(attributeTest != null)
                {
                    Tests.Add(new TestDeclaration() 
                    {
                        PageType = type,
                        Name = attributeTest.Name,
                        Icon = "ms-appx:///Assets/" + attributeTest.Icon,
                    });
                }

                var attributeAxe = type.GetTypeInfo().GetCustomAttribute<AxeScanTestPageAttribute>();
                if (attributeAxe != null)
                {
                    AxeTests.Add(new TestDeclaration() {
                        PageType = type,
                        Name = attributeAxe.Name,
                        Icon = ""
                    });
                }
            }

            Tests.Sort((a, b) =>
            {
                return a.Name.CompareTo(b.Name);
            });
            AxeTests.Sort((a, b) =>
            {
                return a.Name.CompareTo(b.Name);
            });
        }

        public static List<TestDeclaration> Tests { get; private set; }

        public static List<TestDeclaration> AxeTests { get; private set; }
    }
}