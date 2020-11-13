// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Markup;
using Windows.UI;
using System.Windows.Input;

using PipsPager = Microsoft.UI.Xaml.Controls.PipsPager;
using PipsPagerSelectedIndexChangedEventArgs = Microsoft.UI.Xaml.Controls.PipsPagerSelectedIndexChangedEventArgs;
using Windows.UI.Xaml.Input;
using System.Collections.ObjectModel;
using System.Collections.Generic;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "PipsPager")]
    public sealed partial class PipsPagerPage : TestPage
    {
        private List<string> pictures;
        public PipsPagerPage()
        {
            this.InitializeComponent();
            populatePictures();
            Gallery.Loaded += flipView_Loaded;
            Gallery.ItemsSource = pictures;
            PipsPager.NumberOfPages = pictures.Capacity;
            PipsPager.MaxDisplayedPages = 5;
        }

        private void flipView_Loaded(object sender, RoutedEventArgs e)
        {
            Grid grid = (Grid)VisualTreeHelper.GetChild(sender as FlipView, 0);
            for (int i = grid.Children.Count - 1; i >= 0; i--)
                if (grid.Children[i] is Button)
                    grid.Children.RemoveAt(i);
        }

        private void populatePictures()
        {
            pictures = new List<string>();
            pictures.Add("Assets/ingredient1.png");
            pictures.Add("Assets/ingredient2.png");
            pictures.Add("Assets/ingredient3.png");
            pictures.Add("Assets/ingredient4.png");
            pictures.Add("Assets/ingredient5.png");
            pictures.Add("Assets/ingredient6.png");
            pictures.Add("Assets/ingredient7.png");
            pictures.Add("Assets/ingredient8.png");
        }

        private void OnSelectedIndexChanged(PipsPager sender, PipsPagerSelectedIndexChangedEventArgs args)
        {
            Gallery.SelectedIndex = args.NewPageIndex;
        }
    }

}
