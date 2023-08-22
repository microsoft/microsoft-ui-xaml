// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using MUXControlsTestApp.Samples.Model;
using System;
using System.Collections.Generic;
using System.Linq;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Media.Imaging;
using MUXControlsTestApp.Utils;

using ElementAnimator = Microsoft.UI.Xaml.Controls.ElementAnimator;
using RecyclePool = Microsoft.UI.Xaml.Controls.RecyclePool;
using ItemsRepeater = Microsoft.UI.Xaml.Controls.ItemsRepeater;
using ElementFactory = Microsoft.UI.Xaml.Controls.ElementFactory;
using VirtualizingLayout = Microsoft.UI.Xaml.Controls.VirtualizingLayout;
using StackLayout = Microsoft.UI.Xaml.Controls.StackLayout;
using UniformGridLayout = Microsoft.UI.Xaml.Controls.UniformGridLayout;
using ElementFactoryGetArgs = Microsoft.UI.Xaml.Controls.ElementFactoryGetArgs;
using ElementFactoryRecycleArgs = Microsoft.UI.Xaml.Controls.ElementFactoryRecycleArgs;
using RepeaterTestHooks = Microsoft.UI.Private.Controls.RepeaterTestHooks;

namespace MUXControlsTestApp.Samples
{
    public sealed partial class AnimationsDemoPage : Page
    {
        private string _lorem = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Etiam laoreet erat vel massa rutrum, eget mollis massa vulputate. Vivamus semper augue leo, eget faucibus nulla mattis nec. Donec scelerisque lacus at dui ultricies, eget auctor ipsum placerat. Integer aliquet libero sed nisi eleifend, nec rutrum arcu lacinia. Sed a sem et ante gravida congue sit amet ut augue. Donec quis pellentesque urna, non finibus metus. Proin sed ornare tellus. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Etiam laoreet erat vel massa rutrum, eget mollis massa vulputate. Vivamus semper augue leo, eget faucibus nulla mattis nec. Donec scelerisque lacus at dui ultricies, eget auctor ipsum placerat. Integer aliquet libero sed nisi eleifend, nec rutrum arcu lacinia. Sed a sem et ante gravida congue sit amet ut augue. Donec quis pellentesque urna, non finibus metus. Proin sed ornare tellus.";
        private ResetableCollection<RecipeGroup> _groups;
        private DelegatingItemsSource _dataSource;
        private Random _rnd = new Random(12345);

        public AnimationsDemoPage()
        {
            InitializeComponent();

            _groups = new ResetableCollection<RecipeGroup>(Enumerable.Range(0, 10).Select(j =>
                new RecipeGroup(
                    Enumerable.Range(0, _rnd.Next(5, 16)).Select(k => CreateRecipe(j, k)))
                {
                    Name = "Group #" + j
                }));

            _dataSource = new DelegatingItemsSource(_groups);

            Func<DelegatingItemsSource, int, string> KeyFromIndexFunc = (ds, index) => ds.GetAt(index).ToString();

            repeater.ItemsSource = _dataSource;
            _dataSource.KeyFromIndexFunc = KeyFromIndexFunc;

            repeater.Layout = new StackLayout();
            var generator = new AnimationElementFactory(
                (DataTemplate)Resources["RecipeGroupTemplate"],
                (DataTemplate)Resources["RecipeTemplate"],
                (ElementAnimator)Resources["SharedAnimator"],
                KeyFromIndexFunc);
            repeater.ItemTemplate = generator;
            goBack.Click += delegate { Frame.GoBack(); };

            commands.KeyDown += (s, e) =>
            {
                if (e.Key == Windows.System.VirtualKey.Enter)
                {
                    e.Handled = true;
                    ProcessCommands(commands.Text.ToLowerInvariant());
                }
            };

            stackLayout.Checked += delegate { generator.IsInnerLayoutGrid = false; };
            gridLayout.Checked += delegate { generator.IsInnerLayoutGrid = true; };
            withUniqueId.Checked += delegate { generator.UpdateInnerLayouts(); };
            withoutUniqueId.Checked += delegate { generator.UpdateInnerLayouts(); };

            animationSlowdown.ValueChanged += delegate
            {
                //ElementAnimator.AnimationSlowdownFactor = animationSlowdown.Value;
            };
        }

        private void ProcessCommands(string commands)
        {
            var commandsList = commands.Split(',');
            foreach (var command in commandsList)
            {
                ProcessCommand(command);
            }
        }

        private void ProcessCommand(string command)
        {
            if (command.StartsWith("add")) ProcessAddCommand(command);
            else if (command.StartsWith("rem")) ProcessRemoveCommand(command);
            else if (command.StartsWith("res")) ProcessResetCommand(command);
            else if (command.StartsWith("clr")) ProcessClearCommand(command);
        }

        private void ProcessAddCommand(string command)
        {
            // Examples: add5w2, add5.7
            var args = command.Substring(3, command.Length - 3).Split('w');
            var indexPath = args[0];
            int[] indices = indexPath.Split('.').Select(i => int.Parse(i)).ToArray();
            if (indices.Length == 1)
            {
                var groupIndex = indices[0];
                var itemsCount = args.Length == 2 ? int.Parse(args[1]) : 0;
                var newGroup = new RecipeGroup(Enumerable.Range(0, itemsCount).Select(i => CreateRecipe(groupIndex, i)))
                {
                    Name = string.Format("Group #{0}", _groups.Count)
                };

                if (groupIndex == _groups.Count) _groups.Add(newGroup);
                else _groups.Insert(groupIndex, newGroup);
            }
            else if (indices.Length == 2)
            {
                var groupIndex = indices[0];
                var recipeIndex = indices[1];
                var group = _groups[groupIndex];
                var newRecipe = CreateRecipe(groupIndex, group.Count);

                if (recipeIndex == group.Count) group.Add(newRecipe);
                else group.Insert(recipeIndex, newRecipe);
            }
        }

        private void ProcessRemoveCommand(string command)
        {
            // Examples: rem5, rem5.7
            var indexPath = command.Substring(3, command.Length - 3);
            int[] indices = indexPath.Split('.').Select(i => int.Parse(i)).ToArray();
            var groupIndex = indices[0];
            if (indices.Length == 1)
            {
                _groups.RemoveAt(groupIndex);
            }
            else if (indices.Length == 2)
            {
                var itemIndex = indices[1];
                var group = _groups[groupIndex];
                group.RemoveAt(itemIndex);
            }
        }

        private void ProcessResetCommand(string command)
        {
            // Examples: res, res5
            if (command == "res")
            {
                // Shuffles+add+remove
                _groups.ShuffleAndReset();
            }
            else
            {
                var groupIndex = int.Parse(command.Substring(3, command.Length - 3));
                var group = _groups[groupIndex];
                group.ShuffleAndReset();
            }
        }

        private void ProcessClearCommand(string command)
        {
            // Examples: clr, clr5
            if (command == "clr")
            {
                _groups.Clear();
            }
            else
            {
                var groupIndex = int.Parse(command.Substring(3, command.Length - 3));
                _groups[groupIndex].Clear();
            }
        }


        private Recipe CreateRecipe(int groupIndex, int itemIndex)
        {
            return new Recipe
            {
                ImageUri = new Uri(string.Format("ms-appx:///Images/recipe{0}.png", itemIndex % 8 + 1)),
                Description = _lorem.Substring(0, _rnd.Next(200, 650))
            };
        }

        private class AnimationElementFactory : ElementFactory
        {
            private DataTemplate _groupTemplate;
            private DataTemplate _recipeTemplate;
            private ElementAnimator _animator;
            private RecyclePool _recyclePool = new RecyclePool();
            private List<ItemsRepeater> _innerRepeaters = new List<ItemsRepeater>();
            private Func<DelegatingItemsSource, int, string> _KeyFromIndexFunc;
            private bool _isInnerLayoutGrid;


            public bool IsInnerLayoutGrid
            {
                get { return _isInnerLayoutGrid; }
                set
                {
                    if (_isInnerLayoutGrid != value)
                    {
                        _isInnerLayoutGrid = value;
                        UpdateInnerLayouts();
                    }
                }
            }

            public AnimationElementFactory(
                DataTemplate groupTemplate,
                DataTemplate recipeTemplate,
                ElementAnimator animator,
                Func<DelegatingItemsSource, int, string> KeyFromIndexFunc)
            {
                _groupTemplate = groupTemplate;
                _recipeTemplate = recipeTemplate;
                _animator = animator;
                _KeyFromIndexFunc = KeyFromIndexFunc;
            }

            protected override UIElement GetElementCore(ElementFactoryGetArgs args)
            {
                var key = (args.Data is RecipeGroup) ? "Group" : "Recipe";
                var element = (FrameworkElement)_recyclePool.TryGetElement(key, args.Parent);

                if (element == null)
                {
                    if (args.Data is RecipeGroup)
                    {
                        element = (FrameworkElement)_groupTemplate.LoadContent();
                        _innerRepeaters.Add((ItemsRepeater)element.FindName("groupLayout"));
                    }
                    else
                    {
                        element = (FrameworkElement)_recipeTemplate.LoadContent();
                    }
                }

                element.Tag = key;

                if (args.Data is RecipeGroup)
                {
                    var group = (RecipeGroup)args.Data;
                    var groupName = (TextBlock)element.FindName("groupName");
                    var repeater = (ItemsRepeater)element.FindName("groupLayout");

                    groupName.Text = group.Name;
                    var dataSource = new DelegatingItemsSource(group);
                    dataSource.KeyFromIndexFunc = _KeyFromIndexFunc;
                    repeater.ItemsSource = dataSource;
                    repeater.Layout = IsInnerLayoutGrid ? (VirtualizingLayout)new UniformGridLayout() { MinItemWidth = 150, MinItemHeight = 150 } : (VirtualizingLayout)new StackLayout();
                    RepeaterTestHooks.SetLayoutId(repeater.Layout, group.Name);
                    repeater.ItemTemplate = this;
                    repeater.Animator = _animator;
                }
                else
                {
                    var recipe = (Recipe)args.Data;
                    var image = (Image)element.FindName("recipeImage");
                    var description = (TextBlock)element.FindName("recipeDescription");

                    image.Source = new BitmapImage(recipe.ImageUri);
                    description.Text = recipe.Description;
                }

                return element;
            }

            protected override void RecycleElementCore(ElementFactoryRecycleArgs args)
            {
                var repeater = (ItemsRepeater)((FrameworkElement)args.Element).FindName("groupLayout");
                if (repeater != null)
                {
                    // Make sure all elements are cleared.
                    repeater.Layout = null;
                }

                _recyclePool.PutElement(args.Element, (string)((FrameworkElement)args.Element).Tag, args.Parent);
            }

            public void UpdateInnerLayouts()
            {
                foreach (var repeater in _innerRepeaters)
                {
                    if (repeater.Layout != null)
                    {
                        var id = RepeaterTestHooks.GetLayoutId(repeater.Layout);
                        repeater.Layout = IsInnerLayoutGrid ? (VirtualizingLayout)new UniformGridLayout() { MinItemWidth = 150, MinItemHeight = 150 } : (VirtualizingLayout)new StackLayout();
                        RepeaterTestHooks.SetLayoutId(repeater.Layout, id);
                    }
                }
            }
        }
    }
}
