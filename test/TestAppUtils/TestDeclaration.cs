// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;

namespace MUXControlsTestApp
{
    public class TestDeclaration
    {
        public TestDeclaration(string name, Type pageType, string icon = null)
        {
            Name = name;
            Icon = icon;
            PageType = pageType;
        }

        public string Name { get; private set; }

        public string Icon { get; private set; }

        public Type PageType { get; private set; }
    }
}