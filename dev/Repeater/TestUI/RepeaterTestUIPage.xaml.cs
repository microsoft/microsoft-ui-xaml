// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using MUXControlsTestApp.Samples;
using System;
using Windows.UI.Xaml.Controls;
using FlowLayout = Microsoft.UI.Xaml.Controls.FlowLayout;
using FlowLayoutLineAlignment = Microsoft.UI.Xaml.Controls.FlowLayoutLineAlignment;
using StackLayout = Microsoft.UI.Xaml.Controls.StackLayout;
using UniformGridLayout = Microsoft.UI.Xaml.Controls.UniformGridLayout;
using UniformGridLayoutItemsJustification = Microsoft.UI.Xaml.Controls.UniformGridLayoutItemsJustification;
using VirtualizingLayout = Microsoft.UI.Xaml.Controls.VirtualizingLayout;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "ItemsRepeater", Icon = "ListView.png")]
    public sealed partial class RepeaterTestUIPage : Page
    {
        private VirtualizingLayout _stackLayout;
        private VirtualizingLayout _gridLayout;
        private VirtualizingLayout _flowLayout;

        public RepeaterTestUIPage()
        {
            this.InitializeComponent();

            defaultDemo.Click += delegate 
            {
                Frame.NavigateWithoutAnimation(typeof(Defaults));
            };

            basicDemo.Click += delegate
            {
                Frame.NavigateWithoutAnimation(typeof(BasicDemo));
            };

            uniformGridLayoutDemo.Click += delegate 
            {
                Frame.NavigateWithoutAnimation(typeof(UniformGridLayoutDemo));
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
                    MinRowSpacing = double.Parse(itemSpacing.Text),
                    MinColumnSpacing = double.Parse(lineSpacing.Text),
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
