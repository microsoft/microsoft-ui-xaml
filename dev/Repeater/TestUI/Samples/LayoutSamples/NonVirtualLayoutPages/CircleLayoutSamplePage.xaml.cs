// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using MUXControlsTestApp.Utils;
using System;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using System.Collections.Generic;
using Windows.UI.Xaml.Tests.MUXControls.ApiTests.RepeaterTests.Common;

using ItemsSourceView = Microsoft.UI.Xaml.Controls.ItemsSourceView;
using ItemsRepeater = Microsoft.UI.Xaml.Controls.ItemsRepeater;
using ElementFactory = Microsoft.UI.Xaml.Controls.ElementFactory;
using ElementFactoryGetArgs = Microsoft.UI.Xaml.Controls.ElementFactoryGetArgs;
using ElementFactoryRecycleArgs = Microsoft.UI.Xaml.Controls.ElementFactoryRecycleArgs;

namespace MUXControlsTestApp.Samples
{
    public sealed partial class CircleLayoutSamplePage : Page
    {
        private ObservableCollection<Person> _data;
        private int _lastId;
        private Random _rnd = new Random(123);

        public CircleLayoutSamplePage()
        {
            InitializeComponent();

            _data = new ObservableCollection<Person>();
            repeater.ItemsSource = new PersonDataSource(_data);
            repeater.ItemTemplate = new PersonElementFactory((DataTemplate)Resources["PersonTemplate"]); ;
            newItem.Content = NewPerson();
            newItem.Tapped += OnNewItemTapped;

            shuffle.Click += delegate
            {
                for (int i = 0; i < _data.Count; i++)
                {
                    int from = _rnd.Next(0, _data.Count);
                    var value = _data[from];
                    _data.RemoveAt(from);
                    int to = _rnd.Next(0, _data.Count);
                    _data.Insert(to, value);
                }
            };

            bringIntoView.Click += delegate
            {
                if (_data.Count > 0)
                {
                    var anchor = repeater.GetOrCreateElement(0);
                    anchor.StartBringIntoView(new BringIntoViewOptions()
                    {
                        HorizontalAlignmentRatio = 0.5,
                        VerticalAlignmentRatio = 0.5,
                        HorizontalOffset = 0.0,
                        VerticalOffset = 0.0,
                        AnimationDesired = true
                    });
                }
            };

            expandButton.Click += delegate
            {
                expandButton.Height = 500 - expandButton.ActualHeight;
            };

            customAnimations.Checked += delegate { repeater.Animator = new RadialElementAnimator(); };
            customAnimations.Unchecked += delegate { repeater.Animator = new DefaultElementAnimator(); };

            maximumHorizontalCacheLength.TextChanged += delegate
            {
                double maximumHorizontalCacheLengthValue;
                if (double.TryParse(maximumHorizontalCacheLength.Text, out maximumHorizontalCacheLengthValue))
                {
                    repeater.HorizontalCacheLength = maximumHorizontalCacheLengthValue;
                }
            };

            maximumVerticalCacheLength.TextChanged += delegate
            {
                double maximumVerticalCacheLengthValue;
                if (double.TryParse(maximumVerticalCacheLength.Text, out maximumVerticalCacheLengthValue))
                {
                    repeater.VerticalCacheLength = maximumVerticalCacheLengthValue;
                }
            };
        }

        private void OnNewItemTapped(object sender, Windows.UI.Xaml.Input.TappedRoutedEventArgs e)
        {
            _data.Insert(0, (Person)newItem.Content);
            newItem.Content = NewPerson();
        }

        private void OnExistingItemTapped(object sender, Windows.UI.Xaml.Input.TappedRoutedEventArgs e)
        {
            _data.Remove((Person)((FrameworkElement)sender).DataContext);
        }

        private Person NewPerson()
        {
            ++_lastId;
            return new Person(_lastId.ToString());
        }
    }

    public class Person
    {
        private string _id;
        public string Id
        {
            get
            {
                return _id;
            }
        }

        public Person(string id)
        {
            _id = id;
        }
    }

    public class PersonDataSource : CustomItemsSourceViewWithUniqueIdMapping
    {
        private ObservableCollection<Person> _data;

        public PersonDataSource(ObservableCollection<Person> data)
        {
            _data = data;
            _data.CollectionChanged += OnCollectionChanged;
        }

        protected override object GetAtCore(int index)
        { return _data[index]; }
        protected override string KeyFromIndexCore(int index)
        { return _data[index].Id; }
        protected override int GetSizeCore()
        { return _data.Count; }

        private void OnCollectionChanged(object sender, NotifyCollectionChangedEventArgs e)
        {
            OnItemsSourceChanged(new NotifyCollectionChangedEventArgs(NotifyCollectionChangedAction.Reset));
        }
    }

    public class PersonElementFactory : ElementFactory
    {
        private DataTemplate _personTemplate;

        public PersonElementFactory(DataTemplate personTemplate)
        {
            _personTemplate = personTemplate;
        }

        protected override UIElement GetElementCore(ElementFactoryGetArgs args)
        {
            var element = (FrameworkElement)_personTemplate.LoadContent();
            element.DataContext = args.Data;
            return element;
        }

        protected override void RecycleElementCore(ElementFactoryRecycleArgs args)
        {

        }
    }
}