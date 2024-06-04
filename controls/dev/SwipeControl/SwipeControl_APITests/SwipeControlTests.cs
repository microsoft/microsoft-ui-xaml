// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using MUXControlsTestApp.Utilities;
using System;
using System.Threading;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;
using Common;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class SwipeControlTests : ApiTestBase
    {
        [TestMethod]
        public void SwipeItemTest()
        {
            SwipeItem swipeItem = null;
            RunOnUIThread.Execute(() =>
            {
                swipeItem = new SwipeItem();
                swipeItem.Text = "Selfie";
                swipeItem.IconSource = new FontIconSource() { Glyph = "&#xE114;" };
                swipeItem.Background = new SolidColorBrush(Microsoft.UI.Colors.Red);
                swipeItem.Foreground = new SolidColorBrush(Microsoft.UI.Colors.Blue);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(swipeItem.Text, "Selfie");
                Verify.IsTrue(swipeItem.IconSource is FontIconSource);
                Verify.AreEqual((swipeItem.IconSource as FontIconSource).Glyph, "&#xE114;");
                Verify.AreEqual(((SolidColorBrush)swipeItem.Background).Color, Microsoft.UI.Colors.Red);
                Verify.AreEqual(((SolidColorBrush)swipeItem.Foreground).Color, Microsoft.UI.Colors.Blue);
            });
        }

        [TestMethod]
        public void SwipeItemsTest()
        {
            SwipeItems swipeItems = null;

            RunOnUIThread.Execute(() =>
            {
                swipeItems = new SwipeItems();

                // verify the default value
                Verify.AreEqual(swipeItems.Mode, SwipeMode.Reveal);
                Verify.AreEqual(swipeItems.Count, 0);

                swipeItems.Add(new SwipeItem());
                swipeItems.Add(new SwipeItem());
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(swipeItems.Count, 2);
            });
        }

        [TestMethod]
        public void SwipeItemsExecuteThrowsExceptionWhenMoreThanOneItemAreAdded()
        {
            RunOnUIThread.Execute(() =>
            {
                var swipeItems = new SwipeItems();
                swipeItems.Mode = SwipeMode.Execute;
                swipeItems.Add(new SwipeItem());
                Verify.Throws<ArgumentException>(() => { swipeItems.Add(new SwipeItem()); });
            });
        }

        [TestMethod]
        public void SwipeControlTest()
        {
            RunOnUIThread.Execute(() =>
            {
                SwipeControl swipeControl = new SwipeControl();
                Verify.AreEqual(swipeControl.ActualHeight, 0);
                Verify.AreEqual(swipeControl.ActualWidth, 0);
                Verify.IsNull(swipeControl.LeftItems);
                Verify.IsNull(swipeControl.RightItems);
                Verify.IsNull(swipeControl.TopItems);
                Verify.IsNull(swipeControl.BottomItems);
                swipeControl.LeftItems = new SwipeItems();
                swipeControl.RightItems = new SwipeItems();
                Content = swipeControl;
                Content.UpdateLayout();
                Verify.IsFalse(swipeControl.IsTabStop);
                Verify.IsNotNull(swipeControl.LeftItems);
                Verify.IsNotNull(swipeControl.RightItems);
            });
        }

        [TestMethod]
        public void SwipeControlCanOnlyBeHorizontalOrVertical()
        {
            RunOnUIThread.Execute(() =>
            {
                SwipeControl swipeControl = new SwipeControl();
                swipeControl.LeftItems = new SwipeItems();
                var topItems = new SwipeItems();
                topItems.Add(new SwipeItem());
                swipeControl.TopItems = topItems;
                Verify.Throws<ArgumentException>(() => { swipeControl.LeftItems.Add(new SwipeItem()); });
            });
        }

        [TestMethod]
        public void SwipeControlCanOnlyBeHorizontalOrVerticalAfterRendering()
        {
            var resetEvent = new AutoResetEvent(false);
            RunOnUIThread.Execute(() =>
            {
                SwipeControl swipeControl = new SwipeControl();
                swipeControl.Loaded += (object sender, RoutedEventArgs args) => { resetEvent.Set(); };
                Content = swipeControl;
                Content.UpdateLayout();
                swipeControl.TopItems = new SwipeItems();
                swipeControl.LeftItems = new SwipeItems();
                swipeControl.LeftItems.Add(new SwipeItem());
                Verify.Throws<ArgumentException>(() => { swipeControl.TopItems.Add(new SwipeItem()); });
            });
        }

        [TestMethod]
        public void MarkupDefinedSwipeItemDoesNotCrash()
        {
            RunOnUIThread.Execute(() =>
            {
                var rootGrid = (Microsoft.UI.Xaml.Controls.Grid)XamlReader.LoadWithInitialTemplateValidation(
                "<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'> " +
                    "<GridView> " +
                        "<GridViewItem> " +
                            "<SwipeControl> " +
                                "<SwipeControl.RightItems> " +
                                    "<SwipeItems> " +
                                        "<SwipeItem " +
                                            "Background='#E81123' " +
                                            "Foreground='White' " +
                                            "Text='Remove'/> " +
                                    "</SwipeItems> " +
                                "</SwipeControl.RightItems> " +
                                "<Grid Width='200' Height='200' Background='green'/> " +
                            "</SwipeControl> " +
                        "</GridViewItem> " +
                    "</GridView> " +
                "</Grid>");

                Content = rootGrid;
                Content.UpdateLayout();
            });
        }
    }
}
