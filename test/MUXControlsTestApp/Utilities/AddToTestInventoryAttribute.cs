using System;
using System.Collections.Generic;
using System.Text;

namespace MUXControlsTestApp
{
    public class AddToTestInventoryAttribute: Attribute
    {
        public string Name { get; set; } = "NoName";

        public string Icon { get; set; } = String.Empty;
    }
}
