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

        public string Name { get; set; }

        private string automationName = string.Empty;
        public string AutomationName
        {
            get
            {
                if(string.IsNullOrWhiteSpace(automationName))
                {
                    return Name + " Tests";
                }
                else
                {
                    return automationName;
                }
            }

            set
            {
                automationName = value;
            }
        }

        public string Icon { get; set; } = "DefaultIcon.png";

        public Type PageType { get; set; }
    }
}