// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Microsoft.UI.Private.Controls;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using MUXControlsTestApp.Samples;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name="ItemsRepeater", Icon="ItemsRepeater.png")]
    public sealed partial class RepeaterTestUIPage : Page
    {
        private VirtualizingLayout _stackLayout;
        private VirtualizingLayout _gridLayout;
        private VirtualizingLayout _flowLayout;

        public RepeaterTestUIPage()
        {
            this.InitializeComponent();

            objectModelTest.Click += delegate
            {
                Frame.NavigateWithoutAnimation(typeof(ObjectModelTestPage));
            };

            defaultDemo.Click += delegate 
            {
                Frame.NavigateWithoutAnimation(typeof(Defaults));
            };

            basicDemo.Click += delegate
            {
                Frame.NavigateWithoutAnimation(typeof(BasicDemo));
            };

            linedFlowLayoutDemo.Click += delegate 
            {
                Frame.NavigateWithoutAnimation(typeof(LinedFlowLayoutDemo));
            };

            uniformGridLayoutDemo.Click += delegate 
            {
                Frame.NavigateWithoutAnimation(typeof(UniformGridLayoutDemo));
            };

            keyboardNavigationSample.Click += delegate 
            {
                Frame.NavigateWithoutAnimation(typeof(KeyboardNavigationSample));
            };

            itemsSourceDemo.Click += delegate 
            {
                Frame.NavigateWithoutAnimation(typeof(ElementsInItemsSourcePage));
            };

            itemTemplateDemo.Click += delegate
            {
                Frame.NavigateWithoutAnimation(typeof(ItemTemplateDemo));
            };

            collectionChangeDemo.Click += delegate
            {
                Frame.NavigateWithoutAnimation(typeof(CollectionChangeDemo));
            };

            sortingAndFilteringDemo.Click += delegate {
                Frame.NavigateWithoutAnimation(typeof(SortingAndFilteringPage));
            };

            variableSizedItemsDemo.Click += delegate {
                Frame.NavigateWithoutAnimation(typeof(VariableSizedItemsPage));
            };

            animationsDemo.Click += delegate
            {
                Frame.NavigateWithoutAnimation(typeof(AnimationsDemoPage));
            };

            circleLayoutDemo.Click += delegate
            {
                Frame.NavigateWithoutAnimation(typeof(CircleLayoutSamplePage));
            };

            nonVirtualStackLayoutDemo.Click += delegate
            {
                Frame.NavigateWithoutAnimation(typeof(NonVirtualStackLayoutSamplePage));
            };

            virtualFixedStackLayoutDemo.Click += delegate
            {
                Frame.NavigateWithoutAnimation(typeof(VirtualizingUniformStackLayoutSamplePage));
            };

            virtualStackLayoutDemo.Click += delegate
            {
                Frame.NavigateWithoutAnimation(typeof(VirtualizingStackLayoutSamplePage));
            };

            pinterestLayoutDemo.Click += delegate
            {
                Frame.NavigateWithoutAnimation(typeof(PinterestLayoutSamplePage));
            };

            flowLayoutDemo.Click += delegate 
            {
                Frame.NavigateWithoutAnimation(typeof(FlowLayoutDemoPage));
            };

            stackLayoutDemo.Click += delegate
            {
                Frame.NavigateWithoutAnimation(typeof(StackLayoutDemoPage));
            };

            activityFeedLayoutDemo.Click += delegate
            {
                Frame.NavigateWithoutAnimation(typeof(ActivityFeedSamplePage));
            };

            storeDemo.Click += delegate
            {
                Frame.NavigateWithoutAnimation(typeof(MUXControlsTestApp.Samples.StoreDemoPage));
            };

            flatSelectionDemo.Click += delegate
            {
                Frame.NavigateWithoutAnimation(typeof(MUXControlsTestApp.Samples.Selection.FlatSample));
            };

            groupedSelectionDemo.Click += delegate
            {
                Frame.NavigateWithoutAnimation(typeof(MUXControlsTestApp.Samples.Selection.GroupedSample));
            };

            treeSelectionDemo.Click += delegate
            {
                Frame.NavigateWithoutAnimation(typeof(MUXControlsTestApp.Samples.Selection.TreeViewSample));
            };

            animatedScrollDemo.Click += delegate {
                Frame.NavigateWithoutAnimation(typeof(MUXControlsTestApp.Samples.ScaleAnimatedVerticalListDemo));
            };

            noGroupingList.Click += delegate
            {
                Frame.NavigateWithoutAnimation(typeof(ItemsViewWithDataPage),
                    new PageInfo()
                    {
                        Level0Layout = GetStackLayout(),
                        NumLevels = 0,
                        Orientation = orientation.IsOn ? Orientation.Horizontal : Orientation.Vertical,
                    });
            };

            noGroupingGrid.Click += delegate
            {
                Frame.NavigateWithoutAnimation(typeof(ItemsViewWithDataPage),
                    new PageInfo()
                    {
                        Level0Layout = GetGridLayout(),
                        NumLevels = 0,
                        Orientation = orientation.IsOn ? Orientation.Horizontal : Orientation.Vertical,
                    });
            };

            noGroupingFlow.Click += delegate
            {
                Frame.NavigateWithoutAnimation(typeof(ItemsViewWithDataPage),
                    new PageInfo()
                    {
                        Level0Layout = GetFlowLayout(),
                        NumLevels = 0,
                        Orientation = orientation.IsOn ? Orientation.Horizontal : Orientation.Vertical,
                    });
            };

            groupedList.Click += delegate
            {
                Frame.NavigateWithoutAnimation(typeof(ItemsViewWithDataPage),
                    new PageInfo()
                    {
                        Level0Layout = GetStackLayout(),
                        Level1Layout = GetStackLayout(),
                        NumLevels = 1,
                        Orientation = orientation.IsOn ? Orientation.Horizontal : Orientation.Vertical,
                    });
            };

            groupedGrid.Click += delegate
            {
                Frame.NavigateWithoutAnimation(typeof(ItemsViewWithDataPage),
                    new PageInfo()
                    {
                        Level0Layout = GetStackLayout(),
                        Level1Layout = GetGridLayout(),
                        NumLevels = 1,
                        Orientation = orientation.IsOn ? Orientation.Horizontal : Orientation.Vertical,
                    });
            };

            groupedFlow.Click += delegate
            {
                Frame.NavigateWithoutAnimation(typeof(ItemsViewWithDataPage),
                    new PageInfo()
                    {
                        Level0Layout = GetStackLayout(),
                        Level1Layout = GetFlowLayout(),
                        NumLevels = 1,
                        Orientation = orientation.IsOn ? Orientation.Horizontal : Orientation.Vertical,
                    });
            };

            nestedList.Click += delegate
            {
                Frame.NavigateWithoutAnimation(typeof(ItemsViewWithDataPage),
                    new PageInfo()
                    {
                        Level0Layout = GetStackLayout(),
                        Level1Layout = GetStackLayout(),
                        Level2Layout = GetStackLayout(),
                        NumLevels = 2,
                        Orientation = orientation.IsOn ? Orientation.Horizontal : Orientation.Vertical,
                    });
            };

            nestedGrid.Click += delegate
            {
                Frame.NavigateWithoutAnimation(typeof(ItemsViewWithDataPage),
                    new PageInfo()
                    {
                        Level0Layout = GetStackLayout(),
                        Level1Layout = GetStackLayout(),
                        Level2Layout = GetGridLayout(),
                        NumLevels = 2,
                        Orientation = orientation.IsOn ? Orientation.Horizontal : Orientation.Vertical,
                    });
            };

            nestedFlow.Click += delegate
            {
                Frame.NavigateWithoutAnimation(typeof(ItemsViewWithDataPage),
                    new PageInfo()
                    {
                        Level0Layout = GetStackLayout(),
                        Level1Layout = GetStackLayout(),
                        Level2Layout = GetFlowLayout(),
                        NumLevels = 2,
                        Orientation = orientation.IsOn ? Orientation.Horizontal : Orientation.Vertical,
                    });
            };

            ApplyDebugLoggingOptions();
        }

        private void CmbOutputDebugStringLevels_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            ApplyDebugLoggingOptions();
        }

        private void ChkOutputDebugStringLevel_CheckedChanged(object sender, RoutedEventArgs e)
        {
            ApplyDebugLoggingOptions();
        }

        // Applies the OutputDebugString logging level selected in the ComboBox to every checked type, and
        // turns logging off for unchecked types. Reading the current ComboBox selection together with all
        // CheckBox states makes the configuration order-independent: the user can toggle the checkboxes and
        // pick the ComboBox value in any order and still get the requested logging.
        private void ApplyDebugLoggingOptions()
        {
            int selectedLevel = cmbOutputDebugStringLevels?.SelectedIndex ?? 0;
            bool isLoggingInfoLevel = selectedLevel == 1 || selectedLevel == 2;
            bool isLoggingVerboseLevel = selectedLevel == 2;

            SetOutputDebugStringLevelForType("ScrollView", chkScrollView, isLoggingInfoLevel, isLoggingVerboseLevel);
            SetOutputDebugStringLevelForType("LinedFlowLayout", chkLinedFlowLayout, isLoggingInfoLevel, isLoggingVerboseLevel);
        }

        private static void SetOutputDebugStringLevelForType(string type, CheckBox checkBox, bool isLoggingInfoLevel, bool isLoggingVerboseLevel)
        {
            bool isChecked = checkBox != null && checkBox.IsChecked == true;

            MUXControlsTestHooks.SetOutputDebugStringLevelForType(
                type,
                isChecked && isLoggingInfoLevel,
                isChecked && isLoggingVerboseLevel);
        }

        private VirtualizingLayout GetStackLayout()
        {
            if (_stackLayout == null)
            {
                _stackLayout = new StackLayout()
                {
                    Spacing = double.Parse(lineSpacing.Text),
                    Orientation = orientation.IsOn ? Orientation.Horizontal : Orientation.Vertical,
                };
            }

            return _stackLayout;
        }

        private VirtualizingLayout GetGridLayout()
        {
            if (_gridLayout == null)
            {
                _gridLayout = new UniformGridLayout()
                {
                    MinItemWidth = 150,
                    MinItemHeight = 150,
                    MinRowSpacing = double.Parse(itemSpacing.Text),
                    MinColumnSpacing = double.Parse(lineSpacing.Text),
                    ItemsJustification = (UniformGridLayoutItemsJustification)Enum.Parse(typeof(UniformGridLayoutItemsJustification), lineAlignment.Text),
                    Orientation = orientation.IsOn ?  Orientation.Vertical: Orientation.Horizontal,
                };
            }

            return _gridLayout;
        }

        private VirtualizingLayout GetFlowLayout()
        {
            if (_flowLayout == null)
            {
                _flowLayout = new FlowLayout()
                {
                    MinItemSpacing = double.Parse(itemSpacing.Text),
                    LineSpacing = double.Parse(lineSpacing.Text),
                    LineAlignment = (FlowLayoutLineAlignment)Enum.Parse(typeof(FlowLayoutLineAlignment), lineAlignment.Text),
                    Orientation = orientation.IsOn ? Orientation.Vertical : Orientation.Horizontal,
                };
            }

            return _flowLayout;
        }

    }

    public class PageInfo
    {
        public VirtualizingLayout Level0Layout { get; set; }
        public VirtualizingLayout Level1Layout { get; set; }
        public VirtualizingLayout Level2Layout { get; set; }
        public int NumLevels { get; set; }
        public Orientation Orientation { get; set; }
    }
}
