// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using MUXControlsTestApp.Utilities;

using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Common;
using Microsoft.UI.Xaml.Markup;
using System.Collections.Generic;
using Microsoft.UI.Xaml.Media;
using System.Linq;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public partial class RadioButtonsTests : ApiTestBase
    {
        // Scenario: two RadioButtons bound to {"Option 1", "Option 2"}; one ItemTemplate is a plain TextBlock, the other's
        //     template root is already a RadioButton with Foreground=Blue.
        // Expected: ContainerFromIndex(0) is a RadioButton in both; the TextBlock one is wrapped in a default (non-blue)
        //     RadioButton, while the RadioButton template is used as the container itself (blue foreground kept).
        // Failure means: RadioButtonsElementFactory::GetElementCore no longer wraps non-RadioButton template content in a
        //     RadioButton, or wraps/replaces a template whose root is already a RadioButton.
        [TestMethod]
        public void VerifyCustomItemTemplate()
        {
            RadioButtons radioButtons = null;
            RadioButtons radioButtons2 = null;
            RunOnUIThread.Execute(() =>
            {
                radioButtons = new RadioButtons();
                radioButtons.ItemsSource = new List<string>() { "Option 1", "Option 2" };

                // Set a custom ItemTemplate to be wrapped in a RadioButton.
                var itemTemplate = (DataTemplate)XamlReader.Load(
                        @"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                            <TextBlock Text = '{Binding}'/>
                        </DataTemplate>");

                radioButtons.ItemTemplate = itemTemplate;

                radioButtons2 = new RadioButtons();
                radioButtons2.ItemsSource = new List<string>() { "Option 1", "Option 2" };

                // Set a custom ItemTemplate which is already a RadioButton. No wrapping should be performed.
                var itemTemplate2 = (DataTemplate)XamlReader.Load(
                        @"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                            <RadioButton Foreground='Blue'>
                              <TextBlock Text = '{Binding}'/>
                            </RadioButton>
                        </DataTemplate>");

                radioButtons2.ItemTemplate = itemTemplate2;

                var stackPanel = new StackPanel();
                stackPanel.Children.Add(radioButtons);
                stackPanel.Children.Add(radioButtons2);

                Content = stackPanel;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var radioButton1 = radioButtons.ContainerFromIndex(0) as RadioButton;
                var radioButton2 = radioButtons2.ContainerFromIndex(0) as RadioButton;
                Verify.IsNotNull(radioButton1, "Our custom ItemTemplate should have been wrapped in a RadioButton.");
                Verify.IsNotNull(radioButton2, "Our custom ItemTemplate should have been wrapped in a RadioButton.");

                bool testCondition = !(radioButton1.Foreground is SolidColorBrush brush && brush.Color == Colors.Blue);
                Verify.IsTrue(testCondition, "Default foreground color of the RadioButton should not have been [blue].");

                testCondition = radioButton2.Foreground is SolidColorBrush brush2 && brush2.Color == Colors.Blue;
                Verify.IsTrue(testCondition, "The foreground color of the RadioButton should have been [blue].");
            });
        }

        // Scenario: a loaded RadioButtons with IsEnabled toggled true -> false -> true, reading the template root's
        //     CommonStates visual state group after each change.
        // Expected: CommonStates.CurrentState is "Normal", then "Disabled", then "Normal" again.
        // Failure means: RadioButtons::OnIsEnabledChanged/UpdateVisualStateForIsEnabledChange (hooked up in OnApplyTemplate)
        //     no longer drive the Normal/Disabled states, so a disabled RadioButtons doesn't look disabled.
        [TestMethod]
        public void VerifyIsEnabledChangeUpdatesVisualState()
        {
            RadioButtons radioButtons = null; ;
            VisualStateGroup commonStatesGroup = null;
            RunOnUIThread.Execute(() =>
            {
                radioButtons = new RadioButtons();

                // Check 1: Set IsEnabled to true.
                radioButtons.IsEnabled = true;

                Content = radioButtons;
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var radioButtonsLayoutRoot = (FrameworkElement)VisualTreeHelper.GetChild(radioButtons, 0);
                commonStatesGroup = VisualStateManager.GetVisualStateGroups(radioButtonsLayoutRoot).First(vsg => vsg.Name.Equals("CommonStates"));

                Verify.AreEqual("Normal", commonStatesGroup.CurrentState.Name);

                // Check 2: Set IsEnabled to false.
                radioButtons.IsEnabled = false;
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("Disabled", commonStatesGroup.CurrentState.Name);

                // Check 3: Set IsEnabled back to true.
                radioButtons.IsEnabled = true;
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("Normal", commonStatesGroup.CurrentState.Name);
            });
        }

        // Scenario: RadioButtons with a custom template (ScrollViewer Height=120 + virtualizing StackLayout ItemsRepeater),
        //     100 string items, SelectedIndex=0; scroll to the end and back once for a baseline, then 5 more round trips.
        // Expected: fewer than 100 children are realized, ElementClearing fires, the child count after the extra scrolling
        //     equals the baseline, item 0 is RadioButton "Option 0", and GetRows/GetColumns/GetLargerColumns return -1.
        // Failure means: RadioButtonsElementFactory::RecycleElementCore/GetElementCore stopped reusing pooled wrapper
        //     RadioButtons (containers leak on every scroll), or RadioButtons::GetLayout mishandles a non-built-in layout.
        [TestMethod]
        public void VerifyVirtualizingTemplateDoesNotRetainRecycledContainers()
        {
            RadioButtons radioButtons = null;
            ScrollViewer scrollViewer = null;
            ItemsRepeater repeater = null;
            int clearingCount = 0;

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Creating virtualizing RadioButtons template.");
                var virtualizingTemplate = (ControlTemplate)XamlReader.Load(TestUtilities.ProcessTestXamlForRepo(
                    @"<ControlTemplate
                        xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                        xmlns:controls='using:Microsoft.UI.Xaml.Controls'
                        TargetType='controls:RadioButtons'>
                        <ScrollViewer Height='120'>
                            <controls:ItemsRepeater x:Name='InnerRepeater'>
                                <controls:ItemsRepeater.Layout>
                                    <controls:StackLayout />
                                </controls:ItemsRepeater.Layout>
                            </controls:ItemsRepeater>
                        </ScrollViewer>
                    </ControlTemplate>"));

                Log.Comment("Creating RadioButtons instance.");
                radioButtons = new RadioButtons
                {
                    ItemsSource = Enumerable.Range(0, 100).Select(index => $"Option {index}").ToList(),
                    SelectedIndex = 0,
                    Template = virtualizingTemplate
                };

                Log.Comment("Applying RadioButtons template.");
                Content = radioButtons;
                Content.UpdateLayout();

                Log.Comment("Finding virtualizing template descendants.");
                scrollViewer = radioButtons.FindVisualChildByType<ScrollViewer>();
                repeater = radioButtons.FindVisualChildByType<ItemsRepeater>();
                Verify.IsNotNull(scrollViewer);
                Verify.IsNotNull(repeater);

                repeater.ElementClearing += (sender, args) => clearingCount++;
            });

            IdleSynchronizer.Wait();

            void Scroll(bool toEnd)
            {
                RunOnUIThread.Execute(() =>
                {
                    scrollViewer.ChangeView(null, toEnd ? scrollViewer.ScrollableHeight : 0, null, true);
                    Content.UpdateLayout();
                });
                IdleSynchronizer.Wait();
            }

            Scroll(true);
            Scroll(false);

            int baselineChildCount = 0;
            RunOnUIThread.Execute(() =>
            {
                baselineChildCount = VisualTreeHelper.GetChildrenCount(repeater);
                Verify.IsLessThan(baselineChildCount, 100);
            });

            for (int iteration = 0; iteration < 5; iteration++)
            {
                Scroll(true);
                Scroll(false);
            }

            RunOnUIThread.Execute(() =>
            {
                int finalChildCount = VisualTreeHelper.GetChildrenCount(repeater);
                Verify.IsGreaterThan(clearingCount, 0);
                Verify.AreEqual(baselineChildCount, finalChildCount);

                var selectedContainer = radioButtons.ContainerFromIndex(0) as RadioButton;
                Verify.IsNotNull(selectedContainer);
                Verify.AreEqual("Option 0", selectedContainer.Content);

                // The test hooks only report layout data for the built-in ColumnMajorUniformToLargestGridLayout.
                Microsoft.UI.Private.Controls.RadioButtonsTestHooks.SetTestHooksEnabled(radioButtons, true);
                try
                {
                    Verify.AreEqual(-1, Microsoft.UI.Private.Controls.RadioButtonsTestHooks.GetRows(radioButtons));
                    Verify.AreEqual(-1, Microsoft.UI.Private.Controls.RadioButtonsTestHooks.GetColumns(radioButtons));
                    Verify.AreEqual(-1, Microsoft.UI.Private.Controls.RadioButtonsTestHooks.GetLargerColumns(radioButtons));
                }
                finally
                {
                    Microsoft.UI.Private.Controls.RadioButtonsTestHooks.SetTestHooksEnabled(radioButtons, false);
                }
            });
        }

        // Scenario: RadioButtons with 4 string items; SelectedIndex set to 0, then 2, then out-of-range 99, then 0 -> -1,
        //     capturing every SelectionChanged event.
        // Expected: the first selection reports 0 removed / 1 added items; deselecting reports no added items; no
        //     AddedItems or RemovedItems collection ever contains a null entry.
        // Failure means: RadioButtons::Select again appends the null that GetDataAtIndex returns for -1/out-of-range
        //     indices to the SelectionChangedEventArgs, so handlers see null items.
        [TestMethod]
        public void VerifySelectionChangedArgsDoNotContainNullItems()
        {
            RadioButtons radioButtons = null;
            var selectionChangedArgs = new List<SelectionChangedEventArgs>();

            RunOnUIThread.Execute(() =>
            {
                radioButtons = new RadioButtons();
                radioButtons.ItemsSource = new List<string>() { "0", "1", "2", "3" };
                radioButtons.SelectionChanged += (s, e) => selectionChangedArgs.Add(e);

                Content = radioButtons;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            // Local helper: verify every captured event so far has no null entries
            // in either collection, then clear the buffer for the next scenario.
            // The args collections are UI-thread objects, so access them on the UI thread.
            void VerifyNoNullItemsAndClear()
            {
                RunOnUIThread.Execute(() =>
                {
                    foreach (var args in selectionChangedArgs)
                    {
                        foreach (var item in args.AddedItems)
                        {
                            Verify.IsNotNull(item, "AddedItems should never contain a null entry.");
                        }
                        foreach (var item in args.RemovedItems)
                        {
                            Verify.IsNotNull(item, "RemovedItems should never contain a null entry.");
                        }
                    }
                });
                selectionChangedArgs.Clear();
            }

            // Scenario 1: first selection (nothing was selected before). RemovedItems
            // must be empty (Count == 0), AddedItems must contain the newly selected item.
            RunOnUIThread.Execute(() =>
            {
                radioButtons.SelectedIndex = 0;
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsGreaterThan(selectionChangedArgs.Count, 0, "SelectionChanged should have fired for the first selection.");
                var lastArgs = selectionChangedArgs.Last();
                Verify.AreEqual(0, lastArgs.RemovedItems.Count, "First selection should report no removed items.");
                Verify.AreEqual(1, lastArgs.AddedItems.Count, "First selection should report exactly one added item.");
            });
            VerifyNoNullItemsAndClear();

            // Scenario 2: switching selection to another item. No event should carry a null.
            RunOnUIThread.Execute(() =>
            {
                radioButtons.SelectedIndex = 2;
            });
            IdleSynchronizer.Wait();
            VerifyNoNullItemsAndClear();

            // Scenario 3: out-of-range positive SelectedIndex. GetDataAtIndex returns
            // null for such an index, but the args must not carry a null entry.
            RunOnUIThread.Execute(() =>
            {
                radioButtons.SelectedIndex = 99;
            });
            IdleSynchronizer.Wait();
            VerifyNoNullItemsAndClear();

            // Scenario 4: deselect everything. AddedItems must be empty and no null entries.
            RunOnUIThread.Execute(() =>
            {
                radioButtons.SelectedIndex = 0;
            });
            IdleSynchronizer.Wait();
            selectionChangedArgs.Clear();

            RunOnUIThread.Execute(() =>
            {
                radioButtons.SelectedIndex = -1;
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                foreach (var args in selectionChangedArgs)
                {
                    Verify.AreEqual(0, args.AddedItems.Count, "Deselecting should report no added items.");
                }
            });
            VerifyNoNullItemsAndClear();
        }

        private RadioButtons CreateLoadedRadioButtons(int itemCount, int maxColumns, double width = 1000)
        {
            RadioButtons radioButtons = null;
            RunOnUIThread.Execute(() =>
            {
                radioButtons = new RadioButtons();
                radioButtons.MaxColumns = maxColumns;
                radioButtons.Width = width;
                for (int i = 0; i < itemCount; i++)
                {
                    radioButtons.Items.Add("Option " + i);
                }

                Content = radioButtons;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();
            return radioButtons;
        }

        // Scenario: 7 string items at MaxColumns=1 (Width=1000); enable RadioButtonsTestHooks, then set MaxColumns=3 and
        //     later MaxColumns=2.
        // Expected: MaxColumns=3 gives 3 columns, 2 minimum rows, 1 larger column and raises LayoutChanged; MaxColumns=2
        //     gives 2 columns, 3 minimum rows, 1 larger column.
        // Failure means: ColumnMajorUniformToLargestGridLayout::CalculateColumns/ArrangeOverride distribute items across
        //     columns incorrectly, or the test-hook plumbing (SetTestHooksEnabled/LayoutChanged) stopped reporting.
        [TestMethod]
        public void VerifyMultiColumnLayoutTestHooks()
        {
            var radioButtons = CreateLoadedRadioButtons(itemCount: 7, maxColumns: 1);

            int layoutChangedCount = 0;
            global::Windows.Foundation.TypedEventHandler<RadioButtons, object> handler = (s, a) => layoutChangedCount++;

            RunOnUIThread.Execute(() =>
            {
                Microsoft.UI.Private.Controls.RadioButtonsTestHooks.SetTestHooksEnabled(radioButtons, true);
                Microsoft.UI.Private.Controls.RadioButtonsTestHooks.LayoutChanged += handler;
                radioButtons.MaxColumns = 3;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            try
            {
                RunOnUIThread.Execute(() =>
                {
                    // 7 items in 3 columns: column sizes 3, 2, 2 -> one "larger" column, 2 minimum rows.
                    Verify.AreEqual(3, Microsoft.UI.Private.Controls.RadioButtonsTestHooks.GetColumns(radioButtons));
                    Verify.AreEqual(2, Microsoft.UI.Private.Controls.RadioButtonsTestHooks.GetRows(radioButtons));
                    Verify.AreEqual(1, Microsoft.UI.Private.Controls.RadioButtonsTestHooks.GetLargerColumns(radioButtons));
                    Verify.IsGreaterThan(layoutChangedCount, 0, "LayoutChanged should have been raised.");

                    radioButtons.MaxColumns = 2;
                    Content.UpdateLayout();
                });
                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    // 7 items in 2 columns: sizes 4, 3 -> one larger column, 3 minimum rows.
                    Verify.AreEqual(2, Microsoft.UI.Private.Controls.RadioButtonsTestHooks.GetColumns(radioButtons));
                    Verify.AreEqual(3, Microsoft.UI.Private.Controls.RadioButtonsTestHooks.GetRows(radioButtons));
                    Verify.AreEqual(1, Microsoft.UI.Private.Controls.RadioButtonsTestHooks.GetLargerColumns(radioButtons));
                });
            }
            finally
            {
                RunOnUIThread.Execute(() =>
                {
                    Microsoft.UI.Private.Controls.RadioButtonsTestHooks.LayoutChanged -= handler;
                    Microsoft.UI.Private.Controls.RadioButtonsTestHooks.SetTestHooksEnabled(radioButtons, false);
                });
            }
        }

        // Scenario: 4 items with MaxColumns=4 at Width=2000, then Width shrunk to 30 (narrower than a single item).
        // Expected: the test hooks report 1 column, 4 rows and 0 larger columns.
        // Failure means: ColumnMajorUniformToLargestGridLayout::CalculateColumns no longer clamps MaxColumns to the
        //     available width (or returns fewer than 1 column), so items overflow or vanish in narrow layouts.
        [TestMethod]
        public void VerifyMaxColumnsIsClampedByAvailableWidth()
        {
            // Wide enough for all requested columns first, then narrow to force a single column.
            var radioButtons = CreateLoadedRadioButtons(itemCount: 4, maxColumns: 4, width: 2000);

            RunOnUIThread.Execute(() =>
            {
                Microsoft.UI.Private.Controls.RadioButtonsTestHooks.SetTestHooksEnabled(radioButtons, true);
                radioButtons.Width = 30; // Narrower than a single item: layout must still report one column.
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                try
                {
                    Verify.AreEqual(1, Microsoft.UI.Private.Controls.RadioButtonsTestHooks.GetColumns(radioButtons));
                    Verify.AreEqual(4, Microsoft.UI.Private.Controls.RadioButtonsTestHooks.GetRows(radioButtons));
                    Verify.AreEqual(0, Microsoft.UI.Private.Controls.RadioButtonsTestHooks.GetLargerColumns(radioButtons));
                }
                finally
                {
                    Microsoft.UI.Private.Controls.RadioButtonsTestHooks.SetTestHooksEnabled(radioButtons, false);
                }
            });
        }

        // Scenario: ObservableCollection {A, B, C, D} with SelectedIndex=1; remove D and A, then add E and insert F at 0,
        //     then replace ItemsSource with {X, Y}.
        // Expected: after the removals B stays selected at index 0; after the inserts B is at index 1 and container 0 shows
        //     "F"; after the source swap SelectedIndex is -1 and SelectedItem is null.
        // Failure means: RadioButtons::OnRepeaterElementIndexChanged no longer moves selection with the checked item,
        //     recycled RadioButtons keep stale content, or UpdateItemsSource doesn't clear selection on a new source.
        [TestMethod]
        public void VerifyItemsSourceChangesRecycleAndClearElements()
        {
            RadioButtons radioButtons = null;
            var items = new global::System.Collections.ObjectModel.ObservableCollection<string>() { "A", "B", "C", "D" };

            RunOnUIThread.Execute(() =>
            {
                radioButtons = new RadioButtons();
                radioButtons.ItemsSource = items;
                radioButtons.SelectedIndex = 1;
                Content = radioButtons;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                // Removing items clears/recycles their wrapping RadioButtons.
                items.RemoveAt(3);
                items.RemoveAt(0);
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("B", radioButtons.SelectedItem);
                Verify.AreEqual(0, radioButtons.SelectedIndex, "Selection index should track the removal before it.");

                // Re-adding items should reuse recycled radio buttons and show the right content.
                items.Add("E");
                items.Insert(0, "F");
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("B", radioButtons.SelectedItem);
                Verify.AreEqual(1, radioButtons.SelectedIndex);
                var first = radioButtons.ContainerFromIndex(0) as RadioButton;
                Verify.IsNotNull(first);
                Verify.AreEqual("F", first.Content);

                // Replace the entire source: previously realized elements are all cleared.
                radioButtons.ItemsSource = new List<string>() { "X", "Y" };
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(-1, radioButtons.SelectedIndex);
                Verify.IsNull(radioButtons.SelectedItem);
            });
        }

        // Scenario: a loaded RadioButtons with a string header, then a header template and an element header; its automation
        //           peer with and without AutomationProperties.Name; MaxColumns; removing a SelectionChanged handler; and the
        //           dependency property identifiers of RadioButtons and ColumnMajorUniformToLargestGridLayout.
        // Expected: the header and template reach the header presenter; the peer is a Group named after the header unless a
        //           name is set; a removed handler is not called; every identifier is distinct and agrees with its CLR property.
        // Failure means: Header/HeaderTemplate templating, RadioButtonsAutomationPeer::GetNameCore or DP registration broke.
        [TestMethod]
        public void VerifyHeaderPropertiesAndAutomationPeer()
        {
            RadioButtons radioButtons = null;
            ContentPresenter headerPresenter = null;
            DataTemplate headerTemplate = null;
            TextBlock elementHeader = null;

            RunOnUIThread.Execute(() =>
            {
                radioButtons = new RadioButtons
                {
                    Header = "Pick a color",
                    ItemsSource = new List<string>() { "Red", "Green", "Blue" }
                };
                Content = radioButtons;
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                headerPresenter = (ContentPresenter)VisualTreeUtils.FindVisualChildByName(radioButtons, "HeaderContentPresenter");
                Verify.IsNotNull(headerPresenter);
                Verify.AreEqual("Pick a color", radioButtons.Header);
                Verify.AreEqual("Pick a color", headerPresenter.Content);
                Verify.IsNull(radioButtons.HeaderTemplate);

                var peer = Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer.CreatePeerForElement(radioButtons);
                Verify.IsTrue(peer is Microsoft.UI.Xaml.Automation.Peers.RadioButtonsAutomationPeer);
                Verify.AreEqual(Microsoft.UI.Xaml.Automation.Peers.AutomationControlType.Group, peer.GetAutomationControlType());
                Verify.AreEqual("Microsoft.UI.Xaml.Controls.RadioButtons", peer.GetClassName());
                Verify.AreEqual("Pick a color", peer.GetName(), "Without AutomationProperties.Name the header names the group.");

                Microsoft.UI.Xaml.Automation.AutomationProperties.SetName(radioButtons, "Explicit name");
                Verify.AreEqual("Explicit name", peer.GetName());
                radioButtons.ClearValue(Microsoft.UI.Xaml.Automation.AutomationProperties.NameProperty);
                Verify.AreEqual("Pick a color", peer.GetName());

                headerTemplate = (DataTemplate)XamlReader.Load(
                    @"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                        <TextBlock Text='{Binding}' FontStyle='Italic' />
                    </DataTemplate>");
                radioButtons.HeaderTemplate = headerTemplate;

                elementHeader = new TextBlock() { Text = "Element header" };
                radioButtons.Header = elementHeader;

                Verify.AreEqual(1, radioButtons.MaxColumns, "MaxColumns defaults to 1.");
                radioButtons.MaxColumns = 2;
                Verify.AreEqual(2, radioButtons.MaxColumns);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(ReferenceEquals(headerTemplate, radioButtons.HeaderTemplate));
                Verify.IsTrue(ReferenceEquals(headerTemplate, headerPresenter.ContentTemplate));
                Verify.IsTrue(ReferenceEquals(elementHeader, radioButtons.Header));
                Verify.IsTrue(ReferenceEquals(elementHeader, headerPresenter.Content));

                int selectionChangedCount = 0;
                SelectionChangedEventHandler handler = (sender, args) => selectionChangedCount++;
                radioButtons.SelectionChanged += handler;
                radioButtons.SelectedIndex = 0;
                Verify.AreEqual(1, selectionChangedCount);

                radioButtons.SelectionChanged -= handler;
                radioButtons.SelectedIndex = 1;
                Verify.AreEqual(1, radioButtons.SelectedIndex);
                Verify.AreEqual(1, selectionChangedCount, "A removed SelectionChanged handler should not be called.");

                VerifyDependencyPropertyIdentifiers(
                    RadioButtons.ItemsSourceProperty,
                    RadioButtons.ItemsProperty,
                    RadioButtons.ItemTemplateProperty,
                    RadioButtons.SelectedIndexProperty,
                    RadioButtons.SelectedItemProperty,
                    RadioButtons.MaxColumnsProperty,
                    RadioButtons.HeaderProperty,
                    RadioButtons.HeaderTemplateProperty,
                    ColumnMajorUniformToLargestGridLayout.MaxColumnsProperty,
                    ColumnMajorUniformToLargestGridLayout.ColumnSpacingProperty,
                    ColumnMajorUniformToLargestGridLayout.RowSpacingProperty);
                Verify.IsTrue(ReferenceEquals(RadioButtons.HeaderProperty, RadioButtons.HeaderProperty));

                var fresh = new RadioButtons();
                object itemsSource = new List<string>() { "A" };
                VerifyDependencyPropertyRoundTrip(fresh, RadioButtons.ItemsSourceProperty, () => fresh.ItemsSource, v => fresh.ItemsSource = v, null, itemsSource);
                VerifyDependencyPropertyRoundTrip(fresh, RadioButtons.ItemTemplateProperty, () => fresh.ItemTemplate, v => fresh.ItemTemplate = v, null, (object)headerTemplate);
                VerifyDependencyPropertyRoundTrip(fresh, RadioButtons.SelectedIndexProperty, () => fresh.SelectedIndex, v => fresh.SelectedIndex = v, -1, 2);
                VerifyDependencyPropertyRoundTrip(fresh, RadioButtons.SelectedItemProperty, () => fresh.SelectedItem, v => fresh.SelectedItem = v, null, (object)"B");
                VerifyDependencyPropertyRoundTrip(fresh, RadioButtons.MaxColumnsProperty, () => fresh.MaxColumns, v => fresh.MaxColumns = v, 1, 3);
                VerifyDependencyPropertyRoundTrip(fresh, RadioButtons.HeaderProperty, () => fresh.Header, v => fresh.Header = v, null, (object)"Header");
                VerifyDependencyPropertyRoundTrip(fresh, RadioButtons.HeaderTemplateProperty, () => fresh.HeaderTemplate, v => fresh.HeaderTemplate = v, null, headerTemplate);
                fresh.Items.Add("Item");
                Verify.AreEqual(1, ((IList<object>)fresh.GetValue(RadioButtons.ItemsProperty)).Count, "The Items identifier returns the Items collection.");

                var layout = new ColumnMajorUniformToLargestGridLayout();
                VerifyDependencyPropertyRoundTrip(layout, ColumnMajorUniformToLargestGridLayout.ColumnSpacingProperty, () => layout.ColumnSpacing, v => layout.ColumnSpacing = v, 0.0, 6.0);
                VerifyDependencyPropertyRoundTrip(layout, ColumnMajorUniformToLargestGridLayout.RowSpacingProperty, () => layout.RowSpacing, v => layout.RowSpacing = v, 0.0, 7.0);
                // MaxColumns rejects values below 1, so it is checked without ClearValue (see VerifyColumnMajorLayoutStandalone).
                layout.SetValue(ColumnMajorUniformToLargestGridLayout.MaxColumnsProperty, 4);
                Verify.AreEqual(4, layout.MaxColumns);
                layout.MaxColumns = 5;
                Verify.AreEqual(5, (int)layout.GetValue(ColumnMajorUniformToLargestGridLayout.MaxColumnsProperty));
            });
        }

        // Scenario: SelectedItem and test hooks set before a RadioButtons is loaded; SelectedItem changes after load
        //           (in and not in the list); a checked RadioButton added to a loaded RadioButtons; null test-hook arguments.
        // Expected: pre-load SelectedItem selects its index on load; hooks enabled before load report layout changes;
        //           post-load SelectedItem selects by item, an unknown item clears the selection; an added checked
        //           RadioButton becomes the selection; null arguments return -1.
        // Failure means: RadioButtons::OnRepeaterLoaded, UpdateSelectedItem, OnRepeaterElementPrepared or test hooks broke.
        [TestMethod]
        public void VerifySelectionAppliedBeforeAndAfterLoad()
        {
            RadioButtons selectedItemBeforeLoad = null;
            RadioButtons hooksBeforeLoad = null;
            RadioButtons checkedItemAdded = null;
            RadioButtons checkedItemBeforeLoad = null;
            RadioButton addedCheckedItem = null;
            var selectionChangedArgs = new List<SelectionChangedEventArgs>();
            int layoutChangedCount = 0;
            global::Windows.Foundation.TypedEventHandler<RadioButtons, object> layoutChangedHandler = (sender, args) =>
            {
                if (sender == hooksBeforeLoad)
                {
                    layoutChangedCount++;
                }
            };

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(-1, Microsoft.UI.Private.Controls.RadioButtonsTestHooks.GetRows(null));
                Verify.AreEqual(-1, Microsoft.UI.Private.Controls.RadioButtonsTestHooks.GetColumns(null));
                Verify.AreEqual(-1, Microsoft.UI.Private.Controls.RadioButtonsTestHooks.GetLargerColumns(null));

                selectedItemBeforeLoad = new RadioButtons
                {
                    ItemsSource = new List<string>() { "A", "B", "C", "D" },
                    SelectedItem = "C"
                };
                Verify.AreEqual(-1, selectedItemBeforeLoad.SelectedIndex, "Selection by item is resolved once the items are realized.");

                hooksBeforeLoad = new RadioButtons() { MaxColumns = 1 };
                for (int i = 0; i < 4; i++)
                {
                    hooksBeforeLoad.Items.Add("Option " + i);
                }
                Microsoft.UI.Private.Controls.RadioButtonsTestHooks.SetTestHooksEnabled(hooksBeforeLoad, true);
                Microsoft.UI.Private.Controls.RadioButtonsTestHooks.LayoutChanged += layoutChangedHandler;

                checkedItemAdded = new RadioButtons();
                checkedItemAdded.Items.Add(new RadioButton() { Content = "One" });
                checkedItemAdded.Items.Add(new RadioButton() { Content = "Two" });
                checkedItemAdded.SelectionChanged += (sender, args) => selectionChangedArgs.Add(args);

                checkedItemBeforeLoad = new RadioButtons();
                checkedItemBeforeLoad.Items.Add(new RadioButton() { Content = "One" });
                checkedItemBeforeLoad.Items.Add(new RadioButton() { Content = "Two", IsChecked = true });

                var panel = new StackPanel();
                panel.Children.Add(selectedItemBeforeLoad);
                panel.Children.Add(hooksBeforeLoad);
                panel.Children.Add(checkedItemAdded);
                panel.Children.Add(checkedItemBeforeLoad);
                Content = panel;
            });
            IdleSynchronizer.Wait();

            try
            {
                RunOnUIThread.Execute(() =>
                {
                    Verify.AreEqual(2, selectedItemBeforeLoad.SelectedIndex, "SelectedItem set before load should select its index.");
                    Verify.AreEqual("C", selectedItemBeforeLoad.SelectedItem);
                    Verify.IsTrue(((RadioButton)selectedItemBeforeLoad.ContainerFromIndex(2)).IsChecked == true);

                    selectedItemBeforeLoad.SelectedItem = "A";
                    Verify.AreEqual(0, selectedItemBeforeLoad.SelectedIndex);
                    Verify.IsTrue(((RadioButton)selectedItemBeforeLoad.ContainerFromIndex(0)).IsChecked == true);
                    Verify.IsTrue(((RadioButton)selectedItemBeforeLoad.ContainerFromIndex(2)).IsChecked == false);

                    selectedItemBeforeLoad.SelectedItem = "Z";
                    Verify.AreEqual(-1, selectedItemBeforeLoad.SelectedIndex, "An item that isn't in the list selects nothing.");
                    Verify.IsNull(selectedItemBeforeLoad.SelectedItem);
                    Verify.IsTrue(((RadioButton)selectedItemBeforeLoad.ContainerFromIndex(0)).IsChecked == false);

                    Log.Comment("RadioButtons with a RadioButton checked before load: SelectedIndex after load is {0}.", checkedItemBeforeLoad.SelectedIndex);

                    Verify.AreEqual(-1, checkedItemAdded.SelectedIndex);
                    addedCheckedItem = new RadioButton() { Content = "Three", IsChecked = true };
                    checkedItemAdded.Items.Add(addedCheckedItem);

                    // Hooks were enabled before load and never re-enabled; a layout change must still be reported.
                    hooksBeforeLoad.MaxColumns = 2;
                });
                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    Verify.AreEqual(2, checkedItemAdded.SelectedIndex, "A checked RadioButton added to the items becomes the selection.");
                    Verify.IsTrue(ReferenceEquals(addedCheckedItem, checkedItemAdded.SelectedItem));
                    Verify.AreEqual(1, selectionChangedArgs.Count);
                    Verify.AreEqual(1, selectionChangedArgs[0].AddedItems.Count);
                    Verify.IsTrue(ReferenceEquals(addedCheckedItem, selectionChangedArgs[0].AddedItems[0]));
                    Verify.AreEqual(0, selectionChangedArgs[0].RemovedItems.Count);

                    Verify.IsGreaterThan(layoutChangedCount, 0, "Test hooks enabled before load should report layout changes.");
                    Verify.AreEqual(2, Microsoft.UI.Private.Controls.RadioButtonsTestHooks.GetColumns(hooksBeforeLoad));
                    Verify.AreEqual(2, Microsoft.UI.Private.Controls.RadioButtonsTestHooks.GetRows(hooksBeforeLoad));
                    Verify.AreEqual(0, Microsoft.UI.Private.Controls.RadioButtonsTestHooks.GetLargerColumns(hooksBeforeLoad));
                });
            }
            finally
            {
                RunOnUIThread.Execute(() =>
                {
                    Microsoft.UI.Private.Controls.RadioButtonsTestHooks.LayoutChanged -= layoutChangedHandler;
                    Microsoft.UI.Private.Controls.RadioButtonsTestHooks.SetTestHooksEnabled(hooksBeforeLoad, false);
                });
            }
        }

        // Scenario: ColumnMajorUniformToLargestGridLayout used directly by ItemsRepeaters: fixed-size items, then property
        //           changes; zero-width items with no available width; and no items at all. Also invalid MaxColumns values.
        // Expected: items are sized to the largest item and laid out column-major with the configured spacing; each setter
        //           re-measures; MaxColumns below 1 throws and keeps the old value; zero-width items use the requested
        //           column count even with no available width; no items take no width.
        // Failure means: ColumnMajorUniformToLargestGridLayout Measure/ArrangeOverride, CalculateColumns or validation broke.
        [TestMethod]
        public void VerifyColumnMajorLayoutStandalone()
        {
            ColumnMajorUniformToLargestGridLayout layout = null;
            ItemsRepeater repeater = null;
            ItemsRepeater zeroWidthRepeater = null;
            ItemsRepeater emptyRepeater = null;
            int measureInvalidatedCount = 0;

            RunOnUIThread.Execute(() =>
            {
                layout = new ColumnMajorUniformToLargestGridLayout();
                Log.Comment("Standalone ColumnMajorUniformToLargestGridLayout default MaxColumns: {0}", layout.MaxColumns);
                layout.MeasureInvalidated += (sender, args) => measureInvalidatedCount++;

                layout.MaxColumns = 3;
                layout.ColumnSpacing = 10;
                layout.RowSpacing = 5;
                Verify.AreEqual(3, measureInvalidatedCount, "Each property change should invalidate the layout's measure.");
                Verify.AreEqual(3, layout.MaxColumns);
                Verify.AreEqual(10.0, layout.ColumnSpacing);
                Verify.AreEqual(5.0, layout.RowSpacing);

                Verify.Throws<global::System.ArgumentException>(() => layout.MaxColumns = 0, "MaxColumns = 0 should throw.");
                Verify.Throws<global::System.ArgumentException>(() => layout.MaxColumns = -1, "MaxColumns = -1 should throw.");
                Verify.AreEqual(3, layout.MaxColumns, "A rejected MaxColumns keeps the previous value.");

                var otherLayout = new ColumnMajorUniformToLargestGridLayout();
                Verify.Throws<global::System.ArgumentException>(() => otherLayout.SetValue(ColumnMajorUniformToLargestGridLayout.MaxColumnsProperty, -1), "MaxColumnsProperty = -1 should throw.");
                Log.Comment("MaxColumns after a rejected SetValue(-1) reads {0}.", otherLayout.MaxColumns);

                repeater = CreateRepeater(layout, 7, "<Border Width='40' Height='20' />");

                // Zero-width items with zero column spacing and no available width.
                zeroWidthRepeater = CreateRepeater(new ColumnMajorUniformToLargestGridLayout() { MaxColumns = 3, ColumnSpacing = 0, RowSpacing = 4 }, 5, "<Border Width='0' Height='10' />");
                zeroWidthRepeater.Width = 0;

                // No items, with the kind of spacing RadioButtons uses.
                emptyRepeater = CreateRepeater(new ColumnMajorUniformToLargestGridLayout() { MaxColumns = 3, ColumnSpacing = 10, RowSpacing = 4 }, 0, "<Border Width='40' Height='20' />");

                var panel = new StackPanel();
                panel.Children.Add(repeater);
                panel.Children.Add(zeroWidthRepeater);
                panel.Children.Add(emptyRepeater);
                Content = panel;
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                // 7 items in 3 columns of 3, 2 and 2 items.
                Verify.AreEqual(new global::Windows.Foundation.Size(3 * 40 + 2 * 10, 3 * 20 + 2 * 5), repeater.DesiredSize);
                var expectedOffsets = new[] { (0, 0), (0, 25), (0, 50), (50, 0), (50, 25), (100, 0), (100, 25) };
                for (int i = 0; i < expectedOffsets.Length; i++)
                {
                    var offset = repeater.TryGetElement(i).ActualOffset;
                    Verify.AreEqual((float)expectedOffsets[i].Item1, offset.X, "X of item " + i);
                    Verify.AreEqual((float)expectedOffsets[i].Item2, offset.Y, "Y of item " + i);
                }

                // 5 zero-width items in 3 columns of 2, 2 and 1 items.
                Verify.AreEqual(new global::Windows.Foundation.Size(0, 2 * 10 + 4), zeroWidthRepeater.DesiredSize, "Zero-width items fit MaxColumns columns even without available width.");

                Verify.AreEqual(0.0f, emptyRepeater.DesiredSize.Width);
                Log.Comment("No items with RowSpacing = 4: repeater desired size {0} x {1}.", emptyRepeater.DesiredSize.Width, emptyRepeater.DesiredSize.Height);

                layout.ColumnSpacing = 0;
                layout.MaxColumns = 2;
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                // 7 items in 2 columns of 4 and 3 items, no column spacing.
                Verify.AreEqual(new global::Windows.Foundation.Size(2 * 40, 4 * 20 + 3 * 5), repeater.DesiredSize);
                Verify.AreEqual(40.0f, repeater.TryGetElement(4).ActualOffset.X);
                Verify.AreEqual(0.0f, repeater.TryGetElement(4).ActualOffset.Y);
            });
        }

        private static ItemsRepeater CreateRepeater(Layout layout, int itemCount, string itemXaml)
        {
            return new ItemsRepeater()
            {
                HorizontalAlignment = HorizontalAlignment.Left,
                VerticalAlignment = VerticalAlignment.Top,
                Layout = layout,
                ItemsSource = Enumerable.Range(0, itemCount).ToList(),
                ItemTemplate = (DataTemplate)XamlReader.Load(
                    "<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>" + itemXaml + "</DataTemplate>")
            };
        }

        // Scenario: RadioButtons whose ItemTemplate is a DataTemplateSelector, and RadioButtons whose ItemTemplate is a
        //           custom element factory that creates RadioButtons; then remove the selected item from the latter.
        // Expected: each item uses the RadioButton template the selector picks (a non-RadioButton template is wrapped); the
        //           custom factory's RadioButtons are used as containers and the removed one is returned to that factory,
        //           clearing the selection.
        // Failure means: RadioButtonsElementFactory ignores DataTemplateSelector/IElementFactory ItemTemplates or recycling.
        [TestMethod]
        public void VerifyItemTemplateSelectorAndCustomElementFactory()
        {
            RadioButtons selectorRadioButtons = null;
            RadioButtons factoryRadioButtons = null;
            ItemsRepeater factoryRepeater = null;
            UIElement removedContainer = null;
            RecordingRadioButtonFactory factory = null;
            var factoryItems = new global::System.Collections.ObjectModel.ObservableCollection<string>() { "a", "b", "c" };

            RunOnUIThread.Execute(() =>
            {
                factory = new RecordingRadioButtonFactory();
                var selector = new ParityTemplateSelector()
                {
                    Even = (DataTemplate)XamlReader.Load(
                        @"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                            <RadioButton Tag='even' Content='{Binding}' />
                        </DataTemplate>"),
                    Odd = (DataTemplate)XamlReader.Load(
                        @"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                            <RadioButton Tag='odd' Content='{Binding}' />
                        </DataTemplate>"),
                    NotARadioButton = (DataTemplate)XamlReader.Load(
                        @"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                            <TextBlock Text='{Binding}' />
                        </DataTemplate>"),
                };

                selectorRadioButtons = new RadioButtons() { ItemsSource = new List<int>() { 0, 1, 2, 3, 4 }, ItemTemplate = selector };
                factoryRadioButtons = new RadioButtons() { ItemsSource = factoryItems, ItemTemplate = factory };

                var panel = new StackPanel();
                panel.Children.Add(selectorRadioButtons);
                panel.Children.Add(factoryRadioButtons);
                Content = panel;
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                for (int i = 0; i < 4; i++)
                {
                    var container = selectorRadioButtons.ContainerFromIndex(i) as RadioButton;
                    Verify.IsNotNull(container, "Item " + i + " should be a RadioButton from the selected template.");
                    Verify.AreEqual(i % 2 == 0 ? "even" : "odd", container.Tag);
                    Verify.AreEqual(i.ToString(), container.Content?.ToString());
                }

                var wrapped = selectorRadioButtons.ContainerFromIndex(4) as RadioButton;
                Verify.IsNotNull(wrapped, "A selected template whose root isn't a RadioButton is wrapped in a RadioButton.");
                Verify.AreEqual("4", wrapped.Content?.ToString());
                Log.Comment("Selector template with a non-RadioButton root: wrapper ContentTemplate is {0}.", wrapped.ContentTemplate == null ? "null" : "set");

                selectorRadioButtons.SelectedIndex = 1;
                Verify.IsTrue(((RadioButton)selectorRadioButtons.ContainerFromIndex(1)).IsChecked == true);

                Verify.AreEqual(3, factory.Created.Count);
                for (int i = 0; i < factoryItems.Count; i++)
                {
                    var container = factoryRadioButtons.ContainerFromIndex(i);
                    Verify.IsTrue(ReferenceEquals(factory.Created.Single(created => (string)created.Tag == factoryItems[i]), container),
                        "Item " + i + " should use the custom factory's RadioButton.");
                }

                factoryRepeater = (ItemsRepeater)VisualTreeUtils.FindVisualChildByName(factoryRadioButtons, "InnerRepeater");
                factoryRadioButtons.SelectedIndex = 2;
                removedContainer = factoryRadioButtons.ContainerFromIndex(2);
                Verify.IsTrue(((RadioButton)removedContainer).IsChecked == true);

                factoryItems.RemoveAt(2);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(1, factory.Recycled.Count, "The removed element should be returned to the custom factory.");
                Verify.IsTrue(ReferenceEquals(removedContainer, factory.Recycled[0].Element));
                Verify.IsTrue(ReferenceEquals(factoryRepeater, factory.Recycled[0].Parent));
                Verify.AreEqual(3, factory.Created.Count, "Removing an item should not create new elements.");
                Verify.AreEqual(-1, factoryRadioButtons.SelectedIndex, "Removing the selected item clears the selection.");
            });
        }

        private static void VerifyDependencyPropertyIdentifiers(params DependencyProperty[] properties)
        {
            for (int i = 0; i < properties.Length; i++)
            {
                Verify.IsNotNull(properties[i], "Dependency property identifier #" + i + " should not be null");
                for (int j = 0; j < i; j++)
                {
                    Verify.IsFalse(ReferenceEquals(properties[i], properties[j]), "Dependency property identifiers #" + j + " and #" + i + " should be distinct");
                }
            }
        }

        private static void VerifyDependencyPropertyRoundTrip<T>(DependencyObject owner, DependencyProperty property, global::System.Func<T> getClrValue, global::System.Action<T> setClrValue, T defaultValue, T newValue)
        {
            Verify.AreEqual(defaultValue, getClrValue());
            Verify.AreEqual(defaultValue, (T)owner.GetValue(property));

            owner.SetValue(property, newValue);
            Verify.AreEqual(newValue, getClrValue(), "Value set through the DP should be visible through the CLR property");

            owner.ClearValue(property);
            Verify.AreEqual(defaultValue, getClrValue(), "ClearValue should restore the default");

            setClrValue(newValue);
            Verify.AreEqual(newValue, (T)owner.GetValue(property), "Value set through the CLR property should be visible through the DP");

            owner.ClearValue(property);
            Verify.AreEqual(defaultValue, getClrValue());
        }

        private partial class ParityTemplateSelector : DataTemplateSelector
        {
            public DataTemplate Even { get; set; }
            public DataTemplate Odd { get; set; }
            public DataTemplate NotARadioButton { get; set; }

            protected override DataTemplate SelectTemplateCore(object item)
            {
                int value = (int)item;
                if (value == 4)
                {
                    return NotARadioButton;
                }
                return value % 2 == 0 ? Even : Odd;
            }
        }

        private partial class RecordingRadioButtonFactory : ElementFactory
        {
            public List<RadioButton> Created { get; } = new List<RadioButton>();
            public List<(UIElement Element, UIElement Parent)> Recycled { get; } = new List<(UIElement Element, UIElement Parent)>();

            protected override UIElement GetElementCore(ElementFactoryGetArgs args)
            {
                var radioButton = new RadioButton() { Content = "Custom " + args.Data, Tag = args.Data };
                Created.Add(radioButton);
                return radioButton;
            }

            protected override void RecycleElementCore(ElementFactoryRecycleArgs args)
            {
                Recycled.Add((args.Element, args.Parent));
            }
        }
    }
}
