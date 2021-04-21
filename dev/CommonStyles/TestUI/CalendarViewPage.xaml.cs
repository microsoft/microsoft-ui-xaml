// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using Windows.Globalization;
using Windows.UI;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Media;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "CalendarView", Icon = "CalendarView.png")]
    public sealed partial class CalendarViewPage : TestPage
    {
        private string[] _defaultBrushColors = null;

        public CalendarViewPage()
        {
            this.InitializeComponent();

            List<string> calendarIdentifiers = new List<string>()
            {
                CalendarIdentifiers.Gregorian,
                CalendarIdentifiers.Hebrew,
                CalendarIdentifiers.Hijri,
                CalendarIdentifiers.Japanese,
                CalendarIdentifiers.Julian,
                CalendarIdentifiers.Korean,
                CalendarIdentifiers.Persian,
                CalendarIdentifiers.Taiwan,
                CalendarIdentifiers.Thai,
                CalendarIdentifiers.UmAlQura,
            };

            calendarIdentifier.ItemsSource = calendarIdentifiers;
            calendarIdentifier.SelectedItem = CalendarIdentifiers.Gregorian;

            _defaultBrushColors = new string[brushPropertyName.Items.Count];

            PageCalendar.CalendarViewDayItemChanging += CalendarView_CalendarViewDayItemChanging;
            PageCalendar2.CalendarViewDayItemChanging += CalendarView_CalendarViewDayItemChanging;
        }

        private void CalendarView_CalendarViewDayItemChanging(CalendarView sender, CalendarViewDayItemChangingEventArgs args)
        {
            // Render basic day items.
            if (args.Phase == 0)
            {
                if (isSundayBlackedOut.IsChecked.Value || hasDensityBars.IsChecked.Value)
                {
                    // Register callback for next phase.
                    args.RegisterUpdateCallback(CalendarView_CalendarViewDayItemChanging);
                }
            }
            // Set blackout dates.
            else if (args.Phase == 1)
            {
                // Blackout Sundays and/or Today.
                SetBlackout(args.Item);

                if (hasDensityBars.IsChecked.Value)
                {
                    // Register callback for next phase.
                    args.RegisterUpdateCallback(CalendarView_CalendarViewDayItemChanging);
                }
            }
            // Set density bars.
            else if (args.Phase == 2)
            {
                SetDensityColors(args.Item);
            }
        }

        private void SetDensityColors(CalendarViewDayItem dayItem)
        {
            if (hasDensityBars.IsChecked.Value)
            {
                bool isToday = dayItem.Date.Date.Equals(DateTime.Now.Date);

                if (dayItem.Date.Day % 6 == 0 || isToday)
                {
                    List<Color> densityColors = new List<Color>();

                    densityColors.Add(Colors.Green);
                    densityColors.Add(Colors.Green);

                    if (dayItem.Date.Day % 4 == 0 || isToday)
                    {
                        densityColors.Add(Colors.Blue);
                        densityColors.Add(Colors.Blue);
                    }
                    if (dayItem.Date.Day % 9 == 0 || isToday)
                    {
                        densityColors.Add(Colors.Orange);
                    }
                    if (isToday)
                    {
                        densityColors.Add(Colors.Red);
                        densityColors.Add(Colors.Yellow);
                    }

                    dayItem.SetDensityColors(densityColors);
                }
            }
            else
            {
                dayItem.SetDensityColors(null);
            }
        }

        private void SetBlackout(CalendarViewDayItem dayItem)
        {
            dayItem.IsBlackout = 
                (isSundayBlackedOut.IsChecked.Value && dayItem.Date.DayOfWeek == System.DayOfWeek.Sunday) ||
                (isTodayBlackedOut.IsChecked.Value && dayItem.Date.Date.Equals(DateTime.Now.Date));
        }

        private void SelectionMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            CalendarViewSelectionMode selectionMode;
            if (Enum.TryParse<CalendarViewSelectionMode>((sender as ComboBox).SelectedItem.ToString(), out selectionMode))
            {
                if (PageCalendar != null)
                {
                    PageCalendar.SelectionMode = selectionMode;
                }
                if (PageCalendar2 != null)
                {
                    PageCalendar2.SelectionMode = selectionMode;
                }
            }
        }

        private void GetCalendarWidth_Click(object sender, RoutedEventArgs e)
        {
            calendarWidth.Text = PageCalendar2.Width.ToString();
        }

        private void SetCalendarWidth_Click(object sender, RoutedEventArgs e)
        {
            PageCalendar2.Width = Double.Parse(calendarWidth.Text);
        }

        private void SetDayItemMargin_Click(object sender, RoutedEventArgs e)
        {
            string[] thicknessParts = dayItemMargin.Text.Split(',');

            PageCalendar.Tag = 19; // Set DayItemMargin
            PageCalendar.Tag = float.Parse(thicknessParts[0]);
            PageCalendar.Tag = float.Parse(thicknessParts[1]);
            PageCalendar.Tag = float.Parse(thicknessParts[2]);
            PageCalendar.Tag = float.Parse(thicknessParts[3]);

            PageCalendar2.Tag = 19; // Set DayItemMargin
            PageCalendar2.Tag = float.Parse(thicknessParts[0]);
            PageCalendar2.Tag = float.Parse(thicknessParts[1]);
            PageCalendar2.Tag = float.Parse(thicknessParts[2]);
            PageCalendar2.Tag = float.Parse(thicknessParts[3]);
        }

        private void SetMonthYearItemMargin_Click(object sender, RoutedEventArgs e)
        {
            string[] thicknessParts = monthYearItemMargin.Text.Split(',');

            PageCalendar.Tag = 20; // Set MonthYearItemMargin
            PageCalendar.Tag = float.Parse(thicknessParts[0]);
            PageCalendar.Tag = float.Parse(thicknessParts[1]);
            PageCalendar.Tag = float.Parse(thicknessParts[2]);
            PageCalendar.Tag = float.Parse(thicknessParts[3]);

            PageCalendar2.Tag = 20; // Set MonthYearItemMargin
            PageCalendar2.Tag = float.Parse(thicknessParts[0]);
            PageCalendar2.Tag = float.Parse(thicknessParts[1]);
            PageCalendar2.Tag = float.Parse(thicknessParts[2]);
            PageCalendar2.Tag = float.Parse(thicknessParts[3]);
        }

        private void SetFirstOfMonthLabelMargin_Click(object sender, RoutedEventArgs e)
        {
            string[] thicknessParts = firstOfMonthLabelMargin.Text.Split(',');

            PageCalendar.Tag = 21; // Set FirstOfMonthLabelMargin
            PageCalendar.Tag = float.Parse(thicknessParts[0]);
            PageCalendar.Tag = float.Parse(thicknessParts[1]);
            PageCalendar.Tag = float.Parse(thicknessParts[2]);
            PageCalendar.Tag = float.Parse(thicknessParts[3]);

            PageCalendar2.Tag = 21; // Set FirstOfMonthLabelMargin
            PageCalendar2.Tag = float.Parse(thicknessParts[0]);
            PageCalendar2.Tag = float.Parse(thicknessParts[1]);
            PageCalendar2.Tag = float.Parse(thicknessParts[2]);
            PageCalendar2.Tag = float.Parse(thicknessParts[3]);
        }

        private void SetFirstOfYearDecadeLabelMargin_Click(object sender, RoutedEventArgs e)
        {
            string[] thicknessParts = firstOfYearDecadeLabelMargin.Text.Split(',');

            PageCalendar.Tag = 22; // Set FirstOfYearDecadeLabelMargin
            PageCalendar.Tag = float.Parse(thicknessParts[0]);
            PageCalendar.Tag = float.Parse(thicknessParts[1]);
            PageCalendar.Tag = float.Parse(thicknessParts[2]);
            PageCalendar.Tag = float.Parse(thicknessParts[3]);

            PageCalendar2.Tag = 22; // Set FirstOfYearDecadeLabelMargin
            PageCalendar2.Tag = float.Parse(thicknessParts[0]);
            PageCalendar2.Tag = float.Parse(thicknessParts[1]);
            PageCalendar2.Tag = float.Parse(thicknessParts[2]);
            PageCalendar2.Tag = float.Parse(thicknessParts[3]);
        }

        private void GetDayItemFontSize_Click(object sender, RoutedEventArgs e)
        {
            dayItemFontSize.Text = PageCalendar.DayItemFontSize.ToString();
        }

        private void SetDayItemFontSize_Click(object sender, RoutedEventArgs e)
        {
            PageCalendar.DayItemFontSize = double.Parse(dayItemFontSize.Text);
            PageCalendar2.DayItemFontSize = double.Parse(dayItemFontSize.Text);
        }

        private void GetMonthYearItemFontSize_Click(object sender, RoutedEventArgs e)
        {
            monthYearItemFontSize.Text = PageCalendar.MonthYearItemFontSize.ToString();
        }

        private void SetMonthYearItemFontSize_Click(object sender, RoutedEventArgs e)
        {
            PageCalendar.MonthYearItemFontSize = double.Parse(monthYearItemFontSize.Text);
            PageCalendar2.MonthYearItemFontSize = double.Parse(monthYearItemFontSize.Text);
        }

        private void GetFirstOfMonthLabelFontSize_Click(object sender, RoutedEventArgs e)
        {
            firstOfMonthLabelFontSize.Text = PageCalendar.FirstOfMonthLabelFontSize.ToString();
        }

        private void SetFirstOfMonthLabelFontSize_Click(object sender, RoutedEventArgs e)
        {
            PageCalendar.FirstOfMonthLabelFontSize = double.Parse(firstOfMonthLabelFontSize.Text);
            PageCalendar2.FirstOfMonthLabelFontSize = double.Parse(firstOfMonthLabelFontSize.Text);
        }

        private void GetFirstOfYearDecadeLabelFontSize_Click(object sender, RoutedEventArgs e)
        {
            firstOfYearDecadeLabelFontSize.Text = PageCalendar.FirstOfYearDecadeLabelFontSize.ToString();
        }

        private void SetFirstOfYearDecadeLabelFontSize_Click(object sender, RoutedEventArgs e)
        {
            PageCalendar.FirstOfYearDecadeLabelFontSize = double.Parse(firstOfYearDecadeLabelFontSize.Text);
            PageCalendar2.FirstOfYearDecadeLabelFontSize = double.Parse(firstOfYearDecadeLabelFontSize.Text);
        }

        private void SetCalendarItemCornerRadius_Click(object sender, RoutedEventArgs e)
        {
            PageCalendar.Tag = 23; // Set CalendarItemCornerRadius
            PageCalendar.Tag = float.Parse(calendarItemCornerRadius.Text);

            PageCalendar2.Tag = 23; // Set CalendarItemCornerRadius
            PageCalendar2.Tag = float.Parse(calendarItemCornerRadius.Text);
        }

        private void ResetCalendarItemCornerRadius_Click(object sender, RoutedEventArgs e)
        {
            PageCalendar.Tag = 24; // Reset CalendarItemCornerRadius
            PageCalendar2.Tag = 24; // Reset CalendarItemCornerRadius
        }

        private void GetCalendarItemBorderThickness_Click(object sender, RoutedEventArgs e)
        {
            calendarItemBorderThickness.Text = PageCalendar.CalendarItemBorderThickness.ToString();
        }

        private void SetCalendarItemBorderThickness_Click(object sender, RoutedEventArgs e)
        {
            PageCalendar.CalendarItemBorderThickness = new Thickness(Single.Parse(calendarItemBorderThickness.Text));
            PageCalendar2.CalendarItemBorderThickness = new Thickness(Single.Parse(calendarItemBorderThickness.Text));
        }

        private Brush GetBrushFromIndex()
        {
            switch (brushPropertyName.SelectedIndex)
            {
                case 0: // Background
                    return PageCalendar.Background;
                case 2: // BlackoutForeground
                    return PageCalendar.BlackoutForeground;
                case 3: // BorderBrush
                    return PageCalendar.BorderBrush;
                case 4: // CalendarItemBorderBrush
                    return PageCalendar.CalendarItemBorderBrush;
                case 5: // CalendarItemBackground
                    return PageCalendar.CalendarItemBackground;
                case 7: // CalendarItemForeground
                    return PageCalendar.CalendarItemForeground;
                case 11: // Foreground
                    return PageCalendar.Foreground;
                case 12: // HoverBorderBrush
                    return PageCalendar.HoverBorderBrush;
                case 13: // OutOfScopeBackground
                    return PageCalendar.OutOfScopeBackground;
                case 14: // OutOfScopeForeground
                    return PageCalendar.OutOfScopeForeground;
                case 17: // PressedBorderBrush
                    return PageCalendar.PressedBorderBrush;
                case 18: // PressedForeground
                    return PageCalendar.PressedForeground;
                case 19: // SelectedBorderBrush
                    return PageCalendar.SelectedBorderBrush;
                case 22: // SelectedForeground
                    return PageCalendar.SelectedForeground;
                case 23: // SelectedHoverBorderBrush
                    return PageCalendar.SelectedHoverBorderBrush;
                case 25: // SelectedPressedBorderBrush
                    return PageCalendar.SelectedPressedBorderBrush;
                case 31: // TodayForeground
                    return PageCalendar.TodayForeground;
                default:
/*
#if USE_INSIDER_SDK
                    if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.NineteenH1))
                    {
                        switch (brushPropertyName.SelectedIndex)
                        {
                            case 1: // BlackoutBackground
                                return PageCalendar.BlackoutBackground;
                            case 6: // CalendarItemDisabledBackground
                                return PageCalendar.CalendarItemDisabledBackground;
                            case 8: // CalendarItemHoverBackground
                                return PageCalendar.CalendarItemHoverBackground;
                            case 9: // CalendarItemPressedBackground
                                return PageCalendar.CalendarItemPressedBackground;
                            case 10: // DisabledForeground
                                return PageCalendar.DisabledForeground;
                            case 15: // OutOfScopeHoverForeground
                                return PageCalendar.OutOfScopeHoverForeground;
                            case 16: // OutOfScopePressedForeground
                                return PageCalendar.OutOfScopePressedForeground;
                            case 20: // SelectedDisabledBorderBrush
                                return PageCalendar.SelectedDisabledBorderBrush;
                            case 21: // SelectedDisabledForeground
                                return PageCalendar.SelectedDisabledForeground;
                            case 24: // SelectedHoverForeground
                                return PageCalendar.SelectedHoverForeground;
                            case 26: // SelectedPressedForeground
                                return PageCalendar.SelectedPressedForeground;
                            case 27: // TodayBackground
                                return PageCalendar.TodayBackground;
                            case 28: // TodayBlackoutBackground
                                return PageCalendar.TodayBlackoutBackground;
                            case 29: // TodayBlackoutForeground
                                return PageCalendar.TodayBlackoutForeground;
                            case 30: // TodayDisabledBackground
                                return PageCalendar.TodayDisabledBackground;
                            case 32: // TodayHoverBackground
                                return PageCalendar.TodayHoverBackground;
                            case 33: // TodayPressedBackground
                                return PageCalendar.TodayPressedBackground;
                            case 34: // TodaySelectedInnerBorderBrush
                                return PageCalendar.TodaySelectedInnerBorderBrush;
                        }
                    }
#endif // USE_INSIDER_SDK
*/
                    return null;
            }
        }

        private void GetBrushColor_Click(object sender, RoutedEventArgs e)
        {
            GetBrushColor(GetBrushFromIndex());
        }

        private void GetBrushColor(Brush brush)
        {
            brushColor.Text = GetBrushColorString(brush);
        }

        private string GetBrushColorString(Brush brush)
        {
            SolidColorBrush solidColorBrush = brush as SolidColorBrush;

            if (solidColorBrush == null)
            {
                return "N/A";
            }
            else
            {
                return solidColorBrush.Color.ToString();
            }
        }

        private void SetBrushColor_Click(object sender, RoutedEventArgs e)
        {
            SolidColorBrush solidColorBrush = GetSolidColorBrush();

            if (solidColorBrush == null)
            {
                return;
            }

            switch (brushPropertyName.SelectedIndex)
            {
                case 0: // Background
                    PageCalendar.Background = solidColorBrush;
                    PageCalendar2.Background = solidColorBrush;
                    break;
                case 2: // BlackoutForeground
                    PageCalendar.BlackoutForeground = solidColorBrush;
                    PageCalendar2.BlackoutForeground = solidColorBrush;
                    break;
                case 3: // BorderBrush
                    PageCalendar.BorderBrush = solidColorBrush;
                    PageCalendar2.BorderBrush = solidColorBrush;
                    break;
                case 4: // CalendarItemBorderBrush
                    PageCalendar.CalendarItemBorderBrush = solidColorBrush;
                    PageCalendar2.CalendarItemBorderBrush = solidColorBrush;
                    break;
                case 5: // CalendarItemBackground
                    PageCalendar.CalendarItemBackground = solidColorBrush;
                    PageCalendar2.CalendarItemBackground = solidColorBrush;
                    break;
                case 7: // CalendarItemForeground
                    PageCalendar.CalendarItemForeground = solidColorBrush;
                    PageCalendar2.CalendarItemForeground = solidColorBrush;
                    break;
                case 11: // Foreground
                    PageCalendar.Foreground = solidColorBrush;
                    PageCalendar2.Foreground = solidColorBrush;
                    break;
                case 12: // HoverBorderBrush
                    PageCalendar.HoverBorderBrush = solidColorBrush;
                    PageCalendar2.HoverBorderBrush = solidColorBrush;
                    break;
                case 13: // OutOfScopeBackground
                    PageCalendar.OutOfScopeBackground = solidColorBrush;
                    PageCalendar2.OutOfScopeBackground = solidColorBrush;
                    break;
                case 14: // OutOfScopeForeground
                    PageCalendar.OutOfScopeForeground = solidColorBrush;
                    PageCalendar2.OutOfScopeForeground = solidColorBrush;
                    break;
                case 17: // PressedBorderBrush
                    PageCalendar.PressedBorderBrush = solidColorBrush;
                    PageCalendar2.PressedBorderBrush = solidColorBrush;
                    break;
                case 18: // PressedForeground
                    PageCalendar.PressedForeground = solidColorBrush;
                    PageCalendar2.PressedForeground = solidColorBrush;
                    break;
                case 19: // SelectedBorderBrush
                    PageCalendar.SelectedBorderBrush = solidColorBrush;
                    PageCalendar2.SelectedBorderBrush = solidColorBrush;
                    break;
                case 22: // SelectedForeground
                    PageCalendar.SelectedForeground = solidColorBrush;
                    PageCalendar2.SelectedForeground = solidColorBrush;
                    break;
                case 23: // SelectedHoverBorderBrush
                    PageCalendar.SelectedHoverBorderBrush = solidColorBrush;
                    PageCalendar2.SelectedHoverBorderBrush = solidColorBrush;
                    break;
                case 25: // SelectedPressedBorderBrush
                    PageCalendar.SelectedPressedBorderBrush = solidColorBrush;
                    PageCalendar2.SelectedPressedBorderBrush = solidColorBrush;
                    break;
                case 31: // TodayForeground
                    PageCalendar.TodayForeground = solidColorBrush;
                    PageCalendar2.TodayForeground = solidColorBrush;
                    break;
/*
#if USE_INSIDER_SDK
                default:
                    if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.NineteenH1))
                    {
                        switch (brushPropertyName.SelectedIndex)
                        {
                            case 1: // BlackoutBackground
                                PageCalendar.BlackoutBackground = solidColorBrush;
                                break;
                            case 6: // CalendarItemDisabledBackground
                                PageCalendar.CalendarItemDisabledBackground = solidColorBrush;
                                break;
                            case 8: // CalendarItemHoverBackground
                                PageCalendar.CalendarItemHoverBackground = solidColorBrush;
                                break;
                            case 9: // CalendarItemPressedBackground
                                PageCalendar.CalendarItemPressedBackground = solidColorBrush;
                                break;
                            case 10: // DisabledForeground
                                PageCalendar.DisabledForeground = solidColorBrush;
                                break;
                            case 15: // OutOfScopeHoverForeground
                                PageCalendar.OutOfScopeHoverForeground = solidColorBrush;
                                break;
                            case 16: // OutOfScopePressedForeground
                                PageCalendar.OutOfScopePressedForeground = solidColorBrush;
                                break;
                            case 20: // SelectedDisabledBorderBrush
                                PageCalendar.SelectedDisabledBorderBrush = solidColorBrush;
                                break;
                            case 21: // SelectedDisabledForeground
                                PageCalendar.SelectedDisabledForeground = solidColorBrush;
                                break;
                            case 24: // SelectedHoverForeground
                                PageCalendar.SelectedHoverForeground = solidColorBrush;
                                break;
                            case 26: // SelectedPressedForeground
                                PageCalendar.SelectedPressedForeground = solidColorBrush;
                                break;
                            case 27: // TodayBackground
                                PageCalendar.TodayBackground = solidColorBrush;
                                break;
                            case 28: // TodayBlackoutBackground
                                PageCalendar.TodayBlackoutBackground = solidColorBrush;
                                break;
                            case 29: // TodayBlackoutForeground
                                PageCalendar.TodayBlackoutForeground = solidColorBrush;
                                break;
                            case 30: // TodayDisabledBackground
                                PageCalendar.TodayDisabledBackground = solidColorBrush;
                                break;
                            case 32: // TodayHoverBackground
                                PageCalendar.TodayHoverBackground = solidColorBrush;
                                break;
                            case 33: // TodayPressedBackground
                                PageCalendar.TodayPressedBackground = solidColorBrush;
                                break;
                            case 34: // TodaySelectedInnerBorderBrush
                                PageCalendar.TodaySelectedInnerBorderBrush = solidColorBrush;
                                break;
                        }
                    }
                    break;
#endif // USE_INSIDER_SDK
*/
                case 1: // BlackoutBackground
                case 6: // CalendarItemDisabledBackground
                case 8: // CalendarItemHoverBackground
                case 9: // CalendarItemPressedBackground
                case 10: // DisabledForeground
                case 15: // OutOfScopeHoverForeground
                case 16: // OutOfScopePressedForeground
                case 20: // SelectedDisabledBorderBrush
                case 21: // SelectedDisabledForeground
                case 24: // SelectedHoverForeground
                case 26: // SelectedPressedForeground
                case 27: // TodayBackground
                case 28: // TodayBlackoutBackground
                case 29: // TodayBlackoutForeground
                case 30: // TodayDisabledBackground
                case 32: // TodayHoverBackground
                case 33: // TodayPressedBackground
                case 34: // TodaySelectedInnerBorderBrush
                    switch (brushPropertyName.SelectedIndex)
                    {
                        case 1: // BlackoutBackground
                            PageCalendar.Tag = 1;
                            PageCalendar2.Tag = 1;
                            break;
                        case 6: // CalendarItemDisabledBackground
                            PageCalendar.Tag = 2;
                            PageCalendar2.Tag = 2;
                            break;
                        case 8: // CalendarItemHoverBackground
                            PageCalendar.Tag = 3;
                            PageCalendar2.Tag = 3;
                            break;
                        case 9: // CalendarItemPressedBackground
                            PageCalendar.Tag = 4;
                            PageCalendar2.Tag = 4;
                            break;
                        case 10: // DisabledForeground
                            PageCalendar.Tag = 5;
                            PageCalendar2.Tag = 5;
                            break;
                        case 15: // OutOfScopeHoverForeground
                            PageCalendar.Tag = 6;
                            PageCalendar2.Tag = 6;
                            break;
                        case 16: // OutOfScopePressedForeground
                            PageCalendar.Tag = 7;
                            PageCalendar2.Tag = 7;
                            break;
                        case 20: // SelectedDisabledBorderBrush
                            PageCalendar.Tag = 8;
                            PageCalendar2.Tag = 8;
                            break;
                        case 21: // SelectedDisabledForeground
                            PageCalendar.Tag = 9;
                            PageCalendar2.Tag = 9;
                            break;
                        case 24: // SelectedHoverForeground
                            PageCalendar.Tag = 10;
                            PageCalendar2.Tag = 10;
                            break;
                        case 26: // SelectedPressedForeground
                            PageCalendar.Tag = 11;
                            PageCalendar2.Tag = 11;
                            break;
                        case 27: // TodayBackground
                            PageCalendar.Tag = 12;
                            PageCalendar2.Tag = 12;
                            break;
                        case 28: // TodayBlackoutBackground
                            PageCalendar.Tag = 13;
                            PageCalendar2.Tag = 13;
                            break;
                        case 29: // TodayBlackoutForeround
                            PageCalendar.Tag = 14;
                            PageCalendar2.Tag = 14;
                            break;
                        case 30: // TodayDisabledBackground
                            PageCalendar.Tag = 15;
                            PageCalendar2.Tag = 15;
                            break;
                        case 32: // TodayHoverBackground
                            PageCalendar.Tag = 16;
                            PageCalendar2.Tag = 16;
                            break;
                        case 33: // TodayPressedBackground
                            PageCalendar.Tag = 17;
                            PageCalendar2.Tag = 17;
                            break;
                        case 34: // TodaySelectedInnerBorderBrush
                            PageCalendar.Tag = 18;
                            PageCalendar2.Tag = 18;
                            break;
                    }
                    PageCalendar.Tag = solidColorBrush;
                    PageCalendar2.Tag = solidColorBrush;
                    break;
            }
        }

        private SolidColorBrush GetSolidColorBrush()
        {
            string defaultBrushColor = _defaultBrushColors[brushPropertyName.SelectedIndex];

            if (string.IsNullOrEmpty(defaultBrushColor))
            {
                Brush brush = GetBrushFromIndex();

                if (brush == null)
                {
                    switch (brushPropertyName.SelectedIndex)
                    {
                        case 1: // BlackoutBackground
                            _defaultBrushColors[brushPropertyName.SelectedIndex] = "#00FFFFFF";
                            break;
                        case 6: // CalendarItemDisabledBackground
                            _defaultBrushColors[brushPropertyName.SelectedIndex] = "#00FFFFFF";
                            break;
                        case 8: // CalendarItemHoverBackground
                            _defaultBrushColors[brushPropertyName.SelectedIndex] = "#0A000000";
                            break;
                        case 9: // CalendarItemPressedBackground
                            _defaultBrushColors[brushPropertyName.SelectedIndex] = "#06000000";
                            break;
                        case 10: // DisabledForeground
                            _defaultBrushColors[brushPropertyName.SelectedIndex] = "#5C000000";
                            break;
                        case 15: // OutOfScopeHoverForeground
                            _defaultBrushColors[brushPropertyName.SelectedIndex] = "#9B000000";
                            break;
                        case 16: // OutOfScopePressedForeground
                            _defaultBrushColors[brushPropertyName.SelectedIndex] = "#72000000";
                            break;
                        case 20: // SelectedDisabledBorderBrush
                            _defaultBrushColors[brushPropertyName.SelectedIndex] = "#37000000";
                            break;
                        case 21: // SelectedDisabledForeground
                            _defaultBrushColors[brushPropertyName.SelectedIndex] = "#5C000000";
                            break;
                        case 24: // SelectedHoverForeground
                            _defaultBrushColors[brushPropertyName.SelectedIndex] = "#xFF003E92";
                            break;
                        case 26: // SelectedPressedForeground
                            _defaultBrushColors[brushPropertyName.SelectedIndex] = "#FF005FB7";
                            break;
                        case 27: // TodayBackground
                            _defaultBrushColors[brushPropertyName.SelectedIndex] = "#FF0067C0";
                            break;
                        case 28: // TodayBlackoutBackground
                            _defaultBrushColors[brushPropertyName.SelectedIndex] = "#FF0078D4";
                            break;
                        case 29: // TodayBlackoutForeground
                            _defaultBrushColors[brushPropertyName.SelectedIndex] = "#FFFFFFFF";
                            break;
                        case 30: // TodayDisabledBackground
                            _defaultBrushColors[brushPropertyName.SelectedIndex] = "#37000000";
                            break;
                        case 32: // TodayHoverBackground
                            _defaultBrushColors[brushPropertyName.SelectedIndex] = "#FF003E92";
                            break;
                        case 33: // TodayPressedBackground
                            _defaultBrushColors[brushPropertyName.SelectedIndex] = "#FF0078D4";
                            break;
                        case 34: // TodaySelectedInnerBorderBrush
                            _defaultBrushColors[brushPropertyName.SelectedIndex] = "#FFFFFFFF";
                            break;
                        default:
                            _defaultBrushColors[brushPropertyName.SelectedIndex] = "N/A";
                            break;
                    }
                }
                else
                {
                    _defaultBrushColors[brushPropertyName.SelectedIndex] = GetBrushColorString(brush);
                }
            }

            SolidColorBrush solidColorBrush = null;

            switch (brushColor.SelectedIndex)
            {
                case -1:
                    string colorString = brushColor.SelectedValue as string;
                    if (colorString.Length == 9)
                    {
                        Color color = Color.FromArgb(
                            Byte.Parse(colorString.Substring(1, 2), System.Globalization.NumberStyles.HexNumber), // a
                            Byte.Parse(colorString.Substring(3, 2), System.Globalization.NumberStyles.HexNumber), // r
                            Byte.Parse(colorString.Substring(5, 2), System.Globalization.NumberStyles.HexNumber), // g
                            Byte.Parse(colorString.Substring(7, 2), System.Globalization.NumberStyles.HexNumber));// b
                        solidColorBrush = new SolidColorBrush(color);
                    }
                    break;

                case 0: // Default
                    if (defaultBrushColor != null && defaultBrushColor != "N/A")
                    {
                        Color defaultColor = Color.FromArgb(
                            Byte.Parse(defaultBrushColor.Substring(1, 2), System.Globalization.NumberStyles.HexNumber), // a
                            Byte.Parse(defaultBrushColor.Substring(3, 2), System.Globalization.NumberStyles.HexNumber), // r
                            Byte.Parse(defaultBrushColor.Substring(5, 2), System.Globalization.NumberStyles.HexNumber), // g
                            Byte.Parse(defaultBrushColor.Substring(7, 2), System.Globalization.NumberStyles.HexNumber));// b
                        solidColorBrush = new SolidColorBrush(defaultColor);
                    }
                    break;

                case 1: // Red
                    solidColorBrush = new SolidColorBrush(Colors.Red);
                    break;

                case 2: // Orange
                    solidColorBrush = new SolidColorBrush(Colors.Orange);
                    break;

                case 3: // Yellow
                    solidColorBrush = new SolidColorBrush(Colors.Yellow);
                    break;

                case 4: // Green 
                    solidColorBrush = new SolidColorBrush(Colors.Green);
                    break;
            }

            return solidColorBrush;
        }

        private void IsSundayBlackedOut_Checked(object sender, RoutedEventArgs e)
        {
            SetBlackouts(PageCalendar);
            SetBlackouts(PageCalendar2);
        }

        private void IsSundayBlackedOut_Unchecked(object sender, RoutedEventArgs e)
        {
            SetBlackouts(PageCalendar);
            SetBlackouts(PageCalendar2);
        }

        private void IsTodayBlackedOut_Checked(object sender, RoutedEventArgs e)
        {
            SetBlackouts(PageCalendar);
            SetBlackouts(PageCalendar2);
        }

        private void IsTodayBlackedOut_Unchecked(object sender, RoutedEventArgs e)
        {
            SetBlackouts(PageCalendar);
            SetBlackouts(PageCalendar2);
        }

        private void HasDensityBars_Checked(object sender, RoutedEventArgs e)
        {
            SetDensityColors(PageCalendar);
            SetDensityColors(PageCalendar2);
        }

        private void HasDensityBars_Unchecked(object sender, RoutedEventArgs e)
        {
            SetDensityColors(PageCalendar);
            SetDensityColors(PageCalendar2);
        }

        private void SetBlackouts(CalendarView cv)
        {
            if (cv == null) return;

            var dayItems = Utilities.TestUtilities.FindDescendents<CalendarViewDayItem>(cv);

            foreach (var dayItem in dayItems)
            {
                SetBlackout(dayItem);
            }
        }

        private void SetDensityColors(CalendarView cv)
        {
            if (cv == null) return;

            var dayItems = Utilities.TestUtilities.FindDescendents<CalendarViewDayItem>(cv);
            
            foreach (var dayItem in dayItems)
            {
                SetDensityColors(dayItem);
            }
        }
    }
}
