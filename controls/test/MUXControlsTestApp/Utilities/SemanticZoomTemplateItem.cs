// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.ObjectModel;

namespace MUXControlsTestApp
{
    public class SemanticZoomTemplateItem
    {
        public SemanticZoomTemplateItem()
        {
            TemplateNames = new string[0];
        }
        public SemanticZoomTemplateItem(string name, string template)
        {
            DisplayName = name;
            TemplateNames = new string[] { template };
        }
        public string DisplayName { get; set; }
        public string[] TemplateNames { get; set; }

        public override string ToString()
        {
            return DisplayName;
        }
    }
}
