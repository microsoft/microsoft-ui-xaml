// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using Private.Infrastructure;
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


namespace Microsoft.UI.Xaml.Tests.Controls.ListViewBase
{
    [TestClass]
    public class ListViewHeaderFooterTests : XamlTestsBase
    {
        [ClassInitialize]
        [TestProperty("BinaryUnderTest", "Microsoft.UI.Xaml.dll")]
        [TestProperty("RunAs", "UAP")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("UAP:Praid", "XamlManagedTAEFTests")]
        [TestProperty("HelixWorkItemCreation", "CreateWorkItemPerTestClass")]
        public static void Setup(TestContext context)
        {
            AssemblySetup.CommonTestClassSetup();
        }

        [ClassCleanup]
        public void ClassCleanup()
        {
            base.CommonClassCleanup();
        }

        private void VerifyRelativePosition(FrameworkElement item, FrameworkElement panel, float x, float y)
        {
            var pos = item.TransformToVisual(panel).TransformPoint(new Point(0, 0));
            Log.Comment("position: {0}, {1}", pos.X, pos.Y);

            Verify.AreEqual(pos.X, x);
            Verify.AreEqual(pos.Y, y);
        }

        [TestMethod]
        public void ValidateFocusAndKeyboardNavigation()
        {
            TestServices.WindowHelper.SetWindowSizeOverride(new Size(400, 400));

            ListView lv = null;
            Button btn = null;
            UIExecutor.Execute(() =>
            {
                lv = (ListView)XamlReader.Load(@"
                    <ListView xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' Height='200' Width='200'>
                        <ListView.Resources>
                            <x:Double x:Key='ListViewItemMinHeight'>44</x:Double>
                        </ListView.Resources>
                        <ListView.ItemsPanel>
                            <ItemsPanelTemplate>
                                <ItemsStackPanel CacheLength='0'/>
                            </ItemsPanelTemplate>
                        </ListView.ItemsPanel>
                        <ListView.HeaderTemplate>
                            <DataTemplate>
                                <Border BorderThickness='2' BorderBrush='Red'>
                                    <Button Content='{Binding}'/>
                                </Border>
                            </DataTemplate>
                        </ListView.HeaderTemplate>
                        <ListView.FooterTemplate>
                            <DataTemplate>
                                <Border BorderThickness='2' BorderBrush='Blue'>
                                    <Button Content='{Binding}'/>
                                </Border>
                            </DataTemplate>
                        </ListView.FooterTemplate>
                    </ListView>");

                lv.Header = "Header";
                lv.Footer = "Footer";
                lv.ItemsSource = Enumerable.Range(0, 500).ToList();
                btn = new Button { Content = "first item on the page to accept focus" };
                StackPanel sp = new StackPanel();
                sp.Width = 300;
                sp.Height = 300;
                sp.Children.Add(btn);
                sp.Children.Add(lv);
                TestServices.WindowHelper.WindowContent = sp;
            });

            TestServices.WindowHelper.WaitForIdle();

            // the first item on page should have focus by default but this is not always true.
            // to make this test reliable, we put an additional button and force the focus on it
            UIExecutor.Execute(() =>
            {
                btn.Focus(FocusState.Programmatic);
            });

            Log.Comment("Move focus to header");
            TestServices.KeyboardHelper.Tab();

            // focus goes to header
            UIExecutor.Execute(() =>
            {
                var header = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot) as FrameworkElement;
                VerifyRelativePosition(header, lv.ItemsPanelRoot, 2, -34);
            });

            Log.Comment("Move focus to footer by Tab twice");
            TestServices.KeyboardHelper.Tab();
            TestServices.KeyboardHelper.Tab();

            // now focus is on footer
            UIExecutor.Execute(() =>
            {
                var footer = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot) as FrameworkElement;
                VerifyRelativePosition(footer, lv.ItemsPanelRoot, 2, 22002);
            });

            Log.Comment("Move focus back to footer by Shift+Tab twice");
            TestServices.KeyboardHelper.ShiftTab();
            TestServices.KeyboardHelper.ShiftTab();

            // focus goes back to header
            UIExecutor.Execute(() =>
            {
                var header = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot) as FrameworkElement;
                VerifyRelativePosition(header, lv.ItemsPanelRoot, 2, -34);
            });
        }

        [TestMethod]
        public void ValidateScrollToHeaderFooter()
        {
            TestServices.WindowHelper.SetWindowSizeOverride(new Size(400, 400));

            ListView lv = null;
            UIExecutor.Execute(() =>
            {
                lv = (ListView)XamlReader.Load(@"
                    <ListView xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' Height='200' Width='200'>
                        <ListView.Resources>
                            <x:Double x:Key='ListViewItemMinHeight'>44</x:Double>
                        </ListView.Resources>
                        <ListView.ItemsPanel>
                            <ItemsPanelTemplate>
                                <ItemsStackPanel CacheLength='0'/>
                            </ItemsPanelTemplate>
                        </ListView.ItemsPanel>
                        <ListView.HeaderTemplate>
                            <DataTemplate>
                                <Border BorderThickness='2' BorderBrush='Red'>
                                    <Button Content='{Binding}'/>
                                </Border>
                            </DataTemplate>
                        </ListView.HeaderTemplate>
                        <ListView.FooterTemplate>
                            <DataTemplate>
                                <Border BorderThickness='2' BorderBrush='Blue'>
                                    <Button Content='{Binding}'/>
                                </Border>
                            </DataTemplate>
                        </ListView.FooterTemplate>
                    </ListView>");

                lv.Header = "Header";
                lv.Footer = "Footer";
                lv.ItemsSource = Enumerable.Range(0, 500).ToList();
                TestServices.WindowHelper.WindowContent = lv;
            });

            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("ScrollTo footer and check the offset");
            // can scroll to footer
            UIExecutor.Execute(() =>
            {
                lv.ScrollIntoView(lv.Footer);
                lv.UpdateLayout();
                VerifyRelativePosition(lv.ItemsPanelRoot, lv, 0, -21836);
            });

            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("ScrollTo header and check the offset");
            // can scroll to header
            UIExecutor.Execute(() =>
            {
                lv.ScrollIntoView(lv.Header);
                lv.UpdateLayout();
                VerifyRelativePosition(lv.ItemsPanelRoot, lv, 0, 36);
            });
        }

        [TestMethod]
        public void CanChangeHeaderFooterTemplate()
        {
            TestServices.WindowHelper.SetWindowSizeOverride(new Size(400, 400));

            ListView lv = null;
            ItemsPresenter ip = null;

            // in this test we'll see the ItemsPresenter's actual height changes when we apply a DataTemplate to header or footer.
            // put listview in a vertical stackpanel so it will use the desired height as height
            // otherwise it will use available height as height.

            UIExecutor.Execute(() =>
            {
                lv = new ListView();
                lv.Header = "Header";
                lv.Footer = "Footer";
                StackPanel sp = new StackPanel();
                sp.Children.Add(lv);

                TestServices.WindowHelper.WindowContent = sp;
            });

            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("without header/footertemplate, the actual height of itemsPresenter is 38");
            UIExecutor.Execute(() =>
           {
               ip = VisualTreeHelper.GetParent(lv.ItemsPanelRoot) as ItemsPresenter;
               Verify.AreEqual(ip.ActualHeight, 38);
           });

            UIExecutor.Execute(() =>
            {
                var headerFooterTemplate = (DataTemplate)XamlReader.Load(@"
                    <DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                        <Border BorderThickness='2' BorderBrush='Red'>
                            <TextBlock Text='{Binding}' Height='100'/>
                        </Border>
                    </DataTemplate>");
                lv.HeaderTemplate = headerFooterTemplate;
                lv.FooterTemplate = headerFooterTemplate;
            });

            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("applied header/footertemplate, the actual height of itemsPresenter is 208");
            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(ip.ActualHeight, 208);
            });
        }

        [TestMethod]
        public void CanChangeHeaderFooter()
        {
            TestServices.WindowHelper.SetWindowSizeOverride(new Size(400, 400));

            ListView lv = null;
            ItemsPresenter ip = null;

            // in this test we'll see the ItemsPresenter's actual height changes when we change the header or footer.
            // put listview in a vertical stackpanel so it will use the desired height as height
            // otherwise it will use available height as height.

            UIExecutor.Execute(() =>
            {
                lv = new ListView();
                StackPanel sp = new StackPanel();
                sp.Children.Add(lv);

                TestServices.WindowHelper.WindowContent = sp;
            });

            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("without header/footer, the actual height of itemsPresenter is 0");
            UIExecutor.Execute(() =>
            {
                ip = VisualTreeHelper.GetParent(lv.ItemsPanelRoot) as ItemsPresenter;
                Verify.AreEqual(ip.ActualHeight, 0);
            });

            UIExecutor.Execute(() =>
            {
                lv.Header = "Header";
                lv.Footer = "Footer";
            });

            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("with header/footer, the actual height of itemsPresenter is 38");
            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(ip.ActualHeight, 38);
            });
        }


        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")] // fails in WPF mode due to rendering scopeguard not working in WPF yet
        public void ValidateDCompTree()
        {
            using (TestServices.Utilities.CreateRenderingScopeGuard(
                DCompRendering.WUCCompleteSynchronousCompTree,
                resizeWindow: false,
                injectMockDComp: true,
                resetDevice: true))
            {

                TestServices.WindowHelper.SetWindowSizeOverride(new Size(400, 400));

                ListView lv = null;
                UIExecutor.Execute(() =>
                {
                    lv = (ListView)XamlReader.Load(@"
                    <ListView xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' Height='200' Width='200'>
                        <ListView.HeaderTemplate>
                            <DataTemplate>
                                <Border BorderThickness='2' BorderBrush='Red'>
                                    <TextBlock Text='{Binding}'/>
                                </Border>
                            </DataTemplate>
                        </ListView.HeaderTemplate>
                        <ListView.FooterTemplate>
                            <DataTemplate>
                                <Border BorderThickness='2' BorderBrush='Blue'>
                                    <TextBlock Text='{Binding}'/>
                                </Border>
                            </DataTemplate>
                        </ListView.FooterTemplate>
                    </ListView>");

                    lv.Header = "Header";
                    lv.Footer = "Footer";
                    TestServices.WindowHelper.WindowContent = lv;
                });

                TestServices.WindowHelper.WaitForIdle();
                TestServices.Utilities.VerifyMockDCompOutput(MockDComp.SurfaceComparison.NoComparison);
            }
        }

        [TestMethod]
        [TestProperty("TestPass:ExcludeOn", "WindowsCore")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void ValidateUIElementTree()
        {
            TestServices.WindowHelper.SetWindowSizeOverride(new Size(400, 400));

            ListView lv = null;
            UIExecutor.Execute(() =>
            {
                lv = (ListView)XamlReader.Load(@"
                    <ListView xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' Height='200' Width='200'>
                        <ListView.HeaderTemplate>
                            <DataTemplate>
                                <Border BorderThickness='2' BorderBrush='Red'>
                                    <TextBlock Text='{Binding}'/>
                                </Border>
                            </DataTemplate>
                        </ListView.HeaderTemplate>
                        <ListView.FooterTemplate>
                            <DataTemplate>
                                <Border BorderThickness='2' BorderBrush='Blue'>
                                    <TextBlock Text='{Binding}'/>
                                </Border>
                            </DataTemplate>
                        </ListView.FooterTemplate>
                    </ListView>");

                lv.Header = "Header";
                lv.Footer = "Footer";
                TestServices.WindowHelper.WindowContent = lv;
            });

            TestServices.WindowHelper.WaitForIdle();

            TestServices.Utilities.VerifyUIElementTree();
        }
    }
}

