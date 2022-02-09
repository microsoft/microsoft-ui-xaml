// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using MUXControlsTestApp;
using System.Collections.Generic;
using System.Diagnostics;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Automation;
using Windows.UI.Xaml.Automation.Peers;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

namespace MUXExperimentalTestApp
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainPage : Page
    {
        public MainPage()
        {
            this.InitializeComponent();

            AutomationProperties.SetName(this, "MainPage");

            var sc = new Microsoft.Experimental.UI.Xaml.Controls.SampleControl();

            Loaded += OnLoaded;
        }

        public List<TestDeclaration> Tests
        {
            get { return TestInventory.Tests; }
        }

        protected override AutomationPeer OnCreateAutomationPeer()
        {
            return new FrameworkElementAutomationPeer(this);
        }

        private void OnLoaded(object sender, RoutedEventArgs e)
        {
            var automationPeer = new FrameworkElementAutomationPeer(this);
            automationPeer.RaiseAutomationEvent(AutomationEvents.AsyncContentLoaded);
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
                Debug.WriteLine("Warning: Couldn't find the TestContentLoadedCheckBox to uncheck in OnNavigatedTo");
            }
        }
    }
}
