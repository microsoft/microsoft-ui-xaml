using Microsoft.UI.Xaml.Controls;
using System;
using System.Collections.Generic;
using System.Text;
using Windows.UI.Xaml.Controls;

namespace MUXControlsTestApp
{
    public sealed partial class PrototypePager : Control
    {
        public Button FirstPageButtonTestHook, PreviousPageButtonTestHook, NextPageButtonTestHook, LastPageButtonTestHook;
        public NumberBox NumberBoxDisplayTestHook;
        public ComboBox ComboBoxDisplayTestHook;
    }
}
