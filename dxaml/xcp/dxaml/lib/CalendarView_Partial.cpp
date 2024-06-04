// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "CalendarView.g.h"
#include "TrackableDateCollection.h"
#include "CalendarViewGeneratorHost.h"
#include "CalendarViewGeneratorMonthViewHost.h"
#include "CalendarViewGeneratorYearViewHost.h"
#include "CalendarViewGeneratorDecadeViewHost.h"
#include "CalendarViewHeaderAutomationPeer_Partial.h"
#include "CalendarPanel_Partial.h"
#include "CalendarViewTemplateSettings.g.h"
#include "CalendarViewBaseItem.g.h"
#include "CalendarViewDayItem.g.h"
#include "CalendarViewItem.g.h"
#include "Button.g.h"
#include "Grid.g.h"
#include "ScrollViewer.g.h"
#include "InternalStringCollection.h"
#include "CalendarViewAutomationPeer.g.h"
#include "DateComparer.h"
#include "TextBlock.g.h"
#include "AutomationProperties.h"
#include "localizedResource.h"
#include <windows.globalization.datetimeformatting.h>

using namespace DirectUI;
using namespace DirectUISynonyms;

#undef max
#undef min

#define RTL_CHARACTER_CODE 8207

CalendarView::CalendarView()
    : m_dateSourceChanged(true)
    , m_calendarChanged(false)
    , m_itemHostsConnected(false)
    , m_areYearDecadeViewDimensionsSet(false)
    , m_colsInYearDecadeView(4)
    , m_rowsInYearDecadeView(4)
    , m_monthViewStartIndex(0)
    , m_weekDayOfMinDate(wg::DayOfWeek_Sunday)
    , m_isSelectedDatesChangingInternally(false)
    , m_focusItemAfterDisplayModeChanged(false)
    , m_focusStateAfterDisplayModeChanged(xaml::FocusState::FocusState_Programmatic)
    , m_isMultipleEraCalendar(false)
    , m_areDirectManipulationStateChangeHandlersHooked(false)
    , m_isSetDisplayDateRequested(true) // by default there is a displayDate request, which is m_lastDisplayedDate
    , m_isNavigationButtonClicked(false)
{
    m_today.UniversalTime = 0;
    m_maxDate.UniversalTime = 0;
    m_minDate.UniversalTime = 0;
    m_lastDisplayedDate.UniversalTime = 0;
}

CalendarView::~CalendarView()
{
    VERIFYHR(DetachButtonClickedEvents());
    VERIFYHR(DetachHandler(m_epSelectedDatesChangedHandler, m_tpSelectedDates));
    VERIFYHR(DetachScrollViewerKeyDownEvents());

    ctl::ComPtr<wfc::IVector<wf::DateTime>> selectedDates;
    if (m_tpSelectedDates.TryGetSafeReference(&selectedDates))
    {
        selectedDates.Cast<TrackableDateCollection>()->SetCollectionChangingCallback(nullptr);
    }
}

_Check_return_ HRESULT CalendarView::PrepareState()
{
    HRESULT hr = S_OK;

    IFC(CalendarViewGenerated::PrepareState());

    {
        m_dateComparer.reset(new DateComparer());

        ctl::ComPtr<TrackableDateCollection> spSelectedDates;

        IFC(ctl::make(&spSelectedDates));

        IFC(m_epSelectedDatesChangedHandler.AttachEventHandler(spSelectedDates.Get(),
            [this](wfc::IObservableVector<wf::DateTime>* pSender, wfc::IVectorChangedEventArgs* pArgs)
        {
            return OnSelectedDatesChanged(pSender, pArgs);
        }));

        spSelectedDates->SetCollectionChangingCallback(
            [this](_In_ TrackableDateCollection_CollectionChanging action, _In_ wf::DateTime addingDate)
        {
            return OnSelectedDatesChanging(action, addingDate);
        });

        SetPtrValue(m_tpSelectedDates, spSelectedDates);
        IFC(put_SelectedDates(spSelectedDates.Get()));
    }

    {
        ctl::ComPtr<CalendarViewGeneratorMonthViewHost> spMonthViewItemHost;
        ctl::ComPtr<CalendarViewGeneratorYearViewHost> spYearViewItemHost;
        ctl::ComPtr<CalendarViewGeneratorDecadeViewHost> spDecadeViewItemHost;

        IFC(ctl::make(&spMonthViewItemHost));
        SetPtrValue(m_tpMonthViewItemHost, spMonthViewItemHost);
        m_tpMonthViewItemHost->SetOwner(this);

        IFC(ctl::make(&spYearViewItemHost));
        SetPtrValue(m_tpYearViewItemHost, spYearViewItemHost);
        m_tpYearViewItemHost->SetOwner(this);

        IFC(ctl::make(&spDecadeViewItemHost));
        SetPtrValue(m_tpDecadeViewItemHost, spDecadeViewItemHost);
        m_tpDecadeViewItemHost->SetOwner(this);
    }

    {
        IFC(CreateCalendarLanguages());
        IFC(CreateCalendarAndMonthYearFormatter());
    }

    {
        ctl::ComPtr<CalendarViewTemplateSettings> spTemplateSettings;

        IFC(ctl::make(&spTemplateSettings));

        IFC(spTemplateSettings->put_HasMoreViews(TRUE));
        IFC(put_TemplateSettings(spTemplateSettings.Get()));
        SetPtrValue(m_tpTemplateSettings, spTemplateSettings);
    }

Cleanup:
    return hr;
}

// Override the GetDefaultValue method to return the default values
// for Hub dependency properties.
_Check_return_ HRESULT CalendarView::GetDefaultValue2(
    _In_ const CDependencyProperty* pDP,
    _Out_ CValue* pValue)
{
    HRESULT hr = S_OK;

    IFCPTR(pDP);
    IFCPTR(pValue);

    switch (pDP->GetIndex())
    {
    case KnownPropertyIndex::CalendarView_CalendarIdentifier:
        IFC(pValue->SetString(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"GregorianCalendar")).Get()));
        break;
    case KnownPropertyIndex::CalendarView_NumberOfWeeksInView:
        pValue->SetSigned(s_defaultNumberOfWeeks);
        break;
    default:
        IFC(CalendarViewGenerated::GetDefaultValue2(pDP, pValue));
        break;
    }

Cleanup:
    return hr;
}

// Basically these Alignment properties only affect Arrange, but in CalendarView
// the item size and Panel size are also affected when we change the property from
// stretch to unstretch, or vice versa. In these cases we need to invalidate panels' measure.
_Check_return_ HRESULT CalendarView::OnAlignmentChanged(const PropertyChangedParams& args)
{
    XUINT32 oldAlignment = 0;
    XUINT32 newAlignment = 0;
    bool isOldStretched = false;
    bool isNewStretched = false;

    IFC_RETURN(args.m_pOldValue->GetEnum(oldAlignment));
    IFC_RETURN(args.m_pNewValue->GetEnum(newAlignment));

    switch (args.m_pDP->GetIndex())
    {
    case KnownPropertyIndex::Control_HorizontalContentAlignment:
    case KnownPropertyIndex::FrameworkElement_HorizontalAlignment:
        isOldStretched = static_cast<xaml::HorizontalAlignment>(oldAlignment) == xaml::HorizontalAlignment_Stretch;
        isNewStretched = static_cast<xaml::HorizontalAlignment>(newAlignment) == xaml::HorizontalAlignment_Stretch;
        break;
    case KnownPropertyIndex::Control_VerticalContentAlignment:
    case KnownPropertyIndex::FrameworkElement_VerticalAlignment:
        isOldStretched = static_cast<xaml::VerticalAlignment>(oldAlignment) == xaml::VerticalAlignment_Stretch;
        isNewStretched = static_cast<xaml::VerticalAlignment>(newAlignment) == xaml::VerticalAlignment_Stretch;
        break;
    default:
        ASSERT(false);
        break;
    }

    if (isOldStretched != isNewStretched)
    {
        IFC_RETURN(ForeachHost([](CalendarViewGeneratorHost* pHost)
        {
            auto pPanel = pHost->GetPanel();
            if (pPanel)
            {
                IFC_RETURN(pPanel->InvalidateMeasure());
            }
            return S_OK;
        }));
    }

    return S_OK;
}

// Handle the custom property changed event and call the OnPropertyChanged methods.
_Check_return_ HRESULT CalendarView::OnPropertyChanged2(
    _In_ const PropertyChangedParams& args)
{
    IFC_RETURN(CalendarViewGenerated::OnPropertyChanged2(args));

    switch (args.m_pDP->GetIndex())
    {
    case KnownPropertyIndex::Control_HorizontalContentAlignment:
    case KnownPropertyIndex::Control_VerticalContentAlignment:
    case KnownPropertyIndex::FrameworkElement_HorizontalAlignment:
    case KnownPropertyIndex::FrameworkElement_VerticalAlignment:
        IFC_RETURN(OnAlignmentChanged(args));
        break;
    case KnownPropertyIndex::CalendarView_MinDate:
    case KnownPropertyIndex::CalendarView_MaxDate:
        m_dateSourceChanged = true;
        IFC_RETURN(InvalidateMeasure());
        break;
    case KnownPropertyIndex::FrameworkElement_Language:
        // Globlization::Calendar doesn't support changing languages, so when languages changed,
        // we have to create a new Globalization::Calendar, and also we'll update the date source so
        // the change of languages can take effect on the existing items.
        IFC_RETURN(CreateCalendarLanguages());
        // fall through
    case KnownPropertyIndex::CalendarView_CalendarIdentifier:
        m_calendarChanged = true;
        m_dateSourceChanged = true; //calendarid changed, even if the mindate or maxdate is not changed we still need to regenerate all calendar items.
        IFC_RETURN(InvalidateMeasure());
        break;
    case KnownPropertyIndex::CalendarView_NumberOfWeeksInView:
        {
            int rows = 0;
            IFC_RETURN(args.m_pNewValue->GetSigned(rows));

            if (rows < s_minNumberOfWeeks || rows > s_maxNumberOfWeeks)
            {
                IFC_RETURN(ErrorHelper::OriginateErrorUsingResourceID(E_FAIL, ERROR_CALENDAR_NUMBER_OF_WEEKS_OUTOFRANGE));
            }

            if (m_tpMonthViewItemHost->GetPanel())
            {
                IFC_RETURN(m_tpMonthViewItemHost->GetPanel()->SetSuggestedDimension(s_numberOfDaysInWeek, rows));
            }
        }
        break;
    case KnownPropertyIndex::CalendarView_DayOfWeekFormat:
        IFC_RETURN(FormatWeekDayNames());
        // fall through
    case KnownPropertyIndex::CalendarView_FirstDayOfWeek:
        IFC_RETURN(UpdateWeekDayNames());
        break;
    case KnownPropertyIndex::CalendarView_SelectionMode:
        IFC_RETURN(OnSelectionModeChanged());
        break;
    case KnownPropertyIndex::CalendarView_IsOutOfScopeEnabled:
        IFC_RETURN(OnIsOutOfScopePropertyChanged());
        break;
    case KnownPropertyIndex::CalendarView_DisplayMode:
    {
        XUINT32 oldDisplayMode = 0;
        XUINT32 newDisplayMode = 0;

        IFC_RETURN(args.m_pOldValue->GetEnum(oldDisplayMode));
        IFC_RETURN(args.m_pNewValue->GetEnum(newDisplayMode));

        IFC_RETURN(OnDisplayModeChanged(
            static_cast<xaml_controls::CalendarViewDisplayMode>(oldDisplayMode),
            static_cast<xaml_controls::CalendarViewDisplayMode>(newDisplayMode)
            ));
    }
        break;
    case KnownPropertyIndex::CalendarView_IsTodayHighlighted:
        IFC_RETURN(OnIsTodayHighlightedPropertyChanged());
        break;
    case KnownPropertyIndex::CalendarView_IsGroupLabelVisible:
        IFC_RETURN(OnIsLabelVisibleChanged());
        break;

        // To reduce memory usage, we move lots font/brush properties from CalendarViewItem to CalendarView,
        // the cost is we can't benefit from property system to invalidate measure/render automatically.
        // However changing these font/brush properties is not a frequent scenario. So once they are changed
        // we'll manually update the items.
        // Basically we should only update those affected items (e.g. when PressedBackground changed, we should only update
        // the item which is being pressed) but to make the code simple we'll update all realized items, unless
        // we see performance issue here.

        // Border brushes and Background (they are chromes) will take effect in next Render walk.
    case KnownPropertyIndex::CalendarView_FocusBorderBrush:
    case KnownPropertyIndex::CalendarView_SelectedHoverBorderBrush:
    case KnownPropertyIndex::CalendarView_SelectedPressedBorderBrush:
    case KnownPropertyIndex::CalendarView_SelectedDisabledBorderBrush:
    case KnownPropertyIndex::CalendarView_SelectedBorderBrush:
    case KnownPropertyIndex::CalendarView_HoverBorderBrush:
    case KnownPropertyIndex::CalendarView_PressedBorderBrush:
    case KnownPropertyIndex::CalendarView_CalendarItemBorderBrush:
    case KnownPropertyIndex::CalendarView_BlackoutBackground:
    case KnownPropertyIndex::CalendarView_BlackoutStrikethroughBrush:
    case KnownPropertyIndex::CalendarView_OutOfScopeBackground:
    case KnownPropertyIndex::CalendarView_CalendarItemBackground:
    case KnownPropertyIndex::CalendarView_TodaySelectedInnerBorderBrush:
    case KnownPropertyIndex::CalendarView_TodayBackground:
    case KnownPropertyIndex::CalendarView_TodayBlackoutBackground:
    case KnownPropertyIndex::CalendarView_TodayHoverBackground:
    case KnownPropertyIndex::CalendarView_TodayPressedBackground:
    case KnownPropertyIndex::CalendarView_TodayDisabledBackground:
    case KnownPropertyIndex::CalendarView_CalendarItemHoverBackground:
    case KnownPropertyIndex::CalendarView_CalendarItemPressedBackground:
    case KnownPropertyIndex::CalendarView_CalendarItemDisabledBackground:
        IFC_RETURN(ForeachHost([this](CalendarViewGeneratorHost* pHost)
        {
            IFC_RETURN(ForeachChildInPanel(pHost->GetPanel(),
                [](_In_ CalendarViewBaseItem* pItem)
            {
                IFC_RETURN(pItem->UpdateBackgroundAndBorderBrushes());
                return pItem->InvalidateRender();
            }));
            return S_OK;
        }));
        break;

    case KnownPropertyIndex::CalendarView_CalendarItemCornerRadius:
        IFC_RETURN(ForeachHost([this](CalendarViewGeneratorHost* host)
        {
            IFC_RETURN(ForeachChildInPanel(host->GetPanel(),
                [](_In_ CalendarViewBaseItem* item)
            {
                return item->UpdateCornerRadius();
            }));
            return S_OK;
        }));
        break;

        // Foreground will take effect immediately
    case KnownPropertyIndex::CalendarView_DisabledForeground:
    case KnownPropertyIndex::CalendarView_PressedForeground:
    case KnownPropertyIndex::CalendarView_TodayForeground:
    case KnownPropertyIndex::CalendarView_BlackoutForeground:
    case KnownPropertyIndex::CalendarView_TodayBlackoutForeground:
    case KnownPropertyIndex::CalendarView_SelectedForeground:
    case KnownPropertyIndex::CalendarView_SelectedHoverForeground:
    case KnownPropertyIndex::CalendarView_SelectedPressedForeground:
    case KnownPropertyIndex::CalendarView_SelectedDisabledForeground:
    case KnownPropertyIndex::CalendarView_OutOfScopeForeground:
    case KnownPropertyIndex::CalendarView_OutOfScopeHoverForeground:
    case KnownPropertyIndex::CalendarView_OutOfScopePressedForeground:
    case KnownPropertyIndex::CalendarView_CalendarItemForeground:
        IFC_RETURN(ForeachHost([this](CalendarViewGeneratorHost* pHost)
        {
            IFC_RETURN(ForeachChildInPanel(pHost->GetPanel(),
                [](_In_ CalendarViewBaseItem* pItem)
            {
                IFC_RETURN(pItem->UpdateTextBlockForeground());
                return pItem->UpdateTextBlockForeground();
            }));
            return S_OK;
        }));
        break;

    case KnownPropertyIndex::CalendarView_TodayFontWeight:
    {
        IFC_RETURN(ForeachHost([this](CalendarViewGeneratorHost* pHost)
        {
            auto pPanel = pHost->GetPanel();

            if (pPanel)
            {
                int indexOfToday = -1;

                IFC_RETURN(pHost->CalculateOffsetFromMinDate(m_today, &indexOfToday));

                if (indexOfToday != -1)
                {
                    ctl::ComPtr<IDependencyObject> spChildAsIDO;
                    ctl::ComPtr<ICalendarViewBaseItem> spChildAsI;

                    IFC_RETURN(pPanel->ContainerFromIndex(indexOfToday, &spChildAsIDO));
                    spChildAsI = spChildAsIDO.AsOrNull<ICalendarViewBaseItem>();
                    // today item is realized already, we need to update the state here.
                    // if today item is not realized yet, we'll update the state when today item is being prepared.
                    if (spChildAsI)
                    {
                        ctl::ComPtr<CalendarViewBaseItem> spChild;

                        spChild = spChildAsI.Cast<CalendarViewBaseItem>();
                        IFC_RETURN(spChild->UpdateTextBlockFontProperties());
                    }
                }
            }
            return S_OK;
        }));

        break;
    }

        // Font properties for DayItem (affect measure and arrange)
    case KnownPropertyIndex::CalendarView_DayItemFontFamily:
    case KnownPropertyIndex::CalendarView_DayItemFontSize:
    case KnownPropertyIndex::CalendarView_DayItemFontStyle:
    case KnownPropertyIndex::CalendarView_DayItemFontWeight:
    {
        // if these DayItem properties changed, we need to re-determine the
        // biggest dayitem in monthPanel, which will invalidate monthpanel's measure
        auto pMonthPanel = m_tpMonthViewItemHost->GetPanel();
        if (pMonthPanel)
        {
            IFC_RETURN(pMonthPanel->SetNeedsToDetermineBiggestItemSize());
        }
    }
        // fall through

        // Font properties for MonthLabel (they won't affect measure or arrange)
    case KnownPropertyIndex::CalendarView_FirstOfMonthLabelFontFamily:
    case KnownPropertyIndex::CalendarView_FirstOfMonthLabelFontSize:
    case KnownPropertyIndex::CalendarView_FirstOfMonthLabelFontStyle:
    case KnownPropertyIndex::CalendarView_FirstOfMonthLabelFontWeight:
        IFC_RETURN(ForeachChildInPanel(m_tpMonthViewItemHost->GetPanel(),
            [](_In_ CalendarViewBaseItem* pItem)
        {
            IFC_RETURN(pItem->UpdateBlackoutStrikethroughSize());
            return pItem->UpdateTextBlockFontProperties();
        }));
        break;

        // Font properties for MonthYearItem
    case KnownPropertyIndex::CalendarView_MonthYearItemFontFamily:
    case KnownPropertyIndex::CalendarView_MonthYearItemFontSize:
    case KnownPropertyIndex::CalendarView_MonthYearItemFontStyle:
    case KnownPropertyIndex::CalendarView_MonthYearItemFontWeight:
    {
        // these properties will affect MonthItem and YearItem's size, so we should
        // tell their panels to re-determine the biggest item size.
        std::array<CalendarPanel*, 2> pPanels{ { m_tpYearViewItemHost->GetPanel(), m_tpDecadeViewItemHost->GetPanel() } };

        for (unsigned i = 0; i < pPanels.size(); ++i)
        {
            if (pPanels[i])
            {
                IFC_RETURN(pPanels[i]->SetNeedsToDetermineBiggestItemSize());
            }
        }
    }
        // fall through

        // Font properties for  year and decade views
    case KnownPropertyIndex::CalendarView_FirstOfYearDecadeLabelFontFamily:
    case KnownPropertyIndex::CalendarView_FirstOfYearDecadeLabelFontSize:
    case KnownPropertyIndex::CalendarView_FirstOfYearDecadeLabelFontStyle:
    case KnownPropertyIndex::CalendarView_FirstOfYearDecadeLabelFontWeight:
    {
        std::array<CalendarPanel*, 2> pPanels{ { m_tpYearViewItemHost->GetPanel(), m_tpDecadeViewItemHost->GetPanel() } };

        for (unsigned i = 0; i < pPanels.size(); ++i)
        {
            IFC_RETURN(ForeachChildInPanel(pPanels[i],
                [](_In_ CalendarViewBaseItem* pItem)
            {
                IFC_RETURN(pItem->UpdateBlackoutStrikethroughSize());
                return pItem->UpdateTextBlockFontProperties();
            }));

        }
        break;
    }

        // Alignments affect DayItem only
    case KnownPropertyIndex::CalendarView_HorizontalDayItemAlignment:
    case KnownPropertyIndex::CalendarView_VerticalDayItemAlignment:
    case KnownPropertyIndex::CalendarView_HorizontalFirstOfMonthLabelAlignment:
    case KnownPropertyIndex::CalendarView_VerticalFirstOfMonthLabelAlignment:

        IFC_RETURN(ForeachChildInPanel(m_tpMonthViewItemHost->GetPanel(),
            [](_In_ CalendarViewBaseItem* pItem)
        {
            return pItem->UpdateTextBlockAlignments();
        }));

        break;

        // Update main or group labels margin
    case KnownPropertyIndex::CalendarView_DayItemMargin:
    case KnownPropertyIndex::CalendarView_FirstOfMonthLabelMargin:
        IFC_RETURN(ForeachChildInPanel(m_tpMonthViewItemHost->GetPanel(),
            [](_In_ CalendarViewBaseItem* item)
            {
                return item->UpdateTextBlockMargin();
            }));
        break;

        // Update main or group labels margin
    case KnownPropertyIndex::CalendarView_MonthYearItemMargin:
    case KnownPropertyIndex::CalendarView_FirstOfYearDecadeLabelMargin:
    {
        std::array<CalendarPanel*, 2> panels{ { m_tpYearViewItemHost->GetPanel(), m_tpDecadeViewItemHost->GetPanel() } };

        for (unsigned i = 0; i < panels.size(); ++i)
        {
            IFC_RETURN(ForeachChildInPanel(panels[i],
                [](_In_ CalendarViewBaseItem* item)
                {
                    return item->UpdateTextBlockMargin();
                }));

        }
        break;
    }

        // border thickness affects measure (and arrange)
    case KnownPropertyIndex::CalendarView_CalendarItemBorderThickness:
        IFC_RETURN(ForeachHost([this](CalendarViewGeneratorHost* pHost)
        {
            IFC_RETURN(ForeachChildInPanel(pHost->GetPanel(),
                [](_In_ CalendarViewBaseItem* pItem)
            {
                IFC_RETURN(pItem->UpdateBackgroundAndBorderBrushes());
                IFC_RETURN(pItem->UpdateCornerRadius());
                return pItem->InvalidateMeasure();
            }));
            return S_OK;
        }));
        break;

        // Dayitem style changed, update style for all existing day items.
    case KnownPropertyIndex::CalendarView_CalendarViewDayItemStyle:
    {
        ctl::ComPtr<IStyle> spStyle;

        IFC_RETURN(ctl::do_query_interface(spStyle, args.m_pNewValueOuterNoRef));
        auto pMonthPanel = m_tpMonthViewItemHost->GetPanel();

        IFC_RETURN(ForeachChildInPanel(pMonthPanel,
            [spStyle](_In_ CalendarViewBaseItem* pItem)
        {
            return CalendarView::SetDayItemStyle(pItem, spStyle.Get());
        }));

        // Some properties could affect dayitem size (e.g. Dayitem font properties, dayitem size),
        // when anyone of them is changed, we need to re-determine the biggest day item.
        // This is not a frequent scenario so we can simply set below flag and invalidate measure.

        if (pMonthPanel)
        {
            IFC_RETURN(pMonthPanel->SetNeedsToDetermineBiggestItemSize());
        }

        break;
    }
    }

    return S_OK;
}

IFACEMETHODIMP
CalendarView::OnApplyTemplate()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_primitives::ICalendarPanel> spMonthViewPanel;
    ctl::ComPtr<xaml_primitives::ICalendarPanel> spYearViewPanel;
    ctl::ComPtr<xaml_primitives::ICalendarPanel> spDecadeViewPanel;
    ctl::ComPtr<xaml_controls::IButton> spHeaderButton;
    ctl::ComPtr<xaml_controls::IButton> spPreviousButton;
    ctl::ComPtr<xaml_controls::IButton> spNextButton;
    ctl::ComPtr<xaml_controls::IGrid> spViewsGrid;
    ctl::ComPtr<xaml_controls::IScrollViewer> spMonthViewScrollViewer;
    ctl::ComPtr<xaml_controls::IScrollViewer> spYearViewScrollViewer;
    ctl::ComPtr<xaml_controls::IScrollViewer> spDecadeViewScrollViewer;
    wrl_wrappers::HString strAutomationName;

#ifdef DBG
    if (!m_roundedCalendarViewBaseItemChromeFallbackPropertiesSet)
    {
        m_roundedCalendarViewBaseItemChromeFallbackPropertiesSet = true;
        SetRoundedCalendarViewBaseItemChromeFallbackProperties();
    }
#endif // DBG

    IFC(DetachVisibleIndicesUpdatedEvents());
    IFC(DetachButtonClickedEvents());
    IFC(DetachScrollViewerFocusEngagedEvents());
    IFC(DetachScrollViewerKeyDownEvents());

    // This will clean up the panels and clear the children
    IFC(DisconnectItemHosts());

    if (m_areDirectManipulationStateChangeHandlersHooked)
    {
        m_areDirectManipulationStateChangeHandlersHooked = false;
        IFC(ForeachHost([](CalendarViewGeneratorHost* pHost)
        {
            HRESULT hr = S_OK;

            auto pScrollViewer = pHost->GetScrollViewer();
            if (pScrollViewer)
            {
                IFC(pScrollViewer->SetDirectManipulationStateChangeHandler(nullptr));
            }

        Cleanup:
            return hr;
        }));
    }

    IFC(ForeachHost([](CalendarViewGeneratorHost* pHost)
    {
        HRESULT hr = S_OK;

        IFC(pHost->SetPanel(nullptr));
        IFC(pHost->SetScrollViewer(nullptr));

    Cleanup:
        return hr;
    }));


    m_tpHeaderButton.Clear();
    m_tpPreviousButton.Clear();
    m_tpNextButton.Clear();
    m_tpViewsGrid.Clear();

    IFC(CalendarViewGenerated::OnApplyTemplate());

    IFC(GetTemplatePart<xaml_primitives::ICalendarPanel>(STR_LEN_PAIR(L"MonthViewPanel"), spMonthViewPanel.ReleaseAndGetAddressOf()));
    IFC(GetTemplatePart<xaml_primitives::ICalendarPanel>(STR_LEN_PAIR(L"YearViewPanel"), spYearViewPanel.ReleaseAndGetAddressOf()));
    IFC(GetTemplatePart<xaml_primitives::ICalendarPanel>(STR_LEN_PAIR(L"DecadeViewPanel"), spDecadeViewPanel.ReleaseAndGetAddressOf()));

    IFC(m_tpMonthViewItemHost->SetPanel(spMonthViewPanel.Get()));
    IFC(m_tpYearViewItemHost->SetPanel(spYearViewPanel.Get()));
    IFC(m_tpDecadeViewItemHost->SetPanel(spDecadeViewPanel.Get()));

    if (spMonthViewPanel)
    {
        CalendarPanel* pPanel = spMonthViewPanel.Cast<CalendarPanel>();
        int numberOfWeeksInView = 0;

        // MonthView panel is the only primary panel (and never changes)
        IFC(pPanel->SetPanelType(CalendarPanel::CalendarPanelType::CalendarPanelType_Primary));

        IFC(get_NumberOfWeeksInView(&numberOfWeeksInView));
        IFC(pPanel->SetSuggestedDimension(s_numberOfDaysInWeek, numberOfWeeksInView));
        IFC(pPanel->put_Orientation(xaml_controls::Orientation_Horizontal));
    }

    if (spYearViewPanel)
    {
        CalendarPanel* pPanel = spYearViewPanel.Cast<CalendarPanel>();

        // YearView panel is a Secondary_SelfAdaptive panel by default
        if (!m_areYearDecadeViewDimensionsSet)
        {
            IFC(pPanel->SetPanelType(CalendarPanel::CalendarPanelType::CalendarPanelType_Secondary_SelfAdaptive));
        }

        IFC(pPanel->SetSuggestedDimension(m_colsInYearDecadeView, m_rowsInYearDecadeView));
        IFC(pPanel->put_Orientation(xaml_controls::Orientation_Horizontal));
    }

    if (spDecadeViewPanel)
    {
        CalendarPanel* pPanel = spDecadeViewPanel.Cast<CalendarPanel>();

        // DecadeView panel is a Secondary_SelfAdaptive panel by default
        if (!m_areYearDecadeViewDimensionsSet)
        {
            IFC(pPanel->SetPanelType(CalendarPanel::CalendarPanelType::CalendarPanelType_Secondary_SelfAdaptive));
        }

        IFC(pPanel->SetSuggestedDimension(m_colsInYearDecadeView, m_rowsInYearDecadeView));
        IFC(pPanel->put_Orientation(xaml_controls::Orientation_Horizontal));
    }

    IFC(GetTemplatePart<IButton>(STR_LEN_PAIR(L"HeaderButton"), spHeaderButton.ReleaseAndGetAddressOf()));
    IFC(GetTemplatePart<IButton>(STR_LEN_PAIR(L"PreviousButton"), spPreviousButton.ReleaseAndGetAddressOf()));
    IFC(GetTemplatePart<IButton>(STR_LEN_PAIR(L"NextButton"), spNextButton.ReleaseAndGetAddressOf()));

    if (spPreviousButton)
    {
        IFC(DirectUI::AutomationProperties::GetNameStatic(spPreviousButton.Cast<Button>(), strAutomationName.ReleaseAndGetAddressOf()))
        if (strAutomationName.Get() == nullptr)
        {
            // USe the same resource string as for FlipView's Previous Button.
            IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_FLIPVIEW_PREVIOUS, strAutomationName.ReleaseAndGetAddressOf()));
            IFC(DirectUI::AutomationProperties::SetNameStatic(spPreviousButton.Cast<Button>(), strAutomationName.Get()))
        }
    }

    if (spNextButton)
    {
        IFC(DirectUI::AutomationProperties::GetNameStatic(spNextButton.Cast<Button>(), strAutomationName.ReleaseAndGetAddressOf()))
        if (strAutomationName.Get() == nullptr)
        {
            // USe the same resource string as for FlipView's Next Button.
            IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_FLIPVIEW_NEXT, strAutomationName.ReleaseAndGetAddressOf()));
            IFC(DirectUI::AutomationProperties::SetNameStatic(spNextButton.Cast<Button>(), strAutomationName.Get()))
        }
    }

    SetPtrValue(m_tpHeaderButton, spHeaderButton.Get());
    SetPtrValue(m_tpPreviousButton, spPreviousButton.Get());
    SetPtrValue(m_tpNextButton, spNextButton.Get());

    IFC(GetTemplatePart<IGrid>(STR_LEN_PAIR(L"Views"), spViewsGrid.ReleaseAndGetAddressOf()));
    SetPtrValue(m_tpViewsGrid, spViewsGrid.Get());

    IFC(GetTemplatePart<IScrollViewer>(STR_LEN_PAIR(L"MonthViewScrollViewer"), spMonthViewScrollViewer.ReleaseAndGetAddressOf()));
    IFC(GetTemplatePart<IScrollViewer>(STR_LEN_PAIR(L"YearViewScrollViewer"), spYearViewScrollViewer.ReleaseAndGetAddressOf()));
    IFC(GetTemplatePart<IScrollViewer>(STR_LEN_PAIR(L"DecadeViewScrollViewer"), spDecadeViewScrollViewer.ReleaseAndGetAddressOf()));

    IFC(m_tpMonthViewItemHost->SetScrollViewer(spMonthViewScrollViewer.Get()));
    IFC(m_tpYearViewItemHost->SetScrollViewer(spYearViewScrollViewer.Get()));
    IFC(m_tpDecadeViewItemHost->SetScrollViewer(spDecadeViewScrollViewer.Get()));

    // Setting custom CalendarScrollViewerAutomationPeer for these ScrollViewers to be the default one.
    if (spMonthViewScrollViewer)
    {
        IFC(spMonthViewScrollViewer.Cast<ScrollViewer>()->put_AutomationPeerFactoryIndex(static_cast<INT>(KnownTypeIndex::CalendarScrollViewerAutomationPeer)));

        SetPtrValue(m_tpMonthViewScrollViewer, spMonthViewScrollViewer.Get());
    }
    if (spYearViewScrollViewer)
    {
        IFC(spYearViewScrollViewer.Cast<ScrollViewer>()->put_AutomationPeerFactoryIndex(static_cast<INT>(KnownTypeIndex::CalendarScrollViewerAutomationPeer)));

        SetPtrValue(m_tpYearViewScrollViewer, spYearViewScrollViewer.Get());
    }
    if (spDecadeViewScrollViewer)
    {
        IFC(spDecadeViewScrollViewer.Cast<ScrollViewer>()->put_AutomationPeerFactoryIndex(static_cast<INT>(KnownTypeIndex::CalendarScrollViewerAutomationPeer)));

        SetPtrValue(m_tpDecadeViewScrollViewer, spDecadeViewScrollViewer.Get());
    }

    ASSERT(!m_areDirectManipulationStateChangeHandlersHooked);

    IFC(ForeachHost([](CalendarViewGeneratorHost* pHost)
    {
        HRESULT hr = S_OK;
        auto pScrollViewer = pHost->GetScrollViewer();
        if (pScrollViewer)
        {
            pScrollViewer->put_TemplatedParentHandlesScrolling(TRUE);
            IFC(pScrollViewer->SetDirectManipulationStateChangeHandler(pHost));
            pScrollViewer->m_templatedParentHandlesMouseButton = TRUE;
        }
    Cleanup:
        return hr;
    }));

    m_areDirectManipulationStateChangeHandlersHooked = true;

    IFC(AttachVisibleIndicesUpdatedEvents());

    IFC(AttachButtonClickedEvents());

    IFC(AttachScrollViewerKeyDownEvents());

    // This will connect the new panels with ItemHosts
    IFC(RegisterItemHosts());

    IFC(AttachScrollViewerFocusEngagedEvents());

    IFC(UpdateVisualState(FALSE /*bUseTransitions*/));

    IFC(UpdateFlowDirectionForView());

Cleanup:
    return hr;
}

// Change to the correct visual state for the control.
_Check_return_ HRESULT CalendarView::ChangeVisualState(
    _In_ bool bUseTransitions)
{
    xaml_controls::CalendarViewDisplayMode mode = xaml_controls::CalendarViewDisplayMode_Month;
    BOOLEAN bIgnored = FALSE;

    IFC_RETURN(get_DisplayMode(&mode));

    if (mode == xaml_controls::CalendarViewDisplayMode_Month)
    {
        IFC_RETURN(GoToState(bUseTransitions, L"Month", &bIgnored));
    }
    else if (mode == xaml_controls::CalendarViewDisplayMode_Year)
    {
        IFC_RETURN(GoToState(bUseTransitions, L"Year", &bIgnored));
    }
    else //if (mode == xaml_controls::CalendarViewDisplayMode_Decade)
    {
        IFC_RETURN(GoToState(bUseTransitions, L"Decade", &bIgnored));
    }

    BOOLEAN isEnabled = FALSE;
    IFC_RETURN(get_IsEnabled(&isEnabled));

    // Common States Group
    if (!isEnabled)
    {
        IFC_RETURN(GoToState(bUseTransitions, L"Disabled", &bIgnored));
    }
    else
    {
        IFC_RETURN(GoToState(bUseTransitions, L"Normal", &bIgnored));
    }

    return S_OK;
}

// Primary panel will determine CalendarView's size, when Primary Panel's desired size changed, we need
// to update the template settings so other template parts can update their size correspondingly.
_Check_return_ HRESULT CalendarView::OnPrimaryPanelDesiredSizeChanged(_In_ CalendarViewGeneratorHost* pHost)
{
    // monthpanel is the only primary panel
    ASSERT(pHost == m_tpMonthViewItemHost.Get());

    auto pMonthViewPanel = pHost->GetPanel();

    ASSERT(pMonthViewPanel);

    wf::Size desiredViewportSize = {};
    IFC_RETURN(pMonthViewPanel->GetDesiredViewportSize(&desiredViewportSize));

    CalendarViewTemplateSettings* pTemplateSettingsConcrete = m_tpTemplateSettings.Cast<CalendarViewTemplateSettings>();
    IFC_RETURN(pTemplateSettingsConcrete->put_MinViewWidth(desiredViewportSize.Width));

    return S_OK;
}

IFACEMETHODIMP CalendarView::MeasureOverride(
    _In_ wf::Size availableSize,
    _Out_ wf::Size* pDesired)
{
    HRESULT hr = S_OK;

    if (m_calendarChanged)
    {
        m_calendarChanged = false;
        IFC(CreateCalendarAndMonthYearFormatter());
        IFC(FormatWeekDayNames());
        IFC(UpdateFlowDirectionForView());
    }

    if (m_dateSourceChanged)
    {
        // m_minDate or m_maxDate changed, we need to refresh all dates
        // so we should disconnect itemhosts and update the itemhosts, then register them again.
        m_dateSourceChanged = false;
        IFC(DisconnectItemHosts());
        IFC(RefreshItemHosts());
        IFC(InitializeIndexCorrectionTableIfNeeded());  // for some timezones, we need to figure out where are the gaps (missing days)
        IFC(RegisterItemHosts());
        IFC(UpdateWeekDayNames());
    }

    IFC(CalendarViewGenerated::MeasureOverride(availableSize, pDesired));

Cleanup:
    return hr;
}

IFACEMETHODIMP CalendarView::ArrangeOverride(
    _In_ wf::Size finalSize,
    _Out_ wf::Size* returnValue)
{
    IFC_RETURN(CalendarViewGenerated::ArrangeOverride(finalSize, returnValue));

    if (m_tpViewsGrid)
    {
        // When switching views, the up-scaled view needs to be clipped by the original size.
        double viewsHeight = 0.;
        double viewsWidth = 0.;
        CalendarViewTemplateSettings* pTemplateSettingsConcrete = m_tpTemplateSettings.Cast<CalendarViewTemplateSettings>();

        IFC_RETURN(m_tpViewsGrid.Cast<Grid>()->get_ActualHeight(&viewsHeight));
        IFC_RETURN(m_tpViewsGrid.Cast<Grid>()->get_ActualWidth(&viewsWidth));

        wf::Rect clipRect = { 0., 0., static_cast<float>(viewsWidth), static_cast<float>(viewsHeight) };

        IFC_RETURN(pTemplateSettingsConcrete->put_ClipRect(clipRect));

        // ScaleTransform.CenterX and CenterY
        IFC_RETURN(pTemplateSettingsConcrete->put_CenterX(viewsWidth / 2));
        IFC_RETURN(pTemplateSettingsConcrete->put_CenterY(viewsHeight / 2));
    }

    if (m_isSetDisplayDateRequested)
    {
        // m_lastDisplayedDate is already coerced and adjusted, time to process this request and clear the flag.
        m_isSetDisplayDateRequested = false;
        IFC_RETURN(SetDisplayDateInternal(m_lastDisplayedDate));
    }

    return S_OK;
}

// create a list of languages to construct Globalization::Calendar and Globalization::DateTimeFormatter.
// here we prepend CalendarView.Language to ApplicationLanguage.Languages as the new list.
_Check_return_ HRESULT CalendarView::CreateCalendarLanguages()
{
    wrl_wrappers::HString strLanguage;
    ctl::ComPtr<wfc::IIterable<HSTRING>> spCalendarLanguages;

    IFC_RETURN(get_Language(strLanguage.GetAddressOf()));
    IFC_RETURN(CreateCalendarLanguagesStatic(std::move(strLanguage), &spCalendarLanguages));
    SetPtrValue(m_tpCalendarLanguages, spCalendarLanguages.Get());

    return S_OK;
}

// helper method to prepend a string into a collection of string.
/*static */_Check_return_ HRESULT CalendarView::CreateCalendarLanguagesStatic(
    _In_ wrl_wrappers::HString&& language,
    _Outptr_ wfc::IIterable<HSTRING>** ppLanguages)
{
    ctl::ComPtr<wg::IApplicationLanguagesStatics> spApplicationLanguagesStatics;
    ctl::ComPtr<wfc::IVectorView<HSTRING>> spApplicationLanguages;
    ctl::ComPtr<InternalStringCollection> spCalendarLanguages;
    unsigned size = 0;

    IFC_RETURN(ctl::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Windows_Globalization_ApplicationLanguages).Get(),
        &spApplicationLanguagesStatics));

    IFC_RETURN(spApplicationLanguagesStatics->get_Languages(&spApplicationLanguages));

    IFC_RETURN(ctl::make(&spCalendarLanguages));
    spCalendarLanguages->emplace_back(std::move(language));

    IFC_RETURN(spApplicationLanguages->get_Size(&size));

    for (unsigned i = 0; i < size; ++i)
    {
        wrl_wrappers::HString strApplicationLanguage;
        IFC_RETURN(spApplicationLanguages->GetAt(i, strApplicationLanguage.GetAddressOf()));
        spCalendarLanguages->emplace_back(std::move(strApplicationLanguage));
    }

    IFC_RETURN(spCalendarLanguages.MoveTo(ppLanguages));

    return S_OK;
}

_Check_return_ HRESULT CalendarView::CreateCalendarAndMonthYearFormatter()
{
    ctl::ComPtr<wg::ICalendarFactory> spCalendarFactory;
    ctl::ComPtr<wg::ICalendar> spCalendar;
    wrl_wrappers::HStringReference strClock(L"24HourClock");    // it doesn't matter if it is 24 or 12 hour clock
    wrl_wrappers::HString strCalendarIdentifier;

    IFC_RETURN(get_CalendarIdentifier(strCalendarIdentifier.GetAddressOf()));

    //Create the calendar
    IFC_RETURN(ctl::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Windows_Globalization_Calendar).Get(),
        &spCalendarFactory));

    IFC_RETURN(spCalendarFactory->CreateCalendar(
        m_tpCalendarLanguages.Get(),
        strCalendarIdentifier.Get(),
        strClock.Get(),
        spCalendar.ReleaseAndGetAddressOf()));

    SetPtrValue(m_tpCalendar, spCalendar);

    // create a Calendar clone (have the same timezone, same calendarlanguages and identifier) for DateComparer and SelectedDates
    // changing the date on the Calendar in DateComparer will not affect the Calendar in CalendarView.
    IFC_RETURN(m_dateComparer->SetCalendarForComparison(spCalendar.Get()));
    IFC_RETURN(m_tpSelectedDates.Cast<TrackableDateCollection>()->SetCalendarForComparison(spCalendar.Get()));

    // in multiple era calendar, we'll have different (slower) logic to handle the decade scope.
    {
        int firstEra = 0;
        int lastEra = 0;
        IFC_RETURN(m_tpCalendar->get_FirstEra(&firstEra));
        IFC_RETURN(m_tpCalendar->get_LastEra(&lastEra));
        m_isMultipleEraCalendar = firstEra != lastEra;
    }

    IFC_RETURN(m_tpCalendar->SetToNow());
    IFC_RETURN(m_tpCalendar->GetDateTime(&m_today));

    // default displaydate is today
    if (m_lastDisplayedDate.UniversalTime == 0)
    {
        m_lastDisplayedDate = m_today;
    }

    IFC_RETURN(ForeachHost([](CalendarViewGeneratorHost* pHost)
    {
        auto pPanel = pHost->GetPanel();
        if (pPanel)
        {
            IFC_RETURN(pPanel->SetNeedsToDetermineBiggestItemSize());
        }
        pHost->ResetPossibleItemStrings();
        return S_OK;
    }));

    {
        ctl::ComPtr<wg::DateTimeFormatting::IDateTimeFormatter> spFormatter;

        // month year formatter needs to be updated when calendar is updated (languages or calendar identifier changed).
        IFC_RETURN(CreateDateTimeFormatter(wrl_wrappers::HStringReference(L"month year").Get(), &spFormatter));
        SetPtrValue(m_tpMonthYearFormatter, spFormatter);

        // year formatter also needs to be updated when the calendar is updated.
        IFC_RETURN(CreateDateTimeFormatter(wrl_wrappers::HStringReference(L"year").Get(), &spFormatter));
        SetPtrValue(m_tpYearFormatter, spFormatter);
    }

    return S_OK;
}

_Check_return_ HRESULT CalendarView::DisconnectItemHosts()
{

    if (m_itemHostsConnected)
    {
        m_itemHostsConnected = false;

        IFC_RETURN(ForeachHost([](CalendarViewGeneratorHost* pHost)
        {
            auto pPanel = pHost->GetPanel();
            if (pPanel)
            {
                IFC_RETURN(pPanel->DisconnectItemsHost());
            }
            return S_OK;
        }));
    }

    return S_OK;
}

_Check_return_ HRESULT CalendarView::RegisterItemHosts()
{
    ASSERT(!m_itemHostsConnected);

    m_itemHostsConnected = true;

    IFC_RETURN(ForeachHost([](CalendarViewGeneratorHost* pHost)
    {
        auto pPanel = pHost->GetPanel();
        if (pPanel)
        {
            IFC_RETURN(pPanel->RegisterItemsHost(pHost));
        }
        return S_OK;
    }));

    return S_OK;
}

_Check_return_ HRESULT CalendarView::RefreshItemHosts()
{
    wf::DateTime minDate;
    wf::DateTime maxDate;

    IFC_RETURN(get_MinDate(&minDate));
    IFC_RETURN(get_MaxDate(&maxDate));

    // making sure our MinDate and MaxDate are supported by the current Calendar
    {
        wf::DateTime tempDate;

        IFC_RETURN(m_tpCalendar->SetToMin());
        IFC_RETURN(m_tpCalendar->GetDateTime(&tempDate));

        m_minDate.UniversalTime = MAX(minDate.UniversalTime, tempDate.UniversalTime);

        IFC_RETURN(m_tpCalendar->SetToMax());
        IFC_RETURN(m_tpCalendar->GetDateTime(&tempDate));

        m_maxDate.UniversalTime = MIN(maxDate.UniversalTime, tempDate.UniversalTime);
    }

    if (m_dateComparer->LessThan(m_maxDate, m_minDate))
    {
        IFC_RETURN(ErrorHelper::OriginateErrorUsingResourceID(E_FAIL, ERROR_CALENDAR_INVALID_MIN_MAX_DATE));
    }

    CoerceDate(m_lastDisplayedDate);

    IFC_RETURN(m_tpCalendar->SetDateTime(m_minDate));
    IFC_RETURN(m_tpCalendar->get_DayOfWeek(&m_weekDayOfMinDate));

    IFC_RETURN(ForeachHost([](CalendarViewGeneratorHost* pHost)
    {
        pHost->ResetScope();    // reset the scope data to force update the scope and header text.
        IFC_RETURN(pHost->ComputeSize());
        return S_OK;
    }));

    return S_OK;
}

_Check_return_ HRESULT CalendarView::AttachVisibleIndicesUpdatedEvents()
{
    return ForeachHost([](CalendarViewGeneratorHost* pHost)
    {
        return pHost->AttachVisibleIndicesUpdatedEvent();
    });
}

_Check_return_ HRESULT CalendarView::DetachVisibleIndicesUpdatedEvents()
{
    return ForeachHost([](CalendarViewGeneratorHost* pHost)
    {
        return pHost->DetachVisibleIndicesUpdatedEvent();
    });
}

// attach FocusEngaged event for all three hosts
_Check_return_ HRESULT CalendarView::AttachScrollViewerFocusEngagedEvents()
{
    return ForeachHost([](CalendarViewGeneratorHost* pHost)
    {
        return pHost->AttachScrollViewerFocusEngagedEvent();
    });
}

// detach FocusEngaged event for all three hosts
_Check_return_ HRESULT CalendarView::DetachScrollViewerFocusEngagedEvents()
{
    return ForeachHost([](CalendarViewGeneratorHost* pHost)
    {
        return pHost->DetachScrollViewerFocusEngagedEvent();
    });
}

_Check_return_ HRESULT CalendarView::AttachButtonClickedEvents()
{
    HRESULT hr = S_OK;

    if (m_tpHeaderButton)
    {
        IFC(m_epHeaderButtonClickHandler.AttachEventHandler(m_tpHeaderButton.Cast<Button>(),
            [this](IInspectable* pSender, IRoutedEventArgs* pArgs)
        {
            return OnHeaderButtonClicked();
        }));
    }

    if (m_tpPreviousButton)
    {
        IFC(m_epPreviousButtonClickHandler.AttachEventHandler(m_tpPreviousButton.Cast<Button>(),
            [this](IInspectable* pSender, IRoutedEventArgs* pArgs)
        {
            return OnNavigationButtonClicked(false /*forward*/);
        }));
    }

    if (m_tpNextButton)
    {
        IFC(m_epNextButtonClickHandler.AttachEventHandler(m_tpNextButton.Cast<Button>(),
            [this](IInspectable* pSender, IRoutedEventArgs* pArgs)
        {
            return OnNavigationButtonClicked(true /*forward*/);
        }));
    }

Cleanup:
    return hr;
}

_Check_return_ HRESULT CalendarView::DetachButtonClickedEvents()
{
    HRESULT hr = S_OK;

    IFC(DetachHandler(m_epHeaderButtonClickHandler, m_tpHeaderButton));
    IFC(DetachHandler(m_epPreviousButtonClickHandler, m_tpPreviousButton));
    IFC(DetachHandler(m_epNextButtonClickHandler, m_tpNextButton));

Cleanup:
    return hr;
}

_Check_return_ HRESULT CalendarView::AttachScrollViewerKeyDownEvents()
{
    //Engagement now prevents events from bubbling from an engaged element. Before we relied on the bubbling behavior to
    //receive the KeyDown events from the ScrollViewer in the CalendarView. Now instead we have to handle the ScrollViewer's
    //On Key Down. To prevent handling the same OnKeyDown twice we only call into OnKeyDown if the ScrollViewer is engaged,
    //if it isn't it will bubble up the event.
    if (m_tpMonthViewScrollViewer)
    {
        IFC_RETURN(m_epMonthViewScrollViewerKeyDownEventHandler.AttachEventHandler(m_tpMonthViewScrollViewer.AsOrNull<IUIElement>().Get(),
            [this](IInspectable* pSender, xaml::Input::IKeyRoutedEventArgs* pArgs)
        {
            BOOLEAN isEngaged = FALSE;
            m_tpMonthViewScrollViewer.Cast<ScrollViewer>()->get_IsFocusEngaged(&isEngaged);
            if (isEngaged)
            {
                return OnKeyDown(pArgs);
            }

            return S_OK;
        }));
    }

    if (m_tpYearViewScrollViewer)
    {
        IFC_RETURN(m_epYearViewScrollViewerKeyDownEventHandler.AttachEventHandler(m_tpYearViewScrollViewer.AsOrNull<IUIElement>().Get(),
            [this](IInspectable* pSender, xaml::Input::IKeyRoutedEventArgs* pArgs)
        {
            BOOLEAN isEngaged = FALSE;
            m_tpYearViewScrollViewer.Cast<ScrollViewer>()->get_IsFocusEngaged(&isEngaged);
            if (isEngaged)
            {
                return OnKeyDown(pArgs);
            }

            return S_OK;
        }));
    }

    if (m_tpDecadeViewScrollViewer)
    {
        IFC_RETURN(m_epDecadeViewScrollViewerKeyDownEventHandler.AttachEventHandler(m_tpDecadeViewScrollViewer.AsOrNull<IUIElement>().Get(),
            [this](IInspectable* pSender, xaml::Input::IKeyRoutedEventArgs* pArgs)
        {
            BOOLEAN isEngaged = FALSE;
            m_tpDecadeViewScrollViewer.Cast<ScrollViewer>()->get_IsFocusEngaged(&isEngaged);
            if (isEngaged)
            {
                return OnKeyDown(pArgs);
            }

            return S_OK;
        }));
    }

    return S_OK;
}

_Check_return_ HRESULT CalendarView::DetachScrollViewerKeyDownEvents()
{
    IFC_RETURN(DetachHandler(m_epMonthViewScrollViewerKeyDownEventHandler, m_tpMonthViewScrollViewer));
    IFC_RETURN(DetachHandler(m_epYearViewScrollViewerKeyDownEventHandler, m_tpYearViewScrollViewer));
    IFC_RETURN(DetachHandler(m_epDecadeViewScrollViewerKeyDownEventHandler, m_tpDecadeViewScrollViewer));

    return S_OK;
}

_Check_return_ HRESULT CalendarView::UpdateHeaderText(_In_ bool withAnimation)
{
    ctl::ComPtr<CalendarViewGeneratorHost> spHost;

    IFC_RETURN(GetActiveGeneratorHost(&spHost));

    CalendarViewTemplateSettings* pTemplateSettingsConcrete = m_tpTemplateSettings.Cast<CalendarViewTemplateSettings>();
    IFC_RETURN(pTemplateSettingsConcrete->put_HeaderText(spHost->GetHeaderTextOfCurrentScope()->Get()));

    if (withAnimation)
    {
        BOOLEAN bIgnored = FALSE;
        // play animation on the HeaderText after view mode changed.
        IFC_RETURN(GoToState(true, L"ViewChanged", &bIgnored));
        IFC_RETURN(GoToState(true, L"ViewChanging", &bIgnored));
    }

    // If UpdateText is because navigation button is clicked, make narrator to say the header.
    if (m_isNavigationButtonClicked)
    {
        m_isNavigationButtonClicked = false;
        IFC_RETURN(RaiseAutomationNotificationAfterNavigationButtonClicked());

    }

    return S_OK;
}

// disable the button if we don't have more content
_Check_return_ HRESULT CalendarView::UpdateNavigationButtonStates()
{
    ctl::ComPtr<CalendarViewGeneratorHost> spHost;

    IFC_RETURN(GetActiveGeneratorHost(&spHost));

    auto pCalendarPanel = spHost->GetPanel();

    if (pCalendarPanel)
    {
        int firstVisibleIndex = 0;
        int lastVisibleIndex = 0;
        unsigned size = 0;
        CalendarViewTemplateSettings* pTemplateSettingsConcrete = m_tpTemplateSettings.Cast<CalendarViewTemplateSettings>();

        IFC_RETURN(pCalendarPanel->get_FirstVisibleIndexBase(&firstVisibleIndex));
        IFC_RETURN(pCalendarPanel->get_LastVisibleIndexBase(&lastVisibleIndex));

        IFC_RETURN(spHost->get_Size(&size));

        IFC_RETURN(pTemplateSettingsConcrete->put_HasMoreContentBefore(firstVisibleIndex > 0));
        IFC_RETURN(pTemplateSettingsConcrete->put_HasMoreContentAfter(lastVisibleIndex + 1 < static_cast<int>(size)));
    }

    return S_OK;
}

_Check_return_ HRESULT CalendarView::OnHeaderButtonClicked()
{
    HRESULT hr = S_OK;
    xaml_controls::CalendarViewDisplayMode mode = xaml_controls::CalendarViewDisplayMode_Month;

    IFC(get_DisplayMode(&mode));

    if (mode != xaml_controls::CalendarViewDisplayMode_Decade)
    {
        if (mode == xaml_controls::CalendarViewDisplayMode_Month)
        {
            mode = xaml_controls::CalendarViewDisplayMode_Year;
        }
        else // if (mode == xaml_controls::CalendarViewDisplayMode_Year)
        {
            mode = xaml_controls::CalendarViewDisplayMode_Decade;
        }

        IFC(put_DisplayMode(mode));
    }
    else
    {
        ASSERT(FALSE, L"header button should be disabled already in decade view mode.");
    }

Cleanup:
    return hr;
}

_Check_return_ HRESULT CalendarView::RaiseAutomationNotificationAfterNavigationButtonClicked()
{
    if (m_tpHeaderButton)
    {
        wrl_wrappers::HString automationName;
        IFC_RETURN(DirectUI::AutomationProperties::GetNameStatic(m_tpHeaderButton.Cast<Button>(), automationName.ReleaseAndGetAddressOf()))
        if (!automationName.Get())
        {
            IFC_RETURN(FrameworkElement::GetStringFromObject(m_tpHeaderButton.Get(), automationName.ReleaseAndGetAddressOf()));
        }

        if (automationName.Get())
        {
            ctl::ComPtr<xaml_automation_peers::IAutomationPeer> calendarViewAutomationPeer;

            IFC_RETURN(GetOrCreateAutomationPeer(&calendarViewAutomationPeer));
            if (calendarViewAutomationPeer)
            {
                // Two possible solution: RaisePropertyChangedEvent or RaiseNotificationEvent. If Raise PropertyChangedEvent each time when head is changing,
                // it would be overkilling since header is already included in other automation event. More information about RaiseNotificationEvent, please
                // refer to https://docs.microsoft.com/en-us/uwp/api/windows.ui.xaml.automation.peers.automationnotificationkind
                IFC_RETURN(calendarViewAutomationPeer->RaiseNotificationEvent(
                    xaml_automation_peers::AutomationNotificationKind::AutomationNotificationKind_ActionCompleted,
                    xaml_automation_peers::AutomationNotificationProcessing::AutomationNotificationProcessing_MostRecent,
                    automationName.Get(),
                    wrl_wrappers::HStringReference(L"CalenderViewNavigationButtonCompleted").Get()));
            }
        }
    }
    return S_OK;
}

_Check_return_ HRESULT CalendarView::OnNavigationButtonClicked(_In_ bool forward)
{
    ctl::ComPtr<CalendarViewGeneratorHost> spHost;

    IFC_RETURN(GetActiveGeneratorHost(&spHost));

    auto pCalendarPanel = spHost->GetPanel();

    if (pCalendarPanel)
    {
        bool canPanelShowFullScope = false;

        int firstVisibleIndex = 0;
        ctl::ComPtr<IDependencyObject> spChildAsIDO;
        ctl::ComPtr<ICalendarViewBaseItem> spChildAsI;
        ctl::ComPtr<CalendarViewBaseItem> spChild;
        wf::DateTime dateOfFirstVisibleItem = {};
        wf::DateTime targetDate = {};

        IFC_RETURN(CanPanelShowFullScope(spHost.Get(), &canPanelShowFullScope));

        IFC_RETURN(pCalendarPanel->get_FirstVisibleIndexBase(&firstVisibleIndex));

        IFC_RETURN(pCalendarPanel->ContainerFromIndex(firstVisibleIndex, &spChildAsIDO));

        spChildAsI = spChildAsIDO.AsOrNull<ICalendarViewBaseItem>();
        if (spChildAsI)
        {
            spChild = spChildAsI.Cast<CalendarViewBaseItem>();
            IFC_RETURN(spChild->GetDate(&dateOfFirstVisibleItem));

            HRESULT hr = S_OK;

            if (canPanelShowFullScope)
            {
                // if Panel can show a full scope, we navigate by a scope.
                hr = spHost->GetFirstDateOfNextScope(dateOfFirstVisibleItem, forward, &targetDate);
            }
            else
            {
                // if Panel can't show a full scope, we navigate by a page, so we don't skip items.
                int cols = 0;
                int rows = 0;

                IFC_RETURN(pCalendarPanel->get_Rows(&rows));
                IFC_RETURN(pCalendarPanel->get_Cols(&cols));
                int numberOfItemsPerPage = cols * rows;
                int distance = forward ? numberOfItemsPerPage : -numberOfItemsPerPage;
                targetDate = dateOfFirstVisibleItem;
                hr = spHost->AddUnits(targetDate, distance);
#ifdef DBG
                if (SUCCEEDED(hr))
                {
                    // targetDate should be still in valid range.
                    auto temp = targetDate;
                    CoerceDate(temp);
                    ASSERT(temp.UniversalTime == targetDate.UniversalTime);
                }
#endif
            }

            if (FAILED(hr))
            {
                // if we crossed the boundaries when we compute the target date, we use the hard limit.
                targetDate = forward ? m_maxDate : m_minDate;
            }

            IFC_RETURN(ScrollToDateWithAnimation(spHost.Get(), targetDate));

            // After navigation button is clicked, header text is not updated immediately. ScrollToDateWithAnimation is the first step,
            // OnVisibleIndicesUpdated and UpdateHeaderText would be in another UI message processing loop.
            // This flag is to identify that the HeaderText update is from navigation button.
            m_isNavigationButtonClicked = true;
        }
    }

    return S_OK;
}


// change the dimensions of YearView and DecadeView.
// API name to be reviewed.
_Check_return_ HRESULT CalendarView::SetYearDecadeDisplayDimensionsImpl(
    _In_ INT columns, _In_ INT rows)
{
    IFCEXPECT_RETURN(columns > 0 && rows > 0);

    // note once this is set, developer can't unset it
    m_areYearDecadeViewDimensionsSet = true;

    m_colsInYearDecadeView = columns;
    m_rowsInYearDecadeView = rows;

    auto pYearPanel = m_tpYearViewItemHost->GetPanel();
    if (pYearPanel)
    {
        // Panel type is no longer Secondary_SelfAdaptive
        IFC_RETURN(pYearPanel->SetPanelType(CalendarPanel::CalendarPanelType::CalendarPanelType_Secondary));
        IFC_RETURN(pYearPanel->SetSuggestedDimension(columns, rows));
    }

    auto pDecadePanel = m_tpDecadeViewItemHost->GetPanel();
    if (pDecadePanel)
    {
        // Panel type is no longer Secondary_SelfAdaptive
        IFC_RETURN(pDecadePanel->SetPanelType(CalendarPanel::CalendarPanelType::CalendarPanelType_Secondary));
        IFC_RETURN(pDecadePanel->SetSuggestedDimension(columns, rows));
    }

    return S_OK;
}

// When we call SetDisplayDate, we'll check if the current view is big enough to hold a whole scope.
// If yes then we'll bring the first date in this scope into view,
// otherwise bring the display date into view then the display date will be on first visible line.
//
// note: when panel can't show a fullscope, we might be still able to show the first day and the requested date
// in the viewport (e.g. NumberOfWeeks is 4, we request to display 1/9/2000, in this case 1/1/2000 and 1/9/2000 can
// be visible at the same time). To consider this case we need more logic, we can fix later when needed.
_Check_return_ HRESULT CalendarView::BringDisplayDateIntoView(
    _In_ CalendarViewGeneratorHost* pHost)
{
    bool canPanelShowFullScope = false;
    wf::DateTime dateToBringIntoView;

    IFC_RETURN(CanPanelShowFullScope(pHost, &canPanelShowFullScope));

    if (canPanelShowFullScope)
    {
        IFC_RETURN(m_tpCalendar->SetDateTime(m_lastDisplayedDate));
        IFC_RETURN(pHost->AdjustToFirstUnitInThisScope(&dateToBringIntoView));
        CoerceDate(dateToBringIntoView);
    }
    else
    {
        dateToBringIntoView.UniversalTime = m_lastDisplayedDate.UniversalTime;
    }

    IFC_RETURN(ScrollToDate(pHost, dateToBringIntoView));

    return S_OK;
}

// bring a item into view
// This function will scroll to the target item immediately,
// when target is far away from realized window, we'll not see unrealized area.
_Check_return_ HRESULT CalendarView::ScrollToDate(
    _In_ CalendarViewGeneratorHost* pHost,
    _In_ wf::DateTime date)
{
    HRESULT hr = S_OK;
    int index = 0;

    IFC(pHost->CalculateOffsetFromMinDate(date, &index));
    ASSERT(index >= 0);
    ASSERT(pHost->GetPanel());
    IFC(pHost->GetPanel()->ScrollItemIntoView(
        index,
        xaml_controls::ScrollIntoViewAlignment_Leading,
        0.0 /* offset */,
        TRUE /* forceSynchronous */));

Cleanup:
    return hr;
}

// Bring a item into view with animation.
// This function will scroll to the target item with DManip animation so
// if target is not realized yet, we might see unrealized area.
// This only gets called in NavigationButton clicked event where
// the target should be less than one page away from visible window.
_Check_return_ HRESULT CalendarView::ScrollToDateWithAnimation(
    _In_ CalendarViewGeneratorHost* pHost,
    _In_ wf::DateTime date)
{
    auto pScrollViewer = pHost->GetScrollViewer();
    if (pScrollViewer)
    {
        int index = 0;
        int firstVisibleIndex = 0;
        int cols = 0;
        ctl::ComPtr<IDependencyObject> spFirstVisibleItemAsI;
        ctl::ComPtr<CalendarViewBaseItem> spFirstVisibleItem;
        ctl::ComPtr<IInspectable> spVerticalOffset;
        ctl::ComPtr<wf::IReference<DOUBLE>> spVerticalOffsetReference;
        boolean handled = false;

        auto pCalendarPanel = pHost->GetPanel();

        // The target item may be not realized yet so we can't get
        // the offset from virtualization information.
        // However all items have the same size so we could deduce the target's
        // exact offset from the current realized item, e.g. the firstVisibleItem

        // 1. compute the target index.
        IFC_RETURN(pHost->CalculateOffsetFromMinDate(date, &index));
        ASSERT(index >= 0);

        IFC_RETURN(pCalendarPanel->get_Cols(&cols));
        ASSERT(cols > 0);

        // cols should not be 0 at this point. If it is, perhaps
        // the calendar view has not been fully brought up yet.
        // If cols is 0, we do not want to bring the process down though.
        // Doing a no-op for the scroll to date in this case.
        if(cols > 0)
        {
            // 2. find the first visible index.
            IFC_RETURN(pCalendarPanel->get_FirstVisibleIndex(&firstVisibleIndex));
            IFC_RETURN(pCalendarPanel->ContainerFromIndex(firstVisibleIndex, &spFirstVisibleItemAsI));
            IFC_RETURN(spFirstVisibleItemAsI.As(&spFirstVisibleItem));

            ASSERT(spFirstVisibleItem->GetVirtualizationInformation());

            // 3. based on the first visible item's bounds, compute the target item's offset
            auto bounds = spFirstVisibleItem->GetVirtualizationInformation()->GetBounds();
            auto verticalDistance = (index - firstVisibleIndex) / cols;

            // if target item is before the first visible index and is not the first in that row, we should substract 1 from the distance
            // because -6 / 7 = 0 (we expect -1).
            if ((index-firstVisibleIndex)%cols!=0 && index<=firstVisibleIndex)
            {
                --verticalDistance;
            }
            // there are some corner cases in Japanese calendar where the target date and current date are in the same row.
            // e.g. Showa 64 only has 1 month, in year view, January Show64 and January Heisei are in the same row.
            // When we try to scroll down from showa64 to Heisei1 in year view, verticalDistance would be 0 since those 2 years are in the same row.
            // We do ++verticalDistance here to point to March of Heise 1 in the next row, otherwise we'll get stuck in the first row and navigate down button would stop working.
            else if (verticalDistance == 0 && index > firstVisibleIndex)
            {
                ++verticalDistance;
            }
            auto offset = bounds.Y + verticalDistance * bounds.Height;

            // 4. scroll to target item's offset (with animation)
            IFC_RETURN(PropertyValue::CreateFromDouble(offset, &spVerticalOffset));
            IFC_RETURN(spVerticalOffset.As(&spVerticalOffsetReference));

            IFC_RETURN(pScrollViewer->ChangeViewWithOptionalAnimation(
                nullptr /*horizontalOffset*/,
                spVerticalOffsetReference.Get(),
                nullptr /*zoomFactor*/,
                false /*disableAnimation*/,
                &handled));
            ASSERT(handled);
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CalendarView::SetDisplayDateImpl(_In_ wf::DateTime date)
{
    // if m_dateSourceChanged is true, this means we might changed m_minDate or m_maxDate
    // so we should not call CoerceDate until next measure pass, by then the m_minDate and
    // m_maxDate are updated.
    if (!m_dateSourceChanged)
    {
        CoerceDate(date);

        IFC_RETURN(SetDisplayDateInternal(date));
    }
    else
    {
        // given that m_dateSourceChanged is true, we'll have a new layout pass soon.
        // we're going to honer the display date request in that layout pass.
        // note: there is an issue to call ScrollItemIntoView in MCBP's measure pass
        // the workaround is call it in Arrange pass or later. here we'll call it
        // in the arrange pass.
        m_isSetDisplayDateRequested = true;
        m_lastDisplayedDate = date;
    }

    return S_OK;
}

_Check_return_ HRESULT CalendarView::SetDisplayDateInternal(_In_ wf::DateTime date)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<CalendarViewGeneratorHost> spHost;

    IFC(GetActiveGeneratorHost(&spHost));

    m_lastDisplayedDate = date;

    if (spHost->GetPanel())
    {
        // note if panel is not loaded yet (i.e. we call SetDisplayDate before Panel is loaded,
        // below call will fail silently. This is not a problem because
        // we'll call this again in panel loaded event.

        IFC(BringDisplayDateIntoView(spHost.Get()));
    }

Cleanup:
    return hr;
}

void CalendarView::CoerceDate(_Inout_ wf::DateTime& date)
{
    // we should not call CoerceDate when m_dateSourceChanged is true, because
    // m_dateSourceChanged being true means the current m_minDate or m_maxDate is
    // out of dated.

    ASSERT(!m_dateSourceChanged);
    if (m_dateComparer->LessThan(date, m_minDate))
    {
        date = m_minDate;
    }

    if (m_dateComparer->LessThan(m_maxDate, date))
    {
        date = m_maxDate;
    }
}

_Check_return_ HRESULT CalendarView::OnVisibleIndicesUpdated(
    _In_ CalendarViewGeneratorHost* pHost)
{
    HRESULT hr = S_OK;
    int firstVisibleIndex = 0;
    int lastVisibleIndex = 0;
    ctl::ComPtr<IDependencyObject> spTempChildAsIDO;
    ctl::ComPtr<ICalendarViewBaseItem> spTempChildAsI;
    wf::DateTime firstDate = {};
    wf::DateTime lastDate = {};
    bool isScopeChanged = false;
    int startIndex = 0;
    int numberOfItemsInCol;

    auto pCalendarPanel = pHost->GetPanel();

    ASSERT(pCalendarPanel);

    // We explicitly call UpdateLayout in OnDisplayModeChanged, this will ocassionaly make CalendarPanelType invalid,
    // which causes CalendarPanel to skip the row&col calculations.
    // If CalendarPanelType is invalid, just skip the current update
    // since this method will be called again in later layout passes.
    if (pCalendarPanel->GetPanelType() != CalendarPanel::CalendarPanelType::CalendarPanelType_Invalid)
    {
        IFC(pCalendarPanel->get_StartIndex(&startIndex));
        IFC(pCalendarPanel->get_Cols(&numberOfItemsInCol));

        ASSERT(startIndex < numberOfItemsInCol);

        IFC(pCalendarPanel->get_FirstVisibleIndexBase(&firstVisibleIndex));
        IFC(pCalendarPanel->get_LastVisibleIndexBase(&lastVisibleIndex));

        IFC(pCalendarPanel->ContainerFromIndex(firstVisibleIndex, &spTempChildAsIDO));

        IFC(spTempChildAsIDO.As(&spTempChildAsI));

        IFC(spTempChildAsI.Cast<CalendarViewBaseItem>()->GetDate(&firstDate));

        IFC(pCalendarPanel->ContainerFromIndex(lastVisibleIndex, spTempChildAsIDO.ReleaseAndGetAddressOf()));

        IFC(spTempChildAsIDO.As(&spTempChildAsI));

        IFC(spTempChildAsI.Cast<CalendarViewBaseItem>()->GetDate(&lastDate));

        //now determine the current scope based on this date.
        IFC(pHost->UpdateScope(firstDate, lastDate, &isScopeChanged));

        if (isScopeChanged)
        {
            IFC(UpdateHeaderText(false /*withAnimation*/));
        }

        // everytime visible indices changed, we need to update
        // navigationButtons' states.
        IFC(UpdateNavigationButtonStates());

        IFC(UpdateItemsScopeState(
            pHost,
            true, /*ignoreWhenIsOutOfScopeDisabled*/
            true /*ignoreInDirectManipulation*/));
    }

Cleanup:
    return hr;
}

// to achieve best visual effect we define that items are in OutOfScope state only when:
// 1. IsOutOfScopeEnabled is true, and
// 2. item is in Visible window and it is not in current scope.
// 3. Not in manipulation.
// for all other cases, item is in InScope state.
//
// this function updates the ScopeState for
// 1. all visible items, and
// 2. the items that are not visible but was marked as OutOfScope (because viewport changed)
//
// so we'll call this function when
// 1. IsOutOfScopeEnabled property changed, or
// 2. Visible Indices changed
// 3. Manipulation state changed.
_Check_return_ HRESULT CalendarView::UpdateItemsScopeState(
    _In_ CalendarViewGeneratorHost* pHost,
    _In_ bool ignoreWhenIsOutOfScopeDisabled,
    _In_ bool ignoreInDirectManipulation)
{
    auto pCalendarPanel = pHost->GetPanel();
    if (!pCalendarPanel)
    {
        // it is possible that we change IsOutOfScopeEnabled property before CalendarView enters visual tree.
        return S_OK;
    }

    BOOLEAN isOutOfScopeEnabled = FALSE;
    IFC_RETURN(get_IsOutOfScopeEnabled(&isOutOfScopeEnabled));

    if (ignoreWhenIsOutOfScopeDisabled && !isOutOfScopeEnabled)
    {
        return S_OK;
    }

    bool isInDirectManipulation = pHost->GetScrollViewer() && pHost->GetScrollViewer()->IsInDirectManipulation();
    if (ignoreInDirectManipulation && isInDirectManipulation)
    {
        return S_OK;
    }

    bool canHaveOutOfScopeState = isOutOfScopeEnabled && !isInDirectManipulation;
    int firstIndex = -1;
    int lastIndex = -1;
    ctl::ComPtr<IDependencyObject> spChildAsIDO;
    ctl::ComPtr<ICalendarViewBaseItem> spChildAsI;
    ctl::ComPtr<CalendarViewBaseItem> spChild;
    wf::DateTime date;

    IFC_RETURN(pCalendarPanel->get_FirstVisibleIndex(&firstIndex));
    IFC_RETURN(pCalendarPanel->get_LastVisibleIndex(&lastIndex));

    // given that all items not in visible window have InScope state, so we only want
    // to check the visible window, plus the items in last visible window. this way
    // we don't need to check against virtualization window.
    auto& lastVisibleIndicesPair = pHost->GetLastVisibleIndicesPairRef();

    if (firstIndex != -1 && lastIndex != -1)
    {
        for (int index = firstIndex; index <= lastIndex; ++index)
        {
            IFC_RETURN(pCalendarPanel->ContainerFromIndex(index, &spChildAsIDO));
            IFC_RETURN(spChildAsIDO.As(&spChildAsI));

            spChild = spChildAsI.Cast<CalendarViewBaseItem>();
            IFC_RETURN(spChild->GetDate(&date));

            bool isOutOfScope = m_dateComparer->LessThan(date, pHost->GetMinDateOfCurrentScope()) || m_dateComparer->LessThan(pHost->GetMaxDateOfCurrentScope(), date);


            IFC_RETURN(spChild->SetIsOutOfScope(canHaveOutOfScopeState && isOutOfScope));
        }
    }

    // now let's check the items were marked as OutOfScope but now not in Visible window (so they should be marked as InScope)


    if (lastVisibleIndicesPair[0] != -1 && lastVisibleIndicesPair[1] != -1)
    {
        if (lastVisibleIndicesPair[0] < firstIndex)
        {
            for (int index = lastVisibleIndicesPair[0]; index <= std::min(lastVisibleIndicesPair[1], firstIndex - 1); ++index)
            {
                IFC_RETURN(pCalendarPanel->ContainerFromIndex(index, &spChildAsIDO));
                spChildAsI = spChildAsIDO.AsOrNull<ICalendarViewBaseItem>();

                if (spChildAsI)
                {
                    // this item is not in visible window but was marked as OutOfScope before, set it to "InScope" now.
                    IFC_RETURN(spChildAsI.Cast<CalendarViewBaseItem>()->SetIsOutOfScope(false));
                }
            }
        }

        if (lastVisibleIndicesPair[1] > lastIndex)
        {
            for (int index = lastVisibleIndicesPair[1]; index >= std::max(lastVisibleIndicesPair[0], lastIndex + 1); --index)
            {
                IFC_RETURN(pCalendarPanel->ContainerFromIndex(index, &spChildAsIDO));
                spChildAsI = spChildAsIDO.AsOrNull<ICalendarViewBaseItem>();

                if (spChildAsI)
                {
                    // this item is not in visible window but was marked as OutOfScope before, set it to "InScope" now.
                    IFC_RETURN(spChildAsI.Cast<CalendarViewBaseItem>()->SetIsOutOfScope(false));
                }
            }
        }
    }

    // store the visible indices pair
    lastVisibleIndicesPair[0] = firstIndex;
    lastVisibleIndicesPair[1] = lastIndex;
    return S_OK;
}

// this property affects Today in MonthView, ThisMonth in YearView and ThisYear in DecadeView.
_Check_return_ HRESULT CalendarView::OnIsTodayHighlightedPropertyChanged()
{
    IFC_RETURN(ForeachHost([this](CalendarViewGeneratorHost* pHost)
    {
        auto pPanel = pHost->GetPanel();
        if (pPanel)
        {
            int indexOfToday = -1;

            IFC_RETURN(pHost->CalculateOffsetFromMinDate(m_today, &indexOfToday));

            if (indexOfToday != -1)
            {
                ctl::ComPtr<IDependencyObject> spChildAsIDO;
                ctl::ComPtr<ICalendarViewBaseItem> spChildAsI;

                IFC_RETURN(pPanel->ContainerFromIndex(indexOfToday, &spChildAsIDO));
                spChildAsI = spChildAsIDO.AsOrNull<ICalendarViewBaseItem>();
                // today item is realized already, we need to update the state here.
                // if today item is not realized yet, we'll update the state when today item is being prepared.
                if (spChildAsI)
                {
                    BOOLEAN isTodayHighlighted = FALSE;

                    IFC_RETURN(get_IsTodayHighlighted(&isTodayHighlighted));
                    IFC_RETURN(spChildAsI.Cast<CalendarViewBaseItem>()->SetIsToday(!!isTodayHighlighted));
                }
            }
        }
        return S_OK;
    }));

    return S_OK;
}

_Check_return_ HRESULT CalendarView::OnIsOutOfScopePropertyChanged()
{
    ctl::ComPtr<CalendarViewGeneratorHost> spHost;
    BOOLEAN isOutOfScopeEnabled = FALSE;

    IFC_RETURN(get_IsOutOfScopeEnabled(&isOutOfScopeEnabled));

    // when IsOutOfScopeEnabled property is false, we don't care about scope state (all are inScope),
    // so we don't need to hook to ScrollViewer's state change handler.
    // when IsOutOfScopeEnabled property is true, we need to do so.
    if (m_areDirectManipulationStateChangeHandlersHooked != !!isOutOfScopeEnabled)
    {
        m_areDirectManipulationStateChangeHandlersHooked = !m_areDirectManipulationStateChangeHandlersHooked;

        IFC_RETURN(ForeachHost([isOutOfScopeEnabled](CalendarViewGeneratorHost* pHost)
        {
            auto pScrollViewer = pHost->GetScrollViewer();
            if (pScrollViewer)
            {
                IFC_RETURN(pScrollViewer->SetDirectManipulationStateChangeHandler(
                    isOutOfScopeEnabled ? pHost : nullptr
                    ));
            }
            return S_OK;
        }));
    }

    IFC_RETURN(GetActiveGeneratorHost(&spHost));
    IFC_RETURN(UpdateItemsScopeState(
        spHost.Get(),
        false, /*ignoreWhenIsOutOfScopeDisabled*/
        true /*ignoreInDirectManipulation*/));

    return S_OK;
}

_Check_return_ HRESULT CalendarView::OnScrollViewerFocusEngaged(
    _In_ xaml_controls::IFocusEngagedEventArgs* pArgs)
{
    ctl::ComPtr<CalendarViewGeneratorHost> spHost;

    IFC_RETURN(GetActiveGeneratorHost(&spHost));

    if (spHost)
    {
        bool focused = false;
        m_focusItemAfterDisplayModeChanged = false;
        ctl::ComPtr<xaml_controls::IFocusEngagedEventArgs> spArgs(pArgs);

        IFC_RETURN(FocusItemByDate(spHost.Get(), m_lastDisplayedDate, m_focusStateAfterDisplayModeChanged, &focused));

        IFC_RETURN(spArgs->put_Handled(focused));
    }

    return S_OK;
}

_Check_return_ HRESULT CalendarView::OnDisplayModeChanged(
    _In_ xaml_controls::CalendarViewDisplayMode oldDisplayMode,
    _In_ xaml_controls::CalendarViewDisplayMode newDisplayMode)
{
    ctl::ComPtr<CalendarViewGeneratorHost> spCurrentHost;
    ctl::ComPtr<CalendarViewGeneratorHost> spOldHost;
    BOOLEAN isEngaged = FALSE;

    IFC_RETURN(GetGeneratorHost(oldDisplayMode, &spOldHost));
    if (spOldHost)
    {
        auto pScrollViewer = spOldHost->GetScrollViewer();

        if (pScrollViewer)
        {
            // if old host is engaged, disengage
            pScrollViewer->get_IsFocusEngaged(&isEngaged);
            if (isEngaged)
            {
                pScrollViewer->RemoveFocusEngagement();
            }
        }
    }

    IFC_RETURN(UpdateLastDisplayedDate(oldDisplayMode));

    IFC_RETURN(UpdateVisualState());

    IFC_RETURN(GetGeneratorHost(newDisplayMode, &spCurrentHost));
    auto pCurrentPanel = spCurrentHost->GetPanel();
    if (pCurrentPanel)
    {
        // if panel is not loaded yet (e.g. the first time we switch to the YearView or DecadeView),
        // ScrollItemIntoView (called by FocusItemByDate) will not work because ScrollViewer is not
        // hooked up yet. Give the panel an extra layout pass to hook up the ScrollViewer.
        if (newDisplayMode != xaml_controls::CalendarViewDisplayMode_Month)
        {
            IFC_RETURN(pCurrentPanel->UpdateLayout());
        }

        // If Engaged, make sure that the new scroll viewer is engaged. Note that
        // you want to Engage before focusing ItemByDate to land on the correct item.
        if (isEngaged)
        {
            auto spScrollViewer = spCurrentHost->GetScrollViewer();
            if (spScrollViewer)
            {
                // The old ScrollViewer was engaged, engage the new ScrollViewer
                ctl::ComPtr<xaml_input::IFocusManagerStaticsPrivate> spFocusManager;

                IFC_RETURN(ctl::GetActivationFactory(
                    wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Input_FocusManager).Get(),
                    &spFocusManager));

                //A control must be focused before we can set Engagement on it, attempt to set focus first
                BOOLEAN focused = FALSE;
                IFC_RETURN(DependencyObject::SetFocusedElement(spScrollViewer, xaml::FocusState_Keyboard, FALSE /*animateIfBringIntoView*/, &focused));
                if (focused)
                {
                    IFC_RETURN(spFocusManager->SetEngagedControl(ctl::as_iinspectable(spScrollViewer)));
                }
            }
        }

        // If we requested to move focus to item, let's do it.
        if (m_focusItemAfterDisplayModeChanged)
        {
            bool focused = false;
            m_focusItemAfterDisplayModeChanged = false;

            IFC_RETURN(FocusItemByDate(spCurrentHost.Get(), m_lastDisplayedDate, m_focusStateAfterDisplayModeChanged, &focused));
        }
        else // we only scroll to the focusedDate without moving focus to it
        {
            IFC_RETURN(BringDisplayDateIntoView(spCurrentHost.Get()));
        }
    }

    CalendarViewTemplateSettings* pTemplateSettingsConcrete = m_tpTemplateSettings.Cast<CalendarViewTemplateSettings>();

    IFC_RETURN(pTemplateSettingsConcrete->put_HasMoreViews(newDisplayMode != xaml_controls::CalendarViewDisplayMode_Decade));
    IFC_RETURN(UpdateHeaderText(true /*withAnimation*/));

    IFC_RETURN(UpdateNavigationButtonStates());

    return S_OK;
}


_Check_return_ HRESULT CalendarView::UpdateLastDisplayedDate(_In_ xaml_controls::CalendarViewDisplayMode lastDisplayMode)
{
    ctl::ComPtr<CalendarViewGeneratorHost> spPreviousHost;
    IFC_RETURN(GetGeneratorHost(lastDisplayMode, &spPreviousHost));

    auto pPreviousPanel = spPreviousHost->GetPanel();
    if (pPreviousPanel)
    {
        int firstVisibleIndex = 0;
        int lastVisibleIndex = 0;
        wf::DateTime firstVisibleDate = {};
        wf::DateTime lastVisibleDate = {};
        ctl::ComPtr<IDependencyObject> spChildAsIDO;
        ctl::ComPtr<ICalendarViewBaseItem> spChildAsI;

        IFC_RETURN(pPreviousPanel->get_FirstVisibleIndexBase(&firstVisibleIndex));
        IFC_RETURN(pPreviousPanel->get_LastVisibleIndexBase(&lastVisibleIndex));

        ASSERT(firstVisibleIndex != -1 && lastVisibleIndex != -1);

        IFC_RETURN(pPreviousPanel->ContainerFromIndex(firstVisibleIndex, &spChildAsIDO));
        IFC_RETURN(spChildAsIDO.As(&spChildAsI));
        IFC_RETURN(spChildAsI.Cast<CalendarViewBaseItem>()->GetDate(&firstVisibleDate));

        IFC_RETURN(pPreviousPanel->ContainerFromIndex(lastVisibleIndex, &spChildAsIDO));
        IFC_RETURN(spChildAsIDO.As(&spChildAsI));
        IFC_RETURN(spChildAsI.Cast<CalendarViewBaseItem>()->GetDate(&lastVisibleDate));

        // check if last displayed Date is visible or not
        bool isLastDisplayedDateVisible = false;
        int result = 0;
        IFC_RETURN(spPreviousHost->CompareDate(m_lastDisplayedDate, firstVisibleDate, &result));
        if (result >= 0)
        {
            IFC_RETURN(spPreviousHost->CompareDate(m_lastDisplayedDate, lastVisibleDate, &result));
            if (result <= 0)
            {
                isLastDisplayedDateVisible = true;
            }
        }
        if (!isLastDisplayedDateVisible)
        {
            // if last displayed date is not visible, we use the first_visible_inscope_date as the last displayed date

            // first try to use the first_inscope_date
            wf::DateTime firstVisibleInscopeDate = spPreviousHost->GetMinDateOfCurrentScope();
            // check if first_inscope_date is visible or not
            IFC_RETURN(spPreviousHost->CompareDate(firstVisibleInscopeDate, firstVisibleDate, &result));
            if (result < 0)
            {
                // the firstInscopeDate is not visible, then we use the firstVisibleDate.
#ifdef DBG
                {
                    // in this case firstVisibleDate must be in scope (i.e. it must be less than or equals to the maxDateOfCurrentScope).
                    int temp = 0;
                    IFC_RETURN(spPreviousHost->CompareDate(firstVisibleDate, spPreviousHost->GetMaxDateOfCurrentScope(), &temp));
                    ASSERT(temp <= 0);
                }
#endif
                firstVisibleInscopeDate = firstVisibleDate;
            }

            // based on the display mode, partially copy the firstVisibleInscopeDate to m_lastDisplayedDate.
            IFC_RETURN(CopyDate(
                lastDisplayMode,
                firstVisibleInscopeDate,
                m_lastDisplayedDate));
        }
    }

    return S_OK;
}


_Check_return_ HRESULT CalendarView::OnIsLabelVisibleChanged()
{
    HRESULT hr = S_OK;

    // we don't have label text in decade view.
    std::array<CalendarViewGeneratorHost*, 2> hosts{ { m_tpMonthViewItemHost.Get(), m_tpYearViewItemHost.Get() } };

    BOOLEAN isLabelVisible = FALSE;

    IFC(get_IsGroupLabelVisible(&isLabelVisible));

    for (unsigned i = 0; i < hosts.size(); ++i)
    {
        auto pHost = hosts[i];
        auto pPanel = pHost->GetPanel();

        if (pPanel)
        {
            IFC(ForeachChildInPanel(pPanel,
                [pHost, isLabelVisible](_In_ CalendarViewBaseItem* pItem)
            {
                return pHost->UpdateLabel(pItem, !!isLabelVisible);
            }));
        }
    }

Cleanup:
    return hr;
}

_Check_return_ HRESULT CalendarView::CreateDateTimeFormatter(
    _In_ HSTRING format,
    _Outptr_ wg::DateTimeFormatting::IDateTimeFormatter** ppDateTimeFormatter)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wg::DateTimeFormatting::IDateTimeFormatterFactory> spFormatterFactory;
    ctl::ComPtr<wg::DateTimeFormatting::IDateTimeFormatter> spFormatter;
    wrl_wrappers::HStringReference strClock(L"24HourClock");    // it doesn't matter if it is 24 or 12 hour clock
    wrl_wrappers::HStringReference strGeographicRegion(L"ZZ");    // geographicRegion doesn't really matter as we have no decimal separator or grouping
    wrl_wrappers::HString strCalendarIdentifier;

    IFC(get_CalendarIdentifier(strCalendarIdentifier.GetAddressOf()));

    IFC(ctl::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Windows_Globalization_DateTimeFormatting_DateTimeFormatter).Get(),
        &spFormatterFactory));

    IFCPTR(spFormatterFactory);

    IFC(spFormatterFactory->CreateDateTimeFormatterContext(
        format,
        m_tpCalendarLanguages.Get(),
        strGeographicRegion.Get(),
        strCalendarIdentifier.Get(),
        strClock.Get(),
        spFormatter.ReleaseAndGetAddressOf()));

    IFC(spFormatter.MoveTo(ppDateTimeFormatter));

Cleanup:
    return hr;
}

_Check_return_ HRESULT CalendarView::FormatWeekDayNames()
{
    HRESULT hr = S_OK;

    if (m_tpMonthViewItemHost->GetPanel())
    {
        ctl::ComPtr<IInspectable> spDayOfWeekFormat;
        BOOLEAN isUnsetValue = FALSE;
        wg::DayOfWeek dayOfWeek = wg::DayOfWeek_Sunday;
        ctl::ComPtr<wg::DateTimeFormatting::IDateTimeFormatter> spFormatter;

        IFC(ReadLocalValue(
            MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::CalendarView_DayOfWeekFormat),
            &spDayOfWeekFormat));
        IFC(DependencyPropertyFactory::IsUnsetValue(spDayOfWeekFormat.Get(), isUnsetValue));

        IFC(m_tpCalendar->SetToNow());
        IFC(m_tpCalendar->get_DayOfWeek(&dayOfWeek));

        // adjust to next sunday.
        IFC(m_tpCalendar->AddDays((s_numberOfDaysInWeek - dayOfWeek) % s_numberOfDaysInWeek));
        m_dayOfWeekNames.clear();
        m_dayOfWeekNames.reserve(s_numberOfDaysInWeek);

        // Fill m_dayOfWeekNamesFull. This will always be the full name of the day regardless of abbreviation used for m_dayOfWeekNames.
        m_dayOfWeekNamesFull.clear();
        m_dayOfWeekNamesFull.reserve(s_numberOfDaysInWeek);

        if (!isUnsetValue)   // format is set, use this format.
        {
            wrl_wrappers::HString dayOfWeekFormat;

            IFC(get_DayOfWeekFormat(dayOfWeekFormat.GetAddressOf()));

            // Workaround: we can't bind an unset value to a property.
            // Here we'll check if the format is empty or not - because in CalendarDatePicker this property
            // is bound to CalendarDatePicker.DayOfWeekFormat which will cause this value is always set.
            if (!dayOfWeekFormat.IsEmpty())
            {
                IFC(CreateDateTimeFormatter(dayOfWeekFormat.Get(), &spFormatter));

            }
        }

        for (int i = 0; i < s_numberOfDaysInWeek; ++i)
        {
            wrl_wrappers::HString string;

            if (spFormatter)    // there is a valid datetimeformatter specified by user, use it
            {
                wf::DateTime date;
                IFC(m_tpCalendar->GetDateTime(&date));
                IFC(spFormatter->Format(date, string.GetAddressOf()));
            }
            else    // otherwise use the shortest string formatted by calendar.
            {
                IFC(m_tpCalendar->DayOfWeekAsString(
                    1, /*shortest length*/
                    string.GetAddressOf()));
            }
            m_dayOfWeekNames.emplace_back(std::move(string));

            // for automation peer name, we always use the full string.
            IFC(m_tpCalendar->DayOfWeekAsFullString(string.GetAddressOf()));
            m_dayOfWeekNamesFull.emplace_back(std::move(string));

            IFC(m_tpCalendar->AddDays(1));
        }
    }

Cleanup:
    return hr;
}

_Check_return_ HRESULT CalendarView::UpdateWeekDayNameAPName(_In_reads_(count) wchar_t* str, _In_ size_t count, _In_ const wrl_wrappers::HString& name)
{
    ctl::ComPtr<xaml_controls::ITextBlock> spWeekDay;
    IFC_RETURN(GetTemplatePart<xaml_controls::ITextBlock>(str, count, spWeekDay.GetAddressOf()));
    IFC_RETURN(AutomationProperties::SetNameStatic(spWeekDay.Cast<TextBlock>(), name.Get()));
    return S_OK;
}

_Check_return_ HRESULT CalendarView::UpdateWeekDayNames()
{
    HRESULT hr = S_OK;

    auto pMonthPanel = m_tpMonthViewItemHost->GetPanel();
    if (pMonthPanel)
    {
        wg::DayOfWeek firstDayOfWeek = wg::DayOfWeek_Sunday;
        int index = 0;
        CalendarViewTemplateSettings* pTemplateSettingsConcrete = m_tpTemplateSettings.Cast<CalendarViewTemplateSettings>();

        IFC(get_FirstDayOfWeek(&firstDayOfWeek));

        if (m_dayOfWeekNames.empty())
        {
            IFC(FormatWeekDayNames());
        }

        index = static_cast<int>(firstDayOfWeek - wg::DayOfWeek_Sunday);

        IFC(pTemplateSettingsConcrete->put_WeekDay1(m_dayOfWeekNames[index].Get()));
        IFC(UpdateWeekDayNameAPName(STR_LEN_PAIR(L"WeekDay1"), m_dayOfWeekNamesFull[index]));
        index = (index + 1) % s_numberOfDaysInWeek;

        IFC(pTemplateSettingsConcrete->put_WeekDay2(m_dayOfWeekNames[index].Get()));
        IFC(UpdateWeekDayNameAPName(STR_LEN_PAIR(L"WeekDay2"), m_dayOfWeekNamesFull[index]));
        index = (index + 1) % s_numberOfDaysInWeek;

        IFC(pTemplateSettingsConcrete->put_WeekDay3(m_dayOfWeekNames[index].Get()));
        IFC(UpdateWeekDayNameAPName(STR_LEN_PAIR(L"WeekDay3"), m_dayOfWeekNamesFull[index]));
        index = (index + 1) % s_numberOfDaysInWeek;

        IFC(pTemplateSettingsConcrete->put_WeekDay4(m_dayOfWeekNames[index].Get()));
        IFC(UpdateWeekDayNameAPName(STR_LEN_PAIR(L"WeekDay4"), m_dayOfWeekNamesFull[index]));
        index = (index + 1) % s_numberOfDaysInWeek;

        IFC(pTemplateSettingsConcrete->put_WeekDay5(m_dayOfWeekNames[index].Get()));
        IFC(UpdateWeekDayNameAPName(STR_LEN_PAIR(L"WeekDay5"), m_dayOfWeekNamesFull[index]));
        index = (index + 1) % s_numberOfDaysInWeek;

        IFC(pTemplateSettingsConcrete->put_WeekDay6(m_dayOfWeekNames[index].Get()));
        IFC(UpdateWeekDayNameAPName(STR_LEN_PAIR(L"WeekDay6"), m_dayOfWeekNamesFull[index]));
        index = (index + 1) % s_numberOfDaysInWeek;

        IFC(pTemplateSettingsConcrete->put_WeekDay7(m_dayOfWeekNames[index].Get()));
        IFC(UpdateWeekDayNameAPName(STR_LEN_PAIR(L"WeekDay7"), m_dayOfWeekNamesFull[index]));

        m_monthViewStartIndex = (m_weekDayOfMinDate - firstDayOfWeek + s_numberOfDaysInWeek) % s_numberOfDaysInWeek;

        ASSERT(m_monthViewStartIndex >= 0 && m_monthViewStartIndex < s_numberOfDaysInWeek);

        IFC(pMonthPanel->put_StartIndex(m_monthViewStartIndex));
    }

Cleanup:
    return hr;
}

_Check_return_ HRESULT CalendarView::GetActiveGeneratorHost(_Outptr_ CalendarViewGeneratorHost** ppHost)
{
    xaml_controls::CalendarViewDisplayMode mode = xaml_controls::CalendarViewDisplayMode_Month;
    *ppHost = nullptr;

    IFC_RETURN(get_DisplayMode(&mode));

    return GetGeneratorHost(mode, ppHost);
}

_Check_return_ HRESULT CalendarView::GetGeneratorHost(
    _In_ xaml_controls::CalendarViewDisplayMode mode,
    _Outptr_ CalendarViewGeneratorHost** ppHost)
{
    if (mode == xaml_controls::CalendarViewDisplayMode_Month)
    {
        IFC_RETURN(m_tpMonthViewItemHost.CopyTo(ppHost));
    }
    else if (mode == xaml_controls::CalendarViewDisplayMode_Year)
    {
        IFC_RETURN(m_tpYearViewItemHost.CopyTo(ppHost));
    }
    else if (mode == xaml_controls::CalendarViewDisplayMode_Decade)
    {
        IFC_RETURN(m_tpDecadeViewItemHost.CopyTo(ppHost));
    }
    else
    {
        ASSERT(false);
    }

    return S_OK;
}

_Check_return_ HRESULT CalendarView::FormatYearName(_In_ wf::DateTime date, _Out_ HSTRING* pName)
{
    return m_tpYearFormatter->Format(date, pName);
}

_Check_return_ HRESULT CalendarView::FormatMonthYearName(_In_ wf::DateTime date, _Out_ HSTRING* pName)
{
    return m_tpMonthYearFormatter->Format(date, pName);
}

// Partially copy date from source to target.
// Only copy the parts we want and keep the remaining part.
// Once the remaining part becomes invalid with the new copied parts,
// we need to adjust the remaining part to the most reasonable value.
// e.g. target: 3/31/2014, source 2/1/2013 and we want to copy month part,
// the target will become 2/31/2013 and we'll adjust the day to 2/28/2013.

_Check_return_ HRESULT CalendarView::CopyDate(
    _In_ xaml_controls::CalendarViewDisplayMode displayMode,
    _In_ wf::DateTime source,
    _Inout_ wf::DateTime& target)
{
    HRESULT hr = S_OK;

    bool copyEra = true;
    bool copyYear = true;
    bool copyMonth = displayMode == xaml_controls::CalendarViewDisplayMode_Month ||
        displayMode == xaml_controls::CalendarViewDisplayMode_Year;
    bool copyDay = displayMode == xaml_controls::CalendarViewDisplayMode_Month;

    if (copyEra && copyYear && copyMonth && copyDay)
    {
        // copy everything.
        target = source;
    }
    else
    {
        int era = 0;
        int year = 0;
        int month = 0;
        int day = 0;

        IFC(m_tpCalendar->SetDateTime(source));
        if (copyEra)
        {
            IFC(m_tpCalendar->get_Era(&era));
        }

        if (copyYear)
        {
            IFC(m_tpCalendar->get_Year(&year));
        }

        if (copyMonth)
        {
            IFC(m_tpCalendar->get_Month(&month));
        }

        if (copyDay)
        {
            IFC(m_tpCalendar->get_Day(&day));
        }

        IFC(m_tpCalendar->SetDateTime(target));

        if (copyEra)
        {
            // era is always valid.
            IFC(m_tpCalendar->put_Era(era));
        }

        if (copyYear)
        {
            // year might not be valid.
            int first = 0;
            int last = 0;
            IFC(m_tpCalendar->get_FirstYearInThisEra(&first));
            IFC(m_tpCalendar->get_LastYearInThisEra(&last));
            year = std::min(last, std::max(first, year));
            IFC(m_tpCalendar->put_Year(year));
        }

        if (copyMonth)
        {
            // month might not be valid.
            int first = 0;
            int last = 0;
            IFC(m_tpCalendar->get_FirstMonthInThisYear(&first));
            IFC(m_tpCalendar->get_LastMonthInThisYear(&last));
            month = std::min(last, std::max(first, month));
            IFC(m_tpCalendar->put_Month(month));
        }

        if (copyDay)
        {
            // day might not be valid.
            int first = 0;
            int last = 0;
            IFC(m_tpCalendar->get_FirstDayInThisMonth(&first));
            IFC(m_tpCalendar->get_LastDayInThisMonth(&last));
            day = std::min(last, std::max(first, day));
            IFC(m_tpCalendar->put_Day(day));
        }

        IFC(m_tpCalendar->GetDateTime(&target));
        // make sure the target is still in range.
        CoerceDate(target);
    }

Cleanup:
    return hr;
}

/*static*/ _Check_return_ HRESULT CalendarView::CanPanelShowFullScope(
    _In_ CalendarViewGeneratorHost* pHost,
    _Out_ bool* pCanPanelShowFullScope)
{
    auto pCalendarPanel = pHost->GetPanel();
    INT row = 0;
    INT col = 0;
    *pCanPanelShowFullScope = false;

    ASSERT(pCalendarPanel);

    IFC_RETURN(pCalendarPanel->get_Rows(&row));
    IFC_RETURN(pCalendarPanel->get_Cols(&col));

    // Consider about the corner case: the first item in this scope
    // is laid on the last col in first row, so according dimension
    // row x col, we could arrange up to (row - 1) x col + 1 items

    *pCanPanelShowFullScope = (row - 1) * col + 1 >= pHost->GetMaximumScopeSize();

    return S_OK;
}

_Check_return_ HRESULT CalendarView::ForeachChildInPanel(
    _In_opt_ CalendarPanel* pCalendarPanel,
    _In_ std::function<HRESULT(_In_ CalendarViewBaseItem*)> func)
{
    if (pCalendarPanel)
    {
        if (pCalendarPanel->IsInLiveTree())
        {
            int firstCacheIndex = 0;
            int lastCacheIndex = 0;

            IFC_RETURN(pCalendarPanel->get_FirstCacheIndex(&firstCacheIndex));
            IFC_RETURN(pCalendarPanel->get_LastCacheIndex(&lastCacheIndex));

            if (firstCacheIndex >= 0 && lastCacheIndex >= 0)
            {
                for (int i = firstCacheIndex; i <= lastCacheIndex; ++i)
                {
                    ctl::ComPtr<IDependencyObject> spChildAsIDO;

                    IFC_RETURN(pCalendarPanel->ContainerFromIndex(i, &spChildAsIDO));

                    if (spChildAsIDO)
                    {
                        ctl::ComPtr<ICalendarViewBaseItem> spChildAsI;

                        IFC_RETURN(spChildAsIDO.As(&spChildAsI));

                        IFC_RETURN(func(spChildAsI.Cast<CalendarViewBaseItem>()));
                    }
                }
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CalendarView::ForeachHost(_In_ std::function<HRESULT(_In_ CalendarViewGeneratorHost* pHost)> func)
{
    IFC_RETURN(func(m_tpMonthViewItemHost.Get()));
    IFC_RETURN(func(m_tpYearViewItemHost.Get()));
    IFC_RETURN(func(m_tpDecadeViewItemHost.Get()));
    return S_OK;
}

/*static*/ _Check_return_ HRESULT CalendarView::SetDayItemStyle(
    _In_ CalendarViewBaseItem* pItem,
    _In_opt_ xaml::IStyle* pStyle)
{
    ASSERT(ctl::is<ICalendarViewDayItem>(pItem));
    if (pStyle)
    {
        IFC_RETURN(pItem->put_Style(pStyle));
    }
    else
    {
        IFC_RETURN(pItem->ClearValue(
            MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::FrameworkElement_Style)));
    }

    return S_OK;
}

IFACEMETHODIMP CalendarView::OnCreateAutomationPeer(_Outptr_result_maybenull_ xaml_automation_peers::IAutomationPeer** ppAutomationPeer)
{
    IFCPTR_RETURN(ppAutomationPeer);
    *ppAutomationPeer = nullptr;

    ctl::ComPtr<CalendarViewAutomationPeer> spAutomationPeer;
    IFC_RETURN(ActivationAPI::ActivateAutomationInstance(KnownTypeIndex::CalendarViewAutomationPeer, GetHandle(), spAutomationPeer.GetAddressOf()));
    IFC_RETURN(spAutomationPeer->put_Owner(this));
    *ppAutomationPeer = spAutomationPeer.Detach();
    return S_OK;
}

// Called when the IsEnabled property changes.
_Check_return_ HRESULT CalendarView::OnIsEnabledChanged(_In_ IsEnabledChangedEventArgs* pArgs)
{
    return UpdateVisualState();
}

_Check_return_ HRESULT CalendarView::GetRowHeaderForItemAutomationPeer(
    _In_ wf::DateTime itemDate,
    _In_ xaml_controls::CalendarViewDisplayMode displayMode,
    _Out_ UINT* pReturnValueCount,
    _Out_writes_to_ptr_(*pReturnValueCount) xaml_automation::Provider::IIRawElementProviderSimple*** ppReturnValue)
{
    *pReturnValueCount = 0;
    *ppReturnValue = nullptr;

    xaml_controls::CalendarViewDisplayMode mode = xaml_controls::CalendarViewDisplayMode_Month;
    IFC_RETURN(get_DisplayMode(&mode));

    //Ensure we only read out this header when in Month mode or Year mode. Decade mode reading the header isn't helpful.
    if (displayMode == mode)
    {
        int month, year;
        IFC_RETURN(m_tpCalendar->SetDateTime(itemDate));
        IFC_RETURN(m_tpCalendar->get_Month(&month));
        IFC_RETURN(m_tpCalendar->get_Year(&year));

        const bool useCurrentHeaderPeer =
            m_currentHeaderPeer &&
            (m_currentHeaderPeer->GetMonth() == month || mode == xaml_controls::CalendarViewDisplayMode_Year) &&
            m_currentHeaderPeer->GetYear() == year &&
            m_currentHeaderPeer->GetMode() == mode;

        const bool usePreviousHeaderPeer =
            m_previousHeaderPeer &&
            (m_previousHeaderPeer->GetMonth() == month || mode == xaml_controls::CalendarViewDisplayMode_Year) &&
            m_previousHeaderPeer->GetYear() == year &&
            m_previousHeaderPeer->GetMode() == mode;

        const bool createNewHeaderPeer = !useCurrentHeaderPeer && !usePreviousHeaderPeer;

        if (createNewHeaderPeer)
        {
            ctl::ComPtr<CalendarViewHeaderAutomationPeer> peer;
            IFC_RETURN(ActivationAPI::ActivateAutomationInstance(
                KnownTypeIndex::CalendarViewHeaderAutomationPeer,
                GetHandle(),
                peer.GetAddressOf()));

            wrl_wrappers::HString headerName;

            if (mode == xaml_controls::CalendarViewDisplayMode_Month)
            {
                IFC_RETURN(FormatMonthYearName(itemDate, headerName.GetAddressOf()));
            }
            else
            {
                ASSERT(mode == xaml_controls::CalendarViewDisplayMode_Year);
                IFC_RETURN(FormatYearName(itemDate, headerName.GetAddressOf()));
            }

            peer->Initialize(std::move(headerName), month, year, mode);

            SetPtrValue(m_previousHeaderPeer, m_currentHeaderPeer.Get());
            SetPtrValue(m_currentHeaderPeer, peer.Get());
        }

        ASSERT(m_currentHeaderPeer || m_previousHeaderPeer);

        const auto peerToUse =
            usePreviousHeaderPeer ?
            m_previousHeaderPeer.Get() :
            m_currentHeaderPeer.Get();

        ctl::ComPtr<xaml_automation::Provider::IIRawElementProviderSimple> provider;
        IFC_RETURN(peerToUse->ProviderFromPeer(peerToUse, &provider));

        unsigned allocSize = sizeof(IIRawElementProviderSimple*);
        *ppReturnValue = static_cast<IIRawElementProviderSimple**>(CoTaskMemAlloc(allocSize));
        IFCOOMFAILFAST(*ppReturnValue);
        ZeroMemory(*ppReturnValue, allocSize);

        (*ppReturnValue)[0] = provider.Detach();
        *pReturnValueCount = 1;
    }

    return S_OK;
}

_Check_return_ HRESULT CalendarView::UpdateFlowDirectionForView()
{
    if (m_tpViewsGrid && m_tpMonthYearFormatter)
    {
        bool isRTL = false;
        {
            ctl::ComPtr<__FIVectorView_1_HSTRING> spPatterns;
            IFC_RETURN(m_tpMonthYearFormatter->get_Patterns(&spPatterns));

            wrl_wrappers::HString strFormatPattern;
            IFC_RETURN(spPatterns->GetAt(0, strFormatPattern.GetAddressOf()));
            if (strFormatPattern.Get())
            {
                UINT32 length = 0;
                auto buffer = strFormatPattern.GetRawBuffer(&length);
                isRTL = buffer[0] == RTL_CHARACTER_CODE;
            }
        }

        auto flowDirection = isRTL ? xaml::FlowDirection_RightToLeft : xaml::FlowDirection_LeftToRight;
        IFC_RETURN(m_tpViewsGrid.Cast<Grid>()->put_FlowDirection(flowDirection));
    }
    return S_OK;
}

#ifdef DBG
void CalendarView::SetRoundedCalendarViewBaseItemChromeFallbackColor(KnownPropertyIndex propertyIndex, XUINT32 color)
{
    DXamlCore* core = DXamlCore::GetCurrent();
    CREATEPARAMETERS cp(core->GetHandle());
    xref_ptr<CSolidColorBrush> fallbackSolidColorBrush = nullptr;
    CValue valColor, valBrush;

    IGNOREHR(GetValueByKnownIndex(propertyIndex, valBrush));

    if (valBrush.IsNull())
    {
        IGNOREHR(CSolidColorBrush::Create(reinterpret_cast<CDependencyObject**>(fallbackSolidColorBrush.ReleaseAndGetAddressOf()), &cp));
        if (fallbackSolidColorBrush)
        {
            valColor.SetColor(color);
            IGNOREHR(fallbackSolidColorBrush->SetValueByKnownIndex(KnownPropertyIndex::SolidColorBrush_Color, valColor));
            valBrush.WrapObjectNoRef(fallbackSolidColorBrush.get());
            IGNOREHR(SetValueByKnownIndex(propertyIndex, valBrush));
        }
    }
}

void CalendarView::SetRoundedCalendarViewBaseItemChromeFallbackThickness(KnownPropertyIndex propertyIndex, XTHICKNESS thickness)
{
    DXamlCore* core = DXamlCore::GetCurrent();
    CREATEPARAMETERS cp(core->GetHandle());
    CValue valThickness;

    IGNOREHR(GetValueByKnownIndex(propertyIndex, valThickness));

    const XTHICKNESS currentThickness = *valThickness.AsThickness();

    if (currentThickness.left == 0.0f && currentThickness.right == 0.0f &&
        currentThickness.top == 0.0f && currentThickness.bottom == 0.0f)
    {
        valThickness.WrapThickness(const_cast<XTHICKNESS*>(&thickness));
        IGNOREHR(SetValueByKnownIndex(propertyIndex, valThickness));
    }
}

void CalendarView::SetRoundedCalendarViewBaseItemChromeFallbackCornerRadius(KnownPropertyIndex propertyIndex, XCORNERRADIUS cornerRadius)
{
    DXamlCore* core = DXamlCore::GetCurrent();
    CREATEPARAMETERS cp(core->GetHandle());
    CValue valCornerRadius;

    IGNOREHR(GetValueByKnownIndex(propertyIndex, valCornerRadius));

    const XCORNERRADIUS currentCornerRadius = *valCornerRadius.AsCornerRadius();

    if (currentCornerRadius.bottomLeft == 0.0f && currentCornerRadius.topLeft == 0.0f &&
        currentCornerRadius.bottomRight == 0.0f && currentCornerRadius.topRight == 0.0f)
    {
        valCornerRadius.WrapCornerRadius(const_cast<XCORNERRADIUS*>(&cornerRadius));
        IGNOREHR(SetValueByKnownIndex(propertyIndex, valCornerRadius));
    }
}

// Sets default fallback light theme colors for testing purposes, for cases where the properties are not set in markup and
// the rounded corner style is forced.
void CalendarView::SetRoundedCalendarViewBaseItemChromeFallbackProperties()
{
    if (CCalendarViewBaseItemChrome::IsRoundedCalendarViewBaseItemChromeForced())
    {
        XTHICKNESS defaultDayItemMargin = { 0, 6, 0, 0 };
        XTHICKNESS defaultMonthYearItemMargin = { 0, 2, 0, 0 };
        XTHICKNESS defaultFirstOfMonthLabelMargin = { 0, 1, 0, 0 };
        XTHICKNESS defaultFirstOfYearDecadeLabelMargin = { 0, 3, 0, 0 };
        //XCORNERRADIUS defaultCalendarItemCornerRadius = { 6, 6, 6, 6 };

        SetRoundedCalendarViewBaseItemChromeFallbackColor(KnownPropertyIndex::CalendarView_OutOfScopeHoverForeground, 0xE4000000);
        SetRoundedCalendarViewBaseItemChromeFallbackColor(KnownPropertyIndex::CalendarView_OutOfScopePressedForeground, 0x72000000);
        SetRoundedCalendarViewBaseItemChromeFallbackColor(KnownPropertyIndex::CalendarView_SelectedHoverForeground, 0xFF003E92);
        SetRoundedCalendarViewBaseItemChromeFallbackColor(KnownPropertyIndex::CalendarView_SelectedPressedForeground, 0xFF005FB7);
        SetRoundedCalendarViewBaseItemChromeFallbackColor(KnownPropertyIndex::CalendarView_SelectedDisabledForeground, 0x5C000000);
        SetRoundedCalendarViewBaseItemChromeFallbackColor(KnownPropertyIndex::CalendarView_TodayBlackoutForeground, 0xFFFFFFFF);
        SetRoundedCalendarViewBaseItemChromeFallbackColor(KnownPropertyIndex::CalendarView_TodaySelectedInnerBorderBrush, 0xFFFFFFFF);
        SetRoundedCalendarViewBaseItemChromeFallbackColor(KnownPropertyIndex::CalendarView_BlackoutStrikethroughBrush, 0x72000000);
        SetRoundedCalendarViewBaseItemChromeFallbackColor(KnownPropertyIndex::CalendarView_BlackoutBackground, 0x00FFFFFF);
        SetRoundedCalendarViewBaseItemChromeFallbackColor(KnownPropertyIndex::CalendarView_TodayBackground, 0xFF0067C0);
        SetRoundedCalendarViewBaseItemChromeFallbackColor(KnownPropertyIndex::CalendarView_TodayBlackoutBackground, 0xFF0078D4);
        SetRoundedCalendarViewBaseItemChromeFallbackColor(KnownPropertyIndex::CalendarView_TodayHoverBackground, 0xFF003E92);
        SetRoundedCalendarViewBaseItemChromeFallbackColor(KnownPropertyIndex::CalendarView_TodayPressedBackground, 0xFF0078D4);
        SetRoundedCalendarViewBaseItemChromeFallbackColor(KnownPropertyIndex::CalendarView_TodayDisabledBackground, 0x37000000);
        SetRoundedCalendarViewBaseItemChromeFallbackColor(KnownPropertyIndex::CalendarView_CalendarItemHoverBackground, 0x0A000000);
        SetRoundedCalendarViewBaseItemChromeFallbackColor(KnownPropertyIndex::CalendarView_CalendarItemPressedBackground, 0x06000000);
        SetRoundedCalendarViewBaseItemChromeFallbackColor(KnownPropertyIndex::CalendarView_CalendarItemDisabledBackground, 0x00FFFFFF);
        SetRoundedCalendarViewBaseItemChromeFallbackColor(KnownPropertyIndex::CalendarView_SelectedDisabledBorderBrush, 0x37000000);
        SetRoundedCalendarViewBaseItemChromeFallbackColor(KnownPropertyIndex::CalendarView_DisabledForeground, 0x5C000000);

        SetRoundedCalendarViewBaseItemChromeFallbackThickness(KnownPropertyIndex::CalendarView_DayItemMargin, defaultDayItemMargin);
        SetRoundedCalendarViewBaseItemChromeFallbackThickness(KnownPropertyIndex::CalendarView_MonthYearItemMargin, defaultMonthYearItemMargin);
        SetRoundedCalendarViewBaseItemChromeFallbackThickness(KnownPropertyIndex::CalendarView_FirstOfMonthLabelMargin, defaultFirstOfMonthLabelMargin);
        SetRoundedCalendarViewBaseItemChromeFallbackThickness(KnownPropertyIndex::CalendarView_FirstOfYearDecadeLabelMargin, defaultFirstOfYearDecadeLabelMargin);
        //SetRoundedCalendarViewBaseItemChromeFallbackCornerRadius(KnownPropertyIndex::CalendarView_CalendarItemCornerRadius, defaultCalendarItemCornerRadius);
    }
}
#endif // DBG
