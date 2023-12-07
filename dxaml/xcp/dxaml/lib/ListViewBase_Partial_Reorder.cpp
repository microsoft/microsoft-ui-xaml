// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ListViewBase.g.h"
#include "ListViewBaseItem.g.h"
#include "AutomationPeer.g.h"
#include "ItemCollection.g.h"
#include "DragStartingEventArgs.g.h"
#include "DragItemsStartingEventArgs.g.h"
#include "DragItemsCompletedEventArgs.g.h"
#include "GroupItem.g.h"
#include "DragEventArgs.g.h"
#include "ScrollViewer.g.h"
#include "DispatcherTimer.g.h"
#include "ToolTipService.g.h"
#include "Panel.g.h"
#include "IItemLookupPanel.g.h"
#include "Window.g.h"
#include "DragDropInternal.h"
#include "DragVisual_partial.h"
#include "StartDragAsyncOperation.h"
#include <FrameworkUdk/BackButtonIntegration.h>
#include "VisualTreeHelper.h"
#include "microsoft.ui.input.dragdrop.h"

#pragma warning(disable:4267) //'var' : conversion from 'size_t' to 'type', possible loss of data

using namespace DirectUI;
using namespace DirectUISynonyms;

// Phone contstants for reorder mode.
// If user moves pointer less than s_reorderConfirmationThresholdInDips device-independent pixels
// during time s_reorderConfirmationDelayInMsec from touch-down then gesture is confirm reorderable item.
// Otherwise it is a pan (flick).
const UINT ListViewBase::s_reorderConfirmationDelayInMsec = 150;
const FLOAT ListViewBase::s_reorderConfirmationThresholdInDips = 3.0f;

#define LISTVIEWBASE_EDGE_SCROLL_EDGE_WIDTH_PX 100
#define LISTVIEWBASE_EDGE_SCROLL_START_DELAY_MSEC 50

// Edge scroll speed varies linearly with distance from edge.
// At 0 px from the edge, the speed is LISTVIEWBASE_EDGE_SCROLL_MAX_SPEED.
// At LISTVIEWBASE_EDGE_SCROLL_EDGE_WIDTH_PX from the edge, the speed is
// LISTVIEWBASE_EDGE_SCROLL_MIN_SPEED.
#define LISTVIEWBASE_EDGE_SCROLL_MIN_SPEED (150.0 /* px/sec */)
#define LISTVIEWBASE_EDGE_SCROLL_MAX_SPEED (1500.0 /* px/sec */)

// The delay time before going playing the live reorder
#define LISTVIEW_LIVEREORDER_TIMER 200
#define GRIDVIEW_LIVEREORDER_TIMER 300

// If there isn't a drag and drop in progress
// 1) fire DragItemsStart to serialize the current selection and to determine if the drag should be canceled.
// 2) If the drag isn't canceled, kick off the drag operation.
//  pointer -          The pointer used to start the new drag.
//  pOriginatingItem -  The item the user performed a drag gesture on.
//  dragStartPoint   -  the point at which the drag should start.
//                      This should line up with the user's current Pointer location.
// onDragStarting    -  Called when we've decided to actually start a drag, meaning that DragItemsStarting
//                      was fired and did not request a cancel (or drag/drop is disabled but reordering is enabled).
//                      NOTE! This doesn't mean *pWasCanceled is guaranteed to become false! We can still fail in other ways.
// pWasCanceled      -  Set if the drag was canceled or failed in some way.
_Check_return_ HRESULT ListViewBase::OnDragGesture(
    _In_ IPointer* pointer,
    _In_ ixp::IPointerPoint* pointerPoint,
    _In_ ListViewBaseItem* pOriginatingItem,
    _In_ DirectUI::DragDropVisual* pDragDropVisual,
    _In_ wf::Point dragStartPoint,
    _In_ std::function<HRESULT ()> onDragStarting,
    _Out_ BOOLEAN* pWasCanceled)
{
    HRESULT hr = S_OK;
    // Take a ref on the visual in case someone removes our dragged item inside DragItemsStarting or similar.
    ctl::ComPtr<DirectUI::DragDropVisual> spDragDropVisual = pDragDropVisual;
    ctl::ComPtr<DirectUI::ListViewBaseItem> spOriginatingItem = pOriginatingItem;

    DragDrop* dragDrop = DXamlCore::GetCurrent()->GetDragDrop();

    IFCPTR(pOriginatingItem);
    IFCPTR(pointer);
    IFCPTR(pDragDropVisual);

    *pWasCanceled = TRUE;

    // Ignore extra drag gestures during a drag
    if (!m_tpDragPointer && !dragDrop->GetIsDragDropInProgress())
    {
        BOOLEAN canDragItems = FALSE;
        BOOLEAN wasCanceled = FALSE;
        const bool useWinRtDragDrop = dragDrop->GetUseCoreDragDrop();
        mui::PointerDeviceType pointerDeviceType = mui::PointerDeviceType_Touch;

        ctl::ComPtr<wadt::IDataPackage> spDataPackage;
        ctl::ComPtr<wfc::IVector<IInspectable*>> spItems;
        ctl::ComPtr<mui::DragDrop::IDragOperation> spDragOperation;
        ctl::ComPtr<DragStartingEventArgs> spDragStartingEventArgs;
        wrl::ComPtr<StartDragAsyncOperation> spStartDragAsyncOperation;

        IFC(pointer->get_PointerDeviceType(&pointerDeviceType));

        IFC(GetDraggedItems(spOriginatingItem.Get(), &spItems, nullptr /* pItemIndices */));

        // WinRT DragDrop API does not support on phone and only works when a CoreWindow is associated with the app which is not the case when XAML Presenter
        // is in use. In those cases, we will fallback to use our own DragDropInternal API.
        if (useWinRtDragDrop)
        {
            IFC(dragDrop->CreateDragOperation(spDragOperation.GetAddressOf()));
            IFC(spDragOperation->get_Data(&spDataPackage));

            ctl::WeakRefPtr wpItem;
            IFC(ctl::AsWeak(pOriginatingItem, &wpItem));

            IFC(wrl::MakeAndInitialize<StartDragAsyncOperation>(&spStartDragAsyncOperation, wpItem, spDragOperation.Get()));
            IFC(ctl::make<DragStartingEventArgs>(&spDragStartingEventArgs));
            IFC(spDragStartingEventArgs->SetOperationContext(dragStartPoint, spStartDragAsyncOperation.Get(), spDataPackage.Get(), pointerPoint));
        }
        else
        {
            IFC(GetBlankDataPackage(&spDataPackage));
        }

        IFC(get_CanDragItems(&canDragItems));
        if (canDragItems)
        {
            // Only raise DragItemsStarting if CanDragItems == TRUE.
            IFC(OnDragItemsStarting(spItems.Get(), spDataPackage.Get(), &wasCanceled));
        }

        if (!wasCanceled)
        {
            DragStartingEventSourceType* pEventSource = nullptr;
            IFC(pOriginatingItem->GetDragStartingEventSourceNoRef(&pEventSource));
            IFC(static_cast<CDragStartingEventSource*>(pEventSource)->Raise(pOriginatingItem, spDragStartingEventArgs.Get(), false /* shouldStartOperation */));

            IFC(spDragStartingEventArgs->get_Cancel(&wasCanceled));
        }

        // Only start dragging if the handler didn't cancel the drag.
        if (!wasCanceled)
        {
            BOOLEAN gotCapture = TRUE;
            m_lastDragPosition.X = 0;
            m_lastDragPosition.Y = 0;

            // Release pointer capture on the dragged item
            IFC(onDragStarting());

            if (!useWinRtDragDrop)
            {
                 //Capture pointer on the listviewbase.
                IFC(CapturePointer(pointer, &gotCapture));
            }

            // Someone else might have already captured this pointer.
            // Only start the drag if we actually got capture. Otherwise, cancel.
            if (gotCapture)
            {
                SetPtrValue(m_tpDragPointer, pointer);
                IFC(spItems->get_Size(&m_dragItemsCount));
                SetPtrValue(m_tpPrimaryDraggedContainer, spOriginatingItem.Get());

                // Update the item's TemplateSettings.DragItemsCount with the size of our dragged items list.
                IFC(spOriginatingItem->SetDragItemsCountDisplay(m_dragItemsCount));

                IFC(NotifyDragVisualCreated(pDragDropVisual));
                m_isDraggingOverSelf = FALSE;
                IFC(dragDrop->DragStart(this, spOriginatingItem.Get(), spDataPackage.Get(), pDragDropVisual, dragStartPoint, m_dragItemsCount, !useWinRtDragDrop /* shouldFireEvent */));

                if (useWinRtDragDrop)
                {
                    ctl::WeakRefPtr wpThis;
                    UINT pointerId = 0;
                    ctl::ComPtr<DragVisual> spDragVisual;

                    IFC(ctl::AsWeak(this, &wpThis));
                    auto callbackPtr = Microsoft::WRL::Callback<wf::IAsyncOperationCompletedHandler<wadt::DataPackageOperation>>(
                            [wpThis, spItems](wf::IAsyncOperation<wadt::DataPackageOperation>* pAsyncOp, wf::AsyncStatus status) mutable
                    {
                        HRESULT hr = S_OK;
                        HRESULT cleanupHr = S_OK;
                        ctl::ComPtr<ListViewBase> spThis = wpThis.AsOrNull<IListViewBase>().Cast<ListViewBase>();
                        BOOLEAN isHandledIgnored = FALSE;

                        if(spThis)
                        {
                            IFC(spThis->OnDragCompleted(pAsyncOp, status, spItems.Get()));

                            // Calls OnDropGesture to update the state of the dragged items
                            IFC(spThis->OnDropGesture(NULL, {-1,-1}, &isHandledIgnored));
                        }

                    Cleanup:
                        // There is some specific ListView stuff in core Drag/Drop that this attempts to 
                        // clean up.  However, it is possible that this event can come in after we have
                        // shut down the core.  So make sure one exists before we try to clean up.
                        if (DXamlCore::GetCurrent()) {
                            cleanupHr = DXamlCore::GetCurrent()->GetDragDrop()->CleanupDragDrop();
                            if (SUCCEEDED(hr))
                            {
                                hr = cleanupHr;
                            }
                        }
                        RRETURN(hr);
                    });

                    IFC(pointer->get_PointerId(&pointerId));

                    mui::PointerDeviceType deviceType;
                    IFC(pointer->get_PointerDeviceType(&deviceType));
                    
                    const bool isMouse = (deviceType == mui::PointerDeviceType_Mouse);

                    AppPolicyWindowingModel policy = AppPolicyWindowingModel_None;
                    LONG status = AppPolicyGetWindowingModel(GetCurrentThreadEffectiveToken(), &policy);
                    if (status != ERROR_SUCCESS)
                    {
                        IFC(E_FAIL);
                    }

                    if (!isMouse && policy != AppPolicyWindowingModel_Universal)
                    {
                        const auto hmod = ::GetModuleHandleA("microsoft.ui.input.dll");
                        if (hmod == nullptr)
                        {
                            IFC(E_FAIL);
                        }
                        using LiftedInputCapturePointerIdForDragAndDrop = HRESULT(*)(UINT);
                        const auto captureForDragAndDrop = reinterpret_cast<LiftedInputCapturePointerIdForDragAndDrop>(::GetProcAddress(hmod, "LiftedInputCapturePointerIdForDragAndDrop"));
                        if (captureForDragAndDrop)
                        {
                            hr = captureForDragAndDrop(pointerId);

                            if (hr == E_ACCESSDENIED)
                            {
                                // This means IXP API used for touch and pen drag and drop does not support current scenario, we should cancel in progress drag and return. 
                                IGNOREHR(DXamlCore::GetCurrent()->GetDragDrop()->CleanupDragDrop());
                                RRETURN(S_OK);
                            }
                            IFC(hr);
                        }
                    }

                    // Capture the source ListViewBase to be able to get position updates from
                    // WinRT DragDrop component. This is only for M1 when setting DragUI is
                    // not supported.
                    dragDrop->SetCapturedDragSource(this);

                    // Start drag operation
                    IFC(spDragStartingEventArgs->StartOrUpdateOperation());
                    IFC(spStartDragAsyncOperation->put_Completed(callbackPtr.Get()));
                }

                m_isHolding = false;
                IFC(ChangeSelectorItemsVisualState(TRUE));
                IFC(UpdateDropTargetDropEffect(FALSE, FALSE /*isLeaving*/));
                IFC(UpdateDragGrabbedItems(FALSE));
                IFC(AutomationPeer::RaiseEventIfListener(spOriginatingItem.Get(), xaml_automation_peers::AutomationEvents_DragStart));
            }
            else
            {
                wasCanceled = TRUE;
            }
        }

        if (wasCanceled)
        {
            // make sure the holding variable is cleared
            SetIsHolding(false /* isHolding */);
            IFC(AutomationPeer::RaiseEventIfListener(spOriginatingItem.Get(), xaml_automation_peers::AutomationEvents_DragCancel));
            IFC(CompleteDrop());
        }
        else
        {
            *pWasCanceled = FALSE;
        }
    }

Cleanup:
    if (FAILED(hr))
    {
        IGNOREHR(DXamlCore::GetCurrent()->GetDragDrop()->CleanupDragDrop());
    }
    RRETURN(hr);
}

//
// This is fired on the drag source ListViewBase when WinRt DragDrop API is used.
//
_Check_return_ HRESULT ListViewBase::OnDragCompleted(
    _In_ wf::IAsyncOperation<wadt::DataPackageOperation>* pAsyncOp,
    _In_ wf::AsyncStatus status,
    _In_ wfc::IVector<IInspectable*>* pItems)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IVectorView<IInspectable*>> spItemsView;
    DragItemsCompletedEventSource* pEventSource = nullptr;
    ctl::ComPtr<DragItemsCompletedEventArgs> spArgs;
    wadt::DataPackageOperation dropResult;

    BOOLEAN canDragItems = FALSE;
    IFC(get_CanDragItems(&canDragItems));

    if (status == wf::AsyncStatus::Completed
         && canDragItems) // only raise DragItemsCompleted if DragItemsStarting was raised
    {
        // Create the event args
        IFC(ctl::make<DragItemsCompletedEventArgs>(&spArgs));

        IFC(pItems->GetView(&spItemsView));
        IFC(spArgs->put_Items(spItemsView.Get()));

        IFC(pAsyncOp->GetResults(&dropResult));
        IFC(spArgs->put_DropResult(dropResult));

        // Raise the event
        IFC(GetDragItemsCompletedEventSourceNoRef(&pEventSource));
        IFC(pEventSource->Raise(
            this,
            spArgs.Get()));
    }

    // stop the timer
    IFC(StopLiveReorderTimer());

    // reset the moved items vector
    m_movedItems.clear();

Cleanup:
    RRETURN(hr);
}


//
// Called by DropTargetOperation directly on the currently captured drop target ListViewBase
// to ensure that the visual position gets updated even though the drag happens outside the
// ListViewBase that originate the drag.
//
_Check_return_ HRESULT ListViewBase::UpdateDragPositionOnCapturedTarget(
    _In_ DragDropMessageType type,
    _In_ wf::Point dragPoint)
{
    HRESULT hr = S_OK;
    BOOLEAN ignored;
    ctl::ComPtr<xaml_media::IGeneralTransform> spTransformFromRoot;
    ctl::ComPtr<xaml_media::IGeneralTransform> spTransformToRoot;

    IFC(TransformToVisual(NULL, &spTransformToRoot));
    IFC(spTransformToRoot->get_Inverse(&spTransformFromRoot));

    IFC(spTransformFromRoot->TransformPoint(dragPoint, &dragPoint));

    switch(type)
    {
    case DragDropMessageType::DragEnter:
    case DragDropMessageType::DragOver:
        IFC(OnDragMoveGesture(nullptr /*PointerId*/, dragPoint, &ignored));
        break;
    case DragDropMessageType::DragLeave:
        break;
    case DragDropMessageType::Drop:
        // this drop action happens outside the current listview
        IFC(OnDropGesture(nullptr /*PointerId*/, dragPoint, &ignored));
        break;
    default:
        ASSERT(FALSE);
    }

Cleanup:
    RRETURN(hr);
}

// Called when the user moves the drag location.
// If the current drag modality isn't the same as the provided modality,
// do nothing.
//  modality   - The input modality used to indicate the drop.
//  dragPoint  - The point the user moved to. Relative to this ListViewBase.
//  pIsHandled - Set to FALSE if the gesture was ignored due to input modality mismatch, set to TRUE otherwise.
_Check_return_ HRESULT ListViewBase::OnDragMoveGesture(
    _In_opt_ xaml_input::IPointer* pointer,
    _In_ wf::Point dragPoint,
    _Out_ BOOLEAN *pIsHandled)
{
    HRESULT hr = S_OK;
    BOOLEAN shouldAccept = FALSE;

    IFCPTR(pIsHandled);

    *pIsHandled = FALSE;

    IFC(ShouldAcceptDragInput(pointer, &shouldAccept));

    *pIsHandled = shouldAccept;

    if (shouldAccept)
    {
        if (m_tpDragDropVisual.Cast<DirectUI::DragDropVisual>()->IsShowing())
        {
            IFC(DXamlCore::GetCurrent()->GetDragDrop()->DragMove(dragPoint, &m_lastDragPosition, pointer != NULL /* shouldFireEvent */));
        }
        else
        {
            // Our visual is no longer showing (usually because someone removed our drag visual template
            // part from the tree). Cancel the drag.
            IFC(CancelDrag());
        }
    }

Cleanup:
    RRETURN(hr);
}

// Only one Pointer can drive drag and drop operations at a time.
// This method returns TRUE if there is no current drag and drop operation,
// or the given IPointer is the pointer currently in charge of a drag
// and drop operation.
_Check_return_ HRESULT ListViewBase::ShouldAcceptDragInput(
    _In_opt_ xaml_input::IPointer* pointer,
    _Out_ BOOLEAN* shouldAccept)
{
    HRESULT hr = S_OK;

    IFCPTR(shouldAccept);

    *shouldAccept = FALSE;

    if (m_tpDragPointer)
    {
        if (pointer == NULL)
        {
            // Since only Mouse drag is supported using WinRT drag and drop API in M1,
            // if m_tpDragPointer is set, return TRUE mouse always produces the same
            // pointer id.
            *shouldAccept = TRUE;
        }
        else
        {
            IFC(AreSamePointer(m_tpDragPointer.Get(), pointer, shouldAccept));
        }
    }
Cleanup:
    RRETURN(hr);
}

// Returns TRUE if the given IPointers refer to the same physical device.
_Check_return_ HRESULT ListViewBase::AreSamePointer(
    _In_ xaml_input::IPointer* pPointerA,
    _In_ xaml_input::IPointer* pPointerB,
    _Out_ BOOLEAN* pAreSame)
{
    HRESULT hr = S_OK;
    UINT idPointerA = 0;
    UINT idPointerB = 1;

    IFCPTR(pPointerA);
    IFCPTR(pPointerB);
    IFCPTR(pAreSame);

    *pAreSame = FALSE;

    IFC(pPointerA->get_PointerId(&idPointerA));
    IFC(pPointerB->get_PointerId(&idPointerB));
    *pAreSame = (idPointerA == idPointerB);

Cleanup:
    RRETURN(hr);
}
// Called when the user indicates the dragged item should be dropped.
// If the current drag modality isn't the same as the provided modality,
// do nothing.
//  pointer    - The input pointer used to indicate the drop.
//  pIsHandled  - Set to FALSE if the gesture was ignored due to input modality mismatch, set to TRUE otherwise.
_Check_return_ HRESULT ListViewBase::OnDropGesture(
    _In_opt_ xaml_input::IPointer* pointer,
    _In_ const wf::Point& dropPoint,
    _Out_ BOOLEAN *pIsHandled)
{
    HRESULT hr = S_OK;
    BOOLEAN shouldAccept = FALSE;

    IFCPTR(pIsHandled);

    *pIsHandled = FALSE;

    IFC(ShouldAcceptDragInput(pointer, &shouldAccept));

    *pIsHandled = shouldAccept;

    if (shouldAccept)
    {
        ctl::ComPtr<xaml_input::IPointer> spLocalDragPointer;
        BOOLEAN dropCausesReorder = FALSE;

        // We must clear m_tpDragPointer before calling ReleasePointerCapture,
        // otherwise we risk re-entering this block (PointerCaptureLost is sync).
        // ShouldAcceptDragInput will not return TRUE if m_tpDragPointer is NULL.
        spLocalDragPointer = m_tpDragPointer.Get();
        m_tpDragPointer.Clear();
        IFC(ReleasePointerCapture(spLocalDragPointer.Get()));
        IFC(NotifyDragVisualReleased());
        ASSERT(m_tpPrimaryDraggedContainer != nullptr);
        IFC(DXamlCore::GetCurrent()->GetDragDrop()->Drop(pointer != NULL /* shouldFireEvent */));
        IFC(UpdateDragGrabbedItems(FALSE));
        IFC(DropCausesReorder(&dropCausesReorder));

        if (!dropCausesReorder)
        {
            IFC(UpdateDropTargetDropEffect(FALSE, FALSE /*isLeaving*/));
            IFC(CompleteDrop());
        }
        else if (pointer != NULL)
        {
            // Phone reordering mode relies on a call to ListViewBase::IsInDragDrop() to reject manipulations while D&D is in progress.
            // This method uses presence of primary dragged container to determine whether operation is in progress.
            // In most cases ListViewBase::OnDropGesture is called from either OnPointerReleased or OnPointerCaptureLost AFTER ListViewBase::OnDragLeave,
            // which sets a flag m_isDraggingOverSelf which determines whether cleanup in OnDropGesture is executed.
            // In a case when user quickly flicks an item out of ListView, the order of events is reversed and primary dragged container is not cleared.
            // This in turn will ignore all following reordering requests, as ListView will be still in dragging operation.
            // This code detects if item is dragged outside of ListView but OnDragLeave has not executed and clears primary dragged container.

            ctl::ComPtr<wfc::IIterable<xaml::UIElement*>> spElements;
            ctl::ComPtr<wfc::IIterator<xaml::UIElement*>> spIterator;
            BOOLEAN hasCurrent = FALSE;
            bool foundThis = false;

            IFC(VisualTreeHelper::FindElementsInHostCoordinatesPointStatic(
                dropPoint,
                nullptr,
                VisualTreeHelper::c_canHitDisabledElementsDefault,
                VisualTreeHelper::c_canHitInvisibleElementsDefault,
                &spElements));

            IFC(spElements->First(&spIterator));
            IFC(spIterator->get_HasCurrent(&hasCurrent));

            while (hasCurrent &&
                    !foundThis)
            {
                ctl::ComPtr<xaml::IUIElement> spElement;

                IFC(spIterator->get_Current(&spElement));

                IFC(ctl::are_equal(
                    ctl::iinspectable_cast(this),
                    ctl::iinspectable_cast(spElement.Get()),
                    &foundThis));

                IFC(spIterator->MoveNext(&hasCurrent));
            }

            if (!foundThis)
            {
                // Reorder hint items will be cleared in OnDragLeave.
                IFC(ClearPrimaryDragContainer());
            }
        }
    }
    else if (m_isDraggingOverSelf && m_tpDragPointer)
    {
        // if m_tpDragPointer is NULL, it means that we have already handled
        // the DropGesture in the upper part of this function
        // in that case, we do not want to clear the variables as the
        // drop may still be using them
        // note that the DropGesture will clear all these variables itself

        // Drop has happened on a descendant, we have to cleanup the D&D
        m_tpDragPointer.Clear();
        IFC(NotifyDragVisualReleased());
        IFC(ClearPrimaryDragContainer());
        IFC(SetPendingAutoPanVelocity(PanVelocity::Stationary()));
        IFC(ChangeSelectorItemsVisualState(TRUE));
        m_isDraggingOverSelf = FALSE;
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ListViewBase::ClearPrimaryDragContainer()
{
    if (m_tpPrimaryDraggedContainer)
    {
        // Update our item's TemplateSettings with a zero DragItemsCount.
        IFC_RETURN(m_tpPrimaryDraggedContainer.Cast<ListViewBaseItem>()->SetDragItemsCountDisplay(0));
        m_tpPrimaryDraggedContainer.Cast<ListViewBaseItem>()->ClearLastTouchPressedArgs();
        m_tpPrimaryDraggedContainer.Clear();
    }

    m_dragItemsCount = 0;

    return S_OK;
}

// Complete a drop gesture.
_Check_return_ HRESULT ListViewBase::CompleteDrop()
{
    HRESULT hr = S_OK;

    IFC(SetPendingAutoPanVelocity(PanVelocity::Stationary()));

    if (m_tpPrimaryDraggedContainer)
    {
        ctl::ComPtr<ListViewBaseItem> spPrimaryDraggedContainer = m_tpPrimaryDraggedContainer.Cast<ListViewBaseItem>();

        IFC(ClearPrimaryDragContainer());
        IFC(AutomationPeer::RaiseEventIfListener(spPrimaryDraggedContainer.Get(), xaml_automation_peers::AutomationEvents_DragComplete));
        IFC(AutomationPeer::RaiseEventIfListener(this, xaml_automation_peers::AutomationEvents_Dropped));
    }

    m_tpDragPointer.Clear();
    IFC(ChangeSelectorItemsVisualState(TRUE));

    // stop the timer
    IFC(StopLiveReorderTimer());

    // reset live reorder variables
    m_liveReorderIndices = { -1, -1, -1 };

    // reset the moved items vector
    m_movedItems.clear();

Cleanup:
    RRETURN(hr);
}

// Builds one or both of:
// - a vector containing the data items that should be dragged. This is the vector that will be passed as the Items property in the DragItemsStartingEvent args.
// - a list containing the current indexes of said items. Not that this isn't ref-counted - it's a std::list.
_Check_return_ HRESULT ListViewBase::GetDraggedItems(
    _In_ ListViewBaseItem* pOriginatingItem,
    _Outptr_opt_ wfc::IVector<IInspectable*>** ppItems,
    _Out_opt_ std::vector<UINT>* pItemIndices)
{
    typedef std::vector<UINT> ContainerType;

    HRESULT hr = S_OK;
    BOOLEAN isOriginatingItemSelected = FALSE;
    INT originatingItemIndex = 0;
    ContainerType selectedItemIndices;

    IFCPTR(pOriginatingItem);

    if (ppItems != nullptr)
    {
        *ppItems = nullptr;
    }

    IFC(pOriginatingItem->get_IsSelected(&isOriginatingItemSelected));
    IFC(IndexFromContainer(pOriginatingItem, &originatingItemIndex));

    // If the item belongs to the selection, we reorder all selected items
    if (isOriginatingItemSelected)
    {
        ContainerType::iterator originatingItem;

        selectedItemIndices = std::move(m_selection.GetSelectedIndexes());

        // Find the item's index in the selected indices to be removed
        originatingItem = std::find(
            begin(selectedItemIndices),
            end(selectedItemIndices),
            originatingItemIndex);

        if (originatingItem != end(selectedItemIndices))
        {
            // remove the dragged item from the selected indices
            // that way, we can add the first item on its own
            selectedItemIndices.erase(originatingItem);
        }
        else
        {
            // making sure the value is -1
            ASSERT(originatingItemIndex == -1);
        }
    }

    // if the dragged item was removed after the drag has started, it won't belong to the item collection anymore
    // and originatingItemIndex will be -1
    // if not, we insert its index to the front of the array of indices
    if (originatingItemIndex != -1)
    {
        selectedItemIndices.insert(begin(selectedItemIndices), originatingItemIndex);
    }

    if (ppItems != nullptr)
    {
        ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;
        ctl::ComPtr<wfc::IVector<IInspectable*>> spGeneratedItemsList;
        ctl::ComPtr<TrackerCollection<IInspectable*>> spGeneratedItemsListImpl;

        // create the list
        IFC(ctl::make<TrackerCollection<IInspectable*>>(&spGeneratedItemsListImpl));
        spGeneratedItemsList = spGeneratedItemsListImpl;

        IFC(get_Items(&spItems));

        // add the dragged items
        for (auto itemIndex : selectedItemIndices)
        {
            ctl::ComPtr<IInspectable> spItem;
            BOOLEAN isItemOwnContainer = FALSE;

            IFC(spItems.Cast<ItemCollection>()->GetAt(itemIndex, &spItem));

            // Do a quick check to see if the item is its own container before proceeding.
            IFC(IsItemItsOwnContainer(spItem.Get(), &isItemOwnContainer));

            if (isItemOwnContainer)
            {
                // Item was a container. Add its data to the dragged items list.
                // Note that we're assuming our containers are at least IContentControls. Drag/drop doesn't work
                // with other containers, so it's reasonable to make this assumption. if the customer makes an odd
                // ICG that mixes container types, we simply won't drag items that are not IContentControls.

                ctl::ComPtr<IContentControl> spItemAsICC = spItem.AsOrNull<IContentControl>();

                if (spItemAsICC)
                {
                    ctl::ComPtr<IInspectable> spItemData;

                    // Try to get the data from the container.
                    IFC(spItemAsICC->get_Content(&spItemData));

                    IFC(spGeneratedItemsList->Append(spItemData.Get()));
                }
            }
            else
            {
                // Item wasn't a container. Go ahead and add to the dragged items list.
                IFC(spGeneratedItemsList->Append(spItem.Get()));
            }
        }

        IFC(spGeneratedItemsList.CopyTo(ppItems));
    }

    if (pItemIndices != nullptr)
    {
        *pItemIndices = std::move(selectedItemIndices);
    }

Cleanup:
    RRETURN(hr);
}

// Raises the DragItemsStarting event to let the handler populate
// the DataPackage and to determine whether or not the drag
// was canceled by the handler.
_Check_return_ HRESULT ListViewBase::OnDragItemsStarting(
    _In_ wfc::IVector<IInspectable*>* pDraggedItems,
    _In_ wadt::IDataPackage* pData,
    _Out_ BOOLEAN* pWasCanceled)
{
    HRESULT hr = S_OK;

    DragItemsStartingEventSource* pEventSource = nullptr;
    ctl::ComPtr<DragItemsStartingEventArgs> spArgs;

    IFCPTR(pWasCanceled);
    IFCPTR(pData);
    IFCPTR(pDraggedItems);

    *pWasCanceled = FALSE;

    // Create the args
    IFC(ctl::make<DragItemsStartingEventArgs>(&spArgs));

    IFC(spArgs->put_Items(pDraggedItems));
    IFC(spArgs->put_Data(pData));

    // Raise the event
    IFC(GetDragItemsStartingEventSourceNoRef(&pEventSource));
    IFC(pEventSource->Raise(
        ctl::as_iinspectable(this),
        spArgs.Get()));

    // Return the results of the event.
    IFC(spArgs->get_Cancel(pWasCanceled));

Cleanup:
    RRETURN(hr);
}

// Sets pResult to TRUE if
// 1) a drag and drop operation is in progress, and
// 2a) the given item is the item the user is physically dragging
//    (as opposed to just part of the selection), or
// 2b) the given item is the owning GroupItem for the item the user is physically dragging.
// Sets pResult to FALSE otherwise.
_Check_return_ HRESULT ListViewBase::IsContainerDragDropOwner(
    _In_ IUIElement* pContainer,
    _Out_ BOOLEAN* pResult)
{
    HRESULT hr = S_OK;

    IFCPTR(pContainer);
    IFCPTR(pResult);

    *pResult = FALSE;

    if (m_tpPrimaryDraggedContainer)
    {
        bool isPrimaryDraggedContainer = false;

        // pContainer could either be a real item, or it could be a GroupItem.
        IFC(PropertyValue::AreEqual(pContainer, m_tpPrimaryDraggedContainer.Get(), &isPrimaryDraggedContainer));
        if (isPrimaryDraggedContainer)
        {
            // Normal item.
            *pResult = TRUE;
        }
        else
        {
            // Possibly group item.
            ctl::ComPtr<IGroupItem> spGroupItem = ctl::query_interface_cast<IGroupItem, IUIElement>(pContainer);
            if (spGroupItem)
            {
                // See if the dragged container is within the group.
                INT groupIndex = -1;
                UINT groupLeftIndex = 0;
                UINT groupSize = 0;
                BOOLEAN foundGroup = FALSE;

                IFC(spGroupItem.Cast<GroupItem>()->GetGroupIndex(&groupIndex));

                if (groupIndex >= 0)
                {
                    IFC(GetGroupInformation(
                        groupIndex,
                        &groupLeftIndex,
                        &groupSize,
                        &foundGroup));
                }

                // We expect the given GroupItem to still be in our collection.
                // If this isn't the case, let the container be recycled.
                ASSERT(foundGroup);
                if (foundGroup)
                {
                    INT draggedItemIndex = 0;
                    IFC(IndexFromContainer(m_tpPrimaryDraggedContainer.Cast<ListViewBaseItem>(), &draggedItemIndex));

                    // Check to see if the item is in the group.
                    *pResult = (static_cast<UINT>(draggedItemIndex) >= groupLeftIndex) && (static_cast<UINT>(draggedItemIndex) < groupLeftIndex + groupSize);
                }
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

// Returns TRUE if a drag and drop is in progress.
BOOLEAN ListViewBase::IsInDragDrop()
{
    return m_tpPrimaryDraggedContainer;
}

// If a drag and drop operation is in progress, returns the number of items being dragged.
// Otherwise, returns 0.
UINT ListViewBase::DragItemsCount()
{
    ASSERT( (m_dragItemsCount != 0) == IsInDragDrop() );
    return m_dragItemsCount;
}

// Called when the user drags into this ListViewBase.
IFACEMETHODIMP ListViewBase::OnDragEnter(
    _In_ xaml::IDragEventArgs* pArgs)
{
    BOOLEAN dropCausesReorder = FALSE;

    IFCPTR_RETURN(pArgs);

    m_isDraggingOverSelf = (m_tpPrimaryDraggedContainer.Get() != NULL);

    IFC_RETURN(DropCausesReorder(&dropCausesReorder));
    if (dropCausesReorder)
    {
        ctl::ComPtr<IDragUIOverride> spDragUIOverride;

        IFC_RETURN(pArgs->get_DragUIOverride(&spDragUIOverride));

        // by default ListView doesn't show a glyph and a caption
        if (spDragUIOverride)
        {
            IFC_RETURN(spDragUIOverride->put_IsCaptionVisible(false));
            IFC_RETURN(spDragUIOverride->put_IsGlyphVisible(false));
        }

        // isHandled == True means that we are reordering
        IFC_RETURN(pArgs->put_Handled(TRUE));
    }

    IFC_RETURN(UpdateDropTargetDropEffect(FALSE, FALSE /*isLeaving*/));
    IFC_RETURN(ListViewBaseGenerated::OnDragEnter(pArgs));
    IFC_RETURN(AutomationPeer::RaiseEventIfListener(this, xaml_automation_peers::AutomationEvents_DragEnter));

    return S_OK;
}

// Called when the user drags out of this ListViewBase.
IFACEMETHODIMP ListViewBase::OnDragLeave(
    _In_ xaml::IDragEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN dropCausesReorder = FALSE;

    IFCPTR(pArgs);

    IFC(DropCausesReorder(&dropCausesReorder));
    if (dropCausesReorder)
    {
        IFC(pArgs->put_Handled(TRUE));
    }

    IFC(SetPendingAutoPanVelocity(PanVelocity::Stationary()));

    IFC(ListViewBaseGenerated::OnDragLeave(pArgs));

    // stop the timer
    IFC(StopLiveReorderTimer());

    // reset live reorder variables
    IFC(ResetAllItemsForLiveReorder());

    m_isDraggingOverSelf = FALSE;
    IFC(AutomationPeer::RaiseEventIfListener(this, xaml_automation_peers::AutomationEvents_DragLeave));
    IFC(UpdateDropTargetDropEffect(FALSE, TRUE /*isLeaving*/));

Cleanup:
    RRETURN(hr);
}

// Processes a drag over event at the given position (relative to the panel).
_Check_return_ HRESULT ListViewBase::ProcessDragOverAt(
    _In_ wf::Point dragPoint,
    _Out_opt_ BOOLEAN* pHandled)
{
    BOOLEAN isHandled = FALSE;

    if (pHandled)
    {
        *pHandled = FALSE;
    }

    // only play live reoder if DragOverItem is null and the accepted operation is not null
    // if the item is not null, that means we are in a folder scenario
    BOOLEAN playLiveReorder = FALSE;

    IFC_RETURN(ShouldPlayLiveReorder(&playLiveReorder));

    if (playLiveReorder)
    {
        ctl::ComPtr<IPanel> spItemsHost;
        ctl::ComPtr<IItemLookupPanel> spItemsHostAsItemLookupPanel;

        IFC_RETURN(get_ItemsHost(&spItemsHost));
        spItemsHostAsItemLookupPanel = spItemsHost.AsOrNull<IItemLookupPanel>();

        // We need the IItemLookupPanel to do reorder and drop into folder
        if (spItemsHostAsItemLookupPanel)
        {
            int draggedIndex = 0;
            int insertionIndex = -1;
            int dragOverIndex = -1;
            int previousDragOverIndex = m_liveReorderIndices.draggedOverIndex;
            int itemsCount = 0;
            xaml_primitives::ElementInfo closestElementInfo = { -1, FALSE };

            IFC_RETURN(spItemsHostAsItemLookupPanel->GetClosestElementInfo(dragPoint, &closestElementInfo));

            // save the dragOverIndex
            dragOverIndex = closestElementInfo.m_childIndex;

            IFC_RETURN(GetItemsCount(&itemsCount));

            // if Reordering from within the list, this will not be null and we can get the dragged index
            if (m_tpPrimaryDraggedContainer)
            {
                ctl::ComPtr<xaml::IDependencyObject> spDraggedContainerAsDO;

                IFC_RETURN(m_tpPrimaryDraggedContainer.As(&spDraggedContainerAsDO));
                IFC_RETURN(IndexFromContainer(spDraggedContainerAsDO.Get(), &draggedIndex));
            }
            else
            {
                // If Dragging over from outside the ListView, set the draggedIndex to be the size of the elements
                draggedIndex = itemsCount;
            }

            // this is the first time we enter this code
            if (previousDragOverIndex == -1)
            {
                previousDragOverIndex = draggedIndex;
            }

            // get the index of insertion in the panel
            IFC_RETURN(spItemsHostAsItemLookupPanel->GetInsertionIndex(dragPoint, &insertionIndex));

            if (draggedIndex == itemsCount && insertionIndex == itemsCount - 1)
            {
                ctl::ComPtr<ListViewBaseItem> spLastElementAsLVBI;
                ctl::ComPtr<xaml::IDependencyObject> spLastElementAsDO;

                IFC_RETURN(ContainerFromIndex(insertionIndex, &spLastElementAsDO));
                spLastElementAsLVBI = spLastElementAsDO.AsOrNull<ListViewBaseItem>();

                if (spLastElementAsLVBI)
                {
                    bool inBottomHalf = false;
                    wf::Point dragPointRelativeToLastItem = { 0, 0 };
                    xaml_controls::Orientation panelLogicalOrientation = xaml_controls::Orientation_Vertical;

                    ctl::ComPtr<xaml_media::IGeneralTransform> spTransform;
                    ctl::ComPtr<IUIElement> spItemsHostAsUIE = spItemsHost.AsOrNull<IUIElement>();

                    // Get the dragpoint position relative to the item
                    // First get the item's location relative to the panel
                    // Then calculate the difference to get the location relative to the item
                    IFC_RETURN(spLastElementAsLVBI->TransformToVisual(spItemsHostAsUIE.Get(), &spTransform));
                    IFC_RETURN(spTransform->TransformPoint(dragPointRelativeToLastItem, &dragPointRelativeToLastItem));

                    dragPointRelativeToLastItem.X = dragPoint.X - dragPointRelativeToLastItem.X;
                    dragPointRelativeToLastItem.Y = dragPoint.Y - dragPointRelativeToLastItem.Y;

                    // Get the panel's logical orientation
                    // We need this to know the orientation of the folder's regions
                    IFC_RETURN(GetPanelOrientations(spItemsHost.Get(), nullptr /*pPhysicalOrientation*/, &panelLogicalOrientation));

                    // Check to see if the item should go into a DragOver state
                    IFC_RETURN(spLastElementAsLVBI->IsInBottomHalfForExternalReorder(dragPointRelativeToLastItem, panelLogicalOrientation, &inBottomHalf));

                    if (inBottomHalf)
                    {
                        insertionIndex = itemsCount;
                    }
                }
            }

            if (insertionIndex == itemsCount)
            {
                dragOverIndex = itemsCount;
            }
            else
            {
                // get the drag over index
                // this function returns the right dragover index based on the direction of the drag
                dragOverIndex = DirectUI::Components::LiveReorderHelper::MovedItems::GetDragOverIndex(dragOverIndex, insertionIndex, previousDragOverIndex);
            }

            // save all the information
            m_liveReorderIndices = { draggedIndex, dragOverIndex, itemsCount };

            // we only want to reset/start a new timer when there is a new dragOverIndex
            // this will allow the LiveReorder to happen when the drag is happening in the Reorder zone
            // note that folder/drop-into scenario is handled in ListViewBaseItem
            if (previousDragOverIndex == draggedIndex || previousDragOverIndex != dragOverIndex)
            {
                IFC_RETURN(StartLiveReorderTimer());
            }

            isHandled = TRUE;
        }
    }

    IFC_RETURN(UpdateDropTargetDropEffect(FALSE, FALSE /*isLeaving*/));

    // return the handled value
    if (pHandled)
    {
        *pHandled = isHandled;
    }

    return S_OK;
}

// Called when the user drags over this ListViewBase.
IFACEMETHODIMP ListViewBase::OnDragOver(
    _In_ xaml::IDragEventArgs* pArgs)
{
    BOOLEAN isHandled = FALSE;

    IFCPTR_RETURN(pArgs);

    m_dragAcceptedOperation = wadt::DataPackageOperation_None;

    // We want the developer to handle the drag over first to set the AcceptedOperation
    IFC_RETURN(ListViewBaseGenerated::OnDragOver(pArgs));

    IFC_RETURN(pArgs->get_AcceptedOperation(&m_dragAcceptedOperation));

    IFC_RETURN(static_cast<DragEventArgs*>(pArgs)->get_Handled(&isHandled));

    if (!isHandled)
    {
        ctl::ComPtr<IPanel> spItemsHost;

        IFC_RETURN(get_ItemsHost(&spItemsHost));
        if (spItemsHost)
        {
            BOOLEAN dropCausesReorder = FALSE;
            PanVelocity panVelocity;
            wf::Point dragPointRelativeToLVB = { 0, 0 };
            ctl::ComPtr<IUIElement> spItemsHostAsUIE;

            IFC_RETURN(spItemsHost.As<IUIElement>(&spItemsHostAsUIE));

            IFC_RETURN(pArgs->GetPosition(spItemsHostAsUIE.Get(), &m_lastDragOverPoint));
            IFC_RETURN(ProcessDragOverAt(m_lastDragOverPoint, &isHandled));
            IFC_RETURN(pArgs->put_Handled(isHandled));

            IFC_RETURN(DropCausesReorder(&dropCausesReorder));

            if (dropCausesReorder)
            {
                // isHandled == True means that we are reordering
                IFC_RETURN(pArgs->put_AcceptedOperation(wadt::DataPackageOperation_Move));
            }

            // See what our edge scrolling action should be...
            IFC_RETURN(pArgs->GetPosition(this, &dragPointRelativeToLVB));
            IFC_RETURN(ComputeEdgeScrollVelocity(dragPointRelativeToLVB, &panVelocity));
            // And request it.
            IFC_RETURN(SetPendingAutoPanVelocity(panVelocity));
        }
    }

    return S_OK;
}

// Called when the user drops onto this ListViewBase.
IFACEMETHODIMP ListViewBase::OnDrop(
    _In_ xaml::IDragEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN isHandled = FALSE;

    IFCPTR(pArgs);

    IFC(static_cast<DragEventArgs*>(pArgs)->get_Handled(&isHandled));

    if (!isHandled)
    {
        BOOLEAN dropCausesReorder = FALSE;

        IFC(DropCausesReorder(&dropCausesReorder));
        if (dropCausesReorder)
        {
            ctl::ComPtr<IPanel> spItemsHost;
            ctl::ComPtr<IUIElement> spItemsHostAsUIE;
            wf::Point dropPointRelativeToPanel = { 0, 0 };

            IFC(get_ItemsHost(&spItemsHost));
            IFC(spItemsHost.As<IUIElement>(&spItemsHostAsUIE));
            IFC(pArgs->GetPosition(spItemsHostAsUIE.Get(), &dropPointRelativeToPanel));
            IFC(OnReorderDrop(dropPointRelativeToPanel));

            IFC(pArgs->put_AcceptedOperation(wadt::DataPackageOperation::DataPackageOperation_Move));
            IFC(pArgs->put_Handled(TRUE));
        }
        else
        {
            IFC(ResetAllItemsForLiveReorder());
        }
    }

    IFC(ListViewBaseGenerated::OnDrop(pArgs));

    IFC(CompleteDrop());

Cleanup:
    RRETURN(hr);
}

// Cancels the current drag/drop operation.
_Check_return_ HRESULT ListViewBase::CancelDrag()
{
    HRESULT hr = S_OK;

    if (IsInDragDrop())
    {
        IFC(NotifyDragVisualReleased());
        DragDrop* dragDrop = DXamlCore::GetCurrent()->GetDragDrop();
        if (dragDrop->GetIsDragDropInProgress())
        {
            IFC(dragDrop->DragCancel(TRUE /* shouldFireEvent */));
        }
        IFC(UpdateDragGrabbedItems(FALSE));
        IFC(UpdateDropTargetDropEffect(FALSE, TRUE /*isLeaving*/));
        IFC(CompleteDrop());
    }

Cleanup:
    RRETURN(hr);
}

// Returns TRUE if reordering is active, meaning:
// * CanReorderItems is TRUE
// * IsGrouping is FALSE
// * ItemsHost is an IItemLookupPanel
_Check_return_ HRESULT ListViewBase::IsItemReorderingActive(
    _Out_ BOOLEAN* pResult)
{
    HRESULT hr = S_OK;

    *pResult = FALSE;

    BOOLEAN canReorderItems = FALSE;
    IFC(get_CanReorderItems(&canReorderItems));

    if (canReorderItems)
    {
        IFC(IsNotGroupingAndHostIsLookupPanel(pResult));
    }

Cleanup:
    RRETURN(hr);
}

// Returns TRUE if:
// * IsGrouping is FALSE
// * ItemsHost is an IItemLookupPanel
_Check_return_ HRESULT ListViewBase::IsNotGroupingAndHostIsLookupPanel(
    _Out_ BOOLEAN* pResult)
{
    HRESULT hr = S_OK;
    BOOLEAN isGrouping = FALSE;

    *pResult = FALSE;

    IFC(get_IsGrouping(&isGrouping));

    if (!isGrouping)
    {
        // To match pointer reordering, only allow the action if our panel is an IItemLookupPanel,
        // even though it's technically possible to do keyboard reordering without this interface.
        ctl::ComPtr<IPanel> spItemsHost;
        IFC(get_ItemsHost(&spItemsHost));
        *pResult = !!ctl::value_is<IItemLookupPanel>(spItemsHost.Get());
    }

Cleanup:
    RRETURN(hr);
}

// Returns TRUE if reordering is enabled. This means CanReorderItems is TRUE,
// AllowDrop is TRUE, we're dragging over ourselves, we're not grouping, and our ItemsHost is an IItemLookupPanel.
_Check_return_ HRESULT ListViewBase::DropCausesReorder(
    _Out_ BOOLEAN* pResult)
{
    HRESULT hr = S_OK;

    IFCPTR(pResult);

    *pResult = FALSE;

    if (m_isDraggingOverSelf)
    {
        BOOLEAN allowDrop = FALSE;
        IFC(get_AllowDrop(&allowDrop));
        if (allowDrop)
        {
            IFC(IsItemReorderingActive(pResult));
        }
    }

Cleanup:
    RRETURN(hr);
}

// Moves the currently dragged items to the given point.
// Depends on ItemsHost being an IItemLookupPanel.
//  dropPoint   - The point where reordered items should be inserted, relative to this ListViewBase.
_Check_return_ HRESULT ListViewBase::OnReorderDrop(
    _In_ wf::Point dropPoint)
{
    HRESULT hr = S_OK;

    ctl::ComPtr<IPanel> spItemsHost;
    ctl::ComPtr<IItemLookupPanel> spItemsHostAsItemLookupPanel;

    IFC(get_ItemsHost(&spItemsHost));
    spItemsHostAsItemLookupPanel = spItemsHost.AsOrNull<IItemLookupPanel>();

    // We need the IItemLookupPanel interface in order to find the reorder insertion index.
    if (spItemsHostAsItemLookupPanel != NULL)
    {
        // The index at which reordered items should be inserted.
        INT insertIndex = -1;
        INT draggedItemIndex = -1;
        INT focusedIndex = m_iFocusedIndex;

        // The indexes of the items we'll be reordering.
        std::vector<UINT> reorderItemIndexes;

        IFC(GetDraggedItems(m_tpPrimaryDraggedContainer.Cast<ListViewBaseItem>(), nullptr /* ppItems */, &reorderItemIndexes));

        insertIndex = GetInsertionIndexForLiveReorder();
        m_liveReorderIndices = { -1, -1, -1 };

        // insertIndex can be -1 if either the app is a Pre-Redstone app
        // or a Redstone app with the drop happening right after an edge scroll
        // but before we update the indices to play the live reorder
        if (insertIndex == -1)
        {
            // Ask our ItemsHost where the dropped items should go.
            IFC(spItemsHostAsItemLookupPanel->GetInsertionIndex(dropPoint, &insertIndex));
        }

        // ReorderItemsTo expects the index list to be sorted in increasing order. Do that.
        std::sort(reorderItemIndexes.begin(), reorderItemIndexes.end());

        // Get the index of item being dragged
        IFC(IndexFromContainer(m_tpPrimaryDraggedContainer.Cast<ListViewBaseItem>(), &draggedItemIndex));

        // Dragged item should be realized
        ASSERT(draggedItemIndex >= 0);

        // Store the last reorder position so we can animate the dropped item into position.
        m_lastReorderPosition = m_lastDragPosition;

        // Do the reorder.
        IFC(ReorderItemsTo(
            &reorderItemIndexes,
            insertIndex,
            &draggedItemIndex,
            &focusedIndex));

        // Force a layout so ScrollIntoView knows where to scroll to.
        IFC(UpdateLayout());

        // Check if one of the dragged items is focused.
        // Note, search is performed on index BEFORE update.
        if (focusedIndex >= 0 &&
            std::binary_search(
                begin(reorderItemIndexes),
                end(reorderItemIndexes),
                static_cast<UINT>(m_iFocusedIndex)))
        {
            // If focused item was dragged, set the focus and scroll it into view.
            // Since this code is only called for pointer input, we can set the FocusState appropriately
            IFC(SetFocusedItem(
                focusedIndex,
                TRUE, // shouldScrollIntoView
                TRUE, // forceFocus
                xaml::FocusState_Pointer));
        }
        else
        {
            if (draggedItemIndex >= 0)
            {
                // Otherwise scroll to the item that was being dragged.
                IFC(Selector::ScrollIntoView(
                    draggedItemIndex,
                    FALSE, // isGroupItemIndex
                    FALSE, // isHeader
                    FALSE, // isFooter
                    FALSE, // isFromPublicAPI
                    TRUE,  // ensureContainerRealized
                    FALSE, // animateIfBringIntoView
                    xaml_controls::ScrollIntoViewAlignment::ScrollIntoViewAlignment_Default));
            }

            if (focusedIndex >= 0)
            {
                // ...and adjust focus to be on the right container, as indices are not updated.
                // This is a lower risk fix to an issue where focus is not adjusted after collection change.
                // A more comprehensive fix would be to update it in collection change notification in Selector.
                IFC(SetFocusedItem(focusedIndex, FALSE));
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

// Helper method for updating tracked index
static inline void UpdateTrackedIndex(
    _In_ INT realItemIndex,
    _In_ INT insertIndex,
    _Inout_opt_ INT* pTrackedIndex)
{
    if (pTrackedIndex != nullptr &&
        *pTrackedIndex >= 0)
    {
        if (*pTrackedIndex == realItemIndex)
        {
            *pTrackedIndex = insertIndex;
        }
        // tracked index is between source and insert index
        // Moving before to after
        else if (
            (realItemIndex < *pTrackedIndex) &&
            (insertIndex >= *pTrackedIndex))
        {
            (*pTrackedIndex)--;
        }
        // Moving after to before
        else if (
            (realItemIndex > *pTrackedIndex) &&
            (insertIndex <= *pTrackedIndex))
        {
            (*pTrackedIndex)++;
        }
    }
}

// Reorder the given list of items (given as a list of indexes) to the given index.
// Items are inserted in their original order.
// Optionally, pTrackedIndex can be specified. It should hold the index of an item you're
// interested in tracking. When the function returns successfully, pTrackedIndex will be updated
// with the new index location of the item originally at *pTrackedIndex.
_Check_return_ HRESULT ListViewBase::ReorderItemsTo(
    _In_ std::vector<UINT>* pReorderItemIndexes,
    _In_ INT insertIndex,
    _Inout_opt_ INT* pTrackedIndex1,
    _Inout_opt_ INT* pTrackedIndex2)
{
    HRESULT hr = S_OK;

    ctl::ComPtr<wfc::IVector<IInspectable*>> spItemSourceVector;
    ctl::ComPtr<xaml_interop::IBindableVector> spItemSourceBindableVector;
    ctl::ComPtr<wfc::IIterable<IInspectable*>> spItemsSource;

    IFCPTR(pReorderItemIndexes);
    IFCEXPECT(insertIndex >= 0);

    // Let our ItemCollection change handler know we're doing a reorder now. This prevents the handler
    // from tearing down fields we need to complete the reorder.
    m_isInItemReorderAfterDrop = TRUE;

    // Get our list of items as a mutable vector. This is usually from ItemsSource, but may be Items if ItemsSource is NULL.
    IFC(get_ItemsSource(&spItemsSource));
    if (spItemsSource != NULL)
    {
        spItemSourceVector = spItemsSource.AsOrNull<wfc::IVector<IInspectable*>>();
        if (spItemSourceVector == NULL)
        {
            spItemSourceBindableVector = spItemsSource.AsOrNull<xaml_interop::IBindableVector>();
            // IBindableVector is v-table compatible with IVector<IInspectable *>, so this reinterpret cast is safe
            spItemSourceVector = reinterpret_cast<wfc::IVector<IInspectable *>*>(spItemSourceBindableVector.Get());
        }
    }
    else
    {
        ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;
        IFC(get_Items(&spItems));
        spItemSourceVector = spItems.AsOrNull<wfc::IVector<IInspectable*>>();
    }

    if (spItemSourceVector != NULL)
    {
        // We need to move the dragged items to where the user dropped,
        // while preserving the original order of the dragged items.

        // The number of items that have been reordered so far.
        // We need this to account for indexes shifting during reorder.
        INT numItemsReorderedSoFar = 0;

        // Increment the Reorder Counter (used for content animations).
        IFC(OnItemsReordered(pReorderItemIndexes->size()));

        // Iterate over the dragged items in reverse index order. Move items as required,
        // keeping track of the insertion index as it changes. We also account for the fact that
        // the real indexes of items referred to in pReorderItemIndexes will shift as we move items
        // around.
        for (std::vector<UINT>::reverse_iterator it = pReorderItemIndexes->rbegin(); it != pReorderItemIndexes->rend(); ++it)
        {
            INT realItemIndex = *it;

            if (realItemIndex < insertIndex)
            {
                // Our insert index will change if we move an item to the insertion index
                // from a lower-index location.
                insertIndex--;
            }
            else
            {
                // If we're reordering an item from after the insertion index,
                // we must account for the shift of indexes due to previous iterations
                // of this loop. Remember, we're going in reverse index order.
                realItemIndex += numItemsReorderedSoFar;
            }

            // Reorder implemented as remove + insert.
            // TODO/REVISIT: This can be slow if the underlying
            // data source is array-based. We can't really detect this though,
            // since the data is provided externally.
            if (insertIndex != realItemIndex)
            {
                // Whether or not the item we're reordering is selected.
                BOOLEAN isCurrentReorderedItemSelected = FALSE;

                // Not used. The index of the item we're reordering in the selected items list, if applicable.
                UINT currentReorderedItemIndexInSelection = 0;

                // The item we're reordering.
                ctl::ComPtr<IInspectable> spCurrentItem;

                // Update tracked indices, if necessary.
                UpdateTrackedIndex(
                    realItemIndex,
                    insertIndex,
                    pTrackedIndex1);

                UpdateTrackedIndex(
                    realItemIndex,
                    insertIndex,
                    pTrackedIndex2);

                IFC(m_selection.Has(realItemIndex, currentReorderedItemIndexInSelection, isCurrentReorderedItemSelected));
                IFC(spItemSourceVector->GetAt(realItemIndex, &spCurrentItem));
                {
                    auto peggedCurrentItem = ctl::make_autopeg(spCurrentItem.Get());
                    IFC(spItemSourceVector->RemoveAt(realItemIndex));
                    IFC(spItemSourceVector->InsertAt(insertIndex, spCurrentItem.Get()));
                }

                // This remove-then-insert step will clear any selection on the current item.
                // If this item was selected, we need to re-select it.
                if (isCurrentReorderedItemSelected)
                {
                    // call the Selector::SelectOneItemInternal
                    IFC(SelectOneItemInternal(insertIndex, spCurrentItem.Get(), TRUE /* clearOldSelection */));
                }
            }

            numItemsReorderedSoFar++;
        }
    }

Cleanup:
    m_isInItemReorderAfterDrop = FALSE;
    return hr;
}

// React to a drag visual being created. The given visual is set to
// m_tpDragDropVisual. Will result in error if there is already a drag
// in progress.
_Check_return_ HRESULT ListViewBase::NotifyDragVisualCreated(
    _In_ DirectUI::DragDropVisual* pDragVisual)
{
    HRESULT hr = S_OK;

    // Only one drag/drop should happen at any given time.
    IFCEXPECT_ASSERT(!m_tpDragDropVisual);

    IFCPTR(pDragVisual);

    SetPtrValue(m_tpDragDropVisual, ctl::as_iinspectable(pDragVisual));

Cleanup:
    RRETURN(hr);
}

// Release m_tpDragDropVisual.
_Check_return_ HRESULT ListViewBase::NotifyDragVisualReleased()
{
    m_tpDragDropVisual.Clear();
    RRETURN(S_OK);
}

// Creates a blank IDataPackage for use in drag and drop.
_Check_return_ HRESULT ListViewBase::GetBlankDataPackage(
    _Outptr_ wadt::IDataPackage** ppData)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wadt::IDataPackage> spDataPackage;

    IFCPTR(ppData);

    *ppData = NULL;

    IFC(wf::ActivateInstance(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"Windows.ApplicationModel.DataTransfer.DataPackage")).Get(), spDataPackage.GetAddressOf()));
    IFCPTR(spDataPackage.Get());

    IFC(spDataPackage.CopyTo(ppData));

Cleanup:
    RRETURN(hr);
}

// Returns TRUE if the current interaction mode requires drag and drop.
// This is the case if CanDragItems == TRUE, or CanReorderItems == TRUE
// unless we're grouping.
_Check_return_ HRESULT ListViewBase::GetIsDragEnabled(
    _Out_ BOOLEAN *pDragEnabled)
{
    HRESULT hr = S_OK;
    BOOLEAN reorderActive = FALSE;
    BOOLEAN canDragItems = FALSE;
    BOOLEAN canReorderItems = FALSE;
    BOOLEAN isGrouping = FALSE;

    IFCPTR(pDragEnabled);

    *pDragEnabled = FALSE;

    IFC(get_IsGrouping(&isGrouping));

    IFC(get_CanDragItems(&canDragItems));
    IFC(get_CanReorderItems(&canReorderItems));

    // Ignore canReorderItems if we're grouping.
    reorderActive = canReorderItems && !isGrouping;
    *pDragEnabled = (canDragItems || reorderActive);

Cleanup:
    RRETURN(hr);
}


// Returns the edge scrolling velocity we should be performing.
// dragPoint - The point over which the user is dragging, relative to the ListViewBase.
_Check_return_ HRESULT ListViewBase::ComputeEdgeScrollVelocity(
    _In_ wf::Point dragPoint,
    _Out_ PanVelocity* pVelocity)
{
    HRESULT hr = S_OK;
    BOOLEAN isHorizontalScrollAllowed = FALSE;
    BOOLEAN isVerticalScrollAllowed = FALSE;
    PanVelocity velocity;

    pVelocity->Clear();

    // See in which directions we've enabled panning.
    if (m_tpScrollViewer != nullptr)
    {
        xaml_controls::ScrollMode verticalScrollMode = xaml_controls::ScrollMode_Disabled;
        xaml_controls::ScrollMode horizontalScrollMode = xaml_controls::ScrollMode_Disabled;

        IFC(m_tpScrollViewer.Cast<ScrollViewer>()->get_VerticalScrollMode(&verticalScrollMode));
        IFC(m_tpScrollViewer.Cast<ScrollViewer>()->get_HorizontalScrollMode(&horizontalScrollMode));

        isVerticalScrollAllowed = verticalScrollMode != xaml_controls::ScrollMode_Disabled;
        isHorizontalScrollAllowed = horizontalScrollMode != xaml_controls::ScrollMode_Disabled;
    }

    if (isHorizontalScrollAllowed)
    {
        DOUBLE offset = 0.0;
        DOUBLE bound = 0.0;

        IFC(m_tpScrollViewer->get_HorizontalOffset(&offset));

        // Try scrolling left.
        velocity.HorizontalVelocity = ComputeEdgeScrollVelocityFromEdgeDistance(dragPoint.X);
        if (velocity.IsStationary())
        {
            // Try scrolling right.
            DOUBLE width = 0;
            IFC(get_ActualWidth(&width));
            velocity.HorizontalVelocity = -ComputeEdgeScrollVelocityFromEdgeDistance(width - dragPoint.X);
            IFC(m_tpScrollViewer->get_ScrollableWidth(&bound));
        }
        else
        {
            // We're scrolling to the left.
            // The minimum horizontal offset obtained here accounts for the presence of the
            // sentinel offset values in the left padding and/or header case. For instance,
            // with no header or padding this will return exactly 2.0.
            IFC(m_tpScrollViewer.Cast<ScrollViewer>()->get_MinHorizontalOffset(&bound));
        }

        // Disallow edge scrolling if we're right up against the edge.
        if (DoubleUtil::AreWithinTolerance(bound, offset, ScrollViewerScrollRoundingTolerance))
        {
            velocity.Clear();
        }
    }

    if (isVerticalScrollAllowed && velocity.IsStationary() /* only allow vertical edge scrolling if there is no horizontal edge scrolling */)
    {
        DOUBLE offset = 0.0;
        DOUBLE bound = 0.0;
        IFC(m_tpScrollViewer->get_VerticalOffset(&offset));

        // Try scrolling up.
        velocity.VerticalVelocity = ComputeEdgeScrollVelocityFromEdgeDistance(dragPoint.Y);
        if (velocity.IsStationary())
        {
            // Try scrolling down.
            DOUBLE height = 0;
            IFC(get_ActualHeight(&height));
            velocity.VerticalVelocity = -ComputeEdgeScrollVelocityFromEdgeDistance(height - dragPoint.Y);
            IFC(m_tpScrollViewer->get_ScrollableHeight(&bound));
        }
        else
        {
            // We're scrolling up.
            // The minimum vertical offset obtained here accounts for the presence of the
            // sentinel offset values in the top padding and/or header case. For instance,
            // with no header or padding this will return exactly 2.0.
            IFC(m_tpScrollViewer.Cast<ScrollViewer>()->get_MinVerticalOffset(&bound));
        }

        // Disallow edge scrolling if we're right up against the edge.
        if (DoubleUtil::AreWithinTolerance(bound, offset, ScrollViewerScrollRoundingTolerance))
        {
            velocity.Clear();
        }
    }

    *pVelocity = velocity;

Cleanup:
    RRETURN(hr);
}

// Computes the speed of an edge scroll, given a distance from the edge.
XFLOAT ListViewBase::ComputeEdgeScrollVelocityFromEdgeDistance(
    _In_ DOUBLE distanceFromEdge)
{
    if (distanceFromEdge <= LISTVIEWBASE_EDGE_SCROLL_EDGE_WIDTH_PX)
    {
        // Linear velocity gradient.
        // 0 distance:                                      LISTVIEWBASE_EDGE_SCROLL_MAX_SPEED
        // LISTVIEWBASE_EDGE_SCROLL_EDGE_WIDTH_PX distance: LISTVIEWBASE_EDGE_SCROLL_MIN_SPEED
        return (XFLOAT)(LISTVIEWBASE_EDGE_SCROLL_MAX_SPEED -
                        (distanceFromEdge / LISTVIEWBASE_EDGE_SCROLL_EDGE_WIDTH_PX) * (LISTVIEWBASE_EDGE_SCROLL_MAX_SPEED - LISTVIEWBASE_EDGE_SCROLL_MIN_SPEED));
    }
    else
    {
        return 0;
    }
}

// Implements the delay-start, but instant-update behavior for edge scrolling.
// If the given velocity is zero, immediately stop edge scrolling.
// If there isn't a currently running edge scroll, start a timer. When that timer
// completes, call ScrollWithVelocity with the given arguments.
// If there is a currently running edge scroll, change the velocity immediately.
_Check_return_ HRESULT ListViewBase::SetPendingAutoPanVelocity(
    _In_ PanVelocity velocity)
{
    HRESULT hr = S_OK;

    if (!velocity.IsStationary())
    {
        if (m_currentAutoPanVelocity.IsStationary())
        {
            // Means we need to start a timer,
            // or we're updating the pending velocity.
            m_pendingAutoPanVelocity = velocity;
            IFC(EnsureStartEdgeScrollTimer());
        }
        else
        {
            // Our velocity is changing.
            m_currentAutoPanVelocity = velocity;
            IFC(ScrollWithVelocity(m_currentAutoPanVelocity));
        }
    }
    else
    {
        IFC(DestroyStartEdgeScrollTimer());
        m_currentAutoPanVelocity.Clear();
        m_pendingAutoPanVelocity.Clear();
        IFC(ScrollWithVelocity(m_currentAutoPanVelocity));
    }

Cleanup:
    RRETURN(hr);
}

// Scroll our ScrollViewer with the given velocity.
_Check_return_ HRESULT ListViewBase::ScrollWithVelocity(
    _In_ PanVelocity velocity)

{
    HRESULT hr = S_OK;

    if (m_tpScrollViewer)
    {
        IFC(m_tpScrollViewer.Cast<ScrollViewer>()->SetConstantVelocities(velocity.HorizontalVelocity, velocity.VerticalVelocity));
    }

Cleanup:
    RRETURN(hr);
}

// Instantiate the edge scroll timer, if necessary, and start it.
_Check_return_ HRESULT ListViewBase::EnsureStartEdgeScrollTimer()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wf::IEventHandler<IInspectable*>> spTickEventHandler;

    if (!m_tpStartEdgeScrollTimer)
    {
        EventRegistrationToken token;
        wf::TimeSpan edgeScrollTimeSpan = { LISTVIEWBASE_EDGE_SCROLL_START_DELAY_MSEC * TICKS_PER_MILLISECOND };
        ctl::ComPtr<DispatcherTimer> spEdgeScrollTimer;

        IFC(ctl::make<DispatcherTimer>(&spEdgeScrollTimer))
        SetPtrValue(m_tpStartEdgeScrollTimer, spEdgeScrollTimer);

        spTickEventHandler.Attach(
            new ClassMemberEventHandler<
                ListViewBase,
                xaml_controls::IListViewBase,
                wf::IEventHandler<IInspectable*>,
                IInspectable,
                IInspectable>(this, &ListViewBase::StartEdgeScrollTimerTick));
        IFC(m_tpStartEdgeScrollTimer->add_Tick(spTickEventHandler.Get(), &token));
        IFC(m_tpStartEdgeScrollTimer->put_Interval(edgeScrollTimeSpan));
        IFC(m_tpStartEdgeScrollTimer->Start());
    }

Cleanup:
    RRETURN(hr);
}

// Stop and releas the edge scroll timer.
_Check_return_ HRESULT ListViewBase::DestroyStartEdgeScrollTimer()
{
    HRESULT hr = S_OK;

    if (m_tpStartEdgeScrollTimer.Get() != NULL)
    {
        IFC(m_tpStartEdgeScrollTimer->Stop());
    }

Cleanup:
    m_tpStartEdgeScrollTimer.Clear();
    RRETURN(hr);
}

// The edge scroll timer calls this function when it's time
// to do our pending edge scroll action.
_Check_return_ HRESULT ListViewBase::StartEdgeScrollTimerTick(
    _In_opt_ IInspectable* pUnused1,
    _In_opt_ IInspectable* pUnused2)
{
    HRESULT hr = S_OK;

    IFC(DestroyStartEdgeScrollTimer());
    m_currentAutoPanVelocity = m_pendingAutoPanVelocity;
    m_pendingAutoPanVelocity.Clear();
    IFC(ScrollWithVelocity(m_currentAutoPanVelocity));

Cleanup:
    RRETURN(hr);
}

// Perform a keyboard reorder in the specified direction,
// originating from the given focused item.
_Check_return_ HRESULT ListViewBase::OnKeyboardReorder(
    _In_ KeyboardReorderDirection direction,
    _In_ ListViewBaseItem* pFocusedItem)
{
    HRESULT hr = S_OK;

    BOOLEAN isItemReorderingActive = FALSE;
    ctl::ComPtr<IPanel> spItemsHost;

    IFCPTR(pFocusedItem);
    IFC(get_ItemsHost(&spItemsHost));

    m_lastDragPosition.X = 0;
    m_lastDragPosition.Y = 0;

    IFC(IsItemReorderingActive(&isItemReorderingActive));

    if (isItemReorderingActive)
    {
        // The indexes of the items we'll be reordering.
        std::vector<UINT> reorderItemIndexes;

        // Get the indexes to reorder.
        IFC(GetDraggedItems(pFocusedItem, nullptr /* ppItems */, &reorderItemIndexes));

        if (!reorderItemIndexes.empty())
        {
            // The index at which reordered items should be inserted.
            INT insertIndex = 0;

            // The index of the item the user is on.
            INT keyboardFocusItemIndex = 0;

            // Item that will serve as the physical origin of the reordering animation.
            // If we're reordering forwards, it's the last item being reordered.
            // If we're reordering backwards, it's the first item being reordered.
            INT animationOriginItemIndex = 0;

            IFC(IndexFromContainer(pFocusedItem, &keyboardFocusItemIndex));

            // Sort the indexes in increasing order.
            std::sort(reorderItemIndexes.begin(), reorderItemIndexes.end());

            if (direction == KeyboardReorderDirection_ToLowerIndex)
            {
                UINT front = reorderItemIndexes.front();
                animationOriginItemIndex = front;
                insertIndex = front-1;
            }
            else
            {
                UINT back = reorderItemIndexes.back();
                animationOriginItemIndex = back;
                insertIndex = back + 2;
            }

            if (insertIndex >= 0)
            {
                ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;
                UINT itemsCount = 0;
                IFC(get_Items(&spItems));
                IFC(spItems.Cast<ItemCollection>()->get_Size(&itemsCount));

                if (itemsCount >= static_cast<UINT>(insertIndex))
                {
                    ctl::ComPtr<xaml::IDependencyObject> spAnimationOriginItem;
                    ctl::ComPtr<xaml::IUIElement> spAnimationOriginItemAsUIE;

                    // Set up the last reorder position to be the top-left of animationOriginItemIndex, so animations
                    // look OK.
                    IFC(ContainerFromIndex(animationOriginItemIndex, &spAnimationOriginItem));
                    spAnimationOriginItemAsUIE = spAnimationOriginItem.AsOrNull<xaml::IUIElement>();
                    if (spAnimationOriginItemAsUIE.Get() != NULL)
                    {
                        // Used to calculate the last reorder position so we can animate the dropped item into position.
                        ctl::ComPtr<IGeneralTransform> spToParent;
                        wf::Point origin = {0, 0};
                        // Store the last reorder position so we can animate the dropped item into position.
                        IFC(spAnimationOriginItemAsUIE->TransformToVisual(this, &spToParent));
                        IFC(spToParent->TransformPoint(origin, &m_lastReorderPosition));
                    }

                    // Disable transitions.
                    IFC(spItemsHost.Cast<Panel>()->put_IsIgnoringTransitions(TRUE));

                    // Do the reorder, and get the new index of the currently focused item and reorder animation origin item.
                    IFC(ReorderItemsTo(&reorderItemIndexes, insertIndex, &keyboardFocusItemIndex, nullptr));
                    // At this point, pFocusedItem and spAnimationOriginItem are possibly no longer in the tree. So we can't rely upon them.

                    // Force a layout so ScrollIntoView knows where to scroll to.
                    IFC(UpdateLayout());

                    // Enable transitions.
                    IFC(spItemsHost.Cast<Panel>()->put_IsIgnoringTransitions(FALSE));

                    // Bring the focused item into view and focus it (since it will have been unfocused by the reorder).
                    IFC(SetFocusedItem(keyboardFocusItemIndex, TRUE /* shouldScrollIntoView */, TRUE /*forceFocus*/, xaml::FocusState_Keyboard));

                    //Set up and call into Narrator to read out a reorder string.
                    IFC(UpdateDropTargetDropEffect(FALSE, FALSE /*isLeaving*/, pFocusedItem));
                    IFC(AutomationPeer::RaiseEventIfListener(this, xaml_automation_peers::AutomationEvents_Dropped));
                }
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ListViewBase::ToggleBackKeyListenerOnPhone(_In_ BOOLEAN isReorderModeEnabled)
{
    if (DXamlCore::GetCurrent()->GetHandle()->BackButtonSupported())
    {
        if(isReorderModeEnabled)
        {
            IFC_RETURN(BackButtonIntegration_RegisterListener(this));
        }
        else
        {
            IFC_RETURN(BackButtonIntegration_UnregisterListener(this));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
ListViewBase::OnBackButtonPressedImpl(_Out_ BOOLEAN* pHandled)
{
    HRESULT hr = S_OK;

    // Disable reorder mode when the back button is pressed
    IFC(put_ReorderMode(xaml_controls::ListViewReorderMode_Disabled));
    *pHandled = TRUE;

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ListViewBase::IsCurrentlyReordering(_Out_ bool* isReordering)
{
    *isReordering = false;
    if (m_isDraggingOverSelf)
    {
        BOOLEAN canReorder = FALSE;
        IFC_RETURN(get_CanReorderItems(&canReorder));
        *isReordering = !!canReorder;
    }
    return S_OK;
}

// Returns TRUE if live reordering is enabled
// This means CanReorderItem is true and AllowDrop is true
_Check_return_ HRESULT ListViewBase::ShouldPlayLiveReorder(
    _Out_ BOOLEAN* pResult)
{
    BOOLEAN allowDrop = FALSE;
    BOOLEAN canReorder = FALSE;
    BOOLEAN isStationary = m_currentAutoPanVelocity.IsStationary(); // we're not at the edge scrolling
    BOOLEAN supportLiveReorder = FALSE;

    *pResult = FALSE;

    if (m_tpPrimaryDraggedContainer)
    {
        supportLiveReorder = TRUE;
    }
    else
    {
        supportLiveReorder = (m_dragAcceptedOperation != wadt::DataPackageOperation_None);
    }

    IFC_RETURN(get_AllowDrop(&allowDrop));
    IFC_RETURN(get_CanReorderItems(&canReorder));

    *pResult = canReorder && allowDrop && isStationary && supportLiveReorder;

    return S_OK;
}

// Returns whether the passed item is the one being dragged over
BOOLEAN ListViewBase::IsDragOverItem(
    _In_ ListViewBaseItem* pItem)
{
    return (pItem == m_tpDragOverItem.Get());
}


// Returns whether the ListViewBase was the source of the drag
_Check_return_ HRESULT ListViewBase::IsDragSourceImpl(
    _Out_ BOOLEAN* pReturnValue)
{
    *pReturnValue = (m_tpPrimaryDraggedContainer.Get() != nullptr);

    return S_OK;
}

_Check_return_ HRESULT ListViewBase::StartLiveReorderTimer()
{
    wf::TimeSpan timeSpan = {};

    // make sure we stop the current timer if it exists
    IFC_RETURN(StopLiveReorderTimer());

    // make sure we have a timer
    IFC_RETURN(EnsureLiveReorderTimer());

    // set the timer duration
    timeSpan.Duration = (ctl::is<xaml_controls::IGridView>(this) ? GRIDVIEW_LIVEREORDER_TIMER : LISTVIEW_LIVEREORDER_TIMER) * TICKS_PER_MILLISECOND;

    // set the interval
    IFC_RETURN(m_tpLiveReorderTimer->put_Interval(timeSpan));

    // start the timer
    IFC_RETURN(m_tpLiveReorderTimer->Start());

    return S_OK;
}

_Check_return_ HRESULT ListViewBase::EnsureLiveReorderTimer()
{
    // if it hasn't been created yet, create it
    if (!m_tpLiveReorderTimer)
    {
        ctl::ComPtr<DispatcherTimer> spNewDispatcherTimer;

        IFC_RETURN(ctl::make<DispatcherTimer>(&spNewDispatcherTimer));

        // attach the handler
        IFC_RETURN(m_epLiveReorderTimerEvent.AttachEventHandler(
            spNewDispatcherTimer.Get(),
            [this](_In_opt_ IInspectable* pUnused1, _In_opt_ IInspectable* pUnused2)
        {
            IFC_RETURN(LiveReorderTimerTickHandler());

            return S_OK;
        }));

        SetPtrValue(m_tpLiveReorderTimer, spNewDispatcherTimer.Get());
    }

    return S_OK;
}

_Check_return_ HRESULT ListViewBase::StopLiveReorderTimer()
{
    if (m_tpLiveReorderTimer)
    {
        IFC_RETURN(m_tpLiveReorderTimer->Stop());
    }

    return S_OK;
}

// Handler for the Tick event on m_tpLiveReorderTimer
_Check_return_ HRESULT ListViewBase::LiveReorderTimerTickHandler()
{
    // stop the timer
    IFC_RETURN(StopLiveReorderTimer());

    if (IsInLiveTree())
    {
        // We started the timer because we had a dragged item.
        ASSERT(m_liveReorderIndices.draggedItemIndex != -1);

        std::vector<DirectUI::Components::LiveReorderHelper::MovedItem> newItems;
        std::vector<DirectUI::Components::LiveReorderHelper::MovedItem> newItemsToMove;
        std::vector<DirectUI::Components::LiveReorderHelper::MovedItem> oldItemsToMoveBack;

        ctl::ComPtr<IPanel> spItemsHost;
        xaml_controls::Orientation panelLogicalOrientation = xaml_controls::Orientation_Vertical;

        // Get the panel's logical orientation
        // We need this to know the orientation of the items
        IFC_RETURN(get_ItemsHost(&spItemsHost));
        IFC_RETURN(GetPanelOrientations(spItemsHost.Get(), nullptr /*pPhysicalOrientation*/, &panelLogicalOrientation));

        // get the new items
        IFC_RETURN(GetNewMovedItemsForLiveReorder(newItems));

        // update the MovedItems
        m_movedItems.Update(panelLogicalOrientation == xaml_controls::Orientation_Vertical /* isOrientationVertical */, newItems, newItemsToMove, oldItemsToMoveBack);

        // move items back to their original location
        IFC_RETURN(MoveItemsForLiveReorder(false /* areNewItems */, oldItemsToMoveBack));

        // move the new items to their destination
        IFC_RETURN(MoveItemsForLiveReorder(true /* areNewItems */, newItemsToMove));
    }

    return S_OK;
}

_Check_return_ HRESULT ListViewBase::GetNewMovedItemsForLiveReorder(
    _Inout_ std::vector<DirectUI::Components::LiveReorderHelper::MovedItem>& newItems)
{
    const int startIndex = m_liveReorderIndices.draggedItemIndex;
    const int endIndex = m_liveReorderIndices.draggedOverIndex;
    const int increment = (startIndex < endIndex) ? 1 : -1;

    ctl::ComPtr<LayoutInformation> spLayoutInformation;

    IFC_RETURN(ctl::make<LayoutInformation>(&spLayoutInformation));

    newItems.clear();

    for (int i = startIndex; i != endIndex; i += increment)
    {
        int targetIndex = i - increment;

        if (i == startIndex)
        {
            targetIndex = -1;
        }

        IFC_RETURN(AddNewItemForLiveReorder(i, targetIndex, spLayoutInformation.Get(), newItems));
    }

    // add the endIndex
    IFC_RETURN(AddNewItemForLiveReorder(endIndex, endIndex - increment, spLayoutInformation.Get(), newItems));

    return S_OK;
}

_Check_return_ HRESULT ListViewBase::AddNewItemForLiveReorder(
    _In_ const int sourceIndex,
    _In_ const int targetIndex,
    _In_ DirectUI::LayoutInformation* layoutInformation,
    _Inout_ std::vector<DirectUI::Components::LiveReorderHelper::MovedItem>& newItems)
{
    const int itemsCount = m_liveReorderIndices.itemsCount;

    wf::Rect sourceRect = {};
    wf::Rect targetRect = {};

    // source information
    // in the case where we're dragging an element from outside the list
    // sourceIndex will be equal to the items' count
    if (sourceIndex != itemsCount)
    {
        IFC_RETURN(GetLayoutSlot(sourceIndex, layoutInformation, &sourceRect));
    }

    // destination information
    // if targetIndex is -1, then this is the element being dragged
    // and we don't need to know its target location since it will be faded out
    // LiveReorderHelper requires us to set the target to -1 for the dragged item
    // and ignores the targetRect in that case
    // if targetIndex is the itemsCount then the drag happened from outside the list
    if (targetIndex != -1 && targetIndex != itemsCount)
    {
        IFC_RETURN(GetLayoutSlot(targetIndex, layoutInformation, &targetRect));
    }

    // add it to the collection
    newItems.push_back(DirectUI::Components::LiveReorderHelper::MovedItem(sourceIndex, targetIndex, sourceRect, targetRect));

    return S_OK;
}

_Check_return_ HRESULT ListViewBase::ResetAllItemsForLiveReorder()
{
    IFC_RETURN(StopLiveReorderTimer());

    for (auto itr = m_movedItems.begin(); itr != m_movedItems.end(); ++itr)
    {
        if (itr->destinationIndex != -1)
        {
            ctl::ComPtr<xaml::IDependencyObject> spCurrentElementAsDO;
            IFC_RETURN(ContainerFromIndex(itr->sourceIndex, &spCurrentElementAsDO));

            if (spCurrentElementAsDO)
            {
                IFC_RETURN(spCurrentElementAsDO.Cast<ListViewBaseItem>()->Arrange(itr->sourceRect));
            }
        }
    }

    m_movedItems.clear();
    m_liveReorderIndices = { -1, -1, -1 };

    return S_OK;
}

_Check_return_ HRESULT ListViewBase::MoveItemsForLiveReorder(
    _In_ bool areNewItems,
    _Inout_ const std::vector<DirectUI::Components::LiveReorderHelper::MovedItem>& newItemsToMove)
{
    wf::Rect sourceRect = {};
    wf::Rect destinationRect = {};
    ctl::ComPtr<xaml::IDependencyObject> spCurrentElementAsDO;

    for (const auto& movedItem : newItemsToMove)
    {
        IFC_RETURN(ContainerFromIndex(movedItem.sourceIndex, &spCurrentElementAsDO));

        if (spCurrentElementAsDO)
        {
            if (areNewItems)
            {
                sourceRect = movedItem.sourceRect;
                destinationRect = movedItem.destinationRect;
            }
            else
            {
                sourceRect = movedItem.destinationRect;
                destinationRect = movedItem.sourceRect;
            }

            IFC_RETURN(spCurrentElementAsDO.Cast<ListViewBaseItem>()->Arrange(destinationRect));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT ListViewBase::GetLayoutSlot(
    _In_ int itemIndex,
    _In_ DirectUI::LayoutInformation* pLayoutInformation,
    _Out_ wf::Rect* pLayoutSlot)
{
    ctl::ComPtr<xaml::IDependencyObject> spItemAsDO;
    IFC_RETURN(ContainerFromIndex(itemIndex, &spItemAsDO));

    if (spItemAsDO)
    {
        ctl::ComPtr<xaml::IFrameworkElement> spItemAsFE;

        IFC_RETURN(spItemAsDO.As(&spItemAsFE));
        IFC_RETURN(pLayoutInformation->GetLayoutSlot(spItemAsFE.Get(), pLayoutSlot));
    }

    return S_OK;
}

_Check_return_ HRESULT ListViewBase::GetItemsCount(
    _Out_ int* pItemsCount)
{
    unsigned int itemsCount = 0;
    ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;

    *pItemsCount = 0;

    IFC_RETURN(get_Items(&spItems));
    IFC_RETURN(spItems.Cast<ItemCollection>()->get_Size(&itemsCount));

    *pItemsCount = static_cast<int>(itemsCount);

    return S_OK;
}

_Check_return_ HRESULT ListViewBase::GetPanelOrientation(
    _Out_ xaml_controls::Orientation* panelOrientation)
{
    ctl::ComPtr<IPanel> spItemsHost;

    *panelOrientation = xaml_controls::Orientation_Vertical;

    IFC_RETURN(get_ItemsHost(&spItemsHost));

    // Get the panel's logical orientation
    // We need this to know the orientation of the folder's regions
    IFC_RETURN(GetPanelOrientations(spItemsHost.Get(), nullptr /*pPhysicalOrientation*/, panelOrientation));

    return S_OK;
}

_Check_return_ HRESULT ListViewBase::SetDragOverItem(
    _In_ ListViewBaseItem* dragOverItem)
{
    if (dragOverItem == nullptr)
    {
        m_tpDragOverItem.Clear();
    }
    else
    {
        SetPtrValue(m_tpDragOverItem, dragOverItem);
    }

    return S_OK;
}

_Check_return_ IFACEMETHODIMP ListViewBase::OverrideContainerArrangeBounds(
    _In_ INT index,
    _In_ wf::Rect suggestedBounds,
    _Out_ wf::Rect* newBounds)
{
    // by default, we use the suggested bounds
    *newBounds = suggestedBounds;

    if (IsInLiveReorder())
    {
        // search for the item in the LiveReorder movedItems array
        // if we find it, we will override the bounds that were passed
        // with the new ones for LiveReorder
        for (auto itr = m_movedItems.begin(); itr != m_movedItems.end(); ++itr)
        {
            // destinationIndex == -1 means that this is the item being dragged -> do not update the bounds
            if (itr->sourceIndex == index && itr->destinationIndex != -1)
            {
                *newBounds = itr->destinationRect;
                break;
            }
        }
    }

    return S_OK;
}
