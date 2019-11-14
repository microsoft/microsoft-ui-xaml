using Microsoft.UI.Xaml.Controls;
using System;

namespace DEPControlsTestApp.ItemsViewPrototype
{
    public class IsGroupEventArgs : EventArgs
    {
        public object Item { get; set; }

        public int Index { get; set; }

        public ItemsRepeater Owner { get; set; }

        public bool IsGroup { get; set; }
    }
}
