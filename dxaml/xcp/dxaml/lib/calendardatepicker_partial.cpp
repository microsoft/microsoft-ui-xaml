// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "CalendarView.g.h"
#include "Button.g.h"
#include "FlyoutBase.g.h"
#include "TextBlock.g.h"
#include "Grid.g.h"
#include "CalendarDatePickerDateChangedEventArgs.g.h"
#include "PointerRoutedEventArgs.g.h"
#include "RightTappedRoutedEventArgs.g.h"
#include "CalendarDatePicker.g.h"
#include "CalendarDatePickerAutomationPeer.g.h"
#include "VisualTreeHelper.h"
#include "localizedResource.h"
#include <windows.globalization.datetimeformatting.h>
#include "ElementSoundPlayerService_Partial.h"

REFERENCE_ELEMENT_NAME_IMPL(wf::DateTime, L"Windows.Foundation.DateTime");

using namespace DirectUI;
using namespace DirectUISynonyms;

#undef min
#undef max

CalendarDatePicker::CalendarDatePicker()
    : m_isYearDecadeViewDimensionRequested(false)
    , m_colsInYearDecadeView(0)
    , m_rowsInYearDecadeView(0)
    , m_isSetDisplayDateRequested(false)
    , m_displayDate({})
    , m_isPointerOverMain(false)
    , m_isPressedOnMain(false)
    , m_shouldPerformActions(false)
    , m_isSelectedDatesChangingInternally(false)
{
}

_Check_return_ HRESULT CalendarDatePicker::PrepareState()
{
    IFC_RETURN(CalendarDatePickerGenerated::PrepareState());

    // Set a default string as the PlaceholderText property value.
    wrl_wrappers::HString strDefaultPlaceholderText;
    IFC_RETURN(DXamlCore::GetCurrent()->GetLocalizedResourceString(TEXT_CALENDARDATEPICKER_DEFAULT_PLACEHOLDER_TEXT, strDefaultPlaceholderText.ReleaseAndGetAddressOf()));
    IFC_RETURN(put_PlaceholderText(strDefaultPlaceholderText.Get()));

    return S_OK;
}

_Check_return_ HRESULT CalendarDatePicker::OnPropertyChanged2(
    _In_ const PropertyChangedParams& args)
{
    IFC_RETURN(CalendarDatePickerGenerated::OnPropertyChanged2(args));

    switch (args.m_pDP->GetIndex())
    {
        case KnownPropertyIndex::CalendarDatePicker_IsCalendarOpen:
        {
            IFC_RETURN(UpdateCalendarVisibility());
            break;
        }

        case KnownPropertyIndex::CalendarDatePicker_Date:
        {
            ctl::ComPtr<wf::IReference<wf::DateTime>> spNewDateReference;
            ctl::ComPtr<wf::IReference<wf::DateTime>> spOldDateReference;
            IFC_RETURN(CValueBoxer::UnboxValue<wf::DateTime>(args.m_pOldValue, &spOldDateReference));
            IFC_RETURN(CValueBoxer::UnboxValue<wf::DateTime>(args.m_pNewValue, &spNewDateReference));
            IFC_RETURN(OnDateChanged(spOldDateReference.Get(), spNewDateReference.Get()));
            break;
        }

        case KnownPropertyIndex::FrameworkElement_Language:
        case KnownPropertyIndex::CalendarDatePicker_CalendarIdentifier:
        case KnownPropertyIndex::CalendarDatePicker_DateFormat:
        {
            IFC_RETURN(OnDateFormatChanged());
            break;
        }

        case KnownPropertyIndex::CalendarDatePicker_Header:
        case KnownPropertyIndex::CalendarDatePicker_HeaderTemplate:
        {
            IFC_RETURN(UpdateHeaderVisibility());
            break;
        }

        case KnownPropertyIndex::CalendarDatePicker_LightDismissOverlayMode:
        {
            if (m_tpFlyout)
            {
                // Forward the new property onto our flyout.
                auto overlayMode = xaml_controls::LightDismissOverlayMode_Off;
                IFC_RETURN(get_LightDismissOverlayMode(&overlayMode));
                IFC_RETURN(m_tpFlyout.Cast<FlyoutBase>()->put_LightDismissOverlayMode(overlayMode));
            }
            break;
        }

#if WI_IS_FEATURE_PRESENT(Feature_HeaderPlacement)
        case KnownPropertyIndex::CalendarDatePicker_HeaderPlacement:
        {
            IFC_RETURN(UpdateVisualState());
            break;
        }
#endif
    }

    return S_OK;
}

IFACEMETHODIMP
CalendarDatePicker::OnApplyTemplate()
{
    ctl::ComPtr<xaml_controls::ICalendarView> spCalendarView;
    ctl::ComPtr<xaml_controls::ITextBlock> spDateText;
    ctl::ComPtr<xaml_controls::IGrid> spRoot;

    IFC_RETURN(DetachHandler(m_epFlyoutOpenedHandler, m_tpFlyout));
    IFC_RETURN(DetachHandler(m_epFlyoutClosedHandler, m_tpFlyout));
    IFC_RETURN(DetachHandler(m_epCalendarViewCalendarViewDayItemChangingHandler, m_tpCalendarView));
    IFC_RETURN(DetachHandler(m_epCalendarViewSelectedDatesChangedHandler, m_tpCalendarView));

    m_tpCalendarView.Clear();
    m_tpHeaderContentPresenter.Clear();
    m_tpDateText.Clear();
    m_tpFlyout.Clear();
    m_tpRoot.Clear();

    IFC_RETURN(CalendarDatePickerGenerated::OnApplyTemplate());

    IFC_RETURN(GetTemplatePart<xaml_controls::ICalendarView>(STR_LEN_PAIR(L"CalendarView"), spCalendarView.ReleaseAndGetAddressOf()));
    IFC_RETURN(GetTemplatePart<xaml_controls::ITextBlock>(STR_LEN_PAIR(L"DateText"), spDateText.ReleaseAndGetAddressOf()));
    IFC_RETURN(GetTemplatePart<xaml_controls::IGrid>(STR_LEN_PAIR(L"Root"), spRoot.ReleaseAndGetAddressOf()));

    SetPtrValue(m_tpCalendarView, spCalendarView.Get());
    SetPtrValue(m_tpDateText, spDateText.Get());
    SetPtrValue(m_tpRoot, spRoot.Get());

    if (m_tpRoot)
    {
        ctl::ComPtr<xaml_primitives::IFlyoutBase> spFlyout;

        IFC_RETURN(FlyoutBaseFactory::GetAttachedFlyoutStatic(m_tpRoot.Cast<Grid>(), spFlyout.ReleaseAndGetAddressOf()));
        SetPtrValue(m_tpFlyout, spFlyout.Get());

        if (m_tpFlyout)
        {
            // by default flyoutbase will resize the presenter's content (here is the CalendarView) if there is
            // not enough space, because the content is inside a ScrollViewer.
            // however CalendarView has 3 huge ScrollViewers, put CalendarView in a ScrollViewer will have a very
            // bad user experience because user can't scroll the outer ScrollViewer until the inner ScrollViewer
            // hits the end of content.
            // we decide to remove the ScrollViewer from presenter's template and let flyoutbase not resize us.
            m_tpFlyout.Cast<FlyoutBase>()->DisablePresenterResizing();

            // Forward the value of LightDismissOverlayMode to our flyout.
            auto overlayMode = xaml_controls::LightDismissOverlayMode_Off;
            IFC_RETURN(get_LightDismissOverlayMode(&overlayMode));
            IFC_RETURN(m_tpFlyout.Cast<FlyoutBase>()->put_LightDismissOverlayMode(overlayMode));

            IFC_RETURN(m_epFlyoutOpenedHandler.AttachEventHandler(m_tpFlyout.Cast<FlyoutBase>(),
                [this](IInspectable* pSender, IInspectable* pArgs)
            {
                OpenedEventSourceType* pEventSource = nullptr;

                IFC_RETURN(GetOpenedEventSourceNoRef(&pEventSource));
                return pEventSource->Raise(ctl::as_iinspectable(this), pArgs);
            }));


            IFC_RETURN(m_epFlyoutClosedHandler.AttachEventHandler(m_tpFlyout.Cast<FlyoutBase>(),
                [this](IInspectable* pSender, IInspectable* pArgs)
            {
                OpenedEventSourceType* pEventSource = nullptr;

                IFC_RETURN(put_IsCalendarOpen(FALSE));

                IFC_RETURN(GetClosedEventSourceNoRef(&pEventSource));
                return pEventSource->Raise(ctl::as_iinspectable(this), pArgs);
            }));
        }
    }

    if (m_tpCalendarView)
    {
        // Forward CalendarViewDayItemChanging event from CalendarView to CalendarDatePicker
        IFC_RETURN(m_epCalendarViewCalendarViewDayItemChangingHandler.AttachEventHandler(m_tpCalendarView.Cast<CalendarView>(),
            [this](ICalendarView* pSender, ICalendarViewDayItemChangingEventArgs* pArgs)
        {
            CalendarViewDayItemChangingEventSourceType* pEventSource = nullptr;

            IFC_RETURN(GetCalendarViewDayItemChangingEventSourceNoRef(&pEventSource));
            return pEventSource->Raise(m_tpCalendarView.Get(), pArgs);
        }));

        // handle SelectedDatesChanged event
        IFC_RETURN(m_epCalendarViewSelectedDatesChangedHandler.AttachEventHandler(m_tpCalendarView.Cast<CalendarView>(),
            [this](ICalendarView* pSender, ICalendarViewSelectedDatesChangedEventArgs* pArgs)
        {
            return OnSelectedDatesChanged(pSender, pArgs);

        }));

        // check if we requested any operations that require CalendarView template part
        if (m_isYearDecadeViewDimensionRequested)
        {
            m_isYearDecadeViewDimensionRequested = false;
            IFC_RETURN(m_tpCalendarView->SetYearDecadeDisplayDimensions(m_colsInYearDecadeView, m_rowsInYearDecadeView));
        }

        if (m_isSetDisplayDateRequested)
        {
            m_isSetDisplayDateRequested = false;
            IFC_RETURN(m_tpCalendarView->SetDisplayDate(m_displayDate));
        }
    }

    // we might set IsCalendarOpen to true before template is applied.
    IFC_RETURN(UpdateCalendarVisibility());

    // Initialize header visibility
    IFC_RETURN(UpdateHeaderVisibility());

    IFC_RETURN(FormatDate());

    IFC_RETURN(UpdateVisualState());

    return S_OK;
}

_Check_return_ HRESULT CalendarDatePicker::OnSelectedDatesChanged(
    _In_ xaml_controls::ICalendarView* pSender,
    _In_ xaml_controls::ICalendarViewSelectedDatesChangedEventArgs* pArgs)
{
    ASSERT(m_tpCalendarView && pSender == m_tpCalendarView.Get());

    if (!m_isSelectedDatesChangingInternally)
    {
        xaml_controls::CalendarViewSelectionMode mode = xaml_controls::CalendarViewSelectionMode_None;
        IFC_RETURN(m_tpCalendarView->get_SelectionMode(&mode));

        // We only care about single selection mode.
        // In case the calendarview's selection mode is set to multiple by developer,
        // we just silently ignore this event.
        if (mode == xaml_controls::CalendarViewSelectionMode_Single)
        {
            ctl::ComPtr<wfc::IVectorView<wf::DateTime>> spAddedDates;
            unsigned addedDatesSize = 0;

            IFC_RETURN(pArgs->get_AddedDates(spAddedDates.ReleaseAndGetAddressOf()));
            IFC_RETURN(spAddedDates->get_Size(&addedDatesSize));

            ASSERT(addedDatesSize <= 1);

            if (addedDatesSize == 1)
            {
                wf::DateTime newDate;
                ctl::ComPtr<IInspectable> spNewDate;
                ctl::ComPtr<wf::IReference<wf::DateTime>> spNewDateReference;

                IFC_RETURN(spAddedDates->GetAt(0, &newDate));
                IFC_RETURN(PropertyValue::CreateFromDateTime(newDate, &spNewDate));
                IFC_RETURN(spNewDate.As(&spNewDateReference));

                IFC_RETURN(put_IsCalendarOpen(FALSE));

                IFC_RETURN(put_Date(spNewDateReference.Get()));
            }
            else // date is deselected
            {
                IFC_RETURN(put_Date(nullptr));
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CalendarDatePicker::OnDateChanged(
    _In_ wf::IReference<wf::DateTime>* pOldDateReference,
    _In_ wf::IReference<wf::DateTime>* pNewDateReference)
{
    if (pNewDateReference)     // new Date property is set.
    {
        wf::DateTime date;
        wf::DateTime coercedDate;
        wf::DateTime minDate;
        wf::DateTime maxDate;

        IFC_RETURN(pNewDateReference->get_Value(&date));

        // coerce dates
        IFC_RETURN(get_MinDate(&minDate));
        IFC_RETURN(get_MaxDate(&maxDate));
        coercedDate.UniversalTime = std::min(maxDate.UniversalTime, std::max(minDate.UniversalTime, date.UniversalTime));

        // if Date is not in the range of min/max date, we'll coerce it and trigger DateChanged again.
        if (coercedDate.UniversalTime != date.UniversalTime)
        {
            ctl::ComPtr<wf::IReference<wf::DateTime>> spCoercedDateReference;
            IFC_RETURN(PropertyValue::CreateFromDateTime(coercedDate, &spCoercedDateReference));
            IFC_RETURN(put_Date(spCoercedDateReference.Get()));
            return S_OK;
        }
    }

    IFC_RETURN(SyncDate());

    // Raise DateChanged event.
    IFC_RETURN(RaiseDateChanged(pOldDateReference, pNewDateReference));

    // Update the Date text.
    IFC_RETURN(FormatDate());

    IFC_RETURN(UpdateVisualState());

    return S_OK;
}

_Check_return_ HRESULT CalendarDatePicker::RaiseDateChanged(
    _In_ wf::IReference<wf::DateTime>* pOldDateReference,
    _In_ wf::IReference<wf::DateTime>* pNewDateReference)
{
    ctl::ComPtr<CalendarDatePickerDateChangedEventArgs> spArgs;
    DateChangedEventSourceType* pEventSource = nullptr;

    IFC_RETURN(ctl::make(&spArgs));
    IFC_RETURN(GetDateChangedEventSourceNoRef(&pEventSource));

    IFC_RETURN(spArgs->put_NewDate(pNewDateReference));
    IFC_RETURN(spArgs->put_OldDate(pOldDateReference));

    IFC_RETURN(pEventSource->Raise(this, spArgs.Get()));

    return S_OK;
}

_Check_return_ HRESULT CalendarDatePicker::SetYearDecadeDisplayDimensionsImpl(_In_ INT columns, _In_ INT rows)
{
    if (m_tpCalendarView)
    {
        IFC_RETURN(m_tpCalendarView->SetYearDecadeDisplayDimensions(columns, rows));
    }
    else
    {
        m_isYearDecadeViewDimensionRequested = true;
        m_colsInYearDecadeView = columns;
        m_rowsInYearDecadeView = rows;
    }

    return S_OK;
}
_Check_return_ HRESULT CalendarDatePicker::SetDisplayDateImpl(_In_ wf::DateTime date)
{
    if (m_tpCalendarView)
    {
        IFC_RETURN(m_tpCalendarView->SetDisplayDate(date));
    }
    else
    {
        m_isSetDisplayDateRequested = true;
        m_displayDate = date;
    }
    return S_OK;
}

_Check_return_ HRESULT CalendarDatePicker::UpdateCalendarVisibility()
{
    if (m_tpFlyout && m_tpRoot)
    {
        BOOLEAN isCalendarOpen = false;

        IFC_RETURN(get_IsCalendarOpen(&isCalendarOpen));
        if (isCalendarOpen)
        {
            IFC_RETURN(m_tpFlyout->ShowAt(m_tpRoot.Cast<Grid>()));
            IFC_RETURN(SyncDate());
        }
        else
        {
            IFC_RETURN(m_tpFlyout->Hide());
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CalendarDatePicker::OnDateFormatChanged()
{
    ctl::ComPtr<IInspectable> spDateFormat;
    BOOLEAN isUnsetValue = FALSE;

    IFC_RETURN(ReadLocalValue(
        MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::CalendarDatePicker_DateFormat),
        &spDateFormat));

    IFC_RETURN(DependencyPropertyFactory::IsUnsetValue(spDateFormat.Get(), isUnsetValue));

    m_tpDateFormatter.Clear();

    if (!isUnsetValue)   // format is set, use this format.
    {
        wrl_wrappers::HString dateFormat;

        IFC_RETURN(get_DateFormat(dateFormat.GetAddressOf()));

        if (!dateFormat.IsEmpty())
        {
            ctl::ComPtr<wg::DateTimeFormatting::IDateTimeFormatterFactory> spFormatterFactory;
            ctl::ComPtr<wg::DateTimeFormatting::IDateTimeFormatter> spFormatter;
            wrl_wrappers::HStringReference strClock(L"24HourClock");    // it doesn't matter if it is 24 or 12 hour clock
            wrl_wrappers::HStringReference strGeographicRegion(L"ZZ");    // geographicRegion doesn't really matter as we have no decimal separator or grouping
            wrl_wrappers::HString strCalendarIdentifier;
            ctl::ComPtr<wfc::IIterable<HSTRING>> spLanguages;
            wrl_wrappers::HString strLanguage;

            IFC_RETURN(get_CalendarIdentifier(strCalendarIdentifier.GetAddressOf()));
            IFC_RETURN(get_Language(strLanguage.GetAddressOf()));
            IFC_RETURN(CalendarView::CreateCalendarLanguagesStatic(std::move(strLanguage), &spLanguages));

            IFC_RETURN(ctl::GetActivationFactory(
                wrl_wrappers::HStringReference(RuntimeClass_Windows_Globalization_DateTimeFormatting_DateTimeFormatter).Get(),
                &spFormatterFactory));

            IFCPTR_RETURN(spFormatterFactory);

            IFC_RETURN(spFormatterFactory->CreateDateTimeFormatterContext(
                dateFormat,
                spLanguages.Get(),
                strGeographicRegion.Get(),
                strCalendarIdentifier.Get(),
                strClock.Get(),
                spFormatter.ReleaseAndGetAddressOf()));

            SetPtrValue(m_tpDateFormatter, spFormatter.Get());
        }
    }

    IFC_RETURN(FormatDate());

    return S_OK;
}

_Check_return_ HRESULT CalendarDatePicker::FormatDate()
{
    if (m_tpDateText)
    {
        ctl::ComPtr<wf::IReference<wf::DateTime>> spDateReference;
        wrl_wrappers::HString dateAsString;

        IFC_RETURN(get_Date(&spDateReference));

        if (spDateReference)
        {
            wf::DateTime date;

            IFC_RETURN(spDateReference->get_Value(&date));

            if (m_tpDateFormatter)  // when there is a formatter, use it
            {
                IFC_RETURN(m_tpDateFormatter->Format(date, dateAsString.GetAddressOf()));
            }
            else        // else use system build-in shortdate formatter.
            {
                ctl::ComPtr<wg::DateTimeFormatting::IDateTimeFormatterStatics> spDateTimeFormatterStatics;
                ctl::ComPtr<wg::DateTimeFormatting::IDateTimeFormatter> spDateFormatter;

                IFC_RETURN(ctl::GetActivationFactory(wrl_wrappers::HStringReference(RuntimeClass_Windows_Globalization_DateTimeFormatting_DateTimeFormatter).Get(), &spDateTimeFormatterStatics));
                IFC_RETURN(spDateTimeFormatterStatics->get_ShortDate(&spDateFormatter));
                IFC_RETURN(spDateFormatter->Format(date, dateAsString.GetAddressOf()));
            }
        }
        else
        {
            // else use placeholder text.
            IFC_RETURN(get_PlaceholderText(dateAsString.GetAddressOf()));
        }

        IFC_RETURN(m_tpDateText->put_Text(dateAsString.Get()));
    }

    return S_OK;
}


_Check_return_ HRESULT CalendarDatePicker::UpdateHeaderVisibility()
{
    ctl::ComPtr<xaml::IDataTemplate> spHeaderTemplate;
    ctl::ComPtr<IInspectable> spHeader;

    IFC_RETURN(get_HeaderTemplate(&spHeaderTemplate));
    IFC_RETURN(get_Header(&spHeader));

    IFC_RETURN(ConditionallyGetTemplatePartAndUpdateVisibility(
        XSTRING_PTR_EPHEMERAL(L"HeaderContentPresenter"),
        (spHeader || spHeaderTemplate),
        m_tpHeaderContentPresenter));

    return S_OK;
}

_Check_return_ HRESULT
CalendarDatePicker::GetPlainText(
    _Out_ HSTRING* strPlainText)
{
    ctl::ComPtr<IInspectable> spHeader;
    *strPlainText = nullptr;

    IFC_RETURN(get_Header(&spHeader));
    if (spHeader != nullptr)
    {
        IFC_RETURN(FrameworkElement::GetStringFromObject(spHeader.Get(), strPlainText));
    }

    return S_OK;
}

_Check_return_ HRESULT CalendarDatePicker::ChangeVisualState(_In_ bool useTransitions)
{
    BOOLEAN isEnabled = false;
    BOOLEAN ignored = false;
    xaml::FocusState focusState = xaml::FocusState_Unfocused;
    ctl::ComPtr<wf::IReference<wf::DateTime>> spDateReference;

    IFC_RETURN(get_IsEnabled(&isEnabled));
    IFC_RETURN(get_FocusState(&focusState));
    IFC_RETURN(get_Date(&spDateReference));

    // CommonStates VisualStateGroup.
    if (!isEnabled)
    {
        IFC_RETURN(GoToState(useTransitions, L"Disabled", &ignored));
    }
    else if (m_isPressedOnMain)
    {
        IFC_RETURN(GoToState(useTransitions, L"Pressed", &ignored));
    }
    else if (m_isPointerOverMain)
    {
        IFC_RETURN(GoToState(useTransitions, L"PointerOver", &ignored));
    }
    else
    {
        IFC_RETURN(GoToState(useTransitions, L"Normal", &ignored));
    }

    // FocusStates VisualStateGroup.
    if (xaml::FocusState_Unfocused != focusState && isEnabled)
    {
        if (xaml::FocusState_Pointer == focusState)
        {
            IFC_RETURN(GoToState(useTransitions, L"PointerFocused", &ignored));
        }
        else
        {
            IFC_RETURN(GoToState(useTransitions, L"Focused", &ignored));
        }
    }
    else
    {
        IFC_RETURN(GoToState(useTransitions, L"Unfocused", &ignored));
    }

    // SelectionStates VisualStateGroup.
    if (spDateReference && isEnabled)
    {
        IFC_RETURN(GoToState(useTransitions, L"Selected", &ignored));
    }
    else
    {
        IFC_RETURN(GoToState(useTransitions, L"Unselected", &ignored));
    }

#if WI_IS_FEATURE_PRESENT(Feature_HeaderPlacement)
    // HeaderStates VisualStateGroup.
    xaml_controls::ControlHeaderPlacement headerPlacement = xaml_controls::ControlHeaderPlacement_Top;
    IFC_RETURN(get_HeaderPlacement(&headerPlacement));

    switch (headerPlacement)
    {
        case DirectUI::ControlHeaderPlacement::Top:
            IFC_RETURN(GoToState(useTransitions, L"TopHeader", &ignored));
            break;

        case DirectUI::ControlHeaderPlacement::Left:
            IFC_RETURN(GoToState(useTransitions, L"LeftHeader", &ignored));
            break;
    }
#endif

    return S_OK;
}

// Responds to the KeyDown event.
IFACEMETHODIMP CalendarDatePicker::OnKeyDown(
    _In_ xaml_input::IKeyRoutedEventArgs* pArgs)
{
    IFC_RETURN(CalendarDatePickerGenerated::OnKeyDown(pArgs));

    boolean isHandled = false;
    IFC_RETURN(pArgs->get_Handled(&isHandled));
    if (isHandled)
    {
        return S_OK;
    }

    wsy::VirtualKey key = wsy::VirtualKey_None;
    wsy::VirtualKeyModifiers modifiers = wsy::VirtualKeyModifiers_None;

    IFC_RETURN(GetKeyboardModifiers(&modifiers));
    IFC_RETURN(pArgs->get_Key(&key));

    if (modifiers == wsy::VirtualKeyModifiers_None)
    {
        switch (key)
        {
        case wsy::VirtualKey_Enter:
        case wsy::VirtualKey_Space:
            IFC_RETURN(put_IsCalendarOpen(TRUE));
            break;
        default:
            break;
        }
    }

    return S_OK;
}

IFACEMETHODIMP CalendarDatePicker::OnPointerPressed(_In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    IFC_RETURN(CalendarDatePickerGenerated::OnPointerPressed(pArgs));

    boolean isHandled = false;
    IFC_RETURN(pArgs->get_Handled(&isHandled));
    if (isHandled)
    {
        return S_OK;
    }

    boolean isEnabled = false;
    IFC_RETURN(get_IsEnabled(&isEnabled));

    if (!isEnabled)
    {
        return S_OK;
    }

    bool isEventSourceTarget = false;
    IFC_RETURN(IsEventSourceTarget(static_cast<PointerRoutedEventArgs*>(pArgs), &isEventSourceTarget));

    if (isEventSourceTarget)
    {
        boolean isLeftButtonPressed = false;

        ctl::ComPtr<ixp::IPointerPoint> spPointerPoint;
        ctl::ComPtr<ixp::IPointerPointProperties> spPointerProperties;

        IFC_RETURN(pArgs->GetCurrentPoint(this, &spPointerPoint));
        IFCPTR_RETURN(spPointerPoint);

        IFC_RETURN(spPointerPoint->get_Properties(&spPointerProperties));
        IFCPTR_RETURN(spPointerProperties);
        IFC_RETURN(spPointerProperties->get_IsLeftButtonPressed(&isLeftButtonPressed));

        if (isLeftButtonPressed)
        {
            IFC_RETURN(pArgs->put_Handled(TRUE));
            m_isPressedOnMain = true;
            // for "Pressed" visual state to render
            IFC_RETURN(UpdateVisualState());
        }
    }
    return S_OK;
}

IFACEMETHODIMP CalendarDatePicker::OnPointerReleased(_In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    IFC_RETURN(CalendarDatePickerGenerated::OnPointerReleased(pArgs));

    boolean isHandled = false;
    IFC_RETURN(pArgs->get_Handled(&isHandled));
    if (isHandled)
    {
        return S_OK;
    }

    boolean isEnabled = false;
    IFC_RETURN(get_IsEnabled(&isEnabled));

    if (!isEnabled)
    {
        return S_OK;
    }

    bool isEventSourceTarget = false;
    IFC_RETURN(IsEventSourceTarget(static_cast<PointerRoutedEventArgs*>(pArgs), &isEventSourceTarget));

    if (isEventSourceTarget)
    {
        boolean isLeftButtonPressed = false;

        ctl::ComPtr<ixp::IPointerPoint> spPointerPoint;
        ctl::ComPtr<ixp::IPointerPointProperties> spPointerProperties;

        IFC_RETURN(pArgs->GetCurrentPoint(this, &spPointerPoint));
        IFCPTR_RETURN(spPointerPoint);

        IFC_RETURN(spPointerPoint->get_Properties(&spPointerProperties));
        IFCPTR_RETURN(spPointerProperties);
        IFC_RETURN(spPointerProperties->get_IsLeftButtonPressed(&isLeftButtonPressed));

        wsy::VirtualKeyModifiers modifiers = wsy::VirtualKeyModifiers_None;

        IFC_RETURN(GetKeyboardModifiers(&modifiers));

        m_shouldPerformActions = (m_isPressedOnMain && !isLeftButtonPressed
            && modifiers == wsy::VirtualKeyModifiers_None);

        if (m_shouldPerformActions)
        {
            m_isPressedOnMain = false;

            IFC_RETURN(pArgs->put_Handled(TRUE));
        }

        GestureModes gestureFollowing = GestureModes::None;
        IFC_RETURN(static_cast<PointerRoutedEventArgs*>(pArgs)->get_GestureFollowing(&gestureFollowing));
        if (gestureFollowing == GestureModes::RightTapped)
        {
            // This will be released OnRightTappedUnhandled or destructor.
            return S_OK;
        }

        IFC_RETURN(PerformPointerUpAction());

        IFC_RETURN(UpdateVisualState());
    }

    return S_OK;
}

_Check_return_ HRESULT CalendarDatePicker::OnRightTappedUnhandled(
    _In_ xaml_input::IRightTappedRoutedEventArgs* pArgs)
{
    IFC_RETURN(CalendarDatePickerGenerated::OnRightTappedUnhandled(pArgs));

    boolean isHandled = false;
    IFC_RETURN(pArgs->get_Handled(&isHandled));
    if (isHandled)
    {
        return S_OK;
    }

    bool isEventSourceTarget = false;
    IFC_RETURN(IsEventSourceTarget(static_cast<RightTappedRoutedEventArgs*>(pArgs), &isEventSourceTarget));

    if (isEventSourceTarget)
    {
        IFC_RETURN(PerformPointerUpAction());
    }

    return S_OK;
}


IFACEMETHODIMP CalendarDatePicker::OnPointerEntered(_In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    bool isEventSourceTarget = false;

    IFC_RETURN(CalendarDatePickerGenerated::OnPointerEntered(pArgs));

    IFC_RETURN(IsEventSourceTarget(static_cast<PointerRoutedEventArgs*>(pArgs), &isEventSourceTarget));

    if (isEventSourceTarget)
    {
        m_isPointerOverMain = true;
        m_isPressedOnMain = false;
        IFC_RETURN(UpdateVisualState());
    }

    return S_OK;
}

IFACEMETHODIMP CalendarDatePicker::OnPointerMoved(_In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    bool isEventSourceTarget = false;

    IFC_RETURN(CalendarDatePickerGenerated::OnPointerMoved(pArgs));

    IFC_RETURN(IsEventSourceTarget(static_cast<PointerRoutedEventArgs*>(pArgs), &isEventSourceTarget));

    if (isEventSourceTarget)
    {
        if (!m_isPointerOverMain)
        {
            m_isPointerOverMain = true;
            IFC_RETURN(UpdateVisualState());
        }
    }
    else if (m_isPointerOverMain)
    {
        // treat as PointerExited.
        m_isPointerOverMain = false;
        m_isPressedOnMain = false;
        IFC_RETURN(UpdateVisualState());
    }
    return S_OK;
}

IFACEMETHODIMP CalendarDatePicker::OnPointerExited(_In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    IFC_RETURN(CalendarDatePickerGenerated::OnPointerExited(pArgs));

    m_isPointerOverMain = false;
    m_isPressedOnMain = false;

    IFC_RETURN(UpdateVisualState());

    return S_OK;
}

IFACEMETHODIMP CalendarDatePicker::OnGotFocus(_In_ xaml::IRoutedEventArgs* pArgs)
{
    return UpdateVisualState();
}

IFACEMETHODIMP CalendarDatePicker::OnLostFocus(_In_ xaml::IRoutedEventArgs* pArgs)
{
    return UpdateVisualState();
}

IFACEMETHODIMP CalendarDatePicker::OnPointerCaptureLost(
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    ctl::ComPtr<xaml_input::IPointer> spPointer;
    ctl::ComPtr<ixp::IPointerPoint> spPointerPoint;
    mui::PointerDeviceType nPointerDeviceType = mui::PointerDeviceType_Touch;

    IFC_RETURN(CalendarDatePickerGenerated::OnPointerCaptureLost(pArgs));

    IFC_RETURN(pArgs->get_Pointer(&spPointer));

    // For touch, we can clear PointerOver when receiving PointerCaptureLost, which we get when the finger is lifted
    // or from cancellation, e.g. pinch-zoom gesture in ScrollViewer.
    // For mouse, we need to wait for PointerExited because the mouse may still be above the control when
    // PointerCaptureLost is received from clicking.
    IFC_RETURN(pArgs->GetCurrentPoint(nullptr, &spPointerPoint));
    IFCPTR_RETURN(spPointerPoint);
    IFC_RETURN(spPointerPoint->get_PointerDeviceType(&nPointerDeviceType));
    if (nPointerDeviceType == mui::PointerDeviceType_Touch)
    {
        m_isPointerOverMain = false;
    }

    m_isPressedOnMain = false;
    IFC_RETURN(UpdateVisualState());

    return S_OK;
}

// Called when the IsEnabled property changes.
_Check_return_ HRESULT CalendarDatePicker::OnIsEnabledChanged(_In_ IsEnabledChangedEventArgs* pArgs)
{
    return UpdateVisualState();
}

// Perform the primary action related to pointer up.
_Check_return_ HRESULT CalendarDatePicker::PerformPointerUpAction()
{
    if (m_shouldPerformActions)
    {
        m_shouldPerformActions = false;
        IFC_RETURN(put_IsCalendarOpen(TRUE));

        ElementSoundPlayerService* soundPlayerService = DXamlCore::GetCurrent()->GetElementSoundPlayerServiceNoRef();
        IFC_RETURN(soundPlayerService->RequestInteractionSoundForElement(xaml::ElementSoundKind_Invoke, this));
    }

    return S_OK;
}


_Check_return_ HRESULT CalendarDatePicker::IsEventSourceTarget(
    _In_ xaml::IRoutedEventArgs* pArgs,
    _Out_ bool* pIsEventSourceChildOfTarget)
{
    ctl::ComPtr<IInspectable> spOriginalSourceAsI;
    ctl::ComPtr<IDependencyObject> spOriginalSourceAsDO;

    IFC_RETURN(pArgs->get_OriginalSource(&spOriginalSourceAsI));
    IFC_RETURN(spOriginalSourceAsI.As(&spOriginalSourceAsDO));
    IFC_RETURN(IsChildOfTarget(
        spOriginalSourceAsDO.Get(),
        true, /* doCacheResult */
        pIsEventSourceChildOfTarget));

    return S_OK;
}

// Used in hit-testing for the CalendarDatePicker target area, which must exclude the header
_Check_return_ HRESULT CalendarDatePicker::IsChildOfTarget(
    _In_opt_ IDependencyObject* pChild,
    _In_ bool doCacheResult,
    _Out_ bool* pIsChildOfTarget)
{
    // Simple perf optimization: most pointer events have the same source as the previous
    // event, so we'll cache the most recent result and reuse it whenever possible.
    static IDependencyObject* pMostRecentSearchChildNoRef = nullptr;
    static bool mostRecentResult = false;

    *pIsChildOfTarget = false;

    if (pChild)
    {
        bool result = mostRecentResult;
        ctl::ComPtr<IDependencyObject> spHeaderPresenterAsDO;
        ctl::ComPtr<IDependencyObject> spCurrentDO(pChild);
        ctl::ComPtr<IDependencyObject> spParentDO;
        IDependencyObject* pThisAsDONoRef = static_cast<IDependencyObject*>(this);
        bool isFound = false;

        spHeaderPresenterAsDO = m_tpHeaderContentPresenter.AsOrNull<IDependencyObject>();

        while (spCurrentDO && !isFound)
        {
            if (spCurrentDO.Get() == pMostRecentSearchChildNoRef)
            {
                // use the cached result
                isFound = true;
            }
            else if (spCurrentDO.Get() == pThisAsDONoRef)
            {
                // meet the CalendarDatePicker itself, break;
                result = true;
                isFound = true;
            }
            else if (spHeaderPresenterAsDO && spCurrentDO.Get() == spHeaderPresenterAsDO.Get())
            {
                // meet the Header, break;
                result = false;
                isFound = true;
            }
            else
            {
                IFC_RETURN(VisualTreeHelper::GetParentStatic(spCurrentDO.Get(), &spParentDO));

                // refcounting note: Attach releases the previously stored ptr, and does not
                // addref the new one.
                spCurrentDO.Attach(spParentDO.Detach());
            }
        }

        if (!isFound)
        {
            result = false;
        }

        if (doCacheResult)
        {
            pMostRecentSearchChildNoRef = pChild;
            mostRecentResult = result;
        }

        *pIsChildOfTarget = result;
    }

    return S_OK;
}

_Check_return_ HRESULT CalendarDatePicker::SyncDate()
{
    if (m_tpCalendarView)
    {
        BOOLEAN isCalendarOpen = false;

        IFC_RETURN(get_IsCalendarOpen(&isCalendarOpen));
        if (isCalendarOpen)
        {
            ctl::ComPtr<wf::IReference<wf::DateTime>> spDateReference;
            ctl::ComPtr<wfc::IVector<wf::DateTime>> spSelectedDates;

            m_isSelectedDatesChangingInternally = true;
            auto selectedDatesChangingGuard = wil::scope_exit([&] { m_isSelectedDatesChangingInternally = false; });

            IFC_RETURN(get_Date(&spDateReference));

            IFC_RETURN(m_tpCalendarView->get_SelectedDates(&spSelectedDates));
            IFC_RETURN(spSelectedDates->Clear());

            if (spDateReference)
            {
                wf::DateTime date;

                IFC_RETURN(spDateReference->get_Value(&date));
                // if Date property is being set, we should always display the Date when Calendar is open.
                IFC_RETURN(SetDisplayDate(date));
                IFC_RETURN(spSelectedDates->Append(date));
            }
        }
    }

    return S_OK;
}

IFACEMETHODIMP CalendarDatePicker::OnCreateAutomationPeer(_Outptr_result_maybenull_ xaml_automation_peers::IAutomationPeer** ppAutomationPeer)
{
    IFCPTR_RETURN(ppAutomationPeer);
    *ppAutomationPeer = nullptr;

    ctl::ComPtr<CalendarDatePickerAutomationPeer> spAutomationPeer;
    IFC_RETURN(ActivationAPI::ActivateAutomationInstance(KnownTypeIndex::CalendarDatePickerAutomationPeer, GetHandle(), spAutomationPeer.GetAddressOf()));
    IFC_RETURN(spAutomationPeer->put_Owner(this));
    *ppAutomationPeer = spAutomationPeer.Detach();
    return S_OK;
}

_Check_return_ HRESULT CalendarDatePicker::GetCurrentFormattedDate(_Out_ HSTRING* value)
{
    *value = nullptr;

    if (m_tpDateText)
    {
        IFC_RETURN(m_tpDateText->get_Text(value));
    }
    return S_OK;
}