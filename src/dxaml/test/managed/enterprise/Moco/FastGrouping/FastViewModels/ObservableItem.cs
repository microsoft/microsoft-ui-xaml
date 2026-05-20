// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System.ComponentModel;

namespace Microsoft.UI.Xaml.Tests.Enterprise.Moco.FastGrouping
{
    public class ObservableItem
    {
        public string ItemContent0 { get; set; }
        public string ItemContent1 { get; set; }

        public override string ToString()
        {
            if (!string.IsNullOrWhiteSpace(ItemContent0) && !string.IsNullOrWhiteSpace(ItemContent1))
                return ItemContent0 + ":" + ItemContent1;
            else
                return "NoContent";
        }
    }
}
