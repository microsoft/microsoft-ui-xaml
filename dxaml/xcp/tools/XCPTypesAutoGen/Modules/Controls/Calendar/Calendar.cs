// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using XamlOM;

namespace Microsoft.UI.Xaml.Controls.Primitives
{
    [CodeGen(partial: true)]
    [ClassFlags(IsHiddenFromIdl = true)]
    [NativeName("CCalendarViewBaseItemChrome")]
    [TypeTable(IsExcludedFromDXaml = true)]
    [DXamlIdlGroup("Controls2")]
    [Guids(ClassGuid = "c1d6ef7b-c032-40e2-b4c9-1f4651aacc9c")]
    public abstract class CalendarViewBaseItem
     : Microsoft.UI.Xaml.Controls.Control
    {
        public CalendarViewBaseItem() { }
    }

    [FrameworkTypePattern]
    [CodeGen(partial: true)]
    [DXamlIdlGroup("Controls2")]
    [NativeName("CCalendarViewItemChrome")]
    [Guids(ClassGuid = "274b7c18-04f5-411a-8e73-25bea5e12a79")]
    internal class CalendarViewItem
     : CalendarViewBaseItem
    {
        [PropertyFlags(IsSetterImplVirtual = true)]
        public Windows.Foundation.DateTime Date
        {
            get;
            internal set;
        }
    }

    [FrameworkTypePattern]
    [DXamlIdlGroup("Controls2")]
    [Guids(ClassGuid = "e28504a1-3a09-4775-aff2-ac2b91ccbe4b")]
    public sealed class CalendarViewTemplateSettings
     : Microsoft.UI.Xaml.DependencyObject
    {

        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.Double MinViewWidth
        {
            get;
            internal set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.String HeaderText
        {
            get;
            internal set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.String WeekDay1
        {
            get;
            internal set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.String WeekDay2
        {
            get;
            internal set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.String WeekDay3
        {
            get;
            internal set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.String WeekDay4
        {
            get;
            internal set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.String WeekDay5
        {
            get;
            internal set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.String WeekDay6
        {
            get;
            internal set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.String WeekDay7
        {
            get;
            internal set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.Boolean HasMoreContentAfter
        {
            get;
            internal set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.Boolean HasMoreContentBefore
        {
            get;
            internal set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.Boolean HasMoreViews
        {
            get;
            internal set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.Rect ClipRect
        {
            get;
            internal set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.Double CenterX
        {
            get;
            internal set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.Double CenterY
        {
            get;
            internal set;
        }

        internal CalendarViewTemplateSettings() { }
    }

    [DXamlIdlGroup("Controls2")]
    [CodeGen(partial: true)]
    [FrameworkTypePattern]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.ILayoutStrategy))]
    [Guids(ClassGuid = "7dc253eb-4830-4093-bb3b-5aaaeb74fc39")]
    internal sealed class CalendarLayoutStrategy
    {

    }

}


namespace Microsoft.UI.Xaml.Controls
{
    [FrameworkTypePattern]
    [DXamlIdlGroup("Controls2")]
    [EnumFlags(HasTypeConverter = true, IsExcludedFromNative = true)]
    [TypeTable(IsExcludedFromCore = true)]
    public enum CalendarViewSelectionMode
    {
        [NativeValueName("CalendarViewSelectionModeNone")]
        None = 0,

        [NativeValueName("CalendarViewSelectionModeSingle")]
        Single = 1,

        [NativeValueName("CalendarViewSelectionModeMultiple")]
        Multiple = 2,
    }

    [FrameworkTypePattern]
    [DXamlIdlGroup("Controls2")]
    [EnumFlags(HasTypeConverter = true, IsExcludedFromNative = true)]
    [TypeTable(IsExcludedFromCore = true)]
    public enum CalendarViewDisplayMode
    {
        [NativeValueName("CalendarViewDisplayModeMonth")]
        Month = 0,

        [NativeValueName("CalendarViewDisplayModeYear")]
        Year = 1,

        [NativeValueName("CalendarViewDisplayModeDecade")]
        Decade = 2
    }

    [FrameworkTypePattern]
    [CodeGen(partial: true)]
    [DXamlIdlGroup("Controls2")]
    [NativeName("CCalendarViewDayItemChrome")]
    [Guids(ClassGuid = "9e3ea06e-20e3-4517-9207-99a9aad67f27")]
    public class CalendarViewDayItem
     : Microsoft.UI.Xaml.Controls.Primitives.CalendarViewBaseItem
    {

        public Windows.Foundation.Boolean IsBlackout
        {
            get;
            set;
        }

        [PropertyFlags(IsSetterImplVirtual = true)]
        public Windows.Foundation.DateTime Date
        {
            get;
            internal set;
        }

        public void SetDensityColors([Optional] Windows.Foundation.Collections.IIterable<Windows.UI.Color> colors)
        {
        }
    }

    [FrameworkTypePattern]
    [CodeGen(partial: true)]
    [DXamlIdlGroup("Controls2")]
    [InstanceCountTelemetry]
    [NativeName("CCalendarView")]
    [Guids(ClassGuid = "75e4fc5d-5fba-49e5-a2e5-6bf05427e18c")]
    public class CalendarView
     : Microsoft.UI.Xaml.Controls.Control
    {
        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        public event Microsoft.UI.Xaml.Controls.CalendarViewDayItemChangingEventHandler CalendarViewDayItemChanging;

        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        public event Microsoft.UI.Xaml.Controls.SelectedDatesChangedEventHandler SelectedDatesChanged;

        public void SetDisplayDate(Windows.Foundation.DateTime date)
        {

        }

        public Windows.Foundation.String CalendarIdentifier
        {
            get;
            set;
        }

        public Windows.Foundation.String DayOfWeekFormat
        {
            get;
            set;
        }

        public Windows.Foundation.Boolean IsGroupLabelVisible
        {
            get;
            set;
        }

        public CalendarViewDisplayMode DisplayMode
        {
            get;
            set;
        }

        public Windows.Globalization.DayOfWeek FirstDayOfWeek
        {
            get;
            set;
        }

        public Windows.Foundation.Boolean IsOutOfScopeEnabled
        {
            get;
            set;
        }

        public Windows.Foundation.Boolean IsTodayHighlighted
        {
            get;
            set;
        }

        public Windows.Foundation.DateTime MaxDate
        {
            get;
            set;
        }

        public Windows.Foundation.DateTime MinDate
        {
            get;
            set;
        }

        public Windows.Foundation.Int32 NumberOfWeeksInView
        {
            get;
            set;
        }

        public Windows.Foundation.Collections.IVector<Windows.Foundation.DateTime> SelectedDates
        {
            get;
            internal set;
        }

        public CalendarViewSelectionMode SelectionMode
        {
            get;
            set;
        }

        public Microsoft.UI.Xaml.Controls.Primitives.CalendarViewTemplateSettings TemplateSettings
        {
            get;
            internal set;
        }

        public void SetYearDecadeDisplayDimensions(Windows.Foundation.Int32 columns, Windows.Foundation.Int32 rows)
        {
        }

        #region BorderBrush

        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pFocusBorderBrush")]
        public Microsoft.UI.Xaml.Media.Brush FocusBorderBrush
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pSelectedHoverBorderBrush")]
        public Microsoft.UI.Xaml.Media.Brush SelectedHoverBorderBrush
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pSelectedPressedBorderBrush")]
        public Microsoft.UI.Xaml.Media.Brush SelectedPressedBorderBrush
        {
            get;
            set;
        }

        [Comment("Brush for a selected disabled item border")]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pSelectedDisabledBorderBrush")]
        public Microsoft.UI.Xaml.Media.Brush SelectedDisabledBorderBrush
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pSelectedBorderBrush")]
        public Microsoft.UI.Xaml.Media.Brush SelectedBorderBrush
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pHoverBorderBrush")]
        public Microsoft.UI.Xaml.Media.Brush HoverBorderBrush
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pPressedBorderBrush")]
        public Microsoft.UI.Xaml.Media.Brush PressedBorderBrush
        {
            get;
            set;
        }

        [Comment("Brush for the current selected item border")]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pTodaySelectedInnerBorderBrush")]
        public Microsoft.UI.Xaml.Media.Brush TodaySelectedInnerBorderBrush
        {
            get;
            set;
        }

        [Comment("Brush for a blacked out item's diagonal strikethrough line")]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pBlackoutStrikethroughBrush")]
        public Microsoft.UI.Xaml.Media.Brush BlackoutStrikethroughBrush
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pCalendarItemBorderBrush")]
        public Microsoft.UI.Xaml.Media.Brush CalendarItemBorderBrush
        {
            get;
            set;
        }

        #endregion

        #region BackgroundBrush

        [Comment("Brush for a blacked out item background")]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pBlackoutBackground")]
        public Microsoft.UI.Xaml.Media.Brush BlackoutBackground
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pOutOfScopeBackground")]
        public Microsoft.UI.Xaml.Media.Brush OutOfScopeBackground
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pCalendarItemBackground")]
        public Microsoft.UI.Xaml.Media.Brush CalendarItemBackground
        {
            get;
            set;
        }

        [Comment("Brush for the pointer-over item background")]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pCalendarItemHoverBackground")]
        public Microsoft.UI.Xaml.Media.Brush CalendarItemHoverBackground
        {
            get;
            set;
        }

        [Comment("Brush for the pressed item background")]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pCalendarItemPressedBackground")]
        public Microsoft.UI.Xaml.Media.Brush CalendarItemPressedBackground
        {
            get;
            set;
        }

        [Comment("Brush for a disabled item background")]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pCalendarItemDisabledBackground")]
        public Microsoft.UI.Xaml.Media.Brush CalendarItemDisabledBackground
        {
            get;
            set;
        }

        [Comment("Brush for the current item background")]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pTodayBackground")]
        public Microsoft.UI.Xaml.Media.Brush TodayBackground
        {
            get;
            set;
        }

        [Comment("Brush for the current item background when blacked out")]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pTodayBlackoutBackground")]
        public Microsoft.UI.Xaml.Media.Brush TodayBlackoutBackground
        {
            get;
            set;
        }

        [Comment("Brush for the current pointer-over item background")]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pTodayHoverBackground")]
        public Microsoft.UI.Xaml.Media.Brush TodayHoverBackground
        {
            get;
            set;
        }

        [Comment("Brush for the current pressed item background")]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pTodayPressedBackground")]
        public Microsoft.UI.Xaml.Media.Brush TodayPressedBackground
        {
            get;
            set;
        }

        [Comment("Brush for the current disabled item background")]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pTodayDisabledBackground")]
        public Microsoft.UI.Xaml.Media.Brush TodayDisabledBackground
        {
            get;
            set;
        }

        #endregion

        #region Foreground Brush

        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pPressedForeground")]
        public Microsoft.UI.Xaml.Media.Brush PressedForeground
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pTodayForeground")]
        public Microsoft.UI.Xaml.Media.Brush TodayForeground
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pBlackoutForeground")]
        public Microsoft.UI.Xaml.Media.Brush BlackoutForeground
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pTodayBlackoutForeground")]
        public Microsoft.UI.Xaml.Media.Brush TodayBlackoutForeground
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pSelectedForeground")]
        public Microsoft.UI.Xaml.Media.Brush SelectedForeground
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pSelectedHoverForeground")]
        public Microsoft.UI.Xaml.Media.Brush SelectedHoverForeground
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pSelectedPressedForeground")]
        public Microsoft.UI.Xaml.Media.Brush SelectedPressedForeground
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pSelectedDisabledForeground")]
        public Microsoft.UI.Xaml.Media.Brush SelectedDisabledForeground
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pOutOfScopeForeground")]
        public Microsoft.UI.Xaml.Media.Brush OutOfScopeForeground
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pOutOfScopeHoverForeground")]
        public Microsoft.UI.Xaml.Media.Brush OutOfScopeHoverForeground
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pOutOfScopePressedForeground")]
        public Microsoft.UI.Xaml.Media.Brush OutOfScopePressedForeground
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pCalendarItemForeground")]
        public Microsoft.UI.Xaml.Media.Brush CalendarItemForeground
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pDisabledForeground")]
        public Microsoft.UI.Xaml.Media.Brush DisabledForeground
        {
            get;
            set;
        }

        #endregion

        #region DayItem Font
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pDayItemFontFamily")]
        public Microsoft.UI.Xaml.Media.FontFamily DayItemFontFamily
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_dayItemFontSize")]
        public Windows.Foundation.Double DayItemFontSize
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_dayItemFontStyle")]
        public Windows.UI.Text.FontStyle DayItemFontStyle
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_dayItemFontWeight")]
        public Windows.UI.Text.FontWeight DayItemFontWeight
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_todayFontWeight")]
        public Windows.UI.Text.FontWeight TodayFontWeight
        {
            get;
            set;
        }
        #endregion

        #region DayItemLabel Font
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pFirstOfMonthLabelFontFamily")]
        public Microsoft.UI.Xaml.Media.FontFamily FirstOfMonthLabelFontFamily
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_firstOfMonthLabelFontSize")]
        public Windows.Foundation.Double FirstOfMonthLabelFontSize
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_firstOfMonthLabelFontStyle")]
        public Windows.UI.Text.FontStyle FirstOfMonthLabelFontStyle
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_firstOfMonthLabelFontWeight")]
        public Windows.UI.Text.FontWeight FirstOfMonthLabelFontWeight
        {
            get;
            set;
        }

        #endregion

        #region MonthYearItem Font

        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pMonthYearItemFontFamily")]
        public Microsoft.UI.Xaml.Media.FontFamily MonthYearItemFontFamily
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_monthYearItemFontSize")]
        public Windows.Foundation.Double MonthYearItemFontSize
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_monthYearItemFontStyle")]
        public Windows.UI.Text.FontStyle MonthYearItemFontStyle
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_monthYearItemFontWeight")]
        public Windows.UI.Text.FontWeight MonthYearItemFontWeight
        {
            get;
            set;
        }

        #endregion

        #region MonthItemLabel Font

        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pFirstOfYearDecadeLabelFontFamily")]
        public Microsoft.UI.Xaml.Media.FontFamily FirstOfYearDecadeLabelFontFamily
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_firstOfYearDecadeLabelFontSize")]
        public Windows.Foundation.Double FirstOfYearDecadeLabelFontSize
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_firstOfYearDecadeLabelFontStyle")]
        public Windows.UI.Text.FontStyle FirstOfYearDecadeLabelFontStyle
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_firstOfYearDecadeLabelFontWeight")]
        public Windows.UI.Text.FontWeight FirstOfYearDecadeLabelFontWeight
        {
            get;
            set;
        }

        #endregion

        #region Margin

        [Comment("Day item margin.")]
        [NativeStorageType(ValueType.valueThickness)]
        [OffsetFieldName("m_dayItemMargin")]
        public Microsoft.UI.Xaml.Thickness DayItemMargin
        {
            get;
            set;
        }

        [Comment("Month/year item margin.")]
        [NativeStorageType(ValueType.valueThickness)]
        [OffsetFieldName("m_monthYearItemMargin")]
        public Microsoft.UI.Xaml.Thickness MonthYearItemMargin
        {
            get;
            set;
        }

        [Comment("First-of-month label margin.")]
        [NativeStorageType(ValueType.valueThickness)]
        [OffsetFieldName("m_firstOfMonthLabelMargin")]
        public Microsoft.UI.Xaml.Thickness FirstOfMonthLabelMargin
        {
            get;
            set;
        }

        [Comment("First-of-year label margin.")]
        [NativeStorageType(ValueType.valueThickness)]
        [OffsetFieldName("m_firstOfYearDecadeLabelMargin")]
        public Microsoft.UI.Xaml.Thickness FirstOfYearDecadeLabelMargin
        {
            get;
            set;
        }

        #endregion

        #region Alignment
        [Comment("Only dayitem and dayitem label have alignment properties.")]

        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_horizontalDayItemAlignment")]
        public Microsoft.UI.Xaml.HorizontalAlignment HorizontalDayItemAlignment
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_verticalDayItemAlignment")]
        public Microsoft.UI.Xaml.VerticalAlignment VerticalDayItemAlignment
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_horizontalFirstOfMonthLabelAlignment")]
        public Microsoft.UI.Xaml.HorizontalAlignment HorizontalFirstOfMonthLabelAlignment
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_verticalFirstOfMonthLabelAlignment")]
        public Microsoft.UI.Xaml.VerticalAlignment VerticalFirstOfMonthLabelAlignment
        {
            get;
            set;
        }

        #endregion

        [NativeStorageType(ValueType.valueThickness)]
        [OffsetFieldName("m_calendarItemBorderThickness")]
        public Microsoft.UI.Xaml.Thickness CalendarItemBorderThickness
        {
            get;
            set;
        }

        public Microsoft.UI.Xaml.Style CalendarViewDayItemStyle
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueCornerRadius)]
        [OffsetFieldName("m_calendarItemCornerRadius")]
        public Microsoft.UI.Xaml.CornerRadius CalendarItemCornerRadius
        {
            get;
            set;
        }

        #region Internal brushes

        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pTodayHoverBorderBrush")]
        internal Microsoft.UI.Xaml.Media.Brush TodayHoverBorderBrush
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pTodayPressedBorderBrush")]
        internal Microsoft.UI.Xaml.Media.Brush TodayPressedBorderBrush
        {
            get;
            set;
        }

        #endregion 
    }

    [DXamlIdlGroup("Controls2")]
    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "e31270da-dc57-4a4e-99bd-111a1b7d4755")]
    public sealed class CalendarViewDayItemChangingEventArgs
     : Microsoft.UI.Xaml.EventArgs
    {
        internal CalendarViewDayItemChangingEventArgs()
        {
        }

        public Windows.Foundation.Boolean InRecycleQueue
        {
            get;
            internal set;
        }

        public Microsoft.UI.Xaml.Controls.CalendarViewDayItem Item
        {
            get;
            internal set;
        }

        public uint Phase
        {
            get;
            internal set;
        }

        internal bool WantsCallBack
        {
            get;
            set;
        }

        [DXamlName("RegisterUpdateCallback")]
        [DXamlOverloadName("RegisterUpdateCallback")]
        public void RegisterUpdateCallback(Windows.Foundation.TypedEventHandler<CalendarView, CalendarViewDayItemChangingEventArgs> callback)
        {
        }

        [DXamlName("RegisterUpdateCallbackWithPhase")]
        [DXamlOverloadName("RegisterUpdateCallback")]
        public void RegisterUpdateCallback(uint callbackPhase, Windows.Foundation.TypedEventHandler<CalendarView, CalendarViewDayItemChangingEventArgs> callback)
        {
        }

        internal Windows.Foundation.TypedEventHandler<CalendarView, CalendarViewDayItemChangingEventArgs> Callback
        {
            get;
            set;
        }

        internal void ResetLifetime() { }
    }

    [DXamlIdlGroup("Controls2")]
    public delegate void CalendarViewDayItemChangingEventHandler(CalendarView sender, Microsoft.UI.Xaml.Controls.CalendarViewDayItemChangingEventArgs e);

    [DXamlIdlGroup("Controls2")]
    [TypeFlags(IsCreateableFromXAML = false)]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "bf7ba845-6228-4f2e-a0f9-88445aa97440")]
    public sealed class CalendarViewSelectedDatesChangedEventArgs

     : Microsoft.UI.Xaml.EventArgs
    {
        internal CalendarViewSelectedDatesChangedEventArgs()
        {
        }

        public Windows.Foundation.Collections.IVectorView<Windows.Foundation.DateTime> AddedDates
        {
            get;
            internal set;
        }

        public Windows.Foundation.Collections.IVectorView<Windows.Foundation.DateTime> RemovedDates
        {
            get;
            internal set;
        }
    }

    [DXamlIdlGroup("Controls2")]
    [StubDelegate]
    [CodeGen(CodeGenLevel.LookupOnly)]
    public delegate void SelectedDatesChangedEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Controls.CalendarViewSelectedDatesChangedEventArgs e);

    [DXamlIdlGroup("Controls2")]
    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "4f2816e4-7628-44bc-b359-9b8cd4a469f4")]
    public sealed class CalendarDatePickerDateChangedEventArgs
     : Microsoft.UI.Xaml.EventArgs
    {
        internal CalendarDatePickerDateChangedEventArgs()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.DateTime? NewDate
        {
            get;
            internal set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.DateTime? OldDate
        {
            get;
            internal set;
        }
    }

    [FrameworkTypePattern]
    [CodeGen(partial: true)]
    [InstanceCountTelemetry]
    [Platform("Feature_HeaderPlacement", typeof(Microsoft.UI.Xaml.WinUIContract), 1)]
    [DXamlIdlGroup("Controls2")]
    [Guids(ClassGuid = "21fc60de-a985-4a7d-a422-cbe43b1627ec")]
    public class CalendarDatePicker
     : Microsoft.UI.Xaml.Controls.Control
    {
        public event Microsoft.UI.Xaml.Controls.CalendarViewDayItemChangingEventHandler CalendarViewDayItemChanging;
        public event Windows.Foundation.TypedEventHandler<CalendarDatePicker, CalendarDatePickerDateChangedEventArgs>  DateChanged;

        [EventHandlerType(EventHandlerKind.TypedArgs)]
        public event Microsoft.UI.Xaml.EventHandler Opened;

        [EventHandlerType(EventHandlerKind.TypedArgs)]
        public event Microsoft.UI.Xaml.EventHandler Closed;

        public void SetDisplayDate(Windows.Foundation.DateTime date)
        {
        }

        public void SetYearDecadeDisplayDimensions(Windows.Foundation.Int32 columns, Windows.Foundation.Int32 rows)
        {
        }

        #region Properties owned by CalendarDatePicker only

        public Windows.Foundation.DateTime? Date { get; set; }

        public Windows.Foundation.Boolean IsCalendarOpen { get; set; }

        public Windows.Foundation.String DateFormat { get; set; }

        public Windows.Foundation.String PlaceholderText { get; set; }

        [VelocityFeature("Feature_HeaderPlacement")]
        [NativeStorageType(ValueType.valueEnum)]
        public Microsoft.UI.Xaml.Controls.ControlHeaderPlacement HeaderPlacement { get; set; }

        public Windows.Foundation.Object Header { get; set; }

        public Microsoft.UI.Xaml.DataTemplate HeaderTemplate { get; set; }

        public Microsoft.UI.Xaml.Style CalendarViewStyle { get; set; }

        public Microsoft.UI.Xaml.Controls.LightDismissOverlayMode LightDismissOverlayMode { get; set; }

        public object Description { get; set; }

        #endregion

        #region Properties to be bound by CalendarView

        public Windows.Foundation.DateTime MinDate { get; set; }

        public Windows.Foundation.DateTime MaxDate { get; set; }

        public Windows.Foundation.Boolean IsTodayHighlighted { get; set; }

        public CalendarViewDisplayMode DisplayMode { get; set; }

        public Windows.Globalization.DayOfWeek FirstDayOfWeek { get; set; }

        public Windows.Foundation.String DayOfWeekFormat { get; set; }

        public Windows.Foundation.String CalendarIdentifier { get; set; }

        public Windows.Foundation.Boolean IsOutOfScopeEnabled { get; set; }

        public Windows.Foundation.Boolean IsGroupLabelVisible { get; set; }

        #endregion
    }
}

namespace Microsoft.UI.Xaml.Automation.Peers
{
    [Platform(typeof(Microsoft.UI.Xaml.WinUIContract), 1)]
    [DXamlIdlGroup("Controls2")]
    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(CanConvertFromString = true)]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.IInvokeProvider))]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.IValueProvider))]
    [Guids(ClassGuid = "2894679d-e8bd-485c-9ecf-63eef126aee6")]
    public class CalendarDatePickerAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithOwner")]
        public CalendarDatePickerAutomationPeer(Microsoft.UI.Xaml.Controls.CalendarDatePicker owner)
            : base(owner) { }
    }
}
