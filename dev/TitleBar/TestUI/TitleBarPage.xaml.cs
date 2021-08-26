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

using Microsoft.Experimental.UI.Xaml.Controls;
using Windows.UI.ViewManagement;
using Windows.Foundation;

namespace MUXControlsTestApp
{
    public sealed partial class TitleBarPage : TestPage
    {
        TitleBar _titleBar = null;

        public TitleBarPage()
        {
            this.InitializeComponent();

            _titleBar = new TitleBar();
            _titleBar.Title = "Experimental Controls Test App";
            var icon = new Microsoft.UI.Xaml.Controls.SymbolIconSource();
            icon.Symbol = Symbol.Keyboard;
            _titleBar.IconSource = icon;

            var testFrame = Window.Current.Content as TestFrame;
            testFrame.CustomElement = _titleBar;

            // Set window min size to the smallest width we support.
            ApplicationView.GetForCurrentView().SetPreferredMinSize(new Size(320, 500));
        }

        private void _titleBar_BackRequested(TitleBar sender, object args)
        {
            BackRequestedTextBox.Text = "BackRequested";
        }

        private void IsBackButtonVisibleCheckBox_CheckedChanged(object sender, RoutedEventArgs e)
        {
            if (_titleBar != null)
            {
                _titleBar.IsBackButtonVisible = IsBackButtonVisibleCheckBox.IsChecked.Value;
            }
        }

        private void IsBackEnabledCheckBox_CheckedChanged(object sender, RoutedEventArgs e)
        {
            if (_titleBar != null)
            {
                _titleBar.IsBackEnabled = IsBackEnabledCheckBox.IsChecked.Value;
            }
        }

        private void SetIconCheckBox_CheckedChanged(object sender, RoutedEventArgs e)
        {
            if (_titleBar != null)
            {
                if (SetIconCheckBox.IsChecked.Value)
                {
                    var icon = new Microsoft.UI.Xaml.Controls.SymbolIconSource();
                    icon.Symbol = Symbol.Mail;
                    _titleBar.IconSource = icon;
                }
                else
                {
                    _titleBar.IconSource = null;
                }
            }
        }

        private void SetTitleCheckBox_CheckedChanged(object sender, RoutedEventArgs e)
        {
            if (_titleBar != null)
            {
                _titleBar.Title = SetTitleCheckBox.IsChecked.Value ? "Title" : String.Empty;
            }
        }
        private void CustomContentCheckBox_CheckedChanged(object sender, RoutedEventArgs e)
        {
            if (_titleBar != null)
            {
                if (CustomContentCheckBox.IsChecked.Value)
                {
                    string xaml =
                    @"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                         <Grid.ColumnDefinitions>
                            <ColumnDefinition Width='Auto'/>
                            <ColumnDefinition Width='*' />
                         </Grid.ColumnDefinitions >

                         <Button Content='Left'/>
                         <Button Grid.Column='1' Content='Right' HorizontalAlignment='Right'/>
                    </Grid>";

                    var element = XamlReader.Load(xaml);
                    _titleBar.CustomContent = element;
                }
                else
                {
                    _titleBar.CustomContent = null;
                }
            }
        }
    }
}
