// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      ListViewBase displays a rich, interactive collection of items.

#include "precomp.h"
#include "ListViewBase.g.h"
#include "ListViewBaseAutomationPeer.g.h"
#include "ScrollViewer.g.h"
#include "RectangleGeometry.g.h"
#include "ListViewBaseItem.g.h"
#include "ListViewBaseHeaderItem.g.h"
#include "ItemsPresenter.g.h"
#include "ModernCollectionBasePanel.g.h"
#include "AppBar.g.h"
#include "IOrientedPanel.g.h"
#include "Window.g.h"
#include "DragDropInternal.h"
#include "focusmgr.h"
#include "VisualTreeHelper.h"
#include "IncrementalLoadingAdapter.h"
#include "ConnectedAnimation.g.h"
#include "ConnectedAnimationService.g.h"
#include "TryStartConnectedAnimationOperation.h"
#include "ApplicationBarService.g.h"
#include "BuildTreeService.g.h"
#include "BudgetManager.g.h"
#include "GettingFocusEventArgs.g.h"
#include "XamlRoot.g.h"

using namespace std::placeholders;
using namespace DirectUI;
using namespace DirectUISynonyms;
using namespace xaml_data;
using namespace xaml_animation;

// Uncomment to output ListViewBase debugging information
//#define LVB_DEBUG

// Initializes a new instance of the ListViewBase class.
ListViewBase::ListViewBase()
    : m_pAnchorIndex(nullptr)
    , m_dragItemsCount(0)
    , m_isDraggingOverSelf(FALSE)
    , m_scrollHostOffsetChangeAction(ScrollHostOffsetChangeAction_None)
    , m_isLoadAsyncInProgress(FALSE)
    , m_maxContentWidth(0)
    , m_maxContentHeight(0)
    , m_semanticZoomCompletedFocusState(xaml::FocusState_Programmatic)
    , m_isInItemReorderAfterDrop(FALSE)
    , m_focusedGroupIndex(-1)
    , m_lastFocusedGroupIndex(0)
    , m_pendingAutoPanVelocity()
    , m_currentAutoPanVelocity()
    , m_lowestPhaseInQueue(-1)
    , m_budget(BUDGET_MANAGER_DEFAULT_LIMIT)
    , m_itemsAreTabStops(TRUE)
    , m_groupsAreTabStops(TRUE)
    , m_allowDrop(FALSE)
    , m_isInOnSelectionModeChanged(FALSE)
    , m_disableScrollingPlaceholders(FALSE)
    , m_bKeyDownArgsFromItem(FALSE)
    , m_itemSelectionGestureId()
    , m_lastIncrementalVisualizationContainerCount(0)
    , m_focusableElementForModernPanelReentrancyGuard(false)
    , m_firstAvailableItemForFocus(-1)
    , m_firstAvailableGroupHeaderForFocus(-1)
    , m_itemIndexHintForHeaderNavigation(-1)
    , m_lastFocusedElementType(Other)
    , m_wasItemOrGroupHeaderFocused(false)
    , m_isHolding(false)
    , m_isDeferredElementFooter(false)
    , m_deferredPoint(DeferPoint::ItemsHostAvailable)
    , m_showsScrollingPlaceholdersLocal(TRUE)
 {
    m_lastReorderPosition.X = 0;
    m_lastReorderPosition.Y = 0;
    m_lastDragPosition.X = 0;
    m_lastDragPosition.Y = 0;
    m_lastDragOverPoint.X = 0;
    m_lastDragOverPoint.Y = 0;
    m_liveReorderIndices = { -1, -1, -1 };
    m_dragAcceptedOperation = wadt::DataPackageOperation_None;
}

// Initialize this ListViewBase
_Check_return_ HRESULT ListViewBase::Initialize()
{
    ctl::ComPtr<TrackerCollection<xaml::UIElement*>> spTrackedContainersForClear;
    ctl::ComPtr<TrackerCollection<IInspectable*>> spTrackedItemsForClear;
    ctl::ComPtr<xaml::IRoutedEventHandler> spLoadedEventHandler;
    EventRegistrationToken eventRegistration;

    IFC_RETURN(ListViewBaseGenerated::Initialize());

    IFC_RETURN(ctl::make(&spTrackedContainersForClear));

    SetPtrValue(m_toBeClearedContainers, std::move(spTrackedContainersForClear));

    IFC_RETURN(ctl::make(&spTrackedItemsForClear));

    SetPtrValue(m_toBeClearedItems, std::move(spTrackedItemsForClear));

    spLoadedEventHandler.Attach(
        new ClassMemberEventHandler<
        ListViewBase,
        xaml_controls::IListViewBase,
        xaml::IRoutedEventHandler,
        IInspectable,
        xaml::IRoutedEventArgs>(this, &ListViewBase::OnLoaded, true /* subscribingToSelf */));
    IFC_RETURN(add_Loaded(spLoadedEventHandler.Get(), &eventRegistration));

    IFC_RETURN(m_gettingFocusHandler.AttachEventHandler(
        this,
        std::bind(&ListViewBase::OnGettingFocus, this, _1, _2)));

    return S_OK;
}

// Destroys an instance of the ListViewBase class.
ListViewBase::~ListViewBase()
{
    if (auto peg = m_tpScrollViewer.TryMakeAutoPeg())
    {
        VERIFYHR(m_tpScrollViewer.Cast<ScrollViewer>()->SetDirectManipulationStateChangeHandler(NULL));
        VERIFYHR(DetachHandler(m_epScrollViewerViewChangingHandler, m_tpScrollViewer));
    }

    // stop the timer if it exists
    {
        auto spLiveReorderTimer = m_tpLiveReorderTimer.GetSafeReference();
        if (spLiveReorderTimer)
        {
            IGNOREHR(spLiveReorderTimer->Stop());
        }
    }

    VERIFYHR(DetachHandler(m_epLiveReorderTimerEvent, m_tpLiveReorderTimer));

    ReleaseAnchorIndex();
    VERIFYHR(DetachScrollViewerPropertyChanged());
}

_Check_return_ HRESULT ListViewBase::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    HRESULT hr = S_OK;

    IFC(ListViewBaseGenerated::OnPropertyChanged2(args));

    switch (args.m_pDP->GetIndex())
    {
    case KnownPropertyIndex::ListViewBase_SelectionMode:
        {
            // Call OnSelectionModeChanged when the selection mode changes.
            IFC(OnSelectionModeChanged(
                static_cast<xaml_controls::ListViewSelectionMode>(args.m_pOldValue->AsEnum()),
                static_cast<xaml_controls::ListViewSelectionMode>(args.m_pNewValue->AsEnum())));

            // Call the Selector::UpdateVisibleAndCachedItemsSelectionAndVisualState function
            // OnSelectionModeChanged will update all Selection related properties
            IFC(UpdateVisibleAndCachedItemsSelectionAndVisualState(false /* updateIsSelected */));
            break;
        }
    case KnownPropertyIndex::ListViewBase_IsSwipeEnabled:
        {
            // TODO: Enable/Disable swipe by registering or unregistering
            // with the Touch Input Engine we're listening to (it's important
            // to let the TIE know we're no listening for swipe so it will
            // let DirectManipulation pick up panning opposite our orientation)
            // NOTE: We have eliminated TIE usage, is the above work applicable to the
            //       the gesture recognizer.
            break;
        }
    case KnownPropertyIndex::Selector_IsSelectionActive:
        {
            IFC(OnIsSelectionActiveChanged());
            break;
        }
    case KnownPropertyIndex::ListViewBase_ShowsScrollingPlaceholders:
        {
            // Recalculate local field.
            IFC(ListViewBaseGenerated::get_ShowsScrollingPlaceholders(&m_showsScrollingPlaceholdersLocal));
            break;
        }
    case KnownPropertyIndex::ItemsControl_ItemsSource:
        // when the ItemsSource changes, reset the last focused index
        // so that when tabbing into the control we dont jump to the remembered index. It
        // is likely not representing the same data item
        SetLastFocusedIndex(0);
    case KnownPropertyIndex::ListViewBase_IncrementalLoadingTrigger:
    case KnownPropertyIndex::ItemsControl_ItemsHost:
        {
            // When any property changes that affects our ItemsHost
            // we hookup to changes.
            IFC(EnsureScrollHostOffsetChangeAction());

            ItemsControl::TraceVirtualizationEnabledByModernPanel();

            // Hookup to the ItemsRangeInfo if it exists
            IFC(InitializeDataSourceItemsRangeInfo());

            // Hookup to the SelectionInfo if it exists
            IFC(InitializeDataSourceSelectionInfo());
            break;
        }
    case KnownPropertyIndex::ItemsControl_ItemsPanel:
        {
            ItemsControl::TraceVirtualizationEnabledByModernPanel();
            break;
        }
    case KnownPropertyIndex::ListViewBase_IsMultiSelectCheckBoxEnabled:
        {
            // Call the Selector::UpdateVisibleAndCachedItemsVisualState function
            IFC(UpdateVisibleAndCachedItemsSelectionAndVisualState(false /* updateIsSelected */));

            break;
        }
    case KnownPropertyIndex::UIElement_Visibility:
        {
            // If we have a pending drag operation, cancel it now.
            IFC(CancelDrag());
            break;
        }
    }

Cleanup:
    return hr;
}

IFACEMETHODIMP ListViewBase::get_ShowsScrollingPlaceholders(_Out_ BOOLEAN* pValue)
{
    *pValue = m_showsScrollingPlaceholdersLocal;
    RRETURN(S_OK);
}

// Called when a ListViewBaseItem is getting a new data item to display.
IFACEMETHODIMP ListViewBase::PrepareContainerForItemOverride(
    _In_ xaml::IDependencyObject* element,
    _In_ IInspectable* item)
{
    HRESULT hr = S_OK;

    IFCPTR(element);

    if (IsListViewBaseItem(ctl::iinspectable_cast(element)))
    {
        ctl::ComPtr<ISelectorItem> spIListViewItemAsSelectorItem;
        IFC(ctl::do_query_interface(spIListViewItemAsSelectorItem, element));

        {
            // Update all states at once whichever changes are made by bases classes
            // Suspends the VisualState changes in order to "batch" them at the end of the preparation
            StateChangeSuspender suspender(spIListViewItemAsSelectorItem.Cast<ListViewBaseItem>(), &hr);

            IFC(spIListViewItemAsSelectorItem.Cast<ListViewBaseItem>()->ClearInteractionState());

            // Will call UpdateVisualState for us.
            IFC(ListViewBaseGenerated::PrepareContainerForItemOverride(element, item));
        }
        IFC(hr); // in case the state could not have been updated by the StateChangeSuspender Destructor
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ListViewBase::PreProcessContentPreparation(
    _In_ xaml::IDependencyObject* pContainer,
    _In_opt_ IInspectable* pItem)
{
    if (IsListViewBaseItem(ctl::iinspectable_cast(pContainer)))
    {
        ctl::ComPtr<ISelectorItem> spIListViewItemAsSelectorItem;
        ctl::ComPtr<IUIElement> spElementAsUIE = ctl::query_interface_cast<IUIElement>(pContainer);

        IFC_RETURN(ctl::do_query_interface(spIListViewItemAsSelectorItem, pContainer));

        // ListViewItems are allowed to grow bigger than the available size and not be clipped.
        // we currently only allow this in ItemsStackPanel
        ctl::ComPtr<IPanel> spItemsHost;
        IFC_RETURN(get_ItemsHost(&spItemsHost));

        bool allowNonClipping = false;
        if (spItemsHost)
        {
            allowNonClipping = static_cast<CPanel*>(static_cast<DirectUI::Panel*>(spItemsHost.Get())->GetHandle())->GetIsNonClippingSubtree();
        }
        static_cast<CListViewBaseItem*>(spIListViewItemAsSelectorItem.Cast<ListViewBaseItem>()->GetHandle())->SetIsNonClippingSubtree(allowNonClipping);

        // we might have been inserted into the list for deferred clear container calls.
        // the fact that we are now being prepared, means that we don't have to perform that clear call.
        // yay! that means we are going to not perform work that has quite a bit of perf overhead.
        // ---
        // we happen to know that the clear will have been pushed to the back of the vector, so optimize
        // the panning scenario by checking in reverse order

        // special case the last element (since we push_back during when we called clear and we expect the next
        // action to be this prepare).
        UINT toBeClearedContainerCount = 0;
        IFC_RETURN(m_toBeClearedContainers->get_Size(&toBeClearedContainerCount));

        for (UINT current = toBeClearedContainerCount -1; toBeClearedContainerCount > 0; --current)
        {
            ctl::ComPtr<IUIElement> spCurrentContainer;
            // go from back to front, since we're most likely in the back.
            IFC_RETURN(m_toBeClearedContainers->GetAt(current, &spCurrentContainer));

            if (spCurrentContainer.Get() == spElementAsUIE.Get())
            {
                // remove from _BOTH_
                IFC_RETURN(m_toBeClearedContainers->RemoveAt(current));
                IFC_RETURN(m_toBeClearedItems->RemoveAt(current));

                // The current container is *not* going to be cleared and it's going to
                // be reused and reprepared right away. If it gets re-templated during
                // the preparation routine, the binding expressions in the new template
                // will fail against the old content (assuming it's set) when the
                // old data context is propagated to the new template tree. To avoid
                // that, we make sure to unset the content here.
                // Note that this operation is a no-op if the app is handling ContainerContentChanging
                // and not relying on data context propagation (which we don't recommend). In that case,
                // the content is never set and we don't run into the binding issue described above.
                ctl::ComPtr<IDataTemplateSelector> itemTemplateSelector;
                IFC_RETURN(get_ItemTemplateSelector(&itemTemplateSelector));
                if (itemTemplateSelector)
                {
                    VirtualizationInformation *virtualizationInformation = spElementAsUIE ? spElementAsUIE.Cast<UIElement>()->GetVirtualizationInformation() : nullptr;
                    bool isContainerFromTemplateRoot = virtualizationInformation != nullptr && virtualizationInformation->GetIsContainerFromTemplateRoot();

                    // Do not clear content if we got the container from ItemTemplate
                    if (!isContainerFromTemplateRoot)
                    {
                        IFC_RETURN(spIListViewItemAsSelectorItem.Cast<SelectorItem>()->ClearValue(
                            MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::ContentControl_Content)));
                    }
                }

                break;
            }

            if (current == 0)
            {
                // UINT
                break;
            }
        }
    }

    return S_OK;
}



// Called when a ListViewBaseItem is getting recycled
IFACEMETHODIMP ListViewBase::ClearContainerForItemOverride(
    _In_ xaml::IDependencyObject* element,
    _In_ IInspectable* item)
{
    HRESULT hr = S_OK;

    IFCPTR(element);

    if (IsListViewBaseItem(ctl::iinspectable_cast(element)))
    {
        ctl::ComPtr<IPanel> spItemsHost;

        IFC(get_ItemsHost(&spItemsHost));

        // There is much perf involved with doing a clear, and usually it is going to be
        // a waste of time since we are going to immediately follow up with a prepare.
        // Perf traces have found this to be about 8 to 12% during a full virtualization pass (!!)
        // Although with other optimizations we would expect that to go down, it is unlikely to go
        // down to 0. Therefore we are deferring the impl work here to later.
        // We have decided to do this only for the new panels.

        // also, do not defer items that are UIElement. They need to be cleared straight away so that
        // they can be messed with again.
        if (ctl::is<ICustomGeneratorItemsHost>(spItemsHost) && !ctl::is<IUIElement>(item))
        {
            ctl::ComPtr<IDependencyObject> spElement = element;
            ctl::ComPtr<IUIElement> spElementUIE = spElement.AsOrNull<IUIElement>();

            // raise the event, so that people can build up a concept of the recycle queue
            // ListView decides to raise the CCC event immediately (even though it is deferring the actual clearing)
            IFC(RaiseContainerContentChangingOnRecycle(spElementUIE.Get(), item));

            // we are going to defer the clear
            m_toBeClearedContainers->Append(spElementUIE.Get());
            IFC(m_toBeClearedItems->Append(item));

            // note that if we are being cleared, we are not going to be in the
            // visible index, or the caches. And thus we will never be called in the
            // prepare queuing part.

            if (!m_isRegisteredForCallbacks)
            {
                ctl::ComPtr<BuildTreeService> spBuildTree;
                IFC(DXamlCore::GetCurrent()->GetBuildTreeService(spBuildTree));
                IFC(spBuildTree->RegisterWork(this));
            }
            ASSERT(m_isRegisteredForCallbacks);
        }
        else
        {
            IFC(ListViewBaseGenerated::ClearContainerForItemOverride(element, item));
        }
    }

Cleanup:
    RRETURN(hr);
}

// Causes the object to scroll into view.  If it is not visible, it is aligned
// either at the top or bottom of the viewport.
_Check_return_ HRESULT ListViewBase::ScrollIntoViewImpl(
    _In_ IInspectable* item)
{
    RRETURN(ScrollIntoViewWithOptionalAnimationImpl(item, xaml_controls::ScrollIntoViewAlignment::ScrollIntoViewAlignment_Default, TRUE /*disableAnimation*/));
}

// Causes the object to scroll into view.
// Item will be always aligned based on alignment mode.
_Check_return_ HRESULT ListViewBase::ScrollIntoViewWithAlignmentImpl(
    _In_ IInspectable* item,
    _In_ xaml_controls::ScrollIntoViewAlignment alignment)
{
    RRETURN(ScrollIntoViewWithOptionalAnimationImpl(item, alignment, TRUE /*disableAnimation*/));
}

// Invoked by the public ListViewBase::ScrollIntoView and ListViewBase::ScrollIntoViewWithAlignment methods,
// as well as the private IListViewBasePrivate2::ScrollIntoViewWithOptionalAnimation.
_Check_return_ HRESULT ListViewBase::ScrollIntoViewWithOptionalAnimationImpl(
    _In_ IInspectable* item,
    _In_ xaml_controls::ScrollIntoViewAlignment alignment,
    _In_ BOOLEAN disableAnimation)
{
    ctl::ComPtr<IInspectable> spHeaderOrFooter;
    bool isHeader = false;
    bool isFooter = false;

    IFC_RETURN(get_Header(&spHeaderOrFooter));
    IFC_RETURN(PropertyValue::AreEqual(spHeaderOrFooter.Get(), item, &isHeader));

    if (!isHeader)
    {
        IFC_RETURN(get_Footer(&spHeaderOrFooter));
        IFC_RETURN(PropertyValue::AreEqual(spHeaderOrFooter.Get(), item, &isFooter));
    }

    IFC_RETURN(ScrollIntoViewInternal(item, isHeader, isFooter, TRUE /*isFromPublicAPI*/, alignment, 0.0 /*offset*/, !disableAnimation /*animateIfBringIntoView*/));

    return S_OK;
}

// Called when a pointer moves within a ListViewBase.
IFACEMETHODIMP ListViewBase::OnPointerMoved(
    _In_ IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN isHandled = FALSE;

    IFCPTR(pArgs);
    IFC(ListViewBaseGenerated::OnPointerMoved(pArgs));


    // XAML Drag and Drop on desktop uses drag events to track drag operations
    if (!DXamlCore::GetCurrent()->GetDragDrop()->GetUseCoreDragDrop())
    {
        IFC(pArgs->get_Handled(&isHandled));
        if (!isHandled)
        {
            ctl::ComPtr<ixp::IPointerPoint> spPointerPoint;
            ctl::ComPtr<IPointer> spPointer;

            BOOLEAN wasGestureHandled = FALSE;
            wf::Point newPosition;

            IFC(pArgs->GetCurrentPoint(this, &spPointerPoint));
            IFCPTR(spPointerPoint.Get());

            IFC(spPointerPoint->get_Position(&newPosition));

            IFC(pArgs->get_Pointer(&spPointer));
            IFC(OnDragMoveGesture(spPointer.Get(), newPosition, &wasGestureHandled));
            if (wasGestureHandled)
            {
                IFC(pArgs->put_Handled(TRUE));
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

// Called when the user releases a pointer over the ListViewBase.
IFACEMETHODIMP ListViewBase::OnPointerReleased(
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN isHandled = FALSE;
    IFCPTR(pArgs);
    IFC(ListViewBaseGenerated::OnPointerReleased(pArgs));

    // XAML Drag and Drop on desktop uses drag events to track drag operations
    if (!DXamlCore::GetCurrent()->GetDragDrop()->GetUseCoreDragDrop())
    {
        IFC(pArgs->get_Handled(&isHandled));

        if (!isHandled)
        {
            BOOLEAN wasGestureHandled = FALSE;
            ctl::ComPtr<IPointer> spPointer;
            ctl::ComPtr<ixp::IPointerPoint> spPointerPoint;
            wf::Point dropPoint = {};
            ctl::ComPtr<IUIElement> spVisualRootAsUIE;
            Window* currentWindow = nullptr;

            IFC(DXamlCore::GetCurrent()->GetAssociatedWindowNoRef(this, &currentWindow));
            if (currentWindow == nullptr)
            {
                goto Cleanup;
            }

            IFC(currentWindow->get_Content(&spVisualRootAsUIE));
            IFC(pArgs->GetCurrentPoint(spVisualRootAsUIE.Get(), &spPointerPoint));
            IFC(spPointerPoint->get_Position(&dropPoint));

            IFC(pArgs->get_Pointer(&spPointer));
            IFC(OnDropGesture(
                spPointer.Get(),
                dropPoint,
                &wasGestureHandled));

            if (wasGestureHandled)
            {
                IFC(pArgs->put_Handled(TRUE));
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

// Called when the ListViewBase or its children lose mouse capture.
IFACEMETHODIMP ListViewBase::OnPointerCaptureLost(
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    bool isOriginalSource = false;

    ctl::ComPtr<IInspectable> spSource;
    ctl::ComPtr<IRoutedEventArgs> spArgsAsREA;
    ctl::ComPtr<xaml_input::IPointerRoutedEventArgs> spArgs = pArgs;

    IFC(ListViewBaseGenerated::OnPointerCaptureLost(pArgs));

    // If we are the original source, terminate any mouse-initiated drag.
    // We must check the source because the ListViewItem (our child) will
    // relinquish mouse capture to us when a drag starts, thus causing this
    // method to be called with the child as the original source. We don't want to
    // terminate the drag at that time, hence this check.
    IFC(spArgs.As<IRoutedEventArgs>(&spArgsAsREA));
    IFC(spArgsAsREA->get_OriginalSource(spSource.GetAddressOf()));
    IFC(ctl::are_equal(spSource.Get(), ctl::as_iinspectable(this), &isOriginalSource));

    if (isOriginalSource)
    {
        BOOLEAN wasGestureHandled = FALSE;
        ctl::ComPtr<IPointer> spPointer;
        ctl::ComPtr<ixp::IPointerPoint> spPointerPoint;
        wf::Point dropPoint = {};
        ctl::ComPtr<IUIElement> spVisualRootAsUIE;
        Window* currentWindow = nullptr;

        IFC(DXamlCore::GetCurrent()->GetAssociatedWindowNoRef(this, &currentWindow));
        if (currentWindow == nullptr)
        {
            goto Cleanup;
        }

        IFC(currentWindow->get_Content(&spVisualRootAsUIE));
        IFC(pArgs->get_Pointer(&spPointer));
        IFC(pArgs->GetCurrentPoint(spVisualRootAsUIE.Get(), &spPointerPoint));
        IFC(spPointerPoint->get_Position(&dropPoint));

        IFC(OnDropGesture(
            spPointer.Get(),
            dropPoint,
            &wasGestureHandled));

        if (wasGestureHandled)
        {
            IFC(pArgs->put_Handled(TRUE));
        }
    }

Cleanup:
    RRETURN(hr);
}

// Apply a template to the ListViewBase.
IFACEMETHODIMP ListViewBase::OnApplyTemplate()
{
    HRESULT hr = S_OK;

    if (m_tpScrollViewer)
    {
        IFC(m_tpScrollViewer.Cast<ScrollViewer>()->SetDirectManipulationStateChangeHandler(NULL));

        // ScrollViewer is reset in Selector::OnApplyTemplate, so the property changed handler should be detached.
        IFC(DetachScrollViewerPropertyChanged());

        // Detach from the ScrollViewer's ViewChanging handler
        if (m_epScrollViewerViewChangingHandler)
        {
            IFC(DetachHandler(m_epScrollViewerViewChangingHandler, m_tpScrollViewer));
        }
    }

    IFC(ListViewBaseGenerated::OnApplyTemplate());

    // Update whether we're listening for the ScrollHost's property changes
    // when the template is changed (and the ScrollHost updated)
    IFC(EnsureScrollHostOffsetChangeAction());

    // Attach scroll viewer property handler.
    IFC(AttachScrollViewerPropertyChanged());

    // Tell our ScrollHost to ignore PointerPressed as well as SemanticZoom input
    // when it's used in conjunction with a SemanticZoom
    if (m_tpScrollViewer)
    {
        m_tpScrollViewer.Cast<ScrollViewer>()->m_templatedParentHandlesMouseButton = TRUE;

        IFC(m_tpScrollViewer.Cast<ScrollViewer>()->SetDirectManipulationStateChangeHandler(static_cast<DirectManipulationStateChangeHandler*>(this)));

        ASSERT(!m_epScrollViewerViewChangingHandler);

        IFC(m_epScrollViewerViewChangingHandler.AttachEventHandler(
            m_tpScrollViewer.Cast<ScrollViewer>(),
            [this](_In_ IInspectable*, _In_ IScrollViewerViewChangingEventArgs*)
        {
            return ResetAllItemsForLiveReorder();
        }));
    }

Cleanup:
    RRETURN(hr);
}

// sets a clip the same size as we are, so that elements can portal and be clipped appropriately

// make sure to call this after an arrange has occurred, since that is the moment the ItemsHost
// will have updated its ItemBounds fully.
HRESULT ListViewBase::UpdateClip()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IPanel> spItemsHost;
    ctl::ComPtr<IItemLookupPanel> spItemsHostAsItemLookupPanel;
    ctl::ComPtr<IOrientedPanel> spItemsHostAsOrientedPanel;
    ctl::ComPtr<xaml::IUIElement> spItemsHostAsUIElement;
    wf::Rect bounds = { };

    // use the clip to communicate (and clip) how far elements can move for portal-ing
    // it is an indirect contract between AddDeleteThemeTransition and the elements that it
    // will portal based on the local clip set on the parent of the elements (the panel).
    IFC(get_ItemsHost(&spItemsHost));

    if (spItemsHost != NULL && !ctl::is<IModernCollectionBasePanel>(spItemsHost))
    {
        IFC(spItemsHost.As<xaml::IUIElement>(&spItemsHostAsUIElement));
        spItemsHostAsItemLookupPanel = spItemsHost.AsOrNull<IItemLookupPanel>();

        // only set a local clip if we have anything useful
        // transitions will choose differently if no clip is set
        if (spItemsHostAsItemLookupPanel != NULL)
        {
            ctl::ComPtr<xaml_media::IRectangleGeometry> spClipGeometry;
            IFC(spItemsHostAsItemLookupPanel->GetItemsBounds(&bounds));
            m_maxContentWidth = MAX(m_maxContentWidth, bounds.Width);
            m_maxContentHeight = MAX(m_maxContentHeight, bounds.Height);

            spItemsHostAsOrientedPanel = spItemsHost.AsOrNull<IOrientedPanel>();
            if (spItemsHostAsOrientedPanel != NULL)
            {
                // if this is an oriented panel, we can be better by expanding the bounds in the direction of
                // of scrolling. Need to do that otherwise the clip is too intrusive:
                // Imagine a wrap grid with 4 elements and a height of 3 elements (so two columns, 2nd column showing one element).
                // When we remove an element from the first column, the clip that we are getting back would immediately clip out
                // the LTE that is waiting to be portal-ed in the 2nd column
                xaml_controls::Orientation orientation = xaml_controls::Orientation_Vertical;
                IFC(spItemsHostAsOrientedPanel->get_PhysicalOrientation(&orientation));
                if (orientation == xaml_controls::Orientation_Vertical)
                {
                    bounds.Height = static_cast<FLOAT>(m_maxContentHeight);
                }
                else
                {
                    bounds.Width = static_cast<FLOAT>(m_maxContentWidth);
                }
            }

            IFC(spItemsHostAsUIElement->get_Clip(&spClipGeometry));

            if (spClipGeometry == NULL)
            {
                ctl::ComPtr<RectangleGeometry> spClipGeometryImpl;
                IFC(ctl::make<RectangleGeometry>(&spClipGeometryImpl));
                IFC(spItemsHostAsUIElement->put_Clip(spClipGeometryImpl.Get()));
                spClipGeometry = spClipGeometryImpl;
            }

            IFC(spClipGeometry->put_Rect(bounds));
        }
    }


Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP ListViewBase::ArrangeOverride(
    _In_ wf::Size finalSize,
    _Out_ wf::Size* returnValue)
{
    HRESULT hr = S_OK;

    // allow panel to arrange
    IFC(ListViewBaseGenerated::ArrangeOverride(finalSize, returnValue));

    // In case of DataVirtualization, if number of of items on screen are less than availableSize,
    // this will trigger to LoadMoreItems
    if(m_scrollHostOffsetChangeAction == ScrollHostOffsetChangeAction_IncrementalEdgeTrigger)
    {
        IFC(LoadMoreItemsIfNeeded(finalSize));
    }

    // also executed from OnScrollHostPropertyChanged
    IFC(UpdateClip());

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP ListViewBase::MeasureOverride(
    _In_ wf::Size availableSize,
    _Out_ wf::Size* pDesired)
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES

    // ETW Trace, we want to raise an ETW event here if we can determine that the configuration
    // does not allow virtualization to happen
    if (EventEnabledVirtualizationIsEnabledByLayoutInfo())
    {
        BOOLEAN isVirtualizationActive = TRUE;
        ctl::ComPtr<IPanel> spItemsPanel;

        if (SUCCEEDED(get_ItemsHost(&spItemsPanel)) && ctl::is<IModernCollectionBasePanel>(spItemsPanel))
        {
            if (   availableSize.Width == std::numeric_limits<FLOAT>::infinity()
                || availableSize.Height == std::numeric_limits<FLOAT>::infinity())
            {
                ctl::ComPtr<xaml_controls::ILayoutStrategy> spLayoutStrategy;
                xaml_controls::Orientation orientation = xaml_controls::Orientation::Orientation_Vertical;

                if (  SUCCEEDED(spItemsPanel.Cast<ModernCollectionBasePanel>()->GetLayoutStrategy(&spLayoutStrategy))
                    && SUCCEEDED(spLayoutStrategy->GetVirtualizationDirection(&orientation)))
                {
                    BOOLEAN isWrappingStrategy = FALSE;
                    // if we can't determine strategy, we can't determine true state of virtualization, just jump to cleanup
                    // and not log anything.
                    IFC(spLayoutStrategy->GetIsWrappingStrategy(&isWrappingStrategy));

                    const FLOAT constraint = (orientation == xaml_controls::Orientation::Orientation_Horizontal)
                                            ? availableSize.Width : availableSize.Height;

                    // for wrapping strategy, the virtualization is active only when the availableSize is finite in both directions
                    // which will never be true here in this block of code
                    // otherwise, virtualization is effectively disabled if the constraint is infinite
                    isVirtualizationActive = !isWrappingStrategy && constraint != std::numeric_limits<FLOAT>::infinity();
                }
            }
            ctl::ComPtr<xaml::IDependencyObject> spParent;
            IFC(static_cast<ListViewBase*>(this)->get_Parent(&spParent));
            TraceVirtualizationIsEnabledByLayoutInfo1(
                isVirtualizationActive,
                reinterpret_cast<UINT64>(GetHandle()),
                GetHandle()->m_strName.GetBuffer(),
                GetHandle()->GetClassName().GetBuffer(),
                (spParent) ? static_cast<DependencyObject*>(spParent.Get())->GetHandle()->GetClassName().GetBuffer() : L"NULL"
            );
        } // else if not modern panel, we shouldn't trace it here.
    }


Cleanup:
    RRETURN(ListViewBaseGenerated::MeasureOverride(availableSize, pDesired));
}

// In case of DataVirtualization, if number of of items on screen are less than availableSize,
// this will trigger to LoadMoreItems
_Check_return_ HRESULT ListViewBase::LoadMoreItemsIfNeeded(
    _In_ wf::Size finalSize)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IPanel> spItemsHost;
    ctl::ComPtr<IItemLookupPanel> spItemsHostAsItemLookupPanel;
    ctl::ComPtr<IOrientedPanel> spItemsHostAsOrientedPanel;

    IFC(get_ItemsHost(&spItemsHost));
    if (spItemsHost != NULL)
    {
        spItemsHostAsItemLookupPanel = spItemsHost.AsOrNull<IItemLookupPanel>();
        spItemsHostAsOrientedPanel = spItemsHost.AsOrNull<IOrientedPanel>();

        if (spItemsHostAsItemLookupPanel != NULL && spItemsHostAsOrientedPanel != NULL)
        {
            wf::Rect bounds = { };
            xaml_controls::Orientation orientation = xaml_controls::Orientation_Vertical;
            IFC(spItemsHostAsOrientedPanel->get_PhysicalOrientation(&orientation));
            IFC(spItemsHostAsItemLookupPanel->GetItemsBounds(&bounds));

            if ((orientation == xaml_controls::Orientation_Vertical && bounds.Height <= finalSize.Height) ||
                (orientation == xaml_controls::Orientation_Horizontal && bounds.Width <= finalSize.Width))
            {
                IFC(OnScrollHostOffsetsChanged());
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

// Adds or removes a property changed handler to the ScrollHost.
_Check_return_ HRESULT ListViewBase::EnsureScrollHostOffsetChangeAction()
{
    HRESULT hr = S_OK;
    bool incrementalEdgeTriggerPossible = false;

    if (m_tpScrollViewer != nullptr)
    {
        ctl::ComPtr<wfc::IIterable<IInspectable*>> spItemsSource;
        xaml_controls::IncrementalLoadingTrigger incrementalLoadingTrigger = xaml_controls::IncrementalLoadingTrigger_None;

        IFC(get_ItemsSource(&spItemsSource));

        if (spItemsSource != nullptr)
        {
            // See if we could activate incremental loading edge trigger.
            IFC(get_IncrementalLoadingTrigger(&incrementalLoadingTrigger));
            if (incrementalLoadingTrigger == xaml_controls::IncrementalLoadingTrigger_Edge)
            {
                incrementalEdgeTriggerPossible = IncrementalLoadingAdapter::SupportsIncrementalLoading(spItemsSource.Get());
            }
        }
    }


    // Determine which action we want to take when scrolling happens.
    m_scrollHostOffsetChangeAction = incrementalEdgeTriggerPossible ? ScrollHostOffsetChangeAction_IncrementalEdgeTrigger : ScrollHostOffsetChangeAction_None;

Cleanup:
    RRETURN(hr);
}

// Adds a property changed handler to the ScrollHost.
// Used to call OnScrollHostOffsetsChanged for Data Virtualization.
_Check_return_ HRESULT ListViewBase::AttachScrollViewerPropertyChanged()
{
    HRESULT hr = S_OK;

    if (!m_tpOffsetChangedHandler && m_tpScrollViewer)
    {
        ctl::ComPtr<IDPChangedEventSource> spScrollViewerEventSource;
        ctl::ComPtr<IDPChangedEventHandler> spNewHandler;

        IFC(m_tpScrollViewer.Cast<ScrollViewer>()->GetDPChangedEventSource(spScrollViewerEventSource.GetAddressOf()));

        spNewHandler.Attach(
            new ClassMemberEventHandler<
            ListViewBase,
            IListViewBase,
            IDPChangedEventHandler,
            xaml::IDependencyObject,
            const CDependencyProperty>(this, &ListViewBase::OnScrollHostPropertyChanged));

        SetPtrValue(m_tpOffsetChangedHandler, spNewHandler);

        IFC(spScrollViewerEventSource->AddHandler(spNewHandler.Get()));
    }

Cleanup:
    RRETURN(hr);
}

// Removes the property changed handler we attached in AttachScrollViewerPropertyChanged
// from the ScrollHost. Used for Data Virtualization.
_Check_return_ HRESULT ListViewBase::DetachScrollViewerPropertyChanged()
{
    HRESULT hr = S_OK;

    if (m_tpOffsetChangedHandler)
    {
        if( auto peg = m_tpScrollViewer.TryMakeAutoPeg() )
        {
            ctl::ComPtr<IDPChangedEventSource> spScrollViewerEventSource;

            IFC(m_tpScrollViewer.Cast<ScrollViewer>()->GetDPChangedEventSource(spScrollViewerEventSource.GetAddressOf()));

            IFC(spScrollViewerEventSource->RemoveHandler(m_tpOffsetChangedHandler.Get()));
        }

        m_tpOffsetChangedHandler.Clear();
    }

Cleanup:
    RRETURN(hr);
}

// ScrollHost property changed handler callback
_Check_return_ HRESULT ListViewBase::OnScrollHostPropertyChanged(
    _In_ xaml::IDependencyObject* pSender,
    _In_ const CDependencyProperty* pDP)
{
    HRESULT hr = S_OK;

    KnownPropertyIndex nPropertyIndex = pDP->GetIndex();

    // TODO: Just replace with a check on the property indices for speed
    if ((nPropertyIndex == KnownPropertyIndex::ScrollViewer_HorizontalOffset) ||
        (nPropertyIndex == KnownPropertyIndex::ScrollViewer_VerticalOffset))
    {
        IFC(OnScrollHostOffsetsChanged());
    }
    else
    {
        if ((nPropertyIndex == KnownPropertyIndex::ScrollViewer_ExtentWidth) ||
            (nPropertyIndex == KnownPropertyIndex::ScrollViewer_ExtentHeight))
        {
            // the fact that the extent has changed means that we are going to have a dirty layout pass
            // anyway. There are side effects to this ListView's arrange (calling UpdateClip) that need
            // to execute after the arrange has occurred. The cheapest way of doing that is making sure
            // we participate in the arrange walk.
            IFC(InvalidateArrange());
        }
    }


Cleanup:
    RRETURN(hr);
}

// Called when the ScrollHost's scrolling offsets changes.
_Check_return_ HRESULT ListViewBase::OnScrollHostOffsetsChanged()
{
    IFC_RETURN(ProcessDataVirtualizationScrollOffsets());

    return S_OK;
}

_Check_return_ HRESULT ListViewBase::GetDropOffsetToRoot(_Out_ wf::Point* returnValue)
{
    *returnValue = m_lastReorderPosition;
    RRETURN(S_OK);
}

// DirectManipulationStateChangeHandler implementation
_Check_return_ HRESULT ListViewBase::NotifyStateChange(
    _In_ DMManipulationState state,
    _In_ FLOAT xCumulativeTranslation,
    _In_ FLOAT yCumulativeTranslation,
    _In_ FLOAT zCumulativeFactor,
    _In_ FLOAT xCenter,
    _In_ FLOAT yCenter,
    _In_ BOOLEAN isInertial,
    _In_ BOOLEAN isTouchConfigurationActivated,
    _In_ BOOLEAN isBringIntoViewportConfigurationActivated)
{
    HRESULT hr = S_OK;

    switch (state)
    {
        case DMManipulationStarted:
            IFC(ClearHoldingState());
            break;
    }

Cleanup:
    RRETURN(hr);
}

// return string representation of Header property
_Check_return_
    HRESULT
    ListViewBase::GetPlainText(_Out_ HSTRING* strPlainText)
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

// Returns next focusable element in a specified direction.
// Since Get{First,Last}FocusableElementOverride is called in different context after refactoring change 601761
// this method needs to be called from Panel::GetFirstFocusableElementOverride in order to retain the same call order as in Win8.
_Check_return_ HRESULT ListViewBase::GetFocusableElement(
    const bool isBackward,
    _Outptr_ DependencyObject** ppFocusable)
{
#ifdef LVB_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"LVB(%s)[0x%p]: GetFocusableElement. isBackward=%d",
        ctl::is<xaml_controls::IGridView>(this) ? L"GV" : L"LV", this, isBackward));
#endif

    HRESULT hr = S_OK;
    BOOLEAN shouldCustomizeTabNavigation = FALSE;
    xaml_input::KeyboardNavigationMode navigationMode;
    ctl::ComPtr<xaml_controls::IGroupItem> spGroupItem;
    ctl::ComPtr<IDependencyObject> spFirstFocusableResult;

    *ppFocusable = nullptr;

    // ShouldCustomizeTabNavigation returns whether the ItemsPanel is a ModernCollectionBasePanel or not (shouldCustomizeTabNavigation is set to True for modern panels).
    IFC(ShouldCustomizeTabNavigation(&shouldCustomizeTabNavigation));

    IFC(get_TabNavigation(&navigationMode));

    // The modern panels may not have to use this GetFirst/GetLast mechanism for controlling tab stop navigation, but rather
    // ProcessTabStopOverride and ProcessCandidateTabStopOverride as those methods work for some scenarios involving
    // enter, in-LVB and exit transitions. 
    // There are cases though requiring the following custom processing, even for modern panels and the default Once TabNavigation:
    // - ListViewBaseItem instances have nested focusable elements and the 2 overrides above are not invoked for this ListViewBase.
    // - irrespective of the TabNavigation mode, the last focused index (returned by GetLastFocusedIndex()) may not be realized
    //   anymore and may require item realization, scrolling and preparation.
    const bool isUsingTabNavigationOnce = navigationMode == xaml_input::KeyboardNavigationMode::KeyboardNavigationMode_Once;
    const bool isUsingTabNavigationCycle = navigationMode == xaml_input::KeyboardNavigationMode::KeyboardNavigationMode_Cycle;
    bool scrollIntoViewAndPrepareContainer = false;
    BOOLEAN isGrouping = FALSE;
    BOOLEAN hasFocus = FALSE;
    INT targetFocusedIndex = -1;
    INT targetGroupFocusedIndex = -1;

    IFC(get_IsGrouping(&isGrouping));
    IFC(HasFocus(&hasFocus));

    // For modern panels (i.e. shouldCustomizeTabNavigation == True), 
    // - for TabNavigation modes Local or Cycle, the first (for isBackward==False) or last (for isBackward==True) item may not be realized,
    // - for TabNavigation mode Once, the last focused item may not be realized.
    // In those cases, that item must be realized and prepared first and selected as the focusable element.
    if (!isGrouping && shouldCustomizeTabNavigation)
    {
        CFocusManager* focusManager = VisualTree::GetFocusManagerForElement(GetHandle());

        ASSERT(focusManager);

        if ((!isBackward && focusManager->IsMovingFocusToNextTabStop()) || (isBackward && focusManager->IsMovingFocusToPreviousTabStop()))
        {
            // The FocusManager is processing the Tab key to actually navigate to an element (as opposed to only accessing a tab stop).
            scrollIntoViewAndPrepareContainer = true;
        }
    }

    // If neither focused item nor focused group exists, retrieve GroupItem for last focused item, if grouping.
    // If not grouping, get container for last focused item.
    // Additional check for HasFocus is necessary because focused index, etc. are set on OnGotFocus/LostFocus and this is async,
    // so even if the indices are set they may be slightly out of sync. Check that focus is within this ListViewBase before using the indices.
    // There is still a possibility that they will not be up to date if focus was rapidly changed within the ListViewBase, but that is not an important
    // scenario.
    // Even when HasFocus is True, when TabNavigation is Cycle, the target index may have to be realized and prepared when cycling from the first to
    // the last item or vice-versa.
    if ((GetFocusedIndex() < 0 && GetFocusedGroupIndex() < 0) ||
        (isUsingTabNavigationCycle && scrollIntoViewAndPrepareContainer) ||
        !hasFocus)
    {
        if (isGrouping)
        {
            ctl::ComPtr<xaml_data::ICollectionViewGroup> spGroup;
            IFC(GetGroupFromItem(
                GetLastFocusedIndex(),
                &spGroup,
                &spGroupItem,
                &targetGroupFocusedIndex,
                NULL));
        }

        if (!spGroupItem)
        {
            if (!isUsingTabNavigationOnce)
            {
                UINT itemCount = 0;

                IFC(GetItemCount(itemCount));

                if (itemCount > 0)
                {
                    ASSERT(!(isUsingTabNavigationCycle && isBackward && GetFocusedIndex() == 0 && !hasFocus));
                    ASSERT(!(isUsingTabNavigationCycle && !isBackward && GetFocusedIndex() == itemCount - 1 && !hasFocus));

                    if (!isUsingTabNavigationCycle ||                                                     // TabNavigation is Local, tabbing into the first item or shift-tabbing into the last one.
                        (isUsingTabNavigationCycle && !hasFocus) ||                                       // TabNavigation is Cycle, hasFocus is False, tabbing into the first item or shift-tabbing into the last one.
                        (isUsingTabNavigationCycle && isBackward && GetFocusedIndex() == 0) ||            // TabNavigation is Cycle, hasFocus is True, alter the default behavior when shift-tabbing from the first to the last item.
                        (isUsingTabNavigationCycle && !isBackward && GetFocusedIndex() == itemCount - 1)) // TabNavigation is Cycle, hasFocus is True, alter the default behavior when tabbing from the last to the first item.
                    {
                        targetFocusedIndex = (!isBackward) ? 0 : itemCount - 1;
                    }
                }
            }
            else
            {
                targetFocusedIndex = static_cast<INT>(GetLastFocusedIndex());
            }
        }
    }

    if (spGroupItem)
    {
        ASSERT(targetFocusedIndex < 0);
        IFC(spGroupItem.As<IDependencyObject>(&spFirstFocusableResult));
    }
    else if (targetFocusedIndex >= 0)
    {
        IFC(ContainerFromIndex(targetFocusedIndex, &spFirstFocusableResult));

        if (scrollIntoViewAndPrepareContainer)
        {
            ctl::ComPtr<IDependencyObject> itemContainerCandidate = spFirstFocusableResult;

            // Make sure the target index is realized and prepared so it can be declared focusable and tabbed into.
            // Since this method brings the target index into view, it is only invoked when the FocusManager is processing
            // the Tab key to actually navigate to an element (as opposed to only accessing a tab stop).
            IFC(GetScrolledIntoViewAndPreparedContainer(targetFocusedIndex, itemContainerCandidate.Detach(), &spFirstFocusableResult));
        }
    }

    *ppFocusable = static_cast<DependencyObject*>(spFirstFocusableResult.Detach());

Cleanup:
    RRETURN(hr);
}

// Shared implementation for GetFirstFocusableElementOverride and GetLastFocusableElementOverride.
_Check_return_ HRESULT ListViewBase::GetFocusableElementForModernPanel(
    _In_ BOOLEAN isBackward,
    _Outptr_ DependencyObject** ppFocusable)
{
#ifdef LVB_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"LVB(%s)[0x%p]: GetFocusableElementForModernPanel. isBackward=%d",
        ctl::is<xaml_controls::IGridView>(this) ? L"GV" : L"LV", this, isBackward));
#endif

    BOOLEAN shouldCustomizeTabNavigation = FALSE;

    *ppFocusable = nullptr;

    IFC_RETURN(ShouldCustomizeTabNavigation(&shouldCustomizeTabNavigation));

    if (shouldCustomizeTabNavigation)
    {
        // Takes care of the initial case where focused element is still NULL and
        // ProcessTabOverride has not been called.
        ctl::ComPtr<DependencyObject> spFocusedElement;

        IFC_RETURN(GetFocusedElement(&spFocusedElement));

        if (!spFocusedElement && !m_focusableElementForModernPanelReentrancyGuard)
        {
            BOOLEAN unused = FALSE;

            // there is a reentrancy issue since ListView calls back into focus manager and
            // he will call back into the ListView if it happens to be that the ListView is hosted in a popup
            // and there are no other focusable elements in the scene
            m_focusableElementForModernPanelReentrancyGuard = true;

            auto processTabStopInternal = wil::scope_exit([&]()
            {
                m_focusableElementForModernPanelReentrancyGuard = false;
            });

            IFC_RETURN(ProcessTabStopInternal(
                nullptr, // pFocusedElement
                nullptr, // pCandidateElement
                isBackward,
                false,   // didCycleFocusAtRootVisualScope,
                &unused,
                ppFocusable));
        }
    }

    RRETURN(S_OK);
}

// Gets first element that should take focus after Tab.
_Check_return_ HRESULT ListViewBase::GetFirstFocusableElementOverride(
    _Outptr_ DependencyObject** ppFirstFocusable)
{
#ifdef LVB_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"LVB(%s)[0x%p]: GetFirstFocusableElementOverride.", ctl::is<xaml_controls::IGridView>(this) ? L"GV" : L"LV", this));
#endif

    RRETURN(GetFocusableElementForModernPanel(
        FALSE,
        ppFirstFocusable));
}

// Gets last element that should take focus after Shift+Tab.
_Check_return_ HRESULT ListViewBase::GetLastFocusableElementOverride(
    _Outptr_ DependencyObject** ppLastFocusable)
{
#ifdef LVB_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"LVB(%s)[0x%p]: GetLastFocusableElementOverride.", ctl::is<xaml_controls::IGridView>(this) ? L"GV" : L"LV", this));
#endif

    RRETURN(GetFocusableElementForModernPanel(
        TRUE,
        ppLastFocusable));
}

// Needs override in Win8 RTM and higher to preserve focus on focused item when Focus() is called on a non
// focusable ListViewBase. See explanation under quirked code for more details.
_Check_return_ HRESULT DirectUI::ListViewBase::FocusImpl(
    _In_ xaml::FocusState value,
    _Out_ BOOLEAN* returnValue
    )
{
#ifdef LVB_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"LVB(%s)[0x%p]: FocusImpl. focusState=%d", 
        ctl::is<xaml_controls::IGridView>(this) ? L"GV" : L"LV", this, value));
#endif

    HRESULT hr = S_OK;
    bool isFocusable = false;
    INT targetFocusedIndex = -1;
    INT targetGroupFocusedIndex = -1;
    BOOLEAN hasFocus = FALSE;
    BOOLEAN focused = FALSE;

    // Win8 RTM and higher - ListViewBase handles focus through a FocusManager callback instead of
    // having items transfer focus after they get focused. This enables TabNavigation.Cycle and programmatic
    // focus to work, but for the case where an item within ListViewBase already has focus and then Focus() is
    // called on ListViewBase, we need to go to the item which had focus assuming ListViewBase is not focusable.

    // Check if ListViewBase is focusable - if it is, it can take focus itself and there is no need to transfer
    // focus elsewhere. However, it may not be Focusable, e.g. IsTabStop=False, in which case focus should go to
    // a focused or focusable child.
    IFC(CoreImports::UIElement_IsFocusable(static_cast<CUIElement *>(GetHandle()), &isFocusable));

    if (!isFocusable)
    {
        // Next check to see if the ListViewBase currently has focus on an item/group using HasFocus and focused indices.
        // If nothing is currently focused, we'll fall back to the FocusManager::GetFirstFocusableElement path which will
        // return the previously focused item or group. However, if an item within ListViewBase is currently
        // focused the GetFirstFocusableElement callback will not return anything, since it mainly handles the case
        // where focus is coming from outside. This is necessary because FocusManager calls GetFirst/LastFocusable child both for
        // tabbing and programmatic focus and doesn't have any specific overrides for tabs, which is what ListViewBase really requires.
        IFC(HasFocus(&hasFocus));
        if (hasFocus)
        {
            if (GetFocusedIndex() >= 0)
            {
                targetFocusedIndex = static_cast<INT>(GetFocusedIndex());
            }
            else if (GetFocusedGroupIndex() >= 0)
            {
                targetGroupFocusedIndex = static_cast<INT>(GetFocusedGroupIndex());
            }
        }

        if (targetFocusedIndex >= 0 ||
            targetGroupFocusedIndex >= 0)
        {
            ctl::ComPtr<xaml::IDependencyObject> spContainer;

            if (targetFocusedIndex >= 0)
            {
                IFC(ContainerFromIndex(targetFocusedIndex, &spContainer));
            }
            else
            {
                IFC(HeaderFromIndex(targetGroupFocusedIndex, &spContainer));
            }

            if (spContainer)
            {
                IFC(static_cast<CFrameworkElement*>(spContainer.Cast<FrameworkElement>()->GetHandle())->InvokeFocus(static_cast<FocusState>(value), &focused));
            }
        }
    }

    if (!focused)
    {
        IFC(ListViewBaseGenerated::InvokeFocus(value, &focused));
    }
    *returnValue = focused;

Cleanup:
    RRETURN(hr);
}

// UIElement override for getting next tab stop on path from focused element to root.
_Check_return_ HRESULT ListViewBase::ProcessTabStopOverride(
    _In_opt_ DependencyObject* pFocusedElement,
    _In_opt_ DependencyObject* pCandidateTabStopElement,
    const bool isBackward,
    const bool didCycleFocusAtRootVisualScope,
    _Outptr_ DependencyObject** ppNewTabStop,
    _Out_ BOOLEAN* pIsTabStopOverridden)
{
#ifdef LVB_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"LVB(%s)[0x%p]: ProcessTabStopOverride. isBackward=%d", 
        ctl::is<xaml_controls::IGridView>(this) ? L"GV" : L"LV", this, isBackward));
#endif

    HRESULT hr = S_OK;

    IFC(ProcessTabStopInternal(
            pFocusedElement,
            pCandidateTabStopElement,
            isBackward,
            didCycleFocusAtRootVisualScope,
            pIsTabStopOverridden,
            ppNewTabStop));

Cleanup:
    RRETURN(hr);
}

// UIElement override for getting next tab stop on path from focus candidate element to root.
_Check_return_ HRESULT ListViewBase::ProcessCandidateTabStopOverride(
    _In_opt_ DependencyObject* pFocusedElement,
    _In_ DependencyObject* pCandidateTabStopElement,
    _In_opt_ DependencyObject* pOverriddenCandidateTabStopElement,
    const bool isBackward,
    _Outptr_ DependencyObject** ppNewTabStop,
    _Out_ BOOLEAN* pIsCandidateTabStopOverridden)
{
#ifdef LVB_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"LVB(%s)[0x%p]: ProcessCandidateTabStopOverride. isBackward=%d", 
        ctl::is<xaml_controls::IGridView>(this) ? L"GV" : L"LV", this, isBackward));
#endif

    BOOLEAN isAncestor = FALSE;
    IFC_RETURN(IsAncestorOf(pFocusedElement, &isAncestor));

    if (!isAncestor)
    {
        xaml_input::KeyboardNavigationMode navigationMode;
        IFC_RETURN(get_TabNavigation(&navigationMode));

        if (navigationMode == xaml_input::KeyboardNavigationMode::KeyboardNavigationMode_Once)
        {
            // For KeyboardNavigationMode == Once, tab or shift-tab into the previously visited item, if any.
            // Or first/last selectable item otherwise.
            IFC_RETURN(ProcessTabStopInternal(
                nullptr, // pFocusedElement
                nullptr, // pCandidateElement
                isBackward,
                false,   // didCycleFocusAtRootVisualScope,
                pIsCandidateTabStopOverridden,
                ppNewTabStop));
        }
        else
        {
            // Attempt to find a tab stop in this priority order: the header if present, 
            // the group headers and items, finally the footer if present. That order is
            // reversed when shift-tabbing.
            ElementType newElementType = Other;
            ctl::ComPtr<IDependencyObject> nextElement;

            IFC_RETURN(TryNextElementType(
                Other /*initialElementType*/,
                isBackward ? Footer : Header /*candidateElementType*/,
                isBackward,
                &newElementType,
                &nextElement));
            
            if (!nextElement)
            {
                IFC_RETURN(TryNextElementType(
                    Other /*initialElementType*/,
                    ItemOrGroupHeader /*candidateElementType*/,
                    isBackward,
                    &newElementType,
                    &nextElement));
            }

            if (!nextElement)
            {
                IFC_RETURN(TryNextElementType(
                    Other /*initialElementType*/,
                    isBackward ? Header : Footer /*candidateElementType*/,
                    isBackward,
                    &newElementType,
                    &nextElement));
            }

            if (nextElement)
            {
                // When a tab stop element is found, try to locate its first or last inner tab stop
                // when tabbing or shift-tabbing respectively.
                xref_ptr<CDependencyObject> tabStopNative;

                if (isBackward)
                {
                    IFC_RETURN(CoreImports::FocusManager_GetLastFocusableElement(
                        nextElement.Cast<DependencyObject>()->GetHandle(),
                        tabStopNative.ReleaseAndGetAddressOf()));
                }
                else
                {
                    IFC_RETURN(CoreImports::FocusManager_GetFirstFocusableElement(
                        nextElement.Cast<DependencyObject>()->GetHandle(),
                        tabStopNative.ReleaseAndGetAddressOf()));
                }

                if (tabStopNative)
                {
                    ctl::ComPtr<DependencyObject> tabStop;

                    IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(tabStopNative, &tabStop));

                    *ppNewTabStop = static_cast<DependencyObject*>(tabStop.Detach());
                }
                else
                {
                    // Fall back to using the element itself when no inner tab stop was found.
                    *ppNewTabStop = static_cast<DependencyObject*>(nextElement.Detach());
                }

                *pIsCandidateTabStopOverridden = TRUE;
            }
        }
    }

    return S_OK;
}

// Ensure itemIndex is within valid range for group with index of groupIndex.  If forceSet is TRUE, then
// set pNewItemIndex to the first item index (atBeginning = TRUE) or the last.
_Check_return_ HRESULT ListViewBase::ValidateItemIndexForGroup(
    _In_ UINT itemIndex,
    _In_ UINT groupIndex,
    _In_ BOOLEAN atBeginning,
    _In_ BOOLEAN forceSet,
    _Out_ UINT* pNewItemIndex)
{
    HRESULT hr = S_OK;
    UINT groupStartIndex = 0;
    UINT groupSize = 0;
    BOOLEAN isValid = FALSE;

    *pNewItemIndex = itemIndex;

    IFC(GetGroupInformation(
        groupIndex,
        &groupStartIndex,
        &groupSize,
        &isValid));

    if (isValid &&
        groupSize > 0)
    {
        if (forceSet ||
            itemIndex < groupStartIndex ||
            itemIndex >= (groupStartIndex + groupSize))
        {
            if (atBeginning)
            {
                *pNewItemIndex = groupStartIndex;
            }
            else
            {
                *pNewItemIndex = groupStartIndex + groupSize - 1;
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

// Test if there are any elements in group.
_Check_return_ HRESULT ListViewBase::GroupHasElements(
    _In_ UINT groupIndex,
    _Out_ BOOLEAN* pGroupHasElements)
{
    HRESULT hr = S_OK;
    INT groupCount = 0;

#ifdef DBG
    BOOLEAN isGrouping = FALSE;
    IFC(get_IsGrouping(&isGrouping));
    ASSERT(isGrouping);
#endif

    *pGroupHasElements = FALSE;

    IFC(GetGroupCount(&groupCount));

    if (groupCount > 0)
    {
        UINT groupStartIndex = 0;
        UINT groupSize = 0;
        BOOLEAN isValid = FALSE;

        IFC(GetGroupInformation(
            groupIndex,
            &groupStartIndex,
            &groupSize,
            &isValid));

        *pGroupHasElements = (isValid && groupSize > 0);
    }

Cleanup:
    RRETURN(hr);
}

// Test if there are empty group headers at the beginning or at the end.
_Check_return_ HRESULT ListViewBase::HasBoundaryEmptyGroup(
    _In_ BOOLEAN atBeginning,
    _Out_ BOOLEAN* pHasBoundaryEmptyGroup)
{
    HRESULT hr = S_OK;
    BOOLEAN isGrouping = FALSE;
    INT groupCount = 0;

    *pHasBoundaryEmptyGroup = FALSE;

    IFC(get_IsGrouping(&isGrouping));

    if (isGrouping)
    {
        IFC(GetGroupCount(&groupCount));

        if (groupCount > 0)
        {
            INT groupIndex = 0;
            ctl::ComPtr<xaml_data::ICollectionViewGroup> spGroup;

            if (atBeginning)
            {
                IFC(GetGroupFromItem(
                    0,
                    &spGroup,
                    nullptr,
                    &groupIndex,
                    nullptr));

                *pHasBoundaryEmptyGroup = (groupIndex > 0);
            }
            else
            {
                UINT itemCount = 0;

                IFC(GetItemCount(itemCount));

                if (itemCount > 0)
                {
                    IFC(GetGroupFromItem(
                        itemCount - 1,
                        &spGroup,
                        nullptr,
                        &groupIndex,
                        nullptr));

                    *pHasBoundaryEmptyGroup = (groupIndex + 1 < groupCount);
                }
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

// Walk up visual tree from pChild towards this LVB, looking for the last element which is an
// item, group header item, header or footer.
_Check_return_ HRESULT ListViewBase::IdentifyParentElement(
    _In_opt_ IDependencyObject* pChild,
    _Out_ ElementType* pParentElementType,
    _Outptr_ IDependencyObject** ppParent)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IDependencyObject> spCurrent = pChild;
    ctl::ComPtr<IDependencyObject> spLastFoundParent;
    ctl::ComPtr<IItemsPresenter> spItemsPresenter;
    ctl::ComPtr<ContentControl> spHeaderContainer;
    ctl::ComPtr<ContentControl> spFooterContainer;

    *pParentElementType = ElementType::Other;
    *ppParent = nullptr;

    IFC(get_ItemsPresenter(&spItemsPresenter));

    if (spItemsPresenter)
    {
        IFC(spItemsPresenter.Cast<ItemsPresenter>()->get_HeaderContainer(&spHeaderContainer));
        IFC(spItemsPresenter.Cast<ItemsPresenter>()->get_FooterContainer(&spFooterContainer));
    }

    while (spCurrent)
    {
        bool found = false;
        IInspectable* pCurrentAsInspectable = ctl::iinspectable_cast(spCurrent.Get());

        IFC(ctl::are_equal(
            ctl::iinspectable_cast(this),
            pCurrentAsInspectable,
            &found));

        // We are interested in the closest element to the LVB, therefore keep searching
        // until we are at the root of hierarchy or parent LVB.

        if (found)
        {
            break;
        }

        // Since most likely the number of items is the greatest, test it first.
        if (IsListViewBaseItem(pCurrentAsInspectable) || IsListViewBaseHeaderItem(pCurrentAsInspectable))
        {
            spLastFoundParent = spCurrent;
            *pParentElementType = ElementType::ItemOrGroupHeader;
        }
        else
        {
            // Is it a header?
            IFC(ctl::are_equal(
                ctl::iinspectable_cast(spHeaderContainer.Get()),
                pCurrentAsInspectable,
                &found));

            if (found)
            {
                spLastFoundParent = spCurrent;
                *pParentElementType = ElementType::Header;
            }
            else
            {
                // Maybe a footer?
                IFC(ctl::are_equal(
                    ctl::iinspectable_cast(spFooterContainer.Get()),
                    pCurrentAsInspectable,
                    &found));

                if (found)
                {
                    spLastFoundParent = spCurrent;
                    *pParentElementType = ElementType::Footer;
                }
            }
        }

        {
            // Advance to parent.
            ctl::ComPtr<xaml::IDependencyObject> spParent;
            IFC(VisualTreeHelper::GetParentStatic(spCurrent.Get(), &spParent));
            spCurrent = spParent;
        }
    }

    IFC(spLastFoundParent.MoveTo(ppParent));

Cleanup:
    RRETURN(hr);
}

// Updates flags which determine whether items and group headers are visited during tab navigation.
// In order to avoid potential realization of all elements, only one item/group header are checked.
// The assumption is that all elements of one type will have the same tab stop value.
// Violating this assumption will have the following consequences:
// * prototype is tab-stoppable but there is an element which is not - tabbing will get stuck on
//   last element which was tab-stoppable.
// * prototype is not tab-stoppable but there is element which is - the element will not be reached.
// For the purpose of tab navigation, the following properties are taken into account to make a determination
// whether item/group header should be visited:
// It's not a Control, but visible UIElement OR
// it's a Control which is visible, enabled and is either a tab stop or one of its children is a tab stop.
_Check_return_ HRESULT ListViewBase::UpdateTabStopFlags()
{
    HRESULT hr = S_OK;
    BOOLEAN isGrouping = FALSE;
    unsigned int itemCount = 0;

    ASSERT(m_firstAvailableItemForFocus == -1);
    ASSERT(m_firstAvailableGroupHeaderForFocus == -1);

    m_itemsAreTabStops = FALSE;
    m_groupsAreTabStops = FALSE;

    m_firstAvailableItemForFocus = 0;
    m_firstAvailableGroupHeaderForFocus = 0;

    IFC(get_IsGrouping(&isGrouping));
    IFC(GetItemCount(itemCount));

    if (itemCount > 0)
    {
        int itemCacheStart = -1;
        int itemCacheEnd = -1;

        IFC(GetCacheStartAndEnd(
            false /*isForGroupIndexes*/,
            &itemCacheStart,
            &itemCacheEnd));

        if (itemCacheStart >= 0)
        {
            ctl::ComPtr<xaml::IDependencyObject> spContainer;

            // loop across the cached containers to find the first item that can focused
            while (!m_itemsAreTabStops && itemCacheStart <= itemCacheEnd)
            {
                IFC(ContainerFromIndex(itemCacheStart, &spContainer));

                if (spContainer)
                {
                    IFC(ItemsControl::IsFocusableHelper(
                        ctl::iinspectable_cast(spContainer.Get()),
                        m_itemsAreTabStops));
                }
                ++itemCacheStart;
            }

            // if we were able to find an item to set focus on
            // we set the index for the first available item for focus
            if (m_itemsAreTabStops)
            {
                // itemCacheStart is incremented in the loop above hence we need to decrement it by 1
                m_firstAvailableItemForFocus = itemCacheStart - 1;
            }
        }
    }

    if (isGrouping)
    {
        int groupCount = 0;

        IFC(GetGroupCount(&groupCount));

        if (groupCount > 0)
        {
            int groupCacheStart = -1;
            int groupCacheEnd = -1;

            IFC(GetCacheStartAndEnd(
                true /*isForGroupIndexes*/,
                &groupCacheStart,
                &groupCacheEnd));

            ctl::ComPtr<xaml::IDependencyObject> spHeader;

            // loop across the cached header containers to find the first group header that can be focused
            while (!m_groupsAreTabStops && groupCacheStart <= groupCacheEnd)
            {
                IFC(HeaderFromIndex(groupCacheStart, &spHeader));

                if (spHeader)
                {
                    IFC(ItemsControl::IsFocusableHelper(
                        ctl::iinspectable_cast(spHeader.Get()),
                        m_groupsAreTabStops));
                }
                ++groupCacheStart;
            }

            // if we were able to find a group header to set focus on
            // we set the index for the first available group header for focus
            if (m_groupsAreTabStops)
            {
                // groupCacheStart is incremented in the loop above hence we need to decrement it by 1
                m_firstAvailableGroupHeaderForFocus = groupCacheStart - 1;
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

// Given a starting element type transition states until the next tab stop is found.
_Check_return_ HRESULT ListViewBase::GetNextTabStopForElementType(
    _In_ ElementType elementType,
    const bool isBackward,
    _Outptr_ xaml::IDependencyObject** ppNextElement)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IDependencyObject> spNextElement;
    ElementType candidateElementType = elementType;
    std::vector<ElementType> visitedStates;
    std::vector<ElementType>::const_iterator foundVisitedState;

    *ppNextElement = nullptr;

    do
    {
        // Sanity check to avoid infinite loop in case if the state machine can't determine next tab stop.
        foundVisitedState = std::find(
            visitedStates.begin(),
            visitedStates.end(),
            candidateElementType);

        if (foundVisitedState != visitedStates.end())
        {
            ASSERT(FALSE, L"State already visited");
            break;
        }

        visitedStates.push_back(candidateElementType);

        IFC(TryNextElementType(
            elementType,
            candidateElementType,
            isBackward,
            &candidateElementType,
            &spNextElement));
    }
    while (!spNextElement &&
        candidateElementType != Other);

    IFC(spNextElement.MoveTo(ppNextElement));

Cleanup:
    RRETURN(hr);
}

// Get next tab stop for currently focused element type.
_Check_return_ HRESULT ListViewBase::GetNextTabStop(
    _In_ ElementType elementType,
    const bool isBackward,
    _Outptr_ IDependencyObject** ppNextElement)
{
#ifdef LVB_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"LVB(%s)[0x%p]: GetNextTabStop. elementType=%d, isBackward=%d", 
        ctl::is<xaml_controls::IGridView>(this) ? L"GV" : L"LV", this, elementType, isBackward));
#endif

    HRESULT hr = S_OK;
    ctl::ComPtr<IDependencyObject> spNextElement;
    CDependencyObject* pNextElementFromBack = nullptr;
    xaml_input::KeyboardNavigationMode navigationMode;

    *ppNextElement = nullptr;

    IFC(get_TabNavigation(&navigationMode));

    IFC(GetNextTabStopForElementType(
        elementType,
        isBackward,
        &spNextElement));

    // If did not find tab stop and navigation mode is Cycle, start over as if we are tabbing from outside element.
    if (!spNextElement &&
        navigationMode == xaml_input::KeyboardNavigationMode::KeyboardNavigationMode_Cycle)
    {
        IFC(GetNextTabStopForElementType(
            Other,
            isBackward,
            &spNextElement));
    }

    // Go to the last focusable element within next tab stop if we are shift-tabbing.
    if (isBackward &&
        spNextElement)
    {
        IFC(CoreImports::FocusManager_GetLastFocusableElement(
            spNextElement.Cast<DependencyObject>()->GetHandle(),
            &pNextElementFromBack));

        if (pNextElementFromBack)
        {
            ctl::ComPtr<DependencyObject> spNextElementFromBack;

            IFC(DXamlCore::GetCurrent()->GetPeer(pNextElementFromBack, &spNextElementFromBack));
            IFC(spNextElementFromBack.As(&spNextElement));
        }
    }

    IFC(spNextElement.MoveTo(ppNextElement));

Cleanup:
    ReleaseInterface(pNextElementFromBack);
    RRETURN(hr);
}

// State machine for determining next tab stop.
// Given the current state information it tests if the transition to candidate state is a valid one
// and if so, it returns a container for the next tab stop.  If the candidate is not accepted, it
// advances the state.
// initialElementType - type of element currently having focus
// candidateElementType - state being tested
// isBackward - direction
// pNewElementType - new state transition
// ppNextElement - next tab stop
_Check_return_ HRESULT ListViewBase::TryNextElementType(
    _In_ ElementType initialElementType,
    _In_ ElementType candidateElementType,
    const bool isBackward,
    _Out_ ElementType* pNewElementType,
    _Outptr_ IDependencyObject** ppNextElement)
{
    xaml_input::KeyboardNavigationMode navigationMode;
    ctl::ComPtr<IDependencyObject> spNextElement;
    BOOLEAN isGrouping = FALSE;
    UINT itemCount = 0;
    INT groupCount = 0;

    *pNewElementType = candidateElementType;
    *ppNextElement = nullptr;

    IFC_RETURN(get_TabNavigation(&navigationMode));
    IFC_RETURN(get_IsGrouping(&isGrouping));
    IFC_RETURN(GetItemCount(itemCount));

    if (isGrouping)
    {
        IFC_RETURN(GetGroupCount(&groupCount));
    }

    const bool supportsItemsAsTabStops = itemCount != 0 && m_itemsAreTabStops;
    const bool supportsGroupHeadersAsTabStops = isGrouping && groupCount != 0 && m_groupsAreTabStops;

    switch(candidateElementType)
    {
    case Header:
    case Footer:
        {
            if (initialElementType == candidateElementType)
            {
                // Go to the next element type if this one already had focus.
                *pNewElementType = GetElementTypeTransition(candidateElementType, isBackward);
            }
            else
            {
                // If not, get appropriate content control and if it is selectable, return it to caller.
                ctl::ComPtr<ContentControl> focusableFooterContainer;
                BOOLEAN isHeader = (candidateElementType == Header);

                IFC_RETURN(GetFocusableHeaderOrFooterContainer(isHeader, &focusableFooterContainer));

                if (focusableFooterContainer)
                {
                    IFC_RETURN(Selector::ScrollIntoView(
                        0,
                        FALSE /*isGroupItemIndex*/,
                        isHeader  /*isHeader*/,
                        !isHeader /*isFooter*/,
                        FALSE /*isFromPublicAPI*/,
                        TRUE  /*ensureContainerRealized*/,
                        FALSE /*animateIfBringIntoView*/,
                        xaml_controls::ScrollIntoViewAlignment::ScrollIntoViewAlignment_Default));

                    IFC_RETURN(focusableFooterContainer.As(&spNextElement));
                }

                if (!spNextElement)
                {
                    // If container cannot be returned, advance the state.
                    *pNewElementType = GetElementTypeTransition(candidateElementType, isBackward);
                }
            }

            if (navigationMode == xaml_input::KeyboardNavigationMode::KeyboardNavigationMode_Local ||
                navigationMode == xaml_input::KeyboardNavigationMode::KeyboardNavigationMode_Cycle)
            {
                // Per spec: for local/cycle navigation modes iterate over all elements disregarding last focused element.
                // When entering from outside of items/groups, if the NavMode is not Once, reset the group and item indices
                // and the last focused element type.
                if (!isBackward)
                {
                    SetLastFocusedGroupIndex(0);
                    SetLastFocusedIndex(0);

                    // Forward navigation with local/cycle should try to hit group headers first,
                    // otherwise try items.
                    if (supportsGroupHeadersAsTabStops)
                    {
                        m_lastFocusedElementType = ElementType::GroupHeader;
                    }
                    else if (supportsItemsAsTabStops)
                    {
                        m_lastFocusedElementType = ElementType::Item;
                    }
                }
                else
                {
                    if (itemCount > 0)
                    {
                        SetLastFocusedIndex(itemCount - 1);
                    }

                    if (isGrouping &&
                        groupCount > 0)
                    {
                        SetLastFocusedGroupIndex(groupCount - 1);
                    }

                    // Backward navigation with local/cycle should try to hit items first,
                    // otherwise try group headers.
                    if (supportsItemsAsTabStops)
                    {
                        m_lastFocusedElementType = ElementType::Item;
                    }
                    else if (supportsGroupHeadersAsTabStops)
                    {
                        m_lastFocusedElementType = ElementType::GroupHeader;
                    }
                }

                if (isGrouping &&
                    *pNewElementType == Item &&
                    !isBackward &&
                    (initialElementType == Header || initialElementType == Other))
                {
                    // If there are any empty group headers at the beginning, in Local or Cycle mode visit them first.
                    BOOLEAN hasBoundaryEmptyGroup = FALSE;

                    IFC_RETURN(HasBoundaryEmptyGroup(
                        TRUE,
                        &hasBoundaryEmptyGroup));

                    if (hasBoundaryEmptyGroup &&
                        m_groupsAreTabStops)
                    {
                        *pNewElementType = GroupHeader;
                    }
                }
            }
        }
        break;

    case ItemOrGroupHeader:
        {
            if ((!supportsItemsAsTabStops && !supportsGroupHeadersAsTabStops)
                || (initialElementType == ItemOrGroupHeader && navigationMode == xaml_input::KeyboardNavigationMode::KeyboardNavigationMode_Once))
            {
                // If neither items nor group headers are supported as tab stops, or an item/group header already has focus and NavMove==Once,
                // then advance to the next state.
                *pNewElementType = GetElementTypeTransition(ItemOrGroupHeader, isBackward);
            }
            else
            {
                int newFocusIndex = -1;
                bool isGroupHeader = false;

                if (initialElementType != ItemOrGroupHeader)
                {
                    // The first time we tab into a list, either try to restore focus to the last element that had
                    // focus within the list or focus either the first group header (if available) or the
                    // first item (unless it's a bottoms up list, in which case we focus the last item).

                    bool isGettingFocusForFirstTime = true;

                    if (m_lastFocusedElementType != ElementType::Other)
                    {
                        // The control has previously had focus on either an item or group header, so we
                        // should restore it.

                        isGroupHeader = (m_lastFocusedElementType == ElementType::GroupHeader);
                        newFocusIndex = (isGroupHeader ? GetLastFocusedGroupIndex() : GetLastFocusedIndex());
                        if (newFocusIndex >= 0 && newFocusIndex < (isGroupHeader ? groupCount : static_cast<int>(itemCount)))
                        {
                            // Last focused element was an item that is currently available in the data.
                            isGettingFocusForFirstTime = false;
                        }
                    }

                    if (isGettingFocusForFirstTime)
                    {
                        // This is the first time the control is getting focus, so focus the first group
                        // header (if supported), otherwise the first item, except the case of bottoms-up
                        // lists, where we will instead focus the last item.

                        bool isBottomsUp = false;

                        ctl::ComPtr<xaml_controls::IPanel> panel;
                        IFC_RETURN(get_ItemsPanelRoot(&panel));
                        auto itemsStackPanel = panel.AsOrNull<xaml_controls::IItemsStackPanel>();
                        if (itemsStackPanel)
                        {
                            auto itemsUpdatingScrollMode = xaml_controls::ItemsUpdatingScrollMode_KeepItemsInView;
                            IFC_RETURN(itemsStackPanel->get_ItemsUpdatingScrollMode(&itemsUpdatingScrollMode));
                            isBottomsUp = (itemsUpdatingScrollMode == xaml_controls::ItemsUpdatingScrollMode_KeepLastItemInView);
                        }

                        if (isBottomsUp)
                        {
                            newFocusIndex = itemCount - 1;
                        }
                        else
                        {
                            isGroupHeader = supportsGroupHeadersAsTabStops;
                            newFocusIndex = (isGroupHeader ? m_firstAvailableGroupHeaderForFocus : m_firstAvailableItemForFocus);
                        }
                    }
                }
                else
                {
                    // For subsequent tab navigation, while on a group header or item, just advance in order through
                    // the list, similar to using arrow keys.

                    if (m_lastFocusedElementType == ElementType::Item)
                    {
                        ASSERT(supportsItemsAsTabStops);

                        IFC_RETURN(FindNextTabStopCandidateFromItem(
                            GetLastFocusedIndex(),
                            isBackward,
                            supportsGroupHeadersAsTabStops,
                            &newFocusIndex,
                            &isGroupHeader));
                    }
                    else if (m_lastFocusedElementType == ElementType::GroupHeader
                        && (!isBackward || GetLastFocusedGroupIndex() > 0))
                    {
                        ASSERT(supportsGroupHeadersAsTabStops);

                        unsigned int newGroupStartIndex = 0;
                        unsigned int newGroupSize = 0;
                        BOOLEAN isValid = FALSE;

                        IFC_RETURN(GetGroupInformation(
                            static_cast<unsigned int>((isBackward) ? GetLastFocusedGroupIndex() - 1 : GetLastFocusedGroupIndex()),
                            &newGroupStartIndex,
                            &newGroupSize,
                            &isValid));

                        if (isValid)
                        {
                            if (isBackward)
                            {
                                if (newGroupSize > 0)
                                {
                                    // Focus the last item in the new group.
                                    newFocusIndex = newGroupStartIndex + newGroupSize - 1;
                                }
                                else
                                {
                                    // The previous group is empty, so focus its header instead.
                                    newFocusIndex = GetLastFocusedGroupIndex() - 1;
                                    isGroupHeader = true;
                                }
                            }
                            else
                            {
                                if (newGroupSize > 0)
                                {
                                    // Focus the first item in the current group.
                                    newFocusIndex = newGroupStartIndex;
                                }
                                else if (GetLastFocusedGroupIndex() < (groupCount - 1))
                                {
                                    // The current group is empty, so focus the next group's header instead.
                                    newFocusIndex = GetLastFocusedGroupIndex() + 1;
                                    isGroupHeader = true;
                                }
                            }
                        }
                    }
                }

                if (newFocusIndex >= 0 && newFocusIndex < (isGroupHeader ? groupCount : static_cast<int>(itemCount)))
                {
                    IFC_RETURN(Selector::ScrollIntoView(
                        newFocusIndex,
                        isGroupHeader ? TRUE : FALSE /*isGroupItemIndex*/,
                        FALSE /*isHeader*/,
                        FALSE /*isFooter*/,
                        FALSE /*isFromPublicAPI*/,
                        TRUE  /*ensureContainerRealized*/,
                        FALSE /*animateIfBringIntoView*/,
                        xaml_controls::ScrollIntoViewAlignment::ScrollIntoViewAlignment_Default));

                    if (isGroupHeader)
                    {
                        IFC_RETURN(HeaderFromIndex(newFocusIndex, &spNextElement));
                    }
                    else
                    {
                        ctl::ComPtr<xaml::IDependencyObject> container;

                        int itemCacheStart = -1;
                        int itemCacheEnd = -1;
                        int currentIndex = -1;

                        IFC_RETURN(GetCacheStartAndEnd(
                            false /*isForGroupIndexes*/,
                            &itemCacheStart,
                            &itemCacheEnd));

                        currentIndex = newFocusIndex < itemCacheStart ? itemCacheStart : newFocusIndex;
                        currentIndex = newFocusIndex > itemCacheEnd ? itemCacheEnd : newFocusIndex;

                        // Make sure our candidate item is selectable.  If not, keep trying until we find
                        // one that is.  We'll do this within the currently realized items to prevent going
                        // through the entire list and if we fail to find a selectable item or group header
                        // within that set, we'll just end up advancing to the next element type.
                        while (currentIndex >= itemCacheStart && currentIndex <= itemCacheEnd)
                        {
                            IFC_RETURN(ContainerFromIndex(currentIndex, &container));
                            if (container)
                            {
                                BOOLEAN isFocusable = FALSE;

                                IFC_RETURN(ItemsControl::IsFocusableHelper(
                                    ctl::iinspectable_cast(container.Get()),
                                    isFocusable));

                                if (isFocusable)
                                {
                                    spNextElement = container;
                                    break;
                                }
                            }

                            IFC_RETURN(FindNextTabStopCandidateFromItem(
                                currentIndex,
                                isBackward,
                                (isGrouping && groupCount > 0 && m_groupsAreTabStops),
                                &currentIndex,
                                &isGroupHeader));

                            if (isGroupHeader)
                            {
                                IFC_RETURN(HeaderFromIndex(currentIndex, &spNextElement));
                                break;
                            }
                        }
                    }
                }

                // If we failed to find a focus candidate, then advance the state.
                if (!spNextElement)
                {
                    *pNewElementType = GetElementTypeTransition(ItemOrGroupHeader, isBackward);
                }
            }
        }
        break;

    case Other:
        *pNewElementType = GetElementTypeTransition(Other, isBackward);
        break;
    }

    IFC_RETURN(spNextElement.MoveTo(ppNextElement));

    return S_OK;
}

_Check_return_ HRESULT
ListViewBase::FindNextTabStopCandidateFromItem(
    int referenceItemIndex,
    bool isBackward,
    bool supportGroupHeaders,
    _Out_ int* focusCandidateIndex,
    _Out_ bool* isCandidateAGroupHeader)
{
#ifdef LVB_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"LVB(%s)[0x%p]: FindNextTabStopCandidateFromItem. referenceItemIndex=%d, isBackward=%d, supportGroupHeaders=%d", 
        ctl::is<xaml_controls::IGridView>(this) ? L"GV" : L"LV", this, referenceItemIndex, isBackward, supportGroupHeaders));
#endif

    unsigned int itemCount = 0;
    IFC_RETURN(GetItemCount(itemCount));

    // Our reference index is expected to be valid.
    ASSERT(referenceItemIndex >= 0 && referenceItemIndex < static_cast<int>(itemCount));

    // Advance item index.
    int newFocusIndex = (isBackward) ? referenceItemIndex - 1 : referenceItemIndex + 1;
    bool isGroupHeader = false;

    if (supportGroupHeaders)
    {
        int groupCount = 0;
        IFC_RETURN(GetGroupCount(&groupCount));

        int referenceItemGroupIndex = 0;
        ctl::ComPtr<xaml_data::ICollectionViewGroup> group;
        IFC_RETURN(GetGroupFromItem(
            referenceItemIndex,
            &group,
            nullptr,
            &referenceItemGroupIndex,
            nullptr));

        if (newFocusIndex < 0)
        {
            // If navigating past the first item, focus this item's group header instead.
            newFocusIndex = referenceItemGroupIndex;
            isGroupHeader = true;
        }
        else if (newFocusIndex >= static_cast<int>(itemCount))
        {
            // If navigating past the last item, see if there are empty group headers at the end that
            // we can focus.
            BOOLEAN hasEmptyGroupAtEnd = FALSE;
            IFC_RETURN(HasBoundaryEmptyGroup(TRUE, &hasEmptyGroupAtEnd));
            if (hasEmptyGroupAtEnd && referenceItemGroupIndex < (groupCount - 1))
            {
                newFocusIndex = referenceItemGroupIndex + 1;
                isGroupHeader = true;
            }
        }
        else
        {
            // If we're navigating past a group boundary, focus the new group header instead.
            int newFocusedItemGroupIndex = 0;
            IFC_RETURN(GetGroupFromItem(
                newFocusIndex,
                &group,
                nullptr,
                &newFocusedItemGroupIndex,
                nullptr));

            if (referenceItemGroupIndex != newFocusedItemGroupIndex)
            {
                // If we are crossing a group boundary, then just advance the group header when moving
                // forward by 1 to account for empty groups between the last focused item's group
                // and the suggested focus item's group.
                newFocusIndex = isBackward ? referenceItemGroupIndex : referenceItemGroupIndex + 1;
                isGroupHeader = true;
            }
        }
    }

    *focusCandidateIndex = newFocusIndex;
    *isCandidateAGroupHeader = isGroupHeader;

    return S_OK;
}

// Helper for getting next focusable control outside of LVB.
_Check_return_ HRESULT ListViewBase::GetNextFocusablePeer(
    const bool isBackward,
    _Outptr_ IDependencyObject** ppOutsidePeer)
{
#ifdef LVB_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"LVB(%s)[0x%p]: GetNextFocusablePeer. isBackward=%d", 
        ctl::is<xaml_controls::IGridView>(this) ? L"GV" : L"LV", this, isBackward));
#endif

    xref_ptr<CDependencyObject> nextFocusableElement;

    *ppOutsidePeer = nullptr;

    if (isBackward)
    {
        nextFocusableElement = VisualTree::GetFocusManagerForElement(GetHandle())->GetPreviousTabStop(GetHandle());
    }
    else
    {
        nextFocusableElement = VisualTree::GetFocusManagerForElement(GetHandle())->GetNextTabStop(GetHandle(), TRUE);
    }

    if (!nextFocusableElement)
    {
        ctl::ComPtr<IApplicationBarService> applicationBarService;

        if (const auto xamlRoot = XamlRoot::GetImplementationForElementStatic(this))
        {
            IFC_RETURN(xamlRoot->GetApplicationBarService(applicationBarService));
        }

        // Process TabStop if the application bar service is available.
        if (applicationBarService)
        {
            ctl::ComPtr<AppBar> spTopAppBar;
            ctl::ComPtr<AppBar> spBottomAppBar;

            // We look for all AppBars (not just open ones) since they can be in minimal state and receive focus
            IFC_RETURN(applicationBarService->GetTopAndBottomAppBars(&spTopAppBar, &spBottomAppBar));

            IFC_RETURN(applicationBarService->GetFirstFocusableElementFromAppBars(
                    spTopAppBar.Get(),
                    spBottomAppBar.Get(),
                    isBackward ? AppBarTabPriority_Top : AppBarTabPriority_Bottom,
                    isBackward ? TRUE : FALSE,
                    nextFocusableElement.ReleaseAndGetAddressOf()));
        }
    }

    if (nextFocusableElement)
    {
        ctl::ComPtr<DependencyObject> spOutsidePeer;
        IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(nextFocusableElement, &spOutsidePeer));
        IFC_RETURN(spOutsidePeer.MoveTo(ppOutsidePeer));
    }

    return S_OK;
}

// If there is a pending scroll into view in the panel, get the target element
// of that scroll into view
_Check_return_ HRESULT ListViewBase::GetFocusCandidateFromPanel(
    _Outptr_ xaml::IDependencyObject** ppFocusCandidate)
{
#ifdef LVB_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"LVB(%s)[0x%p]: GetFocusCandidateFromPanel.",
        ctl::is<xaml_controls::IGridView>(this) ? L"GV" : L"LV", this));
#endif

    INT itemIndex = -1;
    INT groupIndex = -1;
    ctl::ComPtr<IPanel> spItemsPanel;
    ctl::ComPtr<IUIElement> spFocusElement;

    *ppFocusCandidate = nullptr;

    IFC_RETURN(get_ItemsHost(&spItemsPanel));

    ASSERT(ctl::is<IModernCollectionBasePanel>(spItemsPanel));

    IFC_RETURN(spItemsPanel.Cast<ModernCollectionBasePanel>()->GetFocusCandidate(&itemIndex, &groupIndex, &spFocusElement));

    if (itemIndex != -1 || groupIndex != -1)
    {
        BOOLEAN isGrouping = FALSE;

        // panel provided a valid container to focus. use that.
        spFocusElement.AsOrNull<IDependencyObject>().MoveTo(ppFocusCandidate);

        IFC_RETURN(get_IsGrouping(&isGrouping));
        if (isGrouping)
        {
            ctl::ComPtr<xaml_data::ICollectionViewGroup> spGroup;

            if (itemIndex != -1)
            {
                IFC_RETURN(GetGroupFromItem(
                    itemIndex,
                    &spGroup,
                    nullptr /* GroupItem */,
                    &groupIndex,
                    nullptr /* IndexInGroup */));
                SetLastFocusedGroupIndex(groupIndex);
                SetLastFocusedIndex(itemIndex);
            }
            else
            {
                ASSERT(groupIndex != -1);
                SetLastFocusedGroupIndex(groupIndex);
            }
        }
        else
        {
            ASSERT(groupIndex == -1);
            SetLastFocusedIndex(itemIndex);
        }
    }

    return S_OK;
}

// Determines the next tab stop given currently focused element and default focus candidate.
_Check_return_ HRESULT ListViewBase::ProcessTabStopInternal(
    _In_opt_ DependencyObject* pCurrentFocus,
    _In_opt_ DependencyObject* pFocusCandidate,
    const bool isBackward,
    const bool didCycleFocusAtRootVisualScope,
    _Out_ BOOLEAN* pHandled,
    _Outptr_ DependencyObject** ppTabStop)
{
#ifdef LVB_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"LVB(%s)[0x%p]: ProcessTabStopInternal. isBackward=%d",
        ctl::is<xaml_controls::IGridView>(this) ? L"GV" : L"LV", this, isBackward));
#endif

    HRESULT hr = S_OK;
    bool isUsingModernPanel = false;
    bool isUsingOrientedVirtualizingPanel = false;
    bool isUsingFocusableHeader = false;
    bool isUsingFocusableFooter = false;
    bool isUsingLocalOrCycleTabNavigation = false;
    bool isTabIntoOrientedVirtualizingPanel = false;
    ctl::ComPtr<IDependencyObject> spCurrentParent;
    ctl::ComPtr<IDependencyObject> spCandidateParent;
    ElementType currentParentType = Other;
    ElementType candidateParentType = Other;

    *ppTabStop = nullptr;
    *pHandled = FALSE;

    BOOLEAN shouldCustomizeTabNavigation = FALSE;

    IFC(ShouldCustomizeTabNavigation(&shouldCustomizeTabNavigation));

    isUsingModernPanel = shouldCustomizeTabNavigation;

    if (!isUsingModernPanel)
    {
        ctl::ComPtr<IPanel> itemsPanel;

        IFC(get_ItemsHost(&itemsPanel));

        isUsingOrientedVirtualizingPanel = ctl::is<IOrientedVirtualizingPanel>(itemsPanel);

        if (!isUsingOrientedVirtualizingPanel && !isBackward)
        {
            ctl::ComPtr<ContentControl> focusableFooterContainer;

            IFC(GetFocusableHeaderOrFooterContainer(false /*isForHeader*/, &focusableFooterContainer));

            isUsingFocusableFooter = focusableFooterContainer != nullptr;
        }
        else if (isBackward)
        {
            ctl::ComPtr<ContentControl> focusableFooterContainer;

            IFC(GetFocusableHeaderOrFooterContainer(true /*isForHeader*/, &focusableFooterContainer));

            isUsingFocusableHeader = focusableFooterContainer != nullptr;
        }

        // isTabIntoOrientedVirtualizingPanel is set to True when tabbing into a VirtualizingStackPanel to ensure its potential 
        // focusable Header is brought into view because OrientedVirtualizingPanel::MakeVisible is not supported.
        isTabIntoOrientedVirtualizingPanel = isUsingOrientedVirtualizingPanel && !isBackward && !pCurrentFocus && !pFocusCandidate;

        xaml_input::KeyboardNavigationMode navigationMode = xaml_input::KeyboardNavigationMode::KeyboardNavigationMode_Once;

        IFC(get_TabNavigation(&navigationMode));

        isUsingLocalOrCycleTabNavigation = navigationMode != xaml_input::KeyboardNavigationMode::KeyboardNavigationMode_Once;
    }

    IFC(IdentifyParentElement(
        pCurrentFocus,
        &currentParentType,
        &spCurrentParent));

    IFC(IdentifyParentElement(
        pFocusCandidate,
        &candidateParentType,
        &spCandidateParent));

    if (isUsingModernPanel ||                                                                                                                                 // #1
        isTabIntoOrientedVirtualizingPanel ||                                                                                                                 // #2
        isUsingLocalOrCycleTabNavigation ||                                                                                                                   // #3
        (currentParentType == Header && candidateParentType == Other && !isBackward) ||                                                                       // #4
        (isUsingFocusableHeader && currentParentType == ItemOrGroupHeader && candidateParentType == Other) ||                                                 // #5
        (isUsingFocusableFooter && pCurrentFocus && pFocusCandidate && currentParentType == ItemOrGroupHeader && candidateParentType == Other) ||             // #6
        (!isUsingOrientedVirtualizingPanel && pCurrentFocus && pFocusCandidate && currentParentType == Footer && candidateParentType == Other && isBackward)) // #7
    {
        // This custom handling is not only needed for modern panels in general (#1), but also with a VirtualizingStackPanel or StackPanel:
        // - when tabbing into a VirtualizingStackPanel because OrientedVirtualizingPanel::MakeVisible is not supported and the focused Header would not be brought into view. (#2)
        // - when navigationMode is 'Cycle' and tabbing to loop from the last item to the first, or shift-tabbing from the first to the last. (#3)
        // - when navigationMode is 'Local' and tabbing or shift-tabbing out of the control, while the last or first item is focused respectively. (#3)
        // - when navigationMode is 'Once' and tabbing from the Header to a focusable item, otherwise the items are not reachable with the keyboard. (#4)
        // - when navigationMode is 'Once' and shift-tabbing from a focusable item to the Header, otherwise the Header is not reachable with the keyboard. (#5)
        // - when navigationMode is 'Once' and tabbing from a focusable item to the Footer in the StackPanel, otherwise the Footer is not reachable with the keyboard. (#6)
        // - when navigationMode is 'Once' and shift-tabbing from the Footer to a focusable item in the StackPanel, otherwise the items are not reachable with the keyboard. (#7)
        bool doProcessTabStopOverride = true;

        if (spCurrentParent || spCandidateParent)
        {
            // In certain scenarios with TabNavigation == Once, we can get pCurrentFocus == pFocusCandidate.
            // For these cases continue to process the override.
            if (!ctl::are_equal(pCurrentFocus, pFocusCandidate))
            {
                // For cases where the current focused element and the candidate element both have the same
                // parent (sibling focusable elements within the header/footer, or within a group header, or
                // within an item), We don't generally want to continue processing the tab stop override because
                // we'd interfere with normal tabbing behavior within those sub-trees, such as just tabbing
                // through the elements or tab cycling within the sub-tree.
                // However, if focus has wrapped around at the root level, such as when the ListView is set to
                // TabNavigation == Once and it is the root focusable control in the tree, then continue processing
                // the tab stop override; we don't want to mistakenly identify this situation as normal cycling
                // within current focused element's parent's sub-tree (see bug #10607771).
                if (ctl::are_equal(spCurrentParent.Get(), spCandidateParent.Get()) && !didCycleFocusAtRootVisualScope)
                {
                    doProcessTabStopOverride = false;
                }
            }
        }

        if (doProcessTabStopOverride)
        {
            ctl::ComPtr<IDependencyObject> spNextTabStop;

            IFC(UpdateTabStopFlags());

            if (isUsingModernPanel && !m_wasItemOrGroupHeaderFocused)
            {
                // modern panel and nothing is focused, check if the modern panel has an item
                // it wants to focus to. This can happen if there is a pending scroll into view.
                IFC(GetFocusCandidateFromPanel(&spNextTabStop));
            }

            if (!spNextTabStop)
            {
                IFC(GetNextTabStop(
                    currentParentType,
                    isBackward,
                    &spNextTabStop));
            }

            if (!spNextTabStop)
            {
                // No tab stop suggestions, then go outside of control to next/previous.
                IFC(GetNextFocusablePeer(
                    isBackward,
                    &spNextTabStop));
            }

            // Only provide tab stop suggestion if there is one and it's inside ListViewBase.
            // This is to fix cases like TFS #1535996 whereby the focus manager will find the roll-over focus candidate but this control
            // would override the tab stop to NULL thus causing the roll over to not occur.
            if (spNextTabStop ||
                (candidateParentType != Other && spCandidateParent))
            {
                *ppTabStop = static_cast<DependencyObject*>(spNextTabStop.Detach());
                *pHandled = TRUE;
                m_wasItemOrGroupHeaderFocused = true;
            }
        }
    }

Cleanup:
    m_firstAvailableItemForFocus = -1;
    m_firstAvailableGroupHeaderForFocus = -1;

    RRETURN(hr);
}

// Get the next state from current one in the specified direction.
// The order for isBackward = FALSE is: Other -> Header-> Item -> GroupHeader -> Footer -> Other
ListViewBase::ElementType ListViewBase::GetElementTypeTransition(
    _In_ ElementType current,
    const bool isBackward)
{
    ElementType next = Other;

    switch (current)
    {
    case Other:
        next = (isBackward) ? Footer : Header;
        break;

    case Header:
        next = (isBackward) ? Other : ItemOrGroupHeader;
        break;

    case Item:
        next = (isBackward) ? Header : GroupHeader;
        break;

    case GroupHeader:
        next = (isBackward) ? Item : Footer;
        break;

    case ItemOrGroupHeader:
        next = (isBackward) ? Header : Footer;
        break;

    case Footer:
        next = (isBackward) ? ItemOrGroupHeader : Other;
        break;
    }

    return next;
}

// For certain situations (such as drag/drop), the host may hang onto a container for an extended
// period of time. That particular container shouldn't ever be recycled as long as it's being used.
// This method asks whether or not the given container is eligible for recycling.
_Check_return_ IFACEMETHODIMP
ListViewBase::CanRecycleContainer(
    _In_ xaml::IDependencyObject* pContainer,
    _Out_ BOOLEAN* pCanRecycleContainer)
{
    HRESULT hr = S_OK;
    bool isDraggedContainer = false;

    // We shouldn't ever recycle the dragged container.
    IFC(ctl::are_equal(m_tpPrimaryDraggedContainer.Get(), pContainer, &isDraggedContainer));
    *pCanRecycleContainer = !isDraggedContainer;

Cleanup:
    RRETURN(hr);
}

_Check_return_ IFACEMETHODIMP
ListViewBase::SuggestContainerForContainerFromItemLookup(
    _Outptr_ xaml::IDependencyObject** ppContainer)
{
    // yay, i might know better
    m_spContainerBeingClicked.CopyTo(ppContainer);
    RRETURN(S_OK);
}

// Sets up a link from group header to parent LVB used to relay calls.  Used only for modern panels.
_Check_return_ IFACEMETHODIMP
ListViewBase::PrepareGroupContainer(
    _In_ xaml::IDependencyObject* pContainer,
    _In_ xaml_data::ICollectionViewGroup* pGroup)
{
    HRESULT hr = S_OK;

    IFC(ListViewBaseGenerated::PrepareGroupContainer(pContainer, pGroup));

    if (IsListViewBaseHeaderItem(ctl::iinspectable_cast(pContainer)))
    {
        ctl::ComPtr<IContentControl> spIContentControlItem;
        IFC(ctl::do_query_interface(spIContentControlItem, pContainer));
        IFC(spIContentControlItem.Cast<ListViewBaseHeaderItem>()->SetParent(this));
    }

Cleanup:
    RRETURN(hr);
}

// Clears a link from group header to parent LVB used to relay calls.  Used only for modern panels.
_Check_return_ IFACEMETHODIMP
ListViewBase::ClearGroupContainerForGroup(
    _In_ xaml::IDependencyObject* pContainer,
    _In_opt_ xaml_data::ICollectionViewGroup* pGroup)
{
    HRESULT hr = S_OK;

    IFC(ListViewBaseGenerated::ClearGroupContainerForGroup(pContainer, pGroup));

    if (IsListViewBaseHeaderItem(ctl::iinspectable_cast(pContainer)))
    {
        ctl::ComPtr<IContentControl> spIContentControlItem;
        IFC(ctl::do_query_interface(spIContentControlItem, pContainer));
        IFC(spIContentControlItem.Cast<ListViewBaseHeaderItem>()->SetParent(nullptr));
    }

Cleanup:
    RRETURN(hr);
}

// Called when the ListViewBase has been constructed and added to the visual
// tree.
_Check_return_ HRESULT
ListViewBase::OnLoaded(
    _In_ IInspectable* pSender,
    _In_ xaml::IRoutedEventArgs* pArgs)
{
    IFC_RETURN(RegisterIfWorkPending());

    if (m_deferredPoint == DeferPoint::ListViewBaseLoaded)
    {
        IFC_RETURN(ExecuteDeferredScrollCommand());
    }

    return S_OK;
}

// Schedules a deferred scroll command.
void
ListViewBase::ScheduleDeferredScrollCommand(
    _In_ std::function<HRESULT(IInspectable*)>&& command,
    _In_ IInspectable* pScrollToItem,
    _In_ DeferPoint deferUntil)
{
    // It only makes sense to schedule the most recent
    // scroll command because that's what decide where we
    // are going to end up in the list anyways.
    // We make sure, however, that we clear all the tracker
    // pointers related to deferred scrolling once we execute
    // the command.

    SetPtrValue(m_tpDeferredScrollToItem, pScrollToItem);
    m_deferredScrollCommand = std::move(command);

    m_deferredPoint = deferUntil;
}

// Executes a deferred scroll command (if any) and clears it.
_Check_return_ HRESULT
ListViewBase::ExecuteDeferredScrollCommand()
{
    HRESULT hr = S_OK;

    if (m_deferredScrollCommand)
    {
        IFC(m_deferredScrollCommand(m_tpDeferredScrollToItem.Get()));
        m_isDeferredElementFooter = false;
    }

Cleanup:
    // Make sure we clear any 'global' state related
    // to deferred scrolling.
    m_tpDeferredScrollToItem.Clear();
    m_deferredScrollCommand = nullptr;
    RRETURN(hr);
}

_Check_return_ HRESULT
ListViewBase::OnItemsHostAvailable()
{
    IFC_RETURN(ItemsControl::OnItemsHostAvailable());

    // if we have a pending deferred command and the command wants to
    // scroll to the footer, don't delay load the footer
    if (m_isDeferredElementFooter)
    {
        ctl::ComPtr<IItemsPresenter> spItemsPresenter;
        IFC_RETURN(get_ItemsPresenter(&spItemsPresenter));

        if (spItemsPresenter)
        {
            IFC_RETURN(spItemsPresenter.Cast<ItemsPresenter>()->LoadFooter(FALSE /* updateLayout */));
        }
    }

    if (m_deferredPoint == DeferPoint::ItemsHostAvailable)
    {
        IFC_RETURN(ExecuteDeferredScrollCommand());
    }

    return S_OK;
}

bool ListViewBase::IsDraggableOrPannableImpl()
{
    BOOLEAN dragEnabled = FALSE;
    VERIFYHR(GetIsDragEnabled(&dragEnabled));

    return !!dragEnabled;
}

_Check_return_ HRESULT
ListViewBase::OnGettingFocus(_In_ xaml::IUIElement* sender, _In_ xaml_input::IGettingFocusEventArgs* args)
{
#ifdef LVB_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"LVB(%s)[0x%p]: OnGettingFocus.", ctl::is<xaml_controls::IGridView>(this) ? L"GV" : L"LV", this));
#endif

    // Don't take any action if focus is being set programmatically.  We only care to take
    // action when it's set in response to some interaction like tab, arrow keys, or
    // gamepad navigation.
    auto focusNavigationDirection = xaml_input::FocusNavigationDirection_None;
    IFC_RETURN(args->get_Direction(&focusNavigationDirection));
    if (focusNavigationDirection == xaml_input::FocusNavigationDirection_None)
    {
        return S_OK;
    }

    if (focusNavigationDirection == xaml_input::FocusNavigationDirection_Next ||
        focusNavigationDirection == xaml_input::FocusNavigationDirection_Previous)
    {
        auto navigationMode = xaml_input::KeyboardNavigationMode::KeyboardNavigationMode_Once;

        IFC_RETURN(get_TabNavigation(&navigationMode));

        if (navigationMode != xaml_input::KeyboardNavigationMode::KeyboardNavigationMode_Once)
        {
            // When tabbing or shift-tabbing into the ListViewBase while TabNavigation is 'Local' 
            // or 'Cycle', do not give focus to the latest focused element.
            // The first or last focusable element is getting focus instead (either the header, 
            // a group header, an item, or the footer).
            return S_OK;
        }
    }

    auto selectionMode = xaml_controls::ListViewSelectionMode_None;
    IFC_RETURN(get_SelectionMode(&selectionMode));

    int focusIndex = -1;

    // If SingleSelectionFollowsFocus is true and we have a selected item, then re-direct
    // focus to the selected item if focus is coming from the outside, or is coming from the
    // header or footer, and is targeting an item or group header.
    BOOLEAN singleSelectionFollowsFocus = FALSE;
    IFC_RETURN(get_SingleSelectionFollowsFocus(&singleSelectionFollowsFocus));
    if (singleSelectionFollowsFocus && selectionMode == xaml_controls::ListViewSelectionMode_Single)
    {
        int selectedIndex = -1;

        IFC_RETURN(get_SelectedIndex(&selectedIndex));

        focusIndex = selectedIndex;
    }
    else if (focusNavigationDirection == xaml_input::FocusNavigationDirection_Next ||
             focusNavigationDirection == xaml_input::FocusNavigationDirection_Previous)
    {
        focusIndex = GetLastFocusedIndex();
    }

    if (focusIndex != -1)
    {
        ctl::ComPtr<IDependencyObject> elementParent;

        ctl::ComPtr<IDependencyObject> newFocusedElement;
        IFC_RETURN(args->get_NewFocusedElement(&newFocusedElement));

        ctl::ComPtr<IDependencyObject> oldFocusedElement;
        IFC_RETURN(args->get_OldFocusedElement(&oldFocusedElement));

        ElementType newFocusedParentElementType = Other;
        IFC_RETURN(IdentifyParentElement(newFocusedElement.Get(), &newFocusedParentElementType, &elementParent));
        bool isFocusMovingOntoItemOrGroupHeader = (newFocusedParentElementType == Item || newFocusedParentElementType == GroupHeader || newFocusedParentElementType == ItemOrGroupHeader);

        BOOLEAN isAncestor = FALSE;
        IFC_RETURN(IsAncestorOf(oldFocusedElement.Cast<DependencyObject>(), &isAncestor));
        bool isFocusComingFromOutside = !isAncestor;

        // If the old element is a child of this ListViewBase element, then see if it's a child
        // of either the header or footer objects.
        bool isFocusComingFromHeaderOrFooter = false;
        if (isAncestor)
        {
            ElementType oldFocusedParentElementType = Other;
            IFC_RETURN(IdentifyParentElement(oldFocusedElement.Get(), &oldFocusedParentElementType, &elementParent));
            isFocusComingFromHeaderOrFooter = (oldFocusedParentElementType == Header || oldFocusedParentElementType == Footer);
        }

        if ((isFocusComingFromOutside || isFocusComingFromHeaderOrFooter) && isFocusMovingOntoItemOrGroupHeader)
        {
            ctl::ComPtr<IItemContainerMapping> itemMapping;
            BOOLEAN isFocusable = FALSE;

            IFC_RETURN(GetItemContainerMapping(&itemMapping));
            IFC_RETURN(IsItemFocusable(itemMapping.Get(), focusIndex, &isFocusable));

            if (isFocusable)
            {
                // Make sure the focused index is clear so that when we scroll to the focused item
                // and end up realizing items we won't try to set focus to any of them because setting
                // focus on an element within a GettingFocus handler is not allowed and would return an
                // error.
                // This will get set back to the focused item index when it receives focus, as a result
                // of this GettingFocus handler, in SelectorItem::ItemFocused.
                SetFocusedIndex(-1);

                IFC_RETURN(Selector::ScrollIntoView(
                    focusIndex,
                    FALSE /*isGroupItemIndex*/,
                    FALSE /*isHeader*/,
                    FALSE /*isFooter*/,
                    FALSE /*isFromPublicAPI*/,
                    TRUE  /*ensureContainerRealized*/,
                    FALSE /*animateIfBringIntoView*/,
                    xaml_controls::ScrollIntoViewAlignment::ScrollIntoViewAlignment_Default));

                ctl::ComPtr<IDependencyObject> focusIndexContainer;

                IFC_RETURN(ContainerFromIndex(focusIndex, &focusIndexContainer));
                IFC_RETURN(focusIndexContainer.Cast<DependencyObject>()->IsAncestorOf(newFocusedElement.Cast<DependencyObject>(), &isAncestor));

                // When isAncestor is True, focus is about to move to a child of focusIndexContainer. No need to alter this imminent focus change.
                // In particular, setting focus to the container instead of its child would break the tab order when Shift-Tabbing into the ListViewBase.
                if (!isAncestor)
                {
                    BOOLEAN focusMoved = false;

                    IFC_RETURN(static_cast<GettingFocusEventArgs*>(args)->TrySetNewFocusedElement(focusIndexContainer.Get(), &focusMoved));
                    IFC_RETURN(args->put_Handled(focusMoved));
                }
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
ListViewBase::PrepareConnectedAnimationImpl(_In_ HSTRING key, _In_ IInspectable* pItem, _In_ HSTRING elementName, _Outptr_ xaml_animation::IConnectedAnimation** ppReturnValue)
{
    ctl::ComPtr<xaml::IUIElement> animationElement;
    {
        ctl::ComPtr<xaml_controls::IContentControl> containerElement;
        ctl::ComPtr<xaml::IDependencyObject> object;
        IFC_RETURN(ContainerFromItem(pItem, &object));
        IFC_RETURN(object.As(&containerElement));

        if (containerElement != nullptr)
        {
            ctl::ComPtr<xaml::IUIElement> contentElement;
            IFC_RETURN(containerElement->get_ContentTemplateRoot(&contentElement));

            if (contentElement != nullptr)
            {
                ctl::ComPtr<xaml::IFrameworkElement> contentRoot;
                IFC_RETURN(contentElement.As(&contentRoot));

                ctl::ComPtr<IInspectable> animationElementInspectable;
                IFC_RETURN(contentRoot->FindName(elementName, &animationElementInspectable));

                if (animationElementInspectable != nullptr)
                {
                    IFC_RETURN(animationElementInspectable.As(&animationElement));
                }
            }
        }
    }

    if (animationElement != nullptr)
    {
        ctl::ComPtr<xaml_animation::IConnectedAnimationService> service;
        {
            ctl::ComPtr<ConnectedAnimationServiceFactory> connectedAnimationServiceStatics;
            IFC_RETURN(ctl::make<ConnectedAnimationServiceFactory>(&connectedAnimationServiceStatics));
            IFC_RETURN(connectedAnimationServiceStatics->GetForCurrentView(&service));
        }

        IFC_RETURN(service->PrepareToAnimate(key, animationElement.Get(), ppReturnValue));
        return S_OK;
    }
    // We were unable to find the element.
    return E_INVALIDARG;
}

_Check_return_ HRESULT
ListViewBase::TryStartConnectedAnimationAsyncImpl(_In_ xaml_animation::IConnectedAnimation* pAnimation, _In_ IInspectable* pItem, _In_ HSTRING elementName, _Outptr_ wf::IAsyncOperation<bool>** ppReturnValue)
{
    Microsoft::WRL::ComPtr<TryStartConnectedAnimationOperation> tryStartOperation;
    IFC_RETURN(Microsoft::WRL::MakeAndInitialize<TryStartConnectedAnimationOperation>(&tryStartOperation));
    IFC_RETURN(tryStartOperation->InitAndStart(this, pAnimation, pItem, elementName));
    IFC_RETURN(tryStartOperation.CopyTo(ppReturnValue));

    return S_OK;
}

_Check_return_ HRESULT
ListViewBase::IsScrollable(_Out_ bool* scrollable)
{
    *scrollable = false;
    if (m_tpScrollViewer)
    {
        bool horizontallyScrollable = false;
        bool verticallyScrollable = false;
        IFC_RETURN(m_tpScrollViewer.Cast<ScrollViewer>()->IsContentScrollable(
            false, /* ignoreScrollMode */
            false, /* ignoreScrollBarVisibility */
            &horizontallyScrollable,
            &verticallyScrollable));

        *scrollable = horizontallyScrollable || verticallyScrollable;
    }

    return S_OK;
}
