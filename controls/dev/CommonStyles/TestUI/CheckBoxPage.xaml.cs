// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "CheckBox", Icon = "CheckBox.png")]
    public sealed partial class CheckBoxPage : TestPage
    {
        public CheckBoxPage()
        {
            this.InitializeComponent();
        }
        public void OnReverseButtonClicked(object sender, object args)
        {
            var isChecked = ThreeStateCheckbox.IsChecked;
            if (isChecked.HasValue && isChecked.Value)
            {
                ThreeStateCheckbox.IsChecked = false;
            }
            else if(isChecked.HasValue && !isChecked.Value)
            {
                ThreeStateCheckbox.IsChecked = null;
            }
            else if(!isChecked.HasValue)
            {
                ThreeStateCheckbox.IsChecked = true;
            }
        }
    }
}
