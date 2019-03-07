// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using System;
using Windows.ApplicationModel.DataTransfer;
using Windows.Foundation;
using Windows.Foundation.Metadata;
using Windows.System;
using Windows.UI.Core;
using Windows.UI.Text;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Automation;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Documents;
using Windows.UI.Xaml.Input;

#if !BUILD_WINDOWS
using CommandBarFlyout = Microsoft.UI.Xaml.Controls.CommandBarFlyout;
#endif

namespace MUXControlsTestApp
{
    public sealed partial class CommandBarFlyoutPage : TestPage
    {
        public CommandBarFlyoutPage()
        {
            this.InitializeComponent();

            if (ApiInformation.IsTypePresent("Windows.UI.Xaml.Input.KeyboardAccelerator"))
            {
                UndoButton1.KeyboardAccelerators.Add(new KeyboardAccelerator() { Key = VirtualKey.Z, Modifiers = VirtualKeyModifiers.Control });
                UndoButton2.KeyboardAccelerators.Add(new KeyboardAccelerator() { Key = VirtualKey.Z, Modifiers = VirtualKeyModifiers.Control });
                UndoButton3.KeyboardAccelerators.Add(new KeyboardAccelerator() { Key = VirtualKey.Z, Modifiers = VirtualKeyModifiers.Control });
                UndoButton4.KeyboardAccelerators.Add(new KeyboardAccelerator() { Key = VirtualKey.Z, Modifiers = VirtualKeyModifiers.Control });
                UndoButton5.KeyboardAccelerators.Add(new KeyboardAccelerator() { Key = VirtualKey.Z, Modifiers = VirtualKeyModifiers.Control });
                UndoButton6.KeyboardAccelerators.Add(new KeyboardAccelerator() { Key = VirtualKey.Z, Modifiers = VirtualKeyModifiers.Control });

                RedoButton1.KeyboardAccelerators.Add(new KeyboardAccelerator() { Key = VirtualKey.Y, Modifiers = VirtualKeyModifiers.Control });
                RedoButton2.KeyboardAccelerators.Add(new KeyboardAccelerator() { Key = VirtualKey.Y, Modifiers = VirtualKeyModifiers.Control });
                RedoButton3.KeyboardAccelerators.Add(new KeyboardAccelerator() { Key = VirtualKey.Y, Modifiers = VirtualKeyModifiers.Control });
                RedoButton4.KeyboardAccelerators.Add(new KeyboardAccelerator() { Key = VirtualKey.Y, Modifiers = VirtualKeyModifiers.Control });
                RedoButton5.KeyboardAccelerators.Add(new KeyboardAccelerator() { Key = VirtualKey.Y, Modifiers = VirtualKeyModifiers.Control });
                RedoButton6.KeyboardAccelerators.Add(new KeyboardAccelerator() { Key = VirtualKey.Y, Modifiers = VirtualKeyModifiers.Control });

                SelectAllButton1.KeyboardAccelerators.Add(new KeyboardAccelerator() { Key = VirtualKey.A, Modifiers = VirtualKeyModifiers.Control });
                SelectAllButton2.KeyboardAccelerators.Add(new KeyboardAccelerator() { Key = VirtualKey.A, Modifiers = VirtualKeyModifiers.Control });
                SelectAllButton3.KeyboardAccelerators.Add(new KeyboardAccelerator() { Key = VirtualKey.A, Modifiers = VirtualKeyModifiers.Control });
                SelectAllButton4.KeyboardAccelerators.Add(new KeyboardAccelerator() { Key = VirtualKey.A, Modifiers = VirtualKeyModifiers.Control });
                SelectAllButton5.KeyboardAccelerators.Add(new KeyboardAccelerator() { Key = VirtualKey.A, Modifiers = VirtualKeyModifiers.Control });
                SelectAllButton6.KeyboardAccelerators.Add(new KeyboardAccelerator() { Key = VirtualKey.A, Modifiers = VirtualKeyModifiers.Control });
            }

            if (ApiInformation.IsPropertyPresent("Windows.UI.Xaml.UIElement", "ContextFlyout"))
            {
                FlyoutTarget1.ContextFlyout = Flyout1;
                FlyoutTarget2.ContextFlyout = Flyout2;
                FlyoutTarget3.ContextFlyout = Flyout3;
                FlyoutTarget4.ContextFlyout = Flyout4;
                FlyoutTarget5.ContextFlyout = Flyout5;
                FlyoutTarget6.ContextFlyout = Flyout6;
            }

            if (ApiInformation.IsEnumNamedValuePresent("Windows.UI.Xaml.Controls.Primitives.FlyoutPlacementMode", "TopEdgeAlignedLeft"))
            {
                Flyout1.Placement = FlyoutPlacementMode.TopEdgeAlignedLeft;
                Flyout2.Placement = FlyoutPlacementMode.TopEdgeAlignedLeft;
                Flyout3.Placement = FlyoutPlacementMode.TopEdgeAlignedLeft;
                Flyout4.Placement = FlyoutPlacementMode.TopEdgeAlignedLeft;
                Flyout5.Placement = FlyoutPlacementMode.TopEdgeAlignedLeft;
                Flyout6.Placement = FlyoutPlacementMode.TopEdgeAlignedLeft;
            }
        }

        public void OnElementClicked(object sender, object args)
        {
            RecordEvent(sender, "clicked");
        }

        public void OnElementChecked(object sender, object args)
        {
            RecordEvent(sender, "checked");
        }

        public void OnElementUnchecked(object sender, object args)
        {
            RecordEvent(sender, "unchecked");
        }

        public void OnFlyoutOpened(object sender, object args)
        {
            IsFlyoutOpenCheckBox.IsChecked = true;
        }

        public void OnFlyoutClosed(object sender, object args)
        {
            IsFlyoutOpenCheckBox.IsChecked = false;
        }

        private void RecordEvent(string eventString)
        {
            StatusReportingTextBox.Text = eventString;
        }

        private void RecordEvent(object sender, string eventString)
        {
            DependencyObject senderAsDO = sender as DependencyObject;

            if (senderAsDO != null)
            {
                RecordEvent(AutomationProperties.GetAutomationId(senderAsDO) + " " + eventString);
            }
        }

        private void OnFlyoutTarget1Click(object sender, RoutedEventArgs e)
        {
            ShowFlyoutAt(Flyout1, FlyoutTarget1);
        }

        private void OnFlyoutTarget2Click(object sender, RoutedEventArgs e)
        {
            ShowFlyoutAt(Flyout2, FlyoutTarget2);
        }

        private void OnFlyoutTarget3Click(object sender, RoutedEventArgs e)
        {
            ShowFlyoutAt(Flyout3, FlyoutTarget3);
        }

        private void OnFlyoutTarget4Click(object sender, RoutedEventArgs e)
        {
            ShowFlyoutAt(Flyout4, FlyoutTarget4);
        }

        private void OnFlyoutTarget5Click(object sender, RoutedEventArgs e)
        {
            ShowFlyoutAt(Flyout5, FlyoutTarget5);
        }

        private void OnFlyoutTarget6Click(object sender, RoutedEventArgs e)
        {
            ShowFlyoutAt(Flyout6, FlyoutTarget6, FlyoutShowMode.Standard);
        }

        private void ShowFlyoutAt(FlyoutBase flyout, FrameworkElement targetElement, FlyoutShowMode showMode = FlyoutShowMode.Transient)
        {
            if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone5))
            {
                flyout.ShowAt(targetElement, new FlyoutShowOptions { Placement = FlyoutPlacementMode.TopEdgeAlignedLeft, ShowMode = showMode });
            }
            else
            {
                flyout.Placement = FlyoutPlacementMode.Top;
                flyout.ShowAt(targetElement);
            }
        }
    }
}
