// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using MUXControlsTestApp.Utilities;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml;
using System.Threading;
using System;
using Microsoft.UI.Xaml.Media;
using Windows.UI;
using Windows.Foundation;
using Common;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public partial class WrapPanelTests : ApiTestBase
    {
        [ClassInitialize]
        [TestProperty("Classification", "Integration")]
        public static void ClassInitialize(TestContext context) { }

        [TestMethod]
        public void VerifyPaddingLayoutOffset()
        {
            RunOnUIThread.Execute(() =>
            {
                double width = 400;
                double height = 400;
                Thickness padding = new Thickness(10, 20, 30, 40);

                WrapPanel panel = new WrapPanel();
                panel.Width = width;
                panel.Height = height;
                panel.Padding = padding;
                panel.Orientation = Orientation.Horizontal;

                var button = new Button { Content = "Button", Width = 100, Height = 50 };
                var expectedButtonLayoutSlot = new Rect
                {
                    X = padding.Left, // 10
                    Y = padding.Top,  // 20
                    Width = 100,
                    Height = 50
                };
                panel.Children.Add(button);

                Content = panel;
                Content.UpdateLayout();

                Verify.AreEqual(expectedButtonLayoutSlot, LayoutInformation.GetLayoutSlot(button), "Verify LayoutSlot of single child Button with padding");
            });
        }

        [TestMethod]
        public void VerifyHorizontalWrapLayout()
        {
            RunOnUIThread.Execute(() =>
            {
                double width = 300;
                double height = 400;

                WrapPanel panel = new WrapPanel();
                panel.Width = width;
                panel.Height = height;
                panel.Orientation = Orientation.Horizontal;
                panel.ItemSpacing = 10;
                panel.LineSpacing = 5;

                // Add buttons that will wrap to next row
                var button1 = new Button { Content = "Button1", Width = 100, Height = 50 };
                var button2 = new Button { Content = "Button2", Width = 100, Height = 50 };
                var button3 = new Button { Content = "Button3", Width = 100, Height = 50 };
                var button4 = new Button { Content = "Button4", Width = 100, Height = 50 };

                // Button1 and Button2 fit on first row (100 + 10 + 100 = 210 ≤ 300)
                // Button3 wraps because (100 + 220 = 320 > 300)
                var expectedButton1LayoutSlot = new Rect { X = 0, Y = 0, Width = 100, Height = 50 };
                var expectedButton2LayoutSlot = new Rect { X = 110, Y = 0, Width = 100, Height = 50 }; // 100 + 10 spacing
                var expectedButton3LayoutSlot = new Rect { X = 0, Y = 55, Width = 100, Height = 50 }; // New row: 50 + 5 spacing
                var expectedButton4LayoutSlot = new Rect { X = 110, Y = 55, Width = 100, Height = 50 }; // Same row as button3

                panel.Children.Add(button1);
                panel.Children.Add(button2);
                panel.Children.Add(button3);
                panel.Children.Add(button4);

                Content = panel;
                Content.UpdateLayout();

                Verify.AreEqual(expectedButton1LayoutSlot, LayoutInformation.GetLayoutSlot(button1), "Verify LayoutSlot of button 1");
                Verify.AreEqual(expectedButton2LayoutSlot, LayoutInformation.GetLayoutSlot(button2), "Verify LayoutSlot of button 2");
                Verify.AreEqual(expectedButton3LayoutSlot, LayoutInformation.GetLayoutSlot(button3), "Verify LayoutSlot of button 3");
                Verify.AreEqual(expectedButton4LayoutSlot, LayoutInformation.GetLayoutSlot(button4), "Verify LayoutSlot of button 4 (wrapped to new row)");
            });
        }

        [TestMethod]
        public void VerifyVerticalWrapLayout()
        {
            RunOnUIThread.Execute(() =>
            {
                double width = 400;
                double height = 200;

                WrapPanel panel = new WrapPanel();
                panel.Width = width;
                panel.Height = height;
                panel.Orientation = Orientation.Vertical;
                panel.ItemSpacing = 5;
                panel.LineSpacing = 10;

                // Add buttons that will wrap to next column
                var button1 = new Button { Content = "Button1", Width = 100, Height = 50 };
                var button2 = new Button { Content = "Button2", Width = 100, Height = 50 };
                var button3 = new Button { Content = "Button3", Width = 100, Height = 50 };
                var button4 = new Button { Content = "Button4", Width = 100, Height = 50 };

                var expectedButton1LayoutSlot = new Rect { X = 0, Y = 0, Width = 100, Height = 50 };
                var expectedButton2LayoutSlot = new Rect { X = 0, Y = 55, Width = 100, Height = 50 }; // 50 + 5 spacing
                var expectedButton3LayoutSlot = new Rect { X = 0, Y = 110, Width = 100, Height = 50 }; // 105 + 5 spacing
                var expectedButton4LayoutSlot = new Rect { X = 110, Y = 0, Width = 100, Height = 50 }; // New column: 100 + 10 spacing

                panel.Children.Add(button1);
                panel.Children.Add(button2);
                panel.Children.Add(button3);
                panel.Children.Add(button4);

                Content = panel;
                Content.UpdateLayout();

                Verify.AreEqual(expectedButton1LayoutSlot, LayoutInformation.GetLayoutSlot(button1), "Verify LayoutSlot of button 1");
                Verify.AreEqual(expectedButton2LayoutSlot, LayoutInformation.GetLayoutSlot(button2), "Verify LayoutSlot of button 2");
                Verify.AreEqual(expectedButton3LayoutSlot, LayoutInformation.GetLayoutSlot(button3), "Verify LayoutSlot of button 3");
                Verify.AreEqual(expectedButton4LayoutSlot, LayoutInformation.GetLayoutSlot(button4), "Verify LayoutSlot of button 4 (wrapped to new column)");
            });
        }

        [TestMethod]
        public void VerifyItemsStretchLast()
        {
            RunOnUIThread.Execute(() =>
            {
                double width = 300;
                double height = 400;

                WrapPanel panel = new WrapPanel();
                panel.Width = width;
                panel.Height = height;
                panel.Orientation = Orientation.Horizontal;
                panel.ItemsStretch = WrapPanelItemsStretch.Last;

                var button1 = new Button { Content = "Button1", Width = 100, Height = 50 };
                var button2 = new Button { Content = "Button2", Width = 100, Height = 50 };

                // With WrapPanelItemsStretch.Last, the last button should stretch to fill remaining space
                var expectedButton1LayoutSlot = new Rect { X = 0, Y = 0, Width = 100, Height = 50 };
                var expectedButton2LayoutSlot = new Rect { X = 100, Y = 0, Width = 200, Height = 50 }; // Stretched to fill remaining 200 width

                panel.Children.Add(button1);
                panel.Children.Add(button2);

                Content = panel;
                Content.UpdateLayout();

                Verify.AreEqual(expectedButton1LayoutSlot, LayoutInformation.GetLayoutSlot(button1), "Verify LayoutSlot of button 1");
                Verify.AreEqual(expectedButton2LayoutSlot, LayoutInformation.GetLayoutSlot(button2), "Verify LayoutSlot of button 2 (stretched)");
            });
        }

        [TestMethod]
        public void VerifyItemsStretchNone()
        {
            RunOnUIThread.Execute(() =>
            {
                double width = 300;
                double height = 400;

                WrapPanel panel = new WrapPanel();
                panel.Width = width;
                panel.Height = height;
                panel.Orientation = Orientation.Horizontal;
                panel.ItemsStretch = WrapPanelItemsStretch.None;

                var button1 = new Button { Content = "Button1", Width = 100, Height = 50 };
                var button2 = new Button { Content = "Button2", Width = 100, Height = 50 };

                // With WrapPanelItemsStretch.None, buttons should maintain their original sizes
                var expectedButton1LayoutSlot = new Rect { X = 0, Y = 0, Width = 100, Height = 50 };
                var expectedButton2LayoutSlot = new Rect { X = 100, Y = 0, Width = 100, Height = 50 };

                panel.Children.Add(button1);
                panel.Children.Add(button2);

                Content = panel;
                Content.UpdateLayout();

                Verify.AreEqual(expectedButton1LayoutSlot, LayoutInformation.GetLayoutSlot(button1), "Verify LayoutSlot of button 1");
                Verify.AreEqual(expectedButton2LayoutSlot, LayoutInformation.GetLayoutSlot(button2), "Verify LayoutSlot of button 2 (not stretched)");
            });
        }

        [TestMethod]
        public void VerifyPaddingWithSpacing()
        {
            RunOnUIThread.Execute(() =>
            {
                double width = 400;
                double height = 400;
                Thickness padding = new Thickness(5, 10, 15, 20);

                WrapPanel panel = new WrapPanel();
                panel.Width = width;
                panel.Height = height;
                panel.Padding = padding;
                panel.Orientation = Orientation.Horizontal;
                panel.ItemSpacing = 8;
                panel.LineSpacing = 12;

                var button1 = new Button { Content = "Button1", Width = 100, Height = 50 };
                var button2 = new Button { Content = "Button2", Width = 100, Height = 50 };

                var expectedButton1LayoutSlot = new Rect
                {
                    X = padding.Left, // 5
                    Y = padding.Top,  // 10
                    Width = 100,
                    Height = 50
                };
                var expectedButton2LayoutSlot = new Rect
                {
                    X = padding.Left + 100 + 8, // 5 + 100 + 8 = 113
                    Y = padding.Top,             // 10
                    Width = 100,
                    Height = 50
                };

                panel.Children.Add(button1);
                panel.Children.Add(button2);

                Content = panel;
                Content.UpdateLayout();

                Verify.AreEqual(expectedButton1LayoutSlot, LayoutInformation.GetLayoutSlot(button1), "Verify LayoutSlot of button 1 with padding and spacing");
                Verify.AreEqual(expectedButton2LayoutSlot, LayoutInformation.GetLayoutSlot(button2), "Verify LayoutSlot of button 2 with padding and spacing");
            });
        }

        [TestMethod]
        public void VerifyCollapsedChildrenAreIgnored()
        {
            RunOnUIThread.Execute(() =>
            {
                double width = 300;
                double height = 400;

                WrapPanel panel = new WrapPanel();
                panel.Width = width;
                panel.Height = height;
                panel.Orientation = Orientation.Horizontal;
                panel.ItemSpacing = 10;

                var button1 = new Button { Content = "Button1", Width = 100, Height = 50 };
                var button2 = new Button { Content = "Button2", Width = 100, Height = 50, Visibility = Visibility.Collapsed };
                var button3 = new Button { Content = "Button3", Width = 100, Height = 50 };

                // Collapsed button2 should be ignored, so button3 should be positioned as if button2 doesn't exist
                var expectedButton1LayoutSlot = new Rect { X = 0, Y = 0, Width = 100, Height = 50 };
                var expectedButton3LayoutSlot = new Rect { X = 110, Y = 0, Width = 100, Height = 50 }; // 100 + 10 spacing

                panel.Children.Add(button1);
                panel.Children.Add(button2);
                panel.Children.Add(button3);

                Content = panel;
                Content.UpdateLayout();

                Verify.AreEqual(expectedButton1LayoutSlot, LayoutInformation.GetLayoutSlot(button1), "Verify LayoutSlot of button 1");
                Verify.AreEqual(expectedButton3LayoutSlot, LayoutInformation.GetLayoutSlot(button3), "Verify LayoutSlot of button 3 (collapsed button 2 ignored)");
            });
        }

        [TestMethod]
        public void VerifyDynamicOrientationChange()
        {
            WrapPanel panel = null;
            Button button1 = null;
            Button button2 = null;
            Button button3 = null;

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Create WrapPanel with Horizontal orientation");

                panel = new WrapPanel() { Width = 250, Height = 400 };
                panel.Orientation = Orientation.Horizontal;
                panel.ItemSpacing = 10;
                panel.LineSpacing = 5;

                button1 = new Button { Content = "1", Width = 100, Height = 50 };
                button2 = new Button { Content = "2", Width = 100, Height = 50 };
                button3 = new Button { Content = "3", Width = 100, Height = 50 };

                panel.Children.Add(button1);
                panel.Children.Add(button2);
                panel.Children.Add(button3);

                Content = panel;
                Content.UpdateLayout();

                Log.Comment("Verify layout for Horizontal orientation:");
                Verify.AreEqual(new Rect(0, 0, 100, 50), LayoutInformation.GetLayoutSlot(button1), "Verify LayoutSlot of button 1 (horizontal)");
                Verify.AreEqual(new Rect(110, 0, 100, 50), LayoutInformation.GetLayoutSlot(button2), "Verify LayoutSlot of button 2 (horizontal)");
                Verify.AreEqual(new Rect(0, 55, 100, 50), LayoutInformation.GetLayoutSlot(button3), "Verify LayoutSlot of button 3 (horizontal, wrapped)");
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Switch WrapPanel to Vertical orientation");
                panel.Orientation = Orientation.Vertical;
                panel.Width = 400;
                panel.Height = 170; // Should be space for this without wrapping (50x3 + 10x2 spacing = 170)

                Content.UpdateLayout();

                // Remember we use ItemSpacing here still for vertical, not line spacing, so these are still 10 apart
                Log.Comment("Verify layout for Vertical orientation:");
                Verify.AreEqual(new Rect(0, 0, 100, 50), LayoutInformation.GetLayoutSlot(button1), "Verify LayoutSlot of button 1 (vertical)");
                Verify.AreEqual(new Rect(0, 60, 100, 50), LayoutInformation.GetLayoutSlot(button2), "Verify LayoutSlot of button 2 (vertical)");
                Verify.AreEqual(new Rect(0, 120, 100, 50), LayoutInformation.GetLayoutSlot(button3), "Verify LayoutSlot of button 3 (vertical)");
            });
        }

        [TestMethod]
        public void VerifyVariableSizedChildren()
        {
            RunOnUIThread.Execute(() =>
            {
                double width = 220; // Force wrapping with variable sizes
                double height = 400;

                WrapPanel panel = new WrapPanel();
                panel.Width = width;
                panel.Height = height;
                panel.Orientation = Orientation.Horizontal;
                panel.ItemSpacing = 5;
                panel.LineSpacing = 10;

                var button1 = new Button { Content = "Small", Width = 60, Height = 30 };
                var button2 = new Button { Content = "Large", Width = 120, Height = 80 };
                var button3 = new Button { Content = "Medium", Width = 90, Height = 50 };
                var button4 = new Button { Content = "Tiny", Width = 40, Height = 20 };

                // Row 1: button1 (60) + spacing (5) + button2 (120) = 185px ≤ 220px ✓
                // Row 2: button3 would need 185 + 5 + 90 = 280px > 220px ✗ → wraps
                // Row 2: button3 (90) + spacing (5) + button4 (40) = 135px ≤ 220px ✓

                var expectedButton1LayoutSlot = new Rect { X = 0, Y = 0, Width = 60, Height = 80 }; // Height matches button2 (tallest in row 1)
                var expectedButton2LayoutSlot = new Rect { X = 65, Y = 0, Width = 120, Height = 80 }; // 60 + 5 spacing
                var expectedButton3LayoutSlot = new Rect { X = 0, Y = 90, Width = 90, Height = 50 }; // New row: 80 + 10 spacing, height matches button3 (tallest in row 2)
                var expectedButton4LayoutSlot = new Rect { X = 95, Y = 90, Width = 40, Height = 50 }; // Same row as button3, gets row height

                panel.Children.Add(button1);
                panel.Children.Add(button2);
                panel.Children.Add(button3);
                panel.Children.Add(button4);

                Content = panel;
                Content.UpdateLayout();

                Verify.AreEqual(expectedButton1LayoutSlot, LayoutInformation.GetLayoutSlot(button1), "Verify LayoutSlot of small button (row 1)");
                Verify.AreEqual(expectedButton2LayoutSlot, LayoutInformation.GetLayoutSlot(button2), "Verify LayoutSlot of large button (row 1, sets row height)");
                Verify.AreEqual(expectedButton3LayoutSlot, LayoutInformation.GetLayoutSlot(button3), "Verify LayoutSlot of medium button (row 2, sets row height)");
                Verify.AreEqual(expectedButton4LayoutSlot, LayoutInformation.GetLayoutSlot(button4), "Verify LayoutSlot of tiny button (row 2, inherits row height)");
            });
        }

        [TestMethod]
        public void VerifyCollapsedChildFirst()
        {
            RunOnUIThread.Execute(() =>
            {
                double width = 400;
                double height = 200;

                WrapPanel panel = new WrapPanel();
                panel.Width = width;
                panel.Height = height;
                panel.Orientation = Orientation.Horizontal;

                var button1 = new Button { Content = "Button1", Width = 150, Height = 50, Visibility = Visibility.Collapsed };
                var button2 = new Button { Content = "Button2", Width = 100, Height = 50 };
                var button3 = new Button { Content = "Button3", Width = 125, Height = 50 };
                var button4 = new Button { Content = "Button4", Width = 50, Height = 50 };

                // Collapsed first button should not affect layout of visible buttons
                var expectedButton1LayoutSlot = new Rect { X = 0, Y = 0, Width = 0, Height = 0 }; // Collapsed
                var expectedButton2LayoutSlot = new Rect { X = 0, Y = 0, Width = 100, Height = 50 };
                var expectedButton3LayoutSlot = new Rect { X = 100, Y = 0, Width = 125, Height = 50 };
                var expectedButton4LayoutSlot = new Rect { X = 225, Y = 0, Width = 50, Height = 50 };

                panel.Children.Add(button1);
                panel.Children.Add(button2);
                panel.Children.Add(button3);
                panel.Children.Add(button4);

                Content = panel;
                Content.UpdateLayout();

                // Note: Collapsed elements may still have a layout slot but with zero size
                Verify.AreEqual(expectedButton2LayoutSlot, LayoutInformation.GetLayoutSlot(button2), "Verify LayoutSlot of button 2 (first visible)");
                Verify.AreEqual(expectedButton3LayoutSlot, LayoutInformation.GetLayoutSlot(button3), "Verify LayoutSlot of button 3");
                Verify.AreEqual(expectedButton4LayoutSlot, LayoutInformation.GetLayoutSlot(button4), "Verify LayoutSlot of button 4");
            });
        }

        [TestMethod]
        public void VerifyCollapsedChildMiddle()
        {
            RunOnUIThread.Execute(() =>
            {
                double width = 400;
                double height = 200;

                WrapPanel panel = new WrapPanel();
                panel.Width = width;
                panel.Height = height;
                panel.Orientation = Orientation.Horizontal;

                var button1 = new Button { Content = "Button1", Width = 150, Height = 50 };
                var button2 = new Button { Content = "Button2", Width = 100, Height = 50, Visibility = Visibility.Collapsed };
                var button3 = new Button { Content = "Button3", Width = 125, Height = 50 };
                var button4 = new Button { Content = "Button4", Width = 50, Height = 50 };

                // Collapsed middle button should not affect spacing of visible buttons
                var expectedButton1LayoutSlot = new Rect { X = 0, Y = 0, Width = 150, Height = 50 };
                var expectedButton3LayoutSlot = new Rect { X = 150, Y = 0, Width = 125, Height = 50 };
                var expectedButton4LayoutSlot = new Rect { X = 275, Y = 0, Width = 50, Height = 50 };

                panel.Children.Add(button1);
                panel.Children.Add(button2);
                panel.Children.Add(button3);
                panel.Children.Add(button4);

                Content = panel;
                Content.UpdateLayout();

                Verify.AreEqual(expectedButton1LayoutSlot, LayoutInformation.GetLayoutSlot(button1), "Verify LayoutSlot of button 1");
                Verify.AreEqual(expectedButton3LayoutSlot, LayoutInformation.GetLayoutSlot(button3), "Verify LayoutSlot of button 3 (after collapsed)");
                Verify.AreEqual(expectedButton4LayoutSlot, LayoutInformation.GetLayoutSlot(button4), "Verify LayoutSlot of button 4");
            });
        }

        [TestMethod]
        public void VerifyCollapsedChildLast()
        {
            RunOnUIThread.Execute(() =>
            {
                double width = 400;
                double height = 200;

                WrapPanel panel = new WrapPanel();
                panel.Width = width;
                panel.Height = height;
                panel.Orientation = Orientation.Horizontal;

                var button1 = new Button { Content = "Button1", Width = 150, Height = 50 };
                var button2 = new Button { Content = "Button2", Width = 100, Height = 50 };
                var button3 = new Button { Content = "Button3", Width = 125, Height = 50 };
                var button4 = new Button { Content = "Button4", Width = 50, Height = 50, Visibility = Visibility.Collapsed };

                // Collapsed last button should not affect layout of previous buttons
                var expectedButton1LayoutSlot = new Rect { X = 0, Y = 0, Width = 150, Height = 50 };
                var expectedButton2LayoutSlot = new Rect { X = 150, Y = 0, Width = 100, Height = 50 };
                var expectedButton3LayoutSlot = new Rect { X = 250, Y = 0, Width = 125, Height = 50 };

                panel.Children.Add(button1);
                panel.Children.Add(button2);
                panel.Children.Add(button3);
                panel.Children.Add(button4);

                Content = panel;
                Content.UpdateLayout();

                Verify.AreEqual(expectedButton1LayoutSlot, LayoutInformation.GetLayoutSlot(button1), "Verify LayoutSlot of button 1");
                Verify.AreEqual(expectedButton2LayoutSlot, LayoutInformation.GetLayoutSlot(button2), "Verify LayoutSlot of button 2");
                Verify.AreEqual(expectedButton3LayoutSlot, LayoutInformation.GetLayoutSlot(button3), "Verify LayoutSlot of button 3");
            });
        }

        [TestMethod]
        public void VerifyHorizontalLayoutWithUniformSpacing()
        {
            RunOnUIThread.Execute(() =>
            {
                double width = 400;
                double height = 200;

                WrapPanel panel = new WrapPanel();
                panel.Width = width;
                panel.Height = height;
                panel.Orientation = Orientation.Horizontal;
                panel.ItemSpacing = 10;

                var button1 = new Button { Content = "Button1", Width = 100, Height = 50 };
                var button2 = new Button { Content = "Button2", Width = 100, Height = 50 };
                var button3 = new Button { Content = "Button3", Width = 100, Height = 50 };

                // All buttons should fit on one row with spacing
                var expectedButton1LayoutSlot = new Rect { X = 0, Y = 0, Width = 100, Height = 50 };
                var expectedButton2LayoutSlot = new Rect { X = 110, Y = 0, Width = 100, Height = 50 }; // 100 + 10 spacing
                var expectedButton3LayoutSlot = new Rect { X = 220, Y = 0, Width = 100, Height = 50 }; // 210 + 10 spacing

                panel.Children.Add(button1);
                panel.Children.Add(button2);
                panel.Children.Add(button3);

                Content = panel;
                Content.UpdateLayout();

                Verify.AreEqual(expectedButton1LayoutSlot, LayoutInformation.GetLayoutSlot(button1), "Verify LayoutSlot of button 1");
                Verify.AreEqual(expectedButton2LayoutSlot, LayoutInformation.GetLayoutSlot(button2), "Verify LayoutSlot of button 2");
                Verify.AreEqual(expectedButton3LayoutSlot, LayoutInformation.GetLayoutSlot(button3), "Verify LayoutSlot of button 3");
            });
        }

        [TestMethod]
        public void VerifyVerticalLayoutWithUniformSpacing()
        {
            RunOnUIThread.Execute(() =>
            {
                double width = 200;
                double height = 400;

                WrapPanel panel = new WrapPanel();
                panel.Width = width;
                panel.Height = height;
                panel.Orientation = Orientation.Vertical;
                panel.ItemSpacing = 10;

                var button1 = new Button { Content = "Button1", Width = 100, Height = 50 };
                var button2 = new Button { Content = "Button2", Width = 100, Height = 50 };
                var button3 = new Button { Content = "Button3", Width = 100, Height = 50 };

                // All buttons should fit in one column with spacing
                var expectedButton1LayoutSlot = new Rect { X = 0, Y = 0, Width = 100, Height = 50 };
                var expectedButton2LayoutSlot = new Rect { X = 0, Y = 60, Width = 100, Height = 50 }; // 50 + 10 spacing
                var expectedButton3LayoutSlot = new Rect { X = 0, Y = 120, Width = 100, Height = 50 }; // 110 + 10 spacing

                panel.Children.Add(button1);
                panel.Children.Add(button2);
                panel.Children.Add(button3);

                Content = panel;
                Content.UpdateLayout();

                Verify.AreEqual(expectedButton1LayoutSlot, LayoutInformation.GetLayoutSlot(button1), "Verify LayoutSlot of button 1");
                Verify.AreEqual(expectedButton2LayoutSlot, LayoutInformation.GetLayoutSlot(button2), "Verify LayoutSlot of button 2");
                Verify.AreEqual(expectedButton3LayoutSlot, LayoutInformation.GetLayoutSlot(button3), "Verify LayoutSlot of button 3");
            });
        }

        [TestMethod]
        public void VerifyHorizontalLayoutWithSpacingAndPadding()
        {
            RunOnUIThread.Execute(() =>
            {
                double width = 400;
                double height = 200;
                Thickness padding = new Thickness(20);

                WrapPanel panel = new WrapPanel();
                panel.Width = width;
                panel.Height = height;
                panel.Orientation = Orientation.Horizontal;
                panel.ItemSpacing = 10;
                panel.Padding = padding;

                var button1 = new Button { Content = "Button1", Width = 100, Height = 50 };
                var button2 = new Button { Content = "Button2", Width = 100, Height = 50 };
                var button3 = new Button { Content = "Button3", Width = 100, Height = 50 };

                // Buttons should be positioned with padding offset and spacing between them
                var expectedButton1LayoutSlot = new Rect { X = 20, Y = 20, Width = 100, Height = 50 };
                var expectedButton2LayoutSlot = new Rect { X = 130, Y = 20, Width = 100, Height = 50 }; // 20 + 100 + 10 spacing
                var expectedButton3LayoutSlot = new Rect { X = 240, Y = 20, Width = 100, Height = 50 }; // 130 + 100 + 10 spacing

                panel.Children.Add(button1);
                panel.Children.Add(button2);
                panel.Children.Add(button3);

                Content = panel;
                Content.UpdateLayout();

                Verify.AreEqual(expectedButton1LayoutSlot, LayoutInformation.GetLayoutSlot(button1), "Verify LayoutSlot of button 1 with padding and spacing");
                Verify.AreEqual(expectedButton2LayoutSlot, LayoutInformation.GetLayoutSlot(button2), "Verify LayoutSlot of button 2 with padding and spacing");
                Verify.AreEqual(expectedButton3LayoutSlot, LayoutInformation.GetLayoutSlot(button3), "Verify LayoutSlot of button 3 with padding and spacing");
            });
        }

        [TestMethod]
        public void VerifyVerticalLayoutWithSpacingAndPadding()
        {
            RunOnUIThread.Execute(() =>
            {
                double width = 200;
                double height = 400;
                Thickness padding = new Thickness(20);

                WrapPanel panel = new WrapPanel();
                panel.Width = width;
                panel.Height = height;
                panel.Orientation = Orientation.Vertical;
                panel.ItemSpacing = 10;
                panel.Padding = padding;

                var button1 = new Button { Content = "Button1", Width = 100, Height = 50 };
                var button2 = new Button { Content = "Button2", Width = 100, Height = 50 };
                var button3 = new Button { Content = "Button3", Width = 100, Height = 50 };

                // Buttons should be positioned with padding offset and spacing between them
                var expectedButton1LayoutSlot = new Rect { X = 20, Y = 20, Width = 100, Height = 50 };
                var expectedButton2LayoutSlot = new Rect { X = 20, Y = 80, Width = 100, Height = 50 }; // 20 + 50 + 10 spacing
                var expectedButton3LayoutSlot = new Rect { X = 20, Y = 140, Width = 100, Height = 50 }; // 80 + 50 + 10 spacing

                panel.Children.Add(button1);
                panel.Children.Add(button2);
                panel.Children.Add(button3);

                Content = panel;
                Content.UpdateLayout();

                Verify.AreEqual(expectedButton1LayoutSlot, LayoutInformation.GetLayoutSlot(button1), "Verify LayoutSlot of button 1 with padding and spacing");
                Verify.AreEqual(expectedButton2LayoutSlot, LayoutInformation.GetLayoutSlot(button2), "Verify LayoutSlot of button 2 with padding and spacing");
                Verify.AreEqual(expectedButton3LayoutSlot, LayoutInformation.GetLayoutSlot(button3), "Verify LayoutSlot of button 3 with padding and spacing");
            });
        }

        [TestMethod]
        public void VerifyWrappingBehavior()
        {
            RunOnUIThread.Execute(() =>
            {
                double width = 250; // Force wrapping
                double height = 200;

                WrapPanel panel = new WrapPanel();
                panel.Width = width;
                panel.Height = height;
                panel.Orientation = Orientation.Horizontal;

                var button1 = new Button { Content = "Button1", Width = 150, Height = 50 };
                var button2 = new Button { Content = "Button2", Width = 150, Height = 50 };

                // Second button should wrap to new row since total width (300) > panel width (250)
                var expectedButton1LayoutSlot = new Rect { X = 0, Y = 0, Width = 150, Height = 50 };
                var expectedButton2LayoutSlot = new Rect { X = 0, Y = 50, Width = 150, Height = 50 };

                panel.Children.Add(button1);
                panel.Children.Add(button2);

                Content = panel;
                Content.UpdateLayout();

                Verify.AreEqual(expectedButton1LayoutSlot, LayoutInformation.GetLayoutSlot(button1), "Verify LayoutSlot of button 1");
                Verify.AreEqual(expectedButton2LayoutSlot, LayoutInformation.GetLayoutSlot(button2), "Verify LayoutSlot of button 2 (wrapped to new row)");
            });
        }

        [TestMethod]
        public void VerifyVerticalWrappingBehavior()
        {
            RunOnUIThread.Execute(() =>
            {
                double width = 200;
                double height = 120; // Force wrapping

                WrapPanel panel = new WrapPanel();
                panel.Width = width;
                panel.Height = height;
                panel.Orientation = Orientation.Vertical;

                var button1 = new Button { Content = "Button1", Width = 100, Height = 80 };
                var button2 = new Button { Content = "Button2", Width = 100, Height = 80 };

                // Second button should wrap to new column since total height (160) > panel height (120)
                var expectedButton1LayoutSlot = new Rect { X = 0, Y = 0, Width = 100, Height = 80 };
                var expectedButton2LayoutSlot = new Rect { X = 100, Y = 0, Width = 100, Height = 80 };

                panel.Children.Add(button1);
                panel.Children.Add(button2);

                Content = panel;
                Content.UpdateLayout();

                Verify.AreEqual(expectedButton1LayoutSlot, LayoutInformation.GetLayoutSlot(button1), "Verify LayoutSlot of button 1");
                Verify.AreEqual(expectedButton2LayoutSlot, LayoutInformation.GetLayoutSlot(button2), "Verify LayoutSlot of button 2 (wrapped to new column)");
            });
        }
    }
}
