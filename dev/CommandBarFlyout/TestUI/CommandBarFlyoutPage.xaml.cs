// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using System;
using System.Linq;
using Windows.Foundation.Metadata;
using Windows.System;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Automation;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Input;

using CommandBarFlyout = Microsoft.UI.Xaml.Controls.CommandBarFlyout;

namespace MUXControlsTestApp
{
    public sealed partial class CommandBarFlyoutPage : TestPage
    {
        private DispatcherTimer clearSecondaryCommandsTimer = new DispatcherTimer();
        private CommandBarFlyout clearSecondaryCommandsFlyout;

        private DispatcherTimer clearPrimaryCommandsTimer = new DispatcherTimer();
        private CommandBarFlyout clearPrimaryCommandsFlyout;

        private DispatcherTimer dynamicLabelTimer = new DispatcherTimer();
        private DispatcherTimer dynamicCommandTimer = new DispatcherTimer();
        private AppBarButton dynamicLabelSecondaryCommand;
        private CommandBarFlyout dynamicCommandBarFlyout;
        private string originalLabelSecondaryCommand;
        private int dynamicLabelChangeCount;

        public CommandBarFlyoutPage()
        {
            this.InitializeComponent();

            dynamicLabelTimer.Tick += DynamicLabelTimer_Tick;

            dynamicCommandTimer.Tick += DynamicCommandTimer_Tick;

            clearSecondaryCommandsTimer.Interval = new TimeSpan(0, 0, 3 /*sec*/);
            clearSecondaryCommandsTimer.Tick += ClearSecondaryCommandsTimer_Tick;

            clearSecondaryCommandsTimer.Interval = new TimeSpan(0, 0, 3 /*sec*/);
            clearPrimaryCommandsTimer.Tick += ClearPrimaryCommandsTimer_Tick;

            if (ApiInformation.IsTypePresent("Windows.UI.Xaml.Input.KeyboardAccelerator"))
            {
                UndoButton1.KeyboardAccelerators.Add(new KeyboardAccelerator() { Key = VirtualKey.Z, Modifiers = VirtualKeyModifiers.Control });
                UndoButton2.KeyboardAccelerators.Add(new KeyboardAccelerator() { Key = VirtualKey.Z, Modifiers = VirtualKeyModifiers.Control });
                UndoButton3.KeyboardAccelerators.Add(new KeyboardAccelerator() { Key = VirtualKey.Z, Modifiers = VirtualKeyModifiers.Control });
                UndoButton4.KeyboardAccelerators.Add(new KeyboardAccelerator() { Key = VirtualKey.Z, Modifiers = VirtualKeyModifiers.Control });
                UndoButton5.KeyboardAccelerators.Add(new KeyboardAccelerator() { Key = VirtualKey.Z, Modifiers = VirtualKeyModifiers.Control });
                UndoButton6.KeyboardAccelerators.Add(new KeyboardAccelerator() { Key = VirtualKey.Z, Modifiers = VirtualKeyModifiers.Control });
                UndoButton7.KeyboardAccelerators.Add(new KeyboardAccelerator() { Key = VirtualKey.Z, Modifiers = VirtualKeyModifiers.Control });

                RedoButton1.KeyboardAccelerators.Add(new KeyboardAccelerator() { Key = VirtualKey.Y, Modifiers = VirtualKeyModifiers.Control });
                RedoButton2.KeyboardAccelerators.Add(new KeyboardAccelerator() { Key = VirtualKey.Y, Modifiers = VirtualKeyModifiers.Control });
                RedoButton3.KeyboardAccelerators.Add(new KeyboardAccelerator() { Key = VirtualKey.Y, Modifiers = VirtualKeyModifiers.Control });
                RedoButton4.KeyboardAccelerators.Add(new KeyboardAccelerator() { Key = VirtualKey.Y, Modifiers = VirtualKeyModifiers.Control });
                RedoButton5.KeyboardAccelerators.Add(new KeyboardAccelerator() { Key = VirtualKey.Y, Modifiers = VirtualKeyModifiers.Control });
                RedoButton6.KeyboardAccelerators.Add(new KeyboardAccelerator() { Key = VirtualKey.Y, Modifiers = VirtualKeyModifiers.Control });
                RedoButton7.KeyboardAccelerators.Add(new KeyboardAccelerator() { Key = VirtualKey.Y, Modifiers = VirtualKeyModifiers.Control });

                SelectAllButton1.KeyboardAccelerators.Add(new KeyboardAccelerator() { Key = VirtualKey.A, Modifiers = VirtualKeyModifiers.Control });
                SelectAllButton2.KeyboardAccelerators.Add(new KeyboardAccelerator() { Key = VirtualKey.A, Modifiers = VirtualKeyModifiers.Control });
                SelectAllButton3.KeyboardAccelerators.Add(new KeyboardAccelerator() { Key = VirtualKey.A, Modifiers = VirtualKeyModifiers.Control });
                SelectAllButton4.KeyboardAccelerators.Add(new KeyboardAccelerator() { Key = VirtualKey.A, Modifiers = VirtualKeyModifiers.Control });
                SelectAllButton5.KeyboardAccelerators.Add(new KeyboardAccelerator() { Key = VirtualKey.A, Modifiers = VirtualKeyModifiers.Control });
                SelectAllButton6.KeyboardAccelerators.Add(new KeyboardAccelerator() { Key = VirtualKey.A, Modifiers = VirtualKeyModifiers.Control });
                SelectAllButton7.KeyboardAccelerators.Add(new KeyboardAccelerator() { Key = VirtualKey.A, Modifiers = VirtualKeyModifiers.Control });
            }

            if (ApiInformation.IsPropertyPresent("Windows.UI.Xaml.UIElement", "ContextFlyout"))
            {
                FlyoutTarget1.ContextFlyout = Flyout1;
                FlyoutTarget2.ContextFlyout = Flyout2;
                FlyoutTarget3.ContextFlyout = Flyout3;
                FlyoutTarget4.ContextFlyout = Flyout4;
                FlyoutTarget5.ContextFlyout = Flyout5;
                FlyoutTarget6.ContextFlyout = Flyout6;
                FlyoutTarget7.ContextFlyout = Flyout7;
            }

            if (ApiInformation.IsEnumNamedValuePresent("Windows.UI.Xaml.Controls.Primitives.FlyoutPlacementMode", "TopEdgeAlignedLeft"))
            {
                Flyout1.Placement = FlyoutPlacementMode.TopEdgeAlignedLeft;
                Flyout2.Placement = FlyoutPlacementMode.TopEdgeAlignedLeft;
                Flyout3.Placement = FlyoutPlacementMode.TopEdgeAlignedLeft;
                Flyout4.Placement = FlyoutPlacementMode.TopEdgeAlignedLeft;
                Flyout5.Placement = FlyoutPlacementMode.TopEdgeAlignedLeft;
                Flyout6.Placement = FlyoutPlacementMode.TopEdgeAlignedLeft;
                Flyout7.Placement = FlyoutPlacementMode.TopEdgeAlignedLeft;
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
            SetDynamicSecondaryCommand(null);
            SetClearSecondaryCommandsFlyout(null);
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

        private void OnFlyoutTarget7Click(object sender, RoutedEventArgs e)
        {
            ShowFlyoutAt(Flyout7, FlyoutTarget7);
        }

        private void OnFlyoutTarget8Click(object sender, RoutedEventArgs e)
        {
            ShowFlyoutAt(Flyout8, FlyoutTarget8);
        }

        private void OnFlyoutTarget9Click(object sender, RoutedEventArgs e)
        {
            ShowFlyoutAt(Flyout9, FlyoutTarget9);
        }

        private void OnFlyoutTarget10Click(object sender, RoutedEventArgs e)
        {
            ShowFlyoutAt(Flyout10, FlyoutTarget10);
        }

        private void ShowFlyoutAt(FlyoutBase flyout, FrameworkElement targetElement, FlyoutShowMode showMode = FlyoutShowMode.Transient)
        {
            bool useSecondaryCommandDynamicLabel = (bool)UseSecondaryCommandDynamicLabelCheckBox.IsChecked;
            bool clearSecondaryCommands = (bool)ClearSecondaryCommandsCheckBox.IsChecked;
            bool addPrimaryCommandDynamicallyCheckBox = (bool)AddPrimaryCommandDynamicallyCheckBox.IsChecked;
            bool clearPrimaryCommands = (bool)ClearPrimaryCommandsCheckBox.IsChecked;

            if (useSecondaryCommandDynamicLabel || addPrimaryCommandDynamicallyCheckBox || clearSecondaryCommands || clearPrimaryCommands)
            {
                CommandBarFlyout commandBarFlyout = flyout as CommandBarFlyout;

                if (commandBarFlyout != null)
                {
                    if (commandBarFlyout.SecondaryCommands != null && commandBarFlyout.SecondaryCommands.Count > 0)
                    {
                        if (useSecondaryCommandDynamicLabel)
                        {
                            SetDynamicSecondaryCommand(commandBarFlyout.SecondaryCommands[0] as AppBarButton);
                        }

                        if (clearSecondaryCommands)
                        {
                            SetClearSecondaryCommandsFlyout(commandBarFlyout);
                        }
                    }

                    if (addPrimaryCommandDynamicallyCheckBox)
                    {
                        dynamicCommandBarFlyout = commandBarFlyout;
                        SetDynamicPrimaryCommand();
                    }

                    if (clearPrimaryCommands && commandBarFlyout.PrimaryCommands != null && commandBarFlyout.PrimaryCommands.Count > 0)
                    {
                        SetClearPrimaryCommandsFlyout(commandBarFlyout);
                    }
                }
            }

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

        private void SetClearSecondaryCommandsFlyout(CommandBarFlyout commandBarFlyout)
        {
            if (commandBarFlyout == null)
            {
                clearSecondaryCommandsFlyout = null;
                clearSecondaryCommandsTimer.Stop();
            }
            else
            {
                clearSecondaryCommandsFlyout = commandBarFlyout;
                clearSecondaryCommandsTimer.Start();
            }
        }

        private void SetClearPrimaryCommandsFlyout(CommandBarFlyout commandBarFlyout)
        {
            if (commandBarFlyout == null)
            {
                clearPrimaryCommandsFlyout = null;
                clearPrimaryCommandsTimer.Stop();
            }
            else
            {
                clearPrimaryCommandsFlyout = commandBarFlyout;
                clearPrimaryCommandsTimer.Start();
            }
        }

        private void SetDynamicSecondaryCommand(AppBarButton appBarButton)
        {
            if (appBarButton == null)
            {
                if (dynamicLabelSecondaryCommand != null)
                {
                    if (dynamicLabelSecondaryCommand.Label != originalLabelSecondaryCommand)
                    {
                        dynamicLabelSecondaryCommand.Label = originalLabelSecondaryCommand;
                        SecondaryCommandDynamicLabelChangedCheckBox.IsChecked = true;
                    }
                    dynamicLabelSecondaryCommand = null;
                    dynamicLabelTimer.Stop();
                }
            }
            else
            {
                if (dynamicLabelSecondaryCommand == null)
                {
                    originalLabelSecondaryCommand = appBarButton.Label;
                    dynamicLabelSecondaryCommand = appBarButton;
                    dynamicLabelTimer.Interval = new TimeSpan(0, 0, 0, 0, int.Parse(DynamicLabelTimerIntervalTextBox.Text) /*msec*/);
                    dynamicLabelChangeCount = int.Parse(DynamicLabelChangeCountTextBox.Text);
                    dynamicLabelTimer.Start();
                }
            }
        }

        private void SetDynamicPrimaryCommand()
        {
            dynamicCommandTimer.Interval = new TimeSpan(0, 0, 0, 0, int.Parse(DynamicLabelTimerIntervalTextBox.Text) /*msec*/);
            dynamicLabelChangeCount = int.Parse(DynamicLabelChangeCountTextBox.Text);
            dynamicCommandTimer.Start();
        }

        private void ClearSecondaryCommandsTimer_Tick(object sender, object e)
        {
            if (clearSecondaryCommandsFlyout != null)
            {
                clearSecondaryCommandsFlyout.SecondaryCommands.Clear();
            }
        }

        private void ClearPrimaryCommandsTimer_Tick(object sender, object e)
        {
            if (clearPrimaryCommandsFlyout != null)
            {
                clearPrimaryCommandsFlyout.PrimaryCommands.Clear();
            }
        }

        private void DynamicLabelTimer_Tick(object sender, object e)
        {
            if (dynamicLabelSecondaryCommand != null)
            {
                if (dynamicLabelSecondaryCommand.Label == originalLabelSecondaryCommand)
                {
                    // Testing dynamic label expansion
                    dynamicLabelSecondaryCommand.Label += " with an expanded label";
                }
                else
                {
                    // Testing dynamic label shrinkage
                    dynamicLabelSecondaryCommand.Label = originalLabelSecondaryCommand;
                }
                
                SecondaryCommandDynamicLabelChangedCheckBox.IsChecked = true;

                if (--dynamicLabelChangeCount == 0)
                {
                    dynamicLabelTimer.Stop();
                }
            }
        }


        private void DynamicCommandTimer_Tick(object sender, object e)
        {
            dynamicCommandBarFlyout.PrimaryCommands.Add(new AppBarButton() {
                Content = new TextBlock() { Text = "Test" }
            });

            PrimaryCommandDynamicallyAddedCheckBox.IsChecked = true;

            if (--dynamicLabelChangeCount == 0)
            {
                dynamicCommandTimer.Stop();
            }
        }

        private void IsRTLCheckBox_Checked(object sender, RoutedEventArgs e)
        {
            FlowDirection = FlowDirection.RightToLeft;
        }

        private void IsRTLCheckBox_Unchecked(object sender, RoutedEventArgs e)
        {
            FlowDirection = FlowDirection.LeftToRight;
        }

        private void OnEditCommandCount6Click(object sender, RoutedEventArgs e)
        {
            var flyout6 = Flyout6 as CommandBarFlyout;

            if (flyout6.PrimaryCommands.Count() == 0)
            {
                flyout6.PrimaryCommands.Add(new AppBarButton() {
                    Content = new TextBlock() { Text = "Test" }
                });
            }
            else
            {
                flyout6.PrimaryCommands.RemoveAt(0);
            }
        }
    }
}
