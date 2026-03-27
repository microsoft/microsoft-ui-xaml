// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;

using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;

using Private.Infrastructure;
using Microsoft.UI.Xaml.Tests.Common;

using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Documents;
using Microsoft.UI.Xaml.Input;
using Windows.Foundation;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Media;

namespace Microsoft.UI.Xaml.Tests.Focus.XYFocus
{
    [TestClass]
    public class XYFocusTests : XamlTestsBase
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

        private void FlowDirectionRespected(bool respectFlowDirection)
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <Button x:Name='button' Content='button'/>
                        <Button x:Name='button2' Content='button'/>
                        <Button x:Name='button3' Content='button'/>
                    </StackPanel>";

            StackPanel rootPanel = null;
            Button button = null;
            Button button2 = null;
            Button button3 = null;
            Button target = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button = (Button)rootPanel.FindName("button");
                button2 = (Button)rootPanel.FindName("button2");
                button3 = (Button)rootPanel.FindName("button3");

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                button.XYFocusLeft = button2;
                button.XYFocusRight = button3;
                rootPanel.FlowDirection = FlowDirection.RightToLeft;
            });

            FocusHelper.EnsureFocus(button, FocusState.Keyboard);

            if (respectFlowDirection)
            {
                target = button2;
            }
            else
            {
                target = button3;
            }

            using (var eventTester = new EventTester<Button, RoutedEventArgs>(target, "GotFocus"))
            {
                Log.Comment("Press right on gamepad");
                TestServices.KeyboardHelper.GamepadDpadRight();
                eventTester.Wait();

                Verify.IsTrue(eventTester.HasFired);
            }
        }

        [TestMethod]
        [TestProperty("Description", "Validates that if FlowDirection is RTL, the direction overrides respect that")]
        public void VerifyFlowDirectionRespected()
        {
            FlowDirectionRespected(true);
        }

        private void PreferLeft(FlowDirection direction)
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <Button x:Name='button' Width='100' Content='button'/>
                        <StackPanel Orientation='Horizontal'>
                            <Button x:Name='button2' Width='50' Content='button'/>
                            <Button x:Name='button3' Width='50' Content='button'/>
                        </StackPanel>
                    </StackPanel>";

            StackPanel rootPanel = null;
            Button button = null;
            Button button2 = null;
            Button button3 = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button = (Button)rootPanel.FindName("button");
                button2 = (Button)rootPanel.FindName("button2");
                button3 = (Button)rootPanel.FindName("button3");

                rootPanel.FlowDirection = direction;

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            FocusHelper.EnsureFocus(button, FocusState.Keyboard);

            using (var eventTester = new EventTester<Button, RoutedEventArgs>(button2, "GotFocus"))
            {
                Log.Comment("Press down on gamepad");
                TestServices.KeyboardHelper.GamepadDpadDown();
                eventTester.Wait();

                Verify.IsTrue(eventTester.HasFired);
            }
        }

        [TestMethod]
        [TestProperty("Description", "Validates that if two elements have the same score, we chose the one furthest left")]
        public void VerifyPreferLeft()
        {
            PreferLeft(FlowDirection.LeftToRight);
        }

        [TestMethod]
        [TestProperty("Description", "Validates that if FlowDirection is RTL, then prefer left becomes prefer right")]
        public void VerifyPreferLeftWithFlowDirectionTRL()
        {
            PreferLeft(FlowDirection.RightToLeft);
        }

        /*
              Right = NavigationDirectionDistance
            +-----------------------------------------------------------------+
            |       Right = Heuristic                                         |
            |     +-------------------------------+                           |
            |     |                               |                           |
            |     |   +-----+ +------+ +-----+    |       +-----+             |
            |     |   | A   | |      | |  D  |    |       |  H  |             |
            |     |   +-----+ |      | +-----+    |       +-----+             |
            |     |   +-----+ |      | +-----+    |             +--------+    |
            |     |   |     | |  C   | | R = N    |             |        |    |
            |     |   | B   | |      | |     |    |             |   G    |    |
            |     |   |     | |      | |  E  |    |             |        |    |
            |     |   |     | |      | |     |    |             +--------+    |
            |     |   +-----+ +------+ +-----+    |                           |
            |     |                               |     +-----+               |
            |     +-------------------------------+     |  F  |               |
            |                                           +-----+               |
            |                                                                 |
            |                                                                 |
            |                                                                 |
            +-----------------------------------------------------------------+
         */
        [TestMethod]
        [TestProperty("Description", "Validates that the different strategies work")]
        public void VerifyXYFocusNavigationStrategy()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <StackPanel Orientation='Horizontal' XYFocusLeftNavigationStrategy='NavigationDirectionDistance' XYFocusRightNavigationStrategy='NavigationDirectionDistance'>
                            <StackPanel Orientation='Horizontal' XYFocusRightNavigationStrategy='Projection'>
                                <StackPanel>
                                    <Button x:Name='buttonA' Content='A' Height='50' Width='50'/>
                                    <Button x:Name='buttonB' Content='B' Height='50' Width='50'/>
                                </StackPanel>
                                <StackPanel>
                                    <Button x:Name='buttonC' Content='C' Height='100' Width='100'/>
                                </StackPanel>
                                <StackPanel>
                                    <Button x:Name='buttonD' Content='D' Height='30' Width='50'/>
                                    <Button x:Name='buttonE' XYFocusRightNavigationStrategy='RectilinearDistance' Content='E' Height='70' Width='50'/>
                                </StackPanel>
                            </StackPanel>
                            <StackPanel Margin='50,0,0,0'>
                                <Button x:Name='buttonH' Content='H' Height='30' Width='50'/>
                                <Button x:Name='buttonG' Content='G' Margin='40,0,0,0' Height='70' Width='50'/>
                                <Button x:Name='buttonF' Content='F' Margin='-10,0,0,0' Height='30' Width='50'/>
                            </StackPanel>
                        </StackPanel>
                    </StackPanel>";

            StackPanel rootPanel = null;

            Button buttonA = null;
            Button buttonB = null;
            Button buttonC = null;
            Button buttonD = null;
            Button buttonE = null;
            Button buttonF = null;
            Button buttonG = null;
            Button buttonH = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);

                buttonA = (Button)rootPanel.FindName("buttonA");
                buttonB = (Button)rootPanel.FindName("buttonB");
                buttonC = (Button)rootPanel.FindName("buttonC");
                buttonD = (Button)rootPanel.FindName("buttonD");
                buttonE = (Button)rootPanel.FindName("buttonE");
                buttonF = (Button)rootPanel.FindName("buttonF");
                buttonG = (Button)rootPanel.FindName("buttonG");
                buttonH = (Button)rootPanel.FindName("buttonH");

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            FocusHelper.EnsureFocus(buttonE, FocusState.Keyboard);

            using (var eventTester = new EventTester<Button, RoutedEventArgs>(buttonF, "GotFocus"))
            {
                Log.Comment("Press right on gamepad");
                TestServices.KeyboardHelper.GamepadDpadRight();
                eventTester.Wait();

                Verify.IsTrue(eventTester.HasFired);
            }

            FocusHelper.EnsureFocus(buttonD, FocusState.Keyboard);

            using (var eventTester = new EventTester<Button, RoutedEventArgs>(buttonH, "GotFocus"))
            {
                Log.Comment("Press right on gamepad");
                TestServices.KeyboardHelper.GamepadDpadRight();
                eventTester.Wait();

                Verify.IsTrue(eventTester.HasFired);
            }

            FocusHelper.EnsureFocus(buttonG, FocusState.Keyboard);

            using (var eventTester = new EventTester<Button, RoutedEventArgs>(buttonF, "GotFocus"))
            {
                Log.Comment("Press left on gamepad");
                TestServices.KeyboardHelper.GamepadDpadLeft();
                eventTester.Wait();

                Verify.IsTrue(eventTester.HasFired);
            }
        }
        
        private static FrameworkElement GetVisualChildByName(FrameworkElement parent, string name)
        {
            FrameworkElement child = null;

            int count = VisualTreeHelper.GetChildrenCount(parent);

            for (int i = 0; i < count && child == null; i++)
            {
                var current = VisualTreeHelper.GetChild(parent, i) as FrameworkElement;
                if (current != null && !string.IsNullOrEmpty(current.Name) && current.Name == name)
                {
                    child = current;
                }
                else
                {
                    child = GetVisualChildByName(current, name);
                }
            }

            return child;
        }
        
        [TestMethod]
        [TestProperty("Description", "Validates that XY focus navigation still works with a scale other than 100%")]
        public void VerifyXYFocusWithDifferentScale()
        {
            const string rootPanelXaml =
                    @"<StackPanel Height='1280' Width='800' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                      <CommandBar HorizontalAlignment='Left'>
                        <AppBarButton x:Name='PrimaryPinButton' Label='Pin' Icon='Pin' />
                        <AppBarButton x:Name='PrimaryFavoriteButton' Label='Favorite' Icon='Favorite' />
                        <AppBarButton x:Name='PrimaryLikeButton' Label='Like' Icon='Like' />
                        <CommandBar.SecondaryCommands>
                          <AppBarButton x:Name='SecondaryPinButton' Label='Pin' Icon='Pin' />
                          <AppBarButton x:Name='SecondaryFavoriteButton' Label='Favorite' Icon='Favorite' />
                          <AppBarButton x:Name='SecondaryLikeButton' Label='Like' Icon='Like' />
                        </CommandBar.SecondaryCommands>
                      </CommandBar>
                    </StackPanel>";
                    
            TestServices.WindowHelper.SetWindowSizeOverrideWithScale(new Size(1280, 800), 2.0f);

            StackPanel rootPanel = null;
            
            AppBarButton primaryPinButton = null;
            AppBarButton primaryFavoriteButton = null;
            AppBarButton primaryLikeButton = null;
            Button moreButton = null;
            AppBarButton secondaryPinButton = null;
            AppBarButton secondaryFavoriteButton = null;
            AppBarButton secondaryLikeButton = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);

                primaryPinButton = (AppBarButton)rootPanel.FindName("PrimaryPinButton");
                primaryFavoriteButton = (AppBarButton)rootPanel.FindName("PrimaryFavoriteButton");
                primaryLikeButton = (AppBarButton)rootPanel.FindName("PrimaryLikeButton");
                secondaryPinButton = (AppBarButton)rootPanel.FindName("SecondaryPinButton");
                secondaryFavoriteButton = (AppBarButton)rootPanel.FindName("SecondaryFavoriteButton");
                secondaryLikeButton = (AppBarButton)rootPanel.FindName("SecondaryLikeButton");

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                moreButton = (Button)GetVisualChildByName(rootPanel, "MoreButton");
            });
            
            FocusHelper.EnsureFocus(primaryPinButton, FocusState.Keyboard);

            using (var eventTester = new EventTester<AppBarButton, RoutedEventArgs>(primaryFavoriteButton, "GotFocus"))
            {
                Log.Comment("Press right on gamepad");
                TestServices.KeyboardHelper.GamepadDpadRight();
                eventTester.Wait();

                Verify.IsTrue(eventTester.HasFired);
            }

            using (var eventTester = new EventTester<AppBarButton, RoutedEventArgs>(primaryLikeButton, "GotFocus"))
            {
                Log.Comment("Press right on gamepad");
                TestServices.KeyboardHelper.GamepadDpadRight();
                eventTester.Wait();

                Verify.IsTrue(eventTester.HasFired);
            }

            using (var eventTester = new EventTester<Button, RoutedEventArgs>(moreButton, "GotFocus"))
            {
                Log.Comment("Press right on gamepad");
                TestServices.KeyboardHelper.GamepadDpadRight();
                eventTester.Wait();

                Verify.IsTrue(eventTester.HasFired);
            }

            using (var eventTester = new EventTester<AppBarButton, RoutedEventArgs>(secondaryPinButton, "GotFocus"))
            {
                Log.Comment("Press A on gamepad");
                TestServices.KeyboardHelper.GamepadA();
                eventTester.Wait();

                Verify.IsTrue(eventTester.HasFired);
            }

            using (var eventTester = new EventTester<AppBarButton, RoutedEventArgs>(secondaryFavoriteButton, "GotFocus"))
            {
                Log.Comment("Press down on gamepad");
                TestServices.KeyboardHelper.GamepadDpadDown();
                eventTester.Wait();

                Verify.IsTrue(eventTester.HasFired);
            }

            using (var eventTester = new EventTester<AppBarButton, RoutedEventArgs>(secondaryLikeButton, "GotFocus"))
            {
                Log.Comment("Press down on gamepad");
                TestServices.KeyboardHelper.GamepadDpadDown();
                eventTester.Wait();

                Verify.IsTrue(eventTester.HasFired);
            }
        }

        /*
              Right = NavigationDirectionDistance
            +-----------------------------------------------------------------+
            |       Right = Auto                                              |
            |     +-------------------------------+                           |
            |     |                               |                           |
            |     |   +-----+ +------+ +-----+    |       +-----+             |
            |     |   | A   | |      | |  D  |    |       |  H  |             |
            |     |   +-----+ |      | +-----+    |       +-----+             |
            |     |   +-----+ |      | +-----+    |             +--------+    |
            |     |   |     | |  C   | |     |    |             |        |    |
            |     |   | B   | |      | |     |    |             |   G    |    |
            |     |   |     | |      | |  E  |    |             |        |    |
            |     |   |     | |      | |     |    |             +--------+    |
            |     |   +-----+ +------+ +-----+    |                           |
            |     |                               |     +-----+               |
            |     +-------------------------------+     |  F  |               |
            |                                           +-----+               |
            |                                                                 |
            |                                                                 |
            |                                                                 |
            +-----------------------------------------------------------------+
         */
        [TestMethod]
        [TestProperty("Description", "Validates that the different strategies work")]
        public void VerifyModeChosenWhenParentExplicitlyDeclaresAuto()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <StackPanel Orientation='Horizontal' XYFocusLeftNavigationStrategy='NavigationDirectionDistance' XYFocusRightNavigationStrategy='NavigationDirectionDistance'>
                            <StackPanel Orientation='Horizontal' XYFocusRightNavigationStrategy='Auto'>
                                <StackPanel>
                                    <Button x:Name='buttonA' Content='A' Height='50' Width='50'/>
                                    <Button x:Name='buttonB' Content='B' Height='50' Width='50'/>
                                </StackPanel>
                                <StackPanel>
                                    <Button x:Name='buttonC' Content='C' Height='100' Width='100'/>
                                </StackPanel>
                                <StackPanel>
                                    <Button x:Name='buttonD' Content='D' Height='30' Width='50'/>
                                    <Button x:Name='buttonE' Content='E' Height='70' Width='50'/>
                                </StackPanel>
                            </StackPanel>
                            <StackPanel Margin='50,0,0,0'>
                                <Button x:Name='buttonH' Content='H' Height='30' Width='50'/>
                                <Button x:Name='buttonG' Content='G' Margin='40,0,0,0' Height='70' Width='50'/>
                                <Button x:Name='buttonF' Content='F' Margin='-10,0,0,0' Height='30' Width='50'/>
                            </StackPanel>
                        </StackPanel>
                    </StackPanel>";

            StackPanel rootPanel = null;

            Button buttonA = null;
            Button buttonB = null;
            Button buttonC = null;
            Button buttonD = null;
            Button buttonE = null;
            Button buttonF = null;
            Button buttonG = null;
            Button buttonH = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);

                buttonA = (Button)rootPanel.FindName("buttonA");
                buttonB = (Button)rootPanel.FindName("buttonB");
                buttonC = (Button)rootPanel.FindName("buttonC");
                buttonD = (Button)rootPanel.FindName("buttonD");
                buttonE = (Button)rootPanel.FindName("buttonE");
                buttonF = (Button)rootPanel.FindName("buttonF");
                buttonG = (Button)rootPanel.FindName("buttonG");
                buttonH = (Button)rootPanel.FindName("buttonH");

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            FocusHelper.EnsureFocus(buttonE, FocusState.Keyboard);

            using (var eventTester = new EventTester<Button, RoutedEventArgs>(buttonF, "GotFocus"))
            {
                Log.Comment("Press right on gamepad");
                TestServices.KeyboardHelper.GamepadDpadRight();
                eventTester.Wait();

                Verify.IsTrue(eventTester.HasFired);
            }
        }

        /*
            +--------------+     +------+
            |Hyperlink     |     |  A   |
            +--------------+     +------+
                              +-----+
                              | B   |
                              +-----+

        */
        [TestMethod]
        [TestProperty("Description", "Validates that the different strategies work on hyperlink")]
        public void VerifyXYFocusNavigationStrategyWithHyperlink()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <StackPanel Orientation='Horizontal'>
                            <StackPanel Orientation='Horizontal'>
                                <Button Content='Holder' Width='1' Height='1'/>
                                <TextBlock><Hyperlink x:Name='hyperlink' XYFocusRightNavigationStrategy='NavigationDirectionDistance'>Hyperlink</Hyperlink></TextBlock>
                                <StackPanel>
                                    <Button Margin='50,0,0,0' Content='ButtonA'/>
                                    <Button x:Name='buttonB' Content='ButtonB'/>
                                </StackPanel>
                            </StackPanel>
                        </StackPanel>
                    </StackPanel>";

            StackPanel rootPanel = null;

            Hyperlink hyperlink = null;
            Button buttonB = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                hyperlink = (Hyperlink)rootPanel.FindName("hyperlink");
                buttonB = (Button)rootPanel.FindName("buttonB");

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            using (var eventTester = new EventTester<Hyperlink, RoutedEventArgs>(hyperlink, "GotFocus"))
            {
                Log.Comment("Focus hyperlink");

                UIExecutor.Execute(() =>
                {
                    hyperlink.Focus(FocusState.Keyboard);
                });

                eventTester.Wait();

                Verify.IsTrue(eventTester.HasFired);
            }

            using (var eventTester = new EventTester<Button, RoutedEventArgs>(buttonB, "GotFocus"))
            {
                Log.Comment("Press right on gamepad");
                TestServices.KeyboardHelper.GamepadDpadRight();
                eventTester.Wait();

                Verify.IsTrue(eventTester.HasFired);
            }
        }

        /*
    +----------+---------+----------------+
    |          |         |                |
    |          |         |                |
    |          |    A    |                |
    |          |         |                |
    |          +---------+                |
    |                             +-------+
    |                             |  C    |
    |                             |       |
    +--------+                    +-------+
    |        |                            |
    |   B    |                            |
    |        |                            |
    +--------+                            |
    |                                     |
    |            +-----+                  |
    |            |     |                  |
    |            |  D  |                  |
    +------------+-----+------------------+
*/
        [TestMethod]
        [TestProperty("Description", "Validates that the different strategies work in FindNExtElement")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void VerifyFindNextElementOverrideNavigationStrategy()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <StackPanel Orientation='Horizontal'>
                            <StackPanel Orientation='Vertical'>
                                <Button x:Name='buttonB' Content='B' Height='30' Width='30' Margin='0,120,0,0'/>
                            </StackPanel>
                            <StackPanel Orientation='Vertical'>
                                <Button x:Name='buttonA' Content='A' Height='90' Width='90'/>
                                <Button x:Name='buttonD' Content='D' Height='30' Width='30' Margin='30,90,0,0'/>
                            </StackPanel>
                            <StackPanel Orientation='Vertical'>
                            <Button x:Name='buttonC' Content='C' Height='30' Width='30' Margin='90,90,0,0'/>
                            </StackPanel>
                        </StackPanel>
                    </StackPanel>";

            StackPanel rootPanel = null;

            Button buttonA = null;
            Button buttonB = null;
            Button buttonC = null;
            Button buttonD = null;
            FindNextElementOptions navigationOptions = null;
            DependencyObject candidate = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                navigationOptions = new FindNextElementOptions();
                navigationOptions.SearchRoot = rootPanel;
                buttonA = (Button)rootPanel.FindName("buttonA");
                buttonB = (Button)rootPanel.FindName("buttonB");
                buttonC = (Button)rootPanel.FindName("buttonC");
                buttonD = (Button)rootPanel.FindName("buttonD");

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            FocusHelper.EnsureFocus(buttonA, FocusState.Keyboard);

            UIExecutor.Execute(() =>
            {
                Log.Comment("Trying Projection");
                navigationOptions.XYFocusNavigationStrategyOverride = XYFocusNavigationStrategyOverride.Projection;
                candidate = FocusManager.FindNextElement(FocusNavigationDirection.Down, navigationOptions);
                Verify.AreEqual(buttonD, (Button)candidate);
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Log.Comment("Trying NavigationDirectionDistance");
                navigationOptions.XYFocusNavigationStrategyOverride = XYFocusNavigationStrategyOverride.NavigationDirectionDistance;
                candidate = FocusManager.FindNextElement(FocusNavigationDirection.Down, navigationOptions);
                Verify.AreEqual(buttonC, (Button)candidate);
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Log.Comment("Trying RectilinearDistance");
                navigationOptions.XYFocusNavigationStrategyOverride = XYFocusNavigationStrategyOverride.RectilinearDistance;
                candidate = FocusManager.FindNextElement(FocusNavigationDirection.Down, navigationOptions);
                Verify.AreEqual(buttonB, (Button)candidate);
            });
        }


        /*
            +----------+---------+----------------+
            |          |         |                |
            |          |         |                |
            |          |    A    |                |
            |          |         |                |
            |          +---------+                |
            |                             +-------+
            |                             |  C    |
            |                             |       |
            +--------+                    +-------+
            |        |                            |
            |   B    |                            |
            |        |                            |
            +--------+                            |
            |                                     |
            |            +-----+                  |
            |            |     |                  |
            |            |  D  |                  |
            +------------+-----+------------------+
        */
        [TestMethod]
        [TestProperty("Description", "Validates that the different strategies work in TryMoveFocus")]
        [TestProperty("Hosting:Mode", "UAP")] // fails in WPF mode due to final release queue is not empty cleanup issue
        public void VerifyTryMoveFocusOverrideNavigationStrategy()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <StackPanel Orientation='Horizontal'>
                            <StackPanel Orientation='Vertical'>
                                <Button x:Name='buttonB' Content='B' Height='30' Width='30' Margin='0,120,0,0'/>
                            </StackPanel>
                            <StackPanel Orientation='Vertical'>
                                <Button x:Name='buttonA' Content='A' Height='90' Width='90'/>
                                <Button x:Name='buttonD' Content='D' Height='30' Width='30' Margin='30,90,0,0'/>
                            </StackPanel>
                            <StackPanel Orientation='Vertical'>
                            <Button x:Name='buttonC' Content='C' Height='30' Width='30' Margin='90,90,0,0'/>
                            </StackPanel>
                        </StackPanel>
                    </StackPanel>";

            StackPanel rootPanel = null;

            Button buttonA = null;
            Button buttonB = null;
            Button buttonC = null;
            Button buttonD = null;
            FindNextElementOptions navigationOptions = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                navigationOptions = new FindNextElementOptions();
                navigationOptions.SearchRoot = rootPanel;
                buttonA = (Button)rootPanel.FindName("buttonA");
                buttonB = (Button)rootPanel.FindName("buttonB");
                buttonC = (Button)rootPanel.FindName("buttonC");
                buttonD = (Button)rootPanel.FindName("buttonD");

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            FocusHelper.EnsureFocus(buttonA, FocusState.Keyboard);

            UIExecutor.Execute(() =>
            {
                Log.Comment("Trying Projection");
                navigationOptions.XYFocusNavigationStrategyOverride = XYFocusNavigationStrategyOverride.Projection;
                Verify.IsTrue(FocusManager.TryMoveFocus(FocusNavigationDirection.Down, navigationOptions));
                Verify.AreEqual(buttonD, FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot));
            });
            TestServices.WindowHelper.WaitForIdle();

            FocusHelper.EnsureFocus(buttonA, FocusState.Keyboard);
            UIExecutor.Execute(() =>
            {
                Log.Comment("Trying NavigationDirectionDistance");
                navigationOptions.XYFocusNavigationStrategyOverride = XYFocusNavigationStrategyOverride.NavigationDirectionDistance;
                Verify.IsTrue(FocusManager.TryMoveFocus(FocusNavigationDirection.Down, navigationOptions));
                Verify.AreEqual(buttonC, FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot));
            });
            TestServices.WindowHelper.WaitForIdle();

            FocusHelper.EnsureFocus(buttonA, FocusState.Keyboard);
            UIExecutor.Execute(() =>
            {
                Log.Comment("Trying RectilinearDistance");
                navigationOptions.XYFocusNavigationStrategyOverride = XYFocusNavigationStrategyOverride.RectilinearDistance;
                Verify.IsTrue(FocusManager.TryMoveFocus(FocusNavigationDirection.Down, navigationOptions));

            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(buttonB, FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot));
            });
        }

        /*
                                    Search Root
                                   +-------------------------------------------------------------------------+
                                   |                                                                         |
                                   |                                                                         |
                                   |                                                                         |
                                   |                                                                         |
        Hint Rect                  |                                                                         |
        +---------+                |                                                                         |
        | +-----+ | +-----+        |           +-------------------------+                 +------------+    |
        | |     | | |     |        |           |                         |                 |            |    |
        | |     | | |     |        |           |                         |                 |            |    |
        | | A   | | |  C  |        |           |                    F    |                 |     I      |    |
        | |     | | |     |        |       +---------------+             |                 |            |    |
        | |     | | |     |        |       |   |           |             |                 |            |    |
        | +-----+ | |     |        |       |   |           |             |                 |            |    |
        +---------+ +-----+        |       |   +-------------------------+                 +------------+    |
                    |     |        |       |   |           |             |                 |            |    |
          +-----+   |     |        |       |   |           |             |                 |            |    |
          |     |   |  D  |        |       |   |           |        G    |                 |     J      |    |
          |     |   |     |        |       |   |           |             |                 |            |    |
          |     |   |     |        |       |   |           |             |                 |            |    |
          |     |   |     |        |       |   |           |             |                 |            |    |
          | B   |   +-----+        |       |   +-------------------------+                 +------------+    |
          |     |   |     |        |       |   |           |             |                 |            |    |
          |     |   |     |        |       |   |           |             |                 |            |    |
          |     |   |  E  |        |       |   |           |        H    |                 |     K      |    |
          |     |   |     |        |       |   |           |             |                 |            |    |
          |     |   |     |        |       |   |           |             |                 |            |    |
          |     |   |     |        |       |   |           |             |                 |            |    |
          +-----+   +-----+        |       |   +-------------------------+                 +------------+    |
                                   |       |               |                                                 |
                                   |       |               |                                                 |
                                   |       |               |                                                 |
                                   |       +---------------+                                                 |
                                   |       Exclusion Rect                                                    |
                                   |                                                                         |
                                   +-------------------------------------------------------------------------+


            Hint Rect has the boundaries of A
            Exclusion Rect overlaps with F,G,H

         */
        [TestMethod]
        [TestProperty("Description", "Validates that the Rectangles in the options in FindNextElement behave as expected")]
        [TestProperty("Hosting:Mode", "UAP")] // fails in WPF mode due to final release queue is not empty cleanup issue
        public void VerifyFindNextElementHintAndExclusionRectAreRespected()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <StackPanel Orientation='Horizontal'>
                            <StackPanel Orientation='Vertical'>
                                <Button x:Name='buttonA' Content='A' Height='30' Width='30'/>
                                <Button x:Name='buttonB' Content='B' Height='60' Width='30'/>
                            </StackPanel>
                            <StackPanel Orientation='Vertical' Margin='30,0,0,0'>
                                <Button x:Name='buttonC' Content='C' Height='30' Width='30'/>
                                <Button x:Name='buttonD' Content='D' Height='30' Width='30'/>
                                <Button x:Name='buttonE' Content='E' Height='30' Width='30'/>
                            </StackPanel>
                            <StackPanel x:Name='searchRoot' Margin='30,0,0,0'>
                                <StackPanel Orientation='Vertical'>
                                    <Button x:Name='buttonF' Content='F' Height='30' Width='30'/>
                                    <Button x:Name='buttonG' Content='G' Height='30' Width='30'/>
                                    <Button x:Name='buttonH' Content='H' Height='30' Width='30'/>
                                </StackPanel>
                                <StackPanel Orientation='Vertical'>
                                    <Button x:Name='buttonI' Content='I' Height='30' Width='30'/>
                                    <Button x:Name='buttonJ' Content='J' Height='30' Width='30'/>
                                    <Button x:Name='buttonK' Content='K' Height='30' Width='30'/>
                                </StackPanel>
                            </StackPanel>
                        </StackPanel>
                    </StackPanel>";

            StackPanel rootPanel = null;

            Button buttonB = null;
            Button buttonI = null;
            StackPanel searchRoot = null;
            FindNextElementOptions navigationOptions = null;
            Rect hint = new Rect();
            hint.X = 0;
            hint.Y = 0;
            hint.Width = 30;
            hint.Height = 30;

            Rect exclusion = new Rect();
            exclusion.X = 140;
            exclusion.Y = 25;
            exclusion.Width = 30;
            exclusion.Height = 40;

            DependencyObject candidate = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);

                buttonB = (Button)rootPanel.FindName("buttonB");
                buttonI = (Button)rootPanel.FindName("buttonI");
                searchRoot = (StackPanel)rootPanel.FindName("searchRoot");
                navigationOptions = new FindNextElementOptions();
                navigationOptions.SearchRoot = rootPanel;
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            FocusHelper.EnsureFocus(buttonB, FocusState.Keyboard);

            UIExecutor.Execute(() =>
            {
                navigationOptions.ExclusionRect = exclusion;
                navigationOptions.HintRect = hint;
                navigationOptions.SearchRoot = searchRoot;

                candidate = FocusManager.FindNextElement(FocusNavigationDirection.Right, navigationOptions);
                Verify.AreEqual(buttonI, (Button)candidate);
            });
            TestServices.WindowHelper.WaitForIdle();
        }

        /*
                                    Search Root
                                   +-------------------------------------------------------------------------+
                                   |                                                                         |
                                   |                                                                         |
                                   |                                                                         |
                                   |                                                                         |
        Hint Rect                  |                                                                         |
        +---------+                |                                                                         |
        | +-----+ | +-----+        |           +-------------------------+                 +------------+    |
        | |     | | |     |        |           |                         |                 |            |    |
        | |     | | |     |        |           |                         |                 |            |    |
        | | A   | | |  C  |        |           |                    F    |                 |     I      |    |
        | |     | | |     |        |       +---------------+             |                 |            |    |
        | |     | | |     |        |       |   |           |             |                 |            |    |
        | +-----+ | |     |        |       |   |           |             |                 |            |    |
        +---------+ +-----+        |       |   +-------------------------+                 +------------+    |
                    |     |        |       |   |           |             |                 |            |    |
          +-----+   |     |        |       |   |           |             |                 |            |    |
          |     |   |  D  |        |       |   |           |        G    |                 |     J      |    |
          |     |   |     |        |       |   |           |             |                 |            |    |
          |     |   |     |        |       |   |           |             |                 |            |    |
          |     |   |     |        |       |   |           |             |                 |            |    |
          | B   |   +-----+        |       |   +-------------------------+                 +------------+    |
          |     |   |     |        |       |   |           |             |                 |            |    |
          |     |   |     |        |       |   |           |             |                 |            |    |
          |     |   |  E  |        |       |   |           |        H    |                 |     K      |    |
          |     |   |     |        |       |   |           |             |                 |            |    |
          |     |   |     |        |       |   |           |             |                 |            |    |
          |     |   |     |        |       |   |           |             |                 |            |    |
          +-----+   +-----+        |       |   +-------------------------+                 +------------+    |
                                   |       |               |                                                 |
                                   |       |               |                                                 |
                                   |       |               |                                                 |
                                   |       +---------------+                                                 |
                                   |       Exclusion Rect                                                    |
                                   |                                                                         |
                                   +-------------------------------------------------------------------------+


            Hint Rect has the boundaries of A
            Exclusion Rect overlaps with F,G,H

         */
        [TestMethod]
        [TestProperty("Description", "Validates that the Rectangles in the options in FindNextElement behave as expected")]
        [TestProperty("Hosting:Mode", "UAP")] // fails in WPF mode due to final release queue not empty
        public void VerifyTryMoveFocusHintAndExclusionRectAreRespected()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <StackPanel Orientation='Horizontal'>
                            <StackPanel Orientation='Vertical'>
                                <Button x:Name='buttonA' Content='A' Height='30' Width='30'/>
                                <Button x:Name='buttonB' Content='B' Height='60' Width='30'/>
                            </StackPanel>
                            <StackPanel Orientation='Vertical' Margin='30,0,0,0'>
                                <Button x:Name='buttonC' Content='C' Height='30' Width='30'/>
                                <Button x:Name='buttonD' Content='D' Height='30' Width='30'/>
                                <Button x:Name='buttonE' Content='E' Height='30' Width='30'/>
                            </StackPanel>
                            <StackPanel x:Name='searchRoot' Margin='30,0,0,0'>
                                <StackPanel Orientation='Vertical'>
                                    <Button x:Name='buttonF' Content='F' Height='30' Width='30'/>
                                    <Button x:Name='buttonG' Content='G' Height='30' Width='30'/>
                                    <Button x:Name='buttonH' Content='H' Height='30' Width='30'/>
                                </StackPanel>
                                <StackPanel Orientation='Vertical'>
                                    <Button x:Name='buttonI' Content='I' Height='30' Width='30'/>
                                    <Button x:Name='buttonJ' Content='J' Height='30' Width='30'/>
                                    <Button x:Name='buttonK' Content='K' Height='30' Width='30'/>
                                </StackPanel>
                            </StackPanel>
                        </StackPanel>
                    </StackPanel>";

            StackPanel rootPanel = null;

            Button buttonB = null;
            Button buttonI = null;
            Button candidate = null;
            StackPanel searchRoot = null;
            FindNextElementOptions navigationOptions = null;
            Rect hint = new Rect();
            hint.X = 0;
            hint.Y = 0;
            hint.Width = 30;
            hint.Height = 30;

            Rect exclusion = new Rect();
            exclusion.X = 140;
            exclusion.Y = 25;
            exclusion.Width = 30;
            exclusion.Height = 40;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);

                buttonB = (Button)rootPanel.FindName("buttonB");
                buttonI = (Button)rootPanel.FindName("buttonI");
                searchRoot = (StackPanel)rootPanel.FindName("searchRoot");
                navigationOptions = new FindNextElementOptions();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            FocusHelper.EnsureFocus(buttonB, FocusState.Keyboard);

            UIExecutor.Execute(() =>
            {
                navigationOptions.ExclusionRect = exclusion;
                navigationOptions.HintRect = hint;
                navigationOptions.SearchRoot = searchRoot;
                Verify.IsTrue(FocusManager.TryMoveFocus(FocusNavigationDirection.Right, navigationOptions));
                candidate = (Button)FocusManager.FindNextElement(FocusNavigationDirection.Right, navigationOptions);
            });
            TestServices.WindowHelper.WaitForIdle();
            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(buttonI, FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot));
            });
        }

        /*
                +-------------------------+----------------+
                |                         |                |
                |                         |                |
                |                         |                |
                |           A             |                |
                |                         |                |
                |                         |                |
                |                         |                |
                +-------------------------+                |
                |                         |                |
                |                         |                |
                |           B             |                |
                |                         |                |
                |                         |                |
                |                         |                |
                +-------------------------+                |
                |                          Exclusion Rect  |
                +------------------------------------------+
         */
        [TestMethod]
        [TestProperty("Description", "Verifies that focus cannot be changed using options when the exclusion rect covers all focusable elements")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void VerifyNoFocusChangeWhenExclusionRectCoversAllFocusableElements()
        {
            StackPanel rootPanel = null;

            Button buttonB = null;
            Button buttonA = null;
            FindNextElementOptions navigationOptions = null;
            DependencyObject focusCandidate = null;
            Rect exclusion = new Rect();
            exclusion.X = 0;
            exclusion.Y = 0;
            exclusion.Width = float.MaxValue;
            exclusion.Height = float.MaxValue;

            UIExecutor.Execute(() =>
            {
                rootPanel = new StackPanel();
                rootPanel.Height = 100;
                rootPanel.Width = 100;

                buttonB = new Button();
                buttonB.Height = 50;
                buttonB.Width = 100;
                buttonB.Content = "B";

                buttonA = new Button();
                buttonA.Height = 50;
                buttonA.Width = 100;
                buttonA.Content = "A";

                rootPanel.Children.Add(buttonA);
                rootPanel.Children.Add(buttonB);
                navigationOptions = new FindNextElementOptions();
                navigationOptions.SearchRoot = rootPanel;
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            FocusHelper.EnsureFocus(buttonA, FocusState.Keyboard);

            UIExecutor.Execute(() =>
            {
                navigationOptions.ExclusionRect = exclusion;
                navigationOptions.XYFocusNavigationStrategyOverride = XYFocusNavigationStrategyOverride.Projection;
                focusCandidate = FocusManager.FindNextElement(FocusNavigationDirection.Down, navigationOptions);
                Verify.IsFalse(FocusManager.TryMoveFocus(FocusNavigationDirection.Down, navigationOptions));

            });
            TestServices.WindowHelper.WaitForIdle();
            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(null, focusCandidate);
                Verify.AreEqual(buttonA, FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot));
            });
        }


        /*
            +------------------------+-----+----------------+
            |                        |     |                |
            |                        |  C  |                |
            |                        |     |                |
            |                        +-----+                |
            |                                               |
            |                                               |
            |                                               |
            |                                               |
            |                                               |
            |                                +--------------+
            |                                |              |
            +--------+                       |              |
            |        |                       |              |
            |   D    |                       |      A       |
            |        |                       |              |
            +--------+                       |              |
            |                                |              |
            |                                +--------------+
            |                                               |
            |                                               |
            |                                               |
            |                                               |
            |                                               |
            |                                               |
            |                                               |
            |               +----+                          |
            |               |    |                          |
            |               | B  |                          |
            +---------------+----+--------------------------+


         */
        [TestMethod]
        [TestProperty("Description", "Verifies that focus navigation strategies work with right to left flow direction")]
        public void VerifyNavigationStrategiesWorkWithRightToLeftFlow()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' FlowDirection='RightToLeft' >
                        <StackPanel.RenderTransform>
                            <ScaleTransform ScaleX='0.5' ScaleY='0.5' />
                        </StackPanel.RenderTransform >
                        <StackPanel Orientation='Horizontal' Margin='0,0,0,90'>
                            <Button x:Name='buttonC' Content='C' Height='30' Width='30' Margin='90,210,0,0' Background='Green'/>
                        </StackPanel>
                        <StackPanel Orientation='Horizontal' Margin='0,0,0,0'>
                            <Button x:Name='buttonA' Content='A' Height='90' Width='90' Margin='0,120,0,0' Background='Red'/>
                            <Button x:Name='buttonB' Content='B' Height='30' Width='30' Margin='0,240,0,0' Background='Blue'/>
                            <Button x:Name='buttonD' Content='D' Height='30' Width='30' Margin='30,150,0,0' Background='Yellow'/>
                        </StackPanel>
                    </StackPanel>";

            StackPanel rootPanel = null;

            Button buttonA = null;
            Button buttonB = null;
            Button buttonC = null;
            Button buttonD = null;
            FindNextElementOptions navigationOptions = null;
            DependencyObject candidate = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                navigationOptions = new FindNextElementOptions();
                navigationOptions.SearchRoot = rootPanel;
                buttonA = (Button)rootPanel.FindName("buttonA");
                buttonB = (Button)rootPanel.FindName("buttonB");
                buttonC = (Button)rootPanel.FindName("buttonC");
                buttonD = (Button)rootPanel.FindName("buttonD");

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            FocusHelper.EnsureFocus(buttonA, FocusState.Keyboard);

            UIExecutor.Execute(() =>
            {
                Log.Comment("Trying Projection");
                navigationOptions.XYFocusNavigationStrategyOverride = XYFocusNavigationStrategyOverride.Projection;
                candidate = FocusManager.FindNextElement(FocusNavigationDirection.Left, navigationOptions);
                Verify.AreEqual(buttonD, (Button)candidate);
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Log.Comment("Trying NavigationDirectionDistance");
                navigationOptions.XYFocusNavigationStrategyOverride = XYFocusNavigationStrategyOverride.NavigationDirectionDistance;
                candidate = FocusManager.FindNextElement(FocusNavigationDirection.Left, navigationOptions);
                Verify.AreEqual(buttonC, (Button)candidate);
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Log.Comment("Trying RectilinearDistance");
                navigationOptions.XYFocusNavigationStrategyOverride = XYFocusNavigationStrategyOverride.RectilinearDistance;
                candidate = FocusManager.FindNextElement(FocusNavigationDirection.Left, navigationOptions);
                Verify.AreEqual(buttonB, (Button)candidate);
            });
        }

        /*
      Right = NavigationDirectionDistance
    +---------------------------------------------------------------+
    |                                                               |
    |        +-----+ +------+ +-----+           +-----+             |
    |        | A   | |      | |  D  |           |  H  |             |
    |        +-----+ |      | +-----+           +-----+             |
    |        +-----+ |      | +-----+                 +--------+    |
    |        |     | |  C   | |     |                 |        |    |
    |        | B   | |      | |     |                 |   G    |    |
    |        |     | |      | |  E  |                 |        |    |
    |        |     | |      | |     |                 +--------+    |
    |        +-----+ +------+ +-----+                               |
    |                                         +-----+               |
    |                                         |  F  |               |
    |                                         +-----+               |
    +---------------------------------------------------------------+
 */
        [TestMethod]
        [TestProperty("Description", "Validates that navigation strategy Auto.")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void VerifyCorrectStrategyChosenOnNavigationStrategyAuto()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <StackPanel Orientation='Horizontal' XYFocusLeftNavigationStrategy='NavigationDirectionDistance' XYFocusRightNavigationStrategy='NavigationDirectionDistance'>
                            <StackPanel Orientation='Horizontal'>
                                <StackPanel>
                                    <Button x:Name='buttonA' Content='A' Height='50' Width='50'/>
                                    <Button x:Name='buttonB' Content='B' Height='50' Width='50'/>
                                </StackPanel>
                                <StackPanel>
                                    <Button x:Name='buttonC' Content='C' Height='100' Width='100'/>
                                </StackPanel>
                                <StackPanel>
                                    <Button x:Name='buttonD' Content='D' Height='30' Width='50'/>
                                    <Button x:Name='buttonE' Content='E' Height='70' Width='50' XYFocusRightNavigationStrategy='Projection'/>
                                </StackPanel>
                            </StackPanel>
                            <StackPanel Margin='50,0,0,0'>
                                <Button x:Name='buttonH' Content='H' Height='30' Width='50'/>
                                <Button x:Name='buttonG' Content='G' Margin='40,0,0,0' Height='70' Width='50'/>
                                <Button x:Name='buttonF' Content='F' Margin='-10,0,0,0' Height='30' Width='50'/>
                            </StackPanel>
                        </StackPanel>
                    </StackPanel>";

            StackPanel rootPanel = null;

            Button buttonA = null;
            Button buttonB = null;
            Button buttonC = null;
            Button buttonD = null;
            Button buttonE = null;
            Button buttonF = null;
            Button buttonG = null;
            Button buttonH = null;
            Button candidate = null;
            FindNextElementOptions navigationOptions = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);

                buttonA = (Button)rootPanel.FindName("buttonA");
                buttonB = (Button)rootPanel.FindName("buttonB");
                buttonC = (Button)rootPanel.FindName("buttonC");
                buttonD = (Button)rootPanel.FindName("buttonD");
                buttonE = (Button)rootPanel.FindName("buttonE");
                buttonF = (Button)rootPanel.FindName("buttonF");
                buttonG = (Button)rootPanel.FindName("buttonG");
                buttonH = (Button)rootPanel.FindName("buttonH");
                navigationOptions = new FindNextElementOptions();
                navigationOptions.SearchRoot = rootPanel;
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            FocusHelper.EnsureFocus(buttonE, FocusState.Keyboard);

            using (var eventTester = new EventTester<Button, RoutedEventArgs>(buttonF, "GotFocus"))
            {
                Log.Comment("Find Next Element on Auto");
                UIExecutor.Execute(() =>
                {
                    navigationOptions.XYFocusNavigationStrategyOverride = XYFocusNavigationStrategyOverride.Auto;
                    candidate = (Button)FocusManager.FindNextElement(FocusNavigationDirection.Right, navigationOptions);
                    Verify.IsTrue(FocusManager.TryMoveFocus(FocusNavigationDirection.Right, navigationOptions));
                });
                eventTester.Wait();
                UIExecutor.Execute(() =>
                {
                    Verify.AreEqual(buttonF, candidate);
                    Verify.IsTrue(eventTester.HasFired);
                });
            }
        }

        /*
                       +--------------+
              +-------------------------------------------+
              |        |              |  exclusionRect    |
              +-------------------------------------------+
            +----------+              +--------------------------+
            |          |              |              |           |
            |          |              |              |           |
            |    A     |     B        |       C      |     D     |
            |          |              |              |           |
            |          |              |              |           |
            +----------+              +--------------+-----------+
                       |              |
                       |              |
                       +--------------+

        */
        [TestMethod]
        [TestProperty("Description", "Verifies that the RectilinearDistance strategy respects the exclusion rect")]
        public void VerifyRectilinearDistanceRespectsExclusionRectangle()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <StackPanel Orientation='Horizontal' Margin='0,0,0,0'>
                            <Button x:Name='buttonA' Content='A' Height='30' Width='30' Margin='0,0,0,0' Background='Red'/>
                            <Button x:Name='buttonB' Content='B' Height='30' Width='60' Margin='0,0,0,0' Background='Blue'/>
                            <Button x:Name='buttonC' Content='C' Height='30' Width='30' Margin='0,0,0,0' Background='Green'/>
                            <Button x:Name='buttonD' Content='D' Height='30' Width='30' Margin='0,0,0,0' Background='Yellow'/>
                        </StackPanel>
                    </StackPanel>";

            StackPanel rootPanel = null;

            Button buttonA = null;
            Button buttonB = null;
            Button buttonC = null;
            Button buttonD = null;
            FindNextElementOptions navigationOptions = null;
            DependencyObject candidate = null;
            Rect exclusion = new Rect();
            exclusion.X = 30;
            exclusion.Y = 0;
            exclusion.Width = 30;
            exclusion.Height = 10;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                navigationOptions = new FindNextElementOptions();
                navigationOptions.SearchRoot = rootPanel;
                buttonA = (Button)rootPanel.FindName("buttonA");
                buttonB = (Button)rootPanel.FindName("buttonB");
                buttonC = (Button)rootPanel.FindName("buttonC");
                buttonD = (Button)rootPanel.FindName("buttonD");

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            FocusHelper.EnsureFocus(buttonA, FocusState.Keyboard);

            UIExecutor.Execute(() =>
            {
                Log.Comment("Trying RectilinearDistance to the right");
                navigationOptions.XYFocusNavigationStrategyOverride = XYFocusNavigationStrategyOverride.RectilinearDistance;
                navigationOptions.ExclusionRect = exclusion;
                candidate = FocusManager.FindNextElement(FocusNavigationDirection.Right, navigationOptions);
                Verify.AreEqual(buttonC, (Button)candidate);
            });
        }

        /*
                       +--------------+
              +-------------------------------------------+
              |        |              |  exclusionRect    |
              +-------------------------------------------+
            +----------+              +--------------------------+
            |          |              |              |           |
            |          |              |              |           |
            |    A     |     B        |       C      |     D     |
            |          |              |              |           |
            |          |              |              |           |
            +----------+              +--------------+-----------+
                       |              |
                       |              |
                       +--------------+

        */
        [TestMethod]
        [TestProperty("Description", "Verifies that the Projection strategy respects the exclusion rect")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void VerifyProjectionRespectsExclusionRectangle()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <StackPanel Orientation='Horizontal' Margin='0,0,0,0'>
                            <Button x:Name='buttonA' Content='A' Height='30' Width='30' Margin='0,0,0,0' Background='Red'/>
                            <Button x:Name='buttonB' Content='B' Height='30' Width='60' Margin='0,0,0,0' Background='Blue'/>
                            <Button x:Name='buttonC' Content='C' Height='30' Width='30' Margin='0,0,0,0' Background='Green'/>
                            <Button x:Name='buttonD' Content='D' Height='30' Width='30' Margin='0,0,0,0' Background='Yellow'/>
                        </StackPanel>
                    </StackPanel>";

            StackPanel rootPanel = null;

            Button buttonA = null;
            Button buttonB = null;
            Button buttonC = null;
            Button buttonD = null;
            FindNextElementOptions navigationOptions = null;
            DependencyObject candidate = null;
            Rect exclusion = new Rect();
            exclusion.X = 30;
            exclusion.Y = 0;
            exclusion.Width = 30;
            exclusion.Height = 10;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                navigationOptions = new FindNextElementOptions();
                navigationOptions.SearchRoot = rootPanel;
                buttonA = (Button)rootPanel.FindName("buttonA");
                buttonB = (Button)rootPanel.FindName("buttonB");
                buttonC = (Button)rootPanel.FindName("buttonC");
                buttonD = (Button)rootPanel.FindName("buttonD");

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            FocusHelper.EnsureFocus(buttonA, FocusState.Keyboard);

            UIExecutor.Execute(() =>
            {
                Log.Comment("Trying Projection to the right");
                navigationOptions.XYFocusNavigationStrategyOverride = XYFocusNavigationStrategyOverride.Projection;
                navigationOptions.ExclusionRect = exclusion;
                candidate = FocusManager.FindNextElement(FocusNavigationDirection.Right, navigationOptions);
                Verify.AreEqual(buttonC, (Button)candidate);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that tab navigation in FindNextElementOptions causes an exception to be thrown.")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void VerifyFindNextElementOptionsCannotUseTabNavigation()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <StackPanel Orientation='Horizontal' Margin='0,0,0,0'>
                            <Button x:Name='buttonA' Content='A' Height='30' Width='30' Margin='0,0,0,0' Background='Red'/>
                            <Button x:Name='buttonB' Content='B' Height='30' Width='60' Margin='0,0,0,0' Background='Blue'/>
                            <Button x:Name='buttonC' Content='C' Height='30' Width='30' Margin='0,0,0,0' Background='Green'/>
                            <Button x:Name='buttonD' Content='D' Height='30' Width='30' Margin='0,0,0,0' Background='Yellow'/>
                        </StackPanel>
                    </StackPanel>";

            StackPanel rootPanel = null;

            Button buttonA = null;
            Button buttonB = null;
            Button buttonC = null;
            Button buttonD = null;
            FindNextElementOptions navigationOptions = null;
            DependencyObject candidate = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                navigationOptions = new FindNextElementOptions();
                buttonA = (Button)rootPanel.FindName("buttonA");
                buttonB = (Button)rootPanel.FindName("buttonB");
                buttonC = (Button)rootPanel.FindName("buttonC");
                buttonD = (Button)rootPanel.FindName("buttonD");

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            FocusHelper.EnsureFocus(buttonA, FocusState.Keyboard);

            UIExecutor.Execute(() =>
            {
                Log.Comment("Trying navigation direction next.");
                navigationOptions.XYFocusNavigationStrategyOverride = XYFocusNavigationStrategyOverride.Projection;
                try
                {
                    candidate = FocusManager.FindNextElement(FocusNavigationDirection.Next, navigationOptions);
                    Verify.Fail("Exception was not thrown");
                }
                catch (ArgumentException ex)
                {
                    Verify.AreEqual(ex.HResult, E_INVALIDARG);
                }
            });
        }

        const long E_INVALIDARG = -2147024809;

        /*
            +--------+
            |        |
            |   A    |
            |        |
            +--------+
            +--------+    +--------+
            |        |    |        |
            |        |    |        |
            |        |    |  HINT  |
            |        |    |        |
            |        |    |        |
            |        |    +--------+
            |   B    |
            |        |
            |        |
            |        |
            |        |
            |        |
            |        |
            +--------+

        */

        [TestMethod]
        [TestProperty("Description", "Verify that plateau scaling works correctly with HintRect in the FindNextElement API")]
        [TestProperty("Hosting:Mode", "UAP")]
        [TestProperty("Ignore", "TRUE")]    // [DCPPTest] Xaml tests are failing because Xaml no longer applies the plateau scale
        public void FindNextElementHintRectWorksWithPlateauScaling()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <Button x:Name='buttonA' Content='A' Height='20' Width='100'/>
                        <Button x:Name='buttonB' Content='B' Height='80' Width='100'/>
                    </StackPanel>";

            StackPanel rootPanel = null;

            Button buttonA = null;
            Button buttonB = null;

            Rect hintRect = new Rect();
            hintRect.X = 150;
            hintRect.Y = 25;
            hintRect.Width = 100;
            hintRect.Height = 50;

            FindNextElementOptions navigationOptions = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                buttonA = (Button)rootPanel.FindName("buttonA");
                buttonB = (Button)rootPanel.FindName("buttonB");

                navigationOptions = new FindNextElementOptions();
                navigationOptions.SearchRoot = rootPanel;

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            FocusHelper.EnsureFocus(buttonA, FocusState.Keyboard);

            UIExecutor.Execute(() =>
            {
                navigationOptions.HintRect = hintRect;
                Button candidate = FocusManager.FindNextElement(FocusNavigationDirection.Left, navigationOptions) as Button;

                Verify.AreEqual(buttonB, candidate);
            });

            // Change the scale to 200%
            TestServices.WindowHelper.SetWindowSizeOverrideWithScale(new Size(400, 400), 2.0f);

            UIExecutor.Execute(() =>
            {
                Button candidate = FocusManager.FindNextElement(FocusNavigationDirection.Left, navigationOptions) as Button;
                Verify.AreEqual(buttonB, candidate);
            });
        }

        /*
            +--------+
            |        |
            |   A    |
            |        |
            +--------+
            +--------+    +--------+
            |        |    |        |
            |        |    |        |
            |        |    |  HINT  |
            |        |    |        |
            |        |    |        |
            |        |    +--------+
            |   B    |
            |        |
            |        |
            |        |
            |        |
            |        |
            |        |
            +--------+

        */

        [TestMethod]
        [TestProperty("Description", "Verify that plateau scaling works correctly with HintRect in the FindNextElement API")]
        public void FindNextFocusableElementHintRectWorksWithPlateauScaling()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <Button x:Name='buttonA' Content='A' Height='20' Width='100'/>
                        <Button x:Name='buttonB' Content='B' Height='80' Width='100'/>
                    </StackPanel>";

            StackPanel rootPanel = null;

            Button buttonA = null;
            Button buttonB = null;

            Rect hintRect = new Rect();
            hintRect.X = 150;
            hintRect.Y = 25;
            hintRect.Width = 100;
            hintRect.Height = 50;

            FindNextElementOptions navigationOptions = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                buttonA = (Button)rootPanel.FindName("buttonA");
                buttonB = (Button)rootPanel.FindName("buttonB");

                navigationOptions = new FindNextElementOptions();
                navigationOptions.SearchRoot = rootPanel;
                navigationOptions.HintRect = hintRect;

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            FocusHelper.EnsureFocus(buttonA, FocusState.Keyboard);

            UIExecutor.Execute(() =>
            {
                Button candidate = FocusManager.FindNextElement(FocusNavigationDirection.Left, navigationOptions) as Button;
                Verify.AreEqual(buttonB, candidate);
            });

            // Change the scale to 200%
            TestServices.WindowHelper.SetWindowSizeOverrideWithScale(new Size(400, 400), 2.0f);

            UIExecutor.Execute(() =>
            {
                Button candidate = FocusManager.FindNextElement(FocusNavigationDirection.Left, navigationOptions) as Button;
                Verify.AreEqual(buttonB, candidate);
            });
        }

        /*
            +------------EclusionRect
            | +--------+   |
            | |        |   |
            | |        |   |
            | |   A    |   |
            | |        |   |
            | +--------+   |
            |              |
            | +--------+   |
            | |        |   |
            | |        |   |
            | |   B    |   |
            | |        |   |
            | +--------+   |
            +--------------+
              +--------+
              |        |
              |        |
              |   C    |
              |        |
              +--------+

        */
        [TestMethod]
        [TestProperty("Description", "Verify that plateau scaling works correctly with ExclusionRect in the FindNextElement API")]
        [TestProperty("Hosting:Mode", "UAP")]
        [TestProperty("Ignore", "TRUE")]    // [DCPPTest] Xaml tests are failing because Xaml no longer applies the plateau scale
        public void FindNextElementExclusionRectWorksWithPlateauScaling()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <Button x:Name='buttonA' Content='A' Height='25' Width='50'/>
                        <Button x:Name='buttonB' Content='B' Height='25' Width='50'/>
                        <Button x:Name='buttonC' Content='C' Height='25' Width='50'/>
                    </StackPanel>";

            StackPanel rootPanel = null;

            Button buttonA = null;
            Button buttonB = null;
            Button buttonC = null;

            Rect exclusionRect = new Rect();
            exclusionRect.X = 0;
            exclusionRect.Y = 0;
            exclusionRect.Width = 25;
            exclusionRect.Height = 50;

            FindNextElementOptions navigationOptions = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                buttonA = (Button)rootPanel.FindName("buttonA");
                buttonB = (Button)rootPanel.FindName("buttonB");
                buttonC = (Button)rootPanel.FindName("buttonC");

                navigationOptions = new FindNextElementOptions();
                navigationOptions.SearchRoot = rootPanel;

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            FocusHelper.EnsureFocus(buttonA, FocusState.Keyboard);

            UIExecutor.Execute(() =>
            {
                navigationOptions.ExclusionRect = exclusionRect;
                Button candidate = FocusManager.FindNextElement(FocusNavigationDirection.Down, navigationOptions) as Button;

                Verify.AreEqual(buttonC, candidate);
            });

            // Change the scale to 1000%
            TestServices.WindowHelper.SetWindowSizeOverrideWithScale(new Size(1000, 1000), 10.0f);

            UIExecutor.Execute(() =>
            {
                Button candidate = FocusManager.FindNextElement(FocusNavigationDirection.Down, navigationOptions) as Button;
                Verify.AreEqual(buttonC, candidate);
            });
        }

        /*
         +---------------------------------------------------------------+
         |                                                               |
         |        +-----+ +------+ +-----+                               |
         |        | A   | |      | |  D  |                               |
         |        +-----+ |      | +-----+                               |
         |        +-----+ |      | +-----+                               |
         |        |     | |  C   | |     |                               |
         |        | B   | |      | |     |                               |
         |        |     | |      | |  E  |                               |
         |        |     | |      | |     |                               |
         |        +-----+ +------+ +-----+                               |
         +---------------------------------------------------------------+
        */

        [TestMethod]
        [TestProperty("Description", "Verifies that we do not update the manifold when using the XYFocus APIs")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void ManifoldsAreNotSetWhenUsingFindNextElement()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <StackPanel Orientation='Horizontal'>
                            <StackPanel Orientation='Horizontal'>
                                <StackPanel>
                                    <Button x:Name='buttonA' Content='A' Height='30' Width='50'/>
                                    <Button x:Name='buttonB' Content='B' Height='70' Width='50'/>
                                </StackPanel>
                                <StackPanel>
                                    <Button x:Name='buttonC' Content='C' Height='100' Width='100'/>
                                </StackPanel>
                                <StackPanel>
                                    <Button x:Name='buttonD' Content='D' Height='30' Width='50'/>
                                    <Button x:Name='buttonE' Content='E' Height='70' Width='50'/>
                                </StackPanel>
                            </StackPanel>
                        </StackPanel>
                    </StackPanel>";

            StackPanel rootPanel = null;

            Button buttonB = null;
            Button buttonC = null;
            Button buttonD = null;
            Button buttonE = null;

            FindNextElementOptions navigationOptions = null;
            Rect hint = new Rect();
            hint.X = 50;
            hint.Y = 0;
            hint.Width = 100;
            hint.Height = 30;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);

                buttonB = (Button)rootPanel.FindName("buttonB");
                buttonC = (Button)rootPanel.FindName("buttonC");
                buttonD = (Button)rootPanel.FindName("buttonD");
                buttonE = (Button)rootPanel.FindName("buttonE");

                navigationOptions = new FindNextElementOptions();
                navigationOptions.SearchRoot = rootPanel;
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            FocusHelper.EnsureFocus(buttonB, FocusState.Keyboard);

            using (var buttonCGotFocusTester = new EventTester<Button, RoutedEventArgs>(buttonC, "GotFocus"))
            using (var buttonEGotFocusTester = new EventTester<Button, RoutedEventArgs>(buttonE, "GotFocus"))
            {
                Log.Comment("Press right on gamepad");
                TestServices.KeyboardHelper.GamepadDpadRight();
                buttonCGotFocusTester.Wait();

                Verify.IsTrue(buttonCGotFocusTester.HasFired);

                UIExecutor.Execute(() =>
                {
                    navigationOptions.HintRect = hint;
                    Button candidate = FocusManager.FindNextElement(FocusNavigationDirection.Right, navigationOptions) as Button;
                    Verify.AreEqual(buttonD, candidate);
                });

                Log.Comment("Press right on gamepad");
                TestServices.KeyboardHelper.GamepadDpadRight();
                buttonEGotFocusTester.Wait();

                Verify.IsTrue(buttonEGotFocusTester.HasFired);
            }
        }

        /*
         +---------------------------------------------------------------+
         |                                                               |
         |        +-----+ +------+ +-----+                               |
         |        | A   | |      | |  D  |                               |
         |        +-----+ |      | +-----+                               |
         |        +-----+ |      | +-----+                               |
         |        |     | |  C   | |     |                               |
         |        | B   | |      | |     |                               |
         |        |     | |      | |  E  |                               |
         |        |     | |      | |     |                               |
         |        +-----+ +------+ +-----+                               |
         +---------------------------------------------------------------+
        */

        [TestMethod]
        [TestProperty("Description", "Verifies that we do not update the manifold after focus is cancelled")]
        public void ManifoldsAreNotSetWhenFocusChangeCancelled()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <StackPanel Orientation='Horizontal'>
                            <StackPanel Orientation='Horizontal'>
                                <StackPanel>
                                    <Button x:Name='buttonA' Content='A' Height='30' Width='50'/>
                                    <Button x:Name='buttonB' Content='B' Height='70' Width='50'/>
                                </StackPanel>
                                <StackPanel>
                                    <Button x:Name='buttonC' Content='C' Height='100' Width='100'/>
                                </StackPanel>
                                <StackPanel>
                                    <Button x:Name='buttonD' Content='D' Height='30' Width='50'/>
                                    <Button x:Name='buttonE' Content='E' Height='70' Width='50'/>
                                </StackPanel>
                            </StackPanel>
                        </StackPanel>
                    </StackPanel>";

            StackPanel rootPanel = null;

            Button buttonB = null;
            Button buttonC = null;
            Button buttonD = null;
            Button buttonE = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);

                buttonB = (Button)rootPanel.FindName("buttonB");
                buttonC = (Button)rootPanel.FindName("buttonC");
                buttonD = (Button)rootPanel.FindName("buttonD");
                buttonE = (Button)rootPanel.FindName("buttonE");

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            FocusHelper.EnsureFocus(buttonB, FocusState.Keyboard);

            var buttonBLosingFocusHandler = new Action<object, LosingFocusEventArgs>((source, args) =>
            {
                Log.Comment("Canceling focus change");
                args.Cancel = true;
            });

            using (var buttonBLosingFocusTester = new EventTester<Button, LosingFocusEventArgs>(buttonB, "LosingFocus", buttonBLosingFocusHandler))
            {
                Log.Comment("Press up on gamepad");
                TestServices.KeyboardHelper.GamepadDpadUp();

                buttonBLosingFocusTester.Wait();
            }

            Log.Comment("Focus should still be on Button B");

            using (var buttonCGotFocusTester = new EventTester<Button, RoutedEventArgs>(buttonC, "GotFocus"))
            using (var buttonEGotFocusTester = new EventTester<Button, RoutedEventArgs>(buttonE, "GotFocus"))
            {
                Log.Comment("Press right on gamepad");
                TestServices.KeyboardHelper.GamepadDpadRight();
                buttonCGotFocusTester.Wait();

                Verify.IsTrue(buttonCGotFocusTester.HasFired);

                Log.Comment("Press right on gamepad");
                TestServices.KeyboardHelper.GamepadDpadRight();
                buttonEGotFocusTester.Wait();

                Verify.IsTrue(buttonEGotFocusTester.HasFired);
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that we do not update the manifold after focus is cancelled during TryMoveFocus")]
        public void ManifoldsAreNotSetWhenFocusChangeCancelledDuringTryMoveFocus()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <StackPanel Orientation='Horizontal'>
                            <StackPanel Orientation='Horizontal'>
                                <StackPanel>
                                    <Button x:Name='buttonA' Content='A' Height='30' Width='50'/>
                                    <Button x:Name='buttonB' Content='B' Height='70' Width='50'/>
                                </StackPanel>
                                <StackPanel>
                                    <Button x:Name='buttonC' Content='C' Height='100' Width='100'/>
                                </StackPanel>
                                <StackPanel>
                                    <Button x:Name='buttonD' Content='D' Height='30' Width='50'/>
                                    <Button x:Name='buttonE' Content='E' Height='70' Width='50'/>
                                </StackPanel>
                            </StackPanel>
                        </StackPanel>
                    </StackPanel>";

            StackPanel rootPanel = null;

            Button buttonB = null;
            Button buttonC = null;
            Button buttonD = null;
            Button buttonE = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);

                buttonB = (Button)rootPanel.FindName("buttonB");
                buttonC = (Button)rootPanel.FindName("buttonC");
                buttonD = (Button)rootPanel.FindName("buttonD");
                buttonE = (Button)rootPanel.FindName("buttonE");

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            FocusHelper.EnsureFocus(buttonB, FocusState.Keyboard);

            var buttonBLosingFocusHandler = new Action<object, LosingFocusEventArgs>((source, args) =>
            {
                Log.Comment("Canceling focus change");
                args.Cancel = true;
            });

            using (var buttonBLosingFocusTester = new EventTester<Button, LosingFocusEventArgs>(buttonB, "LosingFocus", buttonBLosingFocusHandler))
            {
                UIExecutor.Execute(() =>
                {
                    FindNextElementOptions options = new FindNextElementOptions();
                    options.SearchRoot = TestServices.WindowHelper.WindowContent;
                    Verify.IsTrue(FocusManager.TryMoveFocus(FocusNavigationDirection.Up, options));
                });
            }

            Log.Comment("Focus should still be on Button B");

            using (var buttonCGotFocusTester = new EventTester<Button, RoutedEventArgs>(buttonC, "GotFocus"))
            using (var buttonEGotFocusTester = new EventTester<Button, RoutedEventArgs>(buttonE, "GotFocus"))
            {
                Log.Comment("Press right on gamepad");
                TestServices.KeyboardHelper.GamepadDpadRight();
                buttonCGotFocusTester.Wait();

                Verify.IsTrue(buttonCGotFocusTester.HasFired);

                Log.Comment("Press right on gamepad");
                TestServices.KeyboardHelper.GamepadDpadRight();
                buttonEGotFocusTester.Wait();

                Verify.IsTrue(buttonEGotFocusTester.HasFired);
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that bubbling up the tree when checking for XYFocusProperties checks for whether the elements are focusable")]
        public void XYFocusPropertiesShouldIgnoreNonFocusableElementsWhenBubbling()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <ContentControl x:Name='content'>
                            <StackPanel>
                                <Button x:Name='button1' Content='Button 1'/>
                                <Button x:Name='button2' Content='Button 2' IsEnabled='False'/>
                            </StackPanel>
                        </ContentControl>
                        <Button x:Name='button3' Content='Button 3'/>
                    </StackPanel>";

            StackPanel rootPanel = null;

            Button button1 = null;
            Button button2 = null;
            Button button3 = null;

            ContentControl content = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);

                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");
                button3 = (Button)rootPanel.FindName("button3");

                content = (ContentControl)rootPanel.FindName("content");

                TestServices.WindowHelper.WindowContent = rootPanel;

                button1.XYFocusDown = button2;
                content.XYFocusDown = button2;
            });

            TestServices.WindowHelper.WaitForIdle();
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(button1, FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot));
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that any overridden elements are within the search scope")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void IgnoreXYFocusPropertiesIfOverrideNotChildOfSearchRoot()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <ContentControl x:Name='content'>
                            <StackPanel>
                                <Button x:Name='button1' Content='Button 1'/>
                                <Button x:Name='button2' Content='Button 2'/>
                            </StackPanel>
                        </ContentControl>
                        <Button x:Name='button3' Content='Button 3'/>
                    </StackPanel>";

            StackPanel rootPanel = null;

            Button button1 = null;
            Button button2 = null;
            Button button3 = null;

            ContentControl content = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);

                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");
                button3 = (Button)rootPanel.FindName("button3");

                content = (ContentControl)rootPanel.FindName("content");

                button1.XYFocusDown = button3;

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            UIExecutor.Execute(() =>
            {
                FindNextElementOptions navigationOptions = new FindNextElementOptions();
                navigationOptions.SearchRoot = content;

                Button candidate = (Button)FocusManager.FindNextElement(FocusNavigationDirection.Down, navigationOptions);
                Verify.AreEqual(button2, candidate);
            });

        }

        [TestMethod]
        [TestProperty("Description", "When a direction override is non focusable, we should attempt to move focus within the element if possible")]
        public void XYFocusDirectionOverrideOnNonFocusableElementShouldFocusChild()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <ContentControl x:Name='content'>
                            <StackPanel>
                                <Button x:Name='button1' Content='Button 1'/>
                                <Button x:Name='button2' Content='Button 2'/>
                            </StackPanel>
                        </ContentControl>
                        <ContentControl x:Name='contentB' IsTabStop='false'>
                            <StackPanel>
                                <Button x:Name='button3' Content='Button 3'/>
                            </StackPanel>
                        </ContentControl>
                    </StackPanel>";

            StackPanel rootPanel = null;

            Button button1 = null;
            Button button2 = null;
            Button button3 = null;

            ContentControl content = null;
            ContentControl contentB = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);

                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");
                button3 = (Button)rootPanel.FindName("button3");

                content = (ContentControl)rootPanel.FindName("content");
                contentB = (ContentControl)rootPanel.FindName("contentB");

                button1.XYFocusDown = contentB;

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            using (var button3GotFocusTester = new EventTester<Button, RoutedEventArgs>(button3, "GotFocus"))
            {
                Log.Comment("Press down on gamepad");
                TestServices.KeyboardHelper.GamepadDpadDown();

                button3GotFocusTester.Wait();

                Verify.IsTrue(button3GotFocusTester.HasFired);
            }

        }

        [TestMethod]
        public void ValidateSupportForThirdPartyScrollers()
        {
            const string rootPanelXaml =
                    @"<Grid Width='600' Height='600' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <Grid.RenderTransform>
                            <ScaleTransform ScaleX='0.5' ScaleY='0.5' />
                        </Grid.RenderTransform >
                        <Button Content='Outer Left Button' HorizontalAlignment='Left' VerticalAlignment='Center' />
                        <Button Content='Outer Top Button' HorizontalAlignment='Center' VerticalAlignment='Top' />
                        <Button Content='Outer Right Button' HorizontalAlignment='Right' VerticalAlignment='Center' />
                        <Button Content='Outer Bottom Button' HorizontalAlignment='Center' VerticalAlignment='Bottom' />

                        <ContentControl x:Name='contentControl'>
                            <Grid Width='600' Height='600' Background='Gray'>
                                <Grid.RenderTransform>
                                    <TranslateTransform X='-200' Y='-200' />
                                </Grid.RenderTransform>

                                <!-- Inner buttons are larger than the outer so that they get ranked better by the XY focus algorithm. -->
                                <Button Content='Inner Left Button' HorizontalAlignment='Left' VerticalAlignment='Center'  Width='180' Height='180' />
                                <Button Content='Inner Top Button' HorizontalAlignment='Center' VerticalAlignment='Top' Width='180' Height='180' />
                                <Button Content='Inner Right Button' HorizontalAlignment='Right' VerticalAlignment='Center' Width='180' Height='180' />
                                <Button Content='Inner Bottom Button' HorizontalAlignment='Center' VerticalAlignment='Bottom' Width='180' Height='180' />

                                <Button x:Name='innerCenterButton' Content='Inner Center Button' HorizontalAlignment='Center' VerticalAlignment='Center' />
                            </Grid>
                        </ContentControl>
                    </Grid>";

            Button innerCenterButton = null;
            ScrollingContentControl scrollingSurface = null;

            UIExecutor.Execute(() =>
            {
                var rootPanel = (Grid)XamlReader.Load(rootPanelXaml);
                innerCenterButton = (Button)rootPanel.FindName("innerCenterButton");

                // Replace the content control by ScrollingContentControl.
                {
                    var contentControl = (ContentControl)rootPanel.FindName("contentControl");
                    rootPanel.Children.Remove(contentControl);

                    var innerGrid = contentControl.Content;
                    contentControl.Content = null;

                    scrollingSurface = new ScrollingContentControl
                    {
                        Content = innerGrid,
                        Width = 200,
                        Height = 200,
                        HorizontalAlignment = HorizontalAlignment.Center,
                        VerticalAlignment = VerticalAlignment.Center
                    };

                    rootPanel.Children.Add(scrollingSurface);
                }

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            FocusHelper.EnsureFocus(innerCenterButton, FocusState.Keyboard);

            UIExecutor.Execute(() =>
            {
                var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot);
                Verify.AreEqual(innerCenterButton, focusedElement);

                FindNextElementOptions navigationOptions = new FindNextElementOptions();
                navigationOptions.SearchRoot = TestServices.WindowHelper.WindowContent;

                Verify.AreEqual("Outer Right Button", ((Button)FocusManager.FindNextElement(FocusNavigationDirection.Right, navigationOptions)).Content);
                Verify.AreEqual("Outer Left Button", ((Button)FocusManager.FindNextElement(FocusNavigationDirection.Left, navigationOptions)).Content);

                scrollingSurface.UpdateConfiguration(isHorizontallyScrollable: true, isVerticallyScrollable: false);
                Verify.AreEqual("Inner Right Button", ((Button)FocusManager.FindNextElement(FocusNavigationDirection.Right, navigationOptions)).Content);
                Verify.AreEqual("Inner Left Button", ((Button)FocusManager.FindNextElement(FocusNavigationDirection.Left, navigationOptions)).Content);

                scrollingSurface.UpdateConfiguration(isHorizontallyScrollable: false, isVerticallyScrollable: false);
                Verify.AreEqual("Outer Top Button", ((Button)FocusManager.FindNextElement(FocusNavigationDirection.Up, navigationOptions)).Content);
                Verify.AreEqual("Outer Bottom Button", ((Button)FocusManager.FindNextElement(FocusNavigationDirection.Down, navigationOptions)).Content);

                scrollingSurface.UpdateConfiguration(isHorizontallyScrollable: false, isVerticallyScrollable: true);
                Verify.AreEqual("Inner Top Button", ((Button)FocusManager.FindNextElement(FocusNavigationDirection.Up, navigationOptions)).Content);
                Verify.AreEqual("Inner Bottom Button", ((Button)FocusManager.FindNextElement(FocusNavigationDirection.Down, navigationOptions)).Content);
            });

        }

        [TestMethod]
        [TestProperty("Description", "When opening a flyout, we should cache the current manifolds and reset them. Once we close, restore the manifolds")]
        [TestProperty("Hosting:Mode", "UAP")]  // it selects Core Window content root instead of XAML Island
        public void XYFocusManifoldsCachedAndResetWhenOpeningFlyout()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <StackPanel Orientation='Horizontal'>
                            <StackPanel>
                                <Button x:Name='button1' Content='Button 1' Height='50' Width='100'/>
                                <Button x:Name='button2' Content='Button 2' Height='50' Width='100'/>
                            </StackPanel>
                            <Button x:Name='flyButton' Height='100' Width='100' Content='Button'>
                                <Button.Flyout>
                                    <Flyout Placement='Bottom' x:Name='flyout'>
                                        <StackPanel>
                                            <Button Width='200' Content='Button'/>
                                            <StackPanel Orientation='Horizontal'>
                                                <Button x:Name='buttonA' Width='50' Content='A'/>
                                                <Button Width='150' Content='B'/>
                                            </StackPanel>
                                        </StackPanel>
                                    </Flyout>
                                </Button.Flyout>
                            </Button>
                            <StackPanel>
                                <Button Content='Button 3' Height='50' Width='100'/>
                                <Button x:Name='button4' Content='Button 4' Height='50' Width='100'/>
                            </StackPanel>
                        </StackPanel>
                    </StackPanel>";

            StackPanel rootPanel = null;

            Button flyButton = null;
            Button button1 = null;
            Button button2 = null;
            Button button4 = null;
            Button buttonA = null;

            Flyout flyout = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);

                flyButton = (Button)rootPanel.FindName("flyButton");
                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");
                button4 = (Button)rootPanel.FindName("button4");
                buttonA = (Button)rootPanel.FindName("buttonA");

                flyout = (Flyout)rootPanel.FindName("flyout");

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            using (var flyButtonGotFocusTester = new EventTester<Button, RoutedEventArgs>(flyButton, "GotFocus"))
            using (var button2GotFocusTester = new EventTester<Button, RoutedEventArgs>(button2, "GotFocus"))
            using (var button4GotFocusTester = new EventTester<Button, RoutedEventArgs>(button4, "GotFocus"))
            using (var buttonAGotFocusTester = new EventTester<Button, RoutedEventArgs>(buttonA, "GotFocus"))
            using (var flyoutOpenedTester = new EventTester<Flyout, object>(flyout, "Opened"))
            using (var flyoutClosedTester = new EventTester<Flyout, object>(flyout, "Closed"))
            {
                Log.Comment("Press down on gamepad");
                TestServices.KeyboardHelper.GamepadDpadDown();
                button2GotFocusTester.Wait();

                Log.Comment("Press right on gamepad");
                TestServices.KeyboardHelper.GamepadDpadRight();
                flyButtonGotFocusTester.Wait();

                Log.Comment("Opening flyout");
                TestServices.KeyboardHelper.Enter();
                flyoutOpenedTester.Wait();

                Log.Comment("Press down on gamepad");
                TestServices.KeyboardHelper.GamepadDpadDown();
                buttonAGotFocusTester.Wait();

                Log.Comment("Closing flyout");
                TestServices.KeyboardHelper.Escape();
                flyoutClosedTester.Wait();

                Log.Comment("Press right on gamepad");
                TestServices.KeyboardHelper.GamepadDpadRight();
                button4GotFocusTester.Wait();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Validates XYFocus with UIElements")]
        public void VerifyXYFocusNavigationOnUIElement()
        {                    
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <Button x:Name='outerSPBtn' Content='OuterSPButton'/>
                            <StackPanel Orientation='Horizontal'>
                                <StackPanel x:Name='innerSP1' IsTabStop='True'>
                                     <Button x:Name='innerSP1Btn' Content='InnerSP1-Button'/>
                                </StackPanel>
                                <StackPanel x:Name='innerSP2' IsTabStop='False'/>
                                <StackPanel x:Name='innerSP3' IsTabStop='True'/>
                                <StackPanel x:Name='innerSP4' IsTabStop='True'>
                                     <Button x:Name='innerSP4Btn' Content='InnerSP4-Button'/>
                                </StackPanel>
                            </StackPanel>
                            <StackPanel x:Name='innerSP5' IsTabStop='True'>
                                <Button x:Name='innerSP5Btn' Content='InnerSP5-Button'/>
                            </StackPanel>
                    </StackPanel>";

            StackPanel rootPanel = null;
            StackPanel innerSP1 = null;
            StackPanel innerSP2 = null;
            StackPanel innerSP3 = null;
            StackPanel innerSP4 = null;
            StackPanel innerSP5 = null;

            Button outerSPBtn = null;
            Button innerSP1Btn = null;
            Button innerSP4Btn = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                innerSP1 = (StackPanel)rootPanel.FindName("innerSP1");
                innerSP2 = (StackPanel)rootPanel.FindName("innerSP2");
                innerSP3 = (StackPanel)rootPanel.FindName("innerSP3");
                innerSP4 = (StackPanel)rootPanel.FindName("innerSP4");
                innerSP5 = (StackPanel)rootPanel.FindName("innerSP5");

                outerSPBtn = (Button)rootPanel.FindName("outerSPBtn");
                innerSP1Btn = (Button)rootPanel.FindName("innerSP1Btn");
                innerSP4Btn = (Button)rootPanel.FindName("innerSP4Btn");

                TestServices.WindowHelper.WindowContent = rootPanel;
            });
            TestServices.WindowHelper.WaitForIdle();
            FocusHelper.EnsureFocus(outerSPBtn, FocusState.Keyboard);

            Log.Comment("Press down on gamepad");
            TestServices.KeyboardHelper.GamepadDpadDown();
            TestServices.WindowHelper.WaitForIdle();
            UIExecutor.Execute(() =>
            {    
                Verify.AreEqual(innerSP1, FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot));
            });

            Log.Comment("Press right on gamepad and innerSP4 should be focused " +
                "as innerSP2 isTabStop is false and innerSP3 is empty");
            TestServices.KeyboardHelper.GamepadDpadRight();
            TestServices.WindowHelper.WaitForIdle();
            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(innerSP4, FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot));
            });

            Log.Comment("Press left on gamepad and innerSP1 should get back focus");
            TestServices.KeyboardHelper.GamepadDpadLeft();
            TestServices.WindowHelper.WaitForIdle();
            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(innerSP1, FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot));
            });   

            Log.Comment("Press down on gamepad to focus innerSP5");
            TestServices.KeyboardHelper.GamepadDpadDown();
            TestServices.WindowHelper.WaitForIdle();
            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(innerSP5, FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot));
            });

            Log.Comment("Press up on gamepad to focus innerSP1");
            TestServices.KeyboardHelper.GamepadDpadUp();
            TestServices.WindowHelper.WaitForIdle();
            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(innerSP1, FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot));
            });

            TestServices.WindowHelper.WaitForIdle();
        }
    }
}
