// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

DECLARE_CONST_XSTRING_PTR_STORAGE(c_strDisabledForegroundStorage, L"SystemControlDisabledBaseMediumLowBrush");
DECLARE_CONST_XSTRING_PTR_STORAGE(c_strTodaySelectedInnerBorderBrushStorage, L"SystemControlHighlightAltAltHighBrush");
DECLARE_CONST_XSTRING_PTR_STORAGE(c_strTodayHoverBorderBrushStorage, L"SystemControlHighlightAltBaseMediumLowBrush");
DECLARE_CONST_XSTRING_PTR_STORAGE(c_strTodayPressedBorderBrushStorage, L"SystemControlHighlightAltBaseMediumBrush");
DECLARE_CONST_XSTRING_PTR_STORAGE(c_strTodayBackgroundStorage, L"SystemControlHighlightAccentBrush");
DECLARE_CONST_XSTRING_PTR_STORAGE(c_strTodayBlackoutBackgroundStorage, L"SystemControlHighlightListAccentLowBrush");

namespace {
    // below properties are not public, we can't set their values in the default style.
    // we have to manually set up the themeresource binding so they can get updated
    // when theme changes.

    // reminder: once we make them public in future, we should also update DirectUI::CalendarView::OnPropertyChanged2
    // like other public properties.
    const std::array<KnownPropertyIndex, 6> g_internalPropertyIndices { {
            KnownPropertyIndex::CalendarView_DisabledForeground,
            KnownPropertyIndex::CalendarView_TodaySelectedInnerBorderBrush,
            KnownPropertyIndex::CalendarView_TodayHoverBorderBrush,
            KnownPropertyIndex::CalendarView_TodayPressedBorderBrush,
            KnownPropertyIndex::CalendarView_TodayBackground, 
            KnownPropertyIndex::CalendarView_TodayBlackoutBackground
        } };

    const std::array<xstring_ptr_storage, 6> g_internalPropertyKeys { {
            c_strDisabledForegroundStorage,
            c_strTodaySelectedInnerBorderBrushStorage,
            c_strTodayHoverBorderBrushStorage,
            c_strTodayPressedBorderBrushStorage,
            c_strTodayBackgroundStorage,
            c_strTodayBlackoutBackgroundStorage
        } };


    const std::array<bool, 6> g_internalPropertyExistences{ {
            false,
            false,
            true,
            true,
            false,
            false
        } };
};

CCalendarView::CCalendarView(_In_ CCoreServices *pCore)
    : CControl(pCore)
    , m_pFocusBorderBrush(nullptr)
    , m_pSelectedHoverBorderBrush(nullptr)
    , m_pSelectedPressedBorderBrush(nullptr)
    , m_pSelectedDisabledBorderBrush(nullptr)
    , m_pSelectedBorderBrush(nullptr)
    , m_pHoverBorderBrush(nullptr)
    , m_pPressedBorderBrush(nullptr)
    , m_pCalendarItemBorderBrush(nullptr)
    , m_pBlackoutStrikethroughBrush(nullptr)
    , m_pBlackoutBackground(nullptr)
    , m_pOutOfScopeBackground(nullptr)
    , m_pCalendarItemBackground(nullptr)
    , m_pCalendarItemHoverBackground(nullptr)
    , m_pCalendarItemPressedBackground(nullptr)
    , m_pCalendarItemDisabledBackground(nullptr)
    , m_pPressedForeground(nullptr)
    , m_pTodayForeground(nullptr)
    , m_pBlackoutForeground(nullptr)
    , m_pTodayBlackoutForeground(nullptr)
    , m_pSelectedForeground(nullptr)
    , m_pSelectedHoverForeground(nullptr)
    , m_pSelectedPressedForeground(nullptr)
    , m_pSelectedDisabledForeground(nullptr)
    , m_pOutOfScopeForeground(nullptr)
    , m_pOutOfScopeHoverForeground(nullptr)
    , m_pOutOfScopePressedForeground(nullptr)
    , m_pCalendarItemForeground(nullptr)
    , m_pDayItemFontFamily(nullptr)
    , m_pDisabledForeground(nullptr)
    , m_pTodaySelectedInnerBorderBrush(nullptr)
    , m_pTodayHoverBorderBrush(nullptr)
    , m_pTodayPressedBorderBrush(nullptr)
    , m_pTodayBackground(nullptr)
    , m_pTodayHoverBackground(nullptr)
    , m_pTodayPressedBackground(nullptr)
    , m_pTodayDisabledBackground(nullptr)
    , m_pTodayBlackoutBackground(nullptr)
    , m_dayItemFontSize(20.0)
    , m_dayItemFontStyle(DirectUI::FontStyle::Normal)
    , m_dayItemFontWeight(DirectUI::CoreFontWeight::Normal)
    , m_todayFontWeight(DirectUI::CoreFontWeight::SemiBold)
    , m_pFirstOfMonthLabelFontFamily(nullptr)
    , m_firstOfMonthLabelFontSize(12.)
    , m_firstOfMonthLabelFontStyle(DirectUI::FontStyle::Normal)
    , m_firstOfMonthLabelFontWeight(DirectUI::CoreFontWeight::Normal)
    , m_pMonthYearItemFontFamily(nullptr)
    , m_monthYearItemFontSize(20.)
    , m_monthYearItemFontStyle(DirectUI::FontStyle::Normal)
    , m_monthYearItemFontWeight(DirectUI::CoreFontWeight::Normal)
    , m_pFirstOfYearDecadeLabelFontFamily(nullptr)
    , m_firstOfYearDecadeLabelFontSize(12.0)
    , m_firstOfYearDecadeLabelFontStyle(DirectUI::FontStyle::Normal)
    , m_firstOfYearDecadeLabelFontWeight(DirectUI::CoreFontWeight::Normal)
    , m_horizontalDayItemAlignment(DirectUI::HorizontalAlignment::Center)
    , m_verticalDayItemAlignment(DirectUI::VerticalAlignment::Center)
    , m_horizontalFirstOfMonthLabelAlignment(DirectUI::HorizontalAlignment::Center)
    , m_verticalFirstOfMonthLabelAlignment(DirectUI::VerticalAlignment::Top)
    , m_calendarItemBorderThickness()
    , m_dayItemMargin()
    , m_monthYearItemMargin()
    , m_firstOfMonthLabelMargin()
    , m_firstOfYearDecadeLabelMargin()
    , m_calendarItemCornerRadius()
{
    SetIsCustomType();
}
CCalendarView::~CCalendarView() noexcept
{
    ReleaseInterface(m_pFocusBorderBrush);
    ReleaseInterface(m_pSelectedHoverBorderBrush);
    ReleaseInterface(m_pSelectedPressedBorderBrush);
    ReleaseInterface(m_pSelectedDisabledBorderBrush);
    ReleaseInterface(m_pSelectedBorderBrush);
    ReleaseInterface(m_pHoverBorderBrush);
    ReleaseInterface(m_pPressedBorderBrush);
    ReleaseInterface(m_pCalendarItemBorderBrush);
    ReleaseInterface(m_pBlackoutStrikethroughBrush);
    ReleaseInterface(m_pBlackoutBackground);
    ReleaseInterface(m_pOutOfScopeBackground);
    ReleaseInterface(m_pCalendarItemBackground);
    ReleaseInterface(m_pCalendarItemHoverBackground);
    ReleaseInterface(m_pCalendarItemPressedBackground);
    ReleaseInterface(m_pCalendarItemDisabledBackground);
    ReleaseInterface(m_pPressedForeground);
    ReleaseInterface(m_pTodayForeground);
    ReleaseInterface(m_pBlackoutForeground);
    ReleaseInterface(m_pTodayBlackoutForeground);
    ReleaseInterface(m_pSelectedForeground);
    ReleaseInterface(m_pSelectedHoverForeground);
    ReleaseInterface(m_pSelectedPressedForeground);
    ReleaseInterface(m_pSelectedDisabledForeground);
    ReleaseInterface(m_pOutOfScopeForeground);
    ReleaseInterface(m_pOutOfScopeHoverForeground);
    ReleaseInterface(m_pOutOfScopePressedForeground);
    ReleaseInterface(m_pCalendarItemForeground);
    ReleaseInterface(m_pDayItemFontFamily);
    ReleaseInterface(m_pFirstOfMonthLabelFontFamily);
    ReleaseInterface(m_pMonthYearItemFontFamily);
    ReleaseInterface(m_pFirstOfYearDecadeLabelFontFamily);
    ReleaseInterface(m_pDisabledForeground);
    ReleaseInterface(m_pTodaySelectedInnerBorderBrush);
    ReleaseInterface(m_pTodayHoverBorderBrush);
    ReleaseInterface(m_pTodayPressedBorderBrush);
    ReleaseInterface(m_pTodayBackground);
    ReleaseInterface(m_pTodayHoverBackground);
    ReleaseInterface(m_pTodayPressedBackground);
    ReleaseInterface(m_pTodayDisabledBackground);
    ReleaseInterface(m_pTodayBlackoutBackground);
}
// create brush by a color
_Check_return_ HRESULT CCalendarView::GetBrushNoRef(_In_ UINT32 color, _Outptr_ CBrush** ppBrush)
{
    CDependencyObject* pBrush = nullptr;

    *ppBrush = nullptr;

    auto it = m_colorToBrushMap.find(color);
    if (it != m_colorToBrushMap.end())
    {
        pBrush = it->second.get();
    }
    else
    {
        CValue value;
        value.SetColor(color);
        CREATEPARAMETERS cp(GetContext(), value);
        xref_ptr<CDependencyObject> pBrushAsDO;
        IFC_RETURN(CBrush::Create(pBrushAsDO.ReleaseAndGetAddressOf(), &cp));
        pBrush = pBrushAsDO.get();
        m_colorToBrushMap.insert(std::make_pair(color, pBrushAsDO));
    }
    *ppBrush = static_cast<CBrush*>(pBrush);

    return S_OK;
}

_Check_return_ HRESULT CCalendarView::InitInstance()
{
    // it is a good time to setup the theme resource bindings for those internal properties.
    return SetupThemeResourceBinding();
}

_Check_return_ HRESULT CCalendarView::SetupThemeResourceBinding()
{
    ASSERT(m_internalThemeResourceExtensions.size() == g_internalPropertyKeys.size()
        && m_internalThemeResourceExtensions.size() == g_internalPropertyIndices.size()
        && m_internalThemeResourceExtensions.size() == g_internalPropertyExistences.size());

    auto core = GetContext();
    CREATEPARAMETERS cp(core);

    for (unsigned i = 0; i < m_internalThemeResourceExtensions.size(); ++i)
    {
        // Only use the hardcoded theme resources for properties that are still internal or when the new 'rounded corner'
        // rendering style is not applied.
        if (g_internalPropertyExistences[i] || !CCalendarViewBaseItemChrome::IsRoundedCalendarViewBaseItemChromeEnabled(core))
        {
            xref_ptr<CDependencyObject> initialValue;

            const xstring_ptr key = XSTRING_PTR_FROM_STORAGE(g_internalPropertyKeys[i]);
            auto propertyIndex = g_internalPropertyIndices[i];
            xref_ptr<CThemeResourceExtension> spThemeResourceExtension;

            // CCoreServices::LookupThemeResource will lazily instantiate the theme resource dictionary
            // if no one else has previously looked up a theme resource
            IFC_RETURN(core->LookupThemeResource(
                key,
                initialValue.ReleaseAndGetAddressOf()));

            IFC_RETURN(CThemeResourceExtension::Create(
                reinterpret_cast<CDependencyObject **>(spThemeResourceExtension.ReleaseAndGetAddressOf()),
                &cp));

            // Set key
            spThemeResourceExtension->m_strResourceKey = key;

            // Set inital value and target dictionary
            IFC_RETURN(spThemeResourceExtension->SetInitialValueAndTargetDictionary(
                initialValue,
                core->GetThemeResources()));

            IFC_RETURN(spThemeResourceExtension->SetThemeResourceBinding(
                this,
                DirectUI::MetadataAPI::GetPropertyByIndex(propertyIndex)));

            m_internalThemeResourceExtensions[i] = spThemeResourceExtension;
        }
        else
        {
            m_internalThemeResourceExtensions[i] = nullptr;
        }
    }

    return S_OK;
}
