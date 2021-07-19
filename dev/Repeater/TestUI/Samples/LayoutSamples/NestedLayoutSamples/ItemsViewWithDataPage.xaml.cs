// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using MUXControlsTestApp.Samples.Model;
using System;
using System.Collections.ObjectModel;
using System.Linq;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Navigation;
using MUXControlsTestApp.Utilities;

using RecyclePool = Microsoft.UI.Xaml.Controls.RecyclePool;
using ItemsRepeater = Microsoft.UI.Xaml.Controls.ItemsRepeater;
using UniformGridLayout = Microsoft.UI.Xaml.Controls.UniformGridLayout;
using StackLayout = Microsoft.UI.Xaml.Controls.StackLayout;
using RepeaterTestHooks = Microsoft.UI.Private.Controls.RepeaterTestHooks;

namespace MUXControlsTestApp.Samples
{
    public sealed partial class ItemsViewWithDataPage : Page
    {
        private string _lorem = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Etiam laoreet erat vel massa rutrum, eget mollis massa vulputate. Vivamus semper augue leo, eget faucibus nulla mattis nec. Donec scelerisque lacus at dui ultricies, eget auctor ipsum placerat. Integer aliquet libero sed nisi eleifend, nec rutrum arcu lacinia. Sed a sem et ante gravida congue sit amet ut augue. Donec quis pellentesque urna, non finibus metus. Proin sed ornare tellus. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Etiam laoreet erat vel massa rutrum, eget mollis massa vulputate. Vivamus semper augue leo, eget faucibus nulla mattis nec. Donec scelerisque lacus at dui ultricies, eget auctor ipsum placerat. Integer aliquet libero sed nisi eleifend, nec rutrum arcu lacinia. Sed a sem et ante gravida congue sit amet ut augue. Donec quis pellentesque urna, non finibus metus. Proin sed ornare tellus.";

        ObservableCollection<Recipe> _flatData;
        ObservableCollection<RecipeGroup> _oneLevelNestedData;
        ObservableCollection<Year> _twoLevelNestedData;
        PageInfo _pageInfo;

        public ItemsViewWithDataPage()
        {
            this.InitializeComponent();

            goBackButton.Click += delegate
            {
                Frame.GoBack();
            };

            //this.LayoutUpdated += OnLayoutUpdated;
            //CompositionTarget.Rendering += OnRendering;
            lineAlignmentComboBox.SelectionChanged += LineAlignmentComboBox_SelectionChanged;
            itemsStretchComboBox.SelectionChanged += ItemsStretchComboBox_SelectionChanged;

            scrollButton.Click += OnScrollButtonClicked;
        }

        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
            _pageInfo = e.Parameter as PageInfo;
            if (_pageInfo != null)
            {
                int levels = _pageInfo.NumLevels;
                var level0Layout = _pageInfo.Level0Layout;
                var level1Layout = _pageInfo.Level1Layout;
                var level2Layout = _pageInfo.Level2Layout;
                var rnd = new Random(12345);

                if (_pageInfo.Orientation == Orientation.Horizontal)
                {
                    scrollViewer.VerticalScrollBarVisibility = ScrollBarVisibility.Disabled;
                    scrollViewer.VerticalScrollMode = ScrollMode.Disabled;
                    scrollViewer.HorizontalScrollMode = ScrollMode.Enabled;
                    scrollViewer.HorizontalScrollBarVisibility = ScrollBarVisibility.Visible;
                }

                var generator = _pageInfo.Orientation == Orientation.Vertical ?
                    new RecipeElementFactory(
                        (DataTemplate)Resources["RecipeTemplate"],
                        (DataTemplate)Resources["RecipeGroupTemplate"],
                        (DataTemplate)Resources["YearTemplate"], _pageInfo) :
                    new RecipeElementFactory(
                        (DataTemplate)Resources["RecipeTemplateHorizontal"],
                        (DataTemplate)Resources["RecipeGroupTemplateHorizontal"],
                        (DataTemplate)Resources["YearTemplateHorizontal"], _pageInfo);

                generator.RecyclePool = new RecyclePool();
                rootRepeater.ItemTemplate = generator;
                rootRepeater.Layout = _pageInfo.Level0Layout;
                RepeaterTestHooks.SetLayoutId(rootRepeater.Layout, "Root");

                switch (levels)
                {
                    case 0:
                        _flatData = new ObservableCollection<Recipe>(Enumerable.Range(0, 300).Select(k =>
                            new Recipe
                            {
                                ImageUri = new Uri(string.Format("ms-appx:///Images/recipe{0}.png", k % 8 + 1)),
                                Description = k + " - " + _lorem.Substring(0, level0Layout is StackLayout ? rnd.Next(200, 650) : level0Layout is UniformGridLayout ? 0 : rnd.Next(10, 30))
                                //Description = k.ToString()
                            }));
                        rootRepeater.ItemsSource = _flatData;
                        break;
                    case 1:
                        _oneLevelNestedData = new ObservableCollection<RecipeGroup>(Enumerable.Range(0, 10).Select(j =>
                            new RecipeGroup(
                                Enumerable.Range(0, rnd.Next(5, 16)).Select(k =>
                                         new Recipe
                                         {
                                             ImageUri = new Uri(string.Format("ms-appx:///Images/recipe{0}.png", k % 8 + 1)),
                                             Description = string.Format("{0}.{1} - {2}", j, k, _lorem.Substring(0, level1Layout is StackLayout ? rnd.Next(200, 650) : level1Layout is UniformGridLayout ? 0 : rnd.Next(10, 30)))
                                         }))
                            {
                                Name = "Group #" + j
                            }));
                        rootRepeater.ItemsSource = _oneLevelNestedData;
                        break;
                    case 2:
                        _twoLevelNestedData = new ObservableCollection<Year>(Enumerable.Range(0, 3).Select(i => new Year(
                            new ObservableCollection<RecipeGroup>(Enumerable.Range(0, rnd.Next(2, 5)).Select(j => new RecipeGroup(
                                Enumerable.Range(0, rnd.Next(5, 16)).Select(k =>
                                         new Recipe
                                         {
                                             ImageUri = new Uri(string.Format("ms-appx:///Images/recipe{0}.png", k % 8 + 1)),
                                             Description = string.Format("{0}.{1}.{2} - {3}", 2016 - i, j, k, _lorem.Substring(0, level2Layout is StackLayout ? rnd.Next(200, 650) : level2Layout is UniformGridLayout ? 0 : rnd.Next(10, 30)))
                                         }))
                            {
                                Name = "Group #" + j
                            })))
                        {
                            Value = 2016 - i
                        }));
                        rootRepeater.ItemsSource = _twoLevelNestedData;
                        break;
                }
            }
        }

        private void OnResizeMeButtonClick(object sender, RoutedEventArgs e)
        {
            var button = (Button)sender;
            button.Height = 5000 - button.Height;
        }

        private void OnMaintainViewportSliderValueChanged(object sender, RangeBaseValueChangedEventArgs e)
        {
            tracker.HorizontalAnchorRatio = maintainViewportSlider.Value;
        }

        private void OnRowSpacingSliderValueChanged(object sender, RangeBaseValueChangedEventArgs e)
        {
            if (_pageInfo.Level0Layout != null)
            {
                LayoutHelper.SetMinRowSpacing(_pageInfo.Level0Layout, rowSpacingSlider.Value); ;
            }

            if (_pageInfo.Level1Layout != null)
            {
                LayoutHelper.SetMinRowSpacing(_pageInfo.Level1Layout, rowSpacingSlider.Value);
            }

            if (_pageInfo.Level2Layout != null)
            {
                LayoutHelper.SetMinRowSpacing(_pageInfo.Level2Layout, rowSpacingSlider.Value);
            }
        }
        private void OnMaxRowsOrColumnsSliderValueChanged(object sender, RangeBaseValueChangedEventArgs e)
        {
            if (_pageInfo.Level0Layout != null)
            {
                LayoutHelper.SetMaxRowsOrColumns(_pageInfo.Level0Layout, (int)(maxRowsOrColumnsSlider.Value));
            }
        }

        private void OnColumnSpacingSliderValueChanged(object sender, RangeBaseValueChangedEventArgs e)
        {
            if (_pageInfo.Level0Layout != null)
            {
                LayoutHelper.SetMinColumnSpacing(_pageInfo.Level0Layout, colSpacingSlider.Value);
            }

            if (_pageInfo.Level1Layout != null)
            {
                LayoutHelper.SetMinColumnSpacing(_pageInfo.Level1Layout, colSpacingSlider.Value);
            }

            if (_pageInfo.Level2Layout != null)
            {
                LayoutHelper.SetMinColumnSpacing(_pageInfo.Level2Layout, colSpacingSlider.Value);
            }
        }

        private void LineAlignmentComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            var alignment = (lineAlignmentComboBox.SelectedItem as ComboBoxItem).Content.ToString();
            if (_pageInfo.Level0Layout != null)
            {
                LayoutHelper.SetLineAlignment(_pageInfo.Level0Layout, alignment);
            }

            if (_pageInfo.Level1Layout != null)
            {
                LayoutHelper.SetLineAlignment(_pageInfo.Level1Layout, alignment);
            }

            if (_pageInfo.Level2Layout != null)
            {
                LayoutHelper.SetLineAlignment(_pageInfo.Level2Layout, alignment);
            }
        }

        private void ItemsStretchComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            var alignment = (itemsStretchComboBox.SelectedItem as ComboBoxItem).Content.ToString();
            if (_pageInfo.Level0Layout != null)
            {
                LayoutHelper.SetItemsStretch(_pageInfo.Level0Layout, alignment);
            }

            if (_pageInfo.Level1Layout != null)
            {
                LayoutHelper.SetItemsStretch(_pageInfo.Level1Layout, alignment);
            }

            if (_pageInfo.Level2Layout != null)
            {
                LayoutHelper.SetItemsStretch(_pageInfo.Level2Layout, alignment);
            }
        }

        private void OnLayoutUpdated(object sender, object e)
        {
            //Logger.WriteLine("---------------------- layout updated ---------------------");
        }

        private void OnRendering(object sender, object e)
        {
            //Logger.WriteLine("----------------------- on rendering ----------------------");
        }

        private void OnScrollButtonClicked(object sender, RoutedEventArgs e)
        {
            var indexPath = scrollIndex.Text.Split('.').Select(i => int.Parse(i)).ToArray();
            var repeater = rootRepeater;

            for (int i = 0; i < indexPath.Length; ++i)
            {
                var container = repeater.GetOrCreateElement(indexPath[i]);

                if (i < indexPath.Length - 1)
                {
                    repeater = container.FindVisualChildByType<ItemsRepeater>();
                }
                else
                {
                    container.StartBringIntoView(new BringIntoViewOptions()
                    {
                        HorizontalAlignmentRatio = 0.5,
                        VerticalAlignmentRatio = maintainViewportSlider.Value,
                        HorizontalOffset = 0.0,
                        VerticalOffset = scrollOffset.Value,
                        AnimationDesired = scrollAnimate.IsChecked.GetValueOrDefault()
                    });
                }
            }

        }

        private void OnRecipeDeleted(object sender, RoutedEventArgs e)
        {
            if (_flatData != null)
            {
                // walk up the tree to find the container and repeater.
                var container = (FrameworkElement)sender;
                while (!(container.Parent is ItemsRepeater))
                {
                    container = (FrameworkElement)container.Parent;
                }

                ItemsRepeater repeater = (ItemsRepeater)container.Parent;
                int index = repeater.GetElementIndex(container);
                _flatData.RemoveAt(index);
            }
        }

        private void OnRecipeInserted(object sender, RoutedEventArgs e)
        {
            if (_flatData != null)
            {
                // walk up the tree to find the container and repeater.
                var container = (FrameworkElement)sender;
                while (!(container.Parent is ItemsRepeater))
                {
                    container = (FrameworkElement)container.Parent;
                }

                ItemsRepeater repeater = (ItemsRepeater)container.Parent;
                int index = repeater.GetElementIndex(container) + 1;
                _flatData.Insert(index, new Recipe()
                {
                    ImageUri = new Uri("ms-appx:///Images/recipe0.png"),
                    Description = "added recipe",
                });
            }
        }
    }
}
