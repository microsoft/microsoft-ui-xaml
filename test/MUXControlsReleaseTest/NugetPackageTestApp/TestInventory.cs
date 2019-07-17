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
                new TestDeclaration("CompactDictionary Tests", typeof(CompactDictionaryTestPage)),
                new TestDeclaration("NavigationView with custom resources Tests", typeof(NavigationViewWithCustomResourcesTestPage)),
                new TestDeclaration("PullToRefresh Tests", typeof(PullToRefreshTestPage)),
                new TestDeclaration("Repeater Tests", typeof(RepeaterTestPage))
            };
        }

        public static List<TestDeclaration> Tests { get; private set; }
    }
}
