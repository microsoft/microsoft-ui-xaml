// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Thumb.g.h"
#include "ThumbAutomationPeer.g.h"
#include "Slider.g.h"
#include "DragStartedEventArgs.g.h"
#include "DragDeltaEventArgs.g.h"
#include "DragCompletedEventArgs.g.h"
#include "ScrollBar.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

// Initializes a new instance of the Thumb class.
Thumb::Thumb():
    m_ignoreTouchInput(FALSE)
{
    m_origin.X = m_origin.Y = 0;
    m_previousPosition.X = m_previousPosition.Y = 0;
}

Thumb::~Thumb()
{
}

// Cancel a drag operation if it is currently in progress.
 _Check_return_ HRESULT Thumb::CancelDragImpl()
{
    HRESULT hr = S_OK;
    BOOLEAN bIsDragging = FALSE;

    IFC(get_IsDragging(&bIsDragging));
    if(bIsDragging)
    {
        IFC(put_IsDragging(FALSE));
        IFC(RaiseDragCompleted(TRUE));
    }
Cleanup:
    RRETURN(hr);
}

// Handle the custom property changed event and call the
// OnPropertyChanged2 methods. 
_Check_return_ 
HRESULT 
Thumb::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    HRESULT hr = S_OK;

    IFC(ThumbGenerated::OnPropertyChanged2(args));
          
    switch (args.m_pDP->GetIndex())
    {
        case KnownPropertyIndex::Thumb_IsDragging:
            IFC(OnDraggingChanged());
            break;

        case KnownPropertyIndex::UIElement_Visibility:
            IFC(OnVisibilityChanged());
            break;

        case KnownPropertyIndex::UIElement_ManipulationMode:
            if (m_prePenDragManipulationMode != xaml_input::ManipulationModes_None)
            {
                // This Thumb's ManipulationMode property is being changed after it was temporarily cached and altered to support a pen-driven drag.
                // Reset the cached value so this new ManipulationMode value does not get overridden at the end of the drag.
                m_prePenDragManipulationMode = xaml_input::ManipulationModes_None;
            }
            break;
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT Thumb::OnDraggingChanged()
{
    HRESULT hr = S_OK;
    IFC(UpdateVisualState());

Cleanup:
    RRETURN(hr);
}

// Update the visual states when the Visibility property is changed.
_Check_return_ HRESULT
Thumb::OnVisibilityChanged()
{
    HRESULT hr = S_OK;
    xaml::Visibility visibility = xaml::Visibility_Collapsed;

    IFC(get_Visibility(&visibility));
    if (xaml::Visibility_Visible != visibility)
    {
        IFC(put_IsPointerOver(FALSE));
    }

    IFC(UpdateVisualState());

Cleanup:
    RRETURN(hr);
}

// Called when the IsEnabled property changes.
_Check_return_ HRESULT Thumb::OnIsEnabledChanged(
    _In_ IsEnabledChangedEventArgs* pArgs)
{
    BOOLEAN isEnabled = FALSE;

    IFC_RETURN(get_IsEnabled(&isEnabled));
    if (!isEnabled)
    {
        IFC_RETURN(put_IsPointerOver(FALSE));

        BOOLEAN isDragging = FALSE;

        IFC_RETURN(get_IsDragging(&isDragging));
        if (isDragging)
        {
            IFC_RETURN(put_IsDragging(FALSE));
            IFC_RETURN(RaiseDragCompleted(FALSE /*isCanceled*/));
        }
    }

    IFC_RETURN(UpdateVisualState());

    return S_OK;
}

// GotFocus event handler.
IFACEMETHODIMP Thumb::OnGotFocus(
    _In_ xaml::IRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    // TODO: We should expose FrameworkElement::HasFocus method
    BOOLEAN hasFocus = TRUE;

    IFC(FocusChanged(hasFocus));

Cleanup:
    RRETURN(hr);
}

// LostFocus event handler.
IFACEMETHODIMP Thumb::OnLostFocus(
    _In_ xaml::IRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    // TODO: We should expose FrameworkElement::HasFocus method
    BOOLEAN hasFocus = FALSE;

    IFC(FocusChanged(hasFocus));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT Thumb::FocusChanged(_In_ BOOLEAN hasFocus)
{
    HRESULT hr = S_OK;
    IFC(UpdateVisualState());

Cleanup:
    RRETURN(hr);
}

// Sets a value indicating whether the Thumb reacts to touch input or not.
_Check_return_ HRESULT Thumb::put_IgnoreTouchInput(
    _In_ BOOLEAN value)
{
    m_ignoreTouchInput = value;
    RRETURN(S_OK);
}

_Check_return_ HRESULT Thumb::ShouldIgnoreInput(
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs, 
    _Out_ BOOLEAN* pIgnoreInput)
{
    HRESULT hr = S_OK;
    mui::PointerDeviceType nPointerDeviceType = mui::PointerDeviceType_Touch;

    IFCPTR(pArgs);
    IFCPTR(pIgnoreInput);
    *pIgnoreInput = FALSE;

    if (m_ignoreTouchInput)
    {
        ctl::ComPtr<ixp::IPointerPoint> spPointerPoint;
        
        IFC(pArgs->GetCurrentPoint(NULL, &spPointerPoint));
        IFCPTR(spPointerPoint);
        IFC(spPointerPoint->get_PointerDeviceType(&nPointerDeviceType));
        if (nPointerDeviceType == mui::PointerDeviceType_Touch)
        {
            *pIgnoreInput = TRUE;
        }
    }
Cleanup:
    RRETURN(hr);
}

// PointerEnter event handler.
IFACEMETHODIMP Thumb::OnPointerEntered(
    _In_ IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN bIsEnabled = FALSE;
    BOOLEAN pIgnoreInput = FALSE;
    IFCPTR(pArgs);

    IFC(ShouldIgnoreInput(pArgs, &pIgnoreInput));
    if (pIgnoreInput)
    {
        goto Cleanup;
    }

    IFC(get_IsEnabled(&bIsEnabled));

    if (bIsEnabled)
    {
        IFC(put_IsPointerOver(TRUE));
        IFC(UpdateVisualState());
    }

Cleanup:
    RRETURN(hr);
}

// PointerExited event handler.
IFACEMETHODIMP Thumb::OnPointerExited(
    _In_ IPointerRoutedEventArgs* pArgs)
{

    HRESULT hr = S_OK;
    BOOLEAN bIsEnabled = FALSE;
    BOOLEAN pIgnoreInput = FALSE;
    IFCPTR(pArgs);

    IFC(ShouldIgnoreInput(pArgs, &pIgnoreInput));
    if (pIgnoreInput)
    {
        goto Cleanup;
    }
    IFC(get_IsEnabled(&bIsEnabled));

    if (bIsEnabled)
    {
        IFC(put_IsPointerOver(FALSE));
        IFC(UpdateVisualState());
    }

Cleanup:
    RRETURN(hr);  
}

// PointerMoved event handler.
IFACEMETHODIMP Thumb::OnPointerMoved(
    _In_ IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    wf::Point position = {};
    wf::Point transformedPosition = {};
    BOOLEAN bIsDragging = FALSE;
    BOOLEAN pIgnoreInput = FALSE;

    IFCPTR(pArgs);
    IFC(ShouldIgnoreInput(pArgs, &pIgnoreInput));
    if (pIgnoreInput)
    {
        goto Cleanup;
    }

    IFC(get_IsDragging(&bIsDragging));
    if (bIsDragging)
    {
        ctl::ComPtr<ixp::IPointerPoint> spPointerPoint;
        
        IFC(pArgs->GetCurrentPoint(NULL, &spPointerPoint));
        IFCPTR(spPointerPoint);
        IFC(spPointerPoint->get_Position(&position));
        IFC(m_tpTransformToOriginal->TransformPoint(position, &transformedPosition));

        if (transformedPosition.X != m_previousPosition.X || transformedPosition.Y != m_previousPosition.Y)
        {
            IFC(RaiseDragDelta(transformedPosition.X - m_previousPosition.X, transformedPosition.Y - m_previousPosition.Y));
            m_previousPosition = transformedPosition;
        }
    }

Cleanup:
    RRETURN(hr);
}

// PointerPressed event handler.
IFACEMETHODIMP Thumb::OnPointerPressed(
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IDependencyObject> spParent;
    ctl::ComPtr<ixp::IPointerPoint> spPointerPoint;
    ctl::ComPtr<ixp::IPointerPointProperties> spPointerProperties;
    BOOLEAN bHandled = FALSE;
    BOOLEAN bIsEnabled = FALSE;
    BOOLEAN bIsDragging = FALSE;
    BOOLEAN pIgnoreInput = FALSE;
    wf::Point position = {};
    wf::Point transformedPosition = {};
    BOOLEAN bIsLeftButtonPressed = FALSE;
    mui::PointerDeviceType nPointerDeviceType = mui::PointerDeviceType_Touch;
    AutomaticToolTipInputMode mode = AutomaticToolTipInputMode::None;

    IFCPTR(pArgs);
    IFC(pArgs->get_Handled(&bHandled));
    if (bHandled)
    {
        goto Cleanup;
    }

    IFC(ShouldIgnoreInput(pArgs, &pIgnoreInput));
    if (pIgnoreInput)
    {
        goto Cleanup;
    }

    IFC(get_IsDragging(&bIsDragging));
    IFC(get_IsEnabled(&bIsEnabled));
    IFC(get_Parent(&spParent));

    IFC(pArgs->GetCurrentPoint(NULL, &spPointerPoint));
    IFCPTR(spPointerPoint);
    IFC(spPointerPoint->get_Properties(&spPointerProperties));
    IFCPTR(spPointerProperties);
    IFC(spPointerProperties->get_IsLeftButtonPressed(&bIsLeftButtonPressed));

    if (bIsLeftButtonPressed) 
    {
        if (!bIsDragging && bIsEnabled && spParent)
        {
            ctl::ComPtr<DependencyObject> spTemplatedParent;
            ctl::ComPtr<ISlider> spTemplatedParentAsSlider;
            ctl::ComPtr<IScrollBar> spTemplatedParentAsScrollBar;
            ctl::ComPtr<IGeneralTransform> spTransform;
            ctl::ComPtr<IGeneralTransform> spInvertedTransform;
            ctl::ComPtr<xaml_input::IPointer> spPointer;
            
            BOOLEAN bPointerCaptured = FALSE;

            IFC(spPointerPoint->get_PointerDeviceType(&nPointerDeviceType));

            IFC(this->get_TemplatedParent(&spTemplatedParent));
            spTemplatedParentAsSlider = spTemplatedParent.AsOrNull<ISlider>();
            if (spTemplatedParentAsSlider)
            {
                // Since we are marking the event as handled, Slider never sees it because of this bug:
                //      33598 - RoutedEvent Handled flag shouldn't be modified by templated parts
                // Therefore, as long as Thumb continues to handle this event, we need to report to Slider
                // that it must open its disambiguation UI.
                
                if (nPointerDeviceType == mui::PointerDeviceType_Touch)
                {
                    mode = AutomaticToolTipInputMode::Touch;
                }
                else
                {
                    mode = AutomaticToolTipInputMode::Mouse;
                }

                Slider* pTemplatedParentAsSliderConcreteNoRef = spTemplatedParentAsSlider.Cast<Slider>();
                IFC(pTemplatedParentAsSliderConcreteNoRef->UpdateThumbToolTipVisibility(TRUE, mode));
                IFC(pTemplatedParentAsSliderConcreteNoRef->SetIsPressed(TRUE));
            }

            spTemplatedParentAsScrollBar = spTemplatedParent.AsOrNull<IScrollBar>();
            if (spTemplatedParentAsScrollBar)
            {
                BOOLEAN bIsIgnoringUserInput = TRUE;
                IFC(spTemplatedParentAsScrollBar.Cast<ScrollBar>()->get_IsIgnoringUserInput(bIsIgnoringUserInput));
                if (bIsIgnoringUserInput)
                {
                    // Do not start a thumb drag operation when the owning ScrollBar was told to ignore user input.
                    goto Cleanup;
                }
                IFC(CacheAndReplacePrePenDragManipulationMode(nPointerDeviceType));
            }

            IFC(pArgs->put_Handled(TRUE));

            IFC(pArgs->get_Pointer(&spPointer));
            IFCPTR(spPointer);
            IFC(CapturePointer(spPointer.Get(), &bPointerCaptured));
            IFC(put_IsDragging(TRUE));

            // If logical parent is a popup, TransformToVisual can fail because a popup's child
            // can be in the visual tree without the popup being in the tree, if the popup is
            // created in code and has IsOpen set to true. Therefore, if we're in a popup, we
            // use coordinates relative to the thumb instead of relative to Thumb.Parent.

            // TODO: Uncomment following code when(if) we bring Popup support
            if (FALSE)//ctl::is<xaml_primitives::IPopup>(spParent.Get()))
            {
                IFC(TransformToVisual(NULL, &spTransform));
                IFC(spTransform->get_Inverse(&spInvertedTransform));
            }
            else
            {
                ctl::ComPtr<IUIElement> spParentAsUIE;
                
                IFC(spParent.As(&spParentAsUIE));
                IFC(spParentAsUIE->TransformToVisual(NULL, &spTransform));
                IFC(spTransform->get_Inverse(&spInvertedTransform));
            }

            SetPtrValue(m_tpTransformToOriginal, spInvertedTransform.Get());

            IFC(spPointerPoint->get_Position(&position));
            IFC(m_tpTransformToOriginal->TransformPoint(position, &transformedPosition));

            m_origin = m_previousPosition = transformedPosition;

            hr = RaiseDragStarted();
            if (FAILED(hr))
            {
                IGNOREHR(CancelDrag());
                IFC(hr);
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

// PointerReleased event handler.
IFACEMETHODIMP Thumb::OnPointerReleased(
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN bIsDragging = FALSE;
    BOOLEAN bIsEnabled = FALSE;
    BOOLEAN pIgnoreInput = FALSE;
    ctl::ComPtr<xaml_input::IPointer> spPointer;
    
    IFCPTR(pArgs);

    IFC(RestorePrePenDragManipulationMode(pArgs));

    IFC(ShouldIgnoreInput(pArgs, &pIgnoreInput));
    if (pIgnoreInput)
    {
        goto Cleanup;
    }

    IFC(pArgs->get_Pointer(&spPointer));
    IFC(get_IsDragging(&bIsDragging));
    IFC(get_IsEnabled(&bIsEnabled));
    if(bIsDragging && bIsEnabled)
    {
        IFC(put_IsDragging(FALSE));
        IFC(ReleasePointerCapture(spPointer.Get()));
        IFC(RaiseDragCompleted(FALSE));
    }

Cleanup:
    RRETURN(hr);
}

// PointerCaptureLost event handler.
IFACEMETHODIMP Thumb::OnPointerCaptureLost(
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN bIsDragging = FALSE;
    BOOLEAN bIsEnabled = FALSE;
    BOOLEAN pIgnoreInput = FALSE;
    ctl::ComPtr<xaml_input::IPointer> spPointer;
    
    IFC(ThumbGenerated::OnPointerCaptureLost(pArgs));

    IFC(RestorePrePenDragManipulationMode(pArgs));

    IFC(ShouldIgnoreInput(pArgs, &pIgnoreInput));
    if (pIgnoreInput)
    {
        goto Cleanup;
    }

    IFC(pArgs->get_Pointer(&spPointer));
    IFC(get_IsDragging(&bIsDragging));
    IFC(get_IsEnabled(&bIsEnabled));
    if(bIsDragging && bIsEnabled)
    {
        IFC(put_IsDragging(FALSE));
        IFC(ReleasePointerCapture(spPointer.Get()));
        IFC(RaiseDragCompleted(FALSE));
    }

Cleanup:
    RRETURN(hr);
}

// Restores the Thumb's ManipulationMode used prior to a pen-driven drag, if needed.
IFACEMETHODIMP Thumb::OnPointerCanceled(
    _In_ xaml_input::IPointerRoutedEventArgs* args)
{
    IFC_RETURN(ThumbGenerated::OnPointerCanceled(args));
    IFC_RETURN(RestorePrePenDragManipulationMode(args));
    return S_OK;
}

// Create ThumbAutomationPeer to represent the Thumb.
IFACEMETHODIMP Thumb::OnCreateAutomationPeer(_Outptr_ xaml_automation_peers::IAutomationPeer** ppAutomationPeer)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_automation_peers::IThumbAutomationPeer> spThumbAutomationPeer;
    ctl::ComPtr<xaml_automation_peers::IThumbAutomationPeerFactory> spThumbAPFactory;
    ctl::ComPtr<IActivationFactory> spActivationFactory;
    ctl::ComPtr<IInspectable> spInner;
    
    spActivationFactory.Attach(ctl::ActivationFactoryCreator<DirectUI::ThumbAutomationPeerFactory>::CreateActivationFactory());
    IFC(spActivationFactory.As(&spThumbAPFactory));

    IFC(spThumbAPFactory.Cast<ThumbAutomationPeerFactory>()->CreateInstanceWithOwner(this, 
        NULL, 
        &spInner, 
        &spThumbAutomationPeer));
    IFC(spThumbAutomationPeer.CopyTo(ppAutomationPeer));

Cleanup:
    RRETURN(hr);
}

// Caches and replaces the Thumb's ManipulationMode used prior to a pen-driven drag, if needed.
_Check_return_ HRESULT Thumb::CacheAndReplacePrePenDragManipulationMode(mui::PointerDeviceType nPointerDeviceType)
{
    if (nPointerDeviceType == mui::PointerDeviceType_Pen)
    {
        xaml_input::ManipulationModes prePenDragManipulationMode = xaml_input::ManipulationModes_None;

        IFC_RETURN(get_ManipulationMode(&prePenDragManipulationMode));
        if ((prePenDragManipulationMode & xaml_input::ManipulationModes_System) == xaml_input::ManipulationModes_System)
        {
            IFC_RETURN(put_ManipulationMode(prePenDragManipulationMode & ~xaml_input::ManipulationModes_System));
            m_prePenDragManipulationMode = prePenDragManipulationMode;
        }
    }
    return S_OK;
}

// Restores the Thumb's ManipulationMode used prior to a pen-driven drag, if needed.
_Check_return_ HRESULT Thumb::RestorePrePenDragManipulationMode(
    _In_ xaml_input::IPointerRoutedEventArgs* args)
{
    if (m_prePenDragManipulationMode != xaml_input::ManipulationModes_None)
    {
        mui::PointerDeviceType nPointerDeviceType = mui::PointerDeviceType_Touch;
        ctl::ComPtr<ixp::IPointerPoint> spPointerPoint;

        IFC_RETURN(args->GetCurrentPoint(nullptr, &spPointerPoint));
        IFCPTR_RETURN(spPointerPoint);
        IFC_RETURN(spPointerPoint->get_PointerDeviceType(&nPointerDeviceType));

        if (static_cast<int>(nPointerDeviceType) == static_cast<int>(wdei::PointerDeviceType_Pen))
        {
            xaml_input::ManipulationModes prePenDragManipulationMode = m_prePenDragManipulationMode;

            m_prePenDragManipulationMode = xaml_input::ManipulationModes_None;
            IFC_RETURN(put_ManipulationMode(prePenDragManipulationMode));
        }
    }
    return S_OK;
}

_Check_return_ HRESULT Thumb::RaiseDragStarted()
{
    HRESULT hr = S_OK;
    DragStartedEventSourceType* pEventSource = nullptr;
    ctl::ComPtr<DragStartedEventArgs> spArgs;

    // Create the args
    IFC(ctl::make<DragStartedEventArgs>(&spArgs));

    IFC(spArgs->put_HorizontalOffset(m_origin.X));
    IFC(spArgs->put_VerticalOffset(m_origin.Y));
    
    // Raise the event
    IFC(GetDragStartedEventSourceNoRef(&pEventSource));
    IFC(pEventSource->Raise(ctl::as_iinspectable(this), spArgs.Get()));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT Thumb::RaiseDragDelta(_In_ DOUBLE dDeltaX, _In_ DOUBLE dDeltaY)
{
    HRESULT hr = S_OK;
    DragDeltaEventSourceType* pEventSource = nullptr;
    ctl::ComPtr<DragDeltaEventArgs> spArgs;

    // Create the args
    IFC(ctl::make<DragDeltaEventArgs>(&spArgs));

    IFC(spArgs->put_HorizontalChange(dDeltaX));
    IFC(spArgs->put_VerticalChange(dDeltaY));
    
    // Raise the event
    IFC(GetDragDeltaEventSourceNoRef(&pEventSource));
    IFC(pEventSource->Raise(ctl::as_iinspectable(this), spArgs.Get()));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT Thumb::RaiseDragCompleted(_In_ BOOLEAN isCanceled)
{
    HRESULT hr = S_OK;
    DragCompletedEventSourceType* pEventSource = nullptr;
    ctl::ComPtr<DragCompletedEventArgs> spArgs;

    // Create the args
    IFC(ctl::make<DragCompletedEventArgs>(&spArgs));

    IFC(spArgs->put_HorizontalChange(m_previousPosition.X - m_origin.X));
    IFC(spArgs->put_VerticalChange(m_previousPosition.Y - m_origin.Y));
    IFC(spArgs->put_Canceled(isCanceled));
    
    // Raise the event
    IFC(GetDragCompletedEventSourceNoRef(&pEventSource));
    IFC(pEventSource->Raise(ctl::as_iinspectable(this), spArgs.Get()));

Cleanup:
    RRETURN(hr);
}

/// Change to the correct visual state for the thumb.
_Check_return_ HRESULT Thumb::UpdateVisualState(
    // true to use transitions when updating the visual state, false
    // to snap directly to the new visual state.
    _In_ bool bUseTransitions)
{
    HRESULT hr = S_OK;
    BOOLEAN bIsEnabled = FALSE;
    BOOLEAN bIsDragging = FALSE;
    BOOLEAN bIsPointerOver = FALSE;
    BOOLEAN bIgnored = FALSE;
    xaml::FocusState focusState = xaml::FocusState_Unfocused;

    IFC(get_IsEnabled(&bIsEnabled));
    IFC(get_IsDragging(&bIsDragging));
    IFC(get_IsPointerOver(&bIsPointerOver));
    IFC(get_FocusState(&focusState));

    if (!bIsEnabled)
    {
        IFC(GoToState(bUseTransitions, L"Disabled", &bIgnored));
    }
    else if (bIsDragging)
    {
        IFC(GoToState(bUseTransitions, L"Pressed", &bIgnored));
    }
    else if (bIsPointerOver)
    {
        IFC(GoToState(bUseTransitions, L"PointerOver", &bIgnored));
    }
    else
    {
        IFC(GoToState(bUseTransitions, L"Normal", &bIgnored));
    }

    if (xaml::FocusState_Unfocused != focusState && bIsEnabled)
    {
        IFC(GoToState(bUseTransitions, L"Focused", &bIgnored));
    }
    else
    {
        IFC(GoToState(bUseTransitions, L"Unfocused", &bIgnored));
    }
Cleanup:
    RRETURN(hr);
}

// Apply a template to the Thumb.
IFACEMETHODIMP Thumb::OnApplyTemplate()
{
    HRESULT hr = S_OK;

    IFC(ThumbGenerated::OnApplyTemplate());

    // Sync the logical and visual states of the control
    IFC(UpdateVisualState(FALSE));

Cleanup:
    RRETURN(hr);
}

