// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Generic;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using MUXControlsTestApp.Utilities;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "VisualStates")]
    public sealed partial class VisualStatesPage : Page
    {
        private List<ControlStateViewer> viewers = new List<ControlStateViewer>()
        {
            new ControlStateViewer(typeof(Button),
                new List<string>(){ "Normal", "PointerOver", "Pressed", "Disabled" }),

            new ControlStateViewer(typeof(Button),
                new List<string>(){ "Normal", "PointerOver", "Pressed", "Disabled" },
                Application.Current.Resources["AccentButtonStyle"] as Style),

            new ControlStateViewer(typeof(ToggleButton),
                new List<string>(){ "Normal", "PointerOver", "Pressed", "Disabled", "Checked", "CheckedPointerOver", "CheckedPressed", "CheckedDisabled", "Indeterminate", "IndeterminatePointerOver", "IndeterminatePressed", "IndeterminateDisabled" }),

            new ControlStateViewer(typeof(Microsoft.UI.Xaml.Controls.DropDownButton),
                new List<string>(){ "Normal", "PointerOver", "Pressed", "Disabled" }),

            new ControlStateViewer(typeof(Microsoft.UI.Xaml.Controls.SplitButton),
                new List<string>{ "Normal", "FlyoutOpen", "TouchPressed", "PrimaryPointerOver", "PrimaryPressed", "SecondaryPointerOver", "SecondaryPressed" }),

            new ControlStateViewer(typeof(Microsoft.UI.Xaml.Controls.ToggleSplitButton),
                new List<string>{ "Checked", "CheckedFlyoutOpen", "CheckedTouchPressed", "CheckedPrimaryPointerOver", "CheckedPrimaryPressed", "CheckedSecondaryPointerOver", "CheckedSecondaryPressed" }),

            new ControlStateViewer(typeof(HyperlinkButton),
                new List<string>(){ "Normal", "PointerOver", "Pressed", "Disabled" }),

            new ControlStateViewer(typeof(CheckBox),
                new List<string>(){ "UncheckedNormal", "UncheckedPointerOver", "UncheckedPressed", "UncheckedDisabled", "CheckedNormal", "CheckedPointerOver", "CheckedPressed", "CheckedDisabled", "IndeterminateNormal", "IndeterminatePointerOver", "IndeterminatePressed", "IndeterminateDisabled" }),

            new ControlStateViewer(typeof(RadioButton),
                new List<string>(){ "Unchecked|Normal", "Unchecked|PointerOver", "Unchecked|Pressed", "Unchecked|Disabled", "Checked|Normal", "Checked|PointerOver", "Checked|Pressed", "Checked|Disabled" }),
            
            new ControlStateViewer(typeof(ToggleSwitch),
                new List<string>(){ "Off|Normal", "Off|PointerOver", "Off|Pressed", "Off|Disabled", "On|Normal", "On|PointerOver", "On|Pressed", "On|Disabled" }),

            new ControlStateViewer(typeof(Slider),
                new List<string>(){ "Normal", "PointerOver", "Pressed", "Disabled" }),

            new ControlStateViewer(typeof(TextBox),
                new List<string>(){ "Normal", "PointerOver", "Focused", "Disabled" }),
        };

        public VisualStatesPage()
        {
            this.InitializeComponent();

            foreach (var viewer in viewers)
            {
                ControlPanel.Children.Add(viewer);
            }
        }
    }
}
