// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Represents a control that lets the user select from a range of values
//      by moving a Thumb control along a track.

#include "precomp.h"
#include "Slider.g.h"
#include "Thumb.g.h"
#include "SliderAutomationPeer.g.h"
#include "ToolTipService.g.h"
#include "ToolTip.g.h"
#include "Grid.g.h"
#include "Rectangle.g.h"
#include "TickBar.g.h"
#include "PointerRoutedEventArgs.g.h"
#include "TextBlock.g.h"
#include "Binding.g.h"
#include "KeyRoutedEventArgs.g.h"
#include "InputServices.h"
#include <microsoft.ui.text.h>

#pragma warning(disable:4267) //'var' : conversion from 'size_t' to 'type', possible loss of data

using namespace DirectUI;
using namespace DirectUISynonyms;
using namespace std::placeholders;

// Uncomment to get TickBar debug traces
// #define TICKBAR_DBG_SLIDER

#ifdef TICKBAR_DBG_SLIDER
#define g_szTickBarDbgSliderLen 300
WCHAR g_szTickBarDbgSlider[g_szTickBarDbgSliderLen];
#endif // TICKBAR_DBG_SLIDER

// Initializes a new instance of the Slider class.
Slider::Slider()
    : m_IsPointerOver(FALSE)
    , m_bProcessingInputEvent(FALSE)
    , m_isPressed(FALSE)
    , m_DragValue(0.0)
    , m_bUsingDefaultToolTipForHorizontalThumb(FALSE)
    , m_bUsingDefaultToolTipForVerticalThumb(FALSE)
    , m_capturedPointerId(0)
{
}

// Destroys an instance of the Slider class.
Slider::~Slider()
{
}

// Sets up instances that are expected on this type.
_Check_return_ HRESULT Slider::PrepareState()
{
    IFC_RETURN(add_FocusEngaged(
        wrl::Callback<wf::ITypedEventHandler<xaml_controls::Control*, xaml::Controls::FocusEngagedEventArgs*>>(this, &Slider::OnFocusEngaged).Get(),
        &m_focusEngagedToken));

    IFC_RETURN(add_FocusDisengaged(
        wrl::Callback<wf::ITypedEventHandler<xaml_controls::Control*, xaml::Controls::FocusDisengagedEventArgs*>>(this, &Slider::OnFocusDisengaged).Get(),
        &m_focusDisengagedToken));

    return S_OK;
}

_Check_return_ HRESULT Slider::GetDefaultValue2(
    _In_ const CDependencyProperty* pDP,
    _Out_ CValue* pValue)
{
    HRESULT hr = S_OK;

    switch (pDP->GetIndex())
    {
    case KnownPropertyIndex::RangeBase_Maximum:
        pValue->SetDouble(SLIDER_DEFAULT_MAXIMUM);
        break;
    case KnownPropertyIndex::RangeBase_LargeChange:
        pValue->SetDouble(SLIDER_DEFAULT_LARGE_CHANGE);
        break;
    case KnownPropertyIndex::RangeBase_SmallChange:
        pValue->SetDouble(SLIDER_DEFAULT_SMALL_CHANGE);
        break;
    default:
        IFC(SliderGenerated::GetDefaultValue2(pDP, pValue));
        break;
    }

Cleanup:
    return hr;
}

// Handle the custom property changed event and call the
// OnPropertyChanged2 methods.
_Check_return_
HRESULT
Slider::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    IFC_RETURN(SliderGenerated::OnPropertyChanged2(args));

    switch (args.m_pDP->GetIndex())
    {
        case KnownPropertyIndex::Slider_Orientation:
            IFC_RETURN(OnOrientationChanged());
            IFC_RETURN(UpdateVisualState());
            break;

        case KnownPropertyIndex::Slider_IsDirectionReversed:
        case KnownPropertyIndex::Slider_IntermediateValue:
            IFC_RETURN(UpdateTrackLayout());
            break;

        case KnownPropertyIndex::Slider_TickPlacement:
            IFC_RETURN(OnTickPlacementChanged());
            break;

        case KnownPropertyIndex::Slider_IsThumbToolTipEnabled:
            IFC_RETURN(OnIsThumbToolTipEnabledChanged());
            break;

        case KnownPropertyIndex::Slider_ThumbToolTipValueConverter:
            IFC_RETURN(OnThumbToolTipValueConverterChanged());
            break;

        case KnownPropertyIndex::UIElement_Visibility:
            IFC_RETURN(OnVisibilityChanged());
            break;

        case KnownPropertyIndex::Slider_TickFrequency:
            IFC_RETURN(InvalidateTickBarsArrange());
            break;

        case KnownPropertyIndex::Slider_Header:
        case KnownPropertyIndex::Slider_HeaderTemplate:
            IFC_RETURN(UpdateHeaderPresenterVisibility());
            break;

        case KnownPropertyIndex::Control_IsFocusEngaged:
            IFC_RETURN(OnIsFocusEngagedChanged());
            break;

#if WI_IS_FEATURE_PRESENT(Feature_HeaderPlacement)
        case KnownPropertyIndex::Slider_HeaderPlacement:
            IFC_RETURN(UpdateVisualState());
            break;
#endif
    }

    return S_OK;
}

// OnFocusEngaged property changed handler.
_Check_return_ HRESULT Slider::OnFocusEngaged(
    _In_ xaml_controls::IControl* sender,
    _In_ xaml_controls::IFocusEngagedEventArgs* args)
{
    IFC_RETURN(get_Value(&m_preEngagementValue));

    return S_OK;
}

// OnFocusEngaged property changed handler.
_Check_return_ HRESULT Slider::OnFocusDisengaged(
    _In_ xaml_controls::IControl* sender,
    _In_ xaml_controls::IFocusDisengagedEventArgs* args)
{
    // Revert value
    if (m_preEngagementValue >= 0.0)
    {
        IFC_RETURN(put_Value(m_preEngagementValue));
        m_preEngagementValue = -1.0f;
    }

    return S_OK;
}

// IsEnabled property changed handler.
_Check_return_ HRESULT Slider::OnIsEnabledChanged(
    _In_ IsEnabledChangedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN isEnabled = FALSE;

    IFC(SliderGenerated::OnIsEnabledChanged(pArgs));

    IFC(get_IsEnabled(&isEnabled));
    if (!isEnabled)
    {
        m_IsPointerOver = FALSE;
        m_isPressed = FALSE;
    }
    IFC(UpdateVisualState());

Cleanup:
    RRETURN(hr);
}

// Update the visual states when the Visibility property is changed.
_Check_return_ HRESULT
Slider::OnVisibilityChanged()
{
    HRESULT hr = S_OK;
    xaml::Visibility visibility = xaml::Visibility_Collapsed;

    IFC(get_Visibility(&visibility));
    if (xaml::Visibility_Visible != visibility)
    {
        m_IsPointerOver = FALSE;
        m_isPressed = FALSE;
    }

    IFC(UpdateVisualState());

Cleanup:
    RRETURN(hr);
}

// GotFocus event handler.
IFACEMETHODIMP Slider::OnGotFocus(
    _In_ xaml::IRoutedEventArgs* pArgs)
{
    IFC_RETURN(SliderGenerated::OnGotFocus(pArgs));
    IFC_RETURN(UpdateVisualState(TRUE));

    xaml::FocusState focusState = xaml::FocusState_Unfocused;
    IFC_RETURN(get_FocusState(&focusState));
    if (xaml::FocusState_Keyboard == focusState)
    {
        CContentRoot* contentRoot = VisualTree::GetContentRootForElement(GetHandle());
        bool inputIsGamePadOrRemote = contentRoot->GetInputManager().GetLastInputDeviceType() == DirectUI::InputDeviceType::GamepadOrRemote;

        BOOLEAN isFocusEngaged = FALSE;
        IFC_RETURN(get_IsFocusEngaged(&isFocusEngaged));

        BOOLEAN isFocusEngagementEnabled = FALSE;
        IFC_RETURN(get_IsFocusEngagementEnabled(&isFocusEngagementEnabled));

        BOOLEAN isThumbToolTipEnabled = FALSE;
        IFC_RETURN(get_IsThumbToolTipEnabled(&isThumbToolTipEnabled));

        const bool shouldShowToolTip = isThumbToolTipEnabled && (!inputIsGamePadOrRemote || !isFocusEngagementEnabled || isFocusEngaged);
        if (shouldShowToolTip)
        {
            IFC_RETURN(UpdateThumbToolTipVisibility(TRUE, AutomaticToolTipInputMode::Keyboard));
        }
    }

    return S_OK;
}

// LostFocus event handler.
IFACEMETHODIMP Slider::OnLostFocus(
    _In_ xaml::IRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    IFC(SliderGenerated::OnLostFocus(pArgs));
    IFC(UpdateVisualState(TRUE));
    IFC(UpdateThumbToolTipVisibility(FALSE));

Cleanup:
    RRETURN(hr);
}

// KeyDown event handler.
IFACEMETHODIMP Slider::OnKeyDown(
    _In_ xaml_input::IKeyRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN handled = FALSE;
    BOOLEAN enabled = FALSE;
    wsy::VirtualKey key = wsy::VirtualKey_None;
    BOOLEAN keyPressHandled = TRUE;
    xaml::FocusState focusState = xaml::FocusState_Unfocused;
    BOOLEAN bIgnored;

    IFCPTR(pArgs);
    IFC(SliderGenerated::OnKeyDown(pArgs));
    IFC(pArgs->get_Handled(&handled));
    if (handled)
    {
        goto Cleanup;
    }

    IFC(get_IsEnabled(&enabled));
    if (!enabled)
    {
        goto Cleanup;
    }

    IFC(static_cast<KeyRoutedEventArgs*>(pArgs)->get_OriginalKey(&key));

    IFC(KeyPress::Slider::KeyDown<Slider>(key, this, &keyPressHandled));

    if (keyPressHandled)
    {
        IFC(get_FocusState(&focusState));
        if (xaml::FocusState_Keyboard != focusState)
        {
            IFC(Focus(xaml::FocusState_Keyboard, &bIgnored));
            IFC(UpdateVisualState(TRUE));
            IFC(UpdateThumbToolTipVisibility(TRUE, AutomaticToolTipInputMode::Keyboard));
        }

        IFC(pArgs->put_Handled(TRUE));
    }

Cleanup:
    RRETURN(hr);
}

// PreviewKeyUp event handler.
IFACEMETHODIMP Slider::OnPreviewKeyDownImpl(
    _In_ xaml_input::IKeyRoutedEventArgs* args)
{
    wsy::VirtualKey key = wsy::VirtualKey_None;
    IFC_RETURN(static_cast<KeyRoutedEventArgs*>(args)->get_OriginalKey(&key));

    BOOLEAN isFocusEngagementEnabled = FALSE;
    IFC_RETURN(get_IsFocusEngagementEnabled(&isFocusEngagementEnabled));

    BOOLEAN isFocusEngaged = FALSE;
    IFC_RETURN(get_IsFocusEngaged(&isFocusEngaged));

    BOOLEAN isHandled = FALSE;
    IFC_RETURN(args->get_Handled(&isHandled));

    if (isFocusEngagementEnabled && isFocusEngaged && !isHandled)
    {
        if (key == wsy::VirtualKey_GamepadA)
        {
            // "Commit" value, aka do nothing
            m_preEngagementValue = -1.0f;
            IFC_RETURN(RemoveFocusEngagement());
            IFC_RETURN(args->put_Handled(TRUE));

            m_disengagedWithA = true;
        }
        else if (key == wsy::VirtualKey_GamepadB)
        {
            // Revert value
            IFC_RETURN(put_Value(m_preEngagementValue));
            m_preEngagementValue = -1.0f;
            IFC_RETURN(RemoveFocusEngagement());
            IFC_RETURN(args->put_Handled(TRUE));
        }
    }

    return S_OK;
}

// PreviewKeyUp event handler.
IFACEMETHODIMP Slider::OnPreviewKeyUpImpl(
    _In_ xaml_input::IKeyRoutedEventArgs* args)
{
    wsy::VirtualKey key = wsy::VirtualKey_None;
    IFC_RETURN(static_cast<KeyRoutedEventArgs*>(args)->get_OriginalKey(&key));

    BOOLEAN isFocusEngagementEnabled = FALSE;
    IFC_RETURN(get_IsFocusEngagementEnabled(&isFocusEngagementEnabled));

    if (isFocusEngagementEnabled)
    {
        if (key == wsy::VirtualKey_GamepadA)
        {
            if (m_disengagedWithA)
            {
                // Block the re-engagement
                m_disengagedWithA = false; // We want to do this regardless of handled
                IFC_RETURN(args->put_Handled(TRUE));
            }
        }
    }

    return S_OK;
}

// PointerEnter event handler.
IFACEMETHODIMP Slider::OnPointerEntered(
    _In_ IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<Thumb> spThumb;
    BOOLEAN isDragging = FALSE;
    mui::PointerDeviceType nPointerDeviceType = mui::PointerDeviceType_Touch;
    xaml::FocusState focusState = xaml::FocusState_Unfocused;
    ctl::ComPtr<ixp::IPointerPoint> spPointerPoint;

    IFC(SliderGenerated::OnPointerEntered(pArgs));

    IFC(pArgs->GetCurrentPoint(NULL, &spPointerPoint));
    IFCPTR(spPointerPoint);
    IFC(spPointerPoint->get_PointerDeviceType(&nPointerDeviceType));

    //We don't want to go into the PointerOver state for touch input
    m_IsPointerOver = nPointerDeviceType != mui::PointerDeviceType_Touch;

    IFC(get_Thumb(&spThumb));
    if (spThumb)
    {
        IFC(spThumb->get_IsDragging(&isDragging));
        if (!isDragging)
        {
            IFC(UpdateVisualState());
        }
    }

    // Normally, PointerEntered/PointerExited for touch input toggles the visibility of the Thumb ToolTip.
    // However, the Thumb ToolTip should stay visible as long as the Slider or its Thumb are pressed,
    // so we do nothing here if the Slider or its Thumb are pressed.
    IFC(get_FocusState(&focusState));
    if (!m_isPressed &&
        !isDragging &&
        xaml::FocusState_Keyboard != focusState)
    {
        if (nPointerDeviceType == mui::PointerDeviceType_Touch)
        {
            IFC(UpdateThumbToolTipVisibility(TRUE, AutomaticToolTipInputMode::Touch));
        }
    }

Cleanup:
    RRETURN(hr);
}

// PointerExited event handler.
IFACEMETHODIMP Slider::OnPointerExited(
    _In_ IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<Thumb> spThumb;
    BOOLEAN isDragging = FALSE;
    mui::PointerDeviceType nPointerDeviceType = mui::PointerDeviceType_Touch;
    xaml::FocusState focusState = xaml::FocusState_Unfocused;

    IFC(SliderGenerated::OnPointerExited(pArgs));
    m_IsPointerOver = FALSE;

    IFC(get_Thumb(&spThumb));
    if (spThumb)
    {
        IFC(spThumb->get_IsDragging(&isDragging));
        if (!isDragging)
        {
            IFC(UpdateVisualState());
        }
    }

    // Normally, PointerEntered/PointerExited for touch input toggles the visibility of the Thumb ToolTip.
    // However, the Thumb ToolTip should stay visible as long as the Slider or its Thumb are pressed,
    // so we do nothing here if the Slider or its Thumb are pressed.
    IFC(get_FocusState(&focusState));
    if (!m_isPressed &&
        !isDragging &&
        xaml::FocusState_Keyboard != focusState)
    {
        ctl::ComPtr<ixp::IPointerPoint> spPointerPoint;

        IFC(pArgs->GetCurrentPoint(NULL, &spPointerPoint));
        IFCPTR(spPointerPoint);
        IFC(spPointerPoint->get_PointerDeviceType(&nPointerDeviceType));

        if (nPointerDeviceType == mui::PointerDeviceType_Touch)
        {
            IFC(UpdateThumbToolTipVisibility(FALSE));
        }
    }

Cleanup:
    RRETURN(hr);
}

// PointerPressed event handler.
_Check_return_ HRESULT
Slider::OnPointerPressed(
    _In_ IInspectable* pSender,
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IUIElement> spSender;
    ctl::ComPtr<ixp::IPointerPoint> spPointerPoint;
    ctl::ComPtr<ixp::IPointerPointProperties> spPointerProperties;
    BOOLEAN bIsLeftButtonPressed = FALSE;

    IFC(ctl::ComPtr<IInspectable>(pSender).As(&spSender));

    IFC(pArgs->GetCurrentPoint(NULL, &spPointerPoint));
    IFCPTR(spPointerPoint);

    IFC(spPointerPoint->get_Properties(&spPointerProperties));
    IFCPTR(spPointerProperties);

    IFC(spPointerProperties->get_IsLeftButtonPressed(&bIsLeftButtonPressed));

    if (bIsLeftButtonPressed)
    {
        BOOLEAN bIgnored = FALSE;
        wf::Point position = {};
        ctl::ComPtr<xaml_input::IPointer> spPointer;
        mui::PointerDeviceType nPointerDeviceType = mui::PointerDeviceType_Touch;

        IFC(spPointerPoint->get_PointerDeviceType(&nPointerDeviceType));

        IFC(pArgs->put_Handled(TRUE));

        AutomaticToolTipInputMode mode = AutomaticToolTipInputMode::None;
        if (nPointerDeviceType == mui::PointerDeviceType_Touch)
        {
            mode = AutomaticToolTipInputMode::Touch;
        }
        else
        {
            mode = AutomaticToolTipInputMode::Mouse;
        }
        IFC(UpdateThumbToolTipVisibility(TRUE, mode));

        IFC(Focus(xaml::FocusState_Pointer, &bIgnored));
        IFC(pArgs->get_Pointer(&spPointer));

        if (m_capturedPointerId == 0)
        {
            BOOLEAN wasCaptured = FALSE;

            IFC(spSender->CapturePointer(spPointer.Get(), &wasCaptured));
            if (wasCaptured)
            {
                IFC(spPointer->get_PointerId(&m_capturedPointerId));
            }
        }

        IFC(spPointerPoint->get_Position(&position));
        IFC(MoveThumbToPoint(position));
        IFC(SetIsPressed(TRUE));
    }

Cleanup:
    RRETURN(hr);
}

// PointerMoved event handler.
_Check_return_ HRESULT
Slider::OnPointerMoved(
    _In_ IInspectable* pSender,
    _In_ IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<Thumb> spThumb;
    wf::Point point = {};

    IFC(get_Thumb(&spThumb));
    if (spThumb)
    {
        BOOLEAN isDragging = FALSE;

        IFC(spThumb->get_IsDragging(&isDragging));

        if (m_isPressed && !isDragging)
        {
            ctl::ComPtr<ixp::IPointerPoint> spPointerPoint;

            IFC(pArgs->put_Handled(TRUE));

            IFC(pArgs->GetCurrentPoint(NULL, &spPointerPoint));
            IFCPTR(spPointerPoint);

            IFC(spPointerPoint->get_Position(&point));
            IFC(MoveThumbToPoint(point));
        }
    }

Cleanup:
    RRETURN(hr);
}

// PointerReleased event handler.
_Check_return_ HRESULT
Slider::OnPointerReleased(
    _In_ IInspectable* pSender,
    _In_ IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    GestureModes gestureFollowing = GestureModes::None;

    IFC(SetIsPressed(FALSE)); // Will also update visual state
    IFC(static_cast<PointerRoutedEventArgs*>(pArgs)->get_GestureFollowing(&gestureFollowing));
    if (gestureFollowing == GestureModes::RightTapped)
    {
        // We will get a right tapped event for every time we visit here, and
        // we will visit before each time we receive a right tapped event
        goto Cleanup;
    }

    // Note that we are intentionally NOT handling the args
    // if we do not fall through here because basically we are no_opting in that case.
    IFC(pArgs->put_Handled(TRUE));
    IFC(PerformPointerUpAction());

    if (m_capturedPointerId != 0)
    {
        ctl::ComPtr<xaml_input::IPointer> spPointer;
        UINT pointerID = 0;

        IFC(pArgs->get_Pointer(&spPointer));
        IFCPTR(spPointer);
        IFC(spPointer->get_PointerId(&pointerID));
        if (pointerID == m_capturedPointerId)
        {
            ctl::ComPtr<IUIElement> spSender;

            IFC(ctl::ComPtr<IInspectable>(pSender).As(&spSender));
            IFC(spSender->ReleasePointerCapture(spPointer.Get()));
            m_capturedPointerId = 0;
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT Slider::OnRightTappedUnhandled(
    _In_ xaml_input::IRightTappedRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN isHandled = FALSE;

    IFC(SliderGenerated::OnRightTappedUnhandled(pArgs));
    IFC(pArgs->get_Handled(&isHandled));
    if (isHandled)
    {
        goto Cleanup;
    }
    IFC(PerformPointerUpAction());

Cleanup:
    RRETURN(hr);
}


// Perform the primary action related to pointer up.
_Check_return_ HRESULT Slider::PerformPointerUpAction()
{
    HRESULT hr = S_OK;
    DOUBLE value = 0;

    // When we handle LMB down, Value is updated to the closest step or tick mark.
    // Therefore, it suffices here to fetch Value and put this into IntermediateValue.
    IFC(get_Value(&value));
    IFC(put_IntermediateValue(value));

    IFC(UpdateThumbToolTipVisibility(FALSE));

Cleanup:
    RRETURN(hr);
}


// PointerCaptureLost event handler.
_Check_return_ HRESULT
Slider::OnPointerCaptureLost(
    _In_ IInspectable* pSender,
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    IFC_RETURN(__super::OnPointerCaptureLost(pArgs));

    const bool hasStateChanged = m_isPressed || m_IsPointerOver;

    m_isPressed = FALSE;

    // when pointer move out of the control but OnPointerExit() would not be invoked so the control might remain in PointerOver visual state.
    // change it to false here
    m_IsPointerOver = FALSE;

    if (hasStateChanged)
    {
        IFC_RETURN(UpdateVisualState(true));
    }

    m_capturedPointerId = 0;

    return S_OK;
}

// Create SliderAutomationPeer to represent the Slider.
IFACEMETHODIMP Slider::OnCreateAutomationPeer(_Outptr_ xaml_automation_peers::IAutomationPeer** ppAutomationPeer)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_automation_peers::ISliderAutomationPeer> spSliderAutomationPeer;
    ctl::ComPtr<xaml_automation_peers::ISliderAutomationPeerFactory> spSliderAPFactory;
    ctl::ComPtr<IActivationFactory> spActivationFactory;
    ctl::ComPtr<IInspectable> spInner;

    spActivationFactory.Attach(ctl::ActivationFactoryCreator<DirectUI::SliderAutomationPeerFactory>::CreateActivationFactory());
    IFC(spActivationFactory.As(&spSliderAPFactory));

    IFC(spSliderAPFactory.Cast<SliderAutomationPeerFactory>()->CreateInstanceWithOwner(this,
        NULL,
        &spInner,
        &spSliderAutomationPeer));
    IFC(spSliderAutomationPeer.CopyTo(ppAutomationPeer));

Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Returns a plain text string to provide a default AutomationProperties.Name
//      in the absence of an explicitly defined one
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT Slider::GetPlainText(_Out_ HSTRING* strPlainText)
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

// Change to the correct visual state for the button.
_Check_return_ HRESULT Slider::ChangeVisualState(
    // true to use transitions when updating the visual state, false
    // to snap directly to the new visual state.
    _In_ bool useTransitions)
{
    BOOLEAN isEnabled = FALSE;
    BOOLEAN isFocusEngaged = FALSE;
    BOOLEAN ignored = FALSE;
    xaml::FocusState focusState = xaml::FocusState_Unfocused;
    xaml_controls::Orientation orientation = xaml_controls::Orientation_Vertical;

    IFC_RETURN(get_IsEnabled(&isEnabled));
    IFC_RETURN(get_IsFocusEngaged(&isFocusEngaged));
    IFC_RETURN(get_FocusState(&focusState));
    IFC_RETURN(get_Orientation(&orientation));

    if (!isEnabled)
    {
        // TODO: VisualStates.GoToState(this, useTransitions, VisualStates.StateDisabled, VisualStates.StateNormal);
        IFC_RETURN(GoToState(useTransitions, L"Disabled", &ignored));
    }
    else if (m_isPressed)
    {
        // TODO: VisualStates.GoToState(this, useTransitions, VisualStates.StatePressed, VisualStates.StateNormal);
        IFC_RETURN(GoToState(useTransitions, L"Pressed", &ignored));
    }
    else if (m_IsPointerOver)
    {
        // TODO: VisualStates.GoToState(this, useTransitions, VisualStates.StatePointerOver, VisualStates.StateNormal);
        IFC_RETURN(GoToState(useTransitions, L"PointerOver", &ignored));
    }
    else
    {
        IFC_RETURN(GoToState(useTransitions, L"Normal", &ignored));
    }

    if (xaml::FocusState_Keyboard == focusState && isEnabled)
    {
        // TODO: VisualStates.GoToState(this, useTransitions, VisualStates.StateFocused, VisualStates.StateUnfocused);
        IFC_RETURN(GoToState(useTransitions, L"Focused", &ignored));
    }
    else
    {
        IFC_RETURN(GoToState(useTransitions, L"Unfocused", &ignored));
    }

    if (isFocusEngaged)
    {
        if (orientation == xaml_controls::Orientation_Horizontal)
        {
            IFC_RETURN(GoToState(useTransitions, L"FocusEngagedHorizontal", &ignored));
        }
        else
        {
            ASSERT(orientation == xaml_controls::Orientation_Vertical);
            IFC_RETURN(GoToState(useTransitions, L"FocusEngagedVertical", &ignored));
        }
    }
    else
    {
        IFC_RETURN(GoToState(useTransitions, L"FocusDisengaged", &ignored));
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

// Apply a template to the Slider.
IFACEMETHODIMP Slider::OnApplyTemplate()
{
    HRESULT hr = S_OK;

    ctl::ComPtr<xaml::IDependencyObject> spElementHorizontalTemplateAsDO;
    ctl::ComPtr<xaml::IDependencyObject> spElementTopTickBarAsDO;
    ctl::ComPtr<xaml::IDependencyObject> spElementHorizontalInlineTickBarAsDO;
    ctl::ComPtr<xaml::IDependencyObject> spElementBottomTickBarAsDO;
    ctl::ComPtr<xaml::IDependencyObject> spElementLeftTickBarAsDO;
    ctl::ComPtr<xaml::IDependencyObject> spElementVerticalInlineTickBarAsDO;
    ctl::ComPtr<xaml::IDependencyObject> spElementRightTickBarAsDO;
    ctl::ComPtr<xaml::IDependencyObject> spElementHorizontalDecreaseRectAsDO;
    ctl::ComPtr<xaml::IDependencyObject> spElementHorizontalThumbAsDO;
    ctl::ComPtr<xaml::IDependencyObject> spElementVerticalTemplateAsDO;
    ctl::ComPtr<xaml::IDependencyObject> spElementVerticalDecreaseRectAsDO;
    ctl::ComPtr<xaml::IDependencyObject> spElementVerticalThumbAsDO;
    ctl::ComPtr<xaml::IDependencyObject> spSliderContainerAsDO;
    ctl::ComPtr<xaml_primitives::IDragStartedEventHandler> spDragStartedHandler;
    ctl::ComPtr<xaml_primitives::IDragDeltaEventHandler> spDragDeltaHandler;
    ctl::ComPtr<xaml_primitives::IDragCompletedEventHandler> spDragCompletedHandler;
    ctl::ComPtr<xaml::ISizeChangedEventHandler> spThumbSizeChangedHandler;
    ctl::ComPtr<IToolTip> spHorizontalThumbToolTip;
    ctl::ComPtr<IToolTip> spVerticalThumbToolTip;
    ctl::ComPtr<xaml::IDependencyObject> spFocusVisualWhiteHorizontalDO;
    ctl::ComPtr<xaml::IDependencyObject> spFocusVisualBlackHorizontalDO;
    ctl::ComPtr<xaml::IDependencyObject> spFocusVisualWhiteVerticalDO;
    ctl::ComPtr<xaml::IDependencyObject> spFocusVisualBlackVerticalDO;

    BOOLEAN bIsThumbToolTipEnabled = FALSE;

    // Cleanup any existing template parts
    if (m_tpElementHorizontalThumb)
    {
        IFC(m_tpElementHorizontalThumb->remove_DragStarted(m_ElementHorizontalThumbDragStartedToken));
        IFC(m_tpElementHorizontalThumb->remove_DragDelta(m_ElementHorizontalThumbDragDeltaToken));
        IFC(m_tpElementHorizontalThumb->remove_DragCompleted(m_ElementHorizontalThumbDragCompletedToken));
        IFC(m_tpElementHorizontalThumb.Cast<Thumb>()->remove_SizeChanged(m_ElementHorizontalThumbSizeChangedToken));
    }
    if (m_tpElementVerticalThumb)
    {
        IFC(m_tpElementVerticalThumb->remove_DragStarted(m_ElementVerticalThumbDragStartedToken));
        IFC(m_tpElementVerticalThumb->remove_DragDelta(m_ElementVerticalThumbDragDeltaToken));
        IFC(m_tpElementVerticalThumb->remove_DragCompleted(m_ElementVerticalThumbDragCompletedToken));
        IFC(m_tpElementVerticalThumb.Cast<Thumb>()->remove_SizeChanged(m_ElementVerticalThumbSizeChangedToken));
    }

    {
        ctl::ComPtr<IFrameworkElement> spSliderContainer = m_tpSliderContainer ? m_tpSliderContainer.Cast<FrameworkElement>() : this;

        IFC(m_epSliderContainerPointerPressedHandler.DetachEventHandler(spSliderContainer.Get()));
        IFC(m_epSliderContainerPointerReleasedHandler.DetachEventHandler(spSliderContainer.Get()));
        IFC(m_epSliderContainerPointerMovedHandler.DetachEventHandler(spSliderContainer.Get()));
        IFC(m_epSliderContainerPointerCaptureLostHandler.DetachEventHandler(spSliderContainer.Get()));
        IFC(m_epSliderContainerSizeChangedHandler.DetachEventHandler(spSliderContainer.Get()));
    }

    m_tpHeaderPresenter.Clear();
    m_tpElementHorizontalTemplate.Clear();
    m_tpElementTopTickBar.Clear();
    m_tpElementHorizontalInlineTickBar.Clear();
    m_tpElementBottomTickBar.Clear();
    m_tpElementLeftTickBar.Clear();
    m_tpElementVerticalInlineTickBar.Clear();
    m_tpElementRightTickBar.Clear();
    m_tpElementHorizontalDecreaseRect.Clear();
    m_tpElementHorizontalThumb.Clear();
    m_tpElementVerticalTemplate.Clear();
    m_tpElementVerticalDecreaseRect.Clear();
    m_tpElementVerticalThumb.Clear();
    m_tpSliderContainer.Clear();

    m_bUsingDefaultToolTipForHorizontalThumb = FALSE;
    m_bUsingDefaultToolTipForVerticalThumb = FALSE;

    IFC(SliderGenerated::OnApplyTemplate());

    // Get the parts
    IFC(GetTemplateChild(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"HorizontalTemplate")).Get(), &spElementHorizontalTemplateAsDO));
    SetPtrValueWithQIOrNull(m_tpElementHorizontalTemplate, spElementHorizontalTemplateAsDO.Get());
    IFC(GetTemplateChild(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"TopTickBar")).Get(), &spElementTopTickBarAsDO));
    SetPtrValueWithQIOrNull(m_tpElementTopTickBar, spElementTopTickBarAsDO.Get());
    IFC(GetTemplateChild(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"HorizontalInlineTickBar")).Get(), &spElementHorizontalInlineTickBarAsDO));
    SetPtrValueWithQIOrNull(m_tpElementHorizontalInlineTickBar, spElementHorizontalInlineTickBarAsDO.Get());
    IFC(GetTemplateChild(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"BottomTickBar")).Get(), &spElementBottomTickBarAsDO));
    SetPtrValueWithQIOrNull(m_tpElementBottomTickBar, spElementBottomTickBarAsDO.Get());
    IFC(GetTemplateChild(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"LeftTickBar")).Get(), &spElementLeftTickBarAsDO));
    SetPtrValueWithQIOrNull(m_tpElementLeftTickBar, spElementLeftTickBarAsDO.Get());
    IFC(GetTemplateChild(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"VerticalInlineTickBar")).Get(), &spElementVerticalInlineTickBarAsDO));
    SetPtrValueWithQIOrNull(m_tpElementVerticalInlineTickBar, spElementVerticalInlineTickBarAsDO.Get());
    IFC(GetTemplateChild(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"RightTickBar")).Get(), &spElementRightTickBarAsDO));
    SetPtrValueWithQIOrNull(m_tpElementRightTickBar, spElementRightTickBarAsDO.Get());
    IFC(GetTemplateChild(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"HorizontalDecreaseRect")).Get(), &spElementHorizontalDecreaseRectAsDO));
    SetPtrValueWithQIOrNull(m_tpElementHorizontalDecreaseRect, spElementHorizontalDecreaseRectAsDO.Get());
    IFC(GetTemplateChild(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"HorizontalThumb")).Get(), &spElementHorizontalThumbAsDO));
    SetPtrValueWithQIOrNull(m_tpElementHorizontalThumb, spElementHorizontalThumbAsDO.Get());
    IFC(GetTemplateChild(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"VerticalTemplate")).Get(), &spElementVerticalTemplateAsDO));
    SetPtrValueWithQIOrNull(m_tpElementVerticalTemplate, spElementVerticalTemplateAsDO.Get());
    IFC(GetTemplateChild(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"VerticalDecreaseRect")).Get(), &spElementVerticalDecreaseRectAsDO));
    SetPtrValueWithQIOrNull(m_tpElementVerticalDecreaseRect, spElementVerticalDecreaseRectAsDO.Get());
    IFC(GetTemplateChild(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"VerticalThumb")).Get(), &spElementVerticalThumbAsDO));
    SetPtrValueWithQIOrNull(m_tpElementVerticalThumb, spElementVerticalThumbAsDO.Get());
    IFC(GetTemplateChild(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"SliderContainer")).Get(), &spSliderContainerAsDO));
    SetPtrValueWithQIOrNull(m_tpSliderContainer, spSliderContainerAsDO.Get());

    // Attach the event handlers
    if (m_tpElementHorizontalThumb || m_tpElementVerticalThumb)
    {
        spDragStartedHandler.Attach(
            new ClassMemberEventHandler<
            Slider,
            ISlider,
            xaml_primitives::IDragStartedEventHandler,
            IInspectable,
            xaml_primitives::IDragStartedEventArgs>(this, &Slider::OnThumbDragStarted));

        spDragDeltaHandler.Attach(
            new ClassMemberEventHandler<
            Slider,
            ISlider,
            xaml_primitives::IDragDeltaEventHandler,
            IInspectable,
            xaml_primitives::IDragDeltaEventArgs>(this, &Slider::OnThumbDragDelta));

        spDragCompletedHandler.Attach(
            new ClassMemberEventHandler<
            Slider,
            ISlider,
            xaml_primitives::IDragCompletedEventHandler,
            IInspectable,
            xaml_primitives::IDragCompletedEventArgs>(this, &Slider::OnThumbDragCompleted));

        spThumbSizeChangedHandler.Attach(
            new ClassMemberEventHandler<
            Slider,
            ISlider,
            ISizeChangedEventHandler,
            IInspectable,
            ISizeChangedEventArgs>(this, &Slider::OnThumbSizeChanged));

        IFC(get_IsThumbToolTipEnabled(&bIsThumbToolTipEnabled));

        if (m_tpElementHorizontalThumb)
        {
            IFC(m_tpElementHorizontalThumb->add_DragStarted(spDragStartedHandler.Get(), &m_ElementHorizontalThumbDragStartedToken));
            IFC(m_tpElementHorizontalThumb->add_DragDelta(spDragDeltaHandler.Get(), &m_ElementHorizontalThumbDragDeltaToken));
            IFC(m_tpElementHorizontalThumb->add_DragCompleted(spDragCompletedHandler.Get(), &m_ElementHorizontalThumbDragCompletedToken));
            IFC(m_tpElementHorizontalThumb.Cast<Thumb>()->add_SizeChanged(spThumbSizeChangedHandler.Get(), &m_ElementHorizontalThumbSizeChangedToken));

            IFC(ToolTipServiceFactory::GetToolTipObjectStatic(m_tpElementHorizontalThumb.Cast<Thumb>(), &spHorizontalThumbToolTip));
            // If no disambiguation UI ToolTip exist for the Thumb, we create one.
            // TODO: Jupiter 101372 - Cannot use RelativeSource={RelativeSource TemplatedParent} in generic.xaml
            // When the bug is fixed, stop creating a ToolTip in code and move this to XAML.
            if (!spHorizontalThumbToolTip)
            {
                IFC(SetDefaultThumbToolTip(xaml_controls::Orientation_Horizontal));
                m_bUsingDefaultToolTipForHorizontalThumb = TRUE;
            }
            else
            {
                IFC(spHorizontalThumbToolTip.Cast<ToolTip>()->put_IsEnabled(bIsThumbToolTipEnabled));
                spHorizontalThumbToolTip.Cast<ToolTip>()->m_isSliderThumbToolTip = TRUE;
            }
        }

        if (m_tpElementVerticalThumb)
        {
            IFC(m_tpElementVerticalThumb->add_DragStarted(spDragStartedHandler.Get(), &m_ElementVerticalThumbDragStartedToken));
            IFC(m_tpElementVerticalThumb->add_DragDelta(spDragDeltaHandler.Get(), &m_ElementVerticalThumbDragDeltaToken));
            IFC(m_tpElementVerticalThumb->add_DragCompleted(spDragCompletedHandler.Get(), &m_ElementVerticalThumbDragCompletedToken));
            IFC(m_tpElementVerticalThumb.Cast<Thumb>()->add_SizeChanged(spThumbSizeChangedHandler.Get(), &m_ElementVerticalThumbSizeChangedToken));

            IFC(ToolTipServiceFactory::GetToolTipObjectStatic(m_tpElementVerticalThumb.Cast<Thumb>(), &spVerticalThumbToolTip));
            // If no disambiguation UI ToolTip exist for the Thumb, we create one.
            // TODO: Jupiter 101372 - Cannot use RelativeSource={RelativeSource TemplatedParent} in generic.xaml
            // When the bug is fixed, stop creating a ToolTip in code and move this to XAML.
            if (!spVerticalThumbToolTip)
            {
                IFC(SetDefaultThumbToolTip(xaml_controls::Orientation_Vertical));
                m_bUsingDefaultToolTipForVerticalThumb = TRUE;
            }
            else
            {
                IFC(spVerticalThumbToolTip.Cast<ToolTip>()->put_IsEnabled(bIsThumbToolTipEnabled));
                spVerticalThumbToolTip.Cast<ToolTip>()->m_isSliderThumbToolTip = TRUE;
            }
        }
    }

    // Attach to pointer events
    {
        ctl::ComPtr<FrameworkElement> spSliderContainer = m_tpSliderContainer ? m_tpSliderContainer.Cast<FrameworkElement>() : this;

        IFC(m_epSliderContainerPointerPressedHandler.AttachEventHandler(
            spSliderContainer.Get(),
            std::bind(&Slider::OnPointerPressed, this, _1, _2)));

        IFC(m_epSliderContainerPointerReleasedHandler.AttachEventHandler(
            spSliderContainer.Get(),
            std::bind(&Slider::OnPointerReleased, this, _1, _2)));

        IFC(m_epSliderContainerPointerMovedHandler.AttachEventHandler(
            spSliderContainer.Get(),
            std::bind(&Slider::OnPointerMoved, this, _1, _2)));

        IFC(m_epSliderContainerPointerCaptureLostHandler.AttachEventHandler(
            spSliderContainer.Get(),
            std::bind(&Slider::OnPointerCaptureLost, this, _1, _2)));

        IFC(m_epSliderContainerSizeChangedHandler.AttachEventHandler(
            spSliderContainer.Get(),
            std::bind(&Slider::OnSizeChanged, this, _1, _2)));
    }

    // Updating states for parts where properties might have been updated
    // through XAML before the template was loaded.
    IFC(OnOrientationChanged());
    IFC(OnTickPlacementChanged());
    IFC(ChangeVisualState(FALSE));
    IFC(UpdateHeaderPresenterVisibility());

    // The base Control class has special handling for focus visuals that are renderered as alternate black and white
    // rectangles with stroke dash arrays, to produce the effect of alternating black and white stroke dashes.
    // However, this handling uses the default focus visual names for all controls. Slider has different names since
    // it requires unique elements for its horizontal and vertical templates. So it needs a separate version of the
    // code that rounds focus visual stroke thickness to plateau-scaled integer value.
    IFC(GetTemplateChild(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"FocusVisualWhiteHorizontal")).Get(), &spFocusVisualWhiteHorizontalDO));
    IFC(GetTemplateChild(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"FocusVisualBlackHorizontal")).Get(), &spFocusVisualBlackHorizontalDO));
    if (spFocusVisualWhiteHorizontalDO &&
        spFocusVisualBlackHorizontalDO &&
        ctl::is<IRectangle>(spFocusVisualWhiteHorizontalDO) &&
        ctl::is<IRectangle>(spFocusVisualBlackHorizontalDO))
    {
        IFC(LayoutRoundRectangleStrokeThickness(spFocusVisualWhiteHorizontalDO.Cast<Rectangle>()));
        IFC(LayoutRoundRectangleStrokeThickness(spFocusVisualBlackHorizontalDO.Cast<Rectangle>()));
    }

    IFC(GetTemplateChild(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"FocusVisualWhiteVertical")).Get(), &spFocusVisualWhiteVerticalDO));
    IFC(GetTemplateChild(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"FocusVisualBlackVertical")).Get(), &spFocusVisualBlackVerticalDO));
    if (spFocusVisualWhiteVerticalDO &&
        spFocusVisualBlackVerticalDO &&
        ctl::is<IRectangle>(spFocusVisualWhiteVerticalDO) &&
        ctl::is<IRectangle>(spFocusVisualBlackVerticalDO))
    {
        IFC(LayoutRoundRectangleStrokeThickness(spFocusVisualWhiteVerticalDO.Cast<Rectangle>()));
        IFC(LayoutRoundRectangleStrokeThickness(spFocusVisualBlackVerticalDO.Cast<Rectangle>()));
    }

Cleanup:
    RRETURN(hr);
}

// Called when the Value value changed.
IFACEMETHODIMP Slider::OnValueChanged(
    _In_ DOUBLE oldValue,
    _In_ DOUBLE newValue)
{
    HRESULT hr = S_OK;

    IFC(SliderGenerated::OnValueChanged(oldValue, newValue));
    if (!m_bProcessingInputEvent)
    {
        IFC(put_IntermediateValue(newValue));
    }

Cleanup:
    RRETURN(hr);
}

// Called when the Minimum value changed.
IFACEMETHODIMP Slider::OnMinimumChanged(
    _In_ DOUBLE oldMinimum,
    _In_ DOUBLE newMinimum)
{
    HRESULT hr = S_OK;

    IFC(SliderGenerated::OnMinimumChanged(oldMinimum, newMinimum));
    IFC(UpdateTrackLayout());

Cleanup:
    RRETURN(hr);
}

// Called when the Maximum value changed.
IFACEMETHODIMP Slider::OnMaximumChanged(
    _In_ DOUBLE oldMaximum,
    _In_ DOUBLE newMaximum)
{
    HRESULT hr = S_OK;

    IFC(SliderGenerated::OnMaximumChanged(oldMaximum, newMaximum));
    IFC(UpdateTrackLayout());

Cleanup:
    RRETURN(hr);
}

// Called whenever the Thumb drag operation is started.
_Check_return_ HRESULT Slider::OnThumbDragStarted(
    _In_ IInspectable* pSender,
    _In_ xaml_primitives::IDragStartedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN focused = FALSE;

    IFC(Focus(xaml::FocusState_Pointer, &focused));
    IFC(UpdateVisualState(TRUE));
    IFC(get_Value(&m_DragValue));

Cleanup:
    RRETURN(hr);
}

// Whenever the thumb gets dragged, we handle the event through this function to
// update the current value depending upon the thumb drag delta.
_Check_return_ HRESULT Slider::OnThumbDragDelta(
    _In_ IInspectable* pSender,
    _In_ xaml_primitives::IDragDeltaEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<Grid> spRootGrid;
    DOUBLE nominator = 0.0;
    DOUBLE denominator = 0.0;
    DOUBLE offset = 0.0;
    DOUBLE zoom = 1.0;
    DOUBLE maximum = 0.0;
    DOUBLE minimum = 0.0;
    DOUBLE actualSize = 0.0;
    DOUBLE thumbSize = 0.0;
    DOUBLE change = 0.0;
    xaml_controls::Orientation orientation = xaml_controls::Orientation_Vertical;

    m_bProcessingInputEvent = TRUE;

    IFC(GetRootGrid(&spRootGrid));
    if (spRootGrid)
    {
        IFC(get_Maximum(&maximum));
        IFC(get_Minimum(&minimum));

        IFC(get_Orientation(&orientation));
        if (orientation == xaml_controls::Orientation_Horizontal &&
            m_tpElementHorizontalThumb)
        {
            IFC(pArgs->get_HorizontalChange(&change));
            IFC(spRootGrid->get_ActualWidth(&actualSize));
            IFC(m_tpElementHorizontalThumb.Cast<Thumb>()->get_ActualWidth(&thumbSize));

            nominator = (zoom * change) * (maximum - minimum);
            denominator = (actualSize - thumbSize);
            offset = nominator / DoubleUtil::Max(1, denominator);
        }
        else if (orientation == xaml_controls::Orientation_Vertical &&
            m_tpElementVerticalThumb)
        {
            IFC(pArgs->get_VerticalChange(&change));
            IFC(spRootGrid->get_ActualHeight(&actualSize));
            IFC(m_tpElementVerticalThumb.Cast<Thumb>()->get_ActualHeight(&thumbSize));

            nominator = (zoom * -change) * (maximum - minimum);
            denominator = (actualSize - thumbSize);
            offset = nominator / DoubleUtil::Max(1, denominator);
        }

        if (!DoubleUtil::IsNaN(offset) &&
            !DoubleUtil::IsInfinity(offset))
        {
            BOOLEAN reversed = FALSE;
            DOUBLE newValue = 0;
            xaml_primitives::SliderSnapsTo snapsTo = xaml_primitives::SliderSnapsTo_StepValues;
            DOUBLE tickFrequency = 0;
            DOUBLE stepFrequency = 0;
            DOUBLE closestStep = 0;
            DOUBLE value = 0;

            IFC(get_IsDirectionReversed(&reversed));
            m_DragValue += reversed ? -offset : offset;

            newValue = DoubleUtil::Min(maximum, DoubleUtil::Max(minimum, m_DragValue));
            IFC(put_IntermediateValue(newValue));

            // Set value to the closest multiple of StepFrequency.
            IFC(get_Value(&value));
            IFC(get_SnapsTo(&snapsTo));
            if (snapsTo == xaml_primitives::SliderSnapsTo_Ticks)
            {
                IFC(get_TickFrequency(&tickFrequency));
                IFC(GetClosestStep(tickFrequency, newValue, &closestStep));
            }
            else
            {
                IFC(this->get_StepFrequency(&stepFrequency));
                IFC(GetClosestStep(stepFrequency, newValue, &closestStep));
            }

            if (!DoubleUtil::AreClose(value, closestStep))
            {
                IFC(put_Value(closestStep));
            }
        }
    }

Cleanup:
    m_bProcessingInputEvent = FALSE;
    RRETURN(hr);
}

// Whenever the thumb drag completes, we handle the event through this
// function to update IntermediateValue to snap to the closest step or tick mark.
_Check_return_
HRESULT
Slider::OnThumbDragCompleted(
    _In_ IInspectable* pSender,
    _In_ xaml_primitives::IDragCompletedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    DOUBLE value = 0;

    // While we drag, Value is always updated to the closest step or tick mark.
    // Therefore, it suffices here to fetch Value and put this into IntermediateValue.
    IFC(get_Value(&value));
    IFC(put_IntermediateValue(value));

Cleanup:
    RRETURN(hr);
}

// Whenever the thumb size changes, we need to recalculate track layout.
_Check_return_
HRESULT
Slider::OnThumbSizeChanged(
    _In_ IInspectable* pSender,
    _In_ xaml::ISizeChangedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    IFC(UpdateTrackLayout());

Cleanup:
    RRETURN(hr);
}

// Handle the SizeChanged event.
_Check_return_ HRESULT Slider::OnSizeChanged(
    _In_ IInspectable* pSender,
    _In_ xaml::ISizeChangedEventArgs* pArgs)
{
    RRETURN(UpdateTrackLayout());
}

// Change the template being used to display this control when the orientation
// changes.
_Check_return_ HRESULT Slider::OnOrientationChanged()
{
    HRESULT hr = S_OK;
    xaml_controls::Orientation orientation = xaml_controls::Orientation_Vertical;

    IFC(get_Orientation(&orientation));

    if (m_tpElementHorizontalTemplate)
    {
        IFC(m_tpElementHorizontalTemplate.Cast<Thumb>()->put_Visibility(
            orientation == xaml_controls::Orientation_Horizontal ?
            xaml::Visibility_Visible :
            xaml::Visibility_Collapsed));
    }

    if (m_tpElementVerticalTemplate)
    {
        IFC(m_tpElementVerticalTemplate.Cast<Thumb>()->put_Visibility(
            orientation == xaml_controls::Orientation_Horizontal ?
            xaml::Visibility_Collapsed :
            xaml::Visibility_Visible));
    }

    IFC(UpdateTrackLayout());

Cleanup:
    RRETURN(hr);
}

// Updates the track layout using ActualWidth and ActualHeight of the control.
_Check_return_ HRESULT Slider::UpdateTrackLayout()
{
    HRESULT hr = S_OK;
    DOUBLE actualWidth = 0.0;
    DOUBLE actualHeight = 0.0;

    // New behavior for blue: only the slider's root grid is interactive.
    if (m_tpSliderContainer)
    {
        IFC(m_tpSliderContainer->get_ActualWidth(&actualWidth));
        IFC(m_tpSliderContainer->get_ActualHeight(&actualHeight));
    }
    // Old behavior for win8: the entire control is interactive.
    else
    {
        IFC(get_ActualWidth(&actualWidth));
        IFC(get_ActualHeight(&actualHeight));
    }

    // If we are layout rounding ensure that width and height are rounded
    actualWidth = LayoutRoundedDimension(actualWidth);
    actualHeight = LayoutRoundedDimension(actualHeight);

    IFC(UpdateTrackLayout(actualWidth, actualHeight));

Cleanup:
    RRETURN(hr);
}

// This method will take the current min, max, and value to calculate and layout the current control measurements.
_Check_return_ HRESULT Slider::UpdateTrackLayout(
    _In_ DOUBLE actualWidth,
    _In_ DOUBLE actualHeight) noexcept
{
    ctl::ComPtr<Grid> spRoot;
    DOUBLE maximum = 0.0;
    DOUBLE minimum = 0.0;
    DOUBLE currentValue = 0.0;
    DOUBLE multiplier = 1.0;
    xaml_controls::Orientation orientation = xaml_controls::Orientation_Vertical;
    BOOLEAN reversed;
    UINT count = 0;
    xaml::GridLength starGridLength = {};
    xaml::GridLength autoGridLength = {};
    DOUBLE thumbWidth = 0.0;
    DOUBLE thumbHeight = 0.0;
    DOUBLE elementNewLength = 0;
    DOUBLE elementOldLength = 0;
    BOOLEAN isThumbToolTipEnabled = FALSE;
    xaml::Thickness padding = {};

    // Extract padding from the actual size.
    if (!m_tpSliderContainer)
    {
        IFC_RETURN(get_Padding(&padding));
    }

    actualWidth = MAX(actualWidth - padding.Left - padding.Right, 0.0);
    actualHeight = MAX(actualHeight - padding.Top - padding.Bottom, 0.0);

    IFC_RETURN(get_IsThumbToolTipEnabled(&isThumbToolTipEnabled));

    IFC_RETURN(get_Maximum(&maximum));
    IFC_RETURN(get_Minimum(&minimum));
    IFC_RETURN(get_IntermediateValue(&currentValue));
    IFC_RETURN(get_Orientation(&orientation));
    IFC_RETURN(get_IsDirectionReversed(&reversed));
    DOUBLE range = maximum - minimum;
    multiplier = (range <= 0) ? 0 : 1 - (maximum - currentValue) / range;

    IFC_RETURN(GetRootGrid(&spRoot));
    if (spRoot)
    {
        ctl::ComPtr<IInspectable> spDecreaseValue;
        ctl::ComPtr<IToolTip> spToolTip;
        ctl::ComPtr<xaml_media::IGeneralTransform> spTransformToRoot;

        starGridLength.GridUnitType = xaml::GridUnitType_Star;
        starGridLength.Value = 1.0;

        autoGridLength.GridUnitType = xaml::GridUnitType_Auto;
        autoGridLength.Value = 1.0;

        if (orientation == xaml_controls::Orientation_Horizontal)
        {
            ctl::ComPtr<wfc::IVector<xaml_controls::ColumnDefinition*>> spColumns;

            if (m_tpElementHorizontalThumb)
            {
                IFC_RETURN(m_tpElementHorizontalThumb.Cast<Thumb>()->get_ActualWidth(&thumbWidth));
                IFC_RETURN(m_tpElementHorizontalThumb.Cast<Thumb>()->get_ActualHeight(&thumbHeight));
            }

            IFC_RETURN(spRoot->get_ColumnDefinitions(&spColumns));
            if (spColumns)
            {
                IFC_RETURN(spColumns->get_Size(&count));
                if (count == 3 || count == 4) // we shouldn't have this check. Let's just swap first with the last.
                {
                    ctl::ComPtr<xaml_controls::IColumnDefinition> spFirstColumn;
                    ctl::ComPtr<xaml_controls::IColumnDefinition> spLastColumn;

                    IFC_RETURN(spColumns->GetAt(0, &spFirstColumn));
                    IFC_RETURN(spColumns->GetAt(count - 1, &spLastColumn));
                    IFC_RETURN(spFirstColumn->put_Width(reversed ? starGridLength : autoGridLength));
                    IFC_RETURN(spLastColumn->put_Width(reversed ? autoGridLength : starGridLength));

                    if (m_tpElementHorizontalDecreaseRect)
                    {
                        IFC_RETURN(PropertyValue::CreateFromInt32(reversed ? count - 1 : 0, &spDecreaseValue));
                        IFC_RETURN(m_tpElementHorizontalDecreaseRect.Cast<Rectangle>()->SetValueByKnownIndex(KnownPropertyIndex::Grid_Column, spDecreaseValue.Get()));
                    }

                    if (m_tpElementHorizontalDecreaseRect)
                    {
                        elementNewLength = DoubleUtil::Max(0.0, multiplier * (actualWidth - thumbWidth));
                        elementNewLength = LayoutRoundedDimension(elementNewLength);

                        IFC_RETURN(m_tpElementHorizontalDecreaseRect.Cast<Rectangle>()->get_Width(&elementOldLength));
                        // If length has not changed by atleast a rounding step, no reason to redo everything.
                        if (std::isnan(elementOldLength) || DoubleUtil::GreaterThanOrClose(std::abs(elementOldLength - elementNewLength), RoundingStep()))
                        {
                            IFC_RETURN(m_tpElementHorizontalDecreaseRect.Cast<Rectangle>()->put_Width(elementNewLength));
                        }
                        else
                        {
                            elementNewLength = elementOldLength;
                        }
                    }
                }
            }

            if (m_tpElementHorizontalThumb && isThumbToolTipEnabled)
            {
                wf::Point origin = { 0, 0 };
                wf::Point targetTopLeft = {};
                DOUBLE lengthDelta = elementNewLength - elementOldLength;
                xaml::FlowDirection targetFlowDirection = xaml::FlowDirection_LeftToRight;
                RECT rcDockTo = {};

                IFC_RETURN(m_tpElementHorizontalThumb.Cast<Thumb>()->TransformToVisual(nullptr, &spTransformToRoot));
                IFC_RETURN(spTransformToRoot->TransformPoint(origin, &targetTopLeft));

                IFC_RETURN(m_tpElementHorizontalThumb.Cast<Thumb>()->get_FlowDirection(&targetFlowDirection));

                if (xaml::FlowDirection_RightToLeft == targetFlowDirection)
                {
                    // RTL case
                    rcDockTo.left = static_cast<LONG>(targetTopLeft.X - lengthDelta - thumbWidth);
                    rcDockTo.right = static_cast<LONG>(targetTopLeft.X - lengthDelta);
                }
                else
                {
                    // Normal case
                    rcDockTo.left = static_cast<LONG>(targetTopLeft.X + lengthDelta);
                    rcDockTo.right = static_cast<LONG>(targetTopLeft.X + lengthDelta + thumbWidth);
                }
                rcDockTo.top = static_cast<LONG>(targetTopLeft.Y);
                rcDockTo.bottom = static_cast<LONG>(targetTopLeft.Y + thumbHeight);

                IFC_RETURN(ToolTipServiceFactory::GetToolTipObjectStatic(m_tpElementHorizontalThumb.Cast<Thumb>(), &spToolTip));
                IFCEXPECT_RETURN(spToolTip);
                IFC_RETURN(spToolTip.Cast<ToolTip>()->PerformPlacement(&rcDockTo));
            }
        }
        else
        {
            ctl::ComPtr<wfc::IVector<xaml_controls::RowDefinition*>> spRows;

            if (m_tpElementVerticalThumb)
            {
                IFC_RETURN(m_tpElementVerticalThumb.Cast<Thumb>()->get_ActualWidth(&thumbWidth));
                IFC_RETURN(m_tpElementVerticalThumb.Cast<Thumb>()->get_ActualHeight(&thumbHeight));
            }

            IFC_RETURN(spRoot->get_RowDefinitions(&spRows));
            if (spRows)
            {
                IFC_RETURN(spRows->get_Size(&count));
                if (count == 3)
                {
                    ctl::ComPtr<xaml_controls::IRowDefinition> spFirstRow;
                    ctl::ComPtr<xaml_controls::IRowDefinition> spLastRow;

                    IFC_RETURN(spRows->GetAt(0, &spFirstRow));
                    IFC_RETURN(spRows->GetAt(2, &spLastRow));
                    IFC_RETURN(spFirstRow->put_Height(reversed ? autoGridLength : starGridLength));
                    IFC_RETURN(spLastRow->put_Height(reversed ? starGridLength : autoGridLength));

                    if (m_tpElementVerticalDecreaseRect)
                    {
                        IFC_RETURN(PropertyValue::CreateFromInt32(reversed ? 0 : 2, &spDecreaseValue));
                        IFC_RETURN(m_tpElementVerticalDecreaseRect.Cast<Rectangle>()->SetValueByKnownIndex(KnownPropertyIndex::Grid_Row, spDecreaseValue.Get()));
                    }

                     if (m_tpElementVerticalDecreaseRect)
                    {
                        elementNewLength = DoubleUtil::Max(0.0, multiplier * (actualHeight - thumbHeight));
                        elementNewLength = LayoutRoundedDimension(elementNewLength);

                        IFC_RETURN(m_tpElementVerticalDecreaseRect.Cast<Rectangle>()->get_Height(&elementOldLength));
                        // If length has not changed by atleast a rounding step, no reason to redo everything.
                        if (std::isnan(elementOldLength) || DoubleUtil::GreaterThanOrClose(std::abs(elementOldLength - elementNewLength), RoundingStep()))
                        {
                            IFC_RETURN(m_tpElementVerticalDecreaseRect.Cast<Rectangle>()->put_Height(elementNewLength));
                        }
                        else
                        {
                            elementNewLength = elementOldLength;
                        }
                    }
                }
            }

            if (m_tpElementVerticalThumb && isThumbToolTipEnabled)
            {
                wf::Point origin = { 0, 0 };
                wf::Point targetTopLeft = {};
                DOUBLE lengthDelta = elementNewLength - elementOldLength;
                xaml::FlowDirection targetFlowDirection = xaml::FlowDirection_LeftToRight;
                RECT rcDockTo = {};

                IFC_RETURN(m_tpElementVerticalThumb.Cast<Thumb>()->TransformToVisual(nullptr, &spTransformToRoot));
                IFC_RETURN(spTransformToRoot->TransformPoint(origin, &targetTopLeft));

                IFC_RETURN(m_tpElementVerticalThumb.Cast<Thumb>()->get_FlowDirection(&targetFlowDirection));

                if (xaml::FlowDirection_RightToLeft == targetFlowDirection)
                {
                    // RTL case
                    rcDockTo.left = static_cast<LONG>(targetTopLeft.X - thumbWidth);
                    rcDockTo.right = static_cast<LONG>(targetTopLeft.X);
                }
                else
                {
                    // Normal case
                    rcDockTo.left = static_cast<LONG>(targetTopLeft.X);
                    rcDockTo.right = static_cast<LONG>(targetTopLeft.X + thumbWidth);
                }
                rcDockTo.top = static_cast<LONG>(targetTopLeft.Y - lengthDelta);
                rcDockTo.bottom = static_cast<LONG>(targetTopLeft.Y - lengthDelta + thumbHeight);

                IFC_RETURN(ToolTipServiceFactory::GetToolTipObjectStatic(m_tpElementVerticalThumb.Cast<Thumb>(), &spToolTip));
                IFCEXPECT_RETURN(spToolTip);
                IFC_RETURN(spToolTip.Cast<ToolTip>()->PerformPlacement(&rcDockTo));
            }
        }

#ifdef TICKBAR_DBG_SLIDER
        swprintf_s(g_szTickBarDbgSlider, g_szTickBarDbgSliderLen,
            L"Slider::UpdateTrackLayout() - elementNewLength=%.2f", elementNewLength);
        Trace(g_szTickBarDbgSlider);
#endif // TICKBAR_DBG_SLIDER
    }

    IFC_RETURN(InvalidateTickBarsArrange());

    return S_OK;
}

_Check_return_
HRESULT
Slider::get_Thumb(
    _Out_ Thumb** ppThumb)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IThumb> spThumb;
    xaml_controls::Orientation orientation = xaml_controls::Orientation_Vertical;

    IFCPTR(ppThumb);
    *ppThumb = NULL;

    IFC(this->get_Orientation(&orientation));
    if (orientation == xaml_controls::Orientation_Horizontal)
    {
        spThumb = m_tpElementHorizontalThumb.Get();
    }
    else
    {
        spThumb = m_tpElementVerticalThumb.Get();
    }

    *ppThumb = static_cast<Thumb*>(spThumb.Detach());

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
Slider::get_ElementHorizontalTemplate(
    _Outptr_ xaml::IUIElement** ppElement)
{
    HRESULT hr = S_OK;
    IFCPTR(ppElement);

    IFC(ctl::do_query_interface(*ppElement, m_tpElementHorizontalTemplate.Get()));

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
Slider::get_ElementVerticalTemplate(
    _Outptr_ xaml::IUIElement** ppElement)
{
    HRESULT hr = S_OK;
    IFCPTR(ppElement);

    IFC(ctl::do_query_interface(*ppElement, m_tpElementVerticalTemplate.Get()));

Cleanup:
    RRETURN(hr);
}

// Returns the distance across the thumb in the direction of orientation.
//  e.g. returns the thumb width for horizontal orientation.
_Check_return_
HRESULT
Slider::GetThumbLength(
    _Out_ DOUBLE* pThumbLength)
{
    HRESULT hr = S_OK;
    xaml_controls::Orientation orientation = xaml_controls::Orientation_Vertical;
    DOUBLE length = 0;

    IFCPTR(pThumbLength);

    IFC(this->get_Orientation(&orientation));
    if (orientation == xaml_controls::Orientation_Horizontal)
    {
        if (m_tpElementHorizontalThumb)
        {
            IFC(m_tpElementHorizontalThumb.Cast<Thumb>()->get_ActualWidth(&length));
        }
    }
    else
    {
        if (m_tpElementVerticalThumb)
        {
            IFC(m_tpElementVerticalThumb.Cast<Thumb>()->get_ActualHeight(&length));
        }
    }

    *pThumbLength = length;

Cleanup:
    RRETURN(hr);
}

// Causes the TickBars to rearrange.
_Check_return_
HRESULT
Slider::InvalidateTickBarsArrange()
{
    HRESULT hr = S_OK;
    xaml_controls::Orientation orientation = xaml_controls::Orientation_Vertical;

    IFC(get_Orientation(&orientation));
    if (xaml_controls::Orientation_Horizontal == orientation)
    {
        // Horizontal - Top, HorizontalInline, and Bottom TickBars

        if (m_tpElementTopTickBar)
        {
            IFC(m_tpElementTopTickBar.Cast<TickBar>()->InvalidateArrange());
        }
        if (m_tpElementHorizontalInlineTickBar)
        {
            IFC(m_tpElementHorizontalInlineTickBar.Cast<TickBar>()->InvalidateArrange());
        }
        if (m_tpElementBottomTickBar)
        {
            IFC(m_tpElementBottomTickBar.Cast<TickBar>()->InvalidateArrange());
        }
    }
    else
    {
        // Vertical - Left, VerticalInline, and Right TickBars

        if (m_tpElementLeftTickBar)
        {
            IFC(m_tpElementLeftTickBar.Cast<TickBar>()->InvalidateArrange());
        }
        if (m_tpElementVerticalInlineTickBar)
        {
            IFC(m_tpElementVerticalInlineTickBar.Cast<TickBar>()->InvalidateArrange());
        }
        if (m_tpElementRightTickBar)
        {
            IFC(m_tpElementRightTickBar.Cast<TickBar>()->InvalidateArrange());
        }
    }

Cleanup:
    RRETURN(hr);
}

// Set Value to the next step position in the specified direction.  Uses SmallChange
// as the step interval if bUseSmallChange is TRUE; uses LargeChange otherwise.
_Check_return_
HRESULT
Slider::Step(
    _In_ BOOLEAN bUseSmallChange,
    _In_ BOOLEAN bForward)
{
    HRESULT hr = S_OK;
    xaml_primitives::SliderSnapsTo snapsTo = xaml_primitives::SliderSnapsTo_StepValues;
    DOUBLE stepDelta = 0;
    DOUBLE value = 0;
    DOUBLE max = 0;
    DOUBLE newValue = 0;
    DOUBLE closestStep = 0;

    IFC(get_SnapsTo(&snapsTo));
    if (snapsTo == xaml_primitives::SliderSnapsTo_Ticks)
    {
        if (bUseSmallChange)
        {
            IFC(get_TickFrequency(&stepDelta))
        }
        else
        {
            // For SliderSnapsTo=Ticks, we ignore SmallChange and move the Thumb by TickFrequency for arrow key events.
            // However, we still want to honor LargeChange for larger increments while keeping the Thumb on tick mark intervals.
            // To achieve this, we round LargeChange to the nearest multiple of TickFrequency.
            DOUBLE largeChange = 0;
            DOUBLE tickFrequency = 0;
            IFC(get_LargeChange(&largeChange));
            IFC(get_TickFrequency(&tickFrequency));
            stepDelta = DoubleUtil::Floor(largeChange / tickFrequency + 0.5) * tickFrequency;
        }

    }
    else
    {
        if (bUseSmallChange)
        {
            IFC(get_SmallChange(&stepDelta));
        }
        else
        {
            IFC(get_LargeChange(&stepDelta))
        }
    }

    IFC(get_Value(&value));

    // At the max end of the Slider, subtracting stepDelta and then rounding to the closest step may cause the
    // last stepDelta multiple to be skipped.  For example, the last tick mark may be skipped if TickFrequency
    // is not a factor of Maximum - Minimum.  To avoid this, we detect the condition where we are at the end of
    // the Slider and then go to the last stepDelta multiple before the end of the Slider.
    IFC(get_Maximum(&max));
    if (!bForward &&
        DoubleUtil::AreClose(value, max) &&
        !DoubleUtil::AreClose(DoubleUtil::Fractional(value / stepDelta), 0))
    {
        closestStep = DoubleUtil::Floor(value / stepDelta) * stepDelta;
    }
    else
    {
        newValue = bForward ? value + stepDelta : value - stepDelta;
        IFC(GetClosestStep(stepDelta, newValue, &closestStep));
    }
    IFC(put_Value(closestStep));

Cleanup:
    RRETURN(hr);
}

// Find the closest step position from fromValue.  stepValue lets you specify the step interval.
_Check_return_
HRESULT
Slider::GetClosestStep(
    _In_ DOUBLE stepDelta,
    _In_ DOUBLE fromValue,
    _Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;
    DOUBLE min = 0;
    DOUBLE max = 0;
    DOUBLE numSteps = 0;
    DOUBLE nextStep = 0;
    DOUBLE prevStep = 0;

    IFCPTR(pValue);

    IFC(get_Minimum(&min));
    IFC(get_Maximum(&max));

    numSteps = fromValue / stepDelta;
    nextStep = DoubleUtil::Min(max, DoubleUtil::Ceil(numSteps) * stepDelta);
    prevStep = DoubleUtil::Max(min, DoubleUtil::Floor(numSteps) * stepDelta);
    if (DoubleUtil::LessThan(nextStep - fromValue, fromValue - prevStep))
    {
        *pValue = nextStep;
    }
    else
    {
        *pValue = prevStep;
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
Slider::UpdateThumbToolTipVisibility(
    _In_ BOOLEAN bIsVisible,
    _In_ AutomaticToolTipInputMode mode)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<ToolTip> spCurrentThumbToolTip;

    IFC(GetCurrentThumbToolTip(&spCurrentThumbToolTip));

    if (spCurrentThumbToolTip)
    {
        // Set IsOpen.
        IFC(spCurrentThumbToolTip->RemoveAutomaticStatusFromOpenToolTip());
        if (bIsVisible)
        {
            spCurrentThumbToolTip->m_inputMode = mode;
        }
        IFC(spCurrentThumbToolTip->put_IsOpen(bIsVisible));
    }

Cleanup:
    RRETURN(hr);
}

//  Convert from source to target
//
//  Converts the Slider's DOUBLE Value to an HSTRING we can display in the
//  disambiguation UI tooltip's TextBlock.Text property.
//  Uses a weak reference to the Slider as a ConverterParameter.  We get the
//  Slider's StepFrequency and determine the number of significant digits in its
//  mantissa.  We will display the same number of significant digits in the mantissa
//  of the disambiguation UI's value.  We round to the final significant digit.
//
//  We choose to display a maximum of 4 significant digits in our formatted string.
//
//  E.G. If StepFrequency==0.1 and Value==0.57, the disambiguation UI shows 0.6
IFACEMETHODIMP
DefaultDisambiguationUIConverter::Convert(
    _In_ IInspectable *value,
    _In_ wxaml_interop::TypeName targetType,
    _In_opt_ IInspectable *parameter,
    _In_ HSTRING language,
    _Outptr_ IInspectable **returnValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wf::IPropertyValue> spValue;
    ctl::ComPtr<ISlider> spSlider;
    ctl::ComPtr<IInspectable> spBox;
    DOUBLE originalValue = 0;
    DOUBLE stepFrequency = 0;
    DOUBLE epsilon = 0.00001;   // We cap at 4 digits.
    XUINT32 numPlacesPastDecimalPoint = 0;
    const wchar_t* szFormat = nullptr;
    DOUBLE roundedValue = 0;

    IFCPTR(returnValue);
    *returnValue = nullptr;

    IFC(ctl::do_query_interface(spValue, value));
    IFC(spValue->GetDouble(&originalValue));

    spSlider.Attach(ValueWeakReference::get_value_as<ISlider>(parameter));
    if (spSlider)
    {
        IFC(spSlider->get_StepFrequency(&stepFrequency));

        // Determine the number of digits after the decimal point specified in step frequency.
        // We cap at 4 digits.
        while (DoubleUtil::Fractional(stepFrequency) > epsilon ||
            DoubleUtil::Fractional(stepFrequency) < -epsilon)
        {
            ++numPlacesPastDecimalPoint;
            if (numPlacesPastDecimalPoint == 4)
                break;
            stepFrequency *= 10;
        }

        switch (numPlacesPastDecimalPoint)
        {
        case 0:
            szFormat = L"%.0f";
            break;
        case 1:
            szFormat = L"%.1f";
            break;
        case 2:
            szFormat = L"%.2f";
            break;
        case 3:
            szFormat = L"%.3f";
            break;
        default:
            szFormat = L"%.4f";
            break;
        }
    }
    else
    {
        // This should not be a legit case, since we should always be able to resolve the weak reference to the Slider.
        // If it cannot resolve e.g. because we are shutting down, we'll just display the nearest INT.
        szFormat = L"%.0f";
    }

    roundedValue = DoubleUtil::Round(originalValue, numPlacesPastDecimalPoint);

    WCHAR szValue[32];
    IFCEXPECT(swprintf_s(szValue, 32, szFormat, roundedValue) >= 0);

    IFC(IValueBoxer::BoxValue(&spBox, wrl_wrappers::HStringReference(szValue, wcslen(szValue)).Get()));
    IFC(spBox.MoveTo(returnValue));

Cleanup:
    RRETURN(hr);
}

// Helper function to translate the thumb based on an input point.
// Calculates what percentage of the track the point represents, and sets the Slider's
// IntermediateValue and Value accordingly.
_Check_return_
HRESULT
Slider::MoveThumbToPoint(
    _In_ wf::Point point)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<Grid> spRootGrid;
    xaml_controls::Orientation orientation = xaml_controls::Orientation_Horizontal;
    DOUBLE trackActualLength = 0;
    DOUBLE thumbLength = 0;
    DOUBLE trackClickableRegionLength = 0;
    DOUBLE clickDelta = 0;
    DOUBLE clickPercentage = 0;
    BOOLEAN isDirectionReversed = FALSE;
    DOUBLE max = 0;
    DOUBLE min = 0;
    DOUBLE intermediateValue = 0;
    DOUBLE value = 0;
    xaml_primitives::SliderSnapsTo snapsTo = xaml_primitives::SliderSnapsTo_StepValues;
    DOUBLE tickFrequency = 0;
    DOUBLE stepFrequency = 0;
    DOUBLE closestStep = 0;

    m_bProcessingInputEvent = TRUE;

    IFC(GetRootGrid(&spRootGrid));
    if (spRootGrid)
    {
        wf::Point transformedPoint = {};
        ctl::ComPtr<xaml_media::IGeneralTransform> spTransformToRoot;
        ctl::ComPtr<xaml_media::IGeneralTransform> spTransformFromRoot;

        // Determine the length of the track, in pixels.
        IFC(get_Orientation(&orientation));
        if (orientation == xaml_controls::Orientation_Horizontal)
        {
            IFC(spRootGrid->get_ActualWidth(&trackActualLength));
        }
        else
        {
            IFC(spRootGrid->get_ActualHeight(&trackActualLength));
        }
        IFC(GetThumbLength(&thumbLength));

        // When the thumb is at the far left, the midpoint of the Thumb is still at thumbLength/2 offset.
        // So, the clickable region of the track is different from the actual length of the track.
        trackClickableRegionLength = DoubleUtil::Max(trackActualLength - thumbLength, 1);

        IFC(spRootGrid->TransformToVisual(NULL, &spTransformToRoot));
        IFC(spTransformToRoot->get_Inverse(&spTransformFromRoot));
        IFC(spTransformFromRoot->TransformPoint(point, &transformedPoint));

        // Determine what percentage of the track is contained in the input point.
        clickDelta = orientation == xaml_controls::Orientation_Horizontal ?
            transformedPoint.X : trackActualLength - transformedPoint.Y;
        clickPercentage = (clickDelta - thumbLength / 2) / trackClickableRegionLength;
        clickPercentage = DoubleUtil::Max(clickPercentage, 0);
        clickPercentage = DoubleUtil::Min(clickPercentage, 1.0);

        IFC(get_IsDirectionReversed(&isDirectionReversed));
        if (isDirectionReversed)
        {
            clickPercentage = 1 - clickPercentage;
        }

        // Calculate and set intermediateValue as a function of clickPercentage, min, and max.
        IFC(get_Maximum(&max));
        IFC(get_Minimum(&min));
        intermediateValue = min + clickPercentage * (max - min);
        IFC(put_IntermediateValue(intermediateValue));

        // Set value to the nearest step.
        IFC(get_Value(&value));
        IFC(get_SnapsTo(&snapsTo));
        if (snapsTo == xaml_primitives::SliderSnapsTo_Ticks)
        {
            IFC(get_TickFrequency(&tickFrequency));
            IFC(GetClosestStep(tickFrequency, intermediateValue, &closestStep));
        }
        else
        {
            IFC(this->get_StepFrequency(&stepFrequency));
            IFC(GetClosestStep(stepFrequency, intermediateValue, &closestStep));
        }

        if (!DoubleUtil::AreClose(value, closestStep))
        {
            IFC(put_Value(closestStep));
        }
    }

Cleanup:
    m_bProcessingInputEvent = FALSE;
    RRETURN(hr);
}

// Helper function to set a default ToolTip on the Slider Thumb.
//
// Slider has a "Disambiguation UI" feature that displays the Value of the Slider in a ToolTip centered
// on the Thumb.  Currently, this ToolTip is created in code if the Thumb template part's ToolTip is null.
_Check_return_
HRESULT
Slider::SetDefaultThumbToolTip(
    _In_ xaml_controls::Orientation orientation)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spSliderValueWeakReference;    // to be used as ConverterParameter
    ctl::ComPtr<xaml_data::IValueConverter> spConverter;
    ctl::ComPtr<TextBlock> spTextBlock;
    ctl::ComPtr<mut::IFontWeightsStatics> spFontWeightsStatics;
    ctl::ComPtr<Binding> spTextBinding;
    ctl::ComPtr<ToolTip> spToolTip;
    wut::FontWeight fontWeight = {};
    xaml_primitives::PlacementMode placementMode = xaml_primitives::PlacementMode_Top;
    BOOLEAN bIsThumbToolTipEnabled = FALSE;
    xaml::Thickness padding = { SLIDER_TOOLTIP_PADDING_LEFT, SLIDER_TOOLTIP_PADDING_TOP, SLIDER_TOOLTIP_PADDING_RIGHT, SLIDER_TOOLTIP_PADDING_BOTTOM };

    IFC(ValueWeakReference::Create(ctl::as_iinspectable(this), &spSliderValueWeakReference));

    // Use Slider.ThumbToolTipValueConverter if it is specified.  Otherwise, create and use a DefaultDisambiguationUIConverter.
    IFC(get_ThumbToolTipValueConverter(&spConverter));
    if (!spConverter)
    {
        ctl::ComPtr<DefaultDisambiguationUIConverter> spNewConverter;

        IFC(ctl::make<DefaultDisambiguationUIConverter>(&spNewConverter));
        spConverter = spNewConverter;
    }

    IFC(ctl::make<TextBlock>(&spTextBlock));

    IFC(wf::GetActivationFactory(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Text_FontWeights).Get(), spFontWeightsStatics.ReleaseAndGetAddressOf()));
    IFC(spFontWeightsStatics->get_Normal(&fontWeight));
    IFC(spTextBlock->put_FontWeight(fontWeight));
    IFC(spTextBlock->put_FontSize(SLIDER_TOOLTIP_DEFAULT_FONT_SIZE));

    IFC(ctl::make<Binding>(&spTextBinding));

    IFC(spTextBinding->put_Mode(xaml_data::BindingMode_OneWay));
    IFC(spTextBinding->put_Converter(spConverter.Get()));
    IFC(spTextBinding->put_ConverterParameter(spSliderValueWeakReference.Get()));

    IFC(spTextBlock->SetBinding(
        MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::TextBlock_Text),
        spTextBinding.Get()));

    IFC(ctl::make<ToolTip>(&spToolTip));

    spToolTip->put_Padding(padding);

    IFC(spToolTip->put_Content(ctl::as_iinspectable(spTextBlock.Get())));

    // For horizontal Slider, we place the disambiguation UI ToolTip on the side opposite of the user's handedness.
    if (orientation == xaml_controls::Orientation_Vertical)
    {
        placementMode = ToolTipPositioning::IsLefthandedUser() ?
            xaml_primitives::PlacementMode_Right :
            xaml_primitives::PlacementMode_Left;
    }
    IFC(spToolTip->put_Placement(placementMode));

    IFC(get_IsThumbToolTipEnabled(&bIsThumbToolTipEnabled));
    IFC(spToolTip->put_IsEnabled(bIsThumbToolTipEnabled));

    spToolTip->m_isSliderThumbToolTip = TRUE;

    if (orientation == xaml_controls::Orientation_Horizontal)
    {
        IFC(ToolTipServiceFactory::SetToolTipStatic(m_tpElementHorizontalThumb.Cast<Thumb>(), ctl::as_iinspectable(spToolTip.Get())));
    }
    else
    {
        IFC(ToolTipServiceFactory::SetToolTipStatic(m_tpElementVerticalThumb.Cast<Thumb>(), ctl::as_iinspectable(spToolTip.Get())));
    }

Cleanup:
    RRETURN(hr);
}


// Called when IsThumbToolTipEnabled changes.
_Check_return_
HRESULT
Slider::OnIsThumbToolTipEnabledChanged()
{
    HRESULT hr = S_OK;
    BOOLEAN bIsThumbToolTipEnabled = FALSE;

    IFC(get_IsThumbToolTipEnabled(&bIsThumbToolTipEnabled));

    if (m_tpElementHorizontalThumb)
    {
        ctl::ComPtr<IToolTip> spHorizontalThumbToolTip;

        IFC(ToolTipServiceFactory::GetToolTipObjectStatic(m_tpElementHorizontalThumb.Cast<Thumb>(), &spHorizontalThumbToolTip));
        if (spHorizontalThumbToolTip)
        {
            IFC(spHorizontalThumbToolTip.Cast<ToolTip>()->put_IsEnabled(bIsThumbToolTipEnabled));
        }
    }

    if (m_tpElementVerticalThumb)
    {
        ctl::ComPtr<IToolTip> spVerticalThumbToolTip;

        IFC(ToolTipServiceFactory::GetToolTipObjectStatic(m_tpElementVerticalThumb.Cast<Thumb>(), &spVerticalThumbToolTip));
        if (spVerticalThumbToolTip)
        {
            IFC(spVerticalThumbToolTip.Cast<ToolTip>()->put_IsEnabled(bIsThumbToolTipEnabled));
        }
    }

Cleanup:
    RRETURN(hr);
}

// Called when ThumbToolTipValueConverter changes.
_Check_return_
HRESULT
Slider::OnThumbToolTipValueConverterChanged()
{
    HRESULT hr = S_OK;

    if (m_bUsingDefaultToolTipForHorizontalThumb)
        IFC(SetDefaultThumbToolTip(xaml_controls::Orientation_Horizontal));

    if (m_bUsingDefaultToolTipForVerticalThumb)
        IFC(SetDefaultThumbToolTip(xaml_controls::Orientation_Vertical));

Cleanup:
    RRETURN(hr);
}

// Sets the m_isPressed flag, and updates visual state if the flag has changed.
_Check_return_
HRESULT
Slider::SetIsPressed(
    _In_ BOOLEAN isPressed)
{
    HRESULT hr = S_OK;

    if (isPressed != m_isPressed)
    {
        m_isPressed = isPressed;
        IFC(UpdateVisualState());
    }

Cleanup:
    RRETURN(hr);
}


// Gets the Horizontal or Vertical root Grid, depending on the current Orientation.
_Check_return_ HRESULT
Slider::GetRootGrid(
    _Outptr_ Grid** ppRootGrid)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_controls::IGrid> spGrid;
    xaml_controls::Orientation orientation = xaml_controls::Orientation_Vertical;

    IFCPTR(ppRootGrid);
    *ppRootGrid = NULL;

    IFC(get_Orientation(&orientation));

    if (orientation == xaml_controls::Orientation_Horizontal)
    {
        IFC(m_tpElementHorizontalTemplate.As(&spGrid));
    }
    else
    {
        IFC(m_tpElementVerticalTemplate.As(&spGrid));
    }

    *ppRootGrid = static_cast<Grid*>(spGrid.Detach());

Cleanup:
    RRETURN(hr);
}

// Get the disambiguation UI ToolTip for the current Thumb, depending on the orientation.
_Check_return_ HRESULT
Slider::GetCurrentThumbToolTip(
    _Outptr_ ToolTip** ppThumbToolTip)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<Thumb> spCurrentThumb;

    IFCPTR(ppThumbToolTip);
    *ppThumbToolTip = NULL;

    IFC(get_Thumb(&spCurrentThumb));
    if (spCurrentThumb)
    {
        ctl::ComPtr<IToolTip> spToolTip;

        // Get the ToolTip.
        IFC(ToolTipServiceFactory::GetToolTipObjectStatic(spCurrentThumb.Get(), &spToolTip));

        *ppThumbToolTip = static_cast<ToolTip*>(spToolTip.Detach());
    }

Cleanup:
    RRETURN(hr);
}

// Update the visibility of TickBars in the template when the TickPlacement property changes.
_Check_return_ HRESULT
Slider::OnTickPlacementChanged()
{
    HRESULT hr = S_OK;
    xaml_primitives::TickPlacement tickPlacement = xaml_primitives::TickPlacement_None;
    xaml::Visibility topAndLeftVisibility = xaml::Visibility_Collapsed;
    xaml::Visibility inlineVisibility = xaml::Visibility_Collapsed;
    xaml::Visibility bottomAndRightVisibility = xaml::Visibility_Collapsed;

    IFC(get_TickPlacement(&tickPlacement));
    switch (tickPlacement)
    {
    case xaml_primitives::TickPlacement_TopLeft:
        topAndLeftVisibility = xaml::Visibility_Visible;
        break;
    case xaml_primitives::TickPlacement_BottomRight:
        bottomAndRightVisibility = xaml::Visibility_Visible;
        break;
    case xaml_primitives::TickPlacement_Outside:
        topAndLeftVisibility = xaml::Visibility_Visible;
        bottomAndRightVisibility = xaml::Visibility_Visible;
        break;
    case xaml_primitives::TickPlacement_Inline:
        inlineVisibility = xaml::Visibility_Visible;
        break;
    }

    if (m_tpElementTopTickBar)
    {
        IFC(m_tpElementTopTickBar.Cast<TickBar>()->put_Visibility(topAndLeftVisibility));
    }
    if (m_tpElementHorizontalInlineTickBar)
    {
        IFC(m_tpElementHorizontalInlineTickBar.Cast<TickBar>()->put_Visibility(inlineVisibility));
    }
    if (m_tpElementBottomTickBar)
    {
        IFC(m_tpElementBottomTickBar.Cast<TickBar>()->put_Visibility(bottomAndRightVisibility));
    }
    if (m_tpElementLeftTickBar)
    {
        IFC(m_tpElementLeftTickBar.Cast<TickBar>()->put_Visibility(topAndLeftVisibility));
    }
    if (m_tpElementVerticalInlineTickBar)
    {
        IFC(m_tpElementVerticalInlineTickBar.Cast<TickBar>()->put_Visibility(inlineVisibility));
    }
    if (m_tpElementRightTickBar)
    {
        IFC(m_tpElementRightTickBar.Cast<TickBar>()->put_Visibility(bottomAndRightVisibility));
    }

Cleanup:
    RRETURN(hr);
}

// Updates the visibility of the Header ContentPresenter
_Check_return_ HRESULT
Slider::UpdateHeaderPresenterVisibility()
{
    ctl::ComPtr<xaml::IDataTemplate> spHeaderTemplate;
    ctl::ComPtr<IInspectable> spHeader;

    IFC_RETURN(get_HeaderTemplate(&spHeaderTemplate));
    IFC_RETURN(get_Header(&spHeader));

    IFC_RETURN(ConditionallyGetTemplatePartAndUpdateVisibility(
        XSTRING_PTR_EPHEMERAL(L"HeaderContentPresenter"),
        (spHeader || spHeaderTemplate),
        m_tpHeaderPresenter));

    return S_OK;
}

_Check_return_ HRESULT Slider::OnIsFocusEngagedChanged()
{
    IFC_RETURN(UpdateVisualState());

    BOOLEAN isFocusEngaged = FALSE;
    IFC_RETURN(get_IsFocusEngaged(&isFocusEngaged));

    IFC_RETURN(UpdateThumbToolTipVisibility(isFocusEngaged, AutomaticToolTipInputMode::Keyboard));

    return S_OK;
}

DOUBLE Slider::LayoutRoundedDimension(DOUBLE dimension)
{
    BOOLEAN useLayoutRounding = FALSE;
    IFCFAILFAST(get_UseLayoutRounding(&useLayoutRounding));
    if (useLayoutRounding)
    {
        FLOAT dimensionF = static_cast<FLOAT>(dimension);
        IFCFAILFAST(LayoutRound(dimensionF, &dimensionF));
        dimension = dimensionF;
    }

    return dimension;
}

float Slider::RoundingStep() const
{
    const float scale = RootScale::GetRasterizationScaleForElement(GetHandle());
    const float roundingStep = 1.0f / scale;
    return roundingStep;
}
