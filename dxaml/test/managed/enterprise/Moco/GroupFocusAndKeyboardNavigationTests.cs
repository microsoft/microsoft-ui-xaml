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
    public class GroupFocusAndKeyboardNavigationTests : XamlTestsBase
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

        public class MyGroup : ObservableCollection<int>
        {
            public string GroupName { get; set; }
            public string SubName { get; set; }
        }

        bool IsAncestorOf(DependencyObject ancestor, DependencyObject child)
        {
            if (ancestor == child)
            {
                return true;
            }
            if (ancestor != null && child != null)
            {
                return IsAncestorOf(ancestor, VisualTreeHelper.GetParent(child));
            }
            return false;
        }

        [TestMethod]
        public void ValidateFocusOrderWithSelectedItem()
        {
            // Expected focus order: (other)->Header->Selected Item->Footer->(other)
            var lv = CreateListViewWithGrouping();
            const int selectedIndex = 2;
            Button beforeButton = null;
            Button afterButton = null;
            Button headerButton = null;
            Button footerButton = null;
            var beforeButtonFocused = new AutoResetEvent(false);
            UIExecutor.Execute(() =>
            {
                StackPanel sp = new StackPanel();
                beforeButton = new Button { Content = "button before ListView" };
                beforeButton.GotFocus += (s, e) =>
                {
                    beforeButtonFocused.Set();
                };
                afterButton = new Button { Content = "button after ListView" };
                headerButton = new Button { Content = "button in Header" };
                footerButton = new Button { Content = "button in footer" };
                lv.Header = headerButton;
                lv.Footer = footerButton;

                sp.Children.Add(beforeButton);
                sp.Children.Add(lv);
                sp.Children.Add(afterButton);

                TestServices.WindowHelper.WindowContent = sp;
            });

            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("put focus on the button before ListView");
            UIExecutor.Execute(() =>
            {
                lv.SelectedIndex = selectedIndex;
                beforeButton.Focus(FocusState.Programmatic);
            });
            TestServices.WindowHelper.WaitForIdle();

            beforeButtonFocused.WaitOne();

            TestServices.KeyboardHelper.Tab();
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus should be on HeaderButton");
            UIExecutor.Execute(() =>
            {
                var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot);
                Verify.AreEqual(focusedElement, headerButton);
            });

            TestServices.KeyboardHelper.Tab();
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment(string.Format("Focus should be on the selected item: item #{0}", selectedIndex));
            UIExecutor.Execute(() =>
            {
                var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot);
                var selectedItemContainer = lv.ContainerFromIndex(selectedIndex);
                Verify.AreEqual(focusedElement, selectedItemContainer);
            });

            TestServices.KeyboardHelper.Tab();
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus should be on FooterButton");
            UIExecutor.Execute(() =>
            {
                var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot) as Button;
                Verify.AreEqual(focusedElement, footerButton);
            });

            TestServices.KeyboardHelper.Tab();
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus should be on the button after ListView");
            UIExecutor.Execute(() =>
            {
                var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot);
                Verify.AreEqual(focusedElement, afterButton);
            });

            TestServices.KeyboardHelper.ShiftTab();
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus should be on FooterButton");
            UIExecutor.Execute(() =>
            {
                var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot) as Button;
                Verify.AreEqual(focusedElement, footerButton);
            });

            TestServices.KeyboardHelper.ShiftTab();
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment(string.Format("Focus should be on the selected item: item #{0}", selectedIndex));
            UIExecutor.Execute(() =>
            {
                var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot);
                var selectedItemContainer = lv.ContainerFromIndex(selectedIndex);
                Verify.AreEqual(focusedElement, selectedItemContainer);
            });

            TestServices.KeyboardHelper.ShiftTab();
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus should be on HeaderButton");
            UIExecutor.Execute(() =>
            {
                var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot);
                Verify.AreEqual(focusedElement, headerButton);
            });

            TestServices.KeyboardHelper.ShiftTab();
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus should be on the button before ListView");
            UIExecutor.Execute(() =>
            {
                var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot);
                Verify.AreEqual(focusedElement, beforeButton);
            });
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        public void ValidateFocusOrderWithoutSelectedItem()
        {
            // Expected focus order: (other)->Header->GroupHeader->Footer->(other)
            var lv = CreateListViewWithGrouping();
            DependencyObject firstGroupHeaderContainer = null;
            Button beforeButton = null;
            Button afterButton = null;
            Button headerButton = null;
            Button footerButton = null;
            var beforeButtonFocused = new AutoResetEvent(false);
            UIExecutor.Execute(() =>
            {
                // By default, ListViews with grouping will automatically select the first
                // item if selection mode is set to Single.  Set it to None so that we
                // can disable that code-path and test the unselected case.
                lv.SelectionMode = ListViewSelectionMode.None;
                lv.SelectedIndex = -1;

                StackPanel sp = new StackPanel();
                beforeButton = new Button { Content = "button before ListView" };
                beforeButton.GotFocus += (s, e) =>
                {
                    beforeButtonFocused.Set();
                };
                afterButton = new Button { Content = "button after ListView" };
                headerButton = new Button { Content = "button in Header" };
                footerButton = new Button { Content = "button in footer" };
                lv.Header = headerButton;
                lv.Footer = footerButton;

                sp.Children.Add(beforeButton);
                sp.Children.Add(lv);
                sp.Children.Add(afterButton);

                TestServices.WindowHelper.WindowContent = sp;
            });

            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("put focus on the button before ListView");
            UIExecutor.Execute(() =>
            {
                // Make sure nothing is selected.
                lv.SelectedIndex = -1;

                firstGroupHeaderContainer = lv.GroupHeaderContainerFromItemContainer(lv.ContainerFromIndex(0));
                beforeButton.Focus(FocusState.Programmatic);
            });
            TestServices.WindowHelper.WaitForIdle();

            beforeButtonFocused.WaitOne();

            TestServices.KeyboardHelper.Tab();
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus should be on HeaderButton");
            UIExecutor.Execute(() =>
            {
                var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot);
                Verify.AreEqual(focusedElement, headerButton);
            });

            TestServices.KeyboardHelper.Tab();
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus should be on GroupHeader#0");
            UIExecutor.Execute(() =>
            {
                var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot);
                Verify.AreEqual(focusedElement, firstGroupHeaderContainer);
            });

            TestServices.KeyboardHelper.Tab();
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus should be on the first button in GroupHeader#0");
            UIExecutor.Execute(() =>
            {
                var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot) as Button;
                Verify.IsTrue(IsAncestorOf(firstGroupHeaderContainer, focusedElement));
                Verify.AreEqual(focusedElement.Content, "Group#0");
            });

            TestServices.KeyboardHelper.Tab();
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus should be on the second button in GroupHeader#0");
            UIExecutor.Execute(() =>
            {
                var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot) as Button;
                Verify.IsTrue(IsAncestorOf(firstGroupHeaderContainer, focusedElement));
                Verify.AreEqual(focusedElement.Content, "0");
            });

            TestServices.KeyboardHelper.Tab();
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus should be on FooterButton");
            UIExecutor.Execute(() =>
            {
                var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot) as Button;
                Verify.AreEqual(focusedElement, footerButton);
            });

            TestServices.KeyboardHelper.Tab();
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus should be on the button after ListView");
            UIExecutor.Execute(() =>
            {
                var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot);
                Verify.AreEqual(focusedElement, afterButton);
            });

            TestServices.KeyboardHelper.ShiftTab();
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus should be on FooterButton");
            UIExecutor.Execute(() =>
            {
                var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot) as Button;
                Verify.AreEqual(focusedElement, footerButton);
            });

            TestServices.KeyboardHelper.ShiftTab();
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus should be on the second button in GroupHeader#0");
            UIExecutor.Execute(() =>
            {
                var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot) as Button;
                Verify.IsTrue(IsAncestorOf(firstGroupHeaderContainer, focusedElement));
                Verify.AreEqual(focusedElement.Content, "0");
            });

            TestServices.KeyboardHelper.ShiftTab();
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus should be on the first button in GroupHeader#0");
            UIExecutor.Execute(() =>
            {
                var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot) as Button;
                Verify.IsTrue(IsAncestorOf(firstGroupHeaderContainer, focusedElement));
                Verify.AreEqual(focusedElement.Content, "Group#0");
            });

            TestServices.KeyboardHelper.ShiftTab();
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus should be on GroupHeader#0");
            UIExecutor.Execute(() =>
            {
                var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot);
                Verify.AreEqual(focusedElement, firstGroupHeaderContainer);
            });

            TestServices.KeyboardHelper.ShiftTab();
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus should be on HeaderButton");
            UIExecutor.Execute(() =>
            {
                var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot);
                Verify.AreEqual(focusedElement, headerButton);
            });

            TestServices.KeyboardHelper.ShiftTab();
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus should be on the button before ListView");
            UIExecutor.Execute(() =>
            {
                var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot);
                Verify.AreEqual(focusedElement, beforeButton);
            });
        }

        [TestMethod]
        public void ValidateFocusOrderWithoutSelectedItemAndWithDisabledGroupHeaders()
        {
            // Expected focus order: (other)->Header->1st Item->Footer->(other)
            var lv = CreateListViewWithGrouping();
            Button beforeButton = null;
            Button afterButton = null;
            Button headerButton = null;
            Button footerButton = null;
            var beforeButtonFocused = new AutoResetEvent(false);
            UIExecutor.Execute(() =>
            {
                // By default, ListViews with grouping will automatically select the first
                // item if selection mode is set to Single.  Set it to None so that we
                // can disable that code-path and test the unselected case.
                lv.SelectionMode = ListViewSelectionMode.None;
                lv.SelectedIndex = -1;

                var groupHeaderStyle = new Style() { TargetType = typeof(ContentControl) };
                groupHeaderStyle.Setters.Add(new Setter(Control.IsEnabledProperty, false));
                lv.GroupStyle[0].HeaderContainerStyle = groupHeaderStyle;

                StackPanel sp = new StackPanel();
                beforeButton = new Button { Content = "button before ListView" };
                beforeButton.GotFocus += (s, e) =>
                {
                    beforeButtonFocused.Set();
                };
                afterButton = new Button { Content = "button after ListView" };
                headerButton = new Button { Content = "button in Header" };
                footerButton = new Button { Content = "button in footer" };
                lv.Header = headerButton;
                lv.Footer = footerButton;

                sp.Children.Add(beforeButton);
                sp.Children.Add(lv);
                sp.Children.Add(afterButton);

                TestServices.WindowHelper.WindowContent = sp;
            });

            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("put focus on the button before ListView");
            UIExecutor.Execute(() =>
            {
                // Make sure nothing is selected.
                lv.SelectedIndex = -1;

                beforeButton.Focus(FocusState.Programmatic);
            });
            TestServices.WindowHelper.WaitForIdle();

            beforeButtonFocused.WaitOne();

            TestServices.KeyboardHelper.Tab();
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus should be on HeaderButton");
            UIExecutor.Execute(() =>
            {
                var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot);
                Verify.AreEqual(focusedElement, headerButton);
            });

            TestServices.KeyboardHelper.Tab();
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus should be on the first item");
            UIExecutor.Execute(() =>
            {
                var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot);
                var firstItemContainer = lv.ContainerFromIndex(0);
                Verify.AreEqual(focusedElement, firstItemContainer);
            });

            TestServices.KeyboardHelper.Tab();
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus should be on FooterButton");
            UIExecutor.Execute(() =>
            {
                var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot) as Button;
                Verify.AreEqual(focusedElement, footerButton);
            });

            TestServices.KeyboardHelper.Tab();
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus should be on the button after ListView");
            UIExecutor.Execute(() =>
            {
                var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot);
                Verify.AreEqual(focusedElement, afterButton);
            });

            TestServices.KeyboardHelper.ShiftTab();
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus should be on FooterButton");
            UIExecutor.Execute(() =>
            {
                var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot) as Button;
                Verify.AreEqual(focusedElement, footerButton);
            });

            TestServices.KeyboardHelper.ShiftTab();
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus should be on the first item");
            UIExecutor.Execute(() =>
            {
                var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot);
                var firstItemContainer = lv.ContainerFromIndex(0);
                Verify.AreEqual(focusedElement, firstItemContainer);
            });

            TestServices.KeyboardHelper.ShiftTab();
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus should be on HeaderButton");
            UIExecutor.Execute(() =>
            {
                var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot);
                Verify.AreEqual(focusedElement, headerButton);
            });

            TestServices.KeyboardHelper.ShiftTab();
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus should be on the button before ListView");
            UIExecutor.Execute(() =>
            {
                var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot);
                Verify.AreEqual(focusedElement, beforeButton);
            });
        }

        [TestMethod]
        public void ValidateKeyboardNavigation()
        {
            // Focus should move between headers and items.
            var lv = CreateListViewWithGrouping();
            UIExecutor.Execute(() =>
            {
                // By default, ListViews with grouping will automatically select the first
                // item if selection mode is set to Single.  Set it to None so that we
                // can disable that code-path and test the unselected case.
                lv.SelectionMode = ListViewSelectionMode.None;
                lv.SelectedIndex = -1;

                TestServices.WindowHelper.WindowContent = lv;
            });

            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("put focus on the first group");
            UIExecutor.Execute(() =>
            {
                var firstItemContainer = lv.ContainerFromIndex(0);
                var firstGroupHeaderContainer = lv.GroupHeaderContainerFromItemContainer(firstItemContainer);
                (firstGroupHeaderContainer as Control).Focus(FocusState.Programmatic);
            });

            TestServices.KeyboardHelper.Down();
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus should be on the first item");
            UIExecutor.Execute(() =>
            {
                var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot);
                var firstItemContainer = lv.ContainerFromIndex(0);
                Verify.AreEqual(focusedElement, firstItemContainer);
            });

            TestServices.KeyboardHelper.Up();
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus should be back to first group");
            UIExecutor.Execute(() =>
            {
                var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot);
                var firstItemContainer = lv.ContainerFromIndex(0);
                var firstGroupHeaderContainer = lv.GroupHeaderContainerFromItemContainer(firstItemContainer);
                Verify.AreEqual(focusedElement, firstGroupHeaderContainer);
            });

            Log.Comment("put focus on the last item in the first group");
            UIExecutor.Execute(() =>
            {
                var itemContainer = lv.ContainerFromIndex(9);
                (itemContainer as Control).Focus(FocusState.Programmatic);
            });

            TestServices.KeyboardHelper.Down();
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus should be on the second group");
            UIExecutor.Execute(() =>
            {
                var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot);
                var itemContainer = lv.ContainerFromIndex(10);
                var secondGroupHeaderContainer = lv.GroupHeaderContainerFromItemContainer(itemContainer);
                Verify.AreEqual(focusedElement, secondGroupHeaderContainer);
            });

            TestServices.KeyboardHelper.Down();
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus should be on the first item in the second group");
            UIExecutor.Execute(() =>
            {
                var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot);
                var itemContainer = lv.ContainerFromIndex(10);
                Verify.AreEqual(focusedElement, itemContainer);
            });

            TestServices.KeyboardHelper.Up();
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus should be on the second group");
            UIExecutor.Execute(() =>
            {
                var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot);
                var itemContainer = lv.ContainerFromIndex(10);
                var secondGroupHeaderContainer = lv.GroupHeaderContainerFromItemContainer(itemContainer);
                Verify.AreEqual(focusedElement, secondGroupHeaderContainer);
            });

            TestServices.KeyboardHelper.Up();
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus should be on the last item in the first group");
            UIExecutor.Execute(() =>
            {
                var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot);
                var itemContainer = lv.ContainerFromIndex(9);
                Verify.AreEqual(focusedElement, itemContainer);
            });

            TestServices.KeyboardHelper.PressKeySequence("$d$_end#$u$_end");
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus should be on the last item");
            UIExecutor.Execute(() =>
            {
                var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot);
                var lastItemContainer = lv.ContainerFromIndex(99);
                Verify.AreEqual(focusedElement, lastItemContainer);
            });

            TestServices.KeyboardHelper.PressKeySequence("$d$_home#$u$_home");
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus should be back to first item");
            UIExecutor.Execute(() =>
            {
                var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot);
                var firstItemContainer = lv.ContainerFromIndex(0);
                Verify.AreEqual(focusedElement, firstItemContainer);
            });
        }

        [TestMethod]
        public void ValidateKeyboardNavigationSkipsDisabledGroupHeaders()
        {
            // Focus should move between headers and items.
            var lv = CreateListViewWithGrouping();
            UIExecutor.Execute(() =>
            {
                // By default, ListViews with grouping will automatically select the first
                // item if selection mode is set to Single.  Set it to None so that we
                // can disable that code-path and test the unselected case.
                lv.SelectionMode = ListViewSelectionMode.None;
                lv.SelectedIndex = -1;

                var groupHeaderStyle = new Style() { TargetType = typeof(ContentControl) };
                groupHeaderStyle.Setters.Add(new Setter(Control.IsEnabledProperty, false));
                lv.GroupStyle[0].HeaderContainerStyle = groupHeaderStyle;

                TestServices.WindowHelper.WindowContent = lv;
            });

            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("put focus on the last item in the first group");
            UIExecutor.Execute(() =>
            {
                var itemContainer = lv.ContainerFromIndex(9);
                (itemContainer as Control).Focus(FocusState.Programmatic);
            });

            TestServices.KeyboardHelper.Down();
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus should be on the first item in the second group");
            UIExecutor.Execute(() =>
            {
                var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot);
                var itemContainer = lv.ContainerFromIndex(10);
                Verify.AreEqual(focusedElement, itemContainer);
            });

            TestServices.KeyboardHelper.Up();
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus should be on the last item in the first group");
            UIExecutor.Execute(() =>
            {
                var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot);
                var itemContainer = lv.ContainerFromIndex(9);
                Verify.AreEqual(focusedElement, itemContainer);
            });
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        public void CanTabOntoFirstNonEmptyGroupHeaderWithHiddenEmptyGroupsAtTheBeginning()
        {
            ListView lv = null;
            Button button = null;

            UIExecutor.Execute(() =>
            {
                var cvs = CreateCollectionViewSourceWithGrouping();
                var groups = cvs.Source as ObservableCollection<MyGroup>;

                // Add groups at the beginning.
                const int numEmptyGroups = 2;
                for (int i = 0; i < numEmptyGroups; i++)
                {
                    var group = new MyGroup();
                    group.GroupName = string.Format("Empty Group");
                    groups.Insert(0, group);
                }

                // By default, ListViews with grouping will automatically select the first
                // item if selection mode is set to Single.  Set it to None so that we
                // can disable that code-path.
                lv = (ListView)XamlReader.Load(@"
                        <ListView xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                            Height='600' Width='200'
                            SelectionMode='None'
                            SelectedIndex='-1'>
                            <ListView.GroupStyle>
                              <GroupStyle HidesIfEmpty='True'>
                                <GroupStyle.HeaderTemplate>
                                  <DataTemplate>
                                    <StackPanel>
                                      <TextBlock Text='{Binding GroupName}'/>
                                      <TextBlock Text='{Binding SubName}' />
                                    </StackPanel>
                                  </DataTemplate>
                                </GroupStyle.HeaderTemplate>
                              </GroupStyle>
                            </ListView.GroupStyle>
                        </ListView>");

                lv.ItemsSource = cvs.View;

                button = new Button() { Content = "Before" };

                var root = new StackPanel();
                root.Children.Add(button);
                root.Children.Add(lv);

                TestServices.WindowHelper.WindowContent = root;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                lv.SelectedIndex = -1;
                button.Focus(FocusState.Keyboard);
            });
            TestServices.WindowHelper.WaitForIdle();

            for (int entry = 0; entry < 2; entry++)
            {
                Log.Comment("Tab into the ListView, which should focus the header for the first group with items.");
                TestServices.KeyboardHelper.Tab();
                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    // Item 0 will be in the first group with items.
                    var itemContainer = lv.ContainerFromIndex(0);
                    var firstNonEmptyGroupHeader = lv.GroupHeaderContainerFromItemContainer(itemContainer);
                    Verify.IsNotNull(firstNonEmptyGroupHeader);
                    Verify.AreEqual(firstNonEmptyGroupHeader, FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot));
                    Verify.AreEqual(-1, lv.SelectedIndex);
                });

                TestServices.KeyboardHelper.ShiftTab();
                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    Verify.AreEqual(button, FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot));
                });
            }

            Log.Comment("After an item receives focus, tabbing back in should restore that item instead.");
            UIExecutor.Execute(() =>
            {
                Verify.IsTrue(((Control)lv.ContainerFromIndex(1)).Focus(FocusState.Keyboard));
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsTrue(button.Focus(FocusState.Keyboard));
            });
            TestServices.WindowHelper.WaitForIdle();

            TestServices.KeyboardHelper.Tab();
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(lv.ContainerFromIndex(1), FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot));
                Verify.AreEqual(-1, lv.SelectedIndex);
            });
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        public void ValidateArrowNavigationWithHiddenEmptyGroups()
        {
            ListView lv = null;
            Button button = null;

            const uint numNonEmptyGroups = 3;
            const uint itemsPerGroup = 2;

            UIExecutor.Execute(() =>
            {
                var cvs = CreateCollectionViewSourceWithGrouping(numNonEmptyGroups, itemsPerGroup);
                var groups = cvs.Source as ObservableCollection<MyGroup>;

                // Add groups at the beginning.
                const int numEmptyGroups = 2;
                for (int i = 0; i < numEmptyGroups; i++)
                {
                    var group = new MyGroup();
                    group.GroupName = string.Format("Empty Group");
                    groups.Insert(0, group);
                }

                // By default, ListViews with grouping will automatically select the first
                // item if selection mode is set to Single.  Set it to None so that we
                // can disable that code-path.
                lv = (ListView)XamlReader.Load(@"
                        <ListView xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                            Height='600' Width='200'
                            SelectionMode='None'
                            SelectedIndex='-1'>
                            <ListView.GroupStyle>
                              <GroupStyle HidesIfEmpty='True'>
                                <GroupStyle.HeaderTemplate>
                                  <DataTemplate>
                                    <StackPanel>
                                      <TextBlock Text='{Binding GroupName}'/>
                                      <TextBlock Text='{Binding SubName}' />
                                    </StackPanel>
                                  </DataTemplate>
                                </GroupStyle.HeaderTemplate>
                              </GroupStyle>
                            </ListView.GroupStyle>
                        </ListView>");

                lv.ItemsSource = cvs.View;

                button = new Button() { Content = "Before" };

                // Create our root with XY focus navigation enabled so that we can use arrow keys through
                // the test and not worry about tabbing.
                var root = new StackPanel() { XYFocusKeyboardNavigation = XYFocusKeyboardNavigationMode.Enabled };
                root.Children.Add(button);
                root.Children.Add(lv);

                TestServices.WindowHelper.WindowContent = root;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                button.Focus(FocusState.Keyboard);
            });
            TestServices.WindowHelper.WaitForIdle();

            for (uint groupIndex = 0; groupIndex < numNonEmptyGroups; ++groupIndex)
            {
                Log.Comment($"Arrow down onto a header for group #{groupIndex}.");
                TestServices.KeyboardHelper.Down();
                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    var itemContainer = lv.ContainerFromIndex((int)(groupIndex * itemsPerGroup));
                    var groupHeader = lv.GroupHeaderContainerFromItemContainer(itemContainer);
                    Verify.AreEqual(groupHeader, FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot));
                });

                Log.Comment($"Arrow down through group #{groupIndex}'s items.");
                for (uint itemIndex = 0; itemIndex < itemsPerGroup; ++itemIndex)
                {
                    TestServices.KeyboardHelper.Down();
                    TestServices.WindowHelper.WaitForIdle();

                    UIExecutor.Execute(() =>
                    {
                        var itemContainer = lv.ContainerFromIndex((int)(groupIndex * itemsPerGroup + itemIndex));
                        Verify.AreEqual(itemContainer, FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot));
                    });
                }
            }
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        public void ValidateArrowNavigationWithNonFocusableGroups()
        {
            ListView lv = null;
            Button button = null;

            const uint numGroups = 3;
            const uint itemsPerGroup = 2;

            UIExecutor.Execute(() =>
            {
                var cvs = CreateCollectionViewSourceWithGrouping(numGroups, itemsPerGroup);

                // By default, ListViews with grouping will automatically select the first
                // item if selection mode is set to Single.  Set it to None so that we
                // can disable that code-path.
                lv = (ListView)XamlReader.Load(@"
                        <ListView xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                            Height='600' Width='200'
                            SelectionMode='None'
                            SelectedIndex='-1'>
                            <ListView.GroupStyle>
                              <GroupStyle>
                                <GroupStyle.HeaderContainerStyle>
                                  <Style TargetType='ListViewHeaderItem'>
                                    <Setter Property='IsTabStop' Value='False'/>
                                  </Style>
                                </GroupStyle.HeaderContainerStyle>
                                <GroupStyle.HeaderTemplate>
                                  <DataTemplate>
                                    <StackPanel>
                                      <TextBlock Text='{Binding GroupName}'/>
                                      <TextBlock Text='{Binding SubName}' />
                                    </StackPanel>
                                  </DataTemplate>
                                </GroupStyle.HeaderTemplate>
                              </GroupStyle>
                            </ListView.GroupStyle>
                        </ListView>");

                lv.ItemsSource = cvs.View;

                button = new Button() { Content = "Before" };

                var root = new StackPanel();
                root.Children.Add(button);
                root.Children.Add(lv);

                TestServices.WindowHelper.WindowContent = root;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                button.Focus(FocusState.Keyboard);
            });
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment($"Tab into the ListView.");
            TestServices.KeyboardHelper.Tab();
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                var itemContainer = lv.ContainerFromIndex(0);
                if (!Object.ReferenceEquals(itemContainer, FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot)))
                {
                    new Exception("Focus should be on the first item in the list.");
                }
            });

            // Start off at index 1 because we've already tabbed onto item #0.
            for (uint itemIndex = 1; itemIndex < numGroups * itemsPerGroup; ++itemIndex)
            {
                Log.Comment($"Arrow down through item #{itemIndex}.");
                TestServices.KeyboardHelper.Down();
                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    var itemContainer = lv.ContainerFromIndex((int)(itemIndex));
                    Verify.AreEqual(itemContainer, FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot));
                });
            }
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        public void ValidateArrowNavigationWithNonFocusableGroupsThatHaveFocusableChildren()
        {
            ListView lv = null;
            Button button = null;

            const uint numGroups = 3;
            const uint itemsPerGroup = 2;

            UIExecutor.Execute(() =>
            {
                var cvs = CreateCollectionViewSourceWithGrouping(numGroups, itemsPerGroup);

                // By default, ListViews with grouping will automatically select the first
                // item if selection mode is set to Single.  Set it to None so that we
                // can disable that code-path.
                lv = (ListView)XamlReader.Load(@"
                        <ListView xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                            Height='600' Width='200'
                            SelectionMode='None'
                            SelectedIndex='-1'>
                            <ListView.GroupStyle>
                              <GroupStyle>
                                <GroupStyle.HeaderContainerStyle>
                                  <Style TargetType='ListViewHeaderItem'>
                                    <Setter Property='IsTabStop' Value='False'/>
                                  </Style>
                                </GroupStyle.HeaderContainerStyle>
                                <GroupStyle.HeaderTemplate>
                                  <DataTemplate>
                                    <StackPanel>
                                      <TextBlock Text='{Binding GroupName}'/>
                                      <Button Content='{Binding SubName}' />
                                    </StackPanel>
                                  </DataTemplate>
                                </GroupStyle.HeaderTemplate>
                              </GroupStyle>
                            </ListView.GroupStyle>
                        </ListView>");

                lv.ItemsSource = cvs.View;

                button = new Button() { Content = "Before" };

                var root = new StackPanel();
                root.Children.Add(button);
                root.Children.Add(lv);

                TestServices.WindowHelper.WindowContent = root;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                button.Focus(FocusState.Keyboard);
            });
            TestServices.WindowHelper.WaitForIdle();

            for (uint groupIndex = 0; groupIndex < numGroups; ++groupIndex)
            {
                if (groupIndex == 0)
                {
                    Log.Comment($"Tab on the focusable child of group #{groupIndex}'s header.");
                    TestServices.KeyboardHelper.Tab();
                }
                else
                {
                    Log.Comment($"Arrow down onto the focusable child of group #{groupIndex}'s header.");
                    TestServices.KeyboardHelper.Down();
                }
                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    var itemContainer = lv.ContainerFromIndex((int)(groupIndex * itemsPerGroup));
                    var groupHeader = lv.GroupHeaderContainerFromItemContainer(itemContainer);
                    Verify.IsTrue(IsAncestorOf(groupHeader, (DependencyObject)FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot)));
                });

                Log.Comment($"Arrow down through group #{groupIndex}'s items.");
                for (uint itemIndex = 0; itemIndex < itemsPerGroup; ++itemIndex)
                {
                    TestServices.KeyboardHelper.Down();
                    TestServices.WindowHelper.WaitForIdle();

                    UIExecutor.Execute(() =>
                    {
                        var itemContainer = lv.ContainerFromIndex((int)(groupIndex * itemsPerGroup + itemIndex));
                        Verify.AreEqual(itemContainer, FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot));
                    });
                }
            }
        }

        [TestMethod]
        public void ValidateArrowNavigationWithVariableSizedWrapGrid()
        {
            GridView gv = null;
            Button button = null;

            const uint numGroups = 2;
            const uint itemsPerGroup = 4;

            UIExecutor.Execute(() =>
            {
                var cvs = CreateCollectionViewSourceWithGrouping(numGroups, itemsPerGroup);

                // By default, ListViews with grouping will automatically select the first
                // item if selection mode is set to Single.  Set it to None so that we
                // can disable that code-path.
                gv = (GridView)XamlReader.Load(@"
                        <GridView xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                            Height='600' Width='200'>
                            <GridView.GroupStyle>
                              <GroupStyle>
                                <GroupStyle.HeaderContainerStyle>
                                  <Style TargetType='ListViewHeaderItem'>
                                    <Setter Property='IsTabStop' Value='False'/>
                                  </Style>
                                </GroupStyle.HeaderContainerStyle>
                                <GroupStyle.HeaderTemplate>
                                  <DataTemplate>
                                    <StackPanel>
                                      <TextBlock Text='{Binding GroupName}'/>
                                    </StackPanel>
                                  </DataTemplate>
                                </GroupStyle.HeaderTemplate>
                                <GroupStyle.Panel>
                                  <ItemsPanelTemplate>
                                    <VariableSizedWrapGrid Orientation='Horizontal'/>
                                  </ItemsPanelTemplate>
                                </GroupStyle.Panel>
                              </GroupStyle>
                            </GridView.GroupStyle>
                            <GridView.ItemsPanel>
                              <ItemsPanelTemplate>
                                <VariableSizedWrapGrid Orientation='Horizontal'/>
                              </ItemsPanelTemplate>
                            </GridView.ItemsPanel>
                        </GridView>");

                gv.ItemsSource = cvs.View;

                button = new Button() { Content = "Before" };

                var root = new StackPanel();
                root.Children.Add(button);
                root.Children.Add(gv);

                TestServices.WindowHelper.WindowContent = root;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                button.Focus(FocusState.Keyboard);
            });
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment($"Tab onto the first item in the GridView.");
            TestServices.KeyboardHelper.Tab();
            TestServices.WindowHelper.WaitForIdle();

            for (uint groupIndex = 0; groupIndex < numGroups; ++groupIndex)
            {
                for (uint itemIndex = 0; itemIndex < itemsPerGroup; ++itemIndex)
                {
                    UIExecutor.Execute(() =>
                    {
                        var itemContainer = gv.ContainerFromIndex((int)(groupIndex * itemsPerGroup + itemIndex));
                        Verify.AreEqual(itemContainer, FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot));
                    });

                    if (itemIndex < (itemsPerGroup - 1))
                    {
                        Log.Comment($"Arrow right onto the next item.");
                        TestServices.KeyboardHelper.Right();
                        TestServices.WindowHelper.WaitForIdle();
                    }
                    else
                    {
                        // VariableSizedWrapGrid doesn't wrap when you reach the end, so press down
                        // to get to the next group when we're on the last item in the current
                        // group.
                        Log.Comment($"Arrow down into the next group.");
                        TestServices.KeyboardHelper.Down();
                        TestServices.WindowHelper.WaitForIdle();
                    }
                }
            }
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        public void TabEntryUsesSelectionBeforeAnyItemReceivesFocus()
        {
            VerifySingleSelectionTabEntry(reverse: false, useGridView: false);
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        public void ShiftTabEntryUsesSelectionBeforeAnyItemReceivesFocus()
        {
            VerifySingleSelectionTabEntry(reverse: true, useGridView: false);
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        public void GridViewSingleSelectionTabEntryBeforeAnyItemReceivesFocus()
        {
            VerifySingleSelectionTabEntry(reverse: false, useGridView: true);
            VerifySingleSelectionTabEntry(reverse: true,  useGridView: true);
        }

        private void VerifySingleSelectionTabEntry(bool reverse, bool useGridView)
        {
            Microsoft.UI.Xaml.Controls.ListViewBase lv = useGridView ? null : CreateListViewWithGrouping();
            const int selectedIndex = 2;
            Button beforeButton = null;
            Button afterButton = null;
            bool listReceivedFocus = false;

            UIExecutor.Execute(() =>
            {
                if (useGridView)
                {
                    lv = new GridView { Height = 600, Width = 200, ItemsSource = Enumerable.Range(0, 10).ToArray() };
                }
                lv.SelectionMode = ListViewSelectionMode.Single;
                lv.SingleSelectionFollowsFocus = false;
                lv.TabNavigation = KeyboardNavigationMode.Once;
                lv.GotFocus += (s, e) => listReceivedFocus = true;

                beforeButton = new Button { Content = "Before" };
                afterButton = new Button { Content = "After" };
                var root = new StackPanel();
                root.Children.Add(beforeButton);
                root.Children.Add(lv);
                root.Children.Add(afterButton);
                TestServices.WindowHelper.WindowContent = root;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsTrue((reverse ? afterButton : beforeButton).Focus(FocusState.Keyboard));
                Verify.IsNotNull(lv.ContainerFromIndex(selectedIndex));
                lv.SelectedIndex = selectedIndex;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsFalse(listReceivedFocus);
                Verify.AreEqual(selectedIndex, lv.SelectedIndex);
                Verify.AreEqual(reverse ? afterButton : beforeButton, FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot));
            });

            Log.Comment($"{(reverse ? "Shift+Tab" : "Tab")} into the {(useGridView ? "GridView" : "grouped ListView")} after setting selection without focusing an item.");
            if (reverse)
            {
                TestServices.KeyboardHelper.ShiftTab();
            }
            else
            {
                TestServices.KeyboardHelper.Tab();
            }
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(selectedIndex, lv.SelectedIndex);
                Verify.AreEqual(lv.ContainerFromIndex(selectedIndex), FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot));
            });

            if (!useGridView)
            {
                Log.Comment("A subsequently focused group header should take precedence over the existing selection.");
                Control groupHeader = null;
                UIExecutor.Execute(() =>
                {
                    groupHeader = (Control)lv.GroupHeaderContainerFromItemContainer(lv.ContainerFromIndex(selectedIndex));
                    Verify.IsNotNull(groupHeader);
                    Verify.IsTrue(groupHeader.Focus(FocusState.Keyboard));
                });
                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    Verify.AreEqual(groupHeader, FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot));
                    Verify.IsTrue((reverse ? afterButton : beforeButton).Focus(FocusState.Keyboard));
                });
                TestServices.WindowHelper.WaitForIdle();

                if (reverse)
                {
                    TestServices.KeyboardHelper.ShiftTab();
                }
                else
                {
                    TestServices.KeyboardHelper.Tab();
                }
                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    Verify.AreEqual(selectedIndex, lv.SelectedIndex);
                    Verify.IsTrue(IsAncestorOf(groupHeader, (DependencyObject)FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot)));
                });
            }
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        public void MultipleSelectionTabEntryBeforeAnyItemReceivesFocus()
        {
            VerifyMultiSelectionTabEntry(ListViewSelectionMode.Multiple, grouped: false, reverse: false);
            VerifyMultiSelectionTabEntry(ListViewSelectionMode.Multiple, grouped: false, reverse: true);
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        public void GroupedMultipleSelectionTabEntryBeforeAnyItemReceivesFocus()
        {
            VerifyMultiSelectionTabEntry(ListViewSelectionMode.Multiple, grouped: true, reverse: false);
            VerifyMultiSelectionTabEntry(ListViewSelectionMode.Multiple, grouped: true, reverse: true);
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        public void ExtendedSelectionTabEntryBeforeAnyItemReceivesFocus()
        {
            VerifyMultiSelectionTabEntry(ListViewSelectionMode.Extended, grouped: false, reverse: false);
            VerifyMultiSelectionTabEntry(ListViewSelectionMode.Extended, grouped: false, reverse: true);
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        public void GroupedExtendedSelectionTabEntryBeforeAnyItemReceivesFocus()
        {
            VerifyMultiSelectionTabEntry(ListViewSelectionMode.Extended, grouped: true, reverse: false);
            VerifyMultiSelectionTabEntry(ListViewSelectionMode.Extended, grouped: true, reverse: true);
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        public void GridViewMultipleSelectionTabEntryBeforeAnyItemReceivesFocus()
        {
            VerifyMultiSelectionTabEntry(ListViewSelectionMode.Multiple, grouped: false, reverse: false, useGridView: true);
            VerifyMultiSelectionTabEntry(ListViewSelectionMode.Multiple, grouped: false, reverse: true,  useGridView: true);
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        public void GridViewExtendedSelectionTabEntryBeforeAnyItemReceivesFocus()
        {
            VerifyMultiSelectionTabEntry(ListViewSelectionMode.Extended, grouped: false, reverse: false, useGridView: true);
            VerifyMultiSelectionTabEntry(ListViewSelectionMode.Extended, grouped: false, reverse: true,  useGridView: true);
        }

        private void VerifyMultiSelectionTabEntry(ListViewSelectionMode selectionMode, bool grouped, bool reverse, bool useGridView = false)
        {
            Microsoft.UI.Xaml.Controls.ListViewBase lv = (grouped && !useGridView) ? CreateListViewWithGrouping() : null;
            const int selectedIndex = 2;
            Button beforeButton = null;
            Button afterButton = null;
            bool listReceivedFocus = false;

            UIExecutor.Execute(() =>
            {
                if (!grouped)
                {
                    lv = useGridView ? new GridView() : new ListView();
                    lv.Height = 600;
                    lv.Width = 200;
                    lv.ItemsSource = Enumerable.Range(0, 10).ToArray();
                }
                lv.SelectionMode = selectionMode;
                lv.SingleSelectionFollowsFocus = false;
                lv.TabNavigation = KeyboardNavigationMode.Once;
                lv.GotFocus += (s, e) => listReceivedFocus = true;

                beforeButton = new Button { Content = "Before" };
                afterButton = new Button { Content = "After" };
                var root = new StackPanel();
                root.Children.Add(beforeButton);
                root.Children.Add(lv);
                root.Children.Add(afterButton);
                TestServices.WindowHelper.WindowContent = root;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsTrue((reverse ? afterButton : beforeButton).Focus(FocusState.Keyboard));
                Verify.IsNotNull(lv.ContainerFromIndex(selectedIndex));
                lv.SelectedIndex = selectedIndex;
                lv.SelectedItems.Add(lv.Items[selectedIndex + 1]);
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsFalse(listReceivedFocus);
                Verify.AreEqual(selectedIndex, lv.SelectedIndex);
                Verify.AreEqual(2, lv.SelectedItems.Count);
                Verify.AreEqual(reverse ? afterButton : beforeButton, FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot));
            });

            Log.Comment($"{(reverse ? "Shift+Tab" : "Tab")} into a {(grouped ? "grouped" : "ungrouped")} {(useGridView ? "GridView" : "ListView")} with {selectionMode} selection established before first focus.");
            if (reverse)
            {
                TestServices.KeyboardHelper.ShiftTab();
            }
            else
            {
                TestServices.KeyboardHelper.Tab();
            }
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot);
                Log.Comment($"Focused element: {focusedElement?.GetType().Name}; content: {(focusedElement as ContentControl)?.Content}.");
                Verify.AreEqual(selectedIndex, lv.SelectedIndex);
                Verify.AreEqual(2, lv.SelectedItems.Count);
                Verify.IsTrue(lv.SelectedItems.Contains(lv.Items[selectedIndex]));
                Verify.IsTrue(lv.SelectedItems.Contains(lv.Items[selectedIndex + 1]));
                Verify.AreEqual(lv.ContainerFromIndex(selectedIndex), focusedElement);
            });

            if (grouped)
            {
                Log.Comment("A subsequently focused group header should take precedence over the existing multiple selection.");
                Control groupHeader = null;
                UIExecutor.Execute(() =>
                {
                    groupHeader = (Control)lv.GroupHeaderContainerFromItemContainer(lv.ContainerFromIndex(selectedIndex));
                    Verify.IsNotNull(groupHeader);
                    Verify.IsTrue(groupHeader.Focus(FocusState.Keyboard));
                });
                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    Verify.AreEqual(groupHeader, FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot));
                    Verify.IsTrue((reverse ? afterButton : beforeButton).Focus(FocusState.Keyboard));
                });
                TestServices.WindowHelper.WaitForIdle();

                if (reverse)
                {
                    TestServices.KeyboardHelper.ShiftTab();
                }
                else
                {
                    TestServices.KeyboardHelper.Tab();
                }
                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    Verify.AreEqual(selectedIndex, lv.SelectedIndex);
                    Verify.AreEqual(2, lv.SelectedItems.Count);
                    Verify.IsTrue(lv.SelectedItems.Contains(lv.Items[selectedIndex]));
                    Verify.IsTrue(lv.SelectedItems.Contains(lv.Items[selectedIndex + 1]));
                    Verify.IsTrue(IsAncestorOf(groupHeader, (DependencyObject)FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot)));
                });
            }
        }

        private ListView CreateListViewWithGrouping()
        {
            ListView lv = null;
            UIExecutor.Execute(() =>
            {
                lv = (ListView)XamlReader.Load(@"
                        <ListView xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' Height='600' Width='200'>
                            <ListView.ItemsPanel>
                                <ItemsPanelTemplate>
                                    <ItemsStackPanel CacheLength='0'/>
                                </ItemsPanelTemplate>
                            </ListView.ItemsPanel>
                            <ListView.GroupStyle>
                              <GroupStyle>
                                <GroupStyle.HeaderTemplate>
                                  <DataTemplate>
                                    <StackPanel>
                                      <Button Content='{Binding GroupName}'/>
                                      <Button Content='{Binding SubName}' />
                                    </StackPanel>
                                  </DataTemplate>
                                </GroupStyle.HeaderTemplate>
                              </GroupStyle>
                            </ListView.GroupStyle>
                        </ListView>");

                lv.ItemsSource = CreateCollectionViewSourceWithGrouping().View;
            });

            return lv;
        }

        private CollectionViewSource CreateCollectionViewSourceWithGrouping(uint numGroups = 10, uint itemsPerGroup = 10)
        {
            CollectionViewSource cvs = new CollectionViewSource();
            cvs.IsSourceGrouped = true;

            var groups = new ObservableCollection<MyGroup>();
            for (int i = 0; i < numGroups; i++)
            {
                var group = new MyGroup();
                group.GroupName = string.Format("Group#{0}", i);
                group.SubName = i.ToString();
                for (int j = 0; j < itemsPerGroup; j++)
                {
                    group.Add((int)(i * numGroups + j));
                }
                groups.Add(group);
            }

            cvs.Source = groups;

            return cvs;
        }

    }
}
