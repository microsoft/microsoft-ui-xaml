// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Windows.Input;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Media;
using Windows.UI;

namespace MUXControlsTestApp
{
    public sealed partial class AnnotatedScrollBarIScrollControllerPage : TestPage
    {
        public ObservableCollection<Square> SquareCollection = new ObservableCollection<Square>();
        public ObservableCollection<AnnotatedScrollBarLabel> LabelCollection = new ObservableCollection<AnnotatedScrollBarLabel>();

        private const int c_SquareWidth = 100;

        public AnnotatedScrollBarIScrollControllerPage()
        {
            this.InitializeComponent();
            PopulateSquares(200);
        }

        ~AnnotatedScrollBarIScrollControllerPage()
        {
        }

        private void PageContentRepeater_Loaded(object sender, object e)
        {
            PopulateLabels();
        }

        private void AnnotatedScrollBarPage_Loaded(object sender, RoutedEventArgs e)
        {
            pageContentRepeater.ItemsSource = SquareCollection;
            pageContentRepeater.Loaded  += PageContentRepeater_Loaded;

           ScrollPresenter scrollPresenter = pageScrollView.GetValue(ScrollView.ScrollPresenterProperty) as ScrollPresenter;
            scrollPresenter.VerticalScrollController = pageAnnotatedScrollbar as IScrollController;

            pageAnnotatedScrollbar.DetailLabelRequested += AnnotatedScrollBar_DetailLabelRequested;
        }

        private void PopulateSquares(int rows)
        {
            for (int i = 0; i < rows; i++)
            {
                string squareName = "Item: " + i;
                // double itemWidth = (i % 2 == 0) ? c_SquareWidth : c_SquareWidth;
                SquareCollection.Add(new Square() { Name = squareName, Color = GetSquareColor(i), Width = c_SquareWidth });
            }
        }

        private string GetSquareColor(int squareNum)
        {
            if (squareNum <= 30)
            {
                return "Violet";
            }
            else if (squareNum <= 80)
            {
                return "LightBlue";
            }
            else if (squareNum <= 190)
            {
                return "Green";
            }
            else
            {
                return "Red";
            }
        }

        private string GetOffsetLabel(double offset)
        {
            if (offset <= GetOffsetOfItem(30))
            {
                return GetSquareColor(30);
            }
            else if (offset <= GetOffsetOfItem(80))
            {
                return GetSquareColor(80);
            }
            else if (offset <= GetOffsetOfItem(190))
            {
                return GetSquareColor(190);
            }
            else
            {
                return GetSquareColor(191);
            }
        }

        private double MOCK_GetStaticRowHeight()
        {
            return 140;
        }

        private double MOCK_GetItemsPerRow()
        {
            if(pageScrollView != null)
            {
                var totalItemWidth = c_SquareWidth + 20*2 /*margin*/;
                var itemsPerRow = pageScrollView.ActualWidth / totalItemWidth;
                return (int)itemsPerRow;
            }
            return 5;
        }

        private double GetOffsetOfItem(int itemNumber)
        {
            return MOCK_GetStaticRowHeight() * itemNumber / MOCK_GetItemsPerRow();
        }

        private void PopulateLabels()
        {
            pageAnnotatedScrollbar.Labels.Clear();

            pageAnnotatedScrollbar.Labels.Add(new AnnotatedScrollBarLabel("Violet", GetOffsetOfItem(0)));
            pageAnnotatedScrollbar.Labels.Add(new AnnotatedScrollBarLabel("Blue", GetOffsetOfItem(30)));
            pageAnnotatedScrollbar.Labels.Add(new AnnotatedScrollBarLabel("Green", GetOffsetOfItem(80)));
            pageAnnotatedScrollbar.Labels.Add(new AnnotatedScrollBarLabel("Red", GetOffsetOfItem(190)));
        }

        public void AnnotatedScrollBar_DetailLabelRequested(object sender, AnnotatedScrollBarDetailLabelRequestedEventArgs args)
        {
            var maxOffset = pageAnnotatedScrollbar.LabelContentAreaSize;
            args.Content = GetOffsetLabel(args.ScrollOffset);
        }
    }
}
