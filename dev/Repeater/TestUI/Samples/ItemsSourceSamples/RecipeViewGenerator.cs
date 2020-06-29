// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using MUXControlsTestApp.Samples.Model;
using System;
using System.Diagnostics;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Media.Imaging;

using RecyclePool = Microsoft.UI.Xaml.Controls.RecyclePool;
using ItemsRepeater = Microsoft.UI.Xaml.Controls.ItemsRepeater;
using ElementFactory = Microsoft.UI.Xaml.Controls.ElementFactory;
using ElementFactoryGetArgs = Microsoft.UI.Xaml.Controls.ElementFactoryGetArgs;
using ElementFactoryRecycleArgs = Microsoft.UI.Xaml.Controls.ElementFactoryRecycleArgs;
using RepeaterTestHooks = Microsoft.UI.Private.Controls.RepeaterTestHooks;

namespace MUXControlsTestApp.Samples
{
    public class RecipeElementFactory : ElementFactory
    {
        private DataTemplate _itemTemplate;
        private DataTemplate _groupTemplate;
        private DataTemplate _yearTemplate;
        private RecyclePool _sharedRecyclePool = new RecyclePool();
        private PageInfo _pageInfo;

        public const string RecipeContainerType = "RecipeContainer";
        public const string RecipeGroupType = "RecipeGroup";
        public const string YearType = "Year";

        public RecyclePool RecyclePool { get; set; }

        public RecipeElementFactory(
            DataTemplate itemTemplate,
            DataTemplate groupTemplate,
            DataTemplate yearTemplate,
            PageInfo pinfo)
        {
            _itemTemplate = itemTemplate;
            _groupTemplate = groupTemplate;
            _yearTemplate = yearTemplate;
            _pageInfo = pinfo;
        }

        protected override UIElement GetElementCore(ElementFactoryGetArgs args)
        {
            var data = args.Data;
            string key = GetContainerType(data);
            var element = RecyclePool.TryGetElement(key, args.Parent);
            element = element ?? (FrameworkElement)CreateContainer(key);

            if (element == null)
            {
                throw new InvalidOperationException("Unable to create a container for key " + key);
            }

            PrepareContainer(element, key, data);

            (element as FrameworkElement).Tag = key;

            return element;
        }

        protected override void RecycleElementCore(ElementFactoryRecycleArgs args)
        {
            var element = args.Element;
            var owner = args.Parent;
            var key = (String)(element as FrameworkElement).Tag;
            Debug.Assert(key != null);
            RecyclePool.PutElement((FrameworkElement)element, key, owner);


#if DEBUG
            var containerAsFE = (FrameworkElement)element;

            switch (key)
            {
                case RecipeGroupType:
                    {
                        var repeater = (ItemsRepeater)containerAsFE.FindName("groupLayout");
                        var id = RepeaterTestHooks.GetLayoutId(repeater.Layout);
                        RepeaterTestHooks.SetLayoutId(repeater.Layout, id + "*");
                        break;
                    }
                case YearType:
                    {
                        var repeater = (ItemsRepeater)containerAsFE.FindName("yearLayout");
                        var id = RepeaterTestHooks.GetLayoutId(repeater.Layout);
                        RepeaterTestHooks.SetLayoutId(repeater.Layout, id + "*");
                        break;
                    }
            }
#endif
        }

        private string GetContainerType(object data)
        {
            return
               data is Year ? YearType :
               data is RecipeGroup ? RecipeGroupType : RecipeContainerType;
        }

        private UIElement CreateContainer(string recycleKey)
        {
            return
               recycleKey == YearType ?
                   (FrameworkElement)_yearTemplate.LoadContent() :
               recycleKey == RecipeGroupType ?
                   (FrameworkElement)_groupTemplate.LoadContent() :
                   (FrameworkElement)_itemTemplate.LoadContent();
        }

        private void PrepareContainer(UIElement container, string containerType, object data)
        {
            var containerFE = container as FrameworkElement;
            switch (containerType)
            {
                case RecipeContainerType:
                    {
                        var recipe = (Recipe)data;

                        var image = (Image)containerFE.FindName("recipeImage");
                        var description = (TextBlock)containerFE.FindName("recipeDescription");

                        if (image.Source == null)
                        {
                            image.Source = new BitmapImage(recipe.ImageUri);
                        }
                        else
                        {
                            ((BitmapImage)image.Source).UriSource = recipe.ImageUri;
                        }

                        description.Text = recipe.Description;

                        break;
                    }

                case RecipeGroupType:
                    {
                        var group = (RecipeGroup)data;

                        var groupName = (TextBlock)containerFE.FindName("groupName");
                        var repeater = (ItemsRepeater)containerFE.FindName("groupLayout");

                        groupName.Text = group.Name;

                        repeater.ItemsSource = group;
                        repeater.Layout = _pageInfo.NumLevels == 1 ? _pageInfo.Level1Layout : _pageInfo.Level2Layout;
#if DEBUG
                        RepeaterTestHooks.SetLayoutId(repeater.Layout, group.Name);
#endif
                        repeater.ItemTemplate = this;
                        break;
                    }

                case YearType:
                    {
                        var year = (Year)data;

                        var yearName = (TextBlock)containerFE.FindName("yearName");
                        var repeater = (ItemsRepeater)containerFE.FindName("yearLayout");

                        yearName.Text = year.Value.ToString();

                        repeater.ItemsSource = year;
                        repeater.Layout = _pageInfo.Level0Layout;
#if DEBUG
                        RepeaterTestHooks.SetLayoutId(repeater.Layout, string.Format("Year {0}", year.Value));
#endif
                        repeater.ItemTemplate = this;

                        break;
                    }
            }
        }
    }
}
