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
    public class Square
    {
        public string Name { get; set; }
        public string Color { get; set; }
        public double Width { get; set; }
    };

    public sealed partial class AnnotatedScrollBarSummaryPage : TestPage
    {
        public ObservableCollection<Square> SquareCollection = new ObservableCollection<Square>();
        public ObservableCollection<AnnotatedScrollBarLabel> LabelCollection = new ObservableCollection<AnnotatedScrollBarLabel>();

        private double _offsetBetweenLabels = 100;
        private int _numLabels = 0;
        private double _lastRequestedDetailLabelOffset = 0;
        private string _detailLabelContent = null;
        private bool _useLongLabels = false;
        private bool _cancelScrolling = false;
        private double _lastScrollOffset = 0.0;

        public AnnotatedScrollBarSummaryPage()
        {
            this.InitializeComponent();

            Loaded += AnnotatedScrollBarPage_Loaded;

            pageAnnotatedScrollbar.Scrolling += AnnotatedScrollbar_Scrolling;

            PopulateSquares(200);
        }

        ~AnnotatedScrollBarSummaryPage()
        {
        }

        private void AnnotatedScrollBarPage_Loaded(object sender, RoutedEventArgs e)
        {
            pageScrollView.VerticalScrollBarVisibility = ScrollingScrollBarVisibility.Hidden;
            ScrollPresenter scrollPresenter = pageScrollView.GetValue(ScrollView.ScrollPresenterProperty) as ScrollPresenter;
            scrollPresenter.VerticalScrollController = pageAnnotatedScrollbar as IScrollController;
        }

        private void PopulateSquares(int rows)
        {
            for (int i = 0; i < rows; i++)
            {
                string squareName = "Item: " + i;
                double itemWidth = (i % 2 == 0) ? 100 : 100;
                SquareCollection.Add(new Square() { Name = squareName, Color = GetSquareColor(i), Width = itemWidth });
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
                return "Blue";
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

        private void PopulateLabels(int numLabels, double offsetBetweenLabels)
        {
            pageAnnotatedScrollbar.Labels.Clear();

            var labelPrefix = _useLongLabels ? "ThisIsALongLabelWhichShouldExpandASB " : "Num ";

            double currentLabelOffset = 0;
            for(int i = 0; i < numLabels; i++)
            {
                var labelString = labelPrefix + i;
                pageAnnotatedScrollbar.Labels.Add(new AnnotatedScrollBarLabel(labelString, currentLabelOffset));
                currentLabelOffset += offsetBetweenLabels;
            }
        }

        private void BtnPrePopulateLabels_Click(object sender, RoutedEventArgs e)
        {
            if (pageAnnotatedScrollbar != null)
            {
                pageAnnotatedScrollbar.Labels.Clear();
                PopulateLabels(20, 200);
            }
        }

        public void AnnotatedScrollBar_DetailLabelRequested(object sender, AnnotatedScrollBarDetailLabelRequestedEventArgs args)
        {
            _lastRequestedDetailLabelOffset = args.ScrollOffset;
            args.Content = _detailLabelContent;
        }

        private void isDetailLabelEnabled_Checked(object sender, RoutedEventArgs e)
        {
            pageAnnotatedScrollbar.DetailLabelRequested += AnnotatedScrollBar_DetailLabelRequested;
        }

        private void isDetailLabelEnabled_Unchecked(object sender, RoutedEventArgs e)
        {
            pageAnnotatedScrollbar.DetailLabelRequested -= AnnotatedScrollBar_DetailLabelRequested;
        }

        private void useLongLabelContent_Checked(object sender, RoutedEventArgs e)
        {
            _useLongLabels = true;
            _numLabels = 1;
            PopulateLabels(_numLabels, _offsetBetweenLabels);
        }

        private void useLongLabelContent_Unchecked(object sender, RoutedEventArgs e)
        {
            _useLongLabels = false;
            _numLabels = 1;
            PopulateLabels(_numLabels, _offsetBetweenLabels);
        }

        private void cancelScrollingEvent_Checked(object sender, RoutedEventArgs e)
        {
            _cancelScrolling = true;
        }

        private void cancelScrollingEvent_Unchecked(object sender, RoutedEventArgs e)
        {
            _cancelScrolling = false;
        }

        private void AnnotatedScrollbar_Scrolling(AnnotatedScrollBar sender, AnnotatedScrollBarScrollingEventArgs args)
        {
            args.Cancel = _cancelScrolling;
            _lastScrollOffset = args.ScrollOffset;
        }

        private void CmbLabelTemplate_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            switch (cmbLabelTemplate.SelectedIndex)
            {
                case 0:
                    pageAnnotatedScrollbar.LabelTemplate = Resources["LabelTemplate_TextBlock"] as DataTemplate;
                    break;
            }
        }

        private void CmbDetailLabelTemplate_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            switch (cmbDetailLabelTemplate.SelectedIndex)
            {
                case 0:
                    pageAnnotatedScrollbar.DetailLabelTemplate = Resources["DetailLabelTemplate_TextBlock"] as DataTemplate;
                    break;
            }
        }

        private void CmbItemTemplate_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            switch (cmbItemTemplate.SelectedIndex)
            {
                case 0:
                    pageContentRepeater.ItemTemplate = Resources["SquareTemplate"] as DataTemplate;
                    break;
            }
        }

        private void CmbItemsSource_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            switch (cmbItemsSource.SelectedIndex)
            {
                case 0:
                    pageContentRepeater.ItemsSource = null;
                    break;
                case 1:
                    pageContentRepeater.ItemsSource = SquareCollection;
                    break;
            }
        }

        private void BtnGetViewportSize_Click(object sender, RoutedEventArgs e)
        {
            if(txtViewportSize != null)
            {
                txtViewportSize.Text = pageScrollView.ViewportHeight.ToString();
            }
        }

        private void BtnSetViewportSize_Click(object sender, RoutedEventArgs e)
        {
            if(pageAnnotatedScrollbar != null)
            {
                // pageAnnotatedScrollbar.ViewportSize = Convert.ToDouble(txtViewportSize.Text);
                //pageScrollView.ViewportHeight = Convert.ToDouble(txtViewportSize.Text);
            }
        }

        private void BtnGetNumLabels_Click(object sender, RoutedEventArgs e)
        {
            if(pageAnnotatedScrollbar != null)
            {
                txtNumLabels.Text = pageAnnotatedScrollbar.Labels.Count.ToString();
            }
        }

        private void BtnSetNumLabels_Click(object sender, RoutedEventArgs e)
        {
            if(pageAnnotatedScrollbar != null)
            {
                _numLabels = Convert.ToInt16(txtNumLabels.Text);
                PopulateLabels(_numLabels, _offsetBetweenLabels);
            }
        }

        private void BtnGetOffsetBetweenLabels_Click(object sender, RoutedEventArgs e)
        {
            if(pageAnnotatedScrollbar != null)
            {
                txtOffsetBetweenLabels.Text = _offsetBetweenLabels.ToString();
            }
        }

        private void BtnSetOffsetBetweenLabels_Click(object sender, RoutedEventArgs e)
        {
            if(pageAnnotatedScrollbar != null)
            {
                _offsetBetweenLabels = Convert.ToDouble(txtOffsetBetweenLabels.Text);
                PopulateLabels(_numLabels, _offsetBetweenLabels);
            }
        }

        private void BtnGetDetailLabelContent_Click(object sender, RoutedEventArgs e)
        {
            if (pageAnnotatedScrollbar != null)
            {
                txtDetailLabelContent.Text = _detailLabelContent;
            }
        }

        private void BtnSetDetailLabelContent_Click(object sender, RoutedEventArgs e)
        {
            if (pageAnnotatedScrollbar != null)
            {
                _detailLabelContent = txtDetailLabelContent.Text;
            }
        }

        private void BtnGetValue_Click(object sender, RoutedEventArgs e)
        {
            txtValue.Text = _lastScrollOffset.ToString();
        }

        private void BtnGetMinimum_Click(object sender, RoutedEventArgs e)
        {
        }

        private void BtnSetMinimum_Click(object sender, RoutedEventArgs e)
        {
        }
        
        private void BtnGetMaximum_Click(object sender, RoutedEventArgs e)
        {
            txtMaximum.Text = pageScrollView.ScrollableHeight.ToString();
        }

        private void BtnSetMaximum_Click(object sender, RoutedEventArgs e)
        {
        }

        private void BtnGetSmallChange_Click(object sender, RoutedEventArgs e)
        {
            if(pageAnnotatedScrollbar != null)
            {
                txtSmallChange.Text = pageAnnotatedScrollbar.SmallChange.ToString();
            }
        }

        private void BtnSetSmallChange_Click(object sender, RoutedEventArgs e)
        {
            if(pageAnnotatedScrollbar != null)
            {
                pageAnnotatedScrollbar.SmallChange = Convert.ToDouble(txtSmallChange.Text);
            }
        }

        private void BtnGetLastRequestedDetailLabelOffset_Click(object sender, RoutedEventArgs e)
        {
            txtLastRequestedDetailLabelOffset.Text = _lastRequestedDetailLabelOffset.ToString();
        }
    }
}
