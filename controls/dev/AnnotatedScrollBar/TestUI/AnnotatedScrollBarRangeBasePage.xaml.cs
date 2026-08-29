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
    public class IScrollControllerAdapter
    {
        private ScrollViewer _scrollViewer;
        private IScrollController _scrollController;

        // Used to map an IScrollController velocity change request to a ScrollViewer offset change request.
        // This constant allows the 'offsetDelta' value provided to AnnotatedScrollBar::RaiseAddScrollVelocityRequested
        // to be fed to ScrollViewer.ChangeView.
        static private double s_velocityNeededPerPixel = 3.688880455092886;

        public IScrollControllerAdapter(ScrollViewer sv, IScrollController scrollController)
        {
            _scrollViewer = sv;
            _scrollController = scrollController;
            SetUpBridge();
        }

        private void SetUpBridge()
        {
            _scrollViewer.VerticalScrollBarVisibility = ScrollBarVisibility.Hidden;
            _scrollViewer.ViewChanged += pageScrollViewer_ViewChanged;
            _scrollViewer.SizeChanged += pageScrollViewer_SizeChanged;

            _scrollController.ScrollToRequested += _scrollController_ScrollToRequested;
            _scrollController.ScrollByRequested += _scrollController_ScrollByRequested;
            _scrollController.AddScrollVelocityRequested += _scrollController_AddScrollVelocityRequested;
        }

        private void _scrollController_AddScrollVelocityRequested(IScrollController sender, ScrollControllerAddScrollVelocityRequestedEventArgs args)
        {
            var offsetDelta = args.OffsetVelocity / s_velocityNeededPerPixel;
            var newOffset = _scrollViewer.VerticalOffset + offsetDelta;
            newOffset = Math.Max(0, newOffset);
            newOffset = Math.Min(_scrollViewer.ScrollableHeight, newOffset);
            _scrollViewer.ChangeView(null, newOffset, null, false /*disableAnimation*/);
        }

        private void _scrollController_ScrollByRequested(IScrollController sender, ScrollControllerScrollByRequestedEventArgs args)
        {
            var newOffset = _scrollViewer.VerticalOffset + args.OffsetDelta;
            newOffset = Math.Max(0, newOffset);
            newOffset = Math.Min(_scrollViewer.ScrollableHeight, newOffset);
            _scrollViewer.ChangeView(null, newOffset, null, false /*disableAnimation*/);
        }

        private void _scrollController_ScrollToRequested(IScrollController sender, ScrollControllerScrollToRequestedEventArgs args)
        {
            _scrollViewer.ChangeView(null, args.Offset, null, true /*disableAnimation*/);
        }

        private void pageScrollViewer_SizeChanged(object sender, object e)
        {
            _scrollController.SetValues(0, _scrollViewer.ScrollableHeight, _scrollViewer.VerticalOffset, _scrollViewer.ViewportHeight);
        }

        private void pageScrollViewer_ViewChanged(object sender, object e)
        {
            _scrollController.SetValues(0, _scrollViewer.ScrollableHeight, _scrollViewer.VerticalOffset, _scrollViewer.ViewportHeight);
        }
    }

    public sealed partial class AnnotatedScrollBarRangeBasePage : TestPage
    {
        public ObservableCollection<Square> SquareCollection = new ObservableCollection<Square>();
        public ObservableCollection<AnnotatedScrollBarLabel> LabelCollection = new ObservableCollection<AnnotatedScrollBarLabel>();

        private IScrollControllerAdapter adapter = null;

        public AnnotatedScrollBarRangeBasePage()
        {
            this.InitializeComponent();

            adapter = new IScrollControllerAdapter(pageScrollViewer, pageAnnotatedScrollBar.ScrollController);

            PopulateSquares(200);

            Loaded += AnnotatedScrollBarRangeBasePage_Loaded;
        }

        ~AnnotatedScrollBarRangeBasePage()
        {
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
            if(squareNum <= 30)
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
            return 5;
        }

        private double GetOffsetOfItem(int itemNumber)
        {
            return MOCK_GetStaticRowHeight() * itemNumber / MOCK_GetItemsPerRow();
        }

        private void PopulateLabels()
        {
            pageAnnotatedScrollBar.Labels.Clear();

            pageAnnotatedScrollBar.Labels.Add(new AnnotatedScrollBarLabel("Violet", GetOffsetOfItem(0)));
            pageAnnotatedScrollBar.Labels.Add(new AnnotatedScrollBarLabel("Blue", GetOffsetOfItem(30)));
            pageAnnotatedScrollBar.Labels.Add(new AnnotatedScrollBarLabel("Green", GetOffsetOfItem(80)));
            pageAnnotatedScrollBar.Labels.Add(new AnnotatedScrollBarLabel("Red", GetOffsetOfItem(190)));
        }

        public void AnnotatedScrollBar_DetailLabelRequested(object sender, AnnotatedScrollBarDetailLabelRequestedEventArgs args)
        {
            args.Content = GetOffsetLabel(args.ScrollOffset);
        }

        private void AnnotatedScrollBarRangeBasePage_Loaded(object sender, RoutedEventArgs e)
        {
            pageAnnotatedScrollBar.DetailLabelRequested += AnnotatedScrollBar_DetailLabelRequested;
            PopulateLabels();
        }
    }
}
