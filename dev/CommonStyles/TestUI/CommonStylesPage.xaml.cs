// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Generic;
using System.Collections.ObjectModel;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Media;
using System.Linq;
using Common;
using MUXControlsTestApp.Utilities;
using Windows.UI;

namespace MUXControlsTestApp
{
    class SimpleVerify
    {
        private List<string> _errors = new List<string>();

        public void IsTrue(bool condition, string message)
        {
            if (!condition)
            {
                _errors.Add(message);
            }
        }
        public void IsEqual(object actualValue, object expectValue, string message)
        {
            if (expectValue != actualValue && !expectValue.Equals(actualValue))
            {
                _errors.Add(message + " - expect " + expectValue.ToString() + " but it's " + actualValue.ToString());
            }
        }

        public override string ToString()
        {
            if (_errors.Count == 0)
            {
                return "Pass";
            }
            return "Fail: " + string.Join(";", _errors);
        }
    }

    [TopLevelTestPage(Name = "CommonStyles")]
    public sealed partial class CommonStylesPage : TestPage
    {
        ObservableCollection<FontFamily> fonts = new ObservableCollection<FontFamily>();

        public CommonStylesPage()
        {
            this.InitializeComponent();
            fonts.Add(new FontFamily("Arial"));
            fonts.Add(new FontFamily("Courier New"));
            fonts.Add(new FontFamily("Times New Roman"));
        }
        private void SliderDensityTest_Click(object sender, RoutedEventArgs e)
        {
            string expectSliderPreContentMargin = "15";
            string expectSliderPostContentMargin = "15";
            string expectSliderHorizontalHeight = "32";
            string expectSliderVerticalWidth = "32";

            SimpleVerify simpleVerify = new SimpleVerify();
            // Horizontal Slider
            var root = (FrameworkElement)VisualTreeHelper.GetChild(Slider1, 0);         
            var grid = (Grid)root.FindName("HorizontalTemplate");
            simpleVerify.IsTrue(grid != null, "HorizontalTemplate can't be found");
            if (grid != null)
            {
                simpleVerify.IsEqual(grid.MinHeight.ToString(), expectSliderHorizontalHeight, "HorizontalTemplate|MinHeight=SliderHorizontalHeight");
                var rowDefinitions = grid.RowDefinitions;
                if (rowDefinitions.Count == 3)
                {
                    simpleVerify.IsEqual(rowDefinitions[0].Height.ToString(), expectSliderPreContentMargin, "HorizontalTemplate.RowDefinitions[0].Height=SliderPreContentMargin");
                    simpleVerify.IsEqual(rowDefinitions[2].Height.ToString(), expectSliderPostContentMargin, "HorizontalTemplate.RowDefinitions[2].Height=SliderPostContentMargin");
                }
            }

            // Vertical Slider
            root = (FrameworkElement)VisualTreeHelper.GetChild(Slider2, 0);
            grid = (Grid)root.FindName("VerticalTemplate");
            simpleVerify.IsTrue(grid != null, "VerticalTemplate can't be found");
            if (grid != null)
            {
                simpleVerify.IsEqual(grid.MinWidth.ToString(), expectSliderVerticalWidth, "VerticalTemplate|MinWidth=SliderVerticalWidth");
                var columnDefinitions = grid.ColumnDefinitions;
                if (columnDefinitions.Count == 3)
                {
                    simpleVerify.IsEqual(columnDefinitions[0].Width.ToString(), expectSliderPreContentMargin, "VerticalTemplate.columnDefinitions[0].Width=SliderPreContentMargin");
                    simpleVerify.IsEqual(columnDefinitions[2].Width.ToString(), expectSliderPostContentMargin, "VerticalTemplate.columnDefinitions[2].Width=SliderPostContentMargin");
                }
            }
            DensityTestResult.Text = simpleVerify.ToString();
        }

        private void RichEditBoxDensityTest_Click(object sender, RoutedEventArgs e)
        {
            HeaderContentPresenterMarginTest(RichEditBox);
        }

        private void TextBoxDensityTest_Click(object sender, RoutedEventArgs e)
        {
            HeaderContentPresenterMarginTest(TextBox);
        }

        private void PasswordBoxDensityTest_Click(object sender, RoutedEventArgs e)
        {
            HeaderContentPresenterMarginTest(PasswordBox);
        }

        private void ComboBoxDensityTest_Click(object sender, RoutedEventArgs e)
        {
            HeaderContentPresenterMarginTest(ComboBox);
        }

        private void ToggleSwitchDensityTest_Click(object sender, RoutedEventArgs e)
        {
            SimpleVerify simpleVerify = new SimpleVerify();
            var root = (FrameworkElement)VisualTreeHelper.GetChild(ToggleSwitch, 0);
            var contentPresenter = (ContentPresenter)root.FindName("HeaderContentPresenter");
            simpleVerify.IsTrue(contentPresenter != null, "HeaderContentPresenter can't be found");

            string expectedHeaderMargin = "0,0,0,0";
            string expectToggleSwitchPreContentMargin = "6";
            string expectToggleSwitchPostContentMargin = "6";

            if (contentPresenter != null)
            {
                simpleVerify.IsEqual(contentPresenter.Margin.ToString(), expectedHeaderMargin, "HeaderContentPresenter.Margin");
            }

            var grid = (Grid)root;
            if (grid != null)
            {
                var rowDefinitions = grid.RowDefinitions;

                // layout has been changed since RS5 and it's not 4 rows anymore. We expect RS5  will be covered by UIElement tree test case in os repo.
                if (rowDefinitions.Count == 4)
                {
                    simpleVerify.IsEqual(rowDefinitions[1].Height.ToString(), expectToggleSwitchPreContentMargin, "rowDefinitions[1].Height");
                    simpleVerify.IsEqual(rowDefinitions[3].Height.ToString(), expectToggleSwitchPostContentMargin, "rowDefinitions[3].Height");
                }
            }
            DensityTestResult.Text = simpleVerify.ToString();
        }
        private void AutoSuggestBox_TextChanged(AutoSuggestBox sender, AutoSuggestBoxTextChangedEventArgs args)
        {
            if (args.Reason == AutoSuggestionBoxTextChangeReason.UserInput)
            {
                var suggestList = new List<string>();
                suggestList.Add("001");
                suggestList.Add("002");
                suggestList.Add("003");
                suggestList.Add("004");
                suggestList.Add("005");
                sender.ItemsSource = suggestList.Select(text => sender.Text + text);
            }
        }

        private void DatePickerDensityTest_Click(object sender, RoutedEventArgs e)
        {
            SimpleVerify simpleVerify = new SimpleVerify();
            var root = (FrameworkElement)VisualTreeHelper.GetChild(DatePicker, 0);
            var contentPresenter = (ContentPresenter)root.FindName("HeaderContentPresenter");
            simpleVerify.IsTrue(contentPresenter != null, "HeaderContentPresenter can't be found");

            string expectedHeaderMargin = "0,0,0,4";
            string expectDatePickerFlyoutPresenterItemPadding = "0,3,0,6";
            string expectDatePickerFlyoutPresenterMonthPadding = "9,3,0,6";

            if (contentPresenter != null)
            {
                simpleVerify.IsEqual(contentPresenter.Margin.ToString(), expectedHeaderMargin, "HeaderContentPresenter.Margin");
            }

            // Down-level need to change code, so RS5 have different ItemPadding, see bug 19373347, but we will not fix it
            if (!PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone4))
            {
                var textBlock = (TextBlock)root.FindName("DayTextBlock");
                simpleVerify.IsEqual(textBlock.Padding.ToString(), expectDatePickerFlyoutPresenterItemPadding, "DayTextBlock.Padding");

                textBlock = (TextBlock)root.FindName("MonthTextBlock");
                simpleVerify.IsEqual(textBlock.Padding.ToString(), expectDatePickerFlyoutPresenterMonthPadding, "MonthTextBlock.Padding");

                textBlock = (TextBlock)root.FindName("YearTextBlock");
                simpleVerify.IsEqual(textBlock.Padding.ToString(), expectDatePickerFlyoutPresenterItemPadding, "YearTextBlock.Padding");
            }
            DensityTestResult.Text = simpleVerify.ToString();
        }

        private void TimePickerDensityTest_Click(object sender, RoutedEventArgs e)
        {
            SimpleVerify simpleVerify = new SimpleVerify();
            var root = (FrameworkElement)VisualTreeHelper.GetChild(TimePicker, 0);
            var contentPresenter = (ContentPresenter)root.FindName("HeaderContentPresenter");
            simpleVerify.IsTrue(contentPresenter != null, "HeaderContentPresenter can't be found");

            string expectedHeaderMargin = "0,0,0,4";
            string expectTimePickerFlyoutPresenterItemPadding = "0,3,0,6";

            if (contentPresenter != null)
            {
                simpleVerify.IsEqual(contentPresenter.Margin.ToString(), expectedHeaderMargin, "HeaderContentPresenter.Margin");
            }

            // Down-level need to change code, so RS5 have different ItemPadding, see bug 19373347, but we will not fix it
            if (!PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone4))
            {
                var textBlock = (TextBlock)root.FindName("HourTextBlock");
                simpleVerify.IsEqual(textBlock.Padding.ToString(), expectTimePickerFlyoutPresenterItemPadding, "HourTextBlock.Padding");

                textBlock = (TextBlock)root.FindName("MinuteTextBlock");
                simpleVerify.IsEqual(textBlock.Padding.ToString(), expectTimePickerFlyoutPresenterItemPadding, "MinuteTextBlock.Padding");

                textBlock = (TextBlock)root.FindName("PeriodTextBlock");
                simpleVerify.IsEqual(textBlock.Padding.ToString(), expectTimePickerFlyoutPresenterItemPadding, "PeriodTextBlock.Padding");
            }
            DensityTestResult.Text = simpleVerify.ToString();
        }

        private void ListViewItemDensityTest_Click(object sender, RoutedEventArgs e)
        {
            var item = ListView1.FindVisualChildByType<ListViewItem>();
            SimpleVerify simpleVerify = new SimpleVerify();
            if (item != null)
            {
                simpleVerify.IsEqual(item.MinHeight.ToString(), "40", "ListViewItem minHeight is 40");
            }
            else
            {
                simpleVerify.IsTrue(false, "Can't find ListViewItem");
            }
            DensityTestResult.Text = simpleVerify.ToString();
        }

        private void AutoSuggestBoxDensityTest_Click(object sender, RoutedEventArgs e)
        {
            // AutoSuggestBox Density is actually implemented in AutoSuggestBoxTextBoxStyle, so we need to get textbox first
            var root = (FrameworkElement)VisualTreeHelper.GetChild(AutoSuggestBox, 0);
            var textBox = (TextBox)root.FindName("TextBox");
            HeaderContentPresenterMarginTest(textBox);
        }

        private void HeaderContentPresenterMarginTest(DependencyObject control)
        {
            SimpleVerify simpleVerify = new SimpleVerify();
            var root = (FrameworkElement)VisualTreeHelper.GetChild(control, 0);
            var contentPresenter = (ContentPresenter)root.FindName("HeaderContentPresenter");
            simpleVerify.IsTrue(contentPresenter != null, "HeaderContentPresenter can't be found");

            string expectedHeaderMargin = "0,0,0,4";
            if (contentPresenter != null)
            {
                simpleVerify.IsEqual(contentPresenter.Margin.ToString(), expectedHeaderMargin, "HeaderContentPresenter.Margin");
            }

            DensityTestResult.Text = simpleVerify.ToString();
        }
        private void VerifyHeight(FrameworkElement[] frameworkElements, SimpleVerify simpleVerify, int height)
        {
            var expectedHeight = height.ToString();
            foreach (var element in frameworkElements)
            {
                simpleVerify.IsEqual(element.ActualHeight.ToString(), expectedHeight, element.Name.ToString() + ".ActualHeight");
            }
        }

        private void AppBarButtonDensityTest_Click(object sender, RoutedEventArgs e)
        {
            SimpleVerify simpleVerify = new SimpleVerify();
            FrameworkElement[] iconCollapsedElements = { AppBarButton1, AppBarButton3 };
            VerifyHeight(iconCollapsedElements, simpleVerify, 48);

            DensityTestResult.Text = simpleVerify.ToString();
        }

        private void AppBarToggleButtonDensityTest_Click(object sender, RoutedEventArgs e)
        {
            SimpleVerify simpleVerify = new SimpleVerify();
            FrameworkElement[] iconCollapsedElements = { AppBarToggleButton1, AppBarToggleButton3 };
            VerifyHeight(iconCollapsedElements, simpleVerify, 48);

            DensityTestResult.Text = simpleVerify.ToString();
        }

        private void BlueBackground_Click(object sender, RoutedEventArgs e)
        {
            RootSampleControlsPanel.Background = new SolidColorBrush(Color.FromArgb(255,0, 173, 239));
        }

        private void StandardBackground_Click(object sender, RoutedEventArgs e)
        {
            RootSampleControlsPanel.Background = new SolidColorBrush(Colors.Transparent);
        }

    }
}
