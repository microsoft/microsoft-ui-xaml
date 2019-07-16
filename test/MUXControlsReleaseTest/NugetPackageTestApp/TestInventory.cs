// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using MUXControlsTestApp;
using System.Collections.Generic;

namespace NugetPackageTestApp
{
    class TestInventory
    {
        static TestInventory()
        {
            Tests = new List<TestDeclaration>
            {
                new TestDeclaration()
                {
                    Name = "PullToRefresh Tests",
                    PageType = typeof(PullToRefreshTestPage),
                },
                new TestDeclaration()
                {
                    Name = "CompactDictionary Tests",
                    PageType = typeof(CompactDictionaryTestPage),
                },
                new TestDeclaration()
                {
                    Name = "Repeater Tests",
                    PageType = typeof(RepeaterTestPage),
                },
            };
        }

        public static List<TestDeclaration> Tests { get; private set; }
    }
}
