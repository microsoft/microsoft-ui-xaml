// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;

namespace MUXControlsTestApp
{
    public class TestDeclaration
    {
        public TestDeclaration()
        {
        }

        public TestDeclaration(string name, Type pageType)
        {
            Name = name;
            AutomationName = name;
            PageType = pageType;
        }

        public string Name { get; set; }

        public string AutomationName { get; set; }

        public string Icon { get; set; }

        public Type PageType { get; set; }
    }
}