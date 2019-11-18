// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System.Collections.ObjectModel;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "CornerRadius")]
    public sealed partial class CornerRadiusPage : TestPage
    {
        public double ControlCornerRadiusSize
        {
            get { return _controlCornerRadius;  }
            set
            {
                if (_controlCornerRadius != value)
                {
                    _controlCornerRadius = value;
                    ControlCornerRadius = new CornerRadius(value);
                }
            }
        }
        private double _controlCornerRadius = 2;

        public double OverlayCornerRadiusSize
        {
            get { return _overlayCornerRadius; }
            set
            {
                if (_overlayCornerRadius != value)
                {
                    _overlayCornerRadius = value;
                    OverlayCornerRadius = new CornerRadius(value);
                }
            }
        }
        private double _overlayCornerRadius = 4;

        public CornerRadius ControlCornerRadius
        {
            get { return (CornerRadius)GetValue(ControlCornerRadiusProperty); }
            set { SetValue(ControlCornerRadiusProperty, value); }
        }

        public static readonly DependencyProperty ControlCornerRadiusProperty =
            DependencyProperty.Register("ControlCornerRadius", typeof(CornerRadius), typeof(CornerRadiusPage), new PropertyMetadata(new CornerRadius(4)));


        public CornerRadius OverlayCornerRadius
        {
            get { return (CornerRadius)GetValue(OverlayCornerRadiusProperty); }
            set { SetValue(OverlayCornerRadiusProperty, value); }
        }

        public static readonly DependencyProperty OverlayCornerRadiusProperty =
            DependencyProperty.Register("OverlayCornerRadius", typeof(CornerRadius), typeof(CornerRadiusPage), new PropertyMetadata(new CornerRadius(4)));



        public ObservableCollection<string> AutoSuggestSource { get; private set; } = new ObservableCollection<string>();

        public CornerRadiusPage()
        {
            AutoSuggestSource.Add("Item 1");
            AutoSuggestSource.Add("Item 2");
            AutoSuggestSource.Add("Item 3");

            ControlCornerRadius = new CornerRadius(ControlCornerRadiusSize);
            OverlayCornerRadius = new CornerRadius(OverlayCornerRadiusSize);

            this.InitializeComponent();
        }

        private void ShowDialog_Click(object sender, RoutedEventArgs e)
        {
            var dialog = new ContentDialog { Title = "Title", Content = "Content", IsPrimaryButtonEnabled = true, PrimaryButtonText = "PrimaryButton" };
            var result = dialog.ShowAsync();
        }


        private void ShowRoundedDialog_Click(object sender, RoutedEventArgs e)
        {
            var dialog = new ContentDialog { Title = "Title", Content = "Content", IsPrimaryButtonEnabled = true, PrimaryButtonText = "PrimaryButton", CornerRadius = OverlayCornerRadius };
            var result = dialog.ShowAsync();
        }
    }
}
