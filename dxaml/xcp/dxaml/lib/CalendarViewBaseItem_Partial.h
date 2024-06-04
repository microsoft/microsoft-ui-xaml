// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "calendarviewbaseitem.g.h"

namespace DirectUI
{

    class CalendarView;

    PARTIAL_CLASS(CalendarViewBaseItem)
        , public ICalendarViewBaseItem
    {
    public:
        CalendarViewBaseItem()
            : m_pParentCalendarView(nullptr)
#if DBG
            , m_eraForDebug(0)
            , m_yearForDebug(0)
            , m_monthForDebug(0)
            , m_dayForDebug(0)
#endif
        {

        }

    protected:

        // this base panel implementation is hidden from IDL
        _Check_return_ HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override
        {
            if (InlineIsEqualGUID(iid, __uuidof(ICalendarViewBaseItem)))
            {
                *ppObject = static_cast<ICalendarViewBaseItem*>(this);
            }
            else
            {
                RRETURN(CalendarViewBaseItemGenerated::QueryInterfaceImpl(iid, ppObject));
            }

            AddRefOuter();
            RRETURN(S_OK);
        }

        // Called when the user presses a pointer down over the
        // CalendarViewBaseItem.
        IFACEMETHOD(OnPointerPressed)(
            _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
            override;

        // Called when the user releases a pointer over the
        // CalendarViewBaseItem.
        IFACEMETHOD(OnPointerReleased)(
            _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
            override;

        // Called when a pointer enters a CalendarViewBaseItem.
        IFACEMETHOD(OnPointerEntered)(
            _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
            override;

        // Called when a pointer leaves a CalendarViewBaseItem.
        IFACEMETHOD(OnPointerExited)(
            _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
            override;

        // Called when the CalendarViewBaseItem or its children lose pointer capture.
        IFACEMETHOD(OnPointerCaptureLost)(
            _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
            override;

        // Called when the CalendarViewBaseItem receives focus.
        IFACEMETHOD(OnGotFocus)(
            _In_ xaml::IRoutedEventArgs* pArgs)
            override;

        // Called when the CalendarViewBaseItem loses focus.
        IFACEMETHOD(OnLostFocus)(
            _In_ xaml::IRoutedEventArgs* pArgs)
            override;

        IFACEMETHOD(OnRightTapped)(_In_ xaml_input::IRightTappedRoutedEventArgs* pArgs) override;

        // Called when the IsEnabled property changes.
        _Check_return_ HRESULT OnIsEnabledChanged(_In_ DirectUI::IsEnabledChangedEventArgs* pArgs) override;

        // Called when the element enters the tree. Refreshes visual state.
        _Check_return_ HRESULT EnterImpl(
            _In_ bool bLive,
            _In_ bool bSkipNameRegistration,
            _In_ bool bCoercedIsEnabled,
            _In_ bool bUseLayoutRounding) final;

    public:
        void SetParentCalendarView(_In_opt_ CalendarView* pCalendarView);

        CalendarView* GetParentCalendarView();

        _Check_return_ HRESULT SetIsToday(_In_ bool state);
        _Check_return_ HRESULT SetIsKeyboardFocused(_In_ bool state);
        _Check_return_ HRESULT SetIsSelected(_In_ bool state);
        _Check_return_ HRESULT SetIsBlackout(_In_ bool state);
        _Check_return_ HRESULT SetIsHovered(_In_ bool state);
        _Check_return_ HRESULT SetIsPressed(_In_ bool state);
        _Check_return_ HRESULT SetIsOutOfScope(_In_ bool state);

        _Check_return_ HRESULT UpdateText(_In_ HSTRING mainText, _In_opt_ HSTRING labelText, _In_ bool showLabel);
        _Check_return_ HRESULT UpdateMainText(_In_ HSTRING mainText);
        _Check_return_ HRESULT UpdateLabelText(_In_ HSTRING labelText);
        _Check_return_ HRESULT ShowLabelText(_In_ bool showLabel);
        _Check_return_ HRESULT GetMainText(_Out_ HSTRING* pMainText);

        _Check_return_ HRESULT Initialize() override;

        // CalendarViewItem and CalendarViewDayItem will override this method.
        virtual _Check_return_ HRESULT GetDate(_Out_ wf::DateTime* pDate) { return E_NOTIMPL; }

        _Check_return_ HRESULT FocusSelfOrChild(
            _In_ xaml::FocusState focusState,
            _Out_ bool* pFocused,
            _In_ xaml_input::FocusNavigationDirection focusNavigationDirection = xaml_input::FocusNavigationDirection::FocusNavigationDirection_None);

        // invalidate render to make sure chrome properties (background, border, ...) get updated
        _Check_return_ HRESULT InvalidateRender();

        _Check_return_ HRESULT UpdateTextBlockForeground();

        _Check_return_ HRESULT UpdateTextBlockFontProperties();

        _Check_return_ HRESULT UpdateTextBlockAlignments();

        _Check_return_ HRESULT UpdateTextBlockMargin();

        _Check_return_ HRESULT UpdateBackgroundAndBorderBrushes();

        _Check_return_ HRESULT UpdateBlackoutStrikethroughSize();

        _Check_return_ HRESULT UpdateCornerRadius();

#if DBG
        // wf::DateTime has an int64 member which is not intutive enough. This method will convert it
        // into numbers that we can easily read.
        _Check_return_ HRESULT SetDateForDebug(_In_ wf::DateTime value);
#endif

    protected:
        _Check_return_ HRESULT ChangeVisualState(bool useTransitions = true) override;

    private:
        _Check_return_ HRESULT UpdateVisualStateInternal();

        // Handle the SizeChanged event.
        _Check_return_ HRESULT OnSizeChanged(
            _In_ IInspectable* sender,
            _In_ xaml::ISizeChangedEventArgs* args);

        // Returns True because when rounded styling is applied, items by default use hard-coded margins as no public Style is exposed for them.
        virtual bool HasStaticRoundedItemMargin() const { return true; }
        // Returns the hard-coded margins for items when rounded styling is applied.
        virtual xaml::Thickness GetStaticRoundedItemMargin() const { return { 9.0, 9.0, 9.0, 9.0 }; }

    private:
        CalendarView* m_pParentCalendarView;

        ctl::EventPtr<FrameworkElementSizeChangedEventCallback> m_epSizeChangedHandler;

#if DBG
        int m_eraForDebug;
        int m_yearForDebug;
        int m_monthForDebug;
        int m_dayForDebug;
#endif
    };
}
