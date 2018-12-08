// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml.Controls;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Markup;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Media.Animation;

#if !BUILD_WINDOWS
using VirtualizingLayout = Microsoft.UI.Xaml.Controls.VirtualizingLayout;
using ItemsRepeater = Microsoft.UI.Xaml.Controls.ItemsRepeater;
using ElementFactory = Microsoft.UI.Xaml.Controls.ElementFactory;
using RecyclingElementFactory = Microsoft.UI.Xaml.Controls.RecyclingElementFactory;
using StackLayout = Microsoft.UI.Xaml.Controls.StackLayout;
using ScrollAnchorProvider = Microsoft.UI.Xaml.Controls.ScrollAnchorProvider;
using SelectTemplateEventArgs = Microsoft.UI.Xaml.Controls.SelectTemplateEventArgs;
#endif

namespace MUXControlsAdhocApp.RepeaterPages
{
    public sealed partial class PerfComparisonPage : Page
    {
        private Random _rnd = new Random(12345);
        private StackLayout _layout = new StackLayout();
        private Func<string> _printChildrenCount;

        public PerfComparisonPage()
        {
            InitializeComponent();

            var flatData = new ObservableCollection<Recipe>(Enumerable.Range(0, 1000).Select(i => new Recipe(i.ToString())));
            var groupedData = new ObservableCollection<RecipeGroup>(Enumerable.Range(0, 10)
                .Select(i => new RecipeGroup(
                    $"Group #{i}",
                    Enumerable.Range(0, 100).Select(j => new Recipe(j.ToString())))));

            flatRepeaterBinding.Click += delegate { SetupRepeater(flatData, false); };
            flatRepeaterPhased.Click += delegate { SetupRepeater(flatData, true); };
            groupedRepeaterBinding.Click += delegate { SetupRepeater(groupedData, false); };
            groupedRepeaterPhased.Click += delegate { SetupRepeater(groupedData, true); };

            flatGridViewBinding.Click += delegate { SetupListView(flatData); };
            groupedGridViewBinding.Click += delegate { SetupListView(groupedData); };
            printChildrenCount.Click += delegate { childrenCount.Text = _printChildrenCount?.Invoke(); };
        }

        private void SetupRepeater(IList data, bool phased)
        {
            var repeater = new ItemsRepeater();
            repeater.ItemsSource = data;
            if(phased)
            {
                repeater.ItemTemplate = (ElementFactory)Resources["SharedElementFactoryPhased"];
            }
            else
            {
                repeater.ItemTemplate = (ElementFactory)Resources["SharedElementFactoryBinding"];
            }
            
            if (data is IList<Recipe>)
            {
                repeater.Layout = (VirtualizingLayout)Resources["SharedFlowLayout"];
            }
            else
            {
                repeater.Layout = (VirtualizingLayout)Resources["SharedStackLayout"];
            }

            repeater.VerticalCacheLength = 4.0;
            repeater.XYFocusKeyboardNavigation = Windows.UI.Xaml.Input.XYFocusKeyboardNavigationMode.Enabled;
            var tracker = new ScrollAnchorProvider();
            var scrollViewer = new Windows.UI.Xaml.Controls.ScrollViewer();
            
            tracker.Content = scrollViewer;
            scrollViewer.Content = repeater;
            scrollViewer.IsTabStop = false;
            host.Child = tracker;

            _printChildrenCount = () => VisualTreeHelper.GetChildrenCount(repeater).ToString();
        }

        private void SetupListView(object data)
        {
            var grid = new GridView();

            if (data is IList<Recipe>)
            {
                grid.ItemsSource = data;
            }
            else
            {
                var cvs = new CollectionViewSource();
                cvs.Source = data;
                cvs.IsSourceGrouped = true;
                cvs.ItemsPath = new PropertyPath("Recipes");
                grid.ItemsSource = cvs.View;

                var groupStyle = new GroupStyle();
                grid.GroupStyle.Add(groupStyle);
            }

            grid.ShowsScrollingPlaceholders = false;
            grid.ItemTemplate = (DataTemplate)Resources["GridViewItemTemplatePhased"];
            grid.ItemContainerTransitions = new TransitionCollection();
            grid.ItemsPanel = (ItemsPanelTemplate)XamlReader.Load("<ItemsPanelTemplate xmlns='http://schemas.microsoft.com/client/2007'><ItemsWrapGrid Orientation='Horizontal' CacheLength='4'/></ItemsPanelTemplate>");
            host.Child = grid;
            _printChildrenCount = () => grid.ItemsPanelRoot.Children.Count.ToString();
        }

        private void OnSelectTemplateKeyBinding(RecyclingElementFactory sender, SelectTemplateEventArgs args)
        {
            args.TemplateKey = args.DataContext is Recipe ? "RepeaterItemTemplateBinding" : "RepeaterGroupTemplateBinding";
        }

        private void OnSelectTemplateKeyPhased(RecyclingElementFactory sender, SelectTemplateEventArgs args)
        {
            args.TemplateKey = args.DataContext is Recipe ? "RepeaterItemTemplatePhased" : "RepeaterGroupTemplatePhased";
        }
    }

    public class Recipe
    {
        public string Description { get; }

        public Recipe(string description)
        {
            Description = description;
        }
    }

    public class RecipeGroup
    {
        public string Name { get; }
        public object Recipes { get; }

        public RecipeGroup(string name, object recipes)
        {
            Name = name;
            Recipes = recipes;
        }

        public override string ToString() => Name;
    }
}
