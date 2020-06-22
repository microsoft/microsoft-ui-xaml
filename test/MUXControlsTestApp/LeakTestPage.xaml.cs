// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;

using RatingControl = Microsoft.UI.Xaml.Controls.RatingControl;
using NavigationViewItem = Microsoft.UI.Xaml.Controls.NavigationViewItem;
using NavigationView = Microsoft.UI.Xaml.Controls.NavigationView;
using ColorPicker = Microsoft.UI.Xaml.Controls.ColorPicker;
using ColorChangedEventArgs = Microsoft.UI.Xaml.Controls.ColorChangedEventArgs;
using ScrollPresenter = Microsoft.UI.Xaml.Controls.Primitives.ScrollPresenter;

namespace MUXControlsTestApp
{
    public class LeakObject
    {
        public LeakObject()
        {
            _ratingControl1 = new RatingControl();
            _ratingControl2 = new RatingControl();
            _ratingControl2.ValueChanged += _ratingControl2_OnValueChanged;
        }

        ~LeakObject()
        {
            //_ratingControl1.Value = 5;
        }

        private void _ratingControl2_OnValueChanged(RatingControl sender, object args)
        {
            
        }

        public RatingControl _ratingControl1;
        public RatingControl _ratingControl2;
    }

    public class EventCycleTest : Grid
    {
        RatingControl _rating;
        ColorPicker _colorPicker;
        NavigationView _navigationView;
        ScrollPresenter _scroller;

        public EventCycleTest(bool addToTree = true)
        {
            _rating = new RatingControl();
            _rating.ValueChanged += OnRatingValueChanged;
            if (addToTree) Children.Add(_rating);

            _colorPicker = new ColorPicker();
            _colorPicker.ColorChanged += OnColorPickerColorChanged;
            if (addToTree) Children.Add(_colorPicker);

            _navigationView = new NavigationView();
            _navigationView.SelectionChanged += OnNavigationViewSelectionChanged;
            if (addToTree) Children.Add(_navigationView);

            var item = new NavigationViewItem();
            _navigationView.MenuItems.Add(item);
            
            _scroller = new ScrollPresenter();
            _scroller.ViewChanged += OnScrollPresenterViewChanged;
            if (addToTree) Children.Add(_scroller);
        }

        private void OnItemInvoked(NavigationViewItem sender, object args)
        {
        }

        private void OnNavigationViewSelectionChanged(NavigationView sender, object args)
        {
        }

        private void OnColorPickerColorChanged(ColorPicker sender, ColorChangedEventArgs args)
        {
        }
        
        private void OnScrollPresenterViewChanged(ScrollPresenter sender, object args)
        {
        }

        private void OnRatingValueChanged(RatingControl sender, object args)
        {
        }
    }

    [TopLevelTestPage(Name="Leak")]
    public sealed partial class LeakTestPage : TestPage
    {
        public LeakTestPage()
        {
            this.InitializeComponent();

            var x = new LeakObject();
        }

        private void Button_Click(object sender, RoutedEventArgs e)
        {
            new EventCycleTest(false);
        }

        private void TreeButton_Click(object sender, RoutedEventArgs e)
        {
            _stackPanel.Children.Add(new EventCycleTest());
        }
    }
}
