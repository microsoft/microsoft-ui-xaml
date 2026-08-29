// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
namespace MUXControlsTestApp
{
    /// <summary>
    /// Attribute indicating that this page can be used for AXE testing.
    /// A detailed explanation of the decision behind two separate attributes can be found in the <see cref="TestInventory"/> file.
    /// </summary>
    public class AxeScanTestPageAttribute : Attribute
    {
        public string Name { get; set; } = "NoName";

        public override string ToString()
        {
            return Name;
        }
    }
}
