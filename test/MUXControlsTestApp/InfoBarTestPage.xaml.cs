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
using MUXControlsTestApp.Shared;
using InfoBar; 



// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=234238

namespace MUXControlsTestApp.Shared
{
    [TopLevelTestPage(Name = "InfoBar")]
    public sealed partial class InfoBarTestPage : Page
    {


        InfoBarSeverity severity;
        IconSource icon;
        String title;
        String message;
        String actionButtonContent;
        String closeButtonContent;
        bool open;


        public InfoBarTestPage()
        {
            this.InitializeComponent();
        }

        private async void Test_ActionButtonClick(object sender, RoutedEventArgs e)
        {
            await new MessageDialog("Thank you, mate").ShowAsync();
        }

        private async void Test_CloseButtonClick(object sender, InfoBarEventArgs e)
        {
            e.IsHandled = true;
            await new MessageDialog("Thank you, mate").ShowAsync();
        }

        private async void Test_Closing(InfoBar sender, InfoBarClosingEventArgs args)
        {

            await new MessageDialog("Thank you, mate im closing").ShowAsync();
        }

        private async void Test_Closed(InfoBar sender, InfoBarClosedEventArgs args)
        {
            await new MessageDialog("Thank you, mate im closed").ShowAsync();
        }

        private void ComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            string severityName = e.AddedItems[0].ToString();

            switch (severityName)
            {
                case "Critical":
                    severity = InfoBarSeverity.Critical;
                    break;
                case "Warning":
                    severity = InfoBarSeverity.Warning;
                    break;
                case "Informational":
                    severity = InfoBarSeverity.Informational;
                    break;
                case "Success":
                    severity = InfoBarSeverity.Success;
                    break;
                case "Default":
                    severity = InfoBarSeverity.Default;
                    break;
                case "None":
                    severity = InfoBarSeverity.None;
                    break;
            }
        }

        private void SeverityButton_Click(object sender, RoutedEventArgs e)
        {
            Test.Severity = severity;
        }

        private void IconComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            string iconName = e.AddedItems[0].ToString();

            switch (iconName)
            {
                case "Pin Icon":

                    SymbolIconSource sym2 = new SymbolIconSource();
                    sym2.Symbol = Symbol.Pin;

                    icon = (IconSource)sym2;
                    break;
                case "No Icon":
                    SymbolIconSource sym3 = new SymbolIconSource();
                    sym3.Symbol = new Symbol();
                    icon = sym3;
                    break;

            }
        }

        private void IconButton_Click(object sender, RoutedEventArgs e)
        {
            Test.IconSource = icon;
        }

        private void TitleComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            string iconName = e.AddedItems[0].ToString();

            switch (iconName)
            {
                case "Short Title":
                    title = "Short Title.";
                    break;
                case "Long Title":
                    title = "Long Title. Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.";
                    break;
                case "No Title":
                    title = "";
                    break;
            }
        }

        private void TitleButton_Click(object sender, RoutedEventArgs e)
        {
            Test.Title = title;
        }

        private void MessageComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            string iconName = e.AddedItems[0].ToString();

            switch (iconName)
            {
                case "Short Message":
                    message = "Short Message.";
                    break;
                case "Long Message":
                    message = "Long Message. Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.";
                    break;
                case "No Message":
                    message = "";
                    break;
            }
        }

        private void MessageButton_Click(object sender, RoutedEventArgs e)
        {
            Test.Message = message;
        }

        private void CloseButtonContentComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            string iconName = e.AddedItems[0].ToString();

            switch (iconName)
            {
                case "Short Text":
                    closeButtonContent = "C:Short";
                    break;
                case "Long Text":
                    closeButtonContent = "C:LongTextLorem ipsum dolor sit amet.";
                    break;
                case "No Text":
                    closeButtonContent = "";
                    break;
            }
        }

        private void CloseButtonContent_Click(object sender, RoutedEventArgs e)
        {
            Test.CloseButtonContent = closeButtonContent;
        }

        private void ActionButtonContentComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            string iconName = e.AddedItems[0].ToString();

            switch (iconName)
            {
                case "Short Text":
                    actionButtonContent = "A:Short";
                    break;
                case "Long Text":
                    actionButtonContent = "A:LongTextLorem ipsum dolor sit amet.";
                    break;
                case "No Text":
                    actionButtonContent = "";
                    break;
            }
        }

        private void ActionButtonContent_Click(object sender, RoutedEventArgs e)
        {
            Test.ActionButtonContent = actionButtonContent;
        }

        private void IsOpen_Checked(object sender, RoutedEventArgs e)
        {
            open = true;
        }

        private void IsOpen_Unchecked(object sender, RoutedEventArgs e)
        {
            open = false;
        }

        private void IsOpenButton_Click(object sender, RoutedEventArgs e)
        {
            Test.IsOpen = open;
        }
    }
}
