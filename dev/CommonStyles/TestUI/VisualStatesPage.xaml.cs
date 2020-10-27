using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;
using MUXControlsTestApp.Utilities;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "VisualStates")]
    public sealed partial class VisualStatesPage : Page
    {
        private Dictionary<Type, List<string>> controls = new Dictionary<Type, List<string>>()
        {
            {typeof(Button),
                new List<string>(){ "Normal", "PointerOver", "Pressed", "Disabled" } },

            {typeof(ToggleButton),
                new List<string>(){ "Normal", "PointerOver", "Pressed", "Disabled", "Checked", "CheckedPointerOver", "CheckedPressed", "CheckedDisabled", "Indeterminate", "IndeterminatePointerOver", "IndeterminatePressed", "IndeterminateDisabled" } },

            {typeof(Microsoft.UI.Xaml.Controls.DropDownButton),
                new List<string>(){ "Normal", "PointerOver", "Pressed", "Disabled" } },

            {typeof(Microsoft.UI.Xaml.Controls.SplitButton),
                new List<string>{ "Normal", "FlyoutOpen", "TouchPressed", "PrimaryPointerOver", "PrimaryPressed", "SecondaryPointerOver", "SecondaryPressed" } },

            {typeof(Microsoft.UI.Xaml.Controls.ToggleSplitButton),
                new List<string>{ "Checked", "CheckedFlyoutOpen", "CheckedTouchPressed", "CheckedPrimaryPointerOver", "CheckedPrimaryPressed", "CheckedSecondaryPointerOver", "CheckedSecondaryPressed" } },

            {typeof(HyperlinkButton),
                new List<string>(){ "Normal", "PointerOver", "Pressed", "Disabled" } },

            {typeof(CheckBox),
                new List<string>(){ "UncheckedNormal", "UncheckedPointerOver", "UncheckedPressed", "UncheckedDisabled", "CheckedNormal", "CheckedPointerOver", "CheckedPressed", "CheckedDisabled", "IndeterminateNormal", "IndeterminatePointerOver", "IndeterminatePressed", "IndeterminateDisabled" } },

            {typeof(RadioButton),
                new List<string>(){ "Unchecked|Normal", "Unchecked|PointerOver", "Unchecked|Pressed", "Unchecked|Disabled", "Checked|Normal", "Checked|PointerOver", "Checked|Pressed", "Checked|Disabled" } },
            
            {typeof(ToggleSwitch),
                new List<string>(){ "Off|Normal", "Off|PointerOver", "Off|Pressed", "Off|Disabled", "On|Normal", "On|PointerOver", "On|Pressed", "On|Disabled" } },

            {typeof(Slider),
                new List<string>(){ "Normal", "PointerOver", "Pressed", "Disabled" } },
        };

        public VisualStatesPage()
        {
            this.InitializeComponent();

            foreach (var pair in controls)
            {
                ControlStateViewer viewer = new ControlStateViewer();
                viewer.ControlType = pair.Key;
                viewer.States = pair.Value;

                ControlPanel.Children.Add(viewer);
            }
        }
    }
}
