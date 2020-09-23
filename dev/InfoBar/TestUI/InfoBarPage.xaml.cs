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
using Microsoft.UI.Xaml.Controls;

using IconSource = Microsoft.UI.Xaml.Controls.IconSource;
using SymbolIconSource = Microsoft.UI.Xaml.Controls.SymbolIconSource;

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

                case "Informational":
                default:
                    TestInfoBar.Severity = InfoBarSeverity.Informational;
                    break;
            }
        }

        private void ActionButtonComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (TestInfoBar == null) return;

            switch (e.AddedItems[0].ToString())
            {
                case "Button":
                    var button = new Button();
                    button.Content = "Action";
                    TestInfoBar.ActionButton = button;
                    break;

                case "Hyperlink":
                    var link = new HyperlinkButton();
                    link.NavigateUri = new Uri("http://www.microsoft.com/");
                    link.Content = "Informational link";
                    TestInfoBar.ActionButton = link;
                    break;

                case "None":
                default:
                    TestInfoBar.ActionButton = null;
                    break;
            }
        }

        private void IconComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (TestInfoBar == null) return;

            switch (e.AddedItems[0].ToString())
            {
                case "Custom Icon":
                    SymbolIconSource symbolIcon = new SymbolIconSource();
                    symbolIcon.Symbol = Symbol.Pin;
                    TestInfoBar.IconSource = (IconSource)symbolIcon;
                    break;

                case "Default Icon":
                default:
                    TestInfoBar.IconSource = null;
                    break;
            }
        }

        public void OnCloseButtonClick(object sender, object args)
        {
            EventListBox.Items.Add("CloseButtonClick");
        }

        public void OnClosing(object sender, InfoBarClosingEventArgs args)
        {
            EventListBox.Items.Add("Closing: " + args.Reason);

            if (CancelCheckBox.IsChecked.Value)
            {
                args.Cancel = true;
            }
        }

        public void OnClosed(object sender, InfoBarClosedEventArgs args)
        {
            EventListBox.Items.Add("Closed: " + args.Reason);
        }

        public void ClearButtonClick(object sender, object args)
        {
            EventListBox.Items.Clear();
        }

        public void HasCustomContentChanged(object sender, object args)
        {
            if (HasCustomContentCheckBox.IsChecked.Value)
            {
                var content = new CheckBox();
                content.Content = "Custom Content";
                content.Margin = new Thickness(0, 0, 0, 6);
                TestInfoBar.Content = content;
            }
            else
            {
                TestInfoBar.Content = null;
            }
        }

        public void CloseStyleChanged(object sender, object args)
        {
            if (CloseButtonStyleCheckBox.IsChecked.Value)
            {
                TestInfoBar.CloseButtonStyle = this.Resources["CustomCloseButtonStyle"] as Style;
            }
            else
            {
                TestInfoBar.CloseButtonStyle = Application.Current.Resources["InfoBarCloseButtonStyle"] as Style;
            }
        }
    }
}
