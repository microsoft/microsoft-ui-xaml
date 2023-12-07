// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "UIElement.g.h"
#include "StartDragAsyncOperation.h"
#include "DragStartingEventArgs.g.h"
#include "DropCompletedEventArgs.g.h"
#include "DependentAsyncWorker.h"
#include "AutomaticDragHelper.h"
#include "DragDropInternal.h"
#include "microsoft.ui.input.dragdrop.h"

using namespace DirectUI;

_Check_return_ HRESULT UIElement::GetDragStartingEventSourceNoRef(_Outptr_ DragStartingEventSourceType** ppEventSource)
{
    HRESULT hr = S_OK;

    ARG_VALIDRETURNPOINTER(ppEventSource);
    *ppEventSource = nullptr;

    CDragStartingEventSource* pEventSource = static_cast<CDragStartingEventSource*>(GetEventSourceNoRef(KnownEventIndex::UIElement_DragStarting));
    if (pEventSource == nullptr)
    {
        ctl::ComPtr<CDragStartingEventSource> spEventSource;
        IFC(ctl::make(&spEventSource));
        spEventSource->Initialize(KnownEventIndex::UIElement_DragStarting, this, /* bUseEventManager */ false);
        IFC(StoreEventSource(KnownEventIndex::UIElement_DragStarting, spEventSource.Get()));

        // The caller doesn't expect a ref, this function ends in "NoRef".  The ref is now owned by the map (inside StoreEventSource)
        pEventSource = spEventSource.Get();
    }

    *ppEventSource = pEventSource;

Cleanup:
    RRETURN(hr);
}

// Anchor point has to take into account RTL as well as scale factor
void UIElement::GetAnchorPoint(
    _In_ wf::Point pointerLocation,
    _Out_ wf::Point* anchorPoint)
{
    // Check if we need to mirror the anchor point
    // Note that FlowDirection and ActualWidth are not exposed by UIElement but FrameworkElement
    // and this is why we use core apis
    CUIElement* pElementNoRef = static_cast<CUIElement*>(GetHandle());
    if (pElementNoRef->IsRightToLeft())
    {
        pointerLocation.X = pElementNoRef->GetActualWidth() - pointerLocation.X;
    }
    const auto scale = RootScale::GetRasterizationScaleForElement(GetHandle());
    DXamlCore::GetCurrent()->DipsToPhysicalPixels(scale, &pointerLocation, anchorPoint);
}

_Check_return_ HRESULT UIElement::StartDragAsyncImpl(
    _In_ ixp::IPointerPoint* pPointerPoint,
    _Outptr_ wf::IAsyncOperation<wadt::DataPackageOperation>** ppReturnValue)
{
    IFCPTR_RETURN(pPointerPoint);
    IFCPTR_RETURN(ppReturnValue);
    *ppReturnValue = nullptr;

    ctl::ComPtr<mui::DragDrop::IDragOperation> spDragOperation;
    IFC_RETURN(wf::ActivateInstance(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Input_DragDrop_DragOperation).Get(),
        spDragOperation.GetAddressOf()));

    wf::Point position;
    IFC_RETURN(pPointerPoint->get_Position(&position));
    GetAnchorPoint(position, &position);

    UINT pointerId = 0;
    IFC_RETURN(pPointerPoint->get_PointerId(&pointerId));

    mui::PointerDeviceType deviceType;
    IFC_RETURN(pPointerPoint->get_PointerDeviceType(&deviceType));

    const bool isMouse = (deviceType == mui::PointerDeviceType_Mouse);
    if (!isMouse)
    {
        const auto hmod = ::GetModuleHandleA("microsoft.ui.input.dll");
        if (hmod != nullptr)
        {
            using LiftedInputCapturePointerIdForDragAndDrop = HRESULT(*)(UINT);
            const auto captureForDragAndDrop = reinterpret_cast<LiftedInputCapturePointerIdForDragAndDrop>(::GetProcAddress(hmod, "LiftedInputCapturePointerIdForDragAndDrop"));
            if (captureForDragAndDrop)
            {
                IFC_RETURN(captureForDragAndDrop(pointerId));
            }
        }
    }

    ctl::WeakRefPtr wpThis;
    IFC_RETURN(ctl::AsWeak(this, &wpThis));

    wrl::ComPtr<StartDragAsyncOperation> spStartDragAsyncOperation;
    IFC_RETURN(wrl::MakeAndInitialize<StartDragAsyncOperation>(&spStartDragAsyncOperation, wpThis, spDragOperation.Get()));

    // Get the Datapackage
    ctl::ComPtr<wadt::IDataPackage> spDataPackage;
    IFC_RETURN(spDragOperation->get_Data(spDataPackage.GetAddressOf()));

    // Create the args for the evant
    ctl::ComPtr<DragStartingEventArgs> spArgs;
    IFC_RETURN(ctl::make<DragStartingEventArgs>(&spArgs));
    IFC_RETURN(spArgs->SetOperationContext(position, spStartDragAsyncOperation.Get(), spDataPackage.Get(), pPointerPoint));

    // Raise the event
    DragStartingEventSourceType* pEventSource = nullptr;
    IFC_RETURN(GetDragStartingEventSourceNoRef(&pEventSource));
    IFC_RETURN(static_cast<CDragStartingEventSource*>(pEventSource)->Raise(this, spArgs.Get(), true /* shouldStartOperation */));

    // Copy the allowed operations to the core
    wadt::DataPackageOperation allowedOperations;
    IFC_RETURN(spArgs->get_AllowedOperations(&allowedOperations));

    // If allowedOperations has every operation set - treat this as no preference with regards to
    // CoreDragOperation and do not set it.  This will allow the app to set either
    // AllowedOperations(th2) or RequestedOperation(th1).  See TFS 7142358 for more info.
    if (allowedOperations !=
        (wadt::DataPackageOperation_Copy |
         wadt::DataPackageOperation_Move |
         wadt::DataPackageOperation_Link))
    {
        IFC_RETURN(spDragOperation->put_AllowedOperations(allowedOperations));
    }

    // Each DXamlCore instance maintains a persistent instance of DragDrop so
    // it's necessary to update AllowedOperation in the DxamlCore DragDrop every time
    IFC_RETURN((DXamlCore::GetCurrent()->GetDragDrop()->SetAllowedOperations(allowedOperations)));

    *ppReturnValue = spStartDragAsyncOperation.Detach();
    return S_OK;
}

//
// This is fired on the drag source UIElement indicating drop occurred
// accepted by the drop target.
//
_Check_return_ HRESULT UIElement::OnDropCompleted(
    _In_ wadt::DataPackageOperation dropStatus)
{
    DropCompletedEventSourceType* pEventSource = nullptr;
    ctl::ComPtr<DropCompletedEventArgs> spArgs;

    // Create the event args
    IFC_RETURN(ctl::make<DropCompletedEventArgs>(&spArgs));

    IFC_RETURN(spArgs->put_DropResult(dropStatus));

    // Raise the event
    IFC_RETURN(GetDropCompletedEventSourceNoRef(&pEventSource));
    IFC_RETURN(pEventSource->Raise(
        this,
        spArgs.Get()));

    return S_OK;
}

_Check_return_ HRESULT UIElement::NotifyCanDragChanged(
    _In_ CUIElement* nativeTarget,
    _In_ bool fCanDrag)
{
    ctl::ComPtr<DependencyObject> spTarget;

    IFCPTR_RETURN(nativeTarget);

    IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(nativeTarget, &spTarget));
    IFC_RETURN(spTarget.Cast<UIElement>()->ConfigureAutomaticDragHelper(!!fCanDrag));

    return S_OK;
}

_Check_return_ HRESULT
UIElement::ConfigureAutomaticDragHelper(bool startDetectingDrag)
{
    AutomaticDragHelper* helper = DXamlCore::GetCurrent()->GetAutomaticDragHelper(this);

    if (startDetectingDrag)
    {
        if (helper == nullptr)
        {
            helper = new AutomaticDragHelper(this, ShouldAutomaticDragHelperHandleInputEvents());
            DXamlCore::GetCurrent()->SetAutomaticDragHelper(this, helper);
        }
        IFC_RETURN(helper->StartDetectingDrag());
        IFC_RETURN(put_IsDirectManipulationCrossSlideContainer(true));
    }
    else
    {
        if (helper != nullptr)
        {
            IFC_RETURN(helper->StopDetectingDrag());
            DXamlCore::GetCurrent()->SetAutomaticDragHelper(this, nullptr); //remove entry from AutomaticDragHelper map
            IFC_RETURN(put_IsDirectManipulationCrossSlideContainer(false));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
UIElement::OnPointerPressed(
    _In_ IInspectable* pSender,
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    // This handler is attached/detached when CanDrag changes. It is possible that we have 
    // pending events while this happens. For example, someone is clicking repeatedly on the item
    // while CanDrag property changes. In this case, we could end up in this method and have no
    // helper associated with this element. In that case, instead of crashing we can just ignore
    // the drag action.
    if (AutomaticDragHelper* helper = DXamlCore::GetCurrent()->GetAutomaticDragHelper(this))
    {
        IFC_RETURN(helper->HandlePointerPressedEventArgs(pArgs));
    }

    return S_OK;
}

_Check_return_ HRESULT
UIElement::OnPointerMoved(
    _In_ IInspectable* pSender,
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    // This handler is attached/detached when CanDrag changes. It is possible that we have 
    // pending events while this happens. For example, someone is clicking repeatedly on the item
    // while CanDrag property changes. In this case, we could end up in this method and have no
    // helper associated with this element. In that case, instead of crashing we can just ignore
    // the drag action.
    if (AutomaticDragHelper* helper = DXamlCore::GetCurrent()->GetAutomaticDragHelper(this))
    {
        IFC_RETURN(helper->HandlePointerMovedEventArgs(pArgs));
    }

    return S_OK;
}

_Check_return_ HRESULT
UIElement::OnPointerCaptureLost(
    _In_ IInspectable* pSender,
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    // This handler is attached/detached when CanDrag changes. It is possible that we have 
    // pending events while this happens. For example, someone is clicking repeatedly on the item
    // while CanDrag property changes. In this case, we could end up in this method and have no
    // helper associated with this element. In that case, instead of crashing we can just ignore
    // the drag action.
    if (AutomaticDragHelper* helper = DXamlCore::GetCurrent()->GetAutomaticDragHelper(this))
    {
        IFC_RETURN(helper->HandlePointerCaptureLostEventArgs(pArgs));
    }

    return S_OK;
}

_Check_return_ HRESULT
UIElement::OnPointerReleased(
    _In_ IInspectable* pSender,
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    // This handler is attached/detached when CanDrag changes. It is possible that we have 
    // pending events while this happens. For example, someone is clicking repeatedly on the item
    // while CanDrag property changes. In this case, we could end up in this method and have no
    // helper associated with this element. In that case, instead of crashing we can just ignore
    // the drag action.
    if (AutomaticDragHelper* helper = DXamlCore::GetCurrent()->GetAutomaticDragHelper(this))
    {
        IFC_RETURN(helper->HandlePointerReleasedEventArgs(pArgs));
    }

    return S_OK;
}

_Check_return_ HRESULT
UIElement::OnHolding(
    _In_ IInspectable* pSender,
    _In_ xaml_input::IHoldingRoutedEventArgs* pArgs)
{
    // This handler is attached/detached when CanDrag changes. It is possible that we have 
    // pending events while this happens. For example, someone is clicking repeatedly on the item
    // while CanDrag property changes. In this case, we could end up in this method and have no
    // helper associated with this element. In that case, instead of crashing we can just ignore
    // the drag action.
    if (AutomaticDragHelper* helper = DXamlCore::GetCurrent()->GetAutomaticDragHelper(this))
    {
        IFC_RETURN(helper->HandleHoldingEventArgs(pArgs));
    }

    return S_OK;
}

// Called when the InputManager wants to let the cross-slide viewport container know
// dragging (after press and hold) has started.
/*static*/_Check_return_ HRESULT
UIElement::OnDirectManipulationDraggingStarted(
    _In_ CUIElement* nativeTarget)
{
    ctl::ComPtr<DependencyObject> spTarget;
    ctl::ComPtr<UIElement> spUIElement;

    ASSERT(nativeTarget);

    IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(nativeTarget, &spTarget));
    IFC_RETURN(spTarget.As(&spUIElement));

    // This handler is attached/detached when CanDrag changes. It is possible that we have 
    // pending events while this happens. For example, someone is clicking repeatedly on the item
    // while CanDrag property changes. In this case, we could end up in this method and have no
    // helper associated with this element. In that case, instead of crashing we can just ignore
    // the drag action.
    if (AutomaticDragHelper* helper = DXamlCore::GetCurrent()->GetAutomaticDragHelper(spUIElement.Get()))
    {
        IFC_RETURN(helper->HandleDirectManipulationDraggingStarted());
    }

    return S_OK;
}

_Check_return_ HRESULT
UIElement::OnTouchDragStarted(
    _In_ ixp::IPointerPoint* pointerPoint,
    _In_ xaml_input::IPointer* pointer)
{
    // Start drag
    ctl::ComPtr<wf::IAsyncOperation<wadt::DataPackageOperation>> spAsyncOperation;
    return StartDragAsync(pointerPoint, &spAsyncOperation);
}
