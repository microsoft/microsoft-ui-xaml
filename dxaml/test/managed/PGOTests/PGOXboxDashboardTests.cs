// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Threading;
using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using Windows.ApplicationModel.DataTransfer;
using Windows.Foundation;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Tests.Common;
using Private.Infrastructure;

namespace Microsoft.UI.Xaml.Tests.PGO.XboxDashboard
{
    [TestClass]
    public class XBoxDashboardMockTests : XamlTestsBase
    {
        [ClassInitialize]
        [TestProperty("BinaryUnderTest", "Microsoft.UI.Xaml.dll")]
        [TestProperty("RunAs", "UAP")]
        [TestProperty("Hosting:Mode", "UAP")]  // WPF not needed in this case. 
        [TestProperty("UAP:Praid", "XamlManagedTAEFTests")]
        [TestProperty("IsolationLevel", "Method")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("UAP:AppXManifest", AppxManifests.WINDOWS_VERSION_CURRENT)]
        public static void Setup(TestContext context)
        {
            // Make sure you always call XamlTestsBase.SetupBase
            // here to ensure the test services are initialized.
            XamlTestsBase.SetupBase(context);
            TestServices.KeyboardHelper.SetWaitKind(KeyboardWaitKind.WaitForIdleBeforeAndAfter);
        }

        [ClassCleanup]
        public void ClassCleanup()
        {
            base.CommonClassCleanup();
        }

        [TestCleanup]
        public override void TestCleanup()
        {
            TestServices.KeyboardHelper.SetWaitKind(KeyboardWaitKind.Default);
            base.TestCleanup();
        }

        [TestMethod]
        public void PGOLaunchTest()
        {
            UIExecutor.Execute(() =>
            {
                var sp = (StackPanel)XamlReader.Load(@"
                                                   <StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                                                      <Button>Before</Button>
                                                      <ListView
                                                        Width='400' Height='400' HorizontalAlignment='Left'>
                                                        <ListView.GroupStyle>
                                                            <GroupStyle>
                                                                <GroupStyle.HeaderTemplate>
                                                                    <DataTemplate>
                                                                        <TextBox Text='{Binding}' Height='50' />
                                                                    </DataTemplate>
                                                                </GroupStyle.HeaderTemplate>
                                                            </GroupStyle>
                                                        </ListView.GroupStyle>
                                                        <ListView.ItemTemplate>
                                                            <DataTemplate>
                                                                <Button Content='{Binding}' Height='150' />
                                                            </DataTemplate>
                                                        </ListView.ItemTemplate>
                                                    </ListView>
                                                    <Button>After</Button>
                                                 </StackPanel>");

                var listView = (ListView)sp.Children[1];

                var data = new TestObservableCollection<TestObservableCollection<int>>();
                for (int i = 0; i < 20; i++)
                {
                    data.Add(new TestObservableCollection<int>(Enumerable.Range(0, 2)));
                }

                var cvs = new CollectionViewSource() { IsSourceGrouped = true };
                cvs.Source = data;
                listView.ItemsSource = cvs.View;

                ElementSoundPlayer.State = ElementSoundPlayerState.On;
                ElementSoundPlayer.SpatialAudioMode = ElementSpatialAudioMode.On;

                TestServices.WindowHelper.WindowContent = sp;
            });

            TestServices.WindowHelper.WaitForIdle();

            // Gamepad down and up multiple times to go through XYFocus, listview realization
            // and sticky header code paths.
            for (int i = 0; i < 5; i++)
            {
                TestServices.KeyboardHelper.GamepadDpadDown();
            }

            for (int i = 0; i < 5; i++)
            {
                TestServices.KeyboardHelper.GamepadDpadUp();
            }

            TestServices.WindowHelper.WaitForIdle();
        }

        [TestMethod]
        public void PGOXYFocusStrategyTest()
        {
            Canvas canvas = null;
            UIExecutor.Execute(() =>
            {
                var grid = (Grid)XamlReader.Load(@"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                                                         xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                                                        <ScrollViewer Width='200' Height='200' HorizontalScrollBarVisibility='Visible' HorizontalScrollMode='Enabled'>
                                                            <Canvas x:Name='canvas'
                                                                XYFocusKeyboardNavigation='Enabled'
                                                                Width='1000' Height='1000'>
                                                                <Button Canvas.Left='0' Canvas.Top='0' Content='00' />
                                                                <Button Canvas.Left='100' Canvas.Top='0' Content='01' />
                                                                <Button Canvas.Left='200' Canvas.Top='0' Content='02' />

                                                                <Button Canvas.Left='0' Canvas.Top='100' Content='10' />
                                                                <Button Canvas.Left='100' Canvas.Top='100' Content='11' />
                                                                <Button Canvas.Left='200' Canvas.Top='100' Content='12' />

                                                                <Button Canvas.Left='0' Canvas.Top='200' Content='20' />
                                                                <Button Canvas.Left='100' Canvas.Top='200' Content='21' />
                                                                <Button Canvas.Left='200' Canvas.Top='200' Content='22' />
                                                            </Canvas>
                                                        </ScrollViewer>
                                                    </Grid>");

                canvas = (Canvas)grid.FindName("canvas");
                
                ElementSoundPlayer.State = ElementSoundPlayerState.On;
                ElementSoundPlayer.SpatialAudioMode = ElementSpatialAudioMode.On;

                TestServices.WindowHelper.WindowContent = grid;
            });

            TestServices.WindowHelper.WaitForIdle();
            
            foreach (XYFocusNavigationStrategy strategy in Enum.GetValues(typeof(XYFocusNavigationStrategy)))
            {
                UIExecutor.Execute(() =>
                {
                    canvas.XYFocusRightNavigationStrategy = strategy;
                    canvas.XYFocusDownNavigationStrategy = strategy;
                    canvas.XYFocusLeftNavigationStrategy = strategy;
                    canvas.XYFocusUpNavigationStrategy = strategy;
                });

                TestServices.WindowHelper.WaitForIdle();

                TestServices.KeyboardHelper.GamepadDpadRight();
                TestServices.KeyboardHelper.GamepadDpadDown();
                TestServices.KeyboardHelper.GamepadDpadLeft();
                TestServices.KeyboardHelper.GamepadDpadUp();
            }
        }
    }
}
