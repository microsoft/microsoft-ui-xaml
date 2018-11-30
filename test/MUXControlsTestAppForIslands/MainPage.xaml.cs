// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using MUXControlsTestApp;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Automation;
using Windows.UI.Xaml.Automation.Peers;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;

namespace MUXControlsTestAppForIslands
{
    public sealed partial class MainPage : Page
    {
        public MainPage()
        {
            this.InitializeComponent();

            AutomationProperties.SetName(this, "MainPage");
            Loaded += OnLoaded;
        }

        protected override AutomationPeer OnCreateAutomationPeer()
        {
            return new FrameworkElementAutomationPeer(this);
        }

        private void OnLoaded(object sender, RoutedEventArgs e)
        {
            var automationPeer = new FrameworkElementAutomationPeer(this);
            automationPeer.RaiseAutomationEvent(AutomationEvents.AsyncContentLoaded);
            itemsControl.ItemsSource = Tests;
            NavigateToTestCommand.Frame = this.Frame;
        }

        public List<TestDeclaration> Tests
        {
            get { return TestInventory.Tests; }
        }

        DependencyObject SearchVisualTree(DependencyObject root, string name)
        {
            int size = VisualTreeHelper.GetChildrenCount(root);
            DependencyObject child = null;

            for (int i = 0; i < size && child == null; i++)
            {
                DependencyObject depObj = VisualTreeHelper.GetChild(root, i);
                FrameworkElement fe = depObj as FrameworkElement;

                if (fe.Name.Equals(name))
                {
                    child = fe;
                }
                else
                {
                    child = SearchVisualTree(fe, name);
                }
            }

            return child;
        }

        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
            var testContentLoadedCheckBox = SearchVisualTree(this.Frame, "TestContentLoadedCheckBox") as CheckBox;
            if (testContentLoadedCheckBox != null)
            {
                testContentLoadedCheckBox.IsChecked = false;
            }
            else
            {
                //WEX.Logging.Interop.Log.Warning("Warning: Couldn't find the TestContentLoadedCheckBox to uncheck in OnNavigatedTo");
            }
        }
    }
}
