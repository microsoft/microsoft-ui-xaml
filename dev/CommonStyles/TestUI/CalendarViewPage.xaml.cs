// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
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
                if (isTodayBlackedOut.IsChecked.Value || isSundayBlackedOut.IsChecked.Value || hasDensityBars.IsChecked.Value)
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
#if USE_INSIDER_SDK
            if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.TwentyOneH1))
            {
                string[] thicknessParts = dayItemMargin.Text.Split(',');

                PageCalendar.DayItemMargin = new Thickness(
                    float.Parse(thicknessParts[0]),
                    float.Parse(thicknessParts[1]),
                    float.Parse(thicknessParts[2]),
                    float.Parse(thicknessParts[3]));

                PageCalendar2.DayItemMargin = new Thickness(
                    float.Parse(thicknessParts[0]),
                    float.Parse(thicknessParts[1]),
                    float.Parse(thicknessParts[2]),
                    float.Parse(thicknessParts[3]));
            }
#endif
        }

        private void SetMonthYearItemMargin_Click(object sender, RoutedEventArgs e)
        {
#if USE_INSIDER_SDK
            if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.TwentyOneH1))
            {
                string[] thicknessParts = monthYearItemMargin.Text.Split(',');

                PageCalendar.MonthYearItemMargin = new Thickness(
                    float.Parse(thicknessParts[0]),
                    float.Parse(thicknessParts[1]),
                    float.Parse(thicknessParts[2]),
                    float.Parse(thicknessParts[3]));

                PageCalendar2.MonthYearItemMargin = new Thickness(
                    float.Parse(thicknessParts[0]),
                    float.Parse(thicknessParts[1]),
                    float.Parse(thicknessParts[2]),
                    float.Parse(thicknessParts[3]));
            }
#endif
        }

        private void SetFirstOfMonthLabelMargin_Click(object sender, RoutedEventArgs e)
        {
#if USE_INSIDER_SDK
            if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.TwentyOneH1))
            {
                string[] thicknessParts = firstOfMonthLabelMargin.Text.Split(',');

                PageCalendar.FirstOfMonthLabelMargin = new Thickness(
                    float.Parse(thicknessParts[0]),
                    float.Parse(thicknessParts[1]),
                    float.Parse(thicknessParts[2]),
                    float.Parse(thicknessParts[3]));

                PageCalendar2.FirstOfMonthLabelMargin = new Thickness(
                    float.Parse(thicknessParts[0]),
                    float.Parse(thicknessParts[1]),
                    float.Parse(thicknessParts[2]),
                    float.Parse(thicknessParts[3]));
            }
#endif
        }

        private void SetFirstOfYearDecadeLabelMargin_Click(object sender, RoutedEventArgs e)
        {
#if USE_INSIDER_SDK
            if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.TwentyOneH1))
            {
                string[] thicknessParts = firstOfYearDecadeLabelMargin.Text.Split(',');

                PageCalendar.FirstOfYearDecadeLabelMargin = new Thickness(
                    float.Parse(thicknessParts[0]),
                    float.Parse(thicknessParts[1]),
                    float.Parse(thicknessParts[2]),
                    float.Parse(thicknessParts[3]));

                PageCalendar2.FirstOfYearDecadeLabelMargin = new Thickness(
                    float.Parse(thicknessParts[0]),
                    float.Parse(thicknessParts[1]),
                    float.Parse(thicknessParts[2]),
                    float.Parse(thicknessParts[3]));
            }
#endif
        }

        private void GetDayItemFontSize_Click(object sender, RoutedEventArgs e)
        {
            dayItemFontSize.Text = PageCalendar.DayItemFontSize.ToString();
        }

        private void SetDayItemFontSize_Click(object sender, RoutedEventArgs e)
        {
            PageCalendar2.DayItemFontSize = PageCalendar.DayItemFontSize = double.Parse(dayItemFontSize.Text);
        }

        private void GetMonthYearItemFontSize_Click(object sender, RoutedEventArgs e)
        {
            monthYearItemFontSize.Text = PageCalendar.MonthYearItemFontSize.ToString();
        }

        private void SetMonthYearItemFontSize_Click(object sender, RoutedEventArgs e)
        {
            PageCalendar2.MonthYearItemFontSize = PageCalendar.MonthYearItemFontSize = double.Parse(monthYearItemFontSize.Text);
        }

        private void GetFirstOfMonthLabelFontSize_Click(object sender, RoutedEventArgs e)
        {
            firstOfMonthLabelFontSize.Text = PageCalendar.FirstOfMonthLabelFontSize.ToString();
        }

        private void SetFirstOfMonthLabelFontSize_Click(object sender, RoutedEventArgs e)
        {
            PageCalendar2.FirstOfMonthLabelFontSize = PageCalendar.FirstOfMonthLabelFontSize = double.Parse(firstOfMonthLabelFontSize.Text);
        }

        private void GetFirstOfYearDecadeLabelFontSize_Click(object sender, RoutedEventArgs e)
        {
            firstOfYearDecadeLabelFontSize.Text = PageCalendar.FirstOfYearDecadeLabelFontSize.ToString();
        }

        private void SetFirstOfYearDecadeLabelFontSize_Click(object sender, RoutedEventArgs e)
        {
            PageCalendar2.FirstOfYearDecadeLabelFontSize = PageCalendar.FirstOfYearDecadeLabelFontSize = double.Parse(firstOfYearDecadeLabelFontSize.Text);
        }

        private void SetCalendarItemCornerRadius_Click(object sender, RoutedEventArgs e)
        {
#if USE_INSIDER_SDK
            if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.TwentyOneH1))
            {
                PageCalendar2.CalendarItemCornerRadius = PageCalendar.CalendarItemCornerRadius = new CornerRadius(double.Parse(calendarItemCornerRadius.Text));
            }
#endif
        }

        private void ResetCalendarItemCornerRadius_Click(object sender, RoutedEventArgs e)
        {
#if USE_INSIDER_SDK
            if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.TwentyOneH1))
            {
                PageCalendar.ClearValue(CalendarView.CalendarItemCornerRadiusProperty);
                PageCalendar2.ClearValue(CalendarView.CalendarItemCornerRadiusProperty);
            }
#endif
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
                case 4: // BorderBrush
                    return PageCalendar.BorderBrush;
                case 5: // CalendarItemBorderBrush
                    return PageCalendar.CalendarItemBorderBrush;
                case 6: // CalendarItemBackground
                    return PageCalendar.CalendarItemBackground;
                case 8: // CalendarItemForeground
                    return PageCalendar.CalendarItemForeground;
                case 12: // Foreground
                    return PageCalendar.Foreground;
                case 13: // HoverBorderBrush
                    return PageCalendar.HoverBorderBrush;
                case 14: // OutOfScopeBackground
                    return PageCalendar.OutOfScopeBackground;
                case 15: // OutOfScopeForeground
                    return PageCalendar.OutOfScopeForeground;
                case 18: // PressedBorderBrush
                    return PageCalendar.PressedBorderBrush;
                case 19: // PressedForeground
                    return PageCalendar.PressedForeground;
                case 20: // SelectedBorderBrush
                    return PageCalendar.SelectedBorderBrush;
                case 23: // SelectedForeground
                    return PageCalendar.SelectedForeground;
                case 24: // SelectedHoverBorderBrush
                    return PageCalendar.SelectedHoverBorderBrush;
                case 26: // SelectedPressedBorderBrush
                    return PageCalendar.SelectedPressedBorderBrush;
                case 32: // TodayForeground
                    return PageCalendar.TodayForeground;
                case 36: // CalendarViewDayItem.Background
                    return null;
                default:
#if USE_INSIDER_SDK
                    if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.TwentyOneH1))
                    {
                        switch (brushPropertyName.SelectedIndex)
                        {
                            case 1: // BlackoutBackground
                                return PageCalendar.BlackoutBackground;
                            case 3: // BlackoutStrikethroughBrush
                                return PageCalendar.BlackoutStrikethroughBrush;
                            case 4: // CalendarItemDisabledBackground
                                return PageCalendar.CalendarItemDisabledBackground;
                            case 9: // CalendarItemHoverBackground
                                return PageCalendar.CalendarItemHoverBackground;
                            case 10: // CalendarItemPressedBackground
                                return PageCalendar.CalendarItemPressedBackground;
                            case 11: // DisabledForeground
                                return PageCalendar.DisabledForeground;
                            case 16: // OutOfScopeHoverForeground
                                return PageCalendar.OutOfScopeHoverForeground;
                            case 17: // OutOfScopePressedForeground
                                return PageCalendar.OutOfScopePressedForeground;
                            case 21: // SelectedDisabledBorderBrush
                                return PageCalendar.SelectedDisabledBorderBrush;
                            case 22: // SelectedDisabledForeground
                                return PageCalendar.SelectedDisabledForeground;
                            case 25: // SelectedHoverForeground
                                return PageCalendar.SelectedHoverForeground;
                            case 27: // SelectedPressedForeground
                                return PageCalendar.SelectedPressedForeground;
                            case 28: // TodayBackground
                                return PageCalendar.TodayBackground;
                            case 29: // TodayBlackoutBackground
                                return PageCalendar.TodayBlackoutBackground;
                            case 30: // TodayBlackoutForeground
                                return PageCalendar.TodayBlackoutForeground;
                            case 31: // TodayDisabledBackground
                                return PageCalendar.TodayDisabledBackground;
                            case 33: // TodayHoverBackground
                                return PageCalendar.TodayHoverBackground;
                            case 34: // TodayPressedBackground
                                return PageCalendar.TodayPressedBackground;
                            case 35: // TodaySelectedInnerBorderBrush
                                return PageCalendar.TodaySelectedInnerBorderBrush;
                            default:
                                return null;
                        }
                    }
#endif // USE_INSIDER_SDK
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

            if (solidColorBrush == null && brushPropertyName.SelectedIndex != 36)
            {
                return;
            }

            switch (brushPropertyName.SelectedIndex)
            {
                case 0: // Background
                    PageCalendar.Background = PageCalendar2.Background = solidColorBrush;
                    break;
                case 2: // BlackoutForeground
                    PageCalendar.BlackoutForeground = PageCalendar2.BlackoutForeground = solidColorBrush;
                    break;
                case 4: // BorderBrush
                    PageCalendar.BorderBrush = PageCalendar2.BorderBrush = solidColorBrush;
                    break;
                case 5: // CalendarItemBorderBrush
                    PageCalendar.CalendarItemBorderBrush = PageCalendar2.CalendarItemBorderBrush = solidColorBrush;
                    break;
                case 6: // CalendarItemBackground
                    PageCalendar.CalendarItemBackground = PageCalendar2.CalendarItemBackground = solidColorBrush;
                    break;
                case 8: // CalendarItemForeground
                    PageCalendar.CalendarItemForeground = PageCalendar2.CalendarItemForeground = solidColorBrush;
                    break;
                case 12: // Foreground
                    PageCalendar.Foreground = PageCalendar2.Foreground = solidColorBrush;
                    break;
                case 13: // HoverBorderBrush
                    PageCalendar.HoverBorderBrush = PageCalendar2.HoverBorderBrush = solidColorBrush;
                    break;
                case 14: // OutOfScopeBackground
                    PageCalendar.OutOfScopeBackground = PageCalendar2.OutOfScopeBackground = solidColorBrush;
                    break;
                case 15: // OutOfScopeForeground
                    PageCalendar.OutOfScopeForeground = PageCalendar2.OutOfScopeForeground = solidColorBrush;
                    break;
                case 18: // PressedBorderBrush
                    PageCalendar.PressedBorderBrush = PageCalendar2.PressedBorderBrush = solidColorBrush;
                    break;
                case 19: // PressedForeground
                    PageCalendar.PressedForeground = PageCalendar2.PressedForeground = solidColorBrush;
                    break;
                case 20: // SelectedBorderBrush
                    PageCalendar.SelectedBorderBrush = PageCalendar2.SelectedBorderBrush = solidColorBrush;
                    break;
                case 23: // SelectedForeground
                    PageCalendar.SelectedForeground = PageCalendar2.SelectedForeground = solidColorBrush;
                    break;
                case 24: // SelectedHoverBorderBrush
                    PageCalendar.SelectedHoverBorderBrush = PageCalendar2.SelectedHoverBorderBrush = solidColorBrush;
                    break;
                case 26: // SelectedPressedBorderBrush
                    PageCalendar.SelectedPressedBorderBrush = PageCalendar2.SelectedPressedBorderBrush = solidColorBrush;
                    break;
                case 32: // TodayForeground
                    PageCalendar.TodayForeground = PageCalendar2.TodayForeground = solidColorBrush;
                    break;
                case 36: // CalendarViewDayItem.Background
                    SetBackgrounds(PageCalendar, solidColorBrush);
                    SetBackgrounds(PageCalendar2, solidColorBrush);
                    break;
#if USE_INSIDER_SDK
                default:
                    if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.TwentyOneH1))
                    {
                        switch (brushPropertyName.SelectedIndex)
                        {
                            case 1: // BlackoutBackground
                                PageCalendar.BlackoutBackground = PageCalendar2.BlackoutBackground = solidColorBrush;
                                break;
                            case 3: // BlackoutStrikethroughBrush
                                PageCalendar.BlackoutStrikethroughBrush = PageCalendar2.BlackoutStrikethroughBrush = solidColorBrush;
                                break;
                            case 7: // CalendarItemDisabledBackground
                                PageCalendar.CalendarItemDisabledBackground = PageCalendar2.CalendarItemDisabledBackground = solidColorBrush;
                                break;
                            case 9: // CalendarItemHoverBackground
                                PageCalendar.CalendarItemHoverBackground = PageCalendar2.CalendarItemHoverBackground = solidColorBrush;
                                break;
                            case 10: // CalendarItemPressedBackground
                                PageCalendar.CalendarItemPressedBackground = PageCalendar2.CalendarItemPressedBackground = solidColorBrush;
                                break;
                            case 11: // DisabledForeground
                                PageCalendar.DisabledForeground = PageCalendar2.DisabledForeground = solidColorBrush;
                                break;
                            case 16: // OutOfScopeHoverForeground
                                PageCalendar.OutOfScopeHoverForeground = PageCalendar2.OutOfScopeHoverForeground = solidColorBrush;
                                break;
                            case 17: // OutOfScopePressedForeground
                                PageCalendar.OutOfScopePressedForeground = PageCalendar2.OutOfScopePressedForeground = solidColorBrush;
                                break;
                            case 21: // SelectedDisabledBorderBrush
                                PageCalendar.SelectedDisabledBorderBrush = PageCalendar2.SelectedDisabledBorderBrush = solidColorBrush;
                                break;
                            case 22: // SelectedDisabledForeground
                                PageCalendar.SelectedDisabledForeground = PageCalendar2.SelectedDisabledForeground = solidColorBrush;
                                break;
                            case 25: // SelectedHoverForeground
                                PageCalendar.SelectedHoverForeground = PageCalendar2.SelectedHoverForeground = solidColorBrush;
                                break;
                            case 27: // SelectedPressedForeground
                                PageCalendar.SelectedPressedForeground = PageCalendar2.SelectedPressedForeground = solidColorBrush;
                                break;
                            case 28: // TodayBackground
                                PageCalendar.TodayBackground = PageCalendar2.TodayBackground = solidColorBrush;
                                break;
                            case 29: // TodayBlackoutBackground
                                PageCalendar.TodayBlackoutBackground = PageCalendar2.TodayBlackoutBackground = solidColorBrush;
                                break;
                            case 30: // TodayBlackoutForeground
                                PageCalendar.TodayBlackoutForeground = PageCalendar2.TodayBlackoutForeground = solidColorBrush;
                                break;
                            case 31: // TodayDisabledBackground
                                PageCalendar.TodayDisabledBackground = PageCalendar2.TodayDisabledBackground = solidColorBrush;
                                break;
                            case 33: // TodayHoverBackground
                                PageCalendar.TodayHoverBackground = PageCalendar2.TodayHoverBackground = solidColorBrush;
                                break;
                            case 34: // TodayPressedBackground
                                PageCalendar.TodayPressedBackground = PageCalendar2.TodayPressedBackground = solidColorBrush;
                                break;
                            case 35: // TodaySelectedInnerBorderBrush
                                PageCalendar.TodaySelectedInnerBorderBrush = PageCalendar2.TodaySelectedInnerBorderBrush = solidColorBrush;
                                break;
                        }
                    }
                    break;
#endif // USE_INSIDER_SDK
            }
        }

        private SolidColorBrush GetSolidColorBrush()
        {
            string defaultBrushColor = _defaultBrushColors[brushPropertyName.SelectedIndex];

            if (string.IsNullOrEmpty(defaultBrushColor))
            {
                Brush brush = GetBrushFromIndex();

                if (brush != null)
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

        private void SetBackgrounds(CalendarView cv, Brush brush)
        {
            if (cv == null) return;

            var dayItems = Utilities.TestUtilities.FindDescendents<CalendarViewDayItem>(cv);

            foreach (var dayItem in dayItems)
            {
                dayItem.Background = brush;
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
