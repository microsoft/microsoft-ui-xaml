// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;

using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;

using Private.Infrastructure;
using Microsoft.UI.Xaml.Tests.Common;

using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Input;
using Windows.Foundation;
using Microsoft.UI.Xaml.Markup;

namespace Microsoft.UI.Xaml.Tests.Focus.XYFocus
{
    [TestClass]
    public class XYFocusKeyboardNavigationTests : XamlTestsBase
    {
        [ClassInitialize]
        [TestProperty("BinaryUnderTest", "Microsoft.UI.Xaml.dll")]
        [TestProperty("RunAs", "UAP")]
        [TestProperty("UAP:Praid", "XamlManagedTAEFTests")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("VelocityTestPass:OneCoreStrict", "Desktop")]
        public static void Setup(TestContext context)
        {
            AssemblySetup.CommonTestClassSetup();
        }

        [ClassCleanup]
        public void ClassCleanup()
        {
            base.CommonClassCleanup();
        }

        private void DirectionToMove(FocusNavigationDirection direction)
        {
            if (direction == FocusNavigationDirection.Left)
            {
                Log.Comment("Press left on keyboard");
                CommonInputHelper.Left(CommonInputHelper.InputDevice.Keyboard);
            }
            else if (direction == FocusNavigationDirection.Right)
            {
                Log.Comment("Press right on keyboard");
                CommonInputHelper.Right(CommonInputHelper.InputDevice.Keyboard);
            }
            else if (direction == FocusNavigationDirection.Up)
            {
                Log.Comment("Press up on keyboard");
                CommonInputHelper.Up(CommonInputHelper.InputDevice.Keyboard);
            }
            else if (direction == FocusNavigationDirection.Down)
            {
                Log.Comment("Press down on keyboard");
                CommonInputHelper.Down(CommonInputHelper.InputDevice.Keyboard);
            }
        }

        private void ArrowThroughGroup(FocusNavigationDirection direction, UIElement[] elementGroup)
        {
            for (int count = 0; count < elementGroup.Length; count++)
            {
                using (var eventTester = new EventTester<UIElement, RoutedEventArgs>(elementGroup[count], "GotFocus"))
                {
                    DirectionToMove(direction);
                    eventTester.Wait();

                    Verify.IsTrue(eventTester.HasFired);
                }
            }
        }

        /*                                                                                                Enabled
           +------------------------------------------------------------------------------------------------+
           |                                                                                                |
           |   +-----------+    +----------+   +-----------+   +-----------+  +----------+  +-----------+   |
           |   |     B     |    |    B     |   |    B      |   |    B      |  |    B     |  |   B       |   |
           |   |           |    |          |   |           |   |           |  |          |  |           |   |
           |   +-----------+    +----------+   +-----------+   +-----------+  +----------+  +-----------+   |
           |                                                                                                |
           |                                                                                                |
           +------------------------------------------------------------------------------------------------+
       */

        [TestMethod]
        [TestProperty("Description", "Verify that we can arrow through a region where XYFocusKeyboardNavigation is set to Enabled")]
        public void NavigateThroughRegion()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Horizontal' XYFocusKeyboardNavigation='Enabled' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <Button Width='50' x:Name='button1' Content='Button Group 1'/>
                            <Button Width='50' x:Name='button2'  Content='Button2 Group 1'/>
                            <Button Width='50' x:Name='button3' Content='Button3 Group 1'/>
                            <Button x:Name='button4' Width='50' Content='Button Group 2'/>
                            <Button x:Name='button5' Width='50' Content='Button 2 Group 2'/>
                            <Button x:Name='button6' Width='50' Content='Button 3 Group 2'/>
                    </StackPanel>";

            StackPanel rootPanel = null;

            Button button1 = null;
            Button button2 = null;
            Button button3 = null;
            Button button4 = null;
            Button button5 = null;
            Button button6 = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);

                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");
                button3 = (Button)rootPanel.FindName("button3");
                button4 = (Button)rootPanel.FindName("button4");
                button5 = (Button)rootPanel.FindName("button5");
                button6 = (Button)rootPanel.FindName("button6");

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            Button[] buttonGroup = { button2, button3, button4, button5, button6 };
            ArrowThroughGroup(FocusNavigationDirection.Right, buttonGroup);
        }

        /*                                                                                               Disabled
           +------------------------------------------------------------------------------------------------+
           |                                                                                                |
           |   +-----------+    +----------+   +-----------+   +-----------+  +----------+  +-----------+   |
           |   |     B     |    |    B     |   |    B      |   |    B      |  |    B     |  |   B       |   |
           |   |           |    |          |   |           |   |           |  |          |  |           |   |
           |   +-----------+    +----------+   +-----------+   +-----------+  +----------+  +-----------+   |
           |                                                                                                |
           |                                                                                                |
           +------------------------------------------------------------------------------------------------+
       */

        [TestMethod]
        [TestProperty("Description", "Verify that we cannot arrow through a region where XYFocusKeyboardNavigation is set to Disabled/not specified")]
        public void CannotNavigateThroughRegion()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Horizontal' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <Button Width='50' x:Name='button1' Content='Button Group 1'/>
                            <Button Width='50' x:Name='button2'  Content='Button2 Group 1'/>
                            <Button Width='50' x:Name='button3' Content='Button3 Group 1'/>
                            <Button x:Name='button4' Width='50' Content='Button Group 2'/>
                            <Button x:Name='button5' Width='50' Content='Button 2 Group 2'/>
                            <Button x:Name='button6' Width='50' Content='Button 3 Group 2'/>
                    </StackPanel>";

            StackPanel rootPanel = null;

            Button button1 = null;
            Button button2 = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);

                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            // We should move focus
            using (var eventTester = new EventTester<Button, RoutedEventArgs>(button2, "GotFocus"))
            {
                Log.Comment("Press right on keyboard");
                CommonInputHelper.Right(CommonInputHelper.InputDevice.Keyboard);
                eventTester.WaitForNoThrow(TimeSpan.FromMilliseconds(1000));

                Verify.IsFalse(eventTester.HasFired);

                UIExecutor.Execute(() =>
                {
                    Verify.IsTrue(FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot).Equals(button1));
                });
            }
        }

        /*
                                                                                                       Enabled
            +------------------------------------------------------------------------------------------------+
            |----------------------------------------------------------------------------------------------+ |
            ||  +-----------+    +----------+   +-----------+   +-----------+  +----------+  +-----------+ | |
            ||  |     B     |    |    B     |   |    B      |   |    B      |  |    B     |  |   B       | | |
            ||  |           |    |          |   |           |   |           |  |          |  |           | | |
            ||  +-----------+    +----------+   +-----------+   +-----------+  +----------+  +-----------+ | |
            ||                                                                                             | |
            |----------------------------------------------------------------------------------------------+ |
            +------------------------------------------------------------------------------------------------+
        */

        [TestMethod]
        [TestProperty("Description", "Verify that a nested element will bubble up to an element that has XYFocusKeyboardNavigation set to Enabled and move focus")]
        public void NavigateThroughNestedRegion()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Horizontal' XYFocusKeyboardNavigation='Enabled' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <StackPanel Orientation='Horizontal'>
                            <Button Width='50' x:Name='button1' Content='Button Group 1'/>
                            <Button Width='50' x:Name='button2'  Content='Button2 Group 1'/>
                            <Button Width='50' x:Name='button3' Content='Button3 Group 1'/>
                            <Button x:Name='button4' Width='50' Content='Button Group 2'/>
                            <Button x:Name='button5' Width='50' Content='Button 2 Group 2'/>
                            <Button x:Name='button6' Width='50' Content='Button 3 Group 2'/>
                        </StackPanel>
                    </StackPanel>";

            StackPanel rootPanel = null;

            Button button1 = null;
            Button button2 = null;
            Button button3 = null;
            Button button4 = null;
            Button button5 = null;
            Button button6 = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);

                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");
                button3 = (Button)rootPanel.FindName("button3");
                button4 = (Button)rootPanel.FindName("button4");
                button5 = (Button)rootPanel.FindName("button5");
                button6 = (Button)rootPanel.FindName("button6");

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            Button[] buttonGroup = { button2, button3, button4, button5, button6 };
            ArrowThroughGroup(FocusNavigationDirection.Right, buttonGroup);
        }


        /*                                                                                                        Enabled
            +-----------------------------------------------------------------------------------------------------------+
            |                                             Enabled                                              Enabled  |
            |    +---------------------------------------------+ +--------------------------------------------------+   |
            |    |  +----------+   +----------+  +-----------+ | | +-----------+   +-------------+  +------------+  |   |
            |    |  |    B     |   |    B     |  |    B      | | | |    B      |   |      B      |  |     B      |  |   |
            |    |  |          |   |          |  |           | | | |           |   |             |  |            |  |   |
            |    |  +----------+   +----------+  +-----------+ | | +-----------+   +-------------+  +------------+  |   |
            |    |                                             | |                                                  |   |
            |    +---------------------------------------------+ +--------------------------------------------------+   |
            |                                                                                                           |
            +-----------------------------------------------------------------------------------------------------------+
        */

        [TestMethod]
        [TestProperty("Description", "Arrowing between different regions where XYFocusKeyboardNavigation is set is not okay if the parent has XYFocusKeyboardNavigation set to Enabled")]
        public void NavigateBetweenTwoSeperateRegions()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Horizontal' XYFocusKeyboardNavigation='Enabled' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <StackPanel Orientation='Horizontal' XYFocusKeyboardNavigation='Enabled'>
                            <Button Width='50' x:Name='button1' Content='Button Group 1'/>
                            <Button Width='50' x:Name='button2'  Content='Button2 Group 1'/>
                            <Button Width='50' x:Name='button3' Content='Button3 Group 1'/>
                        </StackPanel>
                        <StackPanel Orientation='Horizontal' XYFocusKeyboardNavigation='Enabled'>
                            <Button x:Name='button4' Width='50' Content='Button Group 2'/>
                            <Button x:Name='button5' Width='50' Content='Button 2 Group 2'/>
                            <Button x:Name='button6' Width='50' Content='Button 3 Group 2'/>
                        </StackPanel>
                    </StackPanel>";

            StackPanel rootPanel = null;

            Button button1 = null;
            Button button2 = null;
            Button button3 = null;
            Button button4 = null;
            Button button5 = null;
            Button button6 = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);

                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");
                button3 = (Button)rootPanel.FindName("button3");
                button4 = (Button)rootPanel.FindName("button4");
                button5 = (Button)rootPanel.FindName("button5");
                button6 = (Button)rootPanel.FindName("button6");

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            Button[] buttonGroup = { button2, button3, button4, button5, button6 };
            ArrowThroughGroup(FocusNavigationDirection.Right, buttonGroup);
        }

        /*                                                                                                      Disabled
           +-----------------------------------------------------------------------------------------------------------+
           |                                             Enabled                                              Enabled  |
           |    +---------------------------------------------+ +--------------------------------------------------+   |
           |    |  +----------+   +----------+  +-----------+ | | +-----------+   +-------------+  +------------+  |   |
           |    |  |    B     |   |    B     |  |    B      | | | |    B      |   |      B      |  |     B      |  |   |
           |    |  |          |   |          |  |           | | | |           |   |             |  |            |  |   |
           |    |  +----------+   +----------+  +-----------+ | | +-----------+   +-------------+  +------------+  |   |
           |    |                                             | |                                                  |   |
           |    +---------------------------------------------+ +--------------------------------------------------+   |
           |                                                                                                           |
           +-----------------------------------------------------------------------------------------------------------+
       */

        [TestMethod]
        [TestProperty("Description", "Arrowing between different regions where XYFocusKeyboardNavigation is set is not okay if the parent has XYFocusKeyboardNavigation set to Disabled")]
        [TestProperty("TestPass:ExcludeOn" ,"WindowsCore")]
        public void DoNotNavigateBetweenRegionsWhenParentIsDisabled()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Horizontal' XYFocusKeyboardNavigation='Disabled' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <StackPanel Orientation='Horizontal' XYFocusKeyboardNavigation='Enabled'>
                            <Button Width='50' x:Name='button1' Content='Button Group 1'/>
                            <Button Width='50' x:Name='button2'  Content='Button2 Group 1'/>
                            <Button Width='50' x:Name='button3' Content='Button3 Group 1'/>
                        </StackPanel>
                        <StackPanel Orientation='Horizontal' XYFocusKeyboardNavigation='Enabled'>
                            <Button x:Name='button4' Width='50' Content='Button Group 2'/>
                            <Button x:Name='button5' Width='50' Content='Button 2 Group 2'/>
                            <Button x:Name='button6' Width='50' Content='Button 3 Group 2'/>
                        </StackPanel>
                    </StackPanel>";

            StackPanel rootPanel = null;

            Button button1 = null;
            Button button2 = null;
            Button button3 = null;
            Button button4 = null;
            Button button5 = null;
            Button button6 = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);

                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");
                button3 = (Button)rootPanel.FindName("button3");
                button4 = (Button)rootPanel.FindName("button4");
                button5 = (Button)rootPanel.FindName("button5");
                button6 = (Button)rootPanel.FindName("button6");

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            TestServices.InputHelper.Tap(button1);
            TestServices.WindowHelper.WaitForIdle();

            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            Button[] buttonGroup = { button2, button3 };
            ArrowThroughGroup(FocusNavigationDirection.Right, buttonGroup);

            // We should not leave the region
            using (var eventTester = new EventTester<Button, RoutedEventArgs>(button4, "GotFocus"))
            {
                Log.Comment("Press right on keyboard");
                CommonInputHelper.Right(CommonInputHelper.InputDevice.Keyboard);
                eventTester.WaitForNoThrow(TimeSpan.FromMilliseconds(1000));

                Verify.IsFalse(eventTester.HasFired);
                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot) as Button;
                    Verify.IsNotNull(focusedElement);
                    Log.Comment($"Focused element: {focusedElement.Name}");
                    Verify.IsTrue(focusedElement.Equals(button3));
                });
            }

            FocusHelper.EnsureFocus(button6, FocusState.Keyboard);
            Button[] buttonGroupB = { button5, button4 };
            ArrowThroughGroup(FocusNavigationDirection.Left, buttonGroupB);

            // We should not leave the region
            using (var eventTester = new EventTester<Button, RoutedEventArgs>(button4, "GotFocus"))
            {
                Log.Comment("Press left on keyboard");
                CommonInputHelper.Left(CommonInputHelper.InputDevice.Keyboard);
                eventTester.WaitForNoThrow(TimeSpan.FromMilliseconds(1000));

                Verify.IsFalse(eventTester.HasFired);

                UIExecutor.Execute(() =>
                {
                    Verify.IsTrue(FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot).Equals(button4));
                });
            }
        }

        /*
                                                                                                                Enabled
       +-------------------------------------------------------------------------------------------------------------+
       |                                                                             Disabled                        |
       |                                              +--------------------------------------+                       |
       |                                              |                                      |                       |
       |    +----------+  +---------+  +-----------+  |  +-----------+    +-------------+    | +-----------+         |
       |    |          |  |         |  |           |  |  |           |    |             |    | |           |         |
       |    |          |  |         |  |           |  |  |           |    |             |    | |           |         |
       |    |          |  |         |  |           |  |  |           |    |             |    | |           |         |
       |    +----------+  +---------+  +-----------+  |  +-----------+    +-------------+    | +-----------+         |
       |                                              |                                      |                       |
       |                                              |                                      |                       |
       |                                              +--------------------------------------+                       |
       |                                                                                                             |
       +-------------------------------------------------------------------------------------------------------------+
       */

        [TestMethod]
        [TestProperty("Description", "When there is a Disabled region between two candidates, we should arrow over the Disabled region")]
        public void SkipDisabledRegionWhenArrowingWithinEnabledRegion()
        {
            const string rootPanelXaml =
                    @"<StackPanel XYFocusKeyboardNavigation='Enabled' Orientation='Horizontal' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <Button Width='50' x:Name='button1' Content='Button Group 1'/>
                        <Button Width='50' x:Name='button2'  Content='Button2 Group 1'/>
                        <Button Width='50' x:Name='button3' Content='Button3 Group 1'/>
                        <StackPanel Orientation='Horizontal' XYFocusKeyboardNavigation='Disabled'>
                            <Button Width='50' Content='Button Group 2'/>
                            <Button Width='50' Content='Button 2 Group 2'/>
                        </StackPanel>
                        <Button x:Name='button6' Width='50' Content='Button 3 Group 2'/>
                    </StackPanel>";

            StackPanel rootPanel = null;

            Button button1 = null;
            Button button2 = null;
            Button button3 = null;
            Button button6 = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);

                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");
                button3 = (Button)rootPanel.FindName("button3");
                button6 = (Button)rootPanel.FindName("button6");

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            Button[] buttonGroup = { button2, button3, button6 };
            ArrowThroughGroup(FocusNavigationDirection.Right, buttonGroup);
        }

        /*
                                                                                                                                                                                        Enabled
        +------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
        |                                                                                                                                                                                    |
        |                                                                                                                                                                                    |
        |                                                                                                                                                    Disabled                        |
        |                                                 +---------------------------------------------------------------------------------------------------------+                        |
        |                                                 |                                                    +------------------------------------------------+   |                        |
        |                                                 |                                                    |                                                |   |                        |
        |     +----------+   +----------+    +----------+ |   +------------+   +------------+    +-----------+ |  +------------+   +-----------+  +-----------+ |   | +-----------+          |
        |     |          |   |          |    |          | |   |            |   |            |    |           | |  |            |   |           |  |           | |   | |           |          |
        |     |    B     |   |    B     |    |    B     | |   |     B      |   |     B      |    |    B      | |  |    B       |   |     B     |  |    B      | |   | |      B    |          |
        |     |          |   |          |    |          | |   |            |   |            |    |           | |  |            |   |           |  |           | |   | |           |          |
        |     +----------+   +----------+    +----------+ |   +------------+   +------------+    +-----------+ |  +------------+   +-----------+  +-----------+ |   | +-----------+          |
        |                                                 |                                                    |                                                |   |                        |
        |                                                 |                                                    +------------------------------------------------+   |                        |
        |                                                 |                                                                                               Enabled   |                        |
        |                                                 |                                                                                                         |                        |
        |                                                 +---------------------------------------------------------------------------------------------------------+                        |
        |                                                                                                                                                                                    |
        |                                                                                                                                                                                    |
        |                                                                                                                                                                                    |
        |                                                                                                                                                                                    |
        +------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
        */

        [TestMethod]
        [TestProperty("Description", "Test that we cannot navigate to Enabled region within Disabled region. If we somehow focus within the inner Enabled region, we can navigate between elements in that region, but cannot leave")]
        [TestProperty("Hosting:Mode", "UAP")] // fails in WPF mode
        public void CannotNavigateToEnabledRegionWithinDisabledRegion()
        {
            const string rootPanelXaml =
                    @"<StackPanel XYFocusKeyboardNavigation='Enabled' Orientation='Horizontal' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <Button Width='50' x:Name='outsideButton1' Content='O1'/>
                        <Button Width='50' x:Name='outsideButton2'  Content='O2'/>
                        <Button Width='50' x:Name='outsideButton3' Content='O3'/>
                        <StackPanel XYFocusKeyboardNavigation='Disabled' Orientation='Horizontal'>
                            <Button x:Name='nonFocusButton' Width='50' Content='No Keyboard'/>
                            <Button Width='50' Content='No1'/>
                            <Button Width='50' Content='No2'/>
                            <StackPanel XYFocusKeyboardNavigation='Enabled' Orientation='Horizontal'>
                                <Button Width='50' x:Name='innerButton1' Content='I1'/>
                                <Button Width='50' x:Name='innerButton2'  Content='I2'/>
                                <Button Width='50' x:Name='innerButton3' Content='I3'/>
                            </StackPanel>
                        </StackPanel>
                        <Button x:Name='outsideButton4' Width='50' Content='O4'/>
                    </StackPanel>";

            StackPanel rootPanel = null;

            Button outsideButton1 = null;
            Button outsideButton2 = null;
            Button outsideButton3 = null;
            Button outsideButton4 = null;

            Button nonFocusButton = null;

            Button innerButton1 = null;
            Button innerButton2 = null;
            Button innerButton3 = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);

                outsideButton1 = (Button)rootPanel.FindName("outsideButton1");
                outsideButton2 = (Button)rootPanel.FindName("outsideButton2");
                outsideButton3 = (Button)rootPanel.FindName("outsideButton3");
                outsideButton4 = (Button)rootPanel.FindName("outsideButton4");

                nonFocusButton = (Button)rootPanel.FindName("nonFocusButton");

                innerButton1 = (Button)rootPanel.FindName("innerButton1");
                innerButton2 = (Button)rootPanel.FindName("innerButton2");
                innerButton3 = (Button)rootPanel.FindName("innerButton3");

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            FocusHelper.EnsureFocus(outsideButton1, FocusState.Keyboard);

            Button[] outsideButtonGroup = { outsideButton2, outsideButton3 };
            ArrowThroughGroup(FocusNavigationDirection.Right, outsideButtonGroup);

            FocusHelper.EnsureFocus(innerButton1, FocusState.Keyboard);

            Button[] innerButtonGroup = { innerButton2, innerButton3 };
            ArrowThroughGroup(FocusNavigationDirection.Right, innerButtonGroup);

            // We should not be able to arrow onto outsideButton4 since there is a Disabled keyboard region between innerbutton3 and outerbutton4
            using (var eventTester = new EventTester<Button, RoutedEventArgs>(outsideButton4, "GotFocus"))
            {
                Log.Comment("Press right on keyboard");
                CommonInputHelper.Right(CommonInputHelper.InputDevice.Keyboard);
                eventTester.WaitForNoThrow(TimeSpan.FromMilliseconds(1000));

                Verify.IsFalse(eventTester.HasFired);

                UIExecutor.Execute(() =>
                {
                    Verify.IsTrue(FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot).Equals(innerButton3));
                });
            }
        }

        /*
            +------+  +------------------+ +--------+
            |      |  |                  | |        |
            |  B   |  |                  | |   B    |
            |      |  |                  | |        |
            +------+  |                  | +--------+
                      |                  |
            +------+  |                  | +--------+
            |      |  |                  | |        |
            |  B   |  |        B         | |   B    |
            |      |  |                  | |        |
            +------+  |                  | +--------+
                      |                  |
            +------+  |                  | +---------+
            |      |  |                  | |         |
            |  B   |  |                  | |    B    |
            |      |  |                  | |         |
            +------+  +------------------+ +---------+

        */

        [TestMethod]
        [TestProperty("Description", "Verify that we use manifolds when finding the next directional candidate")]
        public void VerifyManifoldsWorkWithDirectionalFocus()
        {
            const string rootPanelXaml =
                    @"<StackPanel XYFocusKeyboardNavigation='Enabled' Orientation='Horizontal' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <StackPanel>
                            <Button x:Name='button1' Width='100' Height='100' Content='Button 1'/>
                            <Button x:Name='button2' Width='100' Height='100' Content='Button 2'/>
                            <Button x:Name='button3' Width='100' Height='100' Content='Button 3'/>
                        </StackPanel>
                        <StackPanel>
                            <Button x:Name='button4' Width='100' Height='300' Content='Button 4'/>
                        </StackPanel>
                        <StackPanel>
                            <Button Width='100' Height='100' Content='Button 5'/>
                            <Button Width='100' Height='100' Content='Button 6'/>
                            <Button x:Name='button7' Width='100' Height='100' Content='Button 7'/>
                        </StackPanel>
                    </StackPanel>";

            StackPanel rootPanel = null;

            Button button1 = null;
            Button button2 = null;
            Button button3 = null;
            Button button4 = null;
            Button button7 = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);

                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");
                button3 = (Button)rootPanel.FindName("button3");
                button4 = (Button)rootPanel.FindName("button4");
                button7 = (Button)rootPanel.FindName("button7");

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            Button[] outsideButtonGroup = { button2, button3 };
            Button[] outsideButtonGroupB = { button4, button7 };
            ArrowThroughGroup(FocusNavigationDirection.Down, outsideButtonGroup);
            ArrowThroughGroup(FocusNavigationDirection.Right, outsideButtonGroupB);
        }

        /*                                                                                                Enabled
          +------------------------------------------------------------------------------------------------+
          |                                                                                                |
          |   +-----------+    +----------+   +-----------+   +-----------+  +----------+  +-----------+   |
          |   |     B     |    |    B     |   |    B      |   |    B      |  |    B     |  |   B       |   |
          |   |           |    |          |   |           |   |           |  |          |  |           |   |
          |   +-----------+    +----------+   +-----------+   +-----------+  +----------+  +-----------+   |
          |                                                                                                |
          |                                                                                                |
          +------------------------------------------------------------------------------------------------+
      */

        [TestMethod]
        [TestProperty("Description", "Using XYFocusOnKeyboard is a keyboard only concept. Using a gamepad should not change the behavior of gamepad input.")]
        public void XYFocusOnKeyboardShouldNotWorkWithGamepad()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Horizontal' XYFocusKeyboardNavigation='Enabled' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <Button Width='50' x:Name='button1' Content='Button Group 1'/>
                            <Button Width='50' x:Name='button2'  Content='Button2 Group 1'/>
                            <Button Width='50' x:Name='button3' Content='Button3 Group 1'/>
                            <Button x:Name='button4' Width='50' Content='Button Group 2'/>
                            <Button x:Name='button5' Width='50' Content='Button 2 Group 2'/>
                            <Button x:Name='button6' Width='50' Content='Button 3 Group 2'/>
                    </StackPanel>";

            StackPanel rootPanel = null;
            Button button6 = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);

                button6 = (Button)rootPanel.FindName("button6");

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            FocusHelper.EnsureFocus(button6, FocusState.Keyboard);

            using (var eventTester = new EventTester<Button, NoFocusCandidateFoundEventArgs>(button6, "NoFocusCandidateFound"))
            {
                // If gamepad was able to work with XYFocusKeyboardNavigation, then we'd get two separate NoFocusCandidateFound events
                CommonInputHelper.Right(CommonInputHelper.InputDevice.Gamepad);

                eventTester.Wait();
                Verify.AreEqual(1, eventTester.ExecuteCount);

                eventTester.WaitForNoThrow(TimeSpan.FromMilliseconds(100));
                Verify.AreEqual(1, eventTester.ExecuteCount);
            }
        }
    }
}
