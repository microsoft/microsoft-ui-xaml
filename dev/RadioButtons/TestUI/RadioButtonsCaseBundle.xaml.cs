// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using MUXControlsTestApp;
using Windows.UI.Xaml.Controls;

namespace RadioButtons_TestUI
{
    [TopLevelTestPage(Name = "RadioButtons", Icon = "RadioButton.png")]
    public sealed partial class RadioButtonsCaseBundle : TestPage
    {
        public RadioButtonsCaseBundle()
        {
            this.InitializeComponent();

            RadioButtonsPage.Click += delegate { Frame.NavigateWithoutAnimation(typeof(RadioButtonsPage), 0); };
            RadioButtonsFocusPage.Click += delegate { Frame.NavigateWithoutAnimation(typeof(RadioButtonsFocusPage), 0); };
        }
    }
}
