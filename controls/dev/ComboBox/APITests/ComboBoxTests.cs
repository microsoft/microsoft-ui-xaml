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
