// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using MUXControlsTestApp.Utilities;
using System;
using System.Linq;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Markup;
using Common;
using Windows.UI.Xaml.Media;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

#if !BUILD_WINDOWS
using ItemsRepeater = Microsoft.UI.Xaml.Controls.ItemsRepeater;
using ItemsSourceView = Microsoft.UI.Xaml.Controls.ItemsSourceView;
using RecyclingElementFactory = Microsoft.UI.Xaml.Controls.RecyclingElementFactory;
using RecyclePool = Microsoft.UI.Xaml.Controls.RecyclePool;
using StackLayout = Microsoft.UI.Xaml.Controls.StackLayout;
using ItemsRepeaterScrollHost = Microsoft.UI.Xaml.Controls.ItemsRepeaterScrollHost;
using System.Collections.ObjectModel;
using System.Threading;
using System.Collections.Generic;
#endif

namespace Windows.UI.Xaml.Tests.MUXControls.ApiTests.RepeaterTests
{
    [TestClass]
    public class RepeaterTests : TestsBase
    {
        [TestMethod]
        public void ValidateElementToIndexMapping()
        {
            ItemsRepeater repeater = null;
            RunOnUIThread.Execute(() =>
            {
                var elementFactory = new RecyclingElementFactory();
                elementFactory.RecyclePool = new RecyclePool();
                elementFactory.Templates["Item"] = (DataTemplate)XamlReader.Load(
                    @"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'> 
                          <TextBlock Text='{Binding}' Height='50' />
                      </DataTemplate>");

                repeater = new ItemsRepeater()
                {
                    ItemsSource = Enumerable.Range(0, 10).Select(i => string.Format("Item #{0}", i)),
#if BUILD_WINDOWS
                    ItemTemplate = (Windows.UI.Xaml.IElementFactory)elementFactory,
#else
                    ItemTemplate = elementFactory,
#endif
                    // Default is StackLayout, so do not have to explicitly set.
                    // Layout = new StackLayout(),
                };

                Content = new ItemsRepeaterScrollHost()
                {
                    Width = 400,
                    Height = 800,
                    ScrollViewer = new ScrollViewer
                    {
                        Content = repeater
                    }
                };

                Content.UpdateLayout();

                for (int i = 0; i < 10; i++)
                {
                    var element = repeater.TryGetElement(i);
                    Verify.IsNotNull(element);
                    Verify.AreEqual(string.Format("Item #{0}", i), ((TextBlock)element).Text);
                    Verify.AreEqual(i, repeater.GetElementIndex(element));
                }

                Verify.IsNull(repeater.TryGetElement(20));
            });
        }

        [TestMethod]
        public void ValidateRepeaterDefaults()
        {
            RunOnUIThread.Execute(() =>
            {
                var repeater = new ItemsRepeater() 
                {
                    ItemsSource = Enumerable.Range(0, 10).Select(i => string.Format("Item #{0}", i)),
                };

                Content = new ItemsRepeaterScrollHost() {
                    Width = 400,
                    Height = 800,
                    ScrollViewer = new ScrollViewer {
                        Content = repeater
                    }
                };

                Content.UpdateLayout();

                for (int i = 0; i < 10; i++)
                {
                    var element = repeater.TryGetElement(i);
                    Verify.IsNotNull(element);
                    Verify.AreEqual(string.Format("Item #{0}", i), ((TextBlock)element).Text);
                    Verify.AreEqual(i, repeater.GetElementIndex(element));
                }

                Verify.IsNull(repeater.TryGetElement(20));
            });
        }

        [TestMethod]
        [TestProperty("Bug", "12042052")]
        public void CanSetItemsSource()
        {
            // In bug 12042052, we crash when we set ItemsSource to null because we try to subscribe to
            // the DataSourceChanged event on a null instance.
            RunOnUIThread.Execute(() =>
            {
                {
                    var repeater = new ItemsRepeater();
                    repeater.ItemsSource = null;
                    repeater.ItemsSource = Enumerable.Range(0, 5).Select(i => string.Format("Item #{0}", i));
                }

                {
                    var repeater = new ItemsRepeater();
                    repeater.ItemsSource = Enumerable.Range(0, 5).Select(i => string.Format("Item #{0}", i));
                    repeater.ItemsSource = Enumerable.Range(5, 5).Select(i => string.Format("Item #{0}", i));
                    repeater.ItemsSource = null;
                    repeater.ItemsSource = Enumerable.Range(10, 5).Select(i => string.Format("Item #{0}", i));
                    repeater.ItemsSource = null;
                }
            });
        }

        [TestMethod]
        public void ValidateGetSetItemsSource()
        {
            RunOnUIThread.Execute(() =>
            {
                ItemsRepeater repeater = new ItemsRepeater();
                var dataSource = new ItemsSourceView(Enumerable.Range(0, 10).Select(i => string.Format("Item #{0}", i)));
                repeater.SetValue(ItemsRepeater.ItemsSourceProperty, dataSource);
                Verify.AreSame(dataSource, repeater.GetValue(ItemsRepeater.ItemsSourceProperty) as ItemsSourceView);
                Verify.AreSame(dataSource, repeater.ItemsSourceView);
            });
        }

        [TestMethod]
        public void ValidateGetSetBackground()
        {
            RunOnUIThread.Execute(() =>
            {
                ItemsRepeater repeater = new ItemsRepeater();
                var redBrush = new SolidColorBrush(Colors.Red);
                repeater.SetValue(ItemsRepeater.BackgroundProperty, redBrush);
                Verify.AreSame(redBrush, repeater.GetValue(ItemsRepeater.BackgroundProperty) as Brush);
                Verify.AreSame(redBrush, repeater.Background);

                var blueBrush = new SolidColorBrush(Colors.Blue);
                repeater.Background = blueBrush;
                Verify.AreSame(blueBrush, repeater.Background);
            });
        }

        [TestMethod]
        public void VerifyCurrentAnchor()
        {
            ItemsRepeater rootRepeater = null;
            ScrollViewer scrollViewer = null;
            ItemsRepeaterScrollHost scrollhost = null;
            ManualResetEvent viewChanged = new ManualResetEvent(false);
            RunOnUIThread.Execute(() =>
            {
                scrollhost = (ItemsRepeaterScrollHost)XamlReader.Load(
                  @"<controls:ItemsRepeaterScrollHost Width='400' Height='600'
                     xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                     xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                     xmlns:controls='using:Microsoft.UI.Xaml.Controls'>
                    <controls:ItemsRepeaterScrollHost.Resources>
                        <DataTemplate x:Key='ItemTemplate' >
                            <TextBlock Text='{Binding}' Height='50'/>
                        </DataTemplate>
                    </controls:ItemsRepeaterScrollHost.Resources>
                    <ScrollViewer x:Name='scrollviewer'>
                        <controls:ItemsRepeater x:Name='rootRepeater' ItemTemplate='{StaticResource ItemTemplate}' VerticalCacheLength='0' />
                    </ScrollViewer>
                </controls:ItemsRepeaterScrollHost>");

                rootRepeater = (ItemsRepeater)scrollhost.FindName("rootRepeater");
                scrollViewer = (ScrollViewer)scrollhost.FindName("scrollviewer");
                scrollViewer.ViewChanged += (sender, args) =>
                {
                    if (!args.IsIntermediate)
                    {
                        viewChanged.Set();
                    }
                };

                rootRepeater.ItemsSource = Enumerable.Range(0, 500);
                Content = scrollhost;
            });

            // scroll down several times and validate current anchor
            for (int i = 1; i < 10; i++)
            {
                IdleSynchronizer.Wait();
                RunOnUIThread.Execute(() =>
                {
                    scrollViewer.ChangeView(null, i * 200, null);
                });

                Verify.IsTrue(viewChanged.WaitOne(DefaultWaitTimeInMS));
                viewChanged.Reset();

                RunOnUIThread.Execute(() =>
                {
                    Verify.AreEqual(i * 200, scrollViewer.VerticalOffset);
                    var anchor = PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone5) ?
                            scrollhost.CurrentAnchor :
                            scrollViewer.CurrentAnchor;
                    var anchorIndex = rootRepeater.GetElementIndex(anchor);
                    Log.Comment("CurrentAnchor: " + anchorIndex);
                    Verify.AreEqual(i * 4, anchorIndex);
                });
            }
        }

        // Ensure that scrolling a nested repeater works when the 
        // Itemtemplates are data templates.
        [TestMethod]
        public void NestedRepeaterWithDataTemplateScenario()
        {
            ItemsRepeater rootRepeater = null;
            ScrollViewer scrollViewer = null;
            ManualResetEvent viewChanged = new ManualResetEvent(false);
            RunOnUIThread.Execute(() =>
            {
                var anchorProvider = (ItemsRepeaterScrollHost)XamlReader.Load(
                  @"<controls:ItemsRepeaterScrollHost Width='400' Height='600'
                     xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                     xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                     xmlns:controls='using:Microsoft.UI.Xaml.Controls'>
                    <controls:ItemsRepeaterScrollHost.Resources>
                        <DataTemplate x:Key='ItemTemplate' >
                            <TextBlock Text='{Binding}' />
                        </DataTemplate>
                        <DataTemplate x:Key='GroupTemplate'>
                            <StackPanel>
                                <TextBlock Text='{Binding}' />
                                <controls:ItemsRepeater ItemTemplate='{StaticResource ItemTemplate}' ItemsSource='{Binding}' VerticalCacheLength='0'/>
                            </StackPanel>
                        </DataTemplate>
                    </controls:ItemsRepeaterScrollHost.Resources>
                    <ScrollViewer x:Name='scrollviewer'>
                        <controls:ItemsRepeater x:Name='rootRepeater' ItemTemplate='{StaticResource GroupTemplate}' VerticalCacheLength='0' />
                    </ScrollViewer>
                </controls:ItemsRepeaterScrollHost>");

                rootRepeater = (ItemsRepeater)anchorProvider.FindName("rootRepeater");
                scrollViewer = (ScrollViewer)anchorProvider.FindName("scrollviewer");
                scrollViewer.ViewChanged += (sender, args) =>
                {
                    if (!args.IsIntermediate)
                    {
                        viewChanged.Set();
                    }
                };

                var itemsSource = new ObservableCollection<ObservableCollection<int>>();
                for (int i = 0; i < 100; i++)
                {
                    itemsSource.Add(new ObservableCollection<int>(Enumerable.Range(0, 5)));
                };

                rootRepeater.ItemsSource = itemsSource;
                Content = anchorProvider;
            });

            // scroll down several times to cause recycling of elements
            for (int i = 1; i < 10; i++)
            {
                IdleSynchronizer.Wait();
                RunOnUIThread.Execute(() =>
                {
                    scrollViewer.ChangeView(null, i * 200, null);
                });

                Verify.IsTrue(viewChanged.WaitOne(DefaultWaitTimeInMS));
                viewChanged.Reset();

                RunOnUIThread.Execute(() =>
                {
                    Verify.AreEqual(i * 200, scrollViewer.VerticalOffset);
                });
            }
        }

        // ScrollViewer scrolls vertically, but there is an inner 
        // repeater which flows horizontally which needs corrections to be handled.
        [TestMethod]
        public void VerifyCorrectionsInNonScrollableDirection()
        {
            ItemsRepeater rootRepeater = null;
            ScrollViewer scrollViewer = null;
            ItemsRepeaterScrollHost scrollhost = null;
            ManualResetEvent viewChanged = new ManualResetEvent(false);
            RunOnUIThread.Execute(() =>
            {
                scrollhost = (ItemsRepeaterScrollHost)XamlReader.Load(
                  @"<controls:ItemsRepeaterScrollHost Width='400' Height='600'
                     xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                     xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                     xmlns:controls='using:Microsoft.UI.Xaml.Controls'>
                    <ScrollViewer Width='400' Height='400' x:Name='scrollviewer'>
                        <controls:ItemsRepeater x:Name='repeater'>
                            <DataTemplate>
                                <StackPanel>
                                    <controls:ItemsRepeater ItemsSource='{Binding}'>
                                        <controls:ItemsRepeater.Layout>
                                            <controls:StackLayout Orientation='Horizontal' />
                                        </controls:ItemsRepeater.Layout>
                                    </controls:ItemsRepeater>
                                </StackPanel>
                            </DataTemplate>
                        </controls:ItemsRepeater>
                    </ScrollViewer>
                </controls:ItemsRepeaterScrollHost>");

                rootRepeater = (ItemsRepeater)scrollhost.FindName("repeater");
                scrollViewer = (ScrollViewer)scrollhost.FindName("scrollviewer");
                scrollViewer.ViewChanged += (sender, args) =>
                {
                    if (!args.IsIntermediate)
                    {
                        viewChanged.Set();
                    }
                };

                List<List<int>> items = new List<List<int>>();
                for (int i = 0; i < 100; i++)
                {
                    items.Add(Enumerable.Range(0, 4).ToList());
                }
                rootRepeater.ItemsSource = items;
                Content = scrollhost;
            });

            // scroll down several times and validate no crash
            for (int i = 1; i < 5; i++)
            {
                IdleSynchronizer.Wait();
                RunOnUIThread.Execute(() =>
                {
                    scrollViewer.ChangeView(null, i * 200, null);
                });

                Verify.IsTrue(viewChanged.WaitOne(DefaultWaitTimeInMS));
                viewChanged.Reset();
            }
        }
    }
}
