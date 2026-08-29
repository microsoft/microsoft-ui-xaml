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

            // Why are we using two different properties and are separating normal test pages from AXE test pages?
            // 
            // The main issue with treating them the same is that AXE test pages offer not much beyond being available for AXE testing
            // On the other hand normal test pages are very likely to fail regular AXE tests so can't be used for that.
            // 
            // To prevent the list of test pages to be crowded and bloated with all the AXE test pages, we separate them into two inventories.
            // In some cases, the test page for a control might work for AXE testing too.
            // In that case, the page can have both attributes in order to indicate that it's a regular test page and an AXE test page.
            // To enable that, we have the logic below to sort the pages into the correct lists so they can be picked up by the UI for rendering.

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