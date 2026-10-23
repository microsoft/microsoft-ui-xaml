// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections;
using System.Collections.Generic;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

using Common;
using MUXControlsTestApp.Utilities;

using Windows.Foundation.Metadata;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Shapes;
using Windows.UI;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class ThemeResourcesTests : ApiTestBase
    {
        [ClassInitialize]
        [TestProperty("Classification", "Integration")]
        public static void Setup(TestContext context) { }

        [TestMethod]
        public void VerifyTemplateFlyoutResourcesWithCyclicOwner()
        {
            VerifyFlyoutResourcesWithCyclicOwner(true);
        }

        [TestMethod]
        public void VerifyNonTemplateFlyoutResourcesWithCyclicOwner()
        {
            VerifyFlyoutResourcesWithCyclicOwner(false);
        }

        private void VerifyFlyoutResourcesWithCyclicOwner(bool templateOwned)
        {
            Button owner = null;
            Button target = null;
            Button flyoutContent = null;
            Flyout flyout = null;

            RunOnUIThread.Execute(() =>
            {
                var root = new Grid { RequestedTheme = ElementTheme.Light };
                target = new Button { Content = "External target" };
                if (templateOwned)
                {
                    owner = (Button)XamlReader.Load(@"
<Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
        Width='240' Height='40' VerticalAlignment='Top'>
    <Button.Template>
        <ControlTemplate TargetType='Button'>
            <Grid x:Name='TemplateOwner'>
                <TextBlock Text='Template-owned flyout'/>
                <FlyoutBase.AttachedFlyout>
                    <Flyout><Button Content='Flyout content'/></Flyout>
                </FlyoutBase.AttachedFlyout>
            </Grid>
        </ControlTemplate>
    </Button.Template>
</Button>");
                    root.Children.Add(owner);
                }
                root.Children.Add(target);
                MUXControlsTestApp.App.TestContentRoot = root;
            });
            IdleSynchronizer.Wait();

            try
            {
                RunOnUIThread.Execute(() =>
                {
                    if (templateOwned)
                    {
                        var ownerGrid = (Grid)VisualTreeUtils.FindVisualChildByName(owner, "TemplateOwner");
                        flyout = (Flyout)FlyoutBase.GetAttachedFlyout(ownerGrid);
                        flyoutContent = (Button)flyout.Content;
                    }
                    else
                    {
                        flyoutContent = new Button { Content = "Flyout content" };
                        flyout = new Flyout { Content = flyoutContent };
                    }
                    flyoutContent.Flyout = flyout;
                    flyout.ShowAt(target);
                });
                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    Verify.IsTrue(flyout.IsOpen);
                    Verify.AreEqual(
                        ((SolidColorBrush)target.Foreground).Color,
                        ((SolidColorBrush)flyoutContent.Foreground).Color);
                });
            }
            finally
            {
                RunOnUIThread.Execute(() =>
                {
                    flyout?.Hide();
                    if (flyoutContent != null)
                    {
                        flyoutContent.Flyout = null;
                    }
                    MUXControlsTestApp.App.TestContentRoot = null;
                });
                IdleSynchronizer.Wait();
            }
        }

        [TestMethod]
        public void VerifyNonTemplateFlyoutUsesOwnerResources()
        {
            foreach (var ownerLocal in new[] { false, true })
            {
                Grid root = null;
                Button owner = null;
                Button child = null;
                Rectangle codeBoundChild = null;
                Flyout flyout = null;
                var openedEvent = new AutoResetEvent(false);
                var closedEvent = new AutoResetEvent(false);

                RunOnUIThread.Execute(() =>
                {
                    root = (Grid)XamlReader.Load(@"
<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <Grid.Resources>
        <ResourceDictionary>
            <ResourceDictionary.ThemeDictionaries>
                <ResourceDictionary x:Key='Light'>
                    <SolidColorBrush x:Key='ButtonForeground' Color='Red'/>
                </ResourceDictionary>
                <ResourceDictionary x:Key='Default'>
                    <SolidColorBrush x:Key='ButtonForeground' Color='Blue'/>
                </ResourceDictionary>
            </ResourceDictionary.ThemeDictionaries>
        </ResourceDictionary>
    </Grid.Resources>
    <Button x:Name='Owner' Content='Owner' HorizontalAlignment='Center'
            VerticalAlignment='Center'>
        <Button.Flyout>
            <Flyout>
                <StackPanel>
                    <Button x:Name='Child' Content='Flyout content'/>
                    <Rectangle x:Name='CodeBoundChild' Width='100' Height='12'/>
                </StackPanel>
            </Flyout>
        </Button.Flyout>
    </Button>
</Grid>");
                    root.RequestedTheme = ElementTheme.Light;
                    owner = (Button)root.FindName("Owner");
                    child = (Button)root.FindName("Child");
                    codeBoundChild = (Rectangle)root.FindName("CodeBoundChild");
                    flyout = (Flyout)owner.Flyout;
                    flyout.Opened += (sender, args) => openedEvent.Set();
                    flyout.Closed += (sender, args) => closedEvent.Set();
                    if (ownerLocal)
                    {
                        var resources = root.Resources;
                        root.Resources = new ResourceDictionary();
                        owner.Resources = resources;
                    }
                    MUXControlsTestApp.App.TestContentRoot = root;
                });
                IdleSynchronizer.Wait();

                try
                {
                    RunOnUIThread.Execute(() => flyout.ShowAt(owner));
                    TestUtilities.WaitForEvent(openedEvent);
                    IdleSynchronizer.Wait();
                    RunOnUIThread.Execute(() =>
                        codeBoundChild.SetThemeResourceBinding(Rectangle.FillProperty, "ButtonForeground"));

                    foreach (var theme in new[] { ElementTheme.Light, ElementTheme.Dark, ElementTheme.Light })
                    {
                        RunOnUIThread.Execute(() => root.RequestedTheme = theme);
                        IdleSynchronizer.Wait();
                        RunOnUIThread.Execute(() =>
                        {
                            var expected = theme == ElementTheme.Light ? Colors.Red : Colors.Blue;
                            Verify.IsTrue(flyout.IsOpen);
                            Verify.AreEqual(expected, ((SolidColorBrush)owner.Foreground).Color);
                            Verify.AreEqual(expected, ((SolidColorBrush)child.Foreground).Color);
                            Verify.AreEqual(expected, ((SolidColorBrush)codeBoundChild.Fill).Color);
                        });
                    }

                    RunOnUIThread.Execute(() => flyout.Hide());
                    TestUtilities.WaitForEvent(closedEvent);
                    IdleSynchronizer.Wait();
                    RunOnUIThread.Execute(() => flyout.ShowAt(owner));
                    TestUtilities.WaitForEvent(openedEvent);
                    IdleSynchronizer.Wait();
                    RunOnUIThread.Execute(() =>
                    {
                        Verify.IsTrue(flyout.IsOpen);
                        Verify.AreEqual(Colors.Red, ((SolidColorBrush)child.Foreground).Color);
                        Verify.AreEqual(Colors.Red, ((SolidColorBrush)codeBoundChild.Fill).Color);
                    });
                }
                finally
                {
                    RunOnUIThread.Execute(() => flyout.Hide());
                    TestUtilities.WaitForEvent(closedEvent);
                    RunOnUIThread.Execute(() => MUXControlsTestApp.App.TestContentRoot = null);
                    IdleSynchronizer.Wait();
                }
            }
        }

        [TestMethod]
        // Isolate this test because other tests might have run in this context and either loaded the brushes 
        // already (thereby invalidating what we're trying to test) or our changes will adversly affect other tests.
        // Because of the test isolation we cannot use the base classes Content property as it may not be initialized.
        [TestProperty("IsolationLevel", "Method")]
        public void VerifyOverrides()
        {
            RatingControl ratingControl = null;
            Slider slider = null;
            Grid root = null;

            RunOnUIThread.Execute(() =>
            {
                var appResources = Application.Current.Resources;
                // 1) Override WinUI defined brush in App.Resources.
                appResources["RatingControlCaptionForeground"] = new SolidColorBrush(Colors.Orange);

                // 2) Override brush name used by a system control
                appResources["SliderTrackValueFill"] = new SolidColorBrush(Colors.Purple);

                root = new Grid {
                    Background = new SolidColorBrush(Colors.AntiqueWhite),
                };

                StackPanel panel = new StackPanel { Orientation = Orientation.Vertical };
                panel.Children.Add(slider = new Slider());

                panel.Children.Add(ratingControl = new RatingControl() { Value = 2 });

                root.Children.Add(panel);
                // Add an element over top to prevent stray mouse input from interfering.
                root.Children.Add(new Button {
                    Background = new SolidColorBrush(Color.FromArgb(30, 0, 255, 0)),
                    HorizontalAlignment = HorizontalAlignment.Stretch,
                    VerticalAlignment = VerticalAlignment.Stretch
                });

                MUXControlsTestApp.App.TestContentRoot = root;
            });
            IdleSynchronizer.TryWait();

            //System.Threading.Tasks.Task.Delay(TimeSpan.FromSeconds(20)).Wait();

            RunOnUIThread.Execute(() =>
            {
                // 1) Verify that overriding WinUI defined brushes in App.Resources works.
                Verify.AreEqual(Colors.Orange, ((SolidColorBrush)ratingControl.Foreground).Color,
                    "Verify RatingControlCaptionForeground override in Application.Resources gets picked up by WinUI control");

                // 2) Verify that overriding a system brush used by a system control works.
                if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone1))
                {
                    // Below code is comment because of bug 19180323 and we expect the code to be enabled again after test case is moved to nuget testapp

                    //Verify.AreEqual(Colors.Purple, ((SolidColorBrush)slider.Foreground).Color,
                    //    "Verify Slider (which uses SliderTrackValueFill as its .Foreground) overridden in Application.Resources gets picked up by Slider control");
                }
                else
                {
                    // Before RS1, we used to reference system brushes directly.
                }

                Log.Comment("Setting App.TestContentRoot = null");
                MUXControlsTestApp.App.TestContentRoot = null;
            });
            IdleSynchronizer.TryWait();
        }
    }
}