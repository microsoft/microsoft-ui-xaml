// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using MUXControlsTestApp.Utilities;
using System;
using System.Linq;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Markup;
using Common;

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
using ScrollAnchorProvider = Microsoft.UI.Xaml.Controls.ScrollAnchorProvider;
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

                Content = new ScrollAnchorProvider()
                {
                    Width = 400,
                    Height = 800,
                    Content = new ScrollViewer
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
    }
}
