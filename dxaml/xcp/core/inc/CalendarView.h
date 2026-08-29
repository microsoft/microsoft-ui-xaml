// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once


class CCalendarView: public CControl
{
protected:
    CCalendarView(_In_ CCoreServices *pCore);

    ~CCalendarView() noexcept override;

public:
    DECLARE_CREATE(CCalendarView);

    KnownTypeIndex GetTypeIndex() const override
    {
        return KnownTypeIndex::CalendarView;
    }

    bool IsCalendarItemCornerRadiusPropertySet() const
    {
        return !IsPropertyDefaultByIndex(KnownPropertyIndex::CalendarView_CalendarItemCornerRadius);
    }

    _Check_return_ HRESULT GetBrushNoRef(_In_ UINT32 color, _Outptr_ CBrush** ppBrush);

    _Check_return_ HRESULT InitInstance() override;

private:
    _Check_return_ HRESULT SetupThemeResourceBinding();
public:
    // Public properties

    CBrush*                             m_pFocusBorderBrush;
    CBrush*                             m_pSelectedHoverBorderBrush;
    CBrush*                             m_pSelectedPressedBorderBrush;
    CBrush*                             m_pSelectedDisabledBorderBrush;
    CBrush*                             m_pSelectedBorderBrush;
    CBrush*                             m_pHoverBorderBrush;
    CBrush*                             m_pPressedBorderBrush;
    CBrush*                             m_pCalendarItemBorderBrush;
    CBrush*                             m_pTodaySelectedInnerBorderBrush;
    CBrush*                             m_pBlackoutStrikethroughBrush;
    CBrush*                             m_pBlackoutBackground;
    CBrush*                             m_pOutOfScopeBackground;
    CBrush*                             m_pCalendarItemBackground;
    CBrush*                             m_pCalendarItemHoverBackground;
    CBrush*                             m_pCalendarItemPressedBackground;
    CBrush*                             m_pCalendarItemDisabledBackground;
    CBrush*                             m_pTodayBackground;
    CBrush*                             m_pTodayBlackoutBackground;
    CBrush*                             m_pTodayHoverBackground;
    CBrush*                             m_pTodayPressedBackground;
    CBrush*                             m_pTodayDisabledBackground;

    CBrush*                             m_pDisabledForeground;
    CBrush*                             m_pPressedForeground;
    CBrush*                             m_pTodayForeground;
    CBrush*                             m_pBlackoutForeground;
    CBrush*                             m_pTodayBlackoutForeground;
    CBrush*                             m_pSelectedForeground;
    CBrush*                             m_pSelectedHoverForeground;
    CBrush*                             m_pSelectedPressedForeground;
    CBrush*                             m_pSelectedDisabledForeground;
    CBrush*                             m_pOutOfScopeForeground;
    CBrush*                             m_pOutOfScopeHoverForeground;
    CBrush*                             m_pOutOfScopePressedForeground;
    CBrush*                             m_pCalendarItemForeground;

    CFontFamily*                        m_pDayItemFontFamily;
    XFLOAT                              m_dayItemFontSize;
    DirectUI::FontStyle                 m_dayItemFontStyle;
    DirectUI::CoreFontWeight            m_dayItemFontWeight;
    DirectUI::CoreFontWeight            m_todayFontWeight;

    CFontFamily*                        m_pFirstOfMonthLabelFontFamily;
    XFLOAT                              m_firstOfMonthLabelFontSize;
    DirectUI::FontStyle                 m_firstOfMonthLabelFontStyle;
    DirectUI::CoreFontWeight            m_firstOfMonthLabelFontWeight;

    CFontFamily*                        m_pMonthYearItemFontFamily;
    XFLOAT                              m_monthYearItemFontSize;
    DirectUI::FontStyle                 m_monthYearItemFontStyle;
    DirectUI::CoreFontWeight            m_monthYearItemFontWeight;

    CFontFamily*                        m_pFirstOfYearDecadeLabelFontFamily;
    XFLOAT                              m_firstOfYearDecadeLabelFontSize;
    DirectUI::FontStyle                 m_firstOfYearDecadeLabelFontStyle;
    DirectUI::CoreFontWeight            m_firstOfYearDecadeLabelFontWeight;

    DirectUI::HorizontalAlignment       m_horizontalDayItemAlignment;
    DirectUI::VerticalAlignment         m_verticalDayItemAlignment;

    DirectUI::HorizontalAlignment       m_horizontalFirstOfMonthLabelAlignment;
    DirectUI::VerticalAlignment         m_verticalFirstOfMonthLabelAlignment;

    XTHICKNESS                          m_dayItemMargin;
    XTHICKNESS                          m_monthYearItemMargin;
    XTHICKNESS                          m_firstOfMonthLabelMargin;
    XTHICKNESS                          m_firstOfYearDecadeLabelMargin;
    XTHICKNESS                          m_calendarItemBorderThickness;
    XCORNERRADIUS                       m_calendarItemCornerRadius;

    // Below brushes are hardcoded and internal, we can make them public when needed.
    CBrush*                             m_pTodayHoverBorderBrush;
    CBrush*                             m_pTodayPressedBorderBrush;

private:
    std::map<UINT32, xref_ptr<CDependencyObject>> m_colorToBrushMap;

    // hold the resource extension to keep the bindings for the internal properties.
    std::array<xref_ptr<CThemeResourceExtension>, 6> m_internalThemeResourceExtensions;
};
