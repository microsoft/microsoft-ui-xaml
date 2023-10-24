// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ListViewBase.g.h"
#include "ScrollViewer.g.h"
#include "ItemCollection.g.h"
#include "SemanticZoomLocation.g.h"
#include "GroupItem.g.h"
#include "ItemContainerGenerator.g.h"
#include "Panel.g.h"
#include "SemanticZoom.g.h"
#include "IItemLookupPanel.g.h"
#include "XamlBehaviorMode.h"
#include "ListViewBaseItem.g.h"
#include "VisualTreeHelper.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

// Prepare the view for a zoom transition.
_Check_return_ HRESULT ListViewBase::InitializeViewChangeImpl()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<ISemanticZoom> spSemanticZoomOwner;

    // block scrollbars from showing during sezo operation
    if (m_tpScrollViewer.Get())
    {
        IFC(m_tpScrollViewer.Cast<ScrollViewer>()->BlockIndicatorsFromShowing());
    }

    // need to precalculate the focusstate our item is going to be in
    // since the sezo will lose the trigger state soon
    IFC(get_SemanticZoomOwner(&spSemanticZoomOwner));

    if (spSemanticZoomOwner)
    {
        ctl::ComPtr<SemanticZoom> spSemanticZoomOwnerConcrete = spSemanticZoomOwner.Cast<SemanticZoom>();
        m_semanticZoomCompletedFocusState = xaml::FocusState_Programmatic;

        if (spSemanticZoomOwnerConcrete->GetIsProcessingKeyboardInput() ||
            spSemanticZoomOwnerConcrete->GetIsProcessingPointerInput())
        {
            // will set focus to the destination element using either keyboard or pointer focus depending on what
            // triggered the statechange.
            m_semanticZoomCompletedFocusState =
                spSemanticZoomOwnerConcrete->GetIsProcessingKeyboardInput() ?
                xaml::FocusState_Keyboard :
                xaml::FocusState_Pointer;
        }
    }

Cleanup:
    RRETURN(hr);
}

// Cleanup the view after a zoom transition.
_Check_return_ HRESULT ListViewBase::CompleteViewChangeImpl()
{
    HRESULT hr = S_OK;

    // unblock scrollbars from showing during sezo operation
    if (m_tpScrollViewer.Get())
    {
        IFC(m_tpScrollViewer.Cast<ScrollViewer>()->ResetBlockIndicatorsFromShowing());
    }

Cleanup:
    RRETURN(hr);
}

// Forces content to scroll until the coordinate space of the SemanticZoomLocation is
// visible.
_Check_return_ HRESULT ListViewBase::MakeVisibleImpl(
    _In_ ISemanticZoomLocation* pItem) noexcept
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spItem;
    ctl::ComPtr<ISemanticZoomLocation> spSemanticLocation = pItem;
    BOOLEAN isJumpListEnabled = FALSE;
    BOOLEAN isZoomedInView = FALSE;

    ctl::ComPtr<ISemanticZoom> spSemanticZoomOwner;
    IFC(get_SemanticZoomOwner(&spSemanticZoomOwner));
    if (spSemanticZoomOwner)
    {
        isJumpListEnabled = TRUE;

        // For threshold, the jumplist behavior of aligning the items in zoomed out view and zoomed in view
        // is the converged behavior
        IFC(get_IsZoomedInView(&isZoomedInView));
    }

    IFC(spSemanticLocation->get_Item(&spItem));
    if (spItem)
    {
        ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;

        // Get the index of the corresponding item
        IFC(get_Items(&spItems));

        if (m_tpScrollViewer)
        {
            BOOLEAN canScrollIntoView = FALSE;
            IFC(CanScrollIntoView(canScrollIntoView));
            if (canScrollIntoView)
            {
                ctl::ComPtr<xaml::IDependencyObject> spContainer;
                UINT groupItemIndex = 0;
                BOOLEAN foundGroup = FALSE;

                // this could be a group, let's try that first
                IFC(GetGroupItemIndex(spItem.Get(), &groupItemIndex, &foundGroup));

                // On the phone when exiting the JumpList, we want to force top alignment for the selected group
                xaml_controls::ScrollIntoViewAlignment alignment = xaml_controls::ScrollIntoViewAlignment::ScrollIntoViewAlignment_Default;
                if (isJumpListEnabled && isZoomedInView && foundGroup)
                {
                    alignment = xaml_controls::ScrollIntoViewAlignment::ScrollIntoViewAlignment_Leading;
                }

                // whether spItem is a group or an actual item, this should work
                IFC(ScrollIntoViewWithAlignment(spItem.Get(), alignment));
                IFC(UpdateLayout()); // need to flush layout to get the container into view

                // let's get the position relative to the root
                // this is how we communicated the location of the source itemcontainer

                if (foundGroup)
                {
                    IFC(HeaderFromIndexImpl(groupItemIndex, &spContainer));
                }
                else
                {
                    // fallback to expecting just a regular item
                    BOOLEAN found = FALSE;
                    UINT index = 0;

                    // Get the index of the corresponding item
                    IFC(spItems.Cast<ItemCollection>()->IndexOf(spItem.Get(), &index, &found));

                    if (found)
                    {
                        IFC(ContainerFromIndex(index, &spContainer));
                    }
                }

                if (spContainer)
                {
                    ctl::ComPtr<FrameworkElement> spContainerLVI;

                    spContainerLVI = spContainer.AsOrNull<IFrameworkElement>().Cast<FrameworkElement>();

                    if (spContainerLVI)
                    {
                        ctl::ComPtr<xaml_media::IGeneralTransform> spTransform;
                        FLOAT amountToScroll = 0;
                        DOUBLE width = 0;
                        DOUBLE height = 0;
                        DOUBLE viewportWidth = 0;
                        DOUBLE viewportHeight = 0;
                        wf::Point destinationContainerLocation = { };
                        wf::Rect destinationContainerNewLocation = { };
                        BOOLEAN shouldScrollHorizontally = TRUE;
                        BOOLEAN shouldScrollVertically = TRUE;
                        BOOLEAN readLocationFromISZL = TRUE;
                        DOUBLE scrollableAmount = 0;
                        ctl::ComPtr<SemanticZoomLocation> spSezoLocationConcrete;

                        // current destination location
                        IFC(spContainerLVI->TransformToVisual(this, &spTransform));
                        IFC(spTransform->TransformPoint(destinationContainerLocation, &destinationContainerLocation));

                        // If we are following jumplist behavior, we may want the item to be bottom aligned:
                        //   (a) if we are in a JumpList scenario
                        //   (b) and we are the ZoomedOut view
                        if (isJumpListEnabled && (!isZoomedInView))
                        {
                            destinationContainerNewLocation.X = destinationContainerLocation.X;
                            if (spSemanticLocation.Cast<SemanticZoomLocation>()->GetIsBottomAlignment())
                            {
                                DOUBLE lvwHeight = 0.0;
                                DOUBLE containerHeight = 0.0;

                                IFC(get_ActualHeight(&lvwHeight));
                                IFC(spContainerLVI->get_ActualHeight(&containerHeight));
                                destinationContainerNewLocation.Y = static_cast<FLOAT>(lvwHeight - containerHeight);
                            }
                            else
                            {
                                destinationContainerNewLocation.Y = 0;
                            }
                            readLocationFromISZL = FALSE;
                        }

                        if (readLocationFromISZL)
                        {
                            // the location where we really want this container to live
                            IFC(spSemanticLocation->get_Bounds(&destinationContainerNewLocation));
                        }

                        // need to take care of both horizontal and vertical based on the modes of our ScrollViewer
                        IFC(m_tpScrollViewer.Cast<ScrollViewer>()->get_ScrollableHeight(&scrollableAmount));
                        shouldScrollVertically = scrollableAmount > 0;
                        IFC(m_tpScrollViewer.Cast<ScrollViewer>()->get_ScrollableWidth(&scrollableAmount));
                        shouldScrollHorizontally = scrollableAmount > 0;

                        IFC(m_tpScrollViewer.Cast<ScrollViewer>()->ComputePixelViewportWidth(NULL, FALSE, &viewportWidth));
                        IFC(m_tpScrollViewer.Cast<ScrollViewer>()->ComputePixelViewportHeight(NULL, FALSE, &viewportHeight));

                        IFC(spContainerLVI->get_ActualWidth(&width));
                        IFC(spContainerLVI->get_ActualHeight(&height));

                        while (shouldScrollHorizontally || shouldScrollVertically)
                        {
                            DOUBLE originalOffset = 0.0f;
                            // offset we wish to be at:
                            amountToScroll = shouldScrollHorizontally ? destinationContainerNewLocation.X - destinationContainerLocation.X : destinationContainerNewLocation.Y - destinationContainerLocation.Y;

                            // we should not scroll further then the edge of our viewport
                            // the remainer will be done using a rendertransform on the view by the sezo

                            // limit to the left: a container can be placed flush against the left of the viewport
                            amountToScroll = (FLOAT) DoubleUtil::Max(amountToScroll, (shouldScrollHorizontally ? -1 * destinationContainerLocation.X : -1 * destinationContainerLocation.Y));

                            // limit to the right: a container can be placed against the right of the viewport,taking into account its width or height so it is fully visible.
                            amountToScroll = (FLOAT) DoubleUtil::Min(amountToScroll,
                                                                        (shouldScrollHorizontally ?
                                                                        viewportWidth - destinationContainerLocation.X - width :
                                                                        viewportHeight - destinationContainerLocation.Y - height));
                            if (shouldScrollHorizontally)
                            {
                                originalOffset = m_tpScrollViewer.Cast<ScrollViewer>()->GetPixelHorizontalOffset();
                            }
                            else
                            {
                                originalOffset = m_tpScrollViewer.Cast<ScrollViewer>()->GetPixelVerticalOffset();
                            }

                            // Note that ScrollByPixelDelta uses the 2nd argument for non-logical scrolling scenarios (say, with StackPanels), while
                            // it uses the 3d argument for logical scrolling scenarios.
                            // Both the 2nd and 3rd arguments need to be expressed in pixels.
                            IFC(m_tpScrollViewer.Cast<ScrollViewer>()->ScrollByPixelDelta(
                                shouldScrollHorizontally,
                                originalOffset - amountToScroll,
                                -1 * amountToScroll,
                                FALSE /*isDManipInput*/));

                            // setup for next iteration
                            if (shouldScrollHorizontally)
                            {
                                shouldScrollHorizontally = FALSE;
                            }
                            else
                            {
                                shouldScrollVertically = FALSE;
                            }
                        }   // end of the while loop that handles both horizontal and vertical scrolling

                        spSezoLocationConcrete = spSemanticLocation.Cast<SemanticZoomLocation>();
                        if (spSezoLocationConcrete)
                        {
                            // communicate where the destination container actually ended up
                            // this allows the SeZo to apply extra transforms
                            IFC(UpdateLayout()); // need to flush layout to get the container in the right position
                            IFC(spContainerLVI->TransformToVisual(this, &spTransform));
                            destinationContainerLocation.X = destinationContainerLocation.Y = 0;
                            // point of the container will be relative to this listview
                            IFC(spTransform->TransformPoint(destinationContainerLocation, &destinationContainerLocation));
                            destinationContainerNewLocation.X -= destinationContainerLocation.X;
                            destinationContainerNewLocation.Y -= destinationContainerLocation.Y;
                            destinationContainerNewLocation.Width = static_cast<FLOAT>(width);
                            destinationContainerNewLocation.Height = static_cast<FLOAT>(height);
                            // put the delta in the remainder field - this is the remainder that we were unable to scroll
                            // we are currently in lockdown phase, so no new api is allowed.
                            // ofcourse, we would like to make this public and put the remainder field in the IDL or
                            // have some better mechanism to pass this value.
                            IFC(spSezoLocationConcrete->put_Remainder(destinationContainerNewLocation));
                        }
                    }
                }
            }
        }
        else
        {
            m_deferredAlignment = (spSemanticLocation.Cast<SemanticZoomLocation>()->GetIsBottomAlignment()
                                   ? xaml_controls::ScrollIntoViewAlignment_Default
                                   : xaml_controls::ScrollIntoViewAlignment_Leading);
            ScheduleDeferredScrollCommand([this](IInspectable* scrollToItem) -> HRESULT
            {
                HRESULT hr = S_OK;

                if (scrollToItem && m_tpScrollViewer)
                {
                    ctl::ComPtr<IPanel> spPanel;
                    IFC(get_ItemsHost(&spPanel));
                    if (spPanel)
                    {
                        IFC(ScrollIntoViewWithAlignment(scrollToItem, m_deferredAlignment));
                    }
                }

            Cleanup:
                RRETURN(hr);
            },
            spItem.Get(),
            DeferPoint::ItemsHostAvailable);
        }
    }

Cleanup:
    RRETURN(hr);
}

// When this ListViewBase is the active view and we're changing to the other
// view, optionally provide the source and destination items.
//
// Two modes:
//  1. user has tapped an item and this listview has decided to toggle a semantic zoom based on that
//  2. user has used a DM gesture such as pinch or zoomout and SemanticZoom has decided to perform a semantic zoom
//
//  in the first case we can use the focused element
//  in the second case we need to find out what the closest element is to the center point
_Check_return_ HRESULT ListViewBase::StartViewChangeFromImpl(
    _In_ ISemanticZoomLocation* pSource,
    _In_ ISemanticZoomLocation* pDestination) noexcept
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spItem;
    BOOLEAN isGrouping = FALSE;
    INT sourceIndex = -1;
    // the container that represents our source.
    ctl::ComPtr<xaml::IDependencyObject> spContainer;

    if (pSource)
    {
        IFC(pSource->get_Item(&spItem));
        if (!spItem)
        {
            wf::Point zoomPoint = { };
            BOOLEAN validZoomPoint = FALSE;
            BOOLEAN jumpListAlignmentBehavior = FALSE;

            IFC(static_cast<SemanticZoomLocation*>(pSource)->get_ZoomPoint(&zoomPoint));

            validZoomPoint = zoomPoint.X != 0 || zoomPoint.Y != 0;

            // only use zoompoint if there was a gesture that got us to a zoompoint
            if (validZoomPoint)
            {
                // scenario 2: the user has zoomed with DM or used ctrl-mousewheel and that is why we are doing this viewchange
                ctl::ComPtr<IPanel> spItemsHost;
                ctl::ComPtr<IItemLookupPanel> spItemsHostAsItemLookupPanel;
                IFC(get_ItemsHost(&spItemsHost));
                IFCCHECK(spItemsHost);
                spItemsHostAsItemLookupPanel = spItemsHost.AsOrNull<IItemLookupPanel>();

                wf::Point zoomPointToFirstPanel = zoomPoint;
                ctl::ComPtr<xaml_media::IGeneralTransform> spTransformFromLVBtoPanel;

                IFC(TransformToVisual(static_cast<UIElement*>(spItemsHost.Cast<Panel>()), &spTransformFromLVBtoPanel));
                IFC(spTransformFromLVBtoPanel->TransformPoint(zoomPoint, &zoomPointToFirstPanel));

                if (spItemsHostAsItemLookupPanel)
                {
                    BOOLEAN isViewGrouping = FALSE;
                    xaml_primitives::ElementInfo elementInfo = {-1, FALSE};

                    IFC(spItemsHostAsItemLookupPanel->GetClosestElementInfo(zoomPointToFirstPanel, &elementInfo));
                    sourceIndex = elementInfo.m_childIndex;

                    IFC(get_IsGrouping(&isViewGrouping));
                    if (elementInfo.m_childIsHeader)
                    {
                        ASSERT(isViewGrouping);

                        // fallback to the group. The sourceindex still points to the view property
                        // instead of an actual item.

                        ctl::ComPtr<ICollectionView> spCollectionView;
                        IFC(get_CollectionView(&spCollectionView));
                        if (spCollectionView)
                        {
                            ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spGroups;
                            IFC(spCollectionView->get_CollectionGroups(&spGroups));
                            if (spGroups)
                            {
                                ctl::ComPtr<wfc::IVector<IInspectable*>> spGroupsAsVector;
                                IFC(spGroups.As<wfc::IVector<IInspectable*>>(&spGroupsAsVector));
                                IFC(spGroupsAsVector->GetAt(sourceIndex, &spItem));
                            }
                        }
                    }

                    if (isViewGrouping)
                    {
                        if (!ctl::is<IModernCollectionBasePanel>(spItemsHost))
                        {
                            // so we now have an index that points to a group. That is not what we need, we want
                            // to figure out which item was zoomed at.
                            // we will have to look at this group, find the itemscontrol over there, find the panel
                            // and then get a better index.
                            // NOTE: this will change as we get a different grouping panel
                            ctl::ComPtr<IGroupItem> spGroupItem;

                            IFC(HeaderFromIndexImpl(sourceIndex, &spContainer));
                            if (spContainer)
                            {
                                spGroupItem = spContainer.AsOrNull<IGroupItem>();
                            }

                            if (spGroupItem)
                            {
                                ctl::ComPtr<IItemsControl> spItemsControl;
                                xaml_primitives::ElementInfo innerElementInfo = {-1, FALSE};
                                IFC(spGroupItem.Cast<GroupItem>()->GetTemplatedItemsControl(&spItemsControl));
                                if (spItemsControl)
                                {
                                    IFC(spItemsControl.Cast<ItemsControl>()->get_ItemsHost(&spItemsHost));

                                    if (spItemsHost)
                                    {
                                        // we found a panel. This panel might be arbitrarily positioned inside of our moco.
                                        // ZoomPoint is relative to the moco, it would be good to transform that point to
                                        // the panels coordinate space.
                                        wf::Point zoomPointToNestedPanel = zoomPoint;

                                        IFC(TransformToVisual(static_cast<UIElement*>(spItemsHost.Cast<Panel>()), &spTransformFromLVBtoPanel));
                                        IFC(spTransformFromLVBtoPanel->TransformPoint(zoomPoint, &zoomPointToNestedPanel));

                                        // try to use the optimized IITemLookup version
                                        spItemsHostAsItemLookupPanel = spItemsHost.AsOrNull<IItemLookupPanel>();
                                        if (spItemsHostAsItemLookupPanel)
                                        {
                                            IFC(spItemsHostAsItemLookupPanel->GetClosestElementInfo(zoomPointToNestedPanel, &innerElementInfo));
                                        }
                                        else
                                        {
                                            // fallback to a naive and slow (full iteration) implementation of all the
                                            // arranged children
                                            XPOINTF p = {zoomPointToNestedPanel.X, zoomPointToNestedPanel.Y};
                                            IFC(CoreImports::CPanel_PanelGetClosestIndexSlow(static_cast<CPanel*>(spItemsHost.Cast<Panel>()->GetHandle()), p, &innerElementInfo.m_childIndex));
                                        }

                                        // nice, now we have a sourceindex that belongs to another itemscontrol
                                        // what we need is an index into our original itemscollection.
                                        // Grouping does not support virtualization.
                                        ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spObservableItems;
                                        ctl::ComPtr<wfc::IVector<IInspectable*>> spInnerItems;
                                        BOOLEAN found = FALSE;
                                        IFC(spItemsControl->get_Items(&spObservableItems));
                                        spInnerItems = spObservableItems.AsOrNull<wfc::IVector<IInspectable*>>();

                                        if (spInnerItems)
                                        {
                                            UINT innerItemsCount = 0;
                                            UINT i = 0; // will keep either sourceindex or innersourceindex
                                            IFC(spInnerItems->get_Size(&innerItemsCount));

                                            // correct the sourceindex to point to the view
                                            // we are mixing up our UINT and INT's unfortunately. The IItemLookupPanel uses INT's
                                            // but collections use UINT. To be honest, we cannot support an INT amount of children
                                            // so i'm not afraid here.
                                            if (innerElementInfo.m_childIndex >= 0 && innerElementInfo.m_childIndex < (INT)innerItemsCount)
                                            {
                                                ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;
                                                IFC(get_Items(&spItems));

                                                // use inner information
                                                IFC(spInnerItems->GetAt(innerElementInfo.m_childIndex, &spItem));
                                                IFC(spItems.Cast<ItemCollection>()->IndexOf(spItem.Get(), &i, &found));
                                                // assign the actual sourceIndex as it is within the outer collection
                                                sourceIndex = (UINT)i;
                                                // clear out the container since we should try and find a new container
                                                spContainer = nullptr;
                                            }
                                            else
                                            {
                                                // fallback to the group. The sourceindex still points to the view property
                                                // instead of an actual item.
                                                ctl::ComPtr<xaml_controls::IItemContainerGenerator> spGenerator;
                                                ctl::ComPtr<wfc::IVector<IInspectable*>> spView;

                                                IFC(get_ItemContainerGenerator(&spGenerator));
                                                IFC(spGenerator.Cast<ItemContainerGenerator>()->get_View(&spView));

                                                if (spView)
                                                {
                                                    IFC(spView->GetAt(sourceIndex, &spItem));
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        else
                        {
                            if (elementInfo.m_childIsHeader)
                            {
                                IFC(HeaderFromIndexImpl(sourceIndex, &spContainer));
                            }
                            else
                            {
                                IFC(ContainerFromIndexImpl(sourceIndex, &spContainer));
                            }
                        }
                    }
                }
                else
                {
                    // fallback to a naieve and slow (full iteration) implementation of all the
                    // arranged children
                    XPOINTF p = {zoomPoint.X, zoomPoint.Y};
                    IFC(CoreImports::CPanel_PanelGetClosestIndexSlow(static_cast<CPanel*>(spItemsHost.Cast<Panel>()->GetHandle()), p, &sourceIndex));
                }
            }
            else
            {
                BOOLEAN isZoomedInView = FALSE;

                // Let's check if we are in a jump list situation
                ctl::ComPtr<ISemanticZoom> spSemanticZoomOwner;
                IFC(get_SemanticZoomOwner(&spSemanticZoomOwner));
                if (spSemanticZoomOwner)
                {
                    jumpListAlignmentBehavior = TRUE;

                    // SemanticZoom checks other conditions than the apiset such as type of ZI/ZO views,
                    // popup in template, etc
                    IFC(get_IsZoomedInView(&isZoomedInView));
                    if (!isZoomedInView)
                    {
                        // We are the Zoomed Out View
                        if (spSemanticZoomOwner.Cast<SemanticZoom>()->GetIsCancellingJumpList())
                        {
                            // Cancel: don't set a destination item
                            goto Cleanup;
                        }
                    }
                }

                if (jumpListAlignmentBehavior && isZoomedInView)
                {
                    // We are the ZoomedInView
                    UINT startIndex = 0;
                    BOOLEAN bottomAlignment = FALSE;
                    if (m_tpSZRequestingItem)
                    {
                        // If a group has requested the change, let's take it as reference
                        INT groupIndex = -1;
                        ctl::ComPtr<IDependencyObject> spDO;
                        IFC(m_tpSZRequestingItem.As(&spDO));
                        IFC(IndexFromHeaderImpl(spDO.Get(), FALSE /*excludeHiddenEmptyGroups*/, &groupIndex));
                        if (groupIndex >= 0)
                        {
                            startIndex = static_cast<INT>(groupIndex);
                        }
                        bottomAlignment = TRUE;
                    }
                    IFC(FindFirstItem(startIndex, &sourceIndex, &spItem));
                    if (pDestination)
                    {
                        ctl::ComPtr<ISemanticZoomLocation> spDestination(pDestination);
                        spDestination.Cast<SemanticZoomLocation>()->SetIsBottomAlignment(bottomAlignment);
                    }
                }
                else
                {
                    // scenario 1: the user has selected an item and that is why we are doing this viewchange

                    // this is only valid if we are in the zoomedout view (which will never have grouping)
                    sourceIndex = (m_iFocusedIndex >= 0) ?
                           m_iFocusedIndex :
                           static_cast<INT>(m_lastFocusedIndex);
                }
            }


            // some of the specialized codepaths have advised on an spitem already.
            if (!spItem)
            {
                ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;
                UINT itemsCount = 0;

                IFC(get_Items(&spItems));
                IFC(spItems.Cast<ItemCollection>()->get_Size(&itemsCount));

                // the lastFocusedIndex is an INT, not UINT, so -1 does not come into play
                // which is why we make absolutely sure this is a valid index we are going to.
                if (sourceIndex >= 0 && sourceIndex < static_cast<INT>(itemsCount))
                {
                    IFC(spItems.Cast<ItemCollection>()->GetAt(sourceIndex, &spItem));
                }
            }

            // hopefully we found an item somehow. Fill the szl with it.
            if (spItem)
            {
                // Set the focused item as the SourceItem
                IFC(pSource->put_Item(spItem.Get()));
            }

            // set the destination bounding box
            // if we had an item, this is the items container. if we are a group, this
            // is the groups container
            {
                ctl::ComPtr<xaml_media::IGeneralTransform> spTransform;

                // some of the specialized codepaths(grouping) have advised on a container already
                if (!spContainer)
                {
                    IFC(ContainerFromIndex(sourceIndex, &spContainer));
                }


                // container surely can be null, because we're not guaranteed to get back an item
                // that was in view.
                // if the container is null, there is not much we can do.
                // If we are doing jumpList behavior then dont adjust co-ordinate systems.
                if (spContainer && !jumpListAlignmentBehavior)
                {
                    ctl::ComPtr<FrameworkElement> spContainerFE;
                    wf::Point relativeToThis = { };
                    wf::Rect boundingBox = { };
                    DOUBLE dActualWidth = 0.0;
                    DOUBLE dActualHeight = 0.0;

                    spContainerFE = spContainer.AsOrNull<IFrameworkElement>().Cast<FrameworkElement>();

                    IFC(spContainerFE->get_ActualWidth(&dActualWidth));
                    IFC(spContainerFE->get_ActualHeight(&dActualHeight));

                    IFC(spContainerFE->TransformToVisual(this, &spTransform));
                    IFC(spTransform->TransformPoint(relativeToThis, &relativeToThis));

                    boundingBox.Width = static_cast<FLOAT>(dActualWidth);
                    boundingBox.Height = static_cast<FLOAT>(dActualHeight);
                    boundingBox.X = relativeToThis.X;
                    boundingBox.Y = relativeToThis.Y;
                    IFC(pSource->put_Bounds(boundingBox));
                }
                else
                {
                    wf::Rect boundingBox = { };
                    boundingBox.X = zoomPoint.X;
                    boundingBox.Y = zoomPoint.Y;
                    IFC(pSource->put_Bounds(boundingBox));
                }
            }
        }
    }

    // If our ItemsSource is grouped, automatically provide a destination of the
    // group's name (so that if the next level's items are just the names, then
    // we won't need to write any code to handle the mapping)
    IFC(get_CanSemanticZoomGroup(&isGrouping));
    if (isGrouping)
    {
        ctl::ComPtr<IInspectable> spDestination;

        // Create a SemanticZoomLocation if we don't have one yet
        if (pDestination)
        {
            // If there's no DestinationItem currently set, then will use its group
            // name (or the first item in the group if heading the other direction)
            IFC(pDestination->get_Item(&spDestination));
            if (!spDestination && spItem)
            {
                BOOLEAN isZoomedInView = FALSE;

                // If we're changing from the ZoomedInView to the ZoomedOutView then we
                // need to look up a group from an item, but otherwise we'll just
                // get the first item in the group
                IFC(get_IsZoomedInView(&isZoomedInView));
                if (isZoomedInView)
                {
                    if (sourceIndex >= 0)
                    {
                        ctl::ComPtr<ICollectionViewGroup> spDestinationGroup;

                        // in the normal case, spitem is set to an item, and we are going to find a group out of that now
                        // however, in the case of an empty group, that is not true.
                        // That codepath has already adviced on a group
                        spDestinationGroup = spItem.AsOrNull<ICollectionViewGroup>();
                        if (!spDestinationGroup)
                        {
                            // have to dig a little bit deeper
                            IFC(GetGroupFromItem(sourceIndex, &spDestinationGroup, NULL /*ppGroupItem*/, NULL /*pGroupIndex*/, NULL /*pIndexInGroup*/ ));
                        }

                        // use the group to set the destination item
                        if (spDestinationGroup)
                        {
                            spDestination = spDestinationGroup.AsOrNull<IInspectable>();
                        }
                    }
                }
                else
                {
                    // currently in zoomedoutview, going to zoomedinview

                    // Get the first item in the group as the destination
                    ctl::ComPtr<ICollectionViewGroup> spDestinationGroup;
                    spDestinationGroup = spItem.AsOrNull<ICollectionViewGroup>();

                    // if that did not succeed (empty group) or if we are using sticky headers, fallback to the group
                    if (!spDestination && spDestinationGroup)
                    {
                        IFC(spDestinationGroup->get_Group(spDestination.ReleaseAndGetAddressOf()));
                    }
                }


                // If we've found the corresponding destination item
                if (spDestination)
                {
                    // Set the group key as the DestinationItem
                    IFC(pDestination->put_Item(spDestination.Get()));
                }
            }
        }
    }

Cleanup:
    m_tpSZRequestingItem.Clear();
    RRETURN(hr);
}

// Determines whether or not a SemanticZoom associated with this ListViewBase has
// grouped data as its ZoomedInView.
_Check_return_ HRESULT ListViewBase::get_CanSemanticZoomGroup(
    _Out_ BOOLEAN* pCanGroup)
{
    HRESULT hr = S_OK;
    BOOLEAN isZoomedInView = FALSE;

    IFCPTR(pCanGroup);
    *pCanGroup = FALSE;

    IFC(get_IsZoomedInView(&isZoomedInView));
    if (isZoomedInView)
    {
        // If we are the ZoomedInView, then we just need to check if we support
        // grouping
        IFC(get_IsGrouping(pCanGroup));
    }
    else
    {
        // Otherwise, get the ZoomedInView and see if it supports grouping
        ctl::ComPtr<ISemanticZoom> spSemanticZoomOwner;
        ctl::ComPtr<ISemanticZoomInformation> spJumpContent;
        ctl::ComPtr<IItemsControl> spItems;

        IFC(get_SemanticZoomOwner(&spSemanticZoomOwner));
        IFC(spSemanticZoomOwner->get_ZoomedInView(&spJumpContent));
        if (spJumpContent)
        {
            spItems = spJumpContent.AsOrNull<IItemsControl>();
            if (spItems)
            {
                IFC(spItems->get_IsGrouping(pCanGroup));
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

// Get the ICollectionViewGroup associated with an item.
_Check_return_ HRESULT ListViewBase::GetGroupFromItem(
    _In_ UINT itemIndex,
    _Outptr_ ICollectionViewGroup** ppGroup,
    _Outptr_result_maybenull_ IGroupItem** ppGroupItem,
    _Out_opt_ INT* pGroupIndex,
    _Out_opt_ INT* pIndexInGroup)
{
    HRESULT hr = S_OK;

    *ppGroup = NULL;

    if (pIndexInGroup)
    {
        *pIndexInGroup = -1;
    }

    if (ppGroupItem)
    {
        *ppGroupItem = nullptr;
    }

    if (pGroupIndex)
    {
        *pGroupIndex = -1;
    }

    // First try to find the corresponding group by taking a short cut through
    // the visual tree. Skip this if we only need to find the index in the group.
    // This implementation cannot find the index in the group, so we must defer
    // to the other implementation below in that case.
    if (!pIndexInGroup || ppGroupItem)
    {
        ctl::ComPtr<IDependencyObject> spContainer;
        ctl::ComPtr<IGroupItem> spGroupItem;

        // Get the current item's container
        IFC(ContainerFromIndex(itemIndex, &spContainer));

        // Walk up the visual tree looking for a GroupItem
        while (spContainer)
        {
            ctl::ComPtr<IDependencyObject> spParent;

            // Keep walking until we reach a GroupItem or we hit ourself
            spGroupItem = spContainer.AsOrNull<IGroupItem>();
            if (spGroupItem || spContainer.Get() == this)
            {
                if (ppGroupItem)
                {
                    // We found the item.
                    IFC(spGroupItem.CopyTo(ppGroupItem));
                }
                break;
            }

            IFC(VisualTreeHelper::GetParentStatic(spContainer.Get(), &spParent));
            spContainer = spParent;
        }

        // Use the GroupItem's Content if we were able to find it
        if (spGroupItem)
        {
            ctl::ComPtr<ICollectionViewGroup> spContentGroup;
            GroupItem *pGroupItem = static_cast<GroupItem *>(spGroupItem.Get());

            IFC(pGroupItem->GetCollectionViewGroup(&spContentGroup));
            if (spContentGroup)
            {
                IFC(spContentGroup.CopyTo(ppGroup));
            }
        }
    }

    // If we can't find the group using the visual tree (i.e., it hasn't been
    // realized yet), or we need to get the item's index within the group,
    // we'll just walk each of the groups checking their count to
    // see where the given item index belongs.
    if (!*ppGroup || pIndexInGroup)
    {
        ctl::ComPtr<ICollectionView> spCollectionView;
        ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spCollectionGroups;
        ctl::ComPtr<wfc::IVector<IInspectable*>> spCollectionGroupsAsV;
        UINT i = 0;
        UINT size = 0;
        UINT total = 0;

        IFC(get_CollectionView(&spCollectionView));
        IFC(spCollectionView->get_CollectionGroups(&spCollectionGroups));

        // Walk through each of the groups to determine if it
        // contains the current item in which case we'll use it as
        // the destination
        IFC(spCollectionGroups.As<wfc::IVector<IInspectable*>>(&spCollectionGroupsAsV));
        IFC(spCollectionGroupsAsV->get_Size(&size));
        for (i = 0; i < size; i++)
        {
            ctl::ComPtr<IInspectable> spCurrent;
            ctl::ComPtr<ICollectionViewGroup> spCurrentGroup;
            ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spCurrentGroupItems;
            ctl::ComPtr<wfc::IVector<IInspectable*>> spCurrentGroupItemsAsV;
            UINT groupSize = 0;

            IFC(spCollectionGroupsAsV->GetAt(i, &spCurrent));
            IFC(spCurrent.As<ICollectionViewGroup>(&spCurrentGroup));
            IFC(spCurrentGroup->get_GroupItems(&spCurrentGroupItems));
            IFC(spCurrentGroupItems.As<wfc::IVector<IInspectable*>>(&spCurrentGroupItemsAsV));
            IFC(spCurrentGroupItemsAsV->get_Size(&groupSize));

            total += groupSize;
            if (itemIndex < total)
            {
                if (*ppGroup == NULL)
                {
                    IFC(spCurrentGroup.CopyTo(ppGroup));
                }
                if (pIndexInGroup)
                {
                    *pIndexInGroup = itemIndex - (total - groupSize);
                }
                if (pGroupIndex)
                {
                    *pGroupIndex = i;
                }
                break;
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

// When this ListViewBase is the inactive view and we're changing to it,
// optionally provide the source and destination items.
_Check_return_ HRESULT ListViewBase::StartViewChangeToImpl(
    _In_ ISemanticZoomLocation* source,
    _In_ ISemanticZoomLocation* destination)
{
    m_disableScrollingPlaceholders = TRUE;

    RRETURN(S_OK);
}

// Complete the change to the other view when this ListViewBase was
// the active view.
_Check_return_ HRESULT ListViewBase::CompleteViewChangeFromImpl(
    _In_ ISemanticZoomLocation* source,
    _In_ ISemanticZoomLocation* destination)
{
    return S_OK;
}

// Complete the change to make this ListViewBase the active view.
_Check_return_ HRESULT ListViewBase::CompleteViewChangeToImpl(
    _In_ ISemanticZoomLocation* source,
    _In_ ISemanticZoomLocation* destination)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spItem;
    BOOLEAN hasFocus = FALSE;
    ctl::ComPtr<ISemanticZoom> spSemanticZoomOwner;
    BOOLEAN shouldTakeFocus = TRUE;

    IFC(get_SemanticZoomOwner(&spSemanticZoomOwner));
    if (spSemanticZoomOwner)
    {
        IFC(spSemanticZoomOwner.Cast<SemanticZoom>()->HasFocus(&shouldTakeFocus));
    }

    // We only want to focus the destination item (or any item in the destination view) if and
    // only if the semantic zoom has focus at this time.
    if (shouldTakeFocus)
    {
        IFC(destination->get_Item(&spItem));
        if (spItem)
        {
            ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;

            BOOLEAN found = FALSE;
            UINT index = 0;

            // Get the index of the corresponding item
            IFC(get_Items(&spItems));
            IFC(spItems.Cast<ItemCollection>()->IndexOf(spItem.Get(), &index, &found));

            if (found)
            {
                IFC(SetFocusedItem(index,
                                   /*shouldScrollIntoView*/ FALSE,
                                   /*forceFocus*/ TRUE,
                                   m_semanticZoomCompletedFocusState));
            }
            else
            {
                BOOLEAN isGrouping = FALSE;

                IFC(get_IsGrouping(&isGrouping));
                if (isGrouping)
                {
                    BOOLEAN foundGroup = FALSE;
                    UINT groupIndex = 0;

                    IFC(GetGroupItemIndex(spItem.Get(), &groupIndex, &foundGroup));
                    if (foundGroup)
                    {
                        // Our destination is a group.
                        INT groupCount = 0;
                        INT delta = 0;
                        INT firstSelectableElementIndex = -1;

                        IFC(GetGroupCount(&groupCount));
                        while (((INT)groupIndex  + delta) < groupCount && !found)
                        {
                            IFC(FindFirstSelectableItemInGroup(groupIndex + (UINT)delta, TRUE /* from beginning*/,
                                                           -1/* start index */, &firstSelectableElementIndex, &found));
                            delta++;
                        }

                        if (!found)
                        {
                            // Search the previous groups
                            delta = -1;
                            while (((INT)groupIndex + delta) >= 0 && !found)
                            {
                                IFC(FindFirstSelectableItemInGroup(groupIndex + (UINT)delta, TRUE /* from beginning*/,
                                                               -1/* start index */, &firstSelectableElementIndex, &found));
                                delta--;
                            }
                        }

                        if (found)
                        {
                            IFC(SetFocusedItem(firstSelectableElementIndex,
                                               /*shouldScrollIntoView*/ FALSE,
                                               /*forceFocus*/ TRUE,
                                               m_semanticZoomCompletedFocusState));
                        }
                    }
                }
            }
        }

        IFC(HasFocus(&hasFocus));
        if (!hasFocus)
        {
            IFC(SetFocusedItem(m_lastFocusedIndex,
                               /*shouldScrollIntoView*/ TRUE,
                               /*forceFocus*/ TRUE,
                               m_semanticZoomCompletedFocusState));

            IFC(HasFocus(&hasFocus));
            if (!hasFocus)
            {
                // If still we could not focus something that makes sense, focus the  destination view itself
                // for the sake of not leaving the focus on the source view.
                IFC(Focus(m_semanticZoomCompletedFocusState, &hasFocus));
            }
        }
    }

Cleanup:
    m_disableScrollingPlaceholders = FALSE;
    RRETURN(hr);
}

// Find the first non emptyGoup of the underlying collection
_Check_return_ HRESULT ListViewBase::FindFirstItem(
    _In_ UINT targetGroupIndex,
    _Out_ INT* pItemIndex,
    _Outptr_result_maybenull_ IInspectable** ppItem)
{
    HRESULT hr = S_OK;

    *pItemIndex = -1;
    *ppItem = nullptr;

    ctl::ComPtr<ICollectionView> spCollectionView;
    IFC(get_CollectionView(&spCollectionView));
    if (spCollectionView)
    {
        ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spGroups;
        IFC(spCollectionView->get_CollectionGroups(&spGroups));
        if (spGroups)
        {
            UINT itemIndex = 0;
            UINT size = 0;
            ctl::ComPtr<wfc::IVector<IInspectable*>> spGroupsAsVector;
            IFC(spGroups.As<wfc::IVector<IInspectable*>>(&spGroupsAsVector));
            IFC(spGroupsAsVector->get_Size(&size));
            for (UINT groupIndex = 0; groupIndex < size; ++groupIndex)
            {
                ctl::ComPtr<IInspectable> spCurrent;
                ctl::ComPtr<ICollectionViewGroup> spCurrentGroup;
                ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spCurrentGroupItems;
                ctl::ComPtr<wfc::IVector<IInspectable*>> spCurrentGroupItemsAsV;
                UINT groupSize = 0;

                IFC(spGroupsAsVector->GetAt(groupIndex, &spCurrent));
                IFC(spCurrent.As<ICollectionViewGroup>(&spCurrentGroup));
                IFC(spCurrentGroup->get_GroupItems(&spCurrentGroupItems));
                IFC(spCurrentGroupItems.As<wfc::IVector<IInspectable*>>(&spCurrentGroupItemsAsV));
                IFC(spCurrentGroupItemsAsV->get_Size(&groupSize));

                if (groupSize > 0)
                {
                    if (groupIndex >= targetGroupIndex)
                    {
                        // We stop at the first non empty group after (including) targetGroupIndex
                        IFC(spCurrentGroupItemsAsV->GetAt(0, ppItem));
                        *pItemIndex = itemIndex;
                        break;
                    }
                    else
                    {
                        itemIndex += groupSize;
                    }
                }
            }
        }
    }

Cleanup:
    RRETURN(hr);
}
