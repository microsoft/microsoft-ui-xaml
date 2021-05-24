// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using MUXControlsTestApp;
using System.Collections.Generic;

namespace MUXExperimentalTestApp
{
    class TestInventory
    {
        static TestInventory()
        {
            Tests = new List<TestDeclaration>
            {
                new TestDeclaration()
                {
                    Name = "SampleControl Tests",
                    PageType = typeof(SampleControlPage),
                },
            };
        }

        public static List<TestDeclaration> Tests { get; private set; }
    }
}
