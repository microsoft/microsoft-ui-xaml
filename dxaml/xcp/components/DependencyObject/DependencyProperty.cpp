// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <MetadataAPI.h>
#include <CDependencyObject.h>
#include <UIElement.h>
#include <TypeTableStructs.h>
#include <CustomDependencyProperty.h>
#include <InheritedProperties.h>
#include <ContentPresenter.h>
#include <XboxUtility.h>
#include <RuntimeEnabledFeatures.h>
#include <ListViewBaseItemChrome.h>

namespace DirectUI
{
    extern double GetModernDoubleValue(_In_ const int nValue);
}

// Default thicknesses for FrameworkElement's FocusVisualPrimaryThickness
// and FocusVisualSecondaryThickness properties.
const XTHICKNESS DefaultFocusVisualPrimaryThickness = { 2, 2, 2, 2 };
const XTHICKNESS DefaultFocusVisualSecondaryThickness = { 1, 1, 1, 1 };

extern UINT32 GetDefaultSelectionHighlightColor();

using namespace DirectUI;

static const XPOINTF s_gradientEndPoint = { 1.0f, 1.0f };

DECLARE_CONST_XSTRING_PTR_STORAGE(c_strGregorianCalendarStorage, L"GregorianCalendar");

static _Check_return_ HRESULT CreateDefaultGregorianCalendar(
    _Out_ wg::ICalendar** ppCalendar)
{
    ctl::ComPtr<wg::ICalendar> spCalendar;
    ctl::ComPtr<wg::ICalendarFactory> spCalendarFactory;
    ctl::ComPtr<wg::IApplicationLanguagesStatics> spApplicationLanguagesStatics;
    ctl::ComPtr<wfc::IVectorView<HSTRING>> spLanguagesAsIVV;
    ctl::ComPtr<wfc::IIterable<HSTRING>> spLanguagesAsII;
    wrl_wrappers::HStringReference strClock(L"24HourClock");    // it doesn't matter if it is 24 or 12 hour clock
    const xstring_ptr c_strGregorianCalendar = XSTRING_PTR_FROM_STORAGE(c_strGregorianCalendarStorage);
    wrl_wrappers::HStringReference strCalendarIdentifier(c_strGregorianCalendar.GetBuffer());

    // Create the languages
    IFC_RETURN(ctl::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Windows_Globalization_ApplicationLanguages).Get(),
        &spApplicationLanguagesStatics));

    IFC_RETURN(spApplicationLanguagesStatics->get_Languages(&spLanguagesAsIVV));
    IFC_RETURN(spLanguagesAsIVV.As(&spLanguagesAsII));

    //Create the calendar
    IFC_RETURN(ctl::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Windows_Globalization_Calendar).Get(),
        &spCalendarFactory));

    IFC_RETURN(spCalendarFactory->CreateCalendar(
        spLanguagesAsII.Get(),
        strCalendarIdentifier.Get(),
        strClock.Get(),
        spCalendar.ReleaseAndGetAddressOf()));

    IFC_RETURN(spCalendar.CopyTo(ppCalendar));

    return S_OK;
}

_Check_return_ HRESULT
CDependencyProperty::GetDefaultValue(
    _In_ CCoreServices* core,
    _In_opt_ CDependencyObject* referenceObject,
    _In_ const CClassInfo* type,
    _Out_ CValue* defaultValue) const
{
    const float SLIDER_DEFAULT_STEP_FREQUENCY = 1;
    const float SLIDER_DEFAULT_TICK_FREQUENCY = 0;
    const xaml_controls::Orientation SLIDER_DEFAULT_ORIENTATION = xaml_controls::Orientation_Horizontal;
    const xaml_primitives::SliderSnapsTo SLIDER_DEFAULT_SNAPS_TO = xaml_primitives::SliderSnapsTo_StepValues;
    const xaml_primitives::TickPlacement SLIDER_DEFAULT_TICK_PLACEMENT = xaml_primitives::TickPlacement_Inline;
    const float POP_IN_THEME_ANIMATION_OFFSET = 40; // Magnitude of the pop-in translation.
    DECLARE_CONST_XSTRING_PTR_STORAGE(c_strYearFormat, L"year.full");
    DECLARE_CONST_XSTRING_PTR_STORAGE(c_strMonthFormat, L"{month.full}");
    DECLARE_CONST_XSTRING_PTR_STORAGE(c_strDayFormat, L"day");

    const auto typeIndex = GetPropertyTypeIndex();
    IFCPTR_RETURN(defaultValue);

    if (auto customDependencyProperty = AsOrNull<CCustomDependencyProperty>())
    {
        ctl::ComPtr<IInspectable> spDefaultValue;
        IFC_RETURN(customDependencyProperty->GetDefaultValue(&spDefaultValue));
        if (spDefaultValue != nullptr)
        {
            defaultValue->SetIInspectableNoRef(spDefaultValue.Detach());
            return S_OK;
        }
    }

    if (IsInheritedAttachedPropertyInStorageGroup())
    {
        IFC_RETURN(InheritedProperties::GetDefaultValue(m_nIndex, defaultValue));
        return S_OK;
    }

    if (IsInherited())
    {
        IFC_RETURN(GetDefaultInheritedPropertyValue(core, defaultValue));
        return S_OK;
    }

    IFC_RETURN(ValidateType(type));

    // Native objects almost always have default values of 0. This is because the
    // default values are registered on creation of the first object of a given
    // class *BEFORE* any values can be stored. Deal with the few exceptions here

    switch (m_nIndex)
    {
        // Set of properties that depend on the framework peer to provide the default value.
    case KnownPropertyIndex::DatePicker_MinYear:
    case KnownPropertyIndex::DatePicker_MaxYear:
    case KnownPropertyIndex::DatePicker_Date:
    case KnownPropertyIndex::TimePicker_ClockIdentifier:
    case KnownPropertyIndex::TimePicker_Time:
    case KnownPropertyIndex::RangeBase_Maximum:
    case KnownPropertyIndex::RangeBase_LargeChange:
    case KnownPropertyIndex::RangeBase_SmallChange:
    case KnownPropertyIndex::CalendarView_CalendarIdentifier:
    case KnownPropertyIndex::CalendarView_NumberOfWeeksInView:
    case KnownPropertyIndex::Hub_SectionHeaders:
    case KnownPropertyIndex::ToggleSwitch_OnContent:
    case KnownPropertyIndex::ToggleSwitch_OffContent:
    {
        IFC_RETURN(GetDefaultValueFromPeer(referenceObject, defaultValue));
        return S_OK;
    }

        // Regular default values.
    case KnownPropertyIndex::CompositeTransform_ScaleX:
    case KnownPropertyIndex::CompositeTransform_ScaleY:
    case KnownPropertyIndex::CompositeTransform3D_ScaleX:
    case KnownPropertyIndex::CompositeTransform3D_ScaleY:
    case KnownPropertyIndex::CompositeTransform3D_ScaleZ:
    case KnownPropertyIndex::ScaleTransform_ScaleX:
    case KnownPropertyIndex::ScaleTransform_ScaleY:
    case KnownPropertyIndex::TransitionTarget_Opacity:
    case KnownPropertyIndex::UIElement_Opacity:
    case KnownPropertyIndex::Shape_StrokeThickness:
    case KnownPropertyIndex::Brush_Opacity:
    case KnownPropertyIndex::BackEase_Amplitude:
    case KnownPropertyIndex::Timeline_SpeedRatio:
    case KnownPropertyIndex::ScrollViewer_ZoomFactor:
        defaultValue->SetFloat(1.0f);
        return S_OK;
    
    case KnownPropertyIndex::GridViewItemPresenter_DisabledOpacity:
    case KnownPropertyIndex::ListViewItemPresenter_DisabledOpacity:
    case KnownPropertyIndex::GridViewItemPresenter_SelectedBorderThickness:
    case KnownPropertyIndex::ListViewItemPresenter_SelectedBorderThickness:
    case KnownPropertyIndex::ListViewItemPresenter_SelectionIndicatorVisualEnabled:
    {
        const auto runtimeEnabledFeatureDetector = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector();
        const bool denyRoundedListViewBaseItemChrome = runtimeEnabledFeatureDetector->IsFeatureEnabled(RuntimeFeatureBehavior::RuntimeEnabledFeature::DenyRoundedListViewBaseItemChrome);
        const bool forceRoundedListViewBaseItemChrome = runtimeEnabledFeatureDetector->IsFeatureEnabled(RuntimeFeatureBehavior::RuntimeEnabledFeature::ForceRoundedListViewBaseItemChrome);

        bool forRoundedListViewBaseItemChrome = false;

        if (!denyRoundedListViewBaseItemChrome && !forceRoundedListViewBaseItemChrome)
        {
            DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_listViewBaseItemRoundedChromeEnabled, L"ListViewBaseItemRoundedChromeEnabled");

            IFC_RETURN(CDependencyProperty::GetBooleanThemeResourceValue(core, c_listViewBaseItemRoundedChromeEnabled, &forRoundedListViewBaseItemChrome));
        }
        else if (!denyRoundedListViewBaseItemChrome)
        {
            forRoundedListViewBaseItemChrome = true;
        }

        if (m_nIndex == KnownPropertyIndex::ListViewItemPresenter_SelectionIndicatorVisualEnabled)
        {
            defaultValue->SetBool(forRoundedListViewBaseItemChrome);
        }
        else if (m_nIndex == KnownPropertyIndex::GridViewItemPresenter_DisabledOpacity || m_nIndex == KnownPropertyIndex::ListViewItemPresenter_DisabledOpacity)
        {
            defaultValue->SetFloat(CListViewBaseItemChrome::GetDefaultDisabledOpacity(forRoundedListViewBaseItemChrome));
        }
        else
        {
            defaultValue->WrapThickness(CListViewBaseItemChrome::GetSelectedBorderXThickness(forRoundedListViewBaseItemChrome));
        }
        return S_OK;
    }

    case KnownPropertyIndex::ListViewItemPresenter_DragOpacity:
    case KnownPropertyIndex::GridViewItemPresenter_DragOpacity:
        defaultValue->SetFloat(CListViewBaseItemChrome::GetDefaultDragOpacity());
        return S_OK;

    case KnownPropertyIndex::ListViewItemPresenter_ReorderHintOffset:
        defaultValue->SetFloat(CListViewBaseItemChrome::GetDefaultListViewItemReorderHintOffset());
        return S_OK;

    case KnownPropertyIndex::GridViewItemPresenter_ReorderHintOffset:
        defaultValue->SetFloat(CListViewBaseItemChrome::GetDefaultGridViewItemReorderHintOffset());
        return S_OK;

    case KnownPropertyIndex::ScrollViewer_MinZoomFactor:
        defaultValue->SetFloat(0.1f);
        return S_OK;

    case KnownPropertyIndex::Selector_SelectedIndex:
    case KnownPropertyIndex::WrapGrid_MaximumRowsOrColumns:
    case KnownPropertyIndex::VariableSizedWrapGrid_MaximumRowsOrColumns:
    case KnownPropertyIndex::ItemsWrapGrid_MaximumRowsOrColumns:
    case KnownPropertyIndex::Hub_DefaultSectionIndex:
        defaultValue->SetSigned(-1);
        return S_OK;

    case KnownPropertyIndex::ScrollViewer_MaxZoomFactor:
        defaultValue->SetFloat(10.0f);
        return S_OK;

    case KnownPropertyIndex::ItemsWrapGrid_CacheLength:
    case KnownPropertyIndex::ItemsStackPanel_CacheLength:
        defaultValue->SetFloat(static_cast<XFLOAT>(DirectUI::GetModernDoubleValue(0)));
        return S_OK;

    case KnownPropertyIndex::CalendarView_MinDate:
    case KnownPropertyIndex::CalendarView_MaxDate:
    case KnownPropertyIndex::CalendarDatePicker_MinDate:
    case KnownPropertyIndex::CalendarDatePicker_MaxDate:
    {
        static wf::DateTime minDate;
        static wf::DateTime maxDate;

        if (minDate.UniversalTime == 0 && maxDate.UniversalTime == 0)
        {
            ctl::ComPtr<wg::ICalendar> spCalendar;

            int firstDayOfMonth = 0;
            int lastDayOfMonth = 0;
            int firstMonthOfYear = 0;
            int lastMonthOfYear = 0;

            IFC_RETURN(CreateDefaultGregorianCalendar(&spCalendar));

            IFC_RETURN(spCalendar->SetToNow());

            // default mindate is January 1st, 100 years ago
            IFC_RETURN(spCalendar->AddYears(-100));
            IFC_RETURN(spCalendar->get_FirstMonthInThisYear(&firstMonthOfYear));
            IFC_RETURN(spCalendar->put_Month(firstMonthOfYear));
            IFC_RETURN(spCalendar->get_FirstDayInThisMonth(&firstDayOfMonth));
            IFC_RETURN(spCalendar->put_Day(firstDayOfMonth));
            IFC_RETURN(spCalendar->GetDateTime(&minDate));

            // default maxdate is December 31st 100 years in the future
            IFC_RETURN(spCalendar->AddYears(200));
            IFC_RETURN(spCalendar->get_LastMonthInThisYear(&lastMonthOfYear));
            IFC_RETURN(spCalendar->put_Month(lastMonthOfYear));
            IFC_RETURN(spCalendar->get_LastDayInThisMonth(&lastDayOfMonth));
            IFC_RETURN(spCalendar->put_Day(lastDayOfMonth));
            IFC_RETURN(spCalendar->GetDateTime(&maxDate));
        }

        if (m_nIndex == KnownPropertyIndex::CalendarView_MinDate ||
            m_nIndex == KnownPropertyIndex::CalendarDatePicker_MinDate)
        {
            defaultValue->SetDateTime(minDate);
        }
        else if (m_nIndex == KnownPropertyIndex::CalendarView_MaxDate ||
                 m_nIndex == KnownPropertyIndex::CalendarDatePicker_MaxDate)
        {
            defaultValue->SetDateTime(maxDate);
        }
        return S_OK;
    }

    case KnownPropertyIndex::RichTextBlock_IsTextSelectionEnabled:
    case KnownPropertyIndex::RichEditBox_IsSpellCheckEnabled:
    case KnownPropertyIndex::RichEditBox_IsTextPredictionEnabled:
    case KnownPropertyIndex::RichEditBox_AcceptsReturn:
    case KnownPropertyIndex::TextBox_IsTextPredictionEnabled:
    case KnownPropertyIndex::Control_IsEnabled:
    case KnownPropertyIndex::UIElement_IsHitTestVisible:
    case KnownPropertyIndex::UIElement_IsTapEnabled:
    case KnownPropertyIndex::UIElement_IsRightTapEnabled:
    case KnownPropertyIndex::UIElement_IsDoubleTapEnabled:
    case KnownPropertyIndex::UIElement_IsHoldingEnabled:
    case KnownPropertyIndex::UIElement_UseLayoutRounding:
    case KnownPropertyIndex::UIElement_IsGamepadFocusCandidate:
    case KnownPropertyIndex::UIElement_ExitDisplayModeOnAccessKeyInvoked:
    case KnownPropertyIndex::PathFigure_IsFilled:
    case KnownPropertyIndex::FontIcon_IsTextScaleFactorEnabled:
    case KnownPropertyIndex::TextBlock_IsTextScaleFactorEnabled:
    case KnownPropertyIndex::RichTextBlock_IsTextScaleFactorEnabled:
    case KnownPropertyIndex::Control_IsTextScaleFactorEnabled:
    case KnownPropertyIndex::ContentPresenter_IsTextScaleFactorEnabled:
    case KnownPropertyIndex::TextElement_IsTextScaleFactorEnabled:
    case KnownPropertyIndex::TextElement_ExitDisplayModeOnAccessKeyInvoked:
    case KnownPropertyIndex::FrameworkElement_IsTextScaleFactorEnabledInternal:
    case KnownPropertyIndex::Slider_IsThumbToolTipEnabled:
    case KnownPropertyIndex::SemanticZoom_IsZoomedInViewActive:
    case KnownPropertyIndex::SemanticZoom_CanChangeViews:
    case KnownPropertyIndex::SemanticZoom_IsZoomOutButtonEnabled:
    case KnownPropertyIndex::ListViewBase_IsZoomedInView:
    case KnownPropertyIndex::ListViewBase_IsSwipeEnabled:
    case KnownPropertyIndex::ContentDialog_IsPrimaryButtonEnabled:
    case KnownPropertyIndex::ContentDialog_IsSecondaryButtonEnabled:
    case KnownPropertyIndex::DatePicker_DayVisible:
    case KnownPropertyIndex::DatePicker_MonthVisible:
    case KnownPropertyIndex::DatePicker_YearVisible:
    case KnownPropertyIndex::ScrollViewer_IsHorizontalScrollChainingEnabled:
    case KnownPropertyIndex::ScrollViewer_IsVerticalScrollChainingEnabled:
    case KnownPropertyIndex::ScrollViewer_IsZoomChainingEnabled:
    case KnownPropertyIndex::ScrollViewer_IsScrollInertiaEnabled:
    case KnownPropertyIndex::ScrollViewer_IsZoomInertiaEnabled:
    case KnownPropertyIndex::ScrollViewer_IsHorizontalRailEnabled:
    case KnownPropertyIndex::ScrollViewer_IsVerticalRailEnabled:
    case KnownPropertyIndex::ScrollViewer_BringIntoViewOnFocusChange:
    case KnownPropertyIndex::AutoSuggestBox_UpdateTextOnSelect:
    case KnownPropertyIndex::AutoSuggestBox_AutoMaximizeSuggestionArea:
    case KnownPropertyIndex::CalendarView_IsTodayHighlighted:
    case KnownPropertyIndex::CalendarView_IsOutOfScopeEnabled:
    case KnownPropertyIndex::CalendarDatePicker_IsTodayHighlighted:
    case KnownPropertyIndex::CalendarDatePicker_IsOutOfScopeEnabled:
    case KnownPropertyIndex::ListViewBase_IsMultiSelectCheckBoxEnabled:
    case KnownPropertyIndex::RepositionThemeTransition_IsStaggeringEnabled:
    case KnownPropertyIndex::ComboBox_IsTextSearchEnabled:
    case KnownPropertyIndex::AutomationProperties_IsDataValidForForm:
    case KnownPropertyIndex::Frame_IsNavigationStackEnabled:
    case KnownPropertyIndex::GridViewItemPresenter_SelectionCheckMarkVisualEnabled:
    case KnownPropertyIndex::ListViewItemPresenter_SelectionCheckMarkVisualEnabled:
        defaultValue->SetBool(TRUE);
        return S_OK;

    case KnownPropertyIndex::EntranceThemeTransition_IsStaggeringEnabled:
        defaultValue->SetBool(FALSE);
        return S_OK;

    case KnownPropertyIndex::CalendarView_IsGroupLabelVisible:
    case KnownPropertyIndex::CalendarDatePicker_IsGroupLabelVisible:
    case KnownPropertyIndex::CalendarDatePicker_IsCalendarOpen:
    case KnownPropertyIndex::CalendarViewDayItem_IsBlackout:
    case KnownPropertyIndex::ScrollViewer_IsDeferredScrollingEnabled:
        defaultValue->SetBool(FALSE);
        return S_OK;

    case KnownPropertyIndex::UIElement_IsTabStop:
        if (type->m_nIndex == KnownTypeIndex::UserControl)
        {
            defaultValue->SetBool(FALSE);
        }
        else if (DirectUI::MetadataAPI::IsAssignableFrom(KnownTypeIndex::Control, type->m_nIndex))
        {
            defaultValue->SetBool(TRUE);
        }
        else
        {
            defaultValue->SetBool(FALSE);
        }
        return S_OK;

    case KnownPropertyIndex::Hyperlink_IsTabStop:
    case KnownPropertyIndex::ModernCollectionBasePanel_AreStickyGroupHeadersEnabledBase:
    case KnownPropertyIndex::ItemsStackPanel_AreStickyGroupHeadersEnabled:
    case KnownPropertyIndex::ItemsWrapGrid_AreStickyGroupHeadersEnabled:
    case KnownPropertyIndex::CommandBar_IsDynamicOverflowEnabled:
        defaultValue->SetBool(TRUE);
        return S_OK;

    case KnownPropertyIndex::RichTextBlock_SelectionHighlightColor:
    case KnownPropertyIndex::RichEditBox_SelectionHighlightColor:
    case KnownPropertyIndex::PasswordBox_SelectionHighlightColor:
    case KnownPropertyIndex::TextBox_SelectionHighlightColor:
    case KnownPropertyIndex::TextBlock_SelectionHighlightColor:
        defaultValue->SetColor(GetDefaultSelectionHighlightColor());
        return S_OK;

    case KnownPropertyIndex::GridViewItemPresenter_PointerOverBackgroundMargin:
    case KnownPropertyIndex::ListViewItemPresenter_PointerOverBackgroundMargin:
        defaultValue->WrapThickness(reinterpret_cast<const XTHICKNESS*>(&CDependencyObject::DefaultValueRect));
        return S_OK;

    case KnownPropertyIndex::FrameworkElement_FocusVisualSecondaryBrush:
    case KnownPropertyIndex::FrameworkElement_FocusVisualPrimaryBrush:
    {
        const FocusVisualType forFocusVisualType =  m_nIndex == KnownPropertyIndex::FrameworkElement_FocusVisualSecondaryBrush  ? FocusVisualType::Secondary : FocusVisualType::Primary;
        xref_ptr<CDependencyObject> spFocusBorderBrush;
        IFC_RETURN(GetDefaultFocusVisualBrush(
            forFocusVisualType,
            core,
            referenceObject,
            spFocusBorderBrush.ReleaseAndGetAddressOf()));
        defaultValue->SetObjectAddRef(spFocusBorderBrush.get());
        return S_OK;
    }

    case KnownPropertyIndex::FrameworkElement_FocusVisualPrimaryThickness:
        defaultValue->WrapThickness(&DefaultFocusVisualPrimaryThickness);
        return S_OK;

    case KnownPropertyIndex::FrameworkElement_FocusVisualSecondaryThickness:
        defaultValue->WrapThickness(&DefaultFocusVisualSecondaryThickness);
        return S_OK;

    case KnownPropertyIndex::FrameworkElement_FocusVisualMargin:
    {
        defaultValue->WrapThickness(reinterpret_cast<const XTHICKNESS*>(&CDependencyObject::DefaultValueRect));
        return S_OK;
    }

    case KnownPropertyIndex::Popup_Child:
    case KnownPropertyIndex::UserControl_Content:
    case KnownPropertyIndex::Selector_IsSynchronizedWithCurrentItem:
    case KnownPropertyIndex::CalendarDatePicker_Date:
        defaultValue->SetNull();
        return S_OK;

    case KnownPropertyIndex::FrameworkElement_Width:
    case KnownPropertyIndex::FrameworkElement_Height:
    case KnownPropertyIndex::WrapGrid_ItemWidth:
    case KnownPropertyIndex::WrapGrid_ItemHeight:
    case KnownPropertyIndex::VariableSizedWrapGrid_ItemHeight:
    case KnownPropertyIndex::VariableSizedWrapGrid_ItemWidth:
    case KnownPropertyIndex::ItemsWrapGrid_ItemWidth:
    case KnownPropertyIndex::ItemsWrapGrid_ItemHeight:
        defaultValue->SetFloat(static_cast<XFLOAT>(XDOUBLE_NAN));
        return S_OK;

    case KnownPropertyIndex::FrameworkElement_MaxWidth:
    case KnownPropertyIndex::FrameworkElement_MaxHeight:
    case KnownPropertyIndex::ColumnDefinition_MaxWidth:
    case KnownPropertyIndex::RowDefinition_MaxHeight:
        defaultValue->SetFloat(XFLOAT_INF);
        return S_OK;

    case KnownPropertyIndex::CalendarView_FirstDayOfWeek:
    case KnownPropertyIndex::CalendarDatePicker_FirstDayOfWeek:
        defaultValue->Set(wg::DayOfWeek_Sunday, typeIndex);
        return S_OK;

    case KnownPropertyIndex::CalendarView_SelectionMode:
        defaultValue->Set(xaml_controls::CalendarViewSelectionMode_Single, typeIndex);
        return S_OK;

    case KnownPropertyIndex::CalendarView_DisplayMode:
    case KnownPropertyIndex::CalendarDatePicker_DisplayMode:
        defaultValue->Set(xaml_controls::CalendarViewDisplayMode_Month, typeIndex);
        return S_OK;

    case KnownPropertyIndex::AppBar_ClosedDisplayMode:
        defaultValue->Set(xaml_controls::AppBarClosedDisplayMode_Compact, typeIndex);
        return S_OK;

    case KnownPropertyIndex::CommandBar_DefaultLabelPosition:
        defaultValue->Set(xaml_controls::CommandBarDefaultLabelPosition_Bottom, typeIndex);
        return S_OK;

    case KnownPropertyIndex::AppBarButton_LabelPosition:
        defaultValue->Set(xaml_controls::CommandBarLabelPosition_Default, typeIndex);
        return S_OK;

    case KnownPropertyIndex::AppBarToggleButton_LabelPosition:
        defaultValue->Set(xaml_controls::CommandBarLabelPosition_Default, typeIndex);
        return S_OK;

    case KnownPropertyIndex::Binding_Mode:
        defaultValue->Set(xaml_data::BindingMode_OneWay, typeIndex);
        return S_OK;

    case KnownPropertyIndex::Binding_UpdateSourceTrigger:
        defaultValue->Set(xaml_data::UpdateSourceTrigger_Default, typeIndex);
        return S_OK;

    case KnownPropertyIndex::ButtonBase_ClickMode:
        defaultValue->Set(xaml_controls::ClickMode_Release, typeIndex);
        return S_OK;

    case KnownPropertyIndex::Slider_Orientation:
        defaultValue->Set(SLIDER_DEFAULT_ORIENTATION, typeIndex);
        return S_OK;

    case KnownPropertyIndex::Slider_SnapsTo:
        defaultValue->Set(SLIDER_DEFAULT_SNAPS_TO, typeIndex);
        return S_OK;

    case KnownPropertyIndex::Slider_StepFrequency:
        defaultValue->SetFloat(SLIDER_DEFAULT_STEP_FREQUENCY);
        return S_OK;

    case KnownPropertyIndex::Slider_TickFrequency:
        defaultValue->SetFloat(SLIDER_DEFAULT_TICK_FREQUENCY);
        return S_OK;

    case KnownPropertyIndex::Slider_TickPlacement:
        defaultValue->Set(SLIDER_DEFAULT_TICK_PLACEMENT, typeIndex);
        return S_OK;

    case KnownPropertyIndex::ScrollViewer_HorizontalScrollMode:
    case KnownPropertyIndex::ScrollViewer_VerticalScrollMode:
        defaultValue->Set(xaml_controls::ScrollMode_Enabled, typeIndex);
        return S_OK;

    case KnownPropertyIndex::ScrollViewer_ZoomMode:
        defaultValue->Set(xaml_controls::ZoomMode_Enabled, typeIndex);
        return S_OK;

    case KnownPropertyIndex::ScrollViewer_HorizontalScrollBarVisibility:
        defaultValue->Set(xaml_controls::ScrollBarVisibility_Disabled, typeIndex);
        return S_OK;

    case KnownPropertyIndex::ScrollViewer_VerticalScrollBarVisibility:
        // TODO: This should be read from the default style when that's supported
        defaultValue->Set(xaml_controls::ScrollBarVisibility_Visible, typeIndex);
        return S_OK;

    case KnownPropertyIndex::ScrollViewer_ComputedHorizontalScrollBarVisibility:
    case KnownPropertyIndex::ScrollViewer_ComputedVerticalScrollBarVisibility:
        defaultValue->Set(xaml::Visibility_Visible, typeIndex);
        return S_OK;

    case KnownPropertyIndex::ScrollViewer_HorizontalSnapPointsAlignment:
    case KnownPropertyIndex::ScrollViewer_VerticalSnapPointsAlignment:
        defaultValue->Set(xaml_primitives::SnapPointsAlignment_Near, typeIndex);
        return S_OK;

    case KnownPropertyIndex::ScrollViewer_HorizontalSnapPointsType:
    case KnownPropertyIndex::ScrollViewer_VerticalSnapPointsType:
    case KnownPropertyIndex::ScrollViewer_ZoomSnapPointsType:
        defaultValue->Set(xaml_controls::SnapPointsType_Optional, typeIndex);
        return S_OK;

    case KnownPropertyIndex::PopInThemeAnimation_FromHorizontalOffset:
        defaultValue->SetFloat(POP_IN_THEME_ANIMATION_OFFSET);
        return S_OK;

    case KnownPropertyIndex::SplitOpenThemeAnimation_ContentTranslationDirection:
    case KnownPropertyIndex::SplitCloseThemeAnimation_ContentTranslationDirection:
    case KnownPropertyIndex::DragOverThemeAnimation_Direction:
        defaultValue->Set(xaml_primitives::AnimationDirection_Top, typeIndex);
        return S_OK;

    case KnownPropertyIndex::ToolTip_Placement:
    case KnownPropertyIndex::ToolTipService_Placement:
        defaultValue->Set(xaml_primitives::PlacementMode_Top, typeIndex);
        return S_OK;

    case KnownPropertyIndex::FlyoutBase_ShowMode:
        defaultValue->Set(xaml_primitives::FlyoutShowMode_Standard, typeIndex);
        return S_OK;

    case KnownPropertyIndex::FlyoutBase_AreOpenCloseAnimationsEnabled:
        defaultValue->SetBool(TRUE);
        return S_OK;

    case KnownPropertyIndex::UIElement_ManipulationMode:
        defaultValue->Set(DirectUI::ManipulationModes::System, typeIndex);
        return S_OK;

    case KnownPropertyIndex::ScrollBar_IndicatorMode:
        defaultValue->Set(xaml_primitives::ScrollingIndicatorMode_None, typeIndex);
        return S_OK;

    case KnownPropertyIndex::ItemsStackPanel_ItemsUpdatingScrollMode:
        defaultValue->Set(xaml_controls::ItemsUpdatingScrollMode_KeepItemsInView, typeIndex);
        return S_OK;

    case KnownPropertyIndex::ItemsWrapGrid_GroupHeaderPlacement:
    case KnownPropertyIndex::ItemsStackPanel_GroupHeaderPlacement:
        defaultValue->Set(xaml_primitives::GroupHeaderPlacement::GroupHeaderPlacement_Top, typeIndex);
        return S_OK;

    case KnownPropertyIndex::ListViewItemPresenter_SelectionIndicatorMode:
        defaultValue->Set(xaml_primitives::ListViewItemPresenterSelectionIndicatorMode_Overlay, typeIndex);
        return S_OK;

    case KnownPropertyIndex::WrapGrid_Orientation:
    case KnownPropertyIndex::VirtualizingStackPanel_Orientation:
    case KnownPropertyIndex::VariableSizedWrapGrid_Orientation:
    case KnownPropertyIndex::ScrollBar_Orientation:
    case KnownPropertyIndex::ItemsWrapGrid_Orientation:
    case KnownPropertyIndex::ItemsStackPanel_Orientation:
    case KnownPropertyIndex::CalendarPanel_Orientation:
        defaultValue->Set(xaml_controls::Orientation_Vertical, typeIndex);
        return S_OK;

    case KnownPropertyIndex::DatePicker_Orientation:
    case KnownPropertyIndex::Hub_Orientation:
        defaultValue->Set(xaml_controls::Orientation_Horizontal, typeIndex);
        return S_OK;

    case KnownPropertyIndex::WrapGrid_HorizontalChildrenAlignment:
    case KnownPropertyIndex::VariableSizedWrapGrid_HorizontalChildrenAlignment:
        defaultValue->Set(xaml::HorizontalAlignment_Left, typeIndex);
        return S_OK;

    case KnownPropertyIndex::WrapGrid_VerticalChildrenAlignment:
    case KnownPropertyIndex::VariableSizedWrapGrid_VerticalChildrenAlignment:
        defaultValue->Set(xaml::VerticalAlignment_Top, typeIndex);
        return S_OK;

    case KnownPropertyIndex::FrameworkElement_HorizontalAlignment:
    case KnownPropertyIndex::ContentPresenter_HorizontalContentAlignment:
    case KnownPropertyIndex::GridViewItemPresenter_GridViewItemPresenterHorizontalContentAlignment:
    case KnownPropertyIndex::ListViewItemPresenter_ListViewItemPresenterHorizontalContentAlignment:
        defaultValue->Set(DirectUI::HorizontalAlignment::Stretch, typeIndex);
        return S_OK;

    case KnownPropertyIndex::FrameworkElement_VerticalAlignment:
    case KnownPropertyIndex::ContentPresenter_VerticalContentAlignment:
    case KnownPropertyIndex::GridViewItemPresenter_GridViewItemPresenterVerticalContentAlignment:
    case KnownPropertyIndex::ListViewItemPresenter_ListViewItemPresenterVerticalContentAlignment:
        defaultValue->Set(DirectUI::VerticalAlignment::Stretch, typeIndex);
        return S_OK;
    case KnownPropertyIndex::Hyperlink_UnderlineStyle:
        defaultValue->Set(DirectUI::UnderlineStyle::Single, typeIndex);
        return S_OK;
    case KnownPropertyIndex::EdgeUIThemeTransition_Edge:
        defaultValue->Set(xaml_primitives::EdgeTransitionLocation_Top, typeIndex);
        return S_OK;

    case KnownPropertyIndex::PaneThemeTransition_Edge:
        defaultValue->Set(xaml_primitives::EdgeTransitionLocation_Right, typeIndex);
        return S_OK;

    case KnownPropertyIndex::ListViewBase_IncrementalLoadingTrigger:
        defaultValue->Set(xaml_controls::IncrementalLoadingTrigger_Edge, typeIndex);
        return S_OK;

    case KnownPropertyIndex::ListViewBase_SelectionMode:
        defaultValue->Set(xaml_controls::ListViewSelectionMode_Single, typeIndex);
        return S_OK;

    case KnownPropertyIndex::ListViewBase_ReorderMode:
        defaultValue->Set(xaml_controls::ListViewReorderMode_Disabled, typeIndex);
        return S_OK;

    case KnownPropertyIndex::ListBox_SelectionMode:
        defaultValue->Set(xaml_controls::SelectionMode_Single, typeIndex);
        return S_OK;

    case KnownPropertyIndex::Control_HorizontalContentAlignment:
        defaultValue->Set(DirectUI::HorizontalAlignment::Center, typeIndex);
        return S_OK;

    case KnownPropertyIndex::Control_VerticalContentAlignment:
        defaultValue->Set(DirectUI::VerticalAlignment::Center, typeIndex);
        return S_OK;

    case KnownPropertyIndex::TileBrush_AlignmentX:
        defaultValue->Set(DirectUI::AlignmentX::Center, typeIndex);
        return S_OK;

    case KnownPropertyIndex::TileBrush_AlignmentY:
        defaultValue->Set(DirectUI::AlignmentY::Center, typeIndex);
        return S_OK;

    case KnownPropertyIndex::UIElement_TabIndex:
    case KnownPropertyIndex::Hyperlink_TabIndex:
        defaultValue->SetSigned(XINT32_MAX);
        return S_OK;
    
    case KnownPropertyIndex::UIElement_FocusState:
        defaultValue->Set(DirectUI::FocusState::Unfocused, typeIndex);
        return S_OK;

    case KnownPropertyIndex::PlaneProjection_CenterOfRotationX:
    case KnownPropertyIndex::PlaneProjection_CenterOfRotationY:
        defaultValue->SetFloat(0.5f);
        return S_OK;

    case KnownPropertyIndex::BounceEase_Bounciness:
    case KnownPropertyIndex::ExponentialEase_Exponent:
    case KnownPropertyIndex::PowerEase_Power:
        defaultValue->SetFloat(2.0f);
        return S_OK;

    case KnownPropertyIndex::ElasticEase_Springiness:
    case KnownPropertyIndex::ListViewBase_DataFetchSize:
        defaultValue->SetFloat(3.0f);
        return S_OK;

    case KnownPropertyIndex::PopupThemeTransition_FromVerticalOffset:
        defaultValue->SetFloat(40.0);
        return S_OK;

    case KnownPropertyIndex::ContentThemeTransition_HorizontalOffset:
    case KnownPropertyIndex::EntranceThemeTransition_FromHorizontalOffset:
        defaultValue->SetFloat(0.0);
        return S_OK;

    case KnownPropertyIndex::ContentThemeTransition_VerticalOffset:
    case KnownPropertyIndex::EntranceThemeTransition_FromVerticalOffset:
        defaultValue->SetFloat(28.0);
        return S_OK;

    case KnownPropertyIndex::Application_HighContrastAdjustment:
        defaultValue->Set(DirectUI::ApplicationHighContrastAdjustment::Auto, typeIndex);
        return S_OK;

    case KnownPropertyIndex::BounceEase_Bounces:
    case KnownPropertyIndex::ElasticEase_Oscillations:
        defaultValue->SetSigned(3);
        return S_OK;

    case KnownPropertyIndex::Frame_CacheSize:
        defaultValue->SetSigned(10);
        return S_OK;

    case KnownPropertyIndex::RepeatButton_Delay:
        defaultValue->SetSigned(500);
        return S_OK;

    case KnownPropertyIndex::RepeatButton_Interval:
        defaultValue->SetSigned(33);
        return S_OK;

    case KnownPropertyIndex::Shape_StrokeMiterLimit:
    case KnownPropertyIndex::SwipeHintThemeAnimation_ToVerticalOffset:
    case KnownPropertyIndex::SwipeBackThemeAnimation_FromVerticalOffset:
        defaultValue->SetFloat(10.0f);
        return S_OK;

    case KnownPropertyIndex::RichEditBox_TextAlignment:
    case KnownPropertyIndex::RichTextBlock_TextAlignment:
    case KnownPropertyIndex::TextBlock_TextAlignment:
    case KnownPropertyIndex::TextBox_TextAlignment:
        defaultValue->Set(DirectUI::TextAlignment::Left, typeIndex);
        return S_OK;

    case KnownPropertyIndex::RichTextBlock_TextWrapping:
        defaultValue->Set(DirectUI::TextWrapping::Wrap, typeIndex);
        return S_OK;

    case KnownPropertyIndex::RichEditBox_TextWrapping:
    case KnownPropertyIndex::TextBlock_TextWrapping:
    case KnownPropertyIndex::TextBox_TextWrapping:
        defaultValue->Set(DirectUI::TextWrapping::NoWrap, typeIndex);
        return S_OK;

    case KnownPropertyIndex::Image_Stretch:
    case KnownPropertyIndex::Viewbox_Stretch:
        defaultValue->Set(DirectUI::Stretch::Uniform, typeIndex);
        return S_OK;

    case KnownPropertyIndex::TileBrush_Stretch:
        defaultValue->Set(DirectUI::Stretch::Fill, typeIndex);
        return S_OK;

    case KnownPropertyIndex::GradientBrush_ColorInterpolationMode:
        defaultValue->Set(DirectUI::ColorInterpolationMode::SRgbLinearInterpolation, typeIndex);
        return S_OK;

    case KnownPropertyIndex::Viewbox_StretchDirection:
        defaultValue->Set(DirectUI::StretchDirection::Both, typeIndex);
        return S_OK;

    case KnownPropertyIndex::GradientBrush_MappingMode:
        defaultValue->Set(DirectUI::BrushMappingMode::RelativeToBoundingBox, typeIndex);
        return S_OK;

    case KnownPropertyIndex::ContentPresenter_OpticalMarginAlignment:
        defaultValue->Set(CContentPresenter::DefaultOpticalMarginAlignment(), typeIndex);
        return S_OK;

    case KnownPropertyIndex::ContentPresenter_TextLineBounds:
        defaultValue->Set(CContentPresenter::DefaultTextLineBounds(), typeIndex);
        return S_OK;

    case KnownPropertyIndex::ContentPresenter_TextWrapping:
        defaultValue->Set(CContentPresenter::DefaultTextWrapping(), typeIndex);
        return S_OK;

    case KnownPropertyIndex::ContentPresenter_LineStackingStrategy:
        defaultValue->Set(CContentPresenter::DefaultLineStackingStrategy(), typeIndex);
        return S_OK;

    case KnownPropertyIndex::VirtualizingStackPanel_VirtualizationMode:
        defaultValue->Set(xaml_controls::VirtualizationMode_Recycling, KnownTypeIndex::VirtualizationMode);
        return S_OK;

    case KnownPropertyIndex::Shape_Stretch:
        switch (type->m_nIndex)
        {
        case KnownTypeIndex::Ellipse:
        case KnownTypeIndex::Rectangle:
            defaultValue->Set(DirectUI::Stretch::Fill, typeIndex);
            return S_OK;

        default:
            defaultValue->Set(DirectUI::Stretch::None, typeIndex);
            return S_OK;
        }
        break;

    case KnownPropertyIndex::SymbolIcon_Symbol:
        defaultValue->SetEnum(static_cast<XUINT32>(DirectUI::Symbol::Emoji));
        return S_OK;

    case KnownPropertyIndex::LinearGradientBrush_EndPoint:
        defaultValue->WrapPoint(const_cast<XPOINTF *>(&s_gradientEndPoint));
        return S_OK;
    
    case KnownPropertyIndex::ListViewItemPresenter_SelectionIndicatorCornerRadius:
    {
        defaultValue->WrapCornerRadius(&CListViewBaseItemChrome::s_defaultSelectionIndicatorCornerRadius);
        return S_OK;
    }

    case KnownPropertyIndex::ListViewItemPresenter_CheckBoxCornerRadius:
    {
        defaultValue->WrapCornerRadius(&CListViewBaseItemChrome::s_defaultCheckBoxCornerRadius);
        return S_OK;
    }

    case KnownPropertyIndex::RichEditBox_IsColorFontEnabled:
    case KnownPropertyIndex::RichTextBlock_IsColorFontEnabled:
    case KnownPropertyIndex::TextBlock_IsColorFontEnabled:
    case KnownPropertyIndex::TextBox_IsColorFontEnabled:
    case KnownPropertyIndex::ListViewBase_ShowsScrollingPlaceholders:
    case KnownPropertyIndex::FlipView_UseTouchAnimationsForAllNavigation:
        defaultValue->SetBool(TRUE);
        return S_OK;

    case KnownPropertyIndex::Timeline_RepeatBehavior:
        IFC_RETURN(CreateDefaultVO(core, defaultValue));
        return S_OK;

    case KnownPropertyIndex::SplineColorKeyFrame_KeySpline:
    case KnownPropertyIndex::SplineDoubleKeyFrame_KeySpline:
    case KnownPropertyIndex::SplinePointKeyFrame_KeySpline:
    case KnownPropertyIndex::Storyboard_Children:
    case KnownPropertyIndex::DynamicTimeline_Children:
        IFC_RETURN(CreateDefaultValueObject(core, defaultValue));
        return S_OK;

    case KnownPropertyIndex::Grid_RowSpan:
    case KnownPropertyIndex::Grid_ColumnSpan:
    case KnownPropertyIndex::TimePicker_MinuteIncrement:
    case KnownPropertyIndex::VariableSizedWrapGrid_RowSpan:
    case KnownPropertyIndex::VariableSizedWrapGrid_ColumnSpan:
        defaultValue->SetSigned(1);
        return S_OK;

    // TODO: change CalendarView_CalendarIdentifier to below value.
    case KnownPropertyIndex::CalendarDatePicker_CalendarIdentifier:
    case KnownPropertyIndex::DatePicker_CalendarIdentifier:
        defaultValue->SetString(xstring_ptr(c_strGregorianCalendarStorage));
        return S_OK;

    case KnownPropertyIndex::DatePicker_YearFormat:
        defaultValue->SetString(xstring_ptr(c_strYearFormat));
        return S_OK;

    case KnownPropertyIndex::DatePicker_MonthFormat:
        defaultValue->SetString(xstring_ptr(c_strMonthFormat));
        return S_OK;

    case KnownPropertyIndex::DatePicker_DayFormat:
        defaultValue->SetString(xstring_ptr(c_strDayFormat));
        return S_OK;

    case KnownPropertyIndex::AutomationProperties_AccessibilityView:
        defaultValue->Set(DirectUI::AccessibilityView::Content, KnownTypeIndex::AccessibilityView);
        return S_OK;

    case KnownPropertyIndex::AutomationProperties_Name:
    case KnownPropertyIndex::Glyphs_FontUri:
        defaultValue->SetString(xstring_ptr::NullString());
        return S_OK;

    case KnownPropertyIndex::AutoSuggestBox_MaxSuggestionListHeight:
    case KnownPropertyIndex::ComboBox_MaxDropDownHeight:
        defaultValue->SetDouble(CDependencyObject::PositiveInfinity);
        return S_OK;

    case KnownPropertyIndex::AutomationProperties_Level:
    case KnownPropertyIndex::AutomationProperties_PositionInSet:
    case KnownPropertyIndex::AutomationProperties_SizeOfSet:
        defaultValue->SetSigned(-1);
        return S_OK;

    case KnownPropertyIndex::BitmapImage_AutoPlay:
        defaultValue->Set<valueBool>(true);
        return S_OK;

    case KnownPropertyIndex::AutomationProperties_Culture:
        defaultValue->SetSigned(MAKELCID(::GetThreadUILanguage(), SORT_DEFAULT));
        return S_OK;

    case KnownPropertyIndex::UIElement_TabFocusNavigation:
    case KnownPropertyIndex::Control_TabNavigation:
        defaultValue->Set(TAB_NAVIGATION_NOTSET, typeIndex);
        return S_OK;

    case KnownPropertyIndex::IconSource_Foreground:
        defaultValue->SetNull();
        return S_OK;

    case KnownPropertyIndex::BitmapIconSource_UriSource:
        defaultValue->SetNull();
        return S_OK;

    case KnownPropertyIndex::BitmapIconSource_ShowAsMonochrome:
        defaultValue->SetBool(TRUE);
        return S_OK;

    case KnownPropertyIndex::FontIconSource_Glyph:
        defaultValue->SetString(xstring_ptr::NullString());
        return S_OK;

    case KnownPropertyIndex::FontIconSource_FontSize:
        defaultValue->SetDouble(20.0);
        return S_OK;

    case KnownPropertyIndex::FontIconSource_FontFamily:
    {
        xref_ptr<CDependencyObject> fontFamily;
        IFC_RETURN(GetDefaultFontIconFontFamily(core, fontFamily.ReleaseAndGetAddressOf()));
        defaultValue->SetObjectAddRef(fontFamily.get());
        return S_OK;
    }

    case KnownPropertyIndex::FontIconSource_FontWeight:
        defaultValue->Set(DirectUI::CoreFontWeight::Normal, typeIndex);
        return S_OK;

    case KnownPropertyIndex::FontIconSource_FontStyle:
        defaultValue->Set(DirectUI::FontStyle::Normal, typeIndex);
        return S_OK;

    case KnownPropertyIndex::FontIconSource_IsTextScaleFactorEnabled:
        defaultValue->SetBool(TRUE);
        return S_OK;

    case KnownPropertyIndex::FontIconSource_MirroredWhenRightToLeft:
        defaultValue->SetBool(FALSE);
        return S_OK;

    case KnownPropertyIndex::PathIconSource_Data:
        defaultValue->SetNull();
        return S_OK;

    case KnownPropertyIndex::SymbolIconSource_Symbol:
        defaultValue->SetEnum(DirectUI::Symbol::Emoji);
        return S_OK;
    case KnownPropertyIndex::ToolTip_PlacementRect:
        defaultValue->SetNull();
        return S_OK;
    case KnownPropertyIndex::DatePicker_SelectedDate:
        defaultValue->SetNull();
        return S_OK;
    case KnownPropertyIndex::TimePicker_SelectedTime:
        defaultValue->SetNull();
        return S_OK;
    case KnownPropertyIndex::UIElement_ContextFlyout:
        switch (type->m_nIndex)
        {
            case KnownTypeIndex::TextBlock:
            case KnownTypeIndex::RichTextBlock:
            {
                xref_ptr<CDependencyObject> flyout;
                IFC_RETURN(GetDefaultTextControlContextFlyout(core, flyout.ReleaseAndGetAddressOf()));

                if (flyout)
                {
                    defaultValue->SetObjectAddRef(flyout.get());
                }

                return S_OK;
            }

            default:
                break;
        }
        break;

    case KnownPropertyIndex::TextBlock_SelectionFlyout:
    case KnownPropertyIndex::RichTextBlock_SelectionFlyout:
        {
            xref_ptr<CDependencyObject> flyout;
            IFC_RETURN(GetDefaultTextControlSelectionFlyout(core, flyout.ReleaseAndGetAddressOf()));

            if (flyout)
            {
                defaultValue->SetObjectAddRef(flyout.get());
            }

            return S_OK;
        }
        break;
    case KnownPropertyIndex::FlyoutBase_ShouldConstrainToRootBounds:
        switch (type->m_nIndex)
        {
        case KnownTypeIndex::MenuFlyout:
            defaultValue->SetBool(FALSE); // MenuFlyouts aren't constrained by default.
            break;
        default:
            defaultValue->SetBool(TRUE);
            break;
        }
        return S_OK;
    case KnownPropertyIndex::Popup_ShouldConstrainToRootBounds:
        defaultValue->SetBool(TRUE);
        return S_OK;
    case KnownPropertyIndex::FlyoutPresenter_IsDefaultShadowEnabled:
        defaultValue->SetBool(TRUE);
        return S_OK;
    case KnownPropertyIndex::MenuFlyoutPresenter_IsDefaultShadowEnabled:
        defaultValue->SetBool(TRUE);
        return S_OK;
    }

    if (GetPropertyType()->IsEnum())
    {
        if (GetPropertyType()->IsCompactEnum())
        {
            defaultValue->SetEnum8(0);
        }
        else
        {
            defaultValue->SetEnum(0);
        }
    }
    else
    {
        // Infer default from property type.
        switch (GetPropertyType()->m_nIndex)
        {
        case KnownTypeIndex::RelativeSourceMode:
            defaultValue->Set(xaml_data::RelativeSourceMode_None, KnownTypeIndex::RelativeSourceMode);
            break;
        case KnownTypeIndex::UpdateSourceTrigger:
            defaultValue->Set(xaml_data::UpdateSourceTrigger_Default, KnownTypeIndex::UpdateSourceTrigger);
            break;
        case KnownTypeIndex::BindingMode:
            defaultValue->Set(xaml_data::BindingMode_OneWay, KnownTypeIndex::BindingMode);
            break;

        case KnownTypeIndex::Float:
            defaultValue->SetFloat(0.0f);
            break;

        case KnownTypeIndex::Double:
            defaultValue->SetDouble(0.0f);
            break;

        case KnownTypeIndex::Char16:
        case KnownTypeIndex::Int16:
        case KnownTypeIndex::UInt16:
        case KnownTypeIndex::Int32:
        case KnownTypeIndex::UInt32:
            defaultValue->SetSigned(0);
            break;

        case KnownTypeIndex::Int64:
        case KnownTypeIndex::UInt64:
            defaultValue->SetInt64(0);
            break;

        case KnownTypeIndex::Boolean:
            defaultValue->SetBool(FALSE);
            break;

        case KnownTypeIndex::Color:
            defaultValue->SetColor(0);
            break;

            // Note: for the following cases we do *NOT* need to check for the
            // pointer since we're going to assign it to a const object that won't
            // be freed when the CValue goes away.

        case KnownTypeIndex::Point:
            defaultValue->WrapPoint(&CDependencyObject::DefaultValuePoint);
            break;

        case KnownTypeIndex::Rect:
            defaultValue->WrapRect(&CDependencyObject::DefaultValueRect);
            break;

        case KnownTypeIndex::CornerRadius:
            defaultValue->WrapCornerRadius((const XCORNERRADIUS*)&CDependencyObject::DefaultValueRect);
            break;

        case KnownTypeIndex::Thickness:
            defaultValue->WrapThickness((const XTHICKNESS*)&CDependencyObject::DefaultValueRect);
            break;

        case KnownTypeIndex::GridLength:
            defaultValue->WrapGridLength((const XGRIDLENGTH*)&CDependencyObject::DefaultValueGridLength);
            break;

        case KnownTypeIndex::Size:
            defaultValue->WrapSize((const XSIZEF*)&CDependencyObject::DefaultValuePoint);
            break;

        case KnownTypeIndex::String:
            defaultValue->SetNull();
            break;

        case KnownTypeIndex::TimeSpan:
            {
                wf::TimeSpan value = {};
                defaultValue->SetTimeSpan(value);
            }
            break;

        case KnownTypeIndex::DateTime:
            {
                wf::DateTime value = {};
                defaultValue->SetDateTime(value);
            }
            break;

        case KnownTypeIndex::TypeName:
            defaultValue->SetTypeHandle(KnownTypeIndex::UnknownType);
            break;

        default:
            defaultValue->SetNull();
            break;
        }
    }

    return S_OK;
}
