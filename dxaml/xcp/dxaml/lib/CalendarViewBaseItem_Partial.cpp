// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "calendarviewbaseitem.g.h"
#include "CalendarView.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

// Called when the user presses a pointer down over the CalendarViewBaseItem.
IFACEMETHODIMP CalendarViewBaseItem::OnPointerPressed(
    _In_ IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN isHandled = FALSE;

    IFC(CalendarViewBaseItemGenerated::OnPointerPressed(pArgs));

    IFC(pArgs->get_Handled(&isHandled));
    if (!isHandled)
    {
        IFC(SetIsPressed(true));
        IFC(UpdateVisualStateInternal());
    }

Cleanup:
    return hr;
}

// Called when the user releases a pointer over the CalendarViewBaseItem.
IFACEMETHODIMP CalendarViewBaseItem::OnPointerReleased(
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN isHandled = FALSE;

    IFC(CalendarViewBaseItemGenerated::OnPointerReleased(pArgs));

    IFC(pArgs->get_Handled(&isHandled));
    if (!isHandled)
    {
        IFC(SetIsPressed(false));
        IFC(UpdateVisualStateInternal());
    }

Cleanup:
    return hr;
}

// Called when a pointer enters a CalendarViewBaseItem.
IFACEMETHODIMP CalendarViewBaseItem::OnPointerEntered(
    _In_ IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IPointer> spPointer;
    mui::PointerDeviceType pointerDeviceType = mui::PointerDeviceType_Touch;

    IFC(CalendarViewBaseItemGenerated::OnPointerEntered(pArgs));

    // Only update hover state if the pointer type isn't touch
    IFC(pArgs->get_Pointer(&spPointer));
    IFCPTR(spPointer);
    IFC(spPointer->get_PointerDeviceType(&pointerDeviceType));
    if (pointerDeviceType != mui::PointerDeviceType_Touch)
    {
        IFC(SetIsHovered(true));
        IFC(UpdateVisualStateInternal());
    }

Cleanup:
    return hr;
}

// Called when a pointer leaves a CalendarViewBaseItem.
IFACEMETHODIMP CalendarViewBaseItem::OnPointerExited(
    _In_ IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    IFC(CalendarViewBaseItemGenerated::OnPointerExited(pArgs));

    IFC(SetIsHovered(false));
    IFC(SetIsPressed(false));
    IFC(UpdateVisualStateInternal());

Cleanup:
    return hr;

}

// Called when the CalendarViewBaseItem or its children lose pointer capture.
IFACEMETHODIMP CalendarViewBaseItem::OnPointerCaptureLost(
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    IFC(CalendarViewBaseItemGenerated::OnPointerCaptureLost(pArgs));

    IFC(SetIsHovered(false));
    IFC(SetIsPressed(false));
    IFC(UpdateVisualStateInternal());

Cleanup:
    return hr;
}

// Called when the CalendarViewBaseItem receives focus.
IFACEMETHODIMP CalendarViewBaseItem::OnGotFocus(
    _In_ IRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    xaml::FocusState focusState = xaml::FocusState_Unfocused;

    IFC(CalendarViewBaseItemGenerated::OnGotFocus(pArgs));

    if (auto pCalendarView = GetParentCalendarView())
    {
        IFC(pCalendarView->OnItemFocused(this));
    }

    IFC(get_FocusState(&focusState));

    IFC(SetIsKeyboardFocused(focusState == xaml::FocusState_Keyboard));


Cleanup:
    return hr;
}

// Called when the CalendarViewBaseItem loses focus.
IFACEMETHODIMP CalendarViewBaseItem::OnLostFocus(
    _In_ IRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    IFC(CalendarViewBaseItemGenerated::OnLostFocus(pArgs));

    // remove keyboard focused state
    IFC(SetIsKeyboardFocused(false));

Cleanup:
    return hr;
}

IFACEMETHODIMP CalendarViewBaseItem::OnRightTapped(
    _In_ IRightTappedRoutedEventArgs* pArgs)
{
    IFC_RETURN(CalendarViewBaseItemGenerated::OnRightTapped(pArgs));

    BOOLEAN isHandled = FALSE;

    IFC_RETURN(pArgs->get_Handled(&isHandled));

    if (!isHandled)
    {
        bool ignored = false;
        IFC_RETURN(FocusSelfOrChild(xaml::FocusState::FocusState_Pointer, &ignored));
        IFC_RETURN(pArgs->put_Handled(TRUE));
    }
    return S_OK;
}

_Check_return_ HRESULT CalendarViewBaseItem::Initialize()
{
    IFC_RETURN(CalendarViewBaseItemGenerated::Initialize());

    IFC_RETURN(m_epSizeChangedHandler.AttachEventHandler(this,
        [this](IInspectable* sender, ISizeChangedEventArgs* args)
        {
            IFC_RETURN(OnSizeChanged(sender, args));

            return S_OK;
        }));

    return S_OK;
}

// Handle the SizeChanged event.
_Check_return_ HRESULT CalendarViewBaseItem::OnSizeChanged(_In_ IInspectable* sender, _In_ xaml::ISizeChangedEventArgs* args)
{
    if (HasStaticRoundedItemMargin())
    {
        const CCalendarViewBaseItemChrome* chrome = static_cast<CCalendarViewBaseItemChrome*>(GetHandle());

        if (chrome->IsRoundedCalendarViewBaseItemChromeEnabled())
        {
            // When rounded styling and hard-coded margins are applied by default, margins are potentially adjusted
            // to force a circular rendering of selection and focus cues.
            const double roundingStep = 1.0 / RootScale::GetRasterizationScaleForElement(GetHandle());
            wf::Size newItemSize{};

            IFC_RETURN(args->get_NewSize(&newItemSize));

            if (newItemSize.Width - newItemSize.Height > roundingStep || newItemSize.Height - newItemSize.Width > roundingStep)
            {
                // The new item width and height differ beyond the pixel snapping rounding, so potentially adjust the margins.

                const xaml::Thickness minimumItemMargin{ 1.0, 1.0, 1.0, 1.0 }; // Same margin as in the pre-rounded style rendering.
                const xaml::Thickness staticItemMargin = GetStaticRoundedItemMargin();
                const double marginDelta = (newItemSize.Width - newItemSize.Height) / 2.0;
                xaml::Thickness newItemMargin = staticItemMargin;
                xaml::Thickness oldItemMargin{};

                IFC_RETURN(get_Margin(&oldItemMargin));

                if (newItemSize.Width > newItemSize.Height && (oldItemMargin.Left < staticItemMargin.Left || oldItemMargin.Right < staticItemMargin.Right))
                {
                    // Increase the left and right margins & bring them as close to staticItemMargin as possible.
                    newItemMargin.Left = std::min(staticItemMargin.Left, oldItemMargin.Left + marginDelta);
                    newItemMargin.Right = std::min(staticItemMargin.Right, oldItemMargin.Right + marginDelta);
                }
                else if (newItemSize.Height > newItemSize.Width && (oldItemMargin.Top < staticItemMargin.Top || oldItemMargin.Bottom < staticItemMargin.Bottom))
                {
                    // Increase the top and bottom margins & bring them as close to staticItemMargin as possible.
                    newItemMargin.Top = std::min(staticItemMargin.Top, oldItemMargin.Top - marginDelta);
                    newItemMargin.Bottom = std::min(staticItemMargin.Bottom, oldItemMargin.Bottom - marginDelta);
                }
                else if (newItemSize.Width > newItemSize.Height && (oldItemMargin.Top > minimumItemMargin.Top || oldItemMargin.Bottom > minimumItemMargin.Bottom))
                {
                    // Decrease the top and bottom margins & bring them as close to staticItemMargin as possible.
                    newItemMargin.Top = std::max(minimumItemMargin.Top, oldItemMargin.Top - marginDelta);
                    newItemMargin.Bottom = std::max(minimumItemMargin.Bottom, oldItemMargin.Bottom - marginDelta);
                }
                else if (newItemSize.Height > newItemSize.Width && (oldItemMargin.Left > minimumItemMargin.Left || oldItemMargin.Right > minimumItemMargin.Right))
                {
                    // Decrease the left and right margins & bring them as close to staticItemMargin as possible.
                    newItemMargin.Left = std::max(minimumItemMargin.Left, oldItemMargin.Left + marginDelta);
                    newItemMargin.Right = std::max(minimumItemMargin.Right, oldItemMargin.Right + marginDelta);
                }
                else if (newItemSize.Width > newItemSize.Height)
                {
                    ASSERT(oldItemMargin.Top == minimumItemMargin.Top);
                    ASSERT(oldItemMargin.Bottom == minimumItemMargin.Bottom);

                    // Increase the left and right margins.
                    newItemMargin.Left = oldItemMargin.Left + marginDelta;
                    newItemMargin.Right = oldItemMargin.Right + marginDelta;
                    newItemMargin.Top = minimumItemMargin.Top;
                    newItemMargin.Bottom = minimumItemMargin.Bottom;
                }
                else if (newItemSize.Height > newItemSize.Width)
                {
                    ASSERT(oldItemMargin.Left == minimumItemMargin.Left);
                    ASSERT(oldItemMargin.Right == minimumItemMargin.Right);

                    // Increase the top and bottom margins.
                    newItemMargin.Left = minimumItemMargin.Left;
                    newItemMargin.Right = minimumItemMargin.Right;
                    newItemMargin.Top = oldItemMargin.Top - marginDelta;
                    newItemMargin.Bottom = oldItemMargin.Bottom - marginDelta;
                }

                IFC_RETURN(put_Margin(newItemMargin));
            }
        }
    }

    IFC_RETURN(UpdateBlackoutStrikethroughSize());
    IFC_RETURN(UpdateCornerRadius());

    return S_OK;
}

_Check_return_ HRESULT CalendarViewBaseItem::OnIsEnabledChanged(_In_ IsEnabledChangedEventArgs* pArgs)
{
    IFC_RETURN(UpdateBackgroundAndBorderBrushes());

    return UpdateTextBlockForeground();
}

_Check_return_ HRESULT CalendarViewBaseItem::EnterImpl(
    _In_ bool bLive,
    _In_ bool bSkipNameRegistration,
    _In_ bool bCoercedIsEnabled,
    _In_ bool bUseLayoutRounding)
{
    IFC_RETURN(CalendarViewBaseItemGenerated::EnterImpl(bLive, bSkipNameRegistration, bCoercedIsEnabled, bUseLayoutRounding));

    if (bLive)
    {
        // In case any of the TextBlock properties have been updated while
        // we were out of the visual tree, we should update them in order to ensure
        // that we always have the most up-to-date values.
        // An example where this can happen is if the theme changes while
        // the flyout holding the CalendarView for a CalendarDatePicker is closed.
        IFC_RETURN(UpdateTextBlockForeground());
        IFC_RETURN(UpdateTextBlockFontProperties());
        IFC_RETURN(UpdateTextBlockAlignments());
        IFC_RETURN(UpdateTextBlockMargin());
        IFC_RETURN(UpdateVisualStateInternal());

        // Also make sure the chrome's m_outerBorder, m_innerBorder and/or m_strikethroughLine get created and apply the correct brushes when they are not null.
        IFC_RETURN(UpdateBackgroundAndBorderBrushes());
    }

    return S_OK;
}

void CalendarViewBaseItem::SetParentCalendarView(_In_opt_ CalendarView* pCalendarView)
{
    m_pParentCalendarView = pCalendarView;
    CCalendarViewBaseItemChrome* pChrome = static_cast<CCalendarViewBaseItemChrome*>(GetHandle());

    pChrome->SetOwner(pCalendarView ? static_cast<CCalendarView*>(pCalendarView->GetHandle()) : nullptr);
}

CalendarView* CalendarViewBaseItem::GetParentCalendarView()
{
    return m_pParentCalendarView;
}

_Check_return_ HRESULT CalendarViewBaseItem::UpdateMainText(_In_ HSTRING mainText)
{
    CCalendarViewBaseItemChrome* pChrome = static_cast<CCalendarViewBaseItemChrome*>(GetHandle());
    return pChrome->UpdateMainText(mainText);
}

_Check_return_ HRESULT CalendarViewBaseItem::UpdateLabelText(_In_ HSTRING labelText)
{
    CCalendarViewBaseItemChrome* pChrome = static_cast<CCalendarViewBaseItemChrome*>(GetHandle());
    return pChrome->UpdateLabelText(labelText);
}

_Check_return_ HRESULT CalendarViewBaseItem::ShowLabelText(_In_ bool showLabel)
{
    CCalendarViewBaseItemChrome* pChrome = static_cast<CCalendarViewBaseItemChrome*>(GetHandle());
    return pChrome->ShowLabelText(showLabel);
}

_Check_return_ HRESULT CalendarViewBaseItem::GetMainText(_Out_ HSTRING* pMainText)
{
    CCalendarViewBaseItemChrome* pChrome = static_cast<CCalendarViewBaseItemChrome*>(GetHandle());
    return pChrome->GetMainText(pMainText);
}

_Check_return_ HRESULT CalendarViewBaseItem::SetIsToday(_In_ bool state)
{
    CCalendarViewBaseItemChrome* pChrome = static_cast<CCalendarViewBaseItemChrome*>(GetHandle());
    return pChrome->SetIsToday(state);
}

_Check_return_ HRESULT CalendarViewBaseItem::SetIsKeyboardFocused(_In_ bool state)
{
    CCalendarViewBaseItemChrome* pChrome = static_cast<CCalendarViewBaseItemChrome*>(GetHandle());
    return pChrome->SetIsKeyboardFocused(state);
}

_Check_return_ HRESULT CalendarViewBaseItem::SetIsSelected(_In_ bool state)
{
    CCalendarViewBaseItemChrome* pChrome = static_cast<CCalendarViewBaseItemChrome*>(GetHandle());
    return pChrome->SetIsSelected(state);
}

_Check_return_ HRESULT CalendarViewBaseItem::SetIsBlackout(_In_ bool state)
{
    CCalendarViewBaseItemChrome* pChrome = static_cast<CCalendarViewBaseItemChrome*>(GetHandle());
    return pChrome->SetIsBlackout(state);
}

_Check_return_ HRESULT CalendarViewBaseItem::SetIsHovered(_In_ bool state)
{
    CCalendarViewBaseItemChrome* pChrome = static_cast<CCalendarViewBaseItemChrome*>(GetHandle());
    return pChrome->SetIsHovered(state);
}

_Check_return_ HRESULT CalendarViewBaseItem::SetIsPressed(_In_ bool state)
{
    CCalendarViewBaseItemChrome* pChrome = static_cast<CCalendarViewBaseItemChrome*>(GetHandle());
    return pChrome->SetIsPressed(state);
}

_Check_return_ HRESULT CalendarViewBaseItem::SetIsOutOfScope(_In_ bool state)
{
    CCalendarViewBaseItemChrome* pChrome = static_cast<CCalendarViewBaseItemChrome*>(GetHandle());
    return pChrome->SetIsOutOfScope(state);
}

// If this item is unfocused, sets focus on the CalendarViewBaseItem.
// Otherwise, sets focus to whichever element currently has focus
// (so focusState can be propagated).
_Check_return_ HRESULT CalendarViewBaseItem::FocusSelfOrChild(
    _In_ xaml::FocusState focusState,
    _Out_ bool* pFocused,
    _In_ xaml_input::FocusNavigationDirection focusNavigationDirection)
{
    HRESULT hr = S_OK;
    BOOLEAN isItemAlreadyFocused = FALSE;
    ctl::ComPtr<DependencyObject> spItemToFocus = NULL;

    *pFocused = false;

    IFC(HasFocus(&isItemAlreadyFocused));
    if (isItemAlreadyFocused)
    {
        // Re-focus the currently focused item to propagate focusState (the item might be focused
        // under a different FocusState value).
        IFC(GetFocusedElement(&spItemToFocus));
    }
    else
    {
        spItemToFocus = this;
    }

    if (spItemToFocus)
    {
        BOOLEAN focused = FALSE;
        IFC(SetFocusedElementWithDirection(spItemToFocus.Get(), focusState, FALSE /*animateIfBringIntoView*/, &focused, focusNavigationDirection));
        *pFocused = !!focused;
    }

Cleanup:
    return hr;
}

#if DBG
// wf::DateTime has an int64 member which is not intuitive enough. This method will convert it
// into numbers that we can easily read.
_Check_return_ HRESULT CalendarViewBaseItem::SetDateForDebug(_In_ wf::DateTime value)
{
    HRESULT hr = S_OK;

    auto pCalendarView = GetParentCalendarView();
    if (pCalendarView)
    {
        auto pCalendar = pCalendarView->GetCalendar();
        IFC(pCalendar->SetDateTime(value));
        IFC(pCalendar->get_Era(&m_eraForDebug));
        IFC(pCalendar->get_Year(&m_yearForDebug));
        IFC(pCalendar->get_Month(&m_monthForDebug));
        IFC(pCalendar->get_Day(&m_dayForDebug));
    }

Cleanup:
    return hr;
}
#endif


_Check_return_ HRESULT CalendarViewBaseItem::InvalidateRender()
{
    CCalendarViewBaseItemChrome* pChrome = static_cast<CCalendarViewBaseItemChrome*>(GetHandle());
    pChrome->InvalidateRender();
    return S_OK;
}

_Check_return_ HRESULT CalendarViewBaseItem::UpdateTextBlockForeground()
{
    CCalendarViewBaseItemChrome* pChrome = static_cast<CCalendarViewBaseItemChrome*>(GetHandle());
    return pChrome->UpdateTextBlocksForeground();
}

_Check_return_ HRESULT CalendarViewBaseItem::UpdateTextBlockFontProperties()
{
    CCalendarViewBaseItemChrome* pChrome = static_cast<CCalendarViewBaseItemChrome*>(GetHandle());
    return pChrome->UpdateTextBlocksFontProperties();
}

_Check_return_ HRESULT CalendarViewBaseItem::UpdateTextBlockAlignments()
{
    CCalendarViewBaseItemChrome* pChrome = static_cast<CCalendarViewBaseItemChrome*>(GetHandle());
    return pChrome->UpdateTextBlocksAlignments();
}

_Check_return_ HRESULT CalendarViewBaseItem::UpdateTextBlockMargin()
{
    CCalendarViewBaseItemChrome* chrome = static_cast<CCalendarViewBaseItemChrome*>(GetHandle());

    if (chrome->IsRoundedCalendarViewBaseItemChromeEnabled())
    {
        return chrome->UpdateTextBlocksMargin();
    }

    return S_OK;
}

_Check_return_ HRESULT CalendarViewBaseItem::UpdateBackgroundAndBorderBrushes()
{
    CCalendarViewBaseItemChrome* chrome = static_cast<CCalendarViewBaseItemChrome*>(GetHandle());

    if (chrome->IsRoundedCalendarViewBaseItemChromeEnabled())
    {
        return chrome->UpdateBackgroundAndBorderBrushes();
    }

    return S_OK;
}

_Check_return_ HRESULT CalendarViewBaseItem::UpdateBlackoutStrikethroughSize()
{
    CCalendarViewBaseItemChrome* chrome = static_cast<CCalendarViewBaseItemChrome*>(GetHandle());

    if (chrome->IsRoundedCalendarViewBaseItemChromeEnabled())
    {
        IFC_RETURN(chrome->UpdateBlackoutStrikethroughSize());
    }

    return S_OK;
}

_Check_return_ HRESULT CalendarViewBaseItem::UpdateCornerRadius()
{
    CCalendarViewBaseItemChrome* chrome = static_cast<CCalendarViewBaseItemChrome*>(GetHandle());

    if (chrome->IsRoundedCalendarViewBaseItemChromeEnabled())
    {
        IFC_RETURN(chrome->UpdateCornerRadius());
    }

    return S_OK;
}

// Change to the correct visual state for the CalendarViewBaseItem.
_Check_return_ HRESULT CalendarViewBaseItem::ChangeVisualState(
    // true to use transitions when updating the visual state, false
    // to snap directly to the new visual state.
    bool bUseTransitions)
{
    IFC_RETURN(CalendarViewBaseItemGenerated::ChangeVisualState(bUseTransitions));

    CCalendarViewBaseItemChrome* chrome = static_cast<CCalendarViewBaseItemChrome*>(GetHandle());
    BOOLEAN ignored = FALSE;
    BOOLEAN isPointerOver = chrome->IsHovered();
    BOOLEAN isPressed = chrome->IsPressed();

    // Common States Group
    if (isPressed)
    {
        IFC_RETURN(GoToState(bUseTransitions, L"Pressed", &ignored));
    }
    else if (isPointerOver)
    {
        IFC_RETURN(GoToState(bUseTransitions, L"PointerOver", &ignored));
    }
    else
    {
        IFC_RETURN(GoToState(bUseTransitions, L"Normal", &ignored));
    }

    return S_OK;
}

_Check_return_ HRESULT CalendarViewBaseItem::UpdateVisualStateInternal()
{
    CCalendarViewBaseItemChrome* chrome = static_cast<CCalendarViewBaseItemChrome*>(GetHandle());
    if (chrome->HasTemplateChild()) // If !HasTemplateChild, then there is no visual in ControlTemplate for CalendarViewDayItemStyle
                                    // There should be no VisualStateGroup defined, so ignore UpdateVisualState
    {
        IFC_RETURN(UpdateVisualState(FALSE /* fUseTransitions */));
    }

    return S_OK;
}
