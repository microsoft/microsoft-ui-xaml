// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ListViewBaseItem.g.h"
#include "ListViewBaseItemAutomationPeer.g.h"
#include "XamlBehaviorMode.h"
#include "Timeline.g.h"
#include "ScrollViewer.g.h"
#include "ListViewItem.g.h"
#include "GridViewItem.g.h"
#include "ModernCollectionBasePanel.g.h"
#include "PointerRoutedEventArgs.g.h"
#include "Canvas.g.h"
#include "DragDropVisual.h"
#include <DependencyLocator.h>
#include "XamlTraceLogging.h"
#include "XamlTraceSession.h"
#include "VisualStatesHelper.h"
#include "DragDropInternal.h"
#include "AutomaticDragHelper.h"
#include "CVisualStateManager2.h"
#include "KeyRoutedEventArgs.g.h"
#include "DispatcherTimer.g.h"
#include "ToolTipService.g.h"
#include "MatrixTransform.g.h"
#include "VisualTreeHelper.h"

#include <RuntimeEnabledFeatures.h>
#include <DependencyLocator.h>

using namespace DirectUI;
using namespace DirectUI::Components;

// The ZIndex used by ListViewBaseItem during user interaction.
// Note that ZIndex==1 is used by opaque headers which should be above rest items but below dragged one
#define LISTVIEWBASEITEM_INTERACTION_ZINDEX 2
// The ZIndex used by ListViewBaseItem when no user is performing an interaction.
#define LISTVIEWBASEITEM_REST_ZINDEX 0
// The name of the NoSelectionHint VisualState.
#define LISTVIEWBASEITEM_NO_SELECTION_HINT_STATE_NAME L"NoSelectionHint"
// The name of the NoReorderHint VisualState.
#define LISTVIEWBASEITEM_NO_REORDER_HINT_STATE_NAME L"NoReorderHint"
// The name of the content container template part (the bit that becomes the drag visual).
#define LISTVIEWBASEITEM_CONTENT_CONTAINER_PART_NAME L"ContentContainer"
// (phone only) The name of the checkbox container template part (used to determine the source of a PointerPress).
#define LISTVIEWBASEITEM_CHECKBOX_CONTAINER_PART_NAME L"CheckboxContainer"

// The standard Windows mouse drag box size is defined by SM_CXDRAG and SM_CYDRAG.
// ListViewBaseItem uses the standard box size with dimensions multiplied by this constant.
// This arrangement is in place as accidentally triggering a drag was deemed too easy while
// selecting several items with the mouse in quick succession.
// See bug 103860: Jupiter moco: Reorder  kicks in before DropTargetItemThemeAnimation starts
#define LISTVIEWBASEITEM_MOUSE_DRAG_THRESHOLD_MULTIPLIER 2.0

// The delay time before going to the Pressed state after the initial touch
#define LISTVIEWBASEITEM_TOUCH_PRESSED_DELAY 100

// The time to stay in a Pressed state if the touch was less than LISTVIEWBASEITEM_TOUCH_PRESSED_DELAY
// basically, we want to cover the case where the user simply taps
// a visual should be shown
#define LISTVIEWBASEITEM_TOUCH_RELEASED_TIMER 125

// Two variables to define the outside area of the DragOver region
#define LISTVIEWITEM_DRAGOVER_OUTSIDE_MARGIN 0.2
#define GRIDVIEWITEM_DRAGOVER_OUTSIDE_MARGIN 0.3

using namespace ctl;
using namespace DirectUI;
using namespace DirectUISynonyms;
using namespace std::placeholders;

const wf::Point ListViewBaseItem::s_center = { 0.5f, 0.5f };
const double ListViewBaseItem::s_holdingVisualOpacity = 0.8;
const double ListViewBaseItem::s_holdingVisualScale = 1.05;

ListViewBaseItem::ListViewBaseItem()
    : m_isCheckingForMouseDrag(FALSE)
    , m_dragVisualCaptured(FALSE)
    , m_isKeyboardPressed(FALSE)
    , m_isDragOver(FALSE)
{
    m_lastMouseLeftButtonDownPosition.X = 0;
    m_lastMouseLeftButtonDownPosition.Y = 0;

    m_lastTouchDownPosition.X = 0;
    m_lastTouchDownPosition.Y = 0;

    // These static_casts are to keep the compiler happy.
    m_isTopmost = FALSE;
    m_inReorderHintState = FALSE;
    m_inSelectionHintState = FALSE;

    m_shouldEnterPointerOver = FALSE;
    m_isLeftButtonPressed = FALSE;
    m_isRightButtonPressed = FALSE;
    m_inPressedForTouch = FALSE;
    m_isHolding = FALSE;
    m_inCheckboxPressedForTouch = FALSE;

    m_fDragIsGrabbed = FALSE;

    m_isStartingDrag = FALSE;

    m_touchTimerState = TouchTimerState::NoTimer;
}

ListViewBaseItem::~ListViewBaseItem()
{
    // stop the timer if it exists
    {
        auto spTouchTimer = m_tpTouchTimer.GetSafeReference();
        if (spTouchTimer)
        {
            IGNOREHR(spTouchTimer->Stop());
        }
    }

    VERIFYHR(DetachHandler(m_epTouchTimerEvent, m_tpTouchTimer));
    VERIFYHR(UnregisterNoHintVisualStateHandlers());
}

// Get the parent ListViewBase of this item.  Note that we retrieve the parent
// via GetParentSelector whose relationship is setup in PrepareContainer and
// torn down in ClearContainer so it's only valid within that range.  This will
// obviously be NULL if the parent ItemsControl does not implement
// IListViewBase.
ListViewBase* ListViewBaseItem::GetParentListView()
{
    ctl::ComPtr<Selector> spParentSelector;
    ctl::ComPtr<IListViewBase> spParentListView;

    // TODO: Is this going to create really noisy IFC traces?  If so, we should
    // consider implementing a "quiet" version that is happy to handle null or
    // expired references.
    IGNOREHR(GetParentSelector(&spParentSelector));
    if (spParentSelector)
    {
        spParentListView = spParentSelector.AsOrNull<IListViewBase>();
    }

    return spParentListView.Cast<ListViewBase>();
}

// Initialize the ListViewBaseItem by setting up its OnLoaded/OnUnloaded handlers.
_Check_return_ HRESULT ListViewBaseItem::Initialize()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml::IRoutedEventHandler> spLoadedEventHandler;
    EventRegistrationToken loadedToken;

    IFC(ListViewBaseItemGenerated::Initialize());

    spLoadedEventHandler.Attach(
        new ClassMemberEventHandler<
            ListViewBaseItem,
            ISelectorItem,
            xaml::IRoutedEventHandler,
            IInspectable,
            xaml::IRoutedEventArgs>(
                this,
                &ListViewBaseItem::OnLoaded,
                true /* subscribingToSelf */));

    IFC(add_Loaded(spLoadedEventHandler.Get(), &loadedToken));

    m_isCheckingForMouseDrag = FALSE;

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ListViewBaseItem::OnTouchDragStarted(
    _In_ ixp::IPointerPoint* pointerPoint,
    _In_ xaml_input::IPointer* pointer)
{
    // On a touch drag, we don't get Holding Completed event. So, clearing the m_isHolding flag & destroying holding visual here.
    m_isHolding = FALSE;
    IFC_RETURN(DestroyHoldingVisual());

    ASSERT(DXamlCore::GetCurrent()->GetDragDrop()->GetUseCoreDragDrop());

    BOOLEAN didStartDrag = FALSE;
    ctl::ComPtr<IGeneralTransform> spToRoot;
    wf::Point position = { 0, 0 };
    wf::Point visualOffset = { -m_lastTouchDownPosition.X, -m_lastTouchDownPosition.Y };
    wf::Point visualPosition = { 0, 0 };

    IFC_RETURN(pointerPoint->get_Position(&position));

    // In M2, WinRT DnD API is only called on desktop when drag when XamlPresenter is not in use,
    // in that case, system will render the default visual for us.
    IFC_RETURN(EnsureDragDropVisual(visualOffset, DXamlCore::GetCurrent()->GetDragDrop()->GetUseSystemDefaultVisual()));
    IFC_RETURN(m_spDragDropVisual->Show());

    IFC_RETURN(TransformToVisual(nullptr, &spToRoot));
    IFC_RETURN(spToRoot->TransformPoint(position, &visualPosition));
    IFC_RETURN(m_spDragDropVisual->SetPosition(visualPosition));

    // Try the drag. Continuing our previous gesture is not applicable here.
    IFC_RETURN(TryStartDrag(
        pointer,
        pointerPoint,
        position,
        nullptr, // pShouldPreviousGestureContinue
        &didStartDrag));

    if (!didStartDrag)
    {
        m_spDragDropVisual = nullptr;
    }
    IFC_RETURN(ChangeVisualState(TRUE));

    // ListView did not start Drag and Drop, UIElement's Drag and Drop might be enabled on LVI
    if (!didStartDrag)
    {
        IFC_RETURN(StartDragIfEnabled(pointerPoint));
    }
    return S_OK;
}

// Starts a UIElement's Drag and Drop if CanDrag==True
_Check_return_ HRESULT ListViewBaseItem::StartDragIfEnabled(_In_ ixp::IPointerPoint* pointerPoint)
{
    BOOLEAN canDrag = false;
    IFC_RETURN(get_CanDrag(&canDrag));
    if (canDrag)
    {
        ctl::ComPtr<wf::IAsyncOperation<wadt::DataPackageOperation>> spOperation;
        IFC_RETURN(StartDragAsync(pointerPoint, spOperation.GetAddressOf()));
    }
    return S_OK;
}

// Called when the element leaves the tree.
_Check_return_ HRESULT ListViewBaseItem::LeaveImpl(
    _In_ bool bLive,
    _In_ bool bSkipNameRegistration,
    _In_ bool bCoercedIsEnabled,
    _In_ bool bVisualTreeBeingReset)
{
    IFC_RETURN(ListViewBaseItemGenerated::LeaveImpl(bLive, bSkipNameRegistration, bCoercedIsEnabled, bVisualTreeBeingReset));

    IFC_RETURN(StopTouchTimer());

    return S_OK;
}

// Called when the ListViewBaseItem has been constructed and added to the visual
// tree.
_Check_return_ HRESULT ListViewBaseItem::OnLoaded(
    _In_ IInspectable* pSender,
    _In_ IRoutedEventArgs* pArgs)
{
    RRETURN(ChangeVisualState(false));
}

// Apply a template to the ListViewBaseItem.
IFACEMETHODIMP ListViewBaseItem::OnApplyTemplate()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IDependencyObject> spContentContainerValue;
    ctl::ComPtr<IUIElement> spContentContainer;
    ctl::ComPtr<IDependencyObject> spCheckboxContainerValue;
    ctl::ComPtr<IUIElement> spCheckboxContainer;

    IFC(UnregisterNoHintVisualStateHandlers());

    IFC(ListViewBaseItemGenerated::OnApplyTemplate());

    IFC(RegisterNoHintVisualStateHandlers());

    // Grab the ContentContainer template part.
    m_tpContentContainer.Clear();
    IFC(GetTemplateChild(wrl_wrappers::HStringReference(STR_LEN_PAIR(LISTVIEWBASEITEM_CONTENT_CONTAINER_PART_NAME)).Get(), &spContentContainerValue));
    if (spContentContainerValue)
    {
        IFC(spContentContainerValue.As<IUIElement>(&spContentContainer));
        SetPtrValue(m_tpContentContainer, spContentContainer.Get());
    }

    m_tpCheckboxContainer.Clear();
    IFC(GetTemplateChild(wrl_wrappers::HStringReference(STR_LEN_PAIR(LISTVIEWBASEITEM_CHECKBOX_CONTAINER_PART_NAME)).Get(), &spCheckboxContainerValue));
    if (spCheckboxContainerValue)
    {
        IFC(spCheckboxContainerValue.As<IUIElement>(&spCheckboxContainer));
        SetPtrValue(m_tpCheckboxContainer, spCheckboxContainer.Get());
    }

    // Set up the chrome, if we have it.
    {
        CListViewBaseItemChrome* pChrome = nullptr;

        IFC(GetItemChrome(&pChrome));

        if (pChrome)
        {
            IFC(pChrome->SetChromedListViewBaseItem(static_cast<CUIElement*>(GetHandle())));
        }

        // Update the child (refs get taken care of).
        IFC(static_cast<CContentControl*>(GetHandle())->SetGridViewItemChrome(pChrome));
    }

    // Sync the logical and visual states of the control
    IFC(ChangeVisualState(false));

Cleanup:
    RRETURN(hr);
}

// Registers a handler to be called when the ListViewBaseItem completes a transition to the named VisualState.
_Check_return_ HRESULT ListViewBaseItem::AttachTransitionCompleted(
    _In_reads_(nStateNameLength + 1) _Null_terminated_ const WCHAR* pszStateName,
    _In_ XUINT32 nStateNameLength,
    _In_ std::function<HRESULT(IInspectable*, IInspectable*)> callableHandler,
    _Outptr_result_maybenull_ xaml_animation::ITimeline** ppAttachedTo,
    _Out_ EventPtr<TimelineCompletedEventCallback>* pCompletedEvent)
{
    HRESULT hr = S_OK;
    BOOLEAN found = FALSE;
    ctl::ComPtr<xaml::IVisualState> spState;

    IFCPTR(ppAttachedTo);
    IFCPTR(pszStateName);

    *ppAttachedTo = nullptr;

    // TODO: We shouldn't be registering on a VisualState's Storyboard.Completed
    // here. Instead, we should listen to VSG.CurrentStateChanged. There's a bug preventing us from
    // doing so: 101520 - CurrentStateChanged event fires immediately, before animation completes.
    // Current workaround will fail if there are VisualTransitions in the VSG.

    // Try to find the VisualState.
    IFC(VisualStateManager::TryGetState(this, pszStateName, nullptr, &spState, &found));

    if (found && spState)
    {
        ctl::ComPtr<IStoryboard> spStoryboard;

        // Register for Completed on the Storyboard.
        IFC(spState->get_Storyboard(&spStoryboard));

        if (spStoryboard)
        {
            ctl::ComPtr<xaml_animation::ITimeline> spTimeline;

            IFC(spStoryboard.As<ITimeline>(&spTimeline));

            IFC(pCompletedEvent->AttachEventHandler(
                spTimeline.Get(),
                callableHandler));

            *ppAttachedTo = spTimeline.Detach();
        }
    }

Cleanup:
    RRETURN(hr);
}

// Registers event handlers on the Storyboard within the NoSelectionHint
// VisualState.
_Check_return_ HRESULT ListViewBaseItem::RegisterNoHintVisualStateHandlers()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_animation::ITimeline> spNoSelectionHintTimeline;
    ctl::ComPtr<xaml_animation::ITimeline> spNoReorderHintTimeline;

    IFC(AttachTransitionCompleted(
        STR_LEN_PAIR(LISTVIEWBASEITEM_NO_SELECTION_HINT_STATE_NAME),
        std::bind(&ListViewBaseItem::OnNoSelectionHintStoryboardCompleted, this, _1, _2),
        &spNoSelectionHintTimeline,
        &m_epNoSelectionHintTimelineCompletedHandler));

    SetPtrValue(m_tpNoSelectionHintTimeline, spNoSelectionHintTimeline);

    IFC(AttachTransitionCompleted(
        STR_LEN_PAIR(LISTVIEWBASEITEM_NO_REORDER_HINT_STATE_NAME),
        std::bind(&ListViewBaseItem::OnNoReorderHintStoryboardCompleted, this, _1, _2),
        &spNoReorderHintTimeline,
        &m_epNoReorderHintTimelineCompletedHandler));

    SetPtrValue(m_tpNoReorderHintTimeline, spNoReorderHintTimeline);

Cleanup:
    RRETURN(hr);
}

// Unregisters all event handlers on the Storyboard within the NoSelectionHint
// VisualState.
_Check_return_ HRESULT ListViewBaseItem::UnregisterNoHintVisualStateHandlers()
{
    HRESULT hr = S_OK;

    IFC(DetachHandler(m_epNoSelectionHintTimelineCompletedHandler, m_tpNoSelectionHintTimeline));
    IFC(DetachHandler(m_epNoReorderHintTimelineCompletedHandler, m_tpNoReorderHintTimeline));

    m_tpNoSelectionHintTimeline.Clear();
    m_tpNoReorderHintTimeline.Clear();

Cleanup:
    RRETURN(hr);
}

// Called when the user presses a pointer down over the ListViewBaseItem.
IFACEMETHODIMP ListViewBaseItem::OnPointerPressed(
    _In_ IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN isHandled = FALSE;
    BOOLEAN isVisualStateChangePending = FALSE;

    IFCPTR(pArgs);
    IFC(ListViewBaseItemGenerated::OnPointerPressed(pArgs));

    // TODO: We may want to move this functionality into an OnPointerPressed/Released
    // handler.  It would be weird if mouse raised ItemClick for
    // OnMouseLeftButtonDown but touch only raised ItemClick for "PointerReleased"
    // (depending on whether we get an "OnPointerPressed" call or just an
    // "OnTap/OnHold" call for the self-reveal functionality we need to
    // implement for Swipe).

    IFC(pArgs->get_Handled(&isHandled));
    if (!isHandled)
    {
        ctl::ComPtr<IPointer> spPointer;
        UINT pointerId = 0;
        mui::PointerDeviceType pointerDeviceType = mui::PointerDeviceType_Touch;
        ctl::ComPtr<ListViewBase> spListView = GetParentListView();
        ctl::ComPtr<ixp::IPointerPoint> spPointerPoint;


        IFC(pArgs->get_Pointer(&spPointer));
        IFC(spPointer->get_PointerDeviceType(&pointerDeviceType));
        IFC(spPointer->get_PointerId(&pointerId));

        IFC(pArgs->GetCurrentPoint(this, &spPointerPoint));
        IFCPTR(spPointerPoint);

        // TODO: Ignore the PointerPressed if we've already "captured" input
        // from another device (via InteractionManager)

        // Check if this is a mouse button down or a finger down.
        if (pointerDeviceType == mui::PointerDeviceType_Mouse || pointerDeviceType == mui::PointerDeviceType_Pen)
        {
            // Mouse button down.
            ctl::ComPtr<ixp::IPointerPointProperties> spPointerProperties;
            BOOLEAN isLeftButtonPressed = FALSE;
            BOOLEAN isRightButtonPressed = FALSE;

            IFC(spPointerPoint->get_Properties(&spPointerProperties));
            IFCPTR(spPointerProperties);
            IFC(spPointerProperties->get_IsLeftButtonPressed(&isLeftButtonPressed));
            IFC(spPointerProperties->get_IsRightButtonPressed(&isRightButtonPressed));

            // If the left mouse button was the one pressed...
            if (!m_isLeftButtonPressed && isLeftButtonPressed)
            {
                // Start listening for a mouse drag gesture if dragging is
                // enabled on our parent.
                m_isLeftButtonPressed = TRUE;
                isVisualStateChangePending = TRUE;
                isHandled = TRUE;

                IFC(EnqueuePendingTapInteractionForPointerId(pointerId));

                if (spListView && !spListView->IsInExclusiveInteraction())
                {
                    BOOLEAN dragActive = FALSE;
                    IFC(spListView->GetIsDragEnabled(&dragActive));

                    // ListView does not support D&D but maybe the ListViewItem has CanDrag==True
                    if (!dragActive)
                    {
                        IFC(get_CanDrag(&dragActive))
                    }
                    if (dragActive)
                    {
                        IFC(spPointerPoint->get_Position(&m_lastMouseLeftButtonDownPosition));
                        IFC(BeginCheckingForMouseDrag(spPointer.Get()));
                    }
                }
            }

            // If the right mouse button was the one pressed...
            if (!m_isRightButtonPressed && isRightButtonPressed)
            {
                m_isRightButtonPressed = TRUE;
                isVisualStateChangePending = TRUE;

                //With the new changes to the pen input model we want to initaite drag and drops when the pen barrel button is pressed.
                //This is presented to us in the for of isRightButtonPressed.
                if (pointerDeviceType == mui::PointerDeviceType_Pen)
                {
                    // Start listening for a mouse drag gesture if dragging is
                    // enabled on our parent.
                    m_isRightButtonPressed = TRUE;
                    isVisualStateChangePending = TRUE;

                    IFC(EnqueuePendingTapInteractionForPointerId(pointerId));

                    if (spListView && !spListView->IsInExclusiveInteraction())
                    {
                        BOOLEAN dragActive = FALSE;
                        IFC(spListView->GetIsDragEnabled(&dragActive));

                        // ListView does not support D&D but maybe the ListViewItem has CanDrag==True
                        if (!dragActive)
                        {
                            IFC(get_CanDrag(&dragActive))
                        }
                        if (dragActive)
                        {
                            IFC(spPointerPoint->get_Position(&m_lastMouseLeftButtonDownPosition));
                            IFC(BeginCheckingForMouseDrag(spPointer.Get()));
                        }
                    }
                }
            }
        }
        else if (pointerDeviceType == mui::PointerDeviceType_Touch)
        {
            BOOLEAN shouldSwipeContinue = TRUE;

            isHandled = TRUE;
            IFC(EnqueuePendingTapInteractionForPointerId(pointerId));

            // Clear any pointer over visuals.
            isVisualStateChangePending |= m_shouldEnterPointerOver;
            m_shouldEnterPointerOver = FALSE;

            IFC(spPointerPoint->get_Position(&m_lastTouchDownPosition));

            if (m_tpLastTouchPressedArgs)
            {
                ctl::ComPtr<IPointer> spSwipePointer;

                IFC(m_tpLastTouchPressedArgs->get_Pointer(&spSwipePointer));
                IFC(ListViewBase::AreSamePointer(spSwipePointer.Get(), spPointer.Get(), &shouldSwipeContinue));
            }

            // Touch down.
            SetPtrValue(m_tpLastTouchPressedArgs, pArgs);

            if (shouldSwipeContinue)
            {
                // On phone the checkbox is built into the ListViewBaseItem as a Path.
                // When that checkbox is pressed the phone PointerDown Tilt animation
                // must only apply to it. We determine using the RoutedEvent's OriginalSource
                // if the PointerPressed event occurred from within the Checkbox templatepart here.
                BOOLEAN isOverCheckbox = FALSE;
                IFC(IsOverCheckbox(pArgs, &isOverCheckbox));

                if (isOverCheckbox)
                {
                    isVisualStateChangePending |= !m_inCheckboxPressedForTouch;
                    m_inCheckboxPressedForTouch = TRUE;

                    isVisualStateChangePending |= m_inPressedForTouch;
                    m_inPressedForTouch = FALSE;
                }
                else
                {
                    // start the touch pressed timer
                    // this timer delays going into the Pressed state
                    IFC(StartTouchTimer(TouchTimerState::PressedTimer));

                    isVisualStateChangePending |= m_inCheckboxPressedForTouch;
                    m_inCheckboxPressedForTouch = FALSE;
                }

                if (spListView && DXamlCore::GetCurrent()->GetDragDrop()->GetUseCoreDragDrop())
                {
                    bool isDraggable = false;
                    BOOLEAN canDragItems = FALSE;
                    BOOLEAN canReorderItems = FALSE;
                    AutomaticDragHelper* helper = nullptr;

                    IFC(spListView->get_CanDragItems(&canDragItems));
                    IFC(spListView->get_CanReorderItems(&canReorderItems));

                    isDraggable = canDragItems || canReorderItems;

                    // ListView do not accept drag nor reordering, but maybe the ListViewItem does
                    if (!isDraggable)
                    {
                        BOOLEAN canDrag = FALSE;
                        IFC(get_CanDrag(&canDrag));
                        isDraggable = !!canDrag;
                    }

                    IFC(put_IsDraggable(isDraggable));

                    IFC(ConfigureAutomaticDragHelper(isDraggable /*startDetectingDrag*/));

                    if (isDraggable)
                    {
                        helper = DXamlCore::GetCurrent()->GetAutomaticDragHelper(this);
                        ASSERT(helper);

                        IFC(helper->HandlePointerPressedEventArgs(pArgs));
                    }
                }
            }
            else
            {
                isVisualStateChangePending |= m_inPressedForTouch;
                isVisualStateChangePending |= m_inCheckboxPressedForTouch;
                m_inPressedForTouch = FALSE;
                m_inCheckboxPressedForTouch = FALSE;
            }
        }

        IFC(pArgs->put_Handled(isHandled));
    }

    if (isVisualStateChangePending)
    {
        IFC(ChangeVisualState(true));
    }

Cleanup:
    RRETURN(hr);
}

// Called when the user holds a pointer down over the
// ListViewBaseItem (Holding gesture).
IFACEMETHODIMP ListViewBaseItem::OnHolding(
    _In_ xaml_input::IHoldingRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN wasHolding = m_isHolding;
    mui::PointerDeviceType pointerDeviceType = mui::PointerDeviceType_Touch;

    IFCPTR(pArgs);

    m_isHolding = FALSE;

    IFC(ListViewBaseItemGenerated::OnHolding(pArgs));

    IFC(pArgs->get_PointerDeviceType(&pointerDeviceType));

    if (pointerDeviceType == mui::PointerDeviceType_Touch)
    {
        ixp::HoldingState holdingState = ixp::HoldingState_Canceled;
        AutomaticDragHelper* helper = DXamlCore::GetCurrent()->GetAutomaticDragHelper(this);

        IFC(pArgs->get_HoldingState(&holdingState));
        m_isHolding = (holdingState == ixp::HoldingState_Started);

        if (helper != nullptr)
        {
            IFC(helper->HandleHoldingEventArgs(pArgs));
        }

        // Holding gesture will show drag visual
        {
            ctl::ComPtr<ListViewBase> spListView = GetParentListView();

            if (spListView)
            {
                BOOLEAN canDragItems = FALSE;
                BOOLEAN canReorderItems = FALSE;
                IFC(spListView->get_CanDragItems(&canDragItems));
                IFC(spListView->get_CanReorderItems(&canReorderItems));

                if (canDragItems || canReorderItems)
                {
                    if (m_isHolding)
                    {
                        spListView->SetIsHolding(true);
                        IFC(spListView->SetHoldingItem(this));

                        // Show the drag visual LTE for item on which press and hold detected.
                        // We don't need the LTE to escape clip for reordering, since scaling up is only needed for dragging.
                        if (!canReorderItems)
                        {
                            IFC(CreateHoldingVisual());
                            IFC(TransformHoldingVisual(pArgs));
                        }
                    }
                    else
                    {
                        spListView->SetIsHolding(false);
                        IFC(spListView->ClearHoldingItem(this));
                        IFC(DestroyHoldingVisual());
                    }

                    IFC(spListView->ChangeSelectorItemsVisualState(TRUE));
                }
            }
        }
    }

Cleanup:
    if (wasHolding != m_isHolding)
    {
        HRESULT hr2 = ChangeVisualState(true);
        if (!FAILED(hr))
        {
            hr = hr2;
        }
    }

    RRETURN(hr);
}

// Called when the user releases a pointer over the ListViewBaseItem.
IFACEMETHODIMP ListViewBaseItem::OnPointerReleased(
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN isHandled = FALSE;
    ctl::ComPtr<IPointer> spPointer;
    UINT pointerId = 0;
    mui::PointerDeviceType pointerDeviceType = mui::PointerDeviceType_Touch;
    ctl::ComPtr<ListViewBase> spListView;
    BOOLEAN isSelectionEnabled = FALSE;
    BOOLEAN isVisualStateChangePending = FALSE;
    GestureModes gestureFollowing = GestureModes::None;
    BOOLEAN rightTappedPending = FALSE;

    xaml_controls::ListViewSelectionMode selectionMode = xaml_controls::ListViewSelectionMode_Single;

    bool isLeftClick = false;
    bool isRightClick = false;
    std::wstring gestureId;
    const auto traceSession = Instrumentation::GetXamlTraceSession();

    // Clearing the holding visual on pointer released.
    m_isHolding = FALSE;
    IFC(DestroyHoldingVisual());

    IFCPTR(pArgs);
    IFC(ListViewBaseItemGenerated::OnPointerReleased(pArgs));

    // TODO: Ignore the PointerReleased if we've already "captured" input
    // from another device (via InteractionManager)

    IFC(pArgs->get_Handled(&isHandled));

    IFC(pArgs->get_Pointer(&spPointer));
    IFC(spPointer->get_PointerDeviceType(&pointerDeviceType));
    IFC(spPointer->get_PointerId(&pointerId));

    IFC(static_cast<PointerRoutedEventArgs*>(pArgs)->get_GestureFollowing(&gestureFollowing));
    rightTappedPending = (gestureFollowing == GestureModes::RightTapped);

    spListView = GetParentListView();

    if (spListView)
    {
        IFC(spListView->get_SelectionMode(&selectionMode));
        isSelectionEnabled = selectionMode != xaml_controls::ListViewSelectionMode_None;
        traceSession->GetUniqueId(spListView->m_itemSelectionGestureId);
        gestureId = spListView->m_itemSelectionGestureId;
    }

    // Check if this is a mouse button up
    if (pointerDeviceType == mui::PointerDeviceType_Mouse || pointerDeviceType == mui::PointerDeviceType_Pen)
    {
        BOOLEAN isLeftButtonPressed = FALSE;
        BOOLEAN isRightButtonPressed = FALSE;
        ctl::ComPtr<ixp::IPointerPoint> spPointerPoint;
        ctl::ComPtr<ixp::IPointerPointProperties> spPointerProperties;

        IFC(pArgs->GetCurrentPoint(this, &spPointerPoint));
        IFCPTR(spPointerPoint);
        IFC(spPointerPoint->get_Properties(&spPointerProperties));
        IFCPTR(spPointerProperties);
        IFC(spPointerProperties->get_IsLeftButtonPressed(&isLeftButtonPressed));
        IFC(spPointerProperties->get_IsRightButtonPressed(&isRightButtonPressed));

        // if the mouse left button was the one released...
        if (m_isLeftButtonPressed && !isLeftButtonPressed)
        {
            isLeftClick = true;

            // When pen is released after holding, there will be right-tapped event fired.
            ASSERT(!rightTappedPending || pointerDeviceType == mui::PointerDeviceType_Pen);

            m_isLeftButtonPressed = FALSE;
            isVisualStateChangePending = TRUE;

            if (!isHandled && !rightTappedPending)
            {
                IFC(DoPendingTapInteractionForPointerId(pointerId, &isHandled));
            }
        }

        // if the mouse right button was the one released...
        if (m_isRightButtonPressed && !isRightButtonPressed)
        {
            isRightClick = true;
            m_isRightButtonPressed = FALSE;
            isVisualStateChangePending = TRUE;
        }

        // Terminate any mouse drag gesture tracking.
        IFC(StopCheckingForMouseDrag(spPointer.Get()));
    }
    else if (pointerDeviceType == mui::PointerDeviceType_Touch)
    {
        // no need for a quirk here since m_touchTimerState is only set to
        // PressedTimer in Threshold and beyond
        if (m_touchTimerState == TouchTimerState::PressedTimer)
        {
            m_inPressedForTouch = TRUE;

            // start the early release timer
            IFC(StartTouchTimer(TouchTimerState::ReleasedTimer));
        }
        else
        {
            // Touch up.
            m_inPressedForTouch = FALSE;
        }

        m_inCheckboxPressedForTouch = FALSE;

        AutomaticDragHelper* helper = DXamlCore::GetCurrent()->GetAutomaticDragHelper(this);

        m_tpLastTouchPressedArgs.Clear();

        if (!isHandled &&
            GestureModes::Tapped == gestureFollowing)
        {
            IFC(DoPendingTapInteractionForPointerId(pointerId, &isHandled));
        }

        IFC(ReleasePointerCapture(spPointer.Get()));

        if (helper != nullptr)
        {
            IFC(helper->HandlePointerReleasedEventArgs(pArgs));
        }

        isVisualStateChangePending = TRUE;
    }

    IFC(pArgs->put_Handled(isHandled));

    // If a tap interaction is still pending for this pointer, cancel it
    // (we had our chance to handle it above).
    IFC(CancelPendingTapInteractionForPointerId(pointerId));

    if (isVisualStateChangePending)
    {
        IFC(ChangeVisualState(true));
    }

Cleanup:
    RRETURN(hr);
}

// Register the given pointer ID as having a pending tap action. The action will be performed on the next PointerReleased
// for the pointer, unless it is canceled i.e. by PointerExited.
_Check_return_ HRESULT ListViewBaseItem::EnqueuePendingTapInteractionForPointerId(_In_ UINT pointerId)
{
    HRESULT hr = S_OK;

    std::list<UINT>::iterator foundIterator = std::find(m_pendingTapPointerIDs.begin(), m_pendingTapPointerIDs.end(), pointerId);
    if (foundIterator == m_pendingTapPointerIDs.end())
    {
        m_pendingTapPointerIDs.push_back(pointerId);
    }

    RRETURN(hr);//RRETURN_REMOVAL
}

// If the given pointer ID has a pending tap interaction, do the action and remove the pointer id from the pending
// interaction list.
_Check_return_ HRESULT ListViewBaseItem::DoPendingTapInteractionForPointerId(_In_ UINT pointerId, _Out_ BOOLEAN* pIsHandled)
{
    HRESULT hr = S_OK;

    *pIsHandled = FALSE;
    std::list<UINT>::iterator foundIterator = std::find(m_pendingTapPointerIDs.begin(), m_pendingTapPointerIDs.end(), pointerId);
    if (foundIterator != m_pendingTapPointerIDs.end())
    {
        m_pendingTapPointerIDs.erase(foundIterator);
        IFC(DoTapInteraction(pIsHandled));
    }

Cleanup:
    RRETURN(hr);
}

// Cancel a pending tap action for the given pointer ID.
_Check_return_ HRESULT ListViewBaseItem::CancelPendingTapInteractionForPointerId(_In_ UINT pointerId)
{
    HRESULT hr = S_OK;

    std::list<UINT>::iterator foundIterator = std::find(m_pendingTapPointerIDs.begin(), m_pendingTapPointerIDs.end(), pointerId);
    if (foundIterator != m_pendingTapPointerIDs.end())
    {
        m_pendingTapPointerIDs.erase(foundIterator);
    }

    RRETURN(hr);//RRETURN_REMOVAL
}

// Called when a pointer enters a ListViewBaseItem.
IFACEMETHODIMP ListViewBaseItem::OnPointerEntered(
    _In_ IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IPointer> spPointer;
    mui::PointerDeviceType pointerDeviceType = mui::PointerDeviceType_Touch;

    IFCPTR(pArgs);
    IFC(ListViewBaseItemGenerated::OnPointerEntered(pArgs));

    // Only update m_shouldEnterPointerOver if the pointer type isn't touch
    IFC(pArgs->get_Pointer(&spPointer));
    IFC(spPointer->get_PointerDeviceType(&pointerDeviceType));
    if (pointerDeviceType != mui::PointerDeviceType_Touch)
    {
        m_shouldEnterPointerOver = TRUE;
        IFC(ChangeVisualState(true));
    }

Cleanup:
    RRETURN(hr);
}


IFACEMETHODIMP ListViewBaseItem::OnPointerCanceled(
    _In_ IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IPointer> spPointer;
    UINT pointerId = 0;

    IFC(ListViewBaseItemGenerated::OnPointerCanceled(pArgs));

    IFC(pArgs->get_Pointer(&spPointer));
    IFC(spPointer->get_PointerId(&pointerId));

    IFC(CancelPendingTapInteractionForPointerId(pointerId));

Cleanup:
    RRETURN(hr);
}

// Called when a pointer leaves a ListViewBaseItem.
IFACEMETHODIMP ListViewBaseItem::OnPointerExited(
    _In_ IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IPointer> spPointer;
    UINT pointerId = 0;
    mui::PointerDeviceType pointerDeviceType = mui::PointerDeviceType_Touch;

    IFCPTR(pArgs);
    IFC(ListViewBaseItemGenerated::OnPointerExited(pArgs));

    IFC(pArgs->get_Pointer(&spPointer));
    IFC(spPointer->get_PointerId(&pointerId));
    IFC(spPointer->get_PointerDeviceType(&pointerDeviceType));

    IFC(CancelPendingTapInteractionForPointerId(pointerId));

    // Clear pointer over regardless of input method
    m_shouldEnterPointerOver = FALSE;

    if (pointerDeviceType != mui::PointerDeviceType_Touch)
    {
        m_isLeftButtonPressed = FALSE;
        m_isRightButtonPressed = FALSE;
        IFC(ChangeVisualState(true));
    }
    else
    {
        // no need for a quirk here since m_touchTimerState is only set to
        // a value other than NoTimer in Threshold and beyond (with touch input)
        // if the value is not NoTimer, it means that it could either be Pressed or Released
        // in the case of Pressed, it means that the user started the press and quickly exited the item
        // -> we start the ReleasedTimer to show a PressedState
        // in the case of Released, we do nothing, m_inPressedForTouch is already true
        // -> we wait for the released timer to expire and set it back to false
        if (m_touchTimerState == TouchTimerState::NoTimer)
        {
            m_inPressedForTouch = FALSE;
        }
        else if (m_touchTimerState == TouchTimerState::PressedTimer)
        {
            m_inPressedForTouch = TRUE;

            // start the early release timer
            IFC(StartTouchTimer(TouchTimerState::ReleasedTimer));
        }

        // Touch up.
        m_tpLastTouchPressedArgs.Clear();
        m_inCheckboxPressedForTouch = FALSE;
        IFC(ChangeVisualState(true));
    }

Cleanup:
    RRETURN(hr);
}

// Called when a pointer moves within a ListViewBaseItem.
IFACEMETHODIMP ListViewBaseItem::OnPointerMoved(
    _In_ IPointerRoutedEventArgs* pArgs)
{
    ctl::ComPtr<IPointer> spPointer;
    mui::PointerDeviceType pointerDeviceType = mui::PointerDeviceType_Touch;
    wf::Point newMousePosition;

    IFCPTR_RETURN(pArgs);
    IFC_RETURN(ListViewBaseItemGenerated::OnPointerMoved(pArgs));

    BOOLEAN isHandled = FALSE;
    IFC_RETURN(pArgs->get_Handled(&isHandled));
    IFC_RETURN(pArgs->get_Pointer(&spPointer));
    IFC_RETURN(spPointer->get_PointerDeviceType(&pointerDeviceType));
    if (!isHandled)
    {
        // Our behavior is different between mouse and touch.
        // It's up to us to detect mouse drag gestures - if we
        // detect one here, tell our parent to start a drag drop.
        if (pointerDeviceType == mui::PointerDeviceType_Mouse || pointerDeviceType == mui::PointerDeviceType_Pen)
        {
            ctl::ComPtr<ListViewBase> spListView = GetParentListView();

            // Desired behavior when a user touches a container that currently
            // has a mouse hover visual is to clear the hover visual, then re-instate
            // the hover visual only when the mouse is moved.
            // We clear m_shouldEnterPointerOver when the item is touched. We re-instate
            // it here.
            if (!m_shouldEnterPointerOver && !m_inPressedForTouch)
            {
                m_shouldEnterPointerOver = TRUE;
                IFC_RETURN(ChangeVisualState(true));
            }

            // If another item is dragging, prevent a drag from happening.
            if (spListView && spListView->IsInExclusiveInteraction())
            {
                IFC_RETURN(StopCheckingForMouseDrag(spPointer.Get()));
            }
            else
            {
                ctl::ComPtr<ixp::IPointerPoint> spPointerPoint;

                IFC_RETURN(pArgs->GetCurrentPoint(this, &spPointerPoint));
                IFCPTR_RETURN(spPointerPoint);

                IFC_RETURN(spPointerPoint->get_Position(&newMousePosition));

                boolean isInContact = false;
                IFC_RETURN(spPointerPoint->get_IsInContact(&isInContact));

                if (ShouldStartMouseDrag(newMousePosition) && (pointerDeviceType != mui::PointerDeviceType_Pen || isInContact))
                {
                    wf::Point visualOffset = {-m_lastMouseLeftButtonDownPosition.X, -m_lastMouseLeftButtonDownPosition.Y};
                    BOOLEAN didStartDrag = FALSE;
                    IFC_RETURN(StopCheckingForMouseDrag(spPointer.Get()));

                    // TODO: We don't get a MouseLeave on this
                    // element after a drag and drop.  Manually reset the mouse over
                    // state as a workaround. Note that this isn't quite right yet (user
                    // might drop really close to the drag start point). Should be fixed
                    // during stabilization.
                    m_shouldEnterPointerOver = FALSE;
                    IFC_RETURN(ChangeVisualState(true));

                    // System will render the default visual for us when Core Drag Drop API is used
                    IFC_RETURN(EnsureDragDropVisual(visualOffset, DXamlCore::GetCurrent()->GetDragDrop()->GetUseSystemDefaultVisual()));

                    // Try the drag. Continuing our previous gesture is not applicable here.
                    IFC_RETURN(TryStartDrag(
                        spPointer.Get(),
                        spPointerPoint.Get(),
                        m_lastMouseLeftButtonDownPosition,
                        nullptr, // pShouldPreviousGestureContinue
                        &didStartDrag));

                    if (!didStartDrag)
                    {
                        m_spDragDropVisual = nullptr;

                        // ListView did not start Drag, maybe the LVI will
                        IFC_RETURN(StartDragIfEnabled(spPointerPoint.Get()));
                    }

                }
            }
        }
    }

    return S_OK;
}

// Called when the ListViewBaseItem or its children lose pointer capture.
IFACEMETHODIMP ListViewBaseItem::OnPointerCaptureLost(
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IPointer> spPointer;
    UINT pointerId = 0;
    mui::PointerDeviceType pointerDeviceType = mui::PointerDeviceType_Touch;
    BOOLEAN isVisualStateChangePending = FALSE;

    IFC(ListViewBaseItemGenerated::OnPointerCaptureLost(pArgs));

    IFC(pArgs->get_Pointer(&spPointer));
    IFC(spPointer->get_PointerId(&pointerId));

    IFC(CancelPendingTapInteractionForPointerId(pointerId));

    IFC(spPointer->get_PointerDeviceType(&pointerDeviceType));
    if (pointerDeviceType == mui::PointerDeviceType_Mouse || pointerDeviceType == mui::PointerDeviceType_Pen)
    {
        // We're not necessarily going to get a PointerReleased on capture lost, so reset this flag here.
        if (m_isLeftButtonPressed || m_isRightButtonPressed || m_shouldEnterPointerOver)
        {
            m_isLeftButtonPressed = FALSE;
            m_isRightButtonPressed = FALSE;
            m_shouldEnterPointerOver = FALSE;
            isVisualStateChangePending = TRUE;
        }

    }
    else if (pointerDeviceType == mui::PointerDeviceType_Touch)
    {
        AutomaticDragHelper* helper = DXamlCore::GetCurrent()->GetAutomaticDragHelper(this);

        // stop the touch timer
        IFC(StopTouchTimer());

        if (m_inPressedForTouch || m_inCheckboxPressedForTouch || m_shouldEnterPointerOver)
        {
            m_inPressedForTouch = FALSE;
            m_inCheckboxPressedForTouch = FALSE;
            m_shouldEnterPointerOver = FALSE;
            isVisualStateChangePending = TRUE;
        }

        if (helper)
        {
            IFC(helper->HandlePointerCaptureLostEventArgs(pArgs));
        }
        m_tpLastTouchPressedArgs.Clear();
    }

    if (isVisualStateChangePending)
    {
        IFC(ChangeVisualState(true));
    }

Cleanup:
    RRETURN(hr);
}

// Call our ListViewBase's Primary or Secondary interaction gesture, depending on whether or not the
// control key is pressed.
_Check_return_ HRESULT ListViewBaseItem::DoTapInteraction(_Out_ BOOLEAN* pIsHandled)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<ListViewBase> spListView = GetParentListView();

    *pIsHandled = FALSE;

    if (spListView)
    {
        wsy::VirtualKeyModifiers modifiers = wsy::VirtualKeyModifiers_None;

        // A regular click is considered a primary input gesture
        // (equivalent to a tap) but Ctrl+Click is a secondary input
        // gesture so we check modifiers to
        // determine how to route the interaction notification

        IFC(ListViewBase::GetKeyboardModifiers(&modifiers));
        if (!IsFlagSet(modifiers, wsy::VirtualKeyModifiers_Control))
        {
            IFC(spListView->OnItemPrimaryInteractionGesture(this, FALSE, pIsHandled));
        }
        else
        {
            IFC(spListView->OnItemSecondaryInteractionGesture(this, FALSE, pIsHandled));
        }
    }

    IFC(ChangeVisualState(true));

Cleanup:
    RRETURN(hr);
}

// Called when a pointer makes a right-tap gesture on a ListViewBaseItem.
IFACEMETHODIMP ListViewBaseItem::OnRightTapped(
    _In_ IRightTappedRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN isHandled = FALSE;
    mui::PointerDeviceType pointerDeviceType = mui::PointerDeviceType_Touch;

    IFCPTR(pArgs);
    IFC(ListViewBaseItemGenerated::OnRightTapped(pArgs));

    IFC(pArgs->get_Handled(&isHandled));
    IFC(pArgs->get_PointerDeviceType(&pointerDeviceType));

    // Only do interaction for right click, not press-and-hold (the latter will be done in PointerReleased).
    if (!isHandled && pointerDeviceType != mui::PointerDeviceType_Touch)
    {
        ctl::ComPtr<ListViewBase> spListView = GetParentListView();

        if (spListView)
        {
            IFC(spListView->FocusItem(xaml::FocusState_Programmatic, this));
            IFC(pArgs->put_Handled(isHandled));
        }
    }

    // Touch up.
    m_inPressedForTouch = FALSE;
    m_inCheckboxPressedForTouch = FALSE;
    IFC(ChangeVisualState(true));

Cleanup:
    RRETURN(hr);
}

// Called when the value of the IsEnabled property changes.
_Check_return_ HRESULT ListViewBaseItem::OnIsEnabledChanged(_In_ IsEnabledChangedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN isEnabled = FALSE;

    IFC(ListViewBaseItemGenerated::OnIsEnabledChanged(pArgs));

    IFC(get_IsEnabled(&isEnabled));
    if (!isEnabled)
    {
        IFC(ClearStateFlags());
    }
    IFC(ChangeVisualState(true));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ListViewBaseItem::OnContextRequestedImpl(
    _In_ xaml_input::IContextRequestedEventArgs* pArgs)
{
    IFC_RETURN(ShowContextFlyout(pArgs, this));

    boolean handled = true;
    IFC_RETURN(pArgs->get_Handled(&handled));
    if (!handled)
    {
        // LVI did not have a ContextFlyout property set, try the parent LV
        // If LV has a ContextFlyout set, use that flyout to show in the current LVI
        ctl::ComPtr<ListViewBase> spListView = GetParentListView();
        if (spListView)
        {
            IFC_RETURN(ShowContextFlyout(pArgs, spListView.Get()));
        }
    }

    return S_OK;
}

// Called when the ListViewBaseItem receives focus.
IFACEMETHODIMP ListViewBaseItem::OnGotFocus(
    _In_ IRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN hasFocus = FALSE;
    ctl::ComPtr<IInspectable> spOriginalSource;
    ctl::ComPtr<IUIElement> spFocusedElement;
    BOOLEAN notifyParent = FALSE;
    xaml::FocusState focusState = xaml::FocusState_Unfocused;

    IFCPTR(pArgs);
    IFC(ListViewBaseItemGenerated::OnGotFocus(pArgs));

    IFC(HasFocus(&hasFocus));
    IFC(pArgs->get_OriginalSource(&spOriginalSource));
    if (spOriginalSource)
    {
        spFocusedElement = spOriginalSource.AsOrNull<IUIElement>();
        if (spFocusedElement)
        {
            IFC(spFocusedElement->get_FocusState(&focusState));
        }
         notifyParent = (xaml::FocusState_Keyboard == focusState ||
                         xaml::FocusState_Programmatic == focusState) ? TRUE : FALSE;
        // In mouse and touch we should pass notifyParent as false so the listviewbaseitem does not grab the focus itself.
    }
    IFC(FocusChanged(hasFocus, notifyParent));

Cleanup:
    RRETURN(hr);
}

// Called when the ListViewBaseItem loses focus.
IFACEMETHODIMP ListViewBaseItem::OnLostFocus(
    _In_ IRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN hasFocus = FALSE;
    ctl::ComPtr<IInspectable> spOriginalSource;

    IFCPTR(pArgs);
    IFC(ListViewBaseItemGenerated::OnLostFocus(pArgs));

    // clear the KeyboardPressed boolean
    m_isKeyboardPressed = FALSE;

    IFC(HasFocus(&hasFocus));
    IFC(pArgs->get_OriginalSource(&spOriginalSource));
    // Original source is always the listviewbaseitem or one of the controls in it.
    IFC(FocusChanged(hasFocus, TRUE));

Cleanup:
    RRETURN(hr);
}

// Update our parent ListView when focus changes so it can manage the currently
// focused item.
_Check_return_ HRESULT ListViewBaseItem::FocusChanged(
    _In_ BOOLEAN hasFocus,
    _In_ BOOLEAN notifyParent)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<ListViewBase> spListView = GetParentListView();

    // Win 8 RTM and higher - focus transfer is handled through FocusManager callback.
    // Just inform the parent selector that we got focus.
    if (spListView)
    {
        if (hasFocus)
        {
            IFC(spListView->ItemFocused(this));
        }
        else
        {
            IFC(spListView->ItemUnfocused(this));
        }
    }

    IFC(ChangeVisualState(true));

Cleanup:
    RRETURN(hr);
}

// Creates m_spDragDropVisual, if necessary.
_Check_return_ HRESULT ListViewBaseItem::EnsureDragDropVisual(
    _In_ wf::Point visualOffset,
    _In_ bool useSystemDefaultVisual)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IUIElement> spDragVisual;

    if (!m_spDragDropVisual)
    {
        if (!useSystemDefaultVisual)
        {
            IFC(GetDragVisual(&spDragVisual));
            IFCPTR(spDragVisual);
        }
        IFC(DragDropVisual::CreateInstance(spDragVisual.Get(), visualOffset, &m_spDragDropVisual));
        IFC(m_spDragDropVisual->Show());
    }

Cleanup:
    RRETURN(hr);
}

// Create a holding visual LTE that will make the ListViewItem bigger
_Check_return_ HRESULT ListViewBaseItem::CreateHoldingVisual()
{
    ASSERT(!m_tpHoldingVisual);

    ctl::ComPtr<IUIElement> spTarget;
    IFC_RETURN(GetDragVisual(&spTarget));
    IFCPTR_RETURN(spTarget);

    xref_ptr<CUIElement> pHoldingVisualContainer;
    IFC_RETURN(CoreImports::LayoutTransitionElement_Create(
        DXamlCore::GetCurrent()->GetHandle(),
        spTarget.Cast<UIElement>()->GetHandle(),
        nullptr /* pParentElement */,
        true /* isAbsolutelyPositioned */,
        pHoldingVisualContainer.ReleaseAndGetAddressOf()));

    ctl::ComPtr<DependencyObject> spLTEAsDO;
    IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(pHoldingVisualContainer.get(), &spLTEAsDO));

    ctl::ComPtr<IUIElement> spLTEAsIUIE;
    IFC_RETURN(spLTEAsDO.CopyTo(spLTEAsIUIE.ReleaseAndGetAddressOf()));

    SetPtrValue(m_tpHoldingVisual, spLTEAsIUIE.Get());

    return S_OK;
}

_Check_return_ HRESULT ListViewBaseItem::ClearHoldingState()
{
    IFC_RETURN(DestroyHoldingVisual());
    m_isHolding = false;
    IFC_RETURN(ChangeVisualState(true));
    return S_OK;
}

// Destroy the holding visual LTE
_Check_return_ HRESULT ListViewBaseItem::DestroyHoldingVisual()
{
    if (m_tpHoldingVisual)
    {
        ctl::ComPtr<IUIElement> spTarget;
        IFC_RETURN(GetDragVisual(&spTarget));
        IFCPTR_RETURN(spTarget);

        IFC_RETURN(CoreImports::LayoutTransitionElement_Destroy(
            DXamlCore::GetCurrent()->GetHandle(),
            spTarget.Cast<UIElement>()->GetHandle(),
            nullptr /* pParentElement */,
            static_cast<UIElement*>(m_tpHoldingVisual.Get())->GetHandle()));

        m_tpHoldingVisual.Clear();
    }

    return S_OK;
}

// Holding visual will be transformed to size of 1.05, opacity 0.8, and positioned at the same place as the original ListViewItem
_Check_return_ HRESULT ListViewBaseItem::TransformHoldingVisual(
    _In_ xaml_input::IHoldingRoutedEventArgs* pArgs)
{
    if (m_tpHoldingVisual)
    {
        ctl::ComPtr<IGeneralTransform> spToRoot;
        IFC_RETURN(TransformToVisual(nullptr, &spToRoot));

        // Set the RenderTransformOrigin as center of the ListViewItem & Opacity as 0.8
        ctl::ComPtr<ITransform> spToRootAsITransform = nullptr;
        IFC_RETURN(spToRoot.As<ITransform>(&spToRootAsITransform));
        IFC_RETURN(m_tpHoldingVisual->put_RenderTransform(spToRootAsITransform.Get()));
        IFC_RETURN(m_tpHoldingVisual->put_RenderTransformOrigin(s_center));
        IFC_RETURN(m_tpHoldingVisual->put_IsHitTestVisible(FALSE));
        IFC_RETURN(m_tpHoldingVisual->put_Opacity(s_holdingVisualOpacity));

        // Apply a MatrixTransform for Scale and the visual offset
        ctl::ComPtr<MatrixTransform> spHoldingVisualTransform = nullptr;
        IFC_RETURN(spToRoot.As<MatrixTransform>(&spHoldingVisualTransform));

        xaml_media::Matrix matrix;
        IFC_RETURN(spHoldingVisualTransform->get_Matrix(&matrix));

        xaml::FlowDirection direction = xaml::FlowDirection::FlowDirection_LeftToRight;
        IFC_RETURN(get_FlowDirection(&direction));
        if (direction == xaml::FlowDirection::FlowDirection_RightToLeft)
        {
            // For RTL, the item is offset by the width of the LVI
            double width;
            IFC_RETURN(get_ActualWidth(&width));
            matrix.OffsetX -= width;
        }

        matrix.M11 *= s_holdingVisualScale;
        matrix.M22 *= s_holdingVisualScale;

        IFC_RETURN(spHoldingVisualTransform->put_Matrix(matrix));
    }

    return S_OK;
}

// Sets the value to display as the dragged items count.
_Check_return_ HRESULT ListViewBaseItem::SetDragItemsCountDisplay(
    _In_ UINT dragItemsCount)
{
    HRESULT hr = S_OK;
    CListViewBaseItemChrome* pChrome = static_cast<CContentControl*>(this->GetHandle())->GetGridViewItemChromeNoRef();

    if (pChrome)
    {
        IFC(pChrome->SetDragItemsCount(static_cast<XUINT32>(dragItemsCount)));
    }

Cleanup:
    RRETURN(hr);
}

// Change to the correct visual state for the ListViewItem using
// an existing VisualStateManagerBatchContext
_Check_return_ HRESULT ListViewBaseItem::ChangeVisualStateWithContext(
    _In_ VisualStateManagerBatchContext *pContext,
    // true to use transitions when updating the visual state, false
    // to snap directly to the new visual state.
    _In_ bool bUseTransitions)
{
    HRESULT hr = S_OK;

    IFC(ListViewBaseItemGenerated::ChangeVisualStateWithContext(pContext, bUseTransitions));

    // new VisualState changing code for the new style
    IFC(ChangeVisualStateWithContextNewStyle(pContext, bUseTransitions));

Cleanup:
    return hr;
}

// Called when the NoSelectionHint VisualState's Storyboard completes.
_Check_return_ HRESULT ListViewBaseItem::OnNoSelectionHintStoryboardCompleted(
    _In_ IInspectable* pSender,
    _In_ IInspectable* pArgs)
{
    m_inSelectionHintState = FALSE;
    RRETURN(UpdateTopmost());
}

// Called when the NoReorderingHint VisualState's Storyboard completes.
_Check_return_ HRESULT ListViewBaseItem::OnNoReorderHintStoryboardCompleted(
    _In_ IInspectable* pSender,
    _In_ IInspectable* pArgs)
{
    m_inReorderHintState = FALSE;
    RRETURN(UpdateTopmost());
}

// Sets this ListViewBaseItem to the appropriate Z-Order,
// based on whether it is in a Hint state.
_Check_return_ HRESULT ListViewBaseItem::UpdateTopmost()
{
    HRESULT hr = S_OK;

    if (m_inSelectionHintState || m_inReorderHintState)
    {
        IFC(MakeTopmost());
    }
    else
    {
        IFC(ClearTopmost());
    }

Cleanup:
    RRETURN(hr);
}

// Raises this ListViewBaseItem above its siblings.
_Check_return_ HRESULT ListViewBaseItem::MakeTopmost()
{
    HRESULT hr = S_OK;

    if (!m_isTopmost)
    {
        // TODO: Use a Storyboard to avoid overwriting a user-set value. Work item: 102347.
        ctl::ComPtr<IActivationFactory> spCanvasActivationFactory;
        ctl::ComPtr<xaml_controls::ICanvasStatics> spCanvasStatics;
        spCanvasActivationFactory.Attach(ctl::ActivationFactoryCreator<DirectUI::CanvasFactory>::CreateActivationFactory());
        IFC(spCanvasActivationFactory.As<xaml_controls::ICanvasStatics>(&spCanvasStatics));

        IFC(spCanvasStatics->SetZIndex(this, LISTVIEWBASEITEM_INTERACTION_ZINDEX));

        m_isTopmost = TRUE;
    }

Cleanup:
    RRETURN(hr);
}

// Restores this ListViewBaseItem to its default Z-Order.
_Check_return_ HRESULT ListViewBaseItem::ClearTopmost()
{
    HRESULT hr = S_OK;

    if (m_isTopmost)
    {
        // TODO: Use a Storyboard to avoid overwriting a user-set value. Work item: 102347.
        ctl::ComPtr<IActivationFactory> spCanvasActivationFactory;
        ctl::ComPtr<xaml_controls::ICanvasStatics> spCanvasStatics;
        spCanvasActivationFactory.Attach(ctl::ActivationFactoryCreator<DirectUI::CanvasFactory>::CreateActivationFactory());
        IFC(spCanvasActivationFactory.As<xaml_controls::ICanvasStatics>(&spCanvasStatics));

        IFC(spCanvasStatics->SetZIndex(this, LISTVIEWBASEITEM_REST_ZINDEX));

        m_isTopmost = FALSE;
    }

Cleanup:
    RRETURN(hr);
}

// determines if this element should be transitioned using the passed in transition
IFACEMETHODIMP ListViewBaseItem::GetCurrentTransitionContext(
    _In_ INT layoutTickId,
    _Out_ ThemeTransitionContext* pReturnValue)
{
    bool handled = false;
    ctl::ComPtr<IDependencyObject> spParentAsDO;

    *pReturnValue = ThemeTransitionContext::None;

    IFC_RETURN(VisualTreeHelper::GetParentStatic(this, &spParentAsDO));
    if (spParentAsDO)
    {
        auto spContextProviderPanel = spParentAsDO.AsOrNull<IChildTransitionContextProvider>();
        if (spContextProviderPanel)
        {
            IFC_RETURN(spContextProviderPanel->GetChildTransitionContext(this, layoutTickId, pReturnValue));
            handled = true;
        }
    }

    if (!handled)
    {
        ctl::ComPtr<IItemsControl> spIItemsControl;

        IFC_RETURN(ItemsControl::ItemsControlFromItemContainer(this, true, &spIItemsControl));

        if (spIItemsControl)
        {
            IFC_RETURN(spIItemsControl.Cast<ItemsControl>()->GetCurrentTransitionContext(layoutTickId, pReturnValue));
        }
        else
        {
            // In case of Reset, by default we will return ContentTransition
            // There is bug that in case of reset, the parent ListView is null, and we can get correct transition from
            // the Parent
            // bug: 103077
            *pReturnValue = ThemeTransitionContext::ContentTransition;
        }
    }

    // in the case where the list view is in live reorder mode
    // we want to simulate the add theme transition
    // this will include portalling
    if (*pReturnValue == ThemeTransitionContext::None)
    {
        ctl::ComPtr<ListViewBase> spListView = GetParentListView();

        if (spListView && spListView->IsInLiveReorder())
        {
            *pReturnValue = ThemeTransitionContext::SingleAddGrid;
        }
    }

    return S_OK;
}


// determines if mutations are going fast
IFACEMETHODIMP ListViewBaseItem::IsCollectionMutatingFast(
    _Out_ BOOLEAN* pReturnValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IDependencyObject> spParentAsDO;

    *pReturnValue = FALSE;

    IFC(VisualTreeHelper::GetParentStatic(this, &spParentAsDO));
    if (spParentAsDO)
    {
        auto spContextProviderPanel = spParentAsDO.AsOrNull<IChildTransitionContextProvider>();
        if (spContextProviderPanel)
        {
            IFC(spContextProviderPanel->IsCollectionMutatingFast(pReturnValue));
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ListViewBaseItem::GetDropOffsetToRoot(
    _Out_ wf::Point* pReturnValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<ListViewBase> spListView;

    IFCPTR(pReturnValue);
    pReturnValue->X = 0;
    pReturnValue->Y = 0;
    spListView = GetParentListView();

    if (spListView)
    {
        IFC(spListView->GetDropOffsetToRoot(pReturnValue));
    }

Cleanup:
    RRETURN(hr);
}

// Begin tracking the mouse cursor in order to fire a drag start if the pointer
// moves a certain distance away from m_lastMouseLeftButtonDownPosition.
_Check_return_ HRESULT ListViewBaseItem::BeginCheckingForMouseDrag(
    _In_ IPointer* pPointer)
{
    IFCPTR_RETURN(pPointer);
    IFC_RETURN(CapturePointer(pPointer, &m_isCheckingForMouseDrag));

    return S_OK;
}

// Stop tracking the mouse cursor.
_Check_return_ HRESULT ListViewBaseItem::StopCheckingForMouseDrag(
    _In_ IPointer* pPointer)
{
    IFCPTR_RETURN(pPointer);

    // Do not call ReleasePointerCapture() more times than we called CapturePointer()
    if (m_isCheckingForMouseDrag)
    {
        m_isCheckingForMouseDrag = FALSE;
        IFC_RETURN(ReleasePointerCapture(pPointer));
    }

    return S_OK;
}

// Return TRUE if we're tracking the mouse and newMousePosition is outside the drag
// rectangle centered at m_lastMouseLeftButtonDownPosition (see IsOutsideDragRectangle).
bool ListViewBaseItem::ShouldStartMouseDrag(
    _In_ wf::Point newMousePosition)
{
    return m_isCheckingForMouseDrag && IsOutsideDragRectangle(newMousePosition, m_lastMouseLeftButtonDownPosition);
}

// Notify our parent ListViewBase (if any) that the user has requested
// a drag operation with the specified input pointer. Returns whether or
// not our previous gesture should continue.
_Check_return_ HRESULT ListViewBaseItem::TryStartDrag(
    _In_ IPointer* pointer,
    _In_ ixp::IPointerPoint* pointerPoint,
    _In_ wf::Point dragStartPoint,
    _Out_opt_ BOOLEAN* pShouldPreviousGestureContinue,
    _Out_ BOOLEAN* pDidStartDrag)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<ListViewBase> spListView;
    BOOLEAN shouldPreviousGestureContinue = TRUE;

    if (pShouldPreviousGestureContinue)
    {
        *pShouldPreviousGestureContinue = FALSE;
    }

    *pDidStartDrag = FALSE;

    spListView = GetParentListView();
    if (spListView)
    {
        // Check that the ListView accepts reorder or drag
        BOOLEAN canListViewDragOrReorderItems = FALSE;
        IFC(spListView->GetIsDragEnabled(&canListViewDragOrReorderItems));

        if (canListViewDragOrReorderItems)
        {
            // Take a ref to the pointer - otherwise it will lose its last ref if ReleasePointerCapture is called below.
            ctl::ComPtr<IPointer> spDragPointer = pointer;
            // Can't close aroud 'this'. Workaround.
            ctl::ComPtr<ISelectorItem> spThis = this;
            BOOLEAN wasCanceled = FALSE;
            BOOLEAN releasedPointerCapture = FALSE;
            CListViewBaseItemChrome* pChrome = static_cast<CContentControl*>(this->GetHandle())->GetGridViewItemChromeNoRef();

            IFCEXPECT(m_spDragDropVisual);

            // Start the drag drop and hand off our DragDropVisual.
            m_isStartingDrag = TRUE; // Make sure we don't release any viewport on CaptureLost. We need it.

            // Minor chrome hack: The chrome needs to set the opacity of the drag visual. Sadly, the chrome can't do it itself
            // (because of a lack of targetable layers). So, we tell the visual the proper opacity.
            if (pChrome)
            {
                IFC(m_spDragDropVisual->SetOpacity(pChrome->GetDragOpacity()));
            }

            IFC(spListView->OnDragGesture(pointer, pointerPoint, this, m_spDragDropVisual.Get(), dragStartPoint,
                [&spDragPointer, &spThis, &releasedPointerCapture]() -> HRESULT
                {
                    // Called when we're about to start the drag. NOTE! Can still fail after this and call the cancel function below.
                    releasedPointerCapture = TRUE;
                    RRETURN(spThis.Cast<SelectorItem>()->ReleasePointerCapture(spDragPointer.Get()));
                },
                &wasCanceled));

            if (wasCanceled)
            {
                if (releasedPointerCapture)
                {
                    // If we fail, we shouldn't continue the previous gesture.
                    IFC(CapturePointer(spDragPointer.Get(), &shouldPreviousGestureContinue));
                }
            }
            else
            {
                shouldPreviousGestureContinue = FALSE;
                *pDidStartDrag = TRUE;
            }
        }
    }

    // Note that because of issues with GestureRecognizer (see bug Win8: 937812), we can't correctly resume a swipe (if that's
    // the gesture that got us here). Once the recognizer decides it's in a Rearrange, there's no going back. As a result, this flag
    // isn't really necessary, but still exists because removing it and associated code posed too much risk at the end of Win8 development.
    if (pShouldPreviousGestureContinue)
    {
        *pShouldPreviousGestureContinue = shouldPreviousGestureContinue;
    }

Cleanup:
    m_isStartingDrag = FALSE;
    RRETURN(hr);
}

// Obtains the drag visual for this item.
_Check_return_ HRESULT ListViewBaseItem::GetDragVisual(
    _Outptr_ IUIElement** ppDragVisual)
{
    HRESULT hr = S_OK;

    IFCPTR(ppDragVisual);

    if (m_tpContentContainer)
    {
        ctl::ComPtr<IUIElement> spContentContainer = m_tpContentContainer.Get();
        IFC(spContentContainer.CopyTo(ppDragVisual));
    }
    else
    {
        // Try to get our first child. We generally don't want the ListViewBaseItem
        // to be the drag visual, since that will cause problems with input processing
        // when the drag visual becomes an LTE.
        ctl::ComPtr<IDependencyObject> spChildAsIDO;
        ctl::ComPtr<IUIElement> spChildAsIUIE;
        IFC(VisualTreeHelper::GetChildStatic(this, 0, &spChildAsIDO));
        spChildAsIUIE = spChildAsIDO.AsOrNull<IUIElement>();

        if (spChildAsIUIE)
        {
            *ppDragVisual = spChildAsIUIE.Detach();
        }
        else
        {
            // Fall back to using the entire container if we don't have any children.
            ctl::ComPtr<IUIElement> spThisAsUIE = this;
            IFC(spThisAsUIE.CopyTo(ppDragVisual));
        }
    }

Cleanup:
    RRETURN(hr);
}


// Returns TRUE if testPoint is outside of the rectangle
// defined by the SM_CXDRAG and SM_CYDRAG system metrics and
// dragRectangleCenter.
bool ListViewBaseItem::IsOutsideDragRectangle(
    _In_ wf::Point testPoint,
    _In_ wf::Point dragRectangleCenter)
{
    XDOUBLE dx = abs(testPoint.X - dragRectangleCenter.X);
    XDOUBLE dy = abs(testPoint.Y - dragRectangleCenter.Y);

    XDOUBLE maxDx = GetSystemMetrics(SM_CXDRAG);
    XDOUBLE maxDy = GetSystemMetrics(SM_CYDRAG);

    maxDx *= LISTVIEWBASEITEM_MOUSE_DRAG_THRESHOLD_MULTIPLIER;
    maxDy *= LISTVIEWBASEITEM_MOUSE_DRAG_THRESHOLD_MULTIPLIER;

    return (dx > maxDx || dy > maxDy);
}

// Determinese if the RoutedEventArgs originated within
// the checkbox template part by walking up the visual tree
// of the RoutedEventArg's OriginalSource element until either
// the Checkbox or Content container is encountered.
// Supports a phone-specific scenario.
_Check_return_ HRESULT ListViewBaseItem::IsOverCheckbox(
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs,
    _Out_ BOOLEAN* pIsInCheckboxContainer) const
{
    HRESULT hr = S_OK;

    ctl::ComPtr<IRoutedEventArgs> spArgsAsRoutedArgs;
    ctl::ComPtr<IInspectable> spOriginalSource;
    ctl::ComPtr<IUIElement> spOriginalSourceAsIUIE;

    *pIsInCheckboxContainer = FALSE;

    if (m_tpCheckboxContainer.Get())
    {
        IFC(ctl::ComPtr<IPointerRoutedEventArgs>(pArgs).As(&spArgsAsRoutedArgs));
        IFC(spArgsAsRoutedArgs->get_OriginalSource(&spOriginalSource));
        IFC(spOriginalSource.As(&spOriginalSourceAsIUIE));

        if (spOriginalSourceAsIUIE)
        {
            ctl::ComPtr<IUIElement> spCurrentItem(spOriginalSourceAsIUIE);

            while (spCurrentItem)
            {
                if (spCurrentItem.Get() == m_tpCheckboxContainer.Get())
                {
                    *pIsInCheckboxContainer = TRUE;
                    break;
                }
                if (spCurrentItem.Get() == m_tpContentContainer.Get())
                {
                    break;
                }

                ctl::ComPtr<IDependencyObject> spCurrentAsDO;
                ctl::ComPtr<IDependencyObject> spNext;

                IFC(spCurrentItem.As(&spCurrentAsDO));
                IFC(VisualTreeHelper::GetParentStatic(spCurrentAsDO.Get(), &spNext));
                spCurrentItem = spNext.AsOrNull<IUIElement>();
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

// Clears flags related to ongoing pointer input in response to a manipulation starting
// or container recycle.
// Updates the VisualState as appropriate.
_Check_return_ HRESULT ListViewBaseItem::ClearPointerState()
{
    HRESULT hr = S_OK;

    // stop the touch timer
    IFC(StopTouchTimer());

    m_inPressedForTouch = FALSE;
    m_inCheckboxPressedForTouch = FALSE;
    m_shouldEnterPointerOver = FALSE;
    m_isLeftButtonPressed = FALSE;
    m_isRightButtonPressed = FALSE;
    m_pendingTapPointerIDs.clear();

    IFC(UpdateVisualState(TRUE));

Cleanup:
    RRETURN(hr);
}

// Clears all state related to user interaction. Used to prepare a ListViewBaseItem
// for a new data item.
_Check_return_ HRESULT ListViewBaseItem::ClearInteractionState()
{
    HRESULT hr = S_OK;

    IFC(ClearPointerState());
    m_isCheckingForMouseDrag = FALSE;
    m_inReorderHintState = FALSE;
    m_inSelectionHintState = FALSE;
    m_isTopmost = FALSE;

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ListViewBaseItem::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    HRESULT hr = S_OK;

    IFC(ListViewBaseItemGenerated::OnPropertyChanged2(args));

    switch (args.m_pDP->GetIndex())
    {
        case KnownPropertyIndex::UIElement_Visibility:
            IFC(OnVisibilityChanged());
            break;

        case KnownPropertyIndex::Control_BorderThickness:
            {
                CListViewBaseItemChrome* pChrome = nullptr;

                IFC(GetItemChrome(&pChrome));

                if (pChrome)
                {
                    pChrome->InvalidateMeasure();
                    pChrome->InvalidateRender();
                }
            }
            break;

        case KnownPropertyIndex::Control_BorderBrush:
        case KnownPropertyIndex::Control_Background:
            {
                CListViewBaseItemChrome* pChrome = nullptr;

                IFC(GetItemChrome(&pChrome));

                if (pChrome)
                {
                    pChrome->InvalidateRender();
                }
            }
            break;
    }

Cleanup:
    RRETURN(hr);
}

// Update the visual states when the Visibility property is changed.
_Check_return_ HRESULT
ListViewBaseItem::OnVisibilityChanged()
{
    HRESULT hr = S_OK;
    xaml::Visibility visibility = xaml::Visibility_Collapsed;

    IFC(get_Visibility(&visibility));
    if (xaml::Visibility_Visible != visibility)
    {
        IFC(ClearStateFlags());
    }

    IFC(UpdateVisualState());

Cleanup:
    RRETURN(hr);
}

// Clear flags relating to the visual state.  Called when IsEnabled is set to FALSE
// or when Visibility is set to Hidden or Collapsed.
_Check_return_ HRESULT
ListViewBaseItem::ClearStateFlags()
{
    // stop the touch timer
    IFC_RETURN(StopTouchTimer());

    // Clear m_shouldEnterPointerOver because we'll no longer receive mouse input events
    // (including the MouseLeave event) once we've been disabled.
    m_shouldEnterPointerOver = FALSE;
    m_inPressedForTouch = FALSE;
    m_inCheckboxPressedForTouch = FALSE;
    m_isLeftButtonPressed = FALSE;
    m_isRightButtonPressed = FALSE;
    m_isCheckingForMouseDrag = FALSE;
    m_isKeyboardPressed = FALSE;

    return S_OK;
}

// Handles when a key is pressed down on the ListViewBaseItem.
IFACEMETHODIMP ListViewBaseItem::OnKeyDown(
    _In_ xaml_input::IKeyRoutedEventArgs* pArgs)
{
    BOOLEAN isHandled = FALSE;
    ctl::ComPtr<ListViewBase> spListView = GetParentListView();

    // Ignore already handled events
    IFCPTR_RETURN(pArgs);
    IFC_RETURN(ListViewBaseItemGenerated::OnKeyDown(pArgs));
    IFC_RETURN(pArgs->get_Handled(&isHandled));

    if (!isHandled && spListView && spListView->IsInExclusiveInteraction())
    {
        // During an exclusive interaction (drag/drop), disable all keyboard interaction.
        // Handle the event, since other controls don't have the necessary context around drag/drop interactions.
        isHandled = TRUE;
    }

    if (!isHandled)
    {
        wsy::VirtualKey originalKey = wsy::VirtualKey_None;
        wsy::VirtualKeyModifiers modifiers = wsy::VirtualKeyModifiers_None;

        // get the released key
        IFC_RETURN(static_cast<KeyRoutedEventArgs*>(pArgs)->get_OriginalKey(&originalKey));
        IFC_RETURN(ListViewBase::GetKeyboardModifiers(&modifiers));

        switch (originalKey)
        {
            // we should go into the pressed visual state
            case wsy::VirtualKey_GamepadA:
            {
                IFC_RETURN(SetIsKeyboardPressed(TRUE /* isKeyboardPressed */));
                isHandled = TRUE;
                break;
            }

            case wsy::VirtualKey_Left:
            case wsy::VirtualKey_Right:
            case wsy::VirtualKey_Up:
            case wsy::VirtualKey_Down:
            {
                if (spListView &&
                    IsFlagSet(modifiers, wsy::VirtualKeyModifiers_Menu) &&
                    !IsFlagSet(modifiers, wsy::VirtualKeyModifiers_Control) &&
                    IsFlagSet(modifiers, wsy::VirtualKeyModifiers_Shift)
                    )
                {
                    xaml::FlowDirection flowDirection;
                    xaml_controls::Orientation panelOrientation;
                    ListViewBase::KeyboardReorderDirection keyboardReorderDirection;

                    IFC_RETURN(get_FlowDirection(&flowDirection));
                    IFC_RETURN(spListView->GetPanelOrientation(&panelOrientation));

                    if (flowDirection == xaml::FlowDirection::FlowDirection_RightToLeft &&
                        panelOrientation == xaml_controls::Orientation_Horizontal)
                    {
                        // When the ListView is using a horizontal panel in RTL mode, the Left/Up keys are treated like Right/Down and vice-versa.
                        keyboardReorderDirection = (originalKey == wsy::VirtualKey_Left || originalKey == wsy::VirtualKey_Up) ?
                            ListViewBase::KeyboardReorderDirection_ToHigherIndex :
                            ListViewBase::KeyboardReorderDirection_ToLowerIndex;
                    }
                    else
                    {
                        keyboardReorderDirection = (originalKey == wsy::VirtualKey_Left || originalKey == wsy::VirtualKey_Up) ?
                            ListViewBase::KeyboardReorderDirection_ToLowerIndex :
                            ListViewBase::KeyboardReorderDirection_ToHigherIndex;
                    }

                    IFC_RETURN(OnKeyboardReorder(keyboardReorderDirection));
                    isHandled = TRUE;
                }
                break;
            }
        }
    }

    // Inform ListView that the keydown came from an Item. If it came from a header, or blank area
    // instead of an item ListView will just forward the event back to the ScrollView.
    if (!isHandled && spListView)
    {
        spListView->SetHandleKeyDownArgsFromItem(TRUE);
    }

    if (isHandled)
    {
        IFC_RETURN(pArgs->put_Handled(TRUE));
    }

    return S_OK;
}

// Handles when a key is released on the ListViewBaseItem.
IFACEMETHODIMP ListViewBaseItem::OnKeyUp(
    _In_ xaml_input::IKeyRoutedEventArgs* pArgs)
{
    BOOLEAN isHandled = FALSE;
    ctl::ComPtr<ListViewBase> spListView = GetParentListView();

    // Ignore already handled events
    IFCPTR_RETURN(pArgs);
    IFC_RETURN(ListViewBaseItemGenerated::OnKeyUp(pArgs));
    IFC_RETURN(pArgs->get_Handled(&isHandled));

    if (!isHandled && spListView && spListView->IsInExclusiveInteraction())
    {
        // During an exclusive interaction (drag/drop), disable all keyboard interaction.
        // Handle the event, since other controls don't have the necessary context around drag/drop interactions.
        isHandled = TRUE;
    }

    wsy::VirtualKey originalKey = wsy::VirtualKey_None;

    // get the released key
    IFC_RETURN(static_cast<KeyRoutedEventArgs*>(pArgs)->get_OriginalKey(&originalKey));

    // This code needs to always execute to prevent getting stuck in Pressed state.
    auto clearPressedStateGuard = wil::scope_exit(
        [&]()
        {
            switch (originalKey)
            {
                // Gamepads use Gamepad A as both the Primary and Secondary interaction
                // gesture depending on the current state of the control
                case wsy::VirtualKey_GamepadA:
                {
                    VERIFYHR(SetIsKeyboardPressed(FALSE /* isKeyboardPressed */));
                }
            }
        });

    if (!isHandled && m_isKeyboardPressed)
    {
        switch (originalKey)
        {
            // Gamepads use Gamepad A as both the Primary and Secondary interaction
            // Note that OnPrimaryInteractionGesture will do invoke if possible and if not
            // will fallback to selection.
            case wsy::VirtualKey_GamepadA:
            {
                if (spListView)
                {
                    IFC_RETURN(spListView->OnItemPrimaryInteractionGesture(this, TRUE /* isKeyboardInput */, &isHandled));
                }

                break;
            }
        }
    }

    if (isHandled)
    {
        IFC_RETURN(pArgs->put_Handled(TRUE));
    }

    return S_OK;
}

// Helper method to call the parent ListViewBase's OnKeyboardReorder.
_Check_return_ HRESULT ListViewBaseItem::OnKeyboardReorder(
    _In_ ListViewBase::KeyboardReorderDirection direction)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<ListViewBase> spListView;

    spListView = GetParentListView();

    if (spListView)
    {
        IFC(spListView->OnKeyboardReorder(direction, this));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ListViewBaseItem::QueryFor(_In_ IUnknown* pThis, _Outptr_ ListViewBaseItem** ppListViewBaseItem)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IListViewItem> spListViewItem;

    *ppListViewBaseItem = nullptr;

    if (SUCCEEDED(ctl::do_query_interface(spListViewItem, pThis)))
    {
        // Can't do CopyTo (ListViewBaseItem doesn't have a GUID), so must use Detach.
        *ppListViewBaseItem = static_cast<ListViewItem*>(spListViewItem.Detach());
    }
    else
    {
        ctl::ComPtr<IGridViewItem> spGridViewItem;
        if (SUCCEEDED(ctl::do_query_interface(spGridViewItem, pThis)))
        {
            // Can't do CopyTo (ListViewBaseItem doesn't have a GUID), so must use Detach.
            *ppListViewBaseItem = static_cast<GridViewItem*>(spGridViewItem.Detach());
        }
    }

    RRETURN(hr);
}

// UIAutomation Property: DragIsGrabbed
_Check_return_ HRESULT ListViewBaseItem::GetDragIsGrabbed(_Out_ BOOLEAN* value)
{
    *value = m_fDragIsGrabbed;
    return S_OK;
}

_Check_return_ HRESULT ListViewBaseItem::SetDragIsGrabbed(_In_ BOOLEAN newValue)
{
    HRESULT hr = S_OK;
    BOOLEAN oldValue = m_fDragIsGrabbed;
    ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spAP;

    m_fDragIsGrabbed = newValue;

    IFC(GetOrCreateAutomationPeer(&spAP));
    if (spAP && AutomationPeer::ArePropertyChangedListeners() && oldValue != newValue)
    {
        IFC(spAP.Cast<AutomationPeer>()->RaisePropertyChangedEventById(UIAXcp::APIsGrabbedProperty, oldValue, newValue));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ListViewBaseItem::GoToStateWithFallback(
    _In_ bool useTransitions,
    _In_ BOOLEAN tryPrimaryState,
    _In_z_ WCHAR* pszPrimaryState,
    _In_z_ WCHAR* pszFallbackState,
    _Out_ BOOLEAN* used
    )
{
    HRESULT hr = S_OK;

    *used = FALSE;

    if (tryPrimaryState)
    {
        IFC(GoToState(useTransitions, pszPrimaryState, used));
    }

    if (!*used)
    {
        IFC(GoToState(useTransitions, pszFallbackState, used));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ListViewBaseItem::GetLogicalParentForAPProtected(_Outptr_ DependencyObject** ppLogicalParentForAP)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<ListViewBase> spListView;

    IFCPTR(ppLogicalParentForAP);
    *ppLogicalParentForAP = nullptr;

    spListView = GetParentListView();
    if (spListView)
    {
        BOOLEAN isGrouping = FALSE;
        IFC(spListView->get_IsGrouping(&isGrouping));
        if (isGrouping)
        {
            ctl::ComPtr<xaml_controls::IPanel> spPanel;
            ctl::ComPtr<IModernCollectionBasePanel> spItemsHostPanelModernCollection;

            IFC(spListView->get_ItemsPanelRoot(&spPanel));
            spItemsHostPanelModernCollection = spPanel.AsOrNull<IModernCollectionBasePanel>();
            if (spItemsHostPanelModernCollection)
            {
                INT indexContainer = -1;

                IFC(spItemsHostPanelModernCollection.Cast<ModernCollectionBasePanel>()->IndexFromContainer(this, &indexContainer));

                if (indexContainer > -1)
                {
                    INT32 indexGroup = -1;
                    ctl::ComPtr<IDependencyObject> spHeaderElementAsDO;

                    IFC(spItemsHostPanelModernCollection.Cast<ModernCollectionBasePanel>()->GetGroupInformationFromItemIndex(indexContainer, &indexGroup, nullptr, nullptr));
                    IFC(spItemsHostPanelModernCollection.Cast<ModernCollectionBasePanel>()->HeaderFromIndex(indexGroup, &spHeaderElementAsDO));
                    *ppLogicalParentForAP = static_cast<DependencyObject*>(spHeaderElementAsDO.Detach());
                }
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ListViewBaseItem::GetItemChrome(
    _Out_ CListViewBaseItemChrome** ppItemChrome)
{
    HRESULT hr = S_OK;
    CDependencyObject* pHandle = GetHandle();
    KnownTypeIndex typeIndex;

    *ppItemChrome = nullptr;

    if (pHandle)
    {
        CUIElement* pFirstChild = static_cast<CUIElement*>(pHandle)->GetFirstChildNoAddRef();

        if (pFirstChild)
        {
            IFC(CoreImports::DependencyObject_GetTypeIndex(pFirstChild, &typeIndex));

            if ((typeIndex == DependencyObjectTraits<CListViewItemChrome>::Index) ||
                (typeIndex == DependencyObjectTraits<CGridViewItemChrome>::Index))
            {
                *ppItemChrome = static_cast<CListViewBaseItemChrome*>(pFirstChild);
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

// Returns a drag and drop visual for this item.
// If necessary, this method will create one.
_Check_return_ HRESULT
ListViewBaseItem::GetOrCreateDragDropVisual(
    _Outptr_ DirectUI::DragDropVisual** ppVisual)
{
    IFC_RETURN(EnsureDragDropVisual(wf::Point{ 0.0f, 0.0f }, false /*useSystemDefaultVisual*/));
    IFC_RETURN(m_spDragDropVisual.CopyTo(ppVisual));
    return S_OK;
}

// Clears the drag and drop visual.
void ListViewBaseItem::ClearDragDropVisual()
{
    m_spDragDropVisual.Reset();
}

_Check_return_ HRESULT ListViewBaseItem::ChangeVisualStateWithContextNewStyle(
    _In_ VisualStateManagerBatchContext *pContext,
    // true to use transitions when updating the visual state, false
    // to snap directly to the new visual state.
    _In_ bool bUseTransitions)
{
    std::vector<const wchar_t*> validVisualStates;

    ctl::ComPtr<ListViewBase> spListView;

    DirectUI::Components::VisualStatesHelper::ListViewBaseItemVisualStatesCriteria criteria;

    IFC_RETURN(get_IsEnabled(&criteria.isEnabled));
    IFC_RETURN(get_IsSelected(&criteria.isSelected));
    IFC_RETURN(get_FocusState(&criteria.focusState));

    // Pressed state should be handled whether it's mouse or touch
    // m_inCheckboxPressedForTouch is not used because it is part of the 8.1 template
    criteria.isPressed = m_isLeftButtonPressed || m_isRightButtonPressed || m_inPressedForTouch || m_isKeyboardPressed;
    criteria.isPointerOver = m_shouldEnterPointerOver;
    criteria.isDragVisualCaptured = m_dragVisualCaptured;

    spListView = GetParentListView();
    if (spListView)
    {
        xaml_controls::ListViewSelectionMode selectionMode = xaml_controls::ListViewSelectionMode_None;

        criteria.isDragging = spListView->IsInDragDrop();
        criteria.isDraggedOver = spListView->IsDragOverItem(this);
        criteria.dragItemsCount = spListView->DragItemsCount();
        IFC_RETURN(spListView->IsContainerDragDropOwner(this, &criteria.isItemDragPrimary));

        // Holding gesture will show drag visual
        IFC_RETURN(spListView->get_CanDragItems(&criteria.canDrag));
        IFC_RETURN(spListView->get_CanReorderItems(&criteria.canReorder));
        if (spListView->GetIsHolding())
        {
            criteria.isHolding = TRUE;
            if (m_isHolding)
            {
                criteria.isItemDragPrimary = TRUE;
            }
        }

        IFC_RETURN(spListView->get_IsMultiSelectCheckBoxEnabled(&criteria.isMultiSelect));

        criteria.isIndicatorSelect = CListViewBaseItemChrome::IsRoundedListViewBaseItemChromeEnabled(DXamlCore::GetCurrent()->GetHandle());

        IFC_RETURN(spListView->get_SelectionMode(&selectionMode));

        // if the ListView selection mode is None, we should appear as not Selected
        criteria.isSelected &= (selectionMode != xaml_controls::ListViewSelectionMode_None);

        // Read-only mode
        {
            BOOLEAN isItemClickEnabled = FALSE;

            IFC_RETURN(spListView->get_IsItemClickEnabled(&isItemClickEnabled));

            if (selectionMode == xaml_controls::ListViewSelectionMode_None && !isItemClickEnabled)
            {
                criteria.isPressed = FALSE;
                criteria.isPointerOver = FALSE;
            }
        }

        if (criteria.isMultiSelect || criteria.isIndicatorSelect)
        {
            xaml_controls::ListViewSelectionMode selectionMode2 = xaml_controls::ListViewSelectionMode_Single;

            IFC_RETURN(spListView->get_SelectionMode(&selectionMode2));

            if (criteria.isMultiSelect)
            {
                criteria.isMultiSelect &= (selectionMode2 == xaml_controls::ListViewSelectionMode_Multiple);
            }

            if (criteria.isIndicatorSelect)
            {
                criteria.isIndicatorSelect &= (selectionMode == xaml_controls::ListViewSelectionMode_Single || selectionMode == xaml_controls::ListViewSelectionMode_Extended);
            }
        }

        criteria.isInsideListView = TRUE;
    }

    // get all valid visual states
    IFC_RETURN(DirectUI::Components::VisualStatesHelper::GetValidVisualStatesListViewBaseItem(criteria, validVisualStates));

    for (auto& visualState : validVisualStates)
    {
        IFC_RETURN(pContext->GoToState(bUseTransitions, visualState, std::wcslen(visualState)));
    }

    return S_OK;
}

// returns true if we captured the pointer for the drag visual
_Check_return_ HRESULT ListViewBaseItem::SetDragVisualCaptured(
    _In_ BOOLEAN dragVisualCaptured)
{
    m_dragVisualCaptured = dragVisualCaptured;

    // make sure we go to the right state
    IFC_RETURN(ChangeVisualState(TRUE));

    return S_OK;
}

// Sets the state of the isKeyboardPressed boolean
_Check_return_ HRESULT ListViewBaseItem::SetIsKeyboardPressed(
    _In_ BOOLEAN isKeyboardPressed)
{
    if (m_isKeyboardPressed != isKeyboardPressed)
    {
        m_isKeyboardPressed = isKeyboardPressed;

        // make sure we go to the right state
        IFC_RETURN(ChangeVisualState(TRUE));
    }

    return S_OK;
}

_Check_return_ HRESULT ListViewBaseItem::StartTouchTimer(
    _In_ TouchTimerState touchTimerState)
{
    wf::TimeSpan timeSpan = { };

    // make sure we stop the current timer if it exists
    IFC_RETURN(StopTouchTimer());

    // make sure we have a timer
    IFC_RETURN(EnsureTouchTimer());

    // set the state
    m_touchTimerState = touchTimerState;

    switch (m_touchTimerState)
    {
        case TouchTimerState::PressedTimer:
            timeSpan.Duration = LISTVIEWBASEITEM_TOUCH_PRESSED_DELAY * TICKS_PER_MILLISECOND;
            break;

        case TouchTimerState::ReleasedTimer:
            timeSpan.Duration = LISTVIEWBASEITEM_TOUCH_RELEASED_TIMER * TICKS_PER_MILLISECOND;
            break;
    }

    // set the interval
    IFC_RETURN(m_tpTouchTimer->put_Interval(timeSpan));

    // start the timer
    IFC_RETURN(m_tpTouchTimer->Start());

    return S_OK;
}

_Check_return_ HRESULT ListViewBaseItem::EnsureTouchTimer()
{
    // if it hasn't been created yet, create it
    if (!m_tpTouchTimer)
    {
        ctl::ComPtr<DispatcherTimer> spNewDispatcherTimer;

        IFC_RETURN(ctl::make<DispatcherTimer>(&spNewDispatcherTimer));

        // attach the handler
        IFC_RETURN(m_epTouchTimerEvent.AttachEventHandler(
            spNewDispatcherTimer.Get(),
            [this](_In_opt_ IInspectable* pUnused1, _In_opt_ IInspectable* pUnused2)
        {
            IFC_RETURN(TouchTimerTickHandler());

            return S_OK;
        }));

        SetPtrValue(m_tpTouchTimer, spNewDispatcherTimer.Get());
    }

    return S_OK;
}

_Check_return_ HRESULT ListViewBaseItem::StopTouchTimer()
{
    if (m_tpTouchTimer)
    {
        IFC_RETURN(m_tpTouchTimer->Stop());
    }

    m_touchTimerState = TouchTimerState::NoTimer;

    return S_OK;
}

// Handler for the Tick event on m_tpTouchTimer
_Check_return_ HRESULT ListViewBaseItem::TouchTimerTickHandler()
{
    switch (m_touchTimerState)
    {
        case TouchTimerState::PressedTimer:
            m_inPressedForTouch = TRUE;
            break;

        case TouchTimerState::ReleasedTimer:
            m_inPressedForTouch = FALSE;
            break;
    }

    IFC_RETURN(StopTouchTimer());
    IFC_RETURN(ChangeVisualState(true));

    return S_OK;
}

// Returns TRUE if the item should play the DragOver visual state
// this is true if AllowDrop is true
// and if the dragPoint is in the center area of the item
// 20/60/20 for LVI
// 30/40/30 for GVI
_Check_return_ HRESULT ListViewBaseItem::IsDragOver(
    _In_ wf::Point dragPoint,
    _In_ xaml_controls::Orientation panelOrientation,
    _Out_ bool* pResult)
{
    // Represents the outside margin percentage
    // In case of ListViewItem, margins are 20/60/20
    // In case of GridViewItem, margins are 30/40/30
    const double startMargin = ctl::is<xaml_controls::IGridViewItem>(this) ? GRIDVIEWITEM_DRAGOVER_OUTSIDE_MARGIN : LISTVIEWITEM_DRAGOVER_OUTSIDE_MARGIN;
    // Represents the outside margin percentage added to the inside percentage
    // In case of ListViewItem, this will be 0.2 + 0.6 = 0.8 or (1 - 0.2)
    // In case of GridViewItem, this will be 0.3 + 0.4 = 0.7 or (1 - 0.3)
    const double endMargin = 1.0 - startMargin;
    double size = 0;
    double location = 0;

    *pResult = false;

    // depending on the orientation of the panel, we look at either the height or width
    if (panelOrientation == xaml_controls::Orientation_Vertical)
    {
        location = dragPoint.Y;
        IFC_RETURN(get_ActualHeight(&size));
    }
    else
    {
        location = dragPoint.X;
        IFC_RETURN(get_ActualWidth(&size));
    }

    if (location > startMargin * size && location < endMargin * size)
    {
        *pResult = true;
    }

    return S_OK;
}

// Called when the user drags over this ListViewBaseItem
IFACEMETHODIMP ListViewBaseItem::OnDragOver(
    _In_ xaml::IDragEventArgs* args)
{
    ctl::ComPtr<ListViewBase> spListView = GetParentListView();

    if (spListView)
    {
        bool isDragOver = false;
        wf::Point dragPointRelativeToLVBI = { 0, 0 };
        xaml_controls::Orientation panelOrientation = xaml_controls::Orientation_Vertical;

        IFC_RETURN(args->GetPosition(this, &dragPointRelativeToLVBI));
        IFC_RETURN(spListView->GetPanelOrientation(&panelOrientation));

        IFC_RETURN(IsDragOver(dragPointRelativeToLVBI, panelOrientation, &isDragOver));

        if (isDragOver)
        {
            IFC_RETURN(spListView->SetDragOverItem(this));
            args->put_Handled(TRUE);
        }
        else
        {
            IFC_RETURN(spListView->SetDragOverItem(nullptr));
        }

        IFC_RETURN(SetIsDragOver(isDragOver));
    }

    return S_OK;
}

// Called when the user drags out of this ListViewBaseItem
IFACEMETHODIMP ListViewBaseItem::OnDragLeave(
    _In_ xaml::IDragEventArgs* args)
{
    ctl::ComPtr<ListViewBase> spListView = GetParentListView();

    if (spListView)
    {
        IFC_RETURN(spListView->SetDragOverItem(nullptr));
    }

    IFC_RETURN(SetIsDragOver(FALSE /* isDragOver */));

    return S_OK;
}

// Sets the state of the isDragOver boolean
_Check_return_ HRESULT ListViewBaseItem::SetIsDragOver(
    _In_ BOOLEAN isDragOver)
{
    if (m_isDragOver != isDragOver)
    {
        m_isDragOver = isDragOver;
        IFC_RETURN(ChangeVisualState(TRUE));
    }

    return S_OK;
}

// Returns TRUE if the dragPoint is in the topHalf of the item
_Check_return_ HRESULT ListViewBaseItem::IsInBottomHalfForExternalReorder(
    _In_ wf::Point dragPoint,
    _In_ xaml_controls::Orientation panelOrientation,
    _Out_ bool* pResult)
{
    double size = 0;
    double location = 0;

    *pResult = false;

    // depending on the orientation of the panel, we look at either the height or width
    if (panelOrientation == xaml_controls::Orientation_Vertical)
    {
        location = dragPoint.Y;
        IFC_RETURN(get_ActualHeight(&size));
    }
    else
    {
        location = dragPoint.X;
        IFC_RETURN(get_ActualWidth(&size));
    }

    // we only care about the case when the item has moved out of view
    // that means the pointer location should be before the item
    if (location < 0)
    {
        location += size;

        if (location > size / 4)
        {
            *pResult = true;
        }
    }

    return S_OK;
}
