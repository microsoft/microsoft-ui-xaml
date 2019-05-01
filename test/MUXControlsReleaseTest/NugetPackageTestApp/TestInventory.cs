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
                new TestDeclaration("PullToRefresh Tests", typeof(PullToRefreshTestPage)),
                new TestDeclaration("CompactDictionary Tests", typeof(CompactDictionaryTestPage))
            };
        }

        public static List<TestDeclaration> Tests { get; private set; }
    }
}
