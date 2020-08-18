// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Markup;
using Windows.UI;
using System.Windows.Input;

using InfoBar = Microsoft.UI.Xaml.Controls.InfoBar;
using InfoBarSeverity = Microsoft.UI.Xaml.Controls.InfoBarSeverity;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "InfoBar")]
    public sealed partial class InfoBarPage : TestPage
    {
        public InfoBarPage()
        {
            this.InitializeComponent();
        }

        private void SeverityComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            string severityName = e.AddedItems[0].ToString();

            switch (severityName)
            {
                case "Critical":
                    TestInfoBar.Severity = InfoBarSeverity.Critical;
                    break;

                case "Warning":
                    TestInfoBar.Severity = InfoBarSeverity.Warning;
                    break;

                case "Success":
                    TestInfoBar.Severity = InfoBarSeverity.Success;
                    break;

                case "Default":
                default:
                    TestInfoBar.Severity = InfoBarSeverity.Default;
                    break;
            }
        }
    }
}
