// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Threading;
using System.Windows.Input;
using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using Private.Infrastructure;
using Windows.Foundation;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Tests.Common;

using ICommand = System.Windows.Input.ICommand;

namespace Microsoft.UI.Xaml.Tests.Controls
{
    public sealed class HeavyViewModel
    {
        private readonly byte[] m_heavyPayload = new byte[1024 * 1024 * 10];

        public HeavyViewModel()
        {
            LeakingCommand = new DelegateCommand(Leak);
        }

        public ICommand LeakingCommand { get; }

        public void Leak()
        {
            // ICommand is held by XAML, it holds Action, Action holds HeavyViewModel
        }
    }

    public sealed class DelegateCommand : ICommand
    {
        public event EventHandler CanExecuteChanged;

        private readonly Action m_action;

        public DelegateCommand(Action action)
        {
            m_action = action;

            if (CanExecuteChanged != null)
            {
                CanExecuteChanged(this, null);
            }
        }

        public bool CanExecute(object parameter)
        {
            return true;
        }

        public void Execute(object parameter)
        {
            m_action();
        }
    }

    [TestClass]
    public class CommandBarIntegrationTests : XamlTestsBase
    {
        [ClassInitialize]
        [TestProperty("BinaryUnderTest", "Microsoft.UI.Xaml.dll")]
        [TestProperty("RunAs", "UAP")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("UAP:Praid", "XamlManagedTAEFTests")]
        public static void Setup(TestContext context)
        {
            AssemblySetup.CommonTestClassSetup();
        }

        [ClassCleanup]
        public void ClassCleanup()
        {
            base.CommonClassCleanup();
        }

        [TestMethod]
        [TestProperty("Description", "Validates that we do not leak our view model when used to bind to an AppBarButton's Command property.")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void DoesNotLeakViewModelWithCommandBinding()
        {
            var memoryBefore = GC.GetTotalMemory(true) / (1024 * 1024);

            // Use a separate function scope to ensure that the HeavyViewModel is available for GC.
            var action = new Action(() =>
            {
                var viewModel = new HeavyViewModel();

                // Create our CommandBar and reference our view model via the Command property in one of
                // its AppBarButtons.
                UIExecutor.Execute(() =>
                {
                    var commandBar = new CommandBar();
                    commandBar.PrimaryCommands.Add(new AppBarButton() { Command = viewModel.LeakingCommand });

                    TestServices.WindowHelper.WindowContent = commandBar;
                });
                TestServices.WindowHelper.WaitForIdle();

                // Remove local reference.
                viewModel = null;
            });

            action();

            // Clear tree references so that we can validate that we haven't leaked anything.
            UIExecutor.Execute(() =>
            {
                TestServices.WindowHelper.WindowContent = null;
            });
            TestServices.WindowHelper.WaitForIdle();

            var memoryAfter = GC.GetTotalMemory(true) / (1024 * 1024);

            Verify.AreEqual(memoryBefore, memoryAfter);
        }

        [TestMethod]
        [TestProperty("Description", "Validates that we can click on primary buttons when secondary pane is open")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void ValidateClickOnPrimaryButtonWhenSecondaryIsOpen()
        {
            CommandBar commandBar = null;
            AppBarButton primaryAppBarButton = null;
            AppBarButton secondaryAppBarButton = null;
            ManualResetEvent primaryTapped = new ManualResetEvent(false);

            UIExecutor.Execute(() =>
            {
                commandBar = new CommandBar();
                commandBar.DefaultLabelPosition = CommandBarDefaultLabelPosition.Right;

                primaryAppBarButton = new AppBarButton()
                {
                    Label = "Add Button",
                    Icon = new SymbolIcon(Symbol.Add)
                };

                primaryAppBarButton.Click += (sender, args) =>
                {
                    primaryTapped.Set();
                };

                secondaryAppBarButton = new AppBarButton()
                {
                    Label = "Setting",
                    Icon = new SymbolIcon(Symbol.Setting)
                };

                commandBar.PrimaryCommands.Add(primaryAppBarButton);
                commandBar.SecondaryCommands.Add(secondaryAppBarButton);

                TestServices.WindowHelper.WindowContent = commandBar;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                commandBar.IsOpen = true;
            });

            TestServices.WindowHelper.WaitForIdle();

            TestServices.InputHelper.Tap(primaryAppBarButton);
            Verify.IsTrue(primaryTapped.WaitOne(5000));
        }

        [TestMethod]
        [TestProperty("Description", "Validates that a submenu is shown on the left of its parent menu when no room is available on the right.")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void ValidateSubMenuPositioning()
        {
            TestServices.WindowHelper.SetWindowSizeOverride(new Size(1024, 400));

            Grid rootPanel = null;
            CommandBarFlyout commandBarFlyout = null;
            AppBarButton secondaryAppBarButton = null;
            MenuFlyout menuFlyout = null;
            ManualResetEvent menuFlyoutOpenedEvent = new ManualResetEvent(false);
            ManualResetEvent menuFlyoutClosedEvent = new ManualResetEvent(false);
            ManualResetEvent menuFlyoutItemClickEvent = new ManualResetEvent(false);

            UIExecutor.Execute(() =>
            {
                Log.Comment("Building UI");
                rootPanel = new Grid();
                commandBarFlyout = new CommandBarFlyout();

                secondaryAppBarButton = new AppBarButton()
                {
                    Label = "CommandBarFlyoutItem",
                    Icon = new SymbolIcon(Symbol.Setting)
                };

                commandBarFlyout.SecondaryCommands.Add(secondaryAppBarButton);

                var menuFlyoutItem = new MenuFlyoutItem()
                {
                    Text = "MenuFlyoutItem",
                    Icon = new SymbolIcon(Symbol.Flag),
                    KeyboardAcceleratorTextOverride = "Ctrl+Shift+A"
                };

                menuFlyoutItem.Click += (sender, args) =>
                {
                    Log.Comment("MenuFlyoutItem Click");
                    menuFlyoutItemClickEvent.Set();
                };

                menuFlyout = new MenuFlyout();
                menuFlyout.Items.Add(menuFlyoutItem);

                menuFlyout.Opened += (sender, args) =>
                {
                    Log.Comment("MenuFlyout Opened");
                    menuFlyoutOpenedEvent.Set();
                };

                menuFlyout.Closed += (sender, args) =>
                {
                    Log.Comment("MenuFlyout Closed");
                    menuFlyoutClosedEvent.Set();
                };

                secondaryAppBarButton.Flyout = menuFlyout;

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            TestServices.InputHelper.MoveMouse(rootPanel);
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Log.Comment("Opening CommandBarFlyout at right edge of screen");
                var position = new Point()
                {
                    X = 900,
                    Y = 100
                };

                var flyoutShowOptions = new FlyoutShowOptions()
                {
                    Placement = FlyoutPlacementMode.Auto,
                    Position = position
                };

                commandBarFlyout.ShowAt(rootPanel, flyoutShowOptions);
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Log.Comment("CommandBarFlyout expected open");
                Verify.IsTrue(commandBarFlyout.IsOpen);
            });

            Log.Comment("Hovering mouse over CommandBarFlyout");
            TestServices.InputHelper.MoveMouse(secondaryAppBarButton);
            Verify.IsTrue(menuFlyoutOpenedEvent.WaitOne(5000));
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Log.Comment("MenuFlyout expected open");
                Verify.IsTrue(menuFlyout.IsOpen);
            });

            Log.Comment("Moving mouse over expected MenuFlyout position");
            TestServices.InputHelper.MoveMouse(new Point(675, 95));
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Left-button-clicking over MenuFlyoutItem");
            TestServices.InputHelper.LeftMouseClick(new Point(675, 95));
            Verify.IsTrue(menuFlyoutItemClickEvent.WaitOne(5000));
            Verify.IsTrue(menuFlyoutClosedEvent.WaitOne(5000));
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Log.Comment("Hiding CommandBarFlyout");
                commandBarFlyout.Hide();
            });

            TestServices.WindowHelper.WaitForIdle();
        }
    }
}
