using Microsoft.UI.Xaml.Controls;
using System;
using System.Collections.Generic;
using System.Text;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Shapes;

namespace MUXControlsTestApp
{
    public sealed partial class PrototypePager : Control
    {
        public Button FirstPageButtonTestHook, PreviousPageButtonTestHook, NextPageButtonTestHook, LastPageButtonTestHook;
        public NumberBox NumberBoxDisplayTestHook;
        public ComboBox ComboBoxDisplayTestHook;
        public ItemsRepeater NumberPanelDisplayTestHook;
        public Rectangle NumberPanelCurrentPageIdentifierTestHook;

        public int AutoDisplayModeNumberOfPagesThresholdTestHook;
    }
}
