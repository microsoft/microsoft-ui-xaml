// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using MUXControlsTestApp.Utilities;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using System.Linq;
using MUXControlsTestApp;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class ComboBoxTests : ApiTestBase
    {
        [TestMethod]
        public void VerifyComboBoxOverlayCornerRadius()
        {
            var comboBox = SetupComboBox();
            RunOnUIThread.Execute(() =>
            {
                comboBox.CornerRadius = new CornerRadius(2);
                comboBox.IsDropDownOpen = true;
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var background = TestUtilities.FindDescendents<Border>(comboBox).Where(e => e.Name == "Background").Single();
                Verify.AreEqual(new CornerRadius(2, 2, 2, 2), background.CornerRadius);

                var overlayCornerRadius = new CornerRadius(0, 0, 0, 0);
                var radius = App.Current.Resources["OverlayCornerRadius"];
                if (radius != null)
                {
                    overlayCornerRadius = (CornerRadius)radius;
                }
                var popup = VisualTreeHelper.GetOpenPopupsForXamlRoot(comboBox.XamlRoot).Last();
                var popupBorder = TestUtilities.FindDescendents<Border>(popup).Where(e => e.Name=="PopupBorder").Single();
                Verify.AreEqual(overlayCornerRadius, popupBorder.CornerRadius);
            });
        }

        [TestMethod]
        public void VerifyComboBoxEditModeCornerRadius()
        {
            var comboBox = SetupComboBox();
            RunOnUIThread.Execute(() =>
            {
                comboBox.CornerRadius = new CornerRadius(2);
                comboBox.IsEditable = true;
                comboBox.IsDropDownOpen = true;
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var editableText = TestUtilities.FindDescendents<TextBox>(comboBox).Where(e => e.Name == "EditableText").Single();
                Verify.AreEqual(new CornerRadius(2, 2, 0, 0), editableText.CornerRadius);

                var overlayCornerRadius = new CornerRadius(0, 0, 0, 0);
                var radius = App.Current.Resources["OverlayCornerRadius"];
                if (radius != null)
                {
                    overlayCornerRadius = (CornerRadius)radius;
                }
                var popup = VisualTreeHelper.GetOpenPopupsForXamlRoot(comboBox.XamlRoot).Last();
                var popupBorder = TestUtilities.FindDescendents<Border>(popup).Where(e => e.Name == "PopupBorder").Single();
                Verify.AreEqual(new CornerRadius(0, 0, overlayCornerRadius.BottomRight, overlayCornerRadius.BottomLeft), popupBorder.CornerRadius);
            });
        }

        [TestMethod]
        [TestProperty("TestPass:MaxOSVer", WindowsOSVersion._22H2)]    // This test is currently failing on 23h2.
        public void VerifyVisualTree()
        {
            var comboBox = SetupComboBox(useContent: false);
            RunOnUIThread.Execute(() =>
            {
                comboBox.IsDropDownOpen = true;
            });
            IdleSynchronizer.Wait();

            VisualTreeTestHelper.VerifyVisualTree(root: comboBox, verificationFileNamePrefix: "ComboBox");
        }

        // Scenario: Open a non-editable ComboBox with 40 items and SelectedIndex == -1, then scroll down,
        //   close without selecting, re-open, and finally open with SelectedIndex = 25.
        // Expected: When SelectedIndex == -1, opening (and re-opening after scrolling) starts at VerticalOffset 0
        //   with item 0 realized at the top of the popup and focused; when SelectedIndex >= 0, opening scrolls
        //   the selected item into view.
        // Failure means: ComboBox::GetNonPannablePopupLayout / GetPannablePopupLayout fell back to itemCount / 2
        //   for SelectedIndex == -1, or ComboBox::OnOpen skipped ArrangePopup on re-open.
        [TestMethod]
        public void VerifyUnselectedComboBoxOpensAtFirstItem()
        {
            ComboBox comboBox = null;
            RunOnUIThread.Execute(() =>
            {
                comboBox = new ComboBox
                {
                    Width = 280,
                    Margin = new Thickness(24),
                    PlaceholderText = "Select city",
                };
                for (int i = 0; i < 40; i++)
                {
                    comboBox.Items.Add($"City {i}");
                }

                Content = comboBox;
                Content.UpdateLayout();
                Verify.AreEqual(-1, comboBox.SelectedIndex);

                comboBox.IsDropDownOpen = true;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            ScrollViewer popupScrollViewer = null;
            Border popupBorder = null;
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(-1, comboBox.SelectedIndex);

                var popup = VisualTreeHelper.GetOpenPopupsForXamlRoot(comboBox.XamlRoot).Last();
                popupBorder = TestUtilities.FindDescendents<Border>(popup).Where(e => e.Name == "PopupBorder").Single();
                popupScrollViewer = TestUtilities.FindDescendents<ScrollViewer>(popup).Where(e => e.Name == "ScrollViewer").Single();

                Log.Comment($"Initial open with SelectedIndex == -1: VerticalOffset={popupScrollViewer.VerticalOffset}");
                Verify.AreEqual(0.0, popupScrollViewer.VerticalOffset);

                var firstItem = comboBox.ContainerFromIndex(0) as ComboBoxItem;
                Verify.IsNotNull(firstItem);
                var firstItemOffset = firstItem.TransformToVisual(popupBorder).TransformPoint(new global::Windows.Foundation.Point(0, 0));
                Verify.IsTrue(firstItemOffset.Y >= 0.0 && firstItemOffset.Y < firstItem.ActualHeight,
                    $"Expected item 0 near top of PopupBorder, got Y={firstItemOffset.Y}");
                Verify.AreEqual(firstItem, FocusManager.GetFocusedElement(comboBox.XamlRoot));

                Verify.IsTrue(popupScrollViewer.ChangeView(null, 20.0, null, true));
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(popupScrollViewer.VerticalOffset >= 15.0,
                    $"Expected scrolled VerticalOffset >= 15, got {popupScrollViewer.VerticalOffset}");

                comboBox.IsDropDownOpen = false;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(-1, comboBox.SelectedIndex);
                comboBox.IsDropDownOpen = true;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var popup = VisualTreeHelper.GetOpenPopupsForXamlRoot(comboBox.XamlRoot).Last();
                popupBorder = TestUtilities.FindDescendents<Border>(popup).Where(e => e.Name == "PopupBorder").Single();
                popupScrollViewer = TestUtilities.FindDescendents<ScrollViewer>(popup).Where(e => e.Name == "ScrollViewer").Single();

                Log.Comment($"Re-open after scrolling without selection: VerticalOffset={popupScrollViewer.VerticalOffset}");
                Verify.AreEqual(0.0, popupScrollViewer.VerticalOffset);

                var firstItem = comboBox.ContainerFromIndex(0) as ComboBoxItem;
                Verify.IsNotNull(firstItem);
                Verify.AreEqual(firstItem, FocusManager.GetFocusedElement(comboBox.XamlRoot));

                comboBox.IsDropDownOpen = false;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                comboBox.SelectedIndex = 25;
                comboBox.IsDropDownOpen = true;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var popup = VisualTreeHelper.GetOpenPopupsForXamlRoot(comboBox.XamlRoot).Last();
                popupBorder = TestUtilities.FindDescendents<Border>(popup).Where(e => e.Name == "PopupBorder").Single();
                popupScrollViewer = TestUtilities.FindDescendents<ScrollViewer>(popup).Where(e => e.Name == "ScrollViewer").Single();

                Log.Comment($"Open with SelectedIndex == 25: VerticalOffset={popupScrollViewer.VerticalOffset}");
                Verify.IsTrue(popupScrollViewer.VerticalOffset > 0.0);

                var selectedContainer = comboBox.ContainerFromIndex(25) as ComboBoxItem;
                Verify.IsNotNull(selectedContainer);
                var selectedOffset = selectedContainer.TransformToVisual(popupBorder).TransformPoint(new global::Windows.Foundation.Point(0, 0));
                Verify.IsTrue(selectedOffset.Y >= 0.0 && selectedOffset.Y + selectedContainer.ActualHeight <= popupBorder.ActualHeight + 1.0,
                    $"Expected selected item 25 inside PopupBorder (height={popupBorder.ActualHeight}), got Y={selectedOffset.Y}");

                comboBox.IsDropDownOpen = false;
                Content = null;
            });
            IdleSynchronizer.Wait();
        }

        // Scenario: Open unselected ComboBoxes across empty (0 items), non-pannable (6 items centered),
        //   SelectionChangedTrigger.Always (40 items), and IsEditable (40 items) configurations.
        // Expected: Empty ComboBox opens safely; 6-item, Always-trigger, and editable ComboBoxes all open at
        //   VerticalOffset 0 with item 0 visible at the top of the popup.
        // Failure means: Empty-list guard or centerItemIndex clamping regressed one of the sibling popup modes.
        [TestMethod]
        public void VerifyUnselectedComboBoxPopupVariants()
        {
            ComboBox emptyComboBox = null;
            RunOnUIThread.Execute(() =>
            {
                emptyComboBox = new ComboBox { Width = 200, PlaceholderText = "Empty" };
                Content = emptyComboBox;
                Content.UpdateLayout();

                emptyComboBox.IsDropDownOpen = true;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(emptyComboBox.IsDropDownOpen);
                Verify.AreEqual(-1, emptyComboBox.SelectedIndex);
                emptyComboBox.IsDropDownOpen = false;
                Content = null;
            });
            IdleSynchronizer.Wait();

            ComboBox shortComboBox = null;
            RunOnUIThread.Execute(() =>
            {
                shortComboBox = new ComboBox
                {
                    Width = 240,
                    VerticalAlignment = VerticalAlignment.Center,
                    PlaceholderText = "Short list",
                };
                for (int i = 0; i < 6; i++)
                {
                    shortComboBox.Items.Add($"Option {i}");
                }

                Content = shortComboBox;
                Content.UpdateLayout();
                shortComboBox.IsDropDownOpen = true;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var popup = VisualTreeHelper.GetOpenPopupsForXamlRoot(shortComboBox.XamlRoot).Last();
                var scrollViewer = TestUtilities.FindDescendents<ScrollViewer>(popup).Where(e => e.Name == "ScrollViewer").Single();
                Verify.AreEqual(0.0, scrollViewer.VerticalOffset);

                var firstItem = shortComboBox.ContainerFromIndex(0) as ComboBoxItem;
                Verify.IsNotNull(firstItem);
                Verify.AreEqual(firstItem, FocusManager.GetFocusedElement(shortComboBox.XamlRoot));

                shortComboBox.IsDropDownOpen = false;
                Content = null;
            });
            IdleSynchronizer.Wait();

            ComboBox alwaysTriggerComboBox = null;
            RunOnUIThread.Execute(() =>
            {
                alwaysTriggerComboBox = new ComboBox
                {
                    Width = 240,
                    SelectionChangedTrigger = ComboBoxSelectionChangedTrigger.Always,
                };
                for (int i = 0; i < 40; i++)
                {
                    alwaysTriggerComboBox.Items.Add($"Always {i}");
                }

                Content = alwaysTriggerComboBox;
                Content.UpdateLayout();
                alwaysTriggerComboBox.IsDropDownOpen = true;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(0, alwaysTriggerComboBox.SelectedIndex);
                var popup = VisualTreeHelper.GetOpenPopupsForXamlRoot(alwaysTriggerComboBox.XamlRoot).Last();
                var scrollViewer = TestUtilities.FindDescendents<ScrollViewer>(popup).Where(e => e.Name == "ScrollViewer").Single();
                Verify.AreEqual(0.0, scrollViewer.VerticalOffset);

                alwaysTriggerComboBox.IsDropDownOpen = false;
                Content = null;
            });
            IdleSynchronizer.Wait();

            ComboBox editableComboBox = null;
            RunOnUIThread.Execute(() =>
            {
                editableComboBox = new ComboBox
                {
                    Width = 240,
                    IsEditable = true,
                    PlaceholderText = "Editable",
                };
                for (int i = 0; i < 40; i++)
                {
                    editableComboBox.Items.Add($"Editable {i}");
                }

                Content = editableComboBox;
                Content.UpdateLayout();
                editableComboBox.IsDropDownOpen = true;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(-1, editableComboBox.SelectedIndex);
                var popup = VisualTreeHelper.GetOpenPopupsForXamlRoot(editableComboBox.XamlRoot).Last();
                var scrollViewer = TestUtilities.FindDescendents<ScrollViewer>(popup).Where(e => e.Name == "ScrollViewer").Single();
                Verify.AreEqual(0.0, scrollViewer.VerticalOffset);

                editableComboBox.IsDropDownOpen = false;
                Content = null;
            });
            IdleSynchronizer.Wait();
        }

        // Scenario: Open non-editable and editable ComboBoxes with 256 rows of alternating heights and growing widths,
        //   scroll down past the initial realized range, adjust MaxDropDownHeight while scrolled (including expanding
        //   into uncached indices), and verify live Visibility.Collapsed and item-replacement cache invalidation.
        // Expected: Scrolling to wider rows widens the popup without oscillating or jumping PopupBorder height,
        //   changing MaxDropDownHeight while scrolled converges without triggering AG_E_LAYOUT_CYCLE, and live
        //   visibility or item-replacement changes still resize the popup.
        // Failure means: ComboBox::GetItemLayoutHeight re-entered an oscillating MaxHeight feedback loop when items
        //   near SelectedIndex were virtualized out, or failed to invalidate cached heights on item updates.
        [TestMethod]
        public void VerifyScrolledComboBoxWithVaryingItemSizesDoesNotLayoutCycle()
        {
            ComboBox comboBox = null;
            Border popupBorder = null;
            ScrollViewer popupScrollViewer = null;
            double initialActualWidth = 0.0;
            double initialActualHeight = 0.0;
            double initialMaxHeight = 0.0;

            RunOnUIThread.Execute(() =>
            {
                comboBox = new ComboBox
                {
                    Width = 240,
                    MaxDropDownHeight = 385,
                    VerticalAlignment = VerticalAlignment.Top,
                    Margin = new Thickness(20),
                };

                for (int i = 0; i < 256; i++)
                {
                    var border = new Border
                    {
                        Height = (i % 2 == 0) ? 14.0 : 40.0,
                        Width = 200.0 + i,
                        HorizontalAlignment = HorizontalAlignment.Left,
                        Child = new TextBlock
                        {
                            Text = $"Row {i}",
                            VerticalAlignment = VerticalAlignment.Center,
                        },
                    };
                    comboBox.Items.Add(border);
                }

                comboBox.SelectedIndex = 0;
                Content = comboBox;
                Content.UpdateLayout();
                comboBox.IsDropDownOpen = true;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var popup = VisualTreeHelper.GetOpenPopupsForXamlRoot(comboBox.XamlRoot).Last();
                popupBorder = TestUtilities.FindDescendents<Border>(popup).Where(e => e.Name == "PopupBorder").Single();
                popupScrollViewer = TestUtilities.FindDescendents<ScrollViewer>(popup).Where(e => e.Name == "ScrollViewer").Single();

                initialActualWidth = popupBorder.ActualWidth;
                initialActualHeight = popupBorder.ActualHeight;
                initialMaxHeight = popupBorder.MaxHeight;
                Log.Comment($"Initial open: ActualWidth={initialActualWidth}, ActualHeight={initialActualHeight}, MaxHeight={initialMaxHeight}");
                Verify.IsGreaterThan(initialActualHeight, 100.0);

                // Exercise both odd and even visible-start parities so whichever parity aligns with
                // CarouselPanel's before-buffer boundary triggers the A <-> B feedback loop when unfixed.
                Verify.IsTrue(popupScrollViewer.ChangeView(null, 25.55, null, true));
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(popupScrollViewer.ChangeView(null, 26.55, null, true));
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Log.Comment($"After scroll to 26.55: Offset={popupScrollViewer.VerticalOffset}, ActualWidth={popupBorder.ActualWidth}, ActualHeight={popupBorder.ActualHeight}, MaxHeight={popupBorder.MaxHeight}");
                Verify.IsTrue(comboBox.IsDropDownOpen);
                Verify.IsTrue(popupScrollViewer.VerticalOffset >= 20.0, $"Expected VerticalOffset >= 20, got {popupScrollViewer.VerticalOffset}");
                Verify.IsNull(comboBox.ContainerFromIndex(2), "Item 2 beyond the focused/next-focusable pair should be virtualized out after scrolling to offset 26.55.");
                Verify.IsGreaterThan(popupBorder.ActualWidth, initialActualWidth, "Scrolling to wider rows should still widen the popup.");
                Verify.AreEqual(initialMaxHeight, popupBorder.MaxHeight, "Scrolling to wider rows must not jump PopupBorder.MaxHeight.");
                Verify.IsLessThan(global::System.Math.Abs(popupBorder.ActualHeight - initialActualHeight), 1.0,
                    $"Expected stable ActualHeight={initialActualHeight}, got {popupBorder.ActualHeight}");

                // Changing MaxDropDownHeight while scrolled down (with initial items virtualized out) must still
                // update popup height cleanly without entering a layout cycle.
                comboBox.MaxDropDownHeight = 240.0;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Log.Comment($"After MaxDropDownHeight=240 while scrolled: ActualHeight={popupBorder.ActualHeight}, MaxHeight={popupBorder.MaxHeight}");
                Verify.IsTrue(comboBox.IsDropDownOpen);
                Verify.IsLessThan(popupBorder.ActualHeight, 280.0, "Updated MaxDropDownHeight should constrain the open popup.");

                comboBox.IsDropDownOpen = false;
                Content = null;
            });
            IdleSynchronizer.Wait();

            // Editable ComboBox variant with alternating heights and growing widths.
            ComboBox editableComboBox = null;
            RunOnUIThread.Execute(() =>
            {
                editableComboBox = new ComboBox
                {
                    Width = 240,
                    MaxDropDownHeight = 385,
                    IsEditable = true,
                    VerticalAlignment = VerticalAlignment.Top,
                    Margin = new Thickness(20),
                };

                for (int i = 0; i < 256; i++)
                {
                    var border = new Border
                    {
                        Height = (i % 2 == 0) ? 14.0 : 40.0,
                        Width = 200.0 + i,
                        HorizontalAlignment = HorizontalAlignment.Left,
                        Child = new TextBlock
                        {
                            Text = $"EditableRow {i}",
                            VerticalAlignment = VerticalAlignment.Center,
                        },
                    };
                    editableComboBox.Items.Add(border);
                }

                Content = editableComboBox;
                Content.UpdateLayout();
                editableComboBox.IsDropDownOpen = true;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var popup = VisualTreeHelper.GetOpenPopupsForXamlRoot(editableComboBox.XamlRoot).Last();
                popupBorder = TestUtilities.FindDescendents<Border>(popup).Where(e => e.Name == "PopupBorder").Single();
                popupScrollViewer = TestUtilities.FindDescendents<ScrollViewer>(popup).Where(e => e.Name == "ScrollViewer").Single();
                initialActualWidth = popupBorder.ActualWidth;
                initialMaxHeight = popupBorder.MaxHeight;

                Verify.IsTrue(popupScrollViewer.ChangeView(null, 25.55, null, true));
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(editableComboBox.IsDropDownOpen);
                Verify.IsNull(editableComboBox.ContainerFromIndex(0), "Editable item 0 should be virtualized out after scrolling.");
                Verify.IsGreaterThan(popupBorder.ActualWidth, initialActualWidth, "Editable popup should widen when scrolling to wider rows.");
                Verify.AreEqual(initialMaxHeight, popupBorder.MaxHeight, "Editable popup MaxHeight should stay stable while scrolling.");

                editableComboBox.IsDropDownOpen = false;
                Content = null;
            });
            IdleSynchronizer.Wait();

            // Scrolled expansion with alternating-height unrealized items: open short so only the first few items
            // are cached, scroll far enough that items 2..14 are virtualized out, then expand MaxDropDownHeight
            // across both scroll parities so uncached indices exercise the latched fallback height without cycling.
            ComboBox scrolledExpandComboBox = null;
            RunOnUIThread.Execute(() =>
            {
                scrolledExpandComboBox = new ComboBox
                {
                    Width = 240,
                    MaxDropDownHeight = 140,
                    VerticalAlignment = VerticalAlignment.Top,
                    Margin = new Thickness(20),
                };

                for (int i = 0; i < 256; i++)
                {
                    var border = new Border
                    {
                        Height = (i % 2 == 0) ? 14.0 : 40.0,
                        Width = 200.0 + i,
                        HorizontalAlignment = HorizontalAlignment.Left,
                        Child = new TextBlock
                        {
                            Text = $"ExpandRow {i}",
                            VerticalAlignment = VerticalAlignment.Center,
                        },
                    };
                    scrolledExpandComboBox.Items.Add(border);
                }

                scrolledExpandComboBox.SelectedIndex = 0;
                Content = scrolledExpandComboBox;
                Content.UpdateLayout();
                scrolledExpandComboBox.IsDropDownOpen = true;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var popup = VisualTreeHelper.GetOpenPopupsForXamlRoot(scrolledExpandComboBox.XamlRoot).Last();
                popupBorder = TestUtilities.FindDescendents<Border>(popup).Where(e => e.Name == "PopupBorder").Single();
                popupScrollViewer = TestUtilities.FindDescendents<ScrollViewer>(popup).Where(e => e.Name == "ScrollViewer").Single();
                initialActualHeight = popupBorder.ActualHeight;

                Verify.IsTrue(popupScrollViewer.ChangeView(null, 40.5, null, true));
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsNull(scrolledExpandComboBox.ContainerFromIndex(2), "Item 2 beyond the focused/next-focusable pair should be virtualized out at offset 40.5.");
                Verify.IsNull(scrolledExpandComboBox.ContainerFromIndex(14), "Item 14 should be unrealized at offset 40.5.");

                scrolledExpandComboBox.MaxDropDownHeight = 385.0;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            double expandedMaxHeight = 0.0;
            double expandedActualWidth = 0.0;
            RunOnUIThread.Execute(() =>
            {
                expandedMaxHeight = popupBorder.MaxHeight;
                expandedActualWidth = popupBorder.ActualWidth;
                Log.Comment($"After scrolled MaxDropDownHeight expansion at 40.5: InitialHeight={initialActualHeight}, ExpandedHeight={popupBorder.ActualHeight}, MaxHeight={expandedMaxHeight}, Width={expandedActualWidth}");
                Verify.IsTrue(scrolledExpandComboBox.IsDropDownOpen);
                Verify.IsGreaterThan(popupBorder.ActualHeight, initialActualHeight + 100.0,
                    "Expanding MaxDropDownHeight while scrolled should still grow the popup.");

                // Scroll to the opposite parity (45.5 vs 40.5) and wider rows while expanded to verify
                // the latched fallback height keeps MaxHeight stable across width-driven ArrangePopup calls.
                Verify.IsTrue(popupScrollViewer.ChangeView(null, 45.5, null, true));
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Log.Comment($"After opposite-parity scroll to 45.5: ActualWidth={popupBorder.ActualWidth}, MaxHeight={popupBorder.MaxHeight}");
                Verify.IsTrue(scrolledExpandComboBox.IsDropDownOpen);
                Verify.IsGreaterThan(popupBorder.ActualWidth, expandedActualWidth,
                    "Opposite-parity scroll should widen the expanded popup.");
                Verify.AreEqual(expandedMaxHeight, popupBorder.MaxHeight,
                    "Latched fallback height should prevent MaxHeight jumps across opposite-parity scrolls.");

                scrolledExpandComboBox.IsDropDownOpen = false;
                Content = null;
            });
            IdleSynchronizer.Wait();

            // Dynamic item visibility change while open and cache invalidation across close/replace/reopen.
            ComboBox dynamicComboBox = null;
            ComboBoxItem itemToCollapse1 = null;
            ComboBoxItem itemToCollapse2 = null;
            RunOnUIThread.Execute(() =>
            {
                dynamicComboBox = new ComboBox
                {
                    Width = 240,
                    VerticalAlignment = VerticalAlignment.Top,
                    Margin = new Thickness(20),
                };

                for (int i = 0; i < 5; i++)
                {
                    var item = new ComboBoxItem { Content = $"DynamicItem {i}" };
                    if (i == 3) { itemToCollapse1 = item; }
                    if (i == 4) { itemToCollapse2 = item; }
                    dynamicComboBox.Items.Add(item);
                }

                dynamicComboBox.SelectedIndex = 0;
                Content = dynamicComboBox;
                Content.UpdateLayout();
                dynamicComboBox.IsDropDownOpen = true;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var popup = VisualTreeHelper.GetOpenPopupsForXamlRoot(dynamicComboBox.XamlRoot).Last();
                popupBorder = TestUtilities.FindDescendents<Border>(popup).Where(e => e.Name == "PopupBorder").Single();
                initialActualHeight = popupBorder.ActualHeight;

                itemToCollapse1.Visibility = Visibility.Collapsed;
                itemToCollapse2.Visibility = Visibility.Collapsed;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Log.Comment($"After collapsing 2 of 5 items while open: InitialHeight={initialActualHeight}, NewHeight={popupBorder.ActualHeight}");
                Verify.IsTrue(dynamicComboBox.IsDropDownOpen);
                Verify.IsLessThan(popupBorder.ActualHeight, initialActualHeight - 40.0,
                    "Collapsing items while the dropdown is open should still shrink the popup.");

                // Close, replace with taller items, and reopen to verify cached item heights are invalidated.
                dynamicComboBox.IsDropDownOpen = false;
                Content.UpdateLayout();
                dynamicComboBox.Items.Clear();
                for (int i = 0; i < 5; i++)
                {
                    dynamicComboBox.Items.Add(new Border
                    {
                        Height = 60.0,
                        Width = 200.0,
                        Child = new TextBlock { Text = $"TallItem {i}" },
                    });
                }
                dynamicComboBox.SelectedIndex = 0;
                dynamicComboBox.IsDropDownOpen = true;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var popup = VisualTreeHelper.GetOpenPopupsForXamlRoot(dynamicComboBox.XamlRoot).Last();
                popupBorder = TestUtilities.FindDescendents<Border>(popup).Where(e => e.Name == "PopupBorder").Single();
                Log.Comment($"After replacing items with taller rows and reopening: InitialHeight={initialActualHeight}, ReopenedHeight={popupBorder.ActualHeight}");
                Verify.IsTrue(dynamicComboBox.IsDropDownOpen);
                Verify.IsGreaterThan(popupBorder.ActualHeight, initialActualHeight + 40.0,
                    "Replacing items and reopening should invalidate cached heights and grow the popup.");

                dynamicComboBox.IsDropDownOpen = false;
                Content = null;
            });
            IdleSynchronizer.Wait();
        }

        private ComboBox SetupComboBox(bool useContent = true)
        {
            ComboBox comboBox = null;
            RunOnUIThread.Execute(() =>
            {
                comboBox = new ComboBox();
                comboBox.Items.Add("Item 1");
                comboBox.Items.Add("Item 2");
                comboBox.Items.Add("Item 3");
                comboBox.Items.Add("Item 4");
                comboBox.Items.Add("Item 5");
                comboBox.Items.Add("Item 6");
                if (useContent)
                {
                    Content = comboBox;
                    Content.UpdateLayout();
                }
            });
            if(!useContent)
            {
                TestUtilities.SetAsVisualTreeRoot(comboBox);
            }
            Verify.IsNotNull(comboBox);
            return comboBox;
        }

    }
}
