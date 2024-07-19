﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "ItemsRepeater.common.h"
#include "ViewManager.h"
#include "ItemsRepeater.h"
#include "RepeaterTestHooks.h"
#include <FrameworkUdk/Containment.h>

// Bug 50344748: [1.5 Servicing][WASDK] 1-up viewer opens behind Collections (looks like nothing's happened, but the viewer is actually hidden behind the Collections window)
#define WINAPPSDK_CHANGEID_50344748 50344748

ViewManager::ViewManager(ItemsRepeater* owner) :
    m_owner(owner),
    m_resetPool(owner),
    m_lastFocusedElement(owner),
    m_phaser(owner),
    m_ElementFactoryGetArgs(owner),
    m_ElementFactoryRecycleArgs(owner)
{
    // ItemsRepeater is not fully constructed yet. Don't interact with it.
}

winrt::UIElement ViewManager::GetElement(int index, bool forceCreate, bool suppressAutoRecycle)
{
    bool elementIsAnchor = false;
    winrt::UIElement element = forceCreate ? nullptr : GetElementIfAlreadyHeldByLayout(index);

    if (!element)
    {
        // check if this is the anchor made through repeater in preparation 
        // for a bring into view.
        if (auto madeAnchor = m_owner->MadeAnchor())
        {
            auto anchorVirtInfo = ItemsRepeater::TryGetVirtualizationInfo(madeAnchor);
            if (anchorVirtInfo->Index() == index)
            {
                element = madeAnchor;
                elementIsAnchor = true;

#ifdef DBG
                if (VirtualizationInfo::GetLogItemIndex() == index)
                {
                    ITEMSREPEATER_TRACE_INFO(nullptr, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"Returning anchor element.", VirtualizationInfo::GetLogItemIndex());
                }
#endif // DBG
            }
        }
    }

    if (!element) { element = GetElementFromUniqueIdResetPool(index); };

    if (!element || elementIsAnchor)
    {
        // When elementIsAnchor is True and 'element' is already set, it still needs to be removed from
        // the pinned pool if it happens to be in there, for example because it has keyboard focus.
        winrt::UIElement elementFromPool = GetElementFromPinnedElements(index);
        MUX_ASSERT(!elementFromPool || !element || elementFromPool == element);

#ifdef DBG
        if (elementIsAnchor && elementFromPool && VirtualizationInfo::GetLogItemIndex() == index)
        {
            ITEMSREPEATER_TRACE_INFO(nullptr, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"Anchor element removed from pinned pool.", VirtualizationInfo::GetLogItemIndex());
        }
#endif // DBG

        if (!element && elementFromPool)
        {
            element = elementFromPool;
        }
    }

    if (!element) { element = GetElementFromElementFactory(index); }

    auto virtInfo = ItemsRepeater::TryGetVirtualizationInfo(element);
    if (suppressAutoRecycle)
    {
        virtInfo->AutoRecycleCandidate(false);
        ITEMSREPEATER_TRACE_INFO_DBG(nullptr, TRACE_MSG_METH_IND_STR_STR_INT, METH_NAME, this, m_owner->Indent(), L"Not AutoRecycleCandidate", L"virtInfo Index:", virtInfo->Index());
    }
    else
    {
        virtInfo->AutoRecycleCandidate(true);
        virtInfo->KeepAlive(true);
        ITEMSREPEATER_TRACE_INFO_DBG(nullptr, TRACE_MSG_METH_IND_STR_STR_INT, METH_NAME, this, m_owner->Indent(), L"AutoRecycleCandidate", L"virtInfo Index:", virtInfo->Index());
    }

#ifdef DBG
    if (VirtualizationInfo::GetLogItemIndex() == index)
    {
        ITEMSREPEATER_TRACE_INFO(nullptr, TRACE_MSG_METH_PTR_INT, METH_NAME, this, element, VirtualizationInfo::GetLogItemIndex());
    }
#endif // DBG

    MUX_ASSERT(virtInfo->Owner() == ElementOwner::Layout);

    return element;
}

void ViewManager::ClearElement(const winrt::UIElement& element, bool isClearedDueToCollectionChange)
{
    const auto virtInfo = ItemsRepeater::GetVirtualizationInfo(element);
    const int index = virtInfo->Index();
    const bool cleared =
        ClearElementToUniqueIdResetPool(element, virtInfo) ||
        ClearElementToAnimator(element, virtInfo) ||
        ClearElementToPinnedPool(element, virtInfo, isClearedDueToCollectionChange);

    if (!cleared)
    {
        ClearElementToElementFactory(element);
    }

    // Both First and Last indices need to be valid or default.
    MUX_ASSERT((m_firstRealizedElementIndexHeldByLayout == FirstRealizedElementIndexDefault && m_lastRealizedElementIndexHeldByLayout == LastRealizedElementIndexDefault) ||
        (m_firstRealizedElementIndexHeldByLayout != FirstRealizedElementIndexDefault && m_lastRealizedElementIndexHeldByLayout != LastRealizedElementIndexDefault));

    if (index == m_firstRealizedElementIndexHeldByLayout && index == m_lastRealizedElementIndexHeldByLayout)
    {
        // First and last were pointing to the same element and that is going away.
        InvalidateRealizedIndicesHeldByLayout();
    }
    else if (index == m_firstRealizedElementIndexHeldByLayout)
    {
        // The FirstElement is going away, shrink the range by one.
        ++m_firstRealizedElementIndexHeldByLayout;
    }
    else if (index == m_lastRealizedElementIndexHeldByLayout)
    {
        // Last element is going away, shrink the range by one at the end.
        --m_lastRealizedElementIndexHeldByLayout;
    }
    else
    {
        // Index is either outside the range we are keeping track of or inside the range.
        // In both these cases, we just keep the range we have. If this clear was due to 
        // a collection change, then in the CollectionChanged event, we will invalidate these guys.
    }
}

// We need to clear the datacontext to prevent crashes from happening,
//  however we only do that if we were the ones setting it.
// That is when one of the following is the case (numbering taken from line ~642):
// 1.2    No ItemTemplate, data is not a UIElement
// 2.1    ItemTemplate, data is not FrameworkElement
// 2.2.2  Itemtemplate, data is FrameworkElement, ElementFactory returned Element different to data
//
// In all of those three cases, we the ItemTemplateShim is NOT null.
// Luckily when we create the items, we store whether we were the once setting the DataContext.
void ViewManager::ClearElementToElementFactory(const winrt::UIElement& element)
{
    m_owner->OnElementClearing(element);

    auto virtInfo = ItemsRepeater::GetVirtualizationInfo(element);
    virtInfo->MoveOwnershipToElementFactory();

    // During creation of this object, we were the one setting the DataContext, so clear it now.
    if (virtInfo->MustClearDataContext())
    {
        if (const auto elementAsFE = element.try_as<winrt::FrameworkElement>())
        {
            elementAsFE.DataContext(nullptr);
        }
    }

    if (m_owner->ItemTemplateShim())
    {
        if (!m_ElementFactoryRecycleArgs)
        {
            // Create one.
            m_ElementFactoryRecycleArgs = tracker_ref<winrt::ElementFactoryRecycleArgs>(m_owner, winrt::ElementFactoryRecycleArgs());
        }

        auto context = m_ElementFactoryRecycleArgs.get();
        context.Element(element);
        context.Parent(*m_owner);

        m_owner->ItemTemplateShim().RecycleElement(context);

        context.Element(nullptr);
        context.Parent(nullptr);
    }
    else
    {
        // No ItemTemplate to recycle to, remove the element from the children collection.
        const auto children = m_owner->Children();
        unsigned int childIndex = 0;
        const bool found = children.IndexOf(element, childIndex);
        if (!found)
        {
            throw winrt::hresult_error(E_FAIL, L"ItemsRepeater's child not found in its Children collection.");
        }

        children.RemoveAt(childIndex);
    }

    m_phaser.StopPhasing(element, virtInfo);
    if (m_lastFocusedElement == element)
    {
        // Focused element is going away. Remove the tracked last focused element
        // and pick a reasonable next focus if we can find one within the layout 
        // realized elements.
        const int clearedIndex = virtInfo->Index();
        MoveFocusFromClearedIndex(clearedIndex);
    }

    ITEMSREPEATER_TRACE_PERF_DBG(L"ElementCleared");
}

void ViewManager::MoveFocusFromClearedIndex(int clearedIndex)
{
    winrt::UIElement focusedChild = nullptr;
    if (auto focusCandidate = FindFocusCandidate(clearedIndex, focusedChild))
    {
        winrt::FocusState focusState = winrt::FocusState::Programmatic;
        if (m_lastFocusedElement)
        {
            if (auto focusedAsControl = m_lastFocusedElement.try_as<winrt::Control>())
            {
                focusState = focusedAsControl.FocusState();
            }
        }

        // If the last focused element has focus, use its focus state, if not use programmatic.
        focusState = focusState == winrt::FocusState::Unfocused ? winrt::FocusState::Programmatic : focusState;

        if (WinAppSdk::Containment::IsChangeEnabled<WINAPPSDK_CHANGEID_50344748>())
        {
            // Since this focus change is due to the focused element getting recycled, don't activate the window.
            focusCandidate.as<winrt::IUIElementPrivate>().FocusNoActivate(focusState);
        }
        else
        {
            focusCandidate.Focus(focusState);
        }

        m_lastFocusedElement.set(focusedChild);
        // Add pin to hold the focused element.
        UpdatePin(focusedChild, true /* addPin */);
    }
    else
    {
        // We could not find a candiate.
        m_lastFocusedElement.set(nullptr);
    }
}

winrt::Control ViewManager::FindFocusCandidate(int clearedIndex, winrt::UIElement& focusedChild)
{
    // Walk through all the children and find elements with index before and after the cleared index.
    // Note that during a delete the next element would now have the same index.
    int previousIndex = std::numeric_limits<int>::min();
    int nextIndex = std::numeric_limits<int>::max();
    winrt::UIElement nextElement = nullptr;
    winrt::UIElement previousElement = nullptr;
    auto children = m_owner->Children();
    for (unsigned i = 0u; i < children.Size(); ++i)
    {
        auto child = children.GetAt(i);
        auto virtInfo = ItemsRepeater::TryGetVirtualizationInfo(child);
        if (virtInfo && virtInfo->IsHeldByLayout())
        {
            const int currentIndex = virtInfo->Index();
            if (currentIndex < clearedIndex)
            {
                if (currentIndex > previousIndex)
                {
                    previousIndex = currentIndex;
                    previousElement = child;
                }
            }
            else if (currentIndex >= clearedIndex)
            {
                // Note that we use >= above because if we deleted the focused element, 
                // the next element would have the same index now.
                if (currentIndex < nextIndex)
                {
                    nextIndex = currentIndex;
                    nextElement = child;
                }
            }
        }
    }

    // Find the next element if one exists, if not use the previous element.
    // If the container itself is not focusable, find a descendent that is.
    winrt::Control focusCandidate = nullptr;
    if (nextElement)
    {
        focusedChild = nextElement.try_as<winrt::UIElement>();
        focusCandidate = nextElement.try_as<winrt::Control>();
        if (!focusCandidate)
        {
            if (auto firstFocus = winrt::FocusManager::FindFirstFocusableElement(nextElement))
            {
                focusCandidate = firstFocus.try_as<winrt::Control>();
            }
        }
    }

    if (!focusCandidate && previousElement)
    {
        focusedChild = previousElement.try_as<winrt::UIElement>();
        focusCandidate = previousElement.try_as<winrt::Control>();
        if (!previousElement)
        {
            if (auto lastFocus = winrt::FocusManager::FindLastFocusableElement(previousElement))
            {
                focusCandidate = lastFocus.try_as<winrt::Control>();
            }
        }
    }

    return focusCandidate;
}

int ViewManager::GetElementIndex(const winrt::com_ptr<VirtualizationInfo>& virtInfo)
{
    if (!virtInfo)
    {
        //Element is not a child of this ItemsRepeater.
        return -1;
    }

    return virtInfo->IsRealized() || virtInfo->IsInUniqueIdResetPool() ? virtInfo->Index() : -1;
}

void ViewManager::PrunePinnedElements()
{
    EnsureEventSubscriptions();

    // Go through pinned elements and make sure they still have
    // a reason to be pinned.
    for (size_t i = 0; i < m_pinnedPool.size(); ++i)
    {
        auto elementInfo = m_pinnedPool[i];
        auto virtInfo = elementInfo.VirtualizationInfo();

        MUX_ASSERT(virtInfo->Owner() == ElementOwner::PinnedPool);

        if (!virtInfo->IsPinned())
        {
#ifdef DBG
            if (VirtualizationInfo::GetLogItemIndex() == virtInfo->Index())
            {
                ITEMSREPEATER_TRACE_INFO(nullptr, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"Unpinned element removed from PinnedPool.", VirtualizationInfo::GetLogItemIndex());
            }
#endif // DBG

            m_pinnedPool.erase(m_pinnedPool.begin() + i);
            --i;

            // Pinning was the only thing keeping this element alive.
            ClearElementToElementFactory(elementInfo.PinnedElement());
        }
#ifdef DBG
        else if (VirtualizationInfo::GetLogItemIndex() == virtInfo->Index())
        {
            ITEMSREPEATER_TRACE_INFO(nullptr, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"Pinned element remains in PinnedPool.", VirtualizationInfo::GetLogItemIndex());
        }
#endif // DBG
    }
}

void ViewManager::UpdatePin(const winrt::UIElement& element, bool addPin)
{
    auto parent = CachedVisualTreeHelpers::GetParent(element);
    auto child = static_cast<winrt::DependencyObject>(element);

    while (parent)
    {
        if (auto repeater = parent.try_as<winrt::ItemsRepeater>())
        {
            auto virtInfo = ItemsRepeater::GetVirtualizationInfo(child.as<winrt::UIElement>());
            if (virtInfo->IsRealized())
            {
                if (addPin)
                {
                    virtInfo->AddPin();
                }
                else if (virtInfo->IsPinned())
                {
                    if (virtInfo->RemovePin() == 0)
                    {
                        // ElementFactory is invoked during the measure pass.
                        // We will clear the element then.
                        repeater.InvalidateMeasure();
                    }
                }
            }
        }

        child = parent;
        parent = CachedVisualTreeHelpers::GetParent(child);
    }
}

void ViewManager::OnItemsSourceChanged(const winrt::IInspectable&, const winrt::NotifyCollectionChangedEventArgs& args)
{
    // Note: For items that have been removed, the index will not be touched. It will hold
    // the old index before it was removed. It is not valid anymore.
    switch (args.Action())
    {
    case winrt::NotifyCollectionChangedAction::Add:
    {
        const auto newIndex = args.NewStartingIndex();
        const auto newCount = args.NewItems().Size();
        EnsureFirstLastRealizedIndices();
        if (newIndex <= m_lastRealizedElementIndexHeldByLayout)
        {
            m_lastRealizedElementIndexHeldByLayout += newCount;
            auto children = m_owner->Children();
            const auto childCount = children.Size();
            for (unsigned i = 0u; i < childCount; ++i)
            {
                const auto element = children.GetAt(i);
                const auto virtInfo = ItemsRepeater::GetVirtualizationInfo(element);
                const auto dataIndex = virtInfo->Index();

                if (virtInfo->IsRealized() && dataIndex >= newIndex)
                {
                    UpdateElementIndex(element, virtInfo, dataIndex + newCount);
                }
            }
        }
        else
        {
            // Indices held by layout are not affected
            // We could still have items in the pinned elements that need updates. This is usually a very small vector.
            for (size_t i = 0; i < m_pinnedPool.size(); ++i)
            {
                const auto elementInfo = m_pinnedPool[i];
                const auto virtInfo = elementInfo.VirtualizationInfo();
                const auto dataIndex = virtInfo->Index();

                if (virtInfo->IsRealized() && dataIndex >= newIndex)
                {
                    auto element = elementInfo.PinnedElement();
                    UpdateElementIndex(element, virtInfo, dataIndex + newCount);
                }
            }
        }
        break;
    }

    case winrt::NotifyCollectionChangedAction::Replace:
    {
        // Requirement: oldStartIndex == newStartIndex. It is not a replace if this is not true.
        // Two cases here
        // case 1: oldCount == newCount 
        //         indices are not affected. nothing to do here.  
        // case 2: oldCount != newCount
        //         Replaced with less or more items. This is like an insert or remove
        //         depending on the counts.
        const auto oldStartIndex = args.OldStartingIndex();
        const auto newStartingIndex = args.NewStartingIndex();
        const auto oldCount = static_cast<int>(args.OldItems().Size());
        const auto newCount = static_cast<int>(args.NewItems().Size());
        if (oldStartIndex != newStartingIndex)
        {
            throw winrt::hresult_error(E_FAIL, L"Replace is only allowed with OldStartingIndex equals to NewStartingIndex.");
        }

        if (oldCount == 0)
        {
            throw winrt::hresult_error(E_FAIL, L"Replace notification with args.OldItemsCount value of 0 is not allowed. Use Insert action instead.");
        }

        if (newCount == 0)
        {
            throw winrt::hresult_error(E_FAIL, L"Replace notification with args.NewItemCount value of 0 is not allowed. Use Remove action instead.");
        }

        const int countChange = newCount - oldCount;
        if (countChange != 0)
        {
            // countChange > 0 : countChange items were added
            // countChange < 0 : -countChange  items were removed
            const auto children = m_owner->Children();
            for (unsigned i = 0u; i < children.Size(); ++i)
            {
                const auto element = children.GetAt(i);
                const auto virtInfo = ItemsRepeater::GetVirtualizationInfo(element);
                const auto dataIndex = virtInfo->Index();

                if (virtInfo->IsRealized())
                {
                    if (dataIndex >= oldStartIndex + oldCount)
                    {
                        UpdateElementIndex(element, virtInfo, dataIndex + countChange);
                    }
                }
            }

            EnsureFirstLastRealizedIndices();
            m_lastRealizedElementIndexHeldByLayout += countChange;
        }
        break;
    }

    case winrt::NotifyCollectionChangedAction::Remove:
    {
        const auto oldStartIndex = args.OldStartingIndex();
        const auto oldCount = static_cast<int>(args.OldItems().Size());
        const auto children = m_owner->Children();
        for (unsigned i = 0u; i < children.Size(); ++i)
        {
            const auto element = children.GetAt(i);
            const auto virtInfo = ItemsRepeater::GetVirtualizationInfo(element);
            const auto dataIndex = virtInfo->Index();

            if (virtInfo->IsRealized())
            {
                if (virtInfo->AutoRecycleCandidate() && oldStartIndex <= dataIndex && dataIndex < oldStartIndex + oldCount)
                {
                    // If we are doing the mapping, remove the element who's data was removed.
                    m_owner->ClearElementImpl(element);
                }
                else if (dataIndex >= (oldStartIndex + oldCount))
                {
                    UpdateElementIndex(element, virtInfo, dataIndex - oldCount);
                }
            }
        }

        InvalidateRealizedIndicesHeldByLayout();
        break;
    }

    case winrt::NotifyCollectionChangedAction::Reset:
        // If we get multiple resets back to back before
        // running layout, we dont have to clear all the elements again.         
        if (!m_isDataSourceStableResetPending)
        {
            // There should be no elements in the reset pool at this time.
            MUX_ASSERT(m_resetPool.IsEmpty());

            if (m_owner->ItemsSourceView().HasKeyIndexMapping())
            {
                m_isDataSourceStableResetPending = true;
            }

            // Walk through all the elements and make sure they are cleared, they will go into
            // the stable id reset pool.
            auto children = m_owner->Children();
            for (unsigned i = 0u; i < children.Size(); ++i)
            {
                auto element = children.GetAt(i);
                auto virtInfo = ItemsRepeater::GetVirtualizationInfo(element);
                if (virtInfo->IsRealized() && virtInfo->AutoRecycleCandidate())
                {
                    m_owner->ClearElementImpl(element);
                }
            }
        }

        InvalidateRealizedIndicesHeldByLayout();

        break;
    }
}

void ViewManager::EnsureFirstLastRealizedIndices()
{
    if (m_firstRealizedElementIndexHeldByLayout == FirstRealizedElementIndexDefault)
    {
        // This will ensure that the indexes are updated.
        auto element = GetElementIfAlreadyHeldByLayout(0);
    }
}

void ViewManager::OnLayoutChanging()
{
    if (m_owner->ItemsSourceView() &&
        m_owner->ItemsSourceView().HasKeyIndexMapping())
    {
        m_isDataSourceStableResetPending = true;
    }
}

void ViewManager::OnOwnerArranged()
{
    if (m_isDataSourceStableResetPending)
    {
        m_isDataSourceStableResetPending = false;

        for (auto& entry : m_resetPool)
        {
            // TODO: Task 14204306: ItemsRepeater: Find better focus candidate when focused element is deleted in the ItemsSource.
            // Focused element is getting cleared. Need to figure out semantics on where
            // focus should go when the focused element is removed from the data collection.
            ClearElement(entry.second.get(), true /* isClearedDueToCollectionChange */);
        }

        m_resetPool.Clear();

        // Flush the realized indices once the stable reset pool is cleared to start fresh.
        InvalidateRealizedIndicesHeldByLayout();
    }
}

#pragma region GetElement providers

// We optimize for the case where index is not realized to return null as quickly as we can.
// Flow layouts manage containers on their own and will never ask for an index that is already realized.
// If an index that is realized is requested by the layout, we unfortunately have to walk the
// children. Not ideal, but a reasonable default to provide consistent behavior between virtualizing
// and non-virtualizing hosts.
winrt::UIElement ViewManager::GetElementIfAlreadyHeldByLayout(int index)
{
    winrt::UIElement element = nullptr;

    const bool cachedFirstLastIndicesInvalid = m_firstRealizedElementIndexHeldByLayout == FirstRealizedElementIndexDefault;
    MUX_ASSERT(!cachedFirstLastIndicesInvalid || m_lastRealizedElementIndexHeldByLayout == LastRealizedElementIndexDefault);

    const bool isRequestedIndexInRealizedRange = (m_firstRealizedElementIndexHeldByLayout <= index && index <= m_lastRealizedElementIndexHeldByLayout);

    if (cachedFirstLastIndicesInvalid || isRequestedIndexInRealizedRange)
    {
        // Both First and Last indices need to be valid or default.
        MUX_ASSERT((m_firstRealizedElementIndexHeldByLayout == FirstRealizedElementIndexDefault && m_lastRealizedElementIndexHeldByLayout == LastRealizedElementIndexDefault) ||
            (m_firstRealizedElementIndexHeldByLayout != FirstRealizedElementIndexDefault && m_lastRealizedElementIndexHeldByLayout != LastRealizedElementIndexDefault));

        auto children = m_owner->Children();
        for (unsigned i = 0u; i < children.Size(); ++i)
        {
            auto child = children.GetAt(i);
            auto virtInfo = ItemsRepeater::TryGetVirtualizationInfo(child);
            if (virtInfo && virtInfo->IsHeldByLayout())
            {
                // Only give back elements held by layout. If someone else is holding it, they will be served by other methods.
                const int childIndex = virtInfo->Index();
                m_firstRealizedElementIndexHeldByLayout = std::min(m_firstRealizedElementIndexHeldByLayout, childIndex);
                m_lastRealizedElementIndexHeldByLayout = std::max(m_lastRealizedElementIndexHeldByLayout, childIndex);
                if (virtInfo->Index() == index)
                {
                    element = child;
                    // If we have valid first/last indices, we don't have to walk the rest, but if we 
                    // do not, then we keep walking through the entire children collection to get accurate
                    // indices once.
                    if (!cachedFirstLastIndicesInvalid)
                    {
                        break;
                    }
                }
            }
        }
    }

    return element;
}

winrt::UIElement ViewManager::GetElementFromUniqueIdResetPool(int index)
{
    winrt::UIElement element = nullptr;
    // See if you can get it from the reset pool.
    if (m_isDataSourceStableResetPending)
    {
        element = m_resetPool.Remove(index);
        if (element)
        {
            // Make sure that the index is updated to the current one
            auto virtInfo = ItemsRepeater::GetVirtualizationInfo(element);
            virtInfo->MoveOwnershipToLayoutFromUniqueIdResetPool();
            UpdateElementIndex(element, virtInfo, index);

            // Update realized indices
            m_firstRealizedElementIndexHeldByLayout = std::min(m_firstRealizedElementIndexHeldByLayout, index);
            m_lastRealizedElementIndexHeldByLayout = std::max(m_lastRealizedElementIndexHeldByLayout, index);
        }
    }

#ifdef DBG
    if (VirtualizationInfo::GetLogItemIndex() == index)
    {
        ITEMSREPEATER_TRACE_INFO(nullptr, TRACE_MSG_METH_PTR_INT, METH_NAME, this, element, VirtualizationInfo::GetLogItemIndex());
    }
#endif // DBG

    return element;
}

winrt::UIElement ViewManager::GetElementFromPinnedElements(int index)
{
    winrt::UIElement element = nullptr;

    // See if you can find something among the pinned elements.
    for (size_t i = 0; i < m_pinnedPool.size(); ++i)
    {
        auto elementInfo = m_pinnedPool[i];
        auto virtInfo = elementInfo.VirtualizationInfo();

        if (virtInfo->Index() == index)
        {
#ifdef DBG
            if (VirtualizationInfo::GetLogItemIndex() == index)
            {
                ITEMSREPEATER_TRACE_INFO(nullptr, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"Element removed from PinnedPool.", index);
            }
#endif // DBG

            m_pinnedPool.erase(m_pinnedPool.begin() + i);
            element = elementInfo.PinnedElement();
            elementInfo.VirtualizationInfo()->MoveOwnershipToLayoutFromPinnedPool();

            // Update realized indices
            m_firstRealizedElementIndexHeldByLayout = std::min(m_firstRealizedElementIndexHeldByLayout, index);
            m_lastRealizedElementIndexHeldByLayout = std::max(m_lastRealizedElementIndexHeldByLayout, index);
            break;
        }
    }

#ifdef DBG
    if (VirtualizationInfo::GetLogItemIndex() == index)
    {
        ITEMSREPEATER_TRACE_INFO(nullptr, TRACE_MSG_METH_PTR_INT, METH_NAME, this, element, index);
    }
#endif // DBG

    return element;
}

// There are several cases handled here with respect to which element gets returned and when DataContext is modified.
//
// 1. If there is no ItemTemplate:
//    1.1 If data is a UIElement -> the data is returned
//    1.2 If data is not a UIElement -> a default DataTemplate is used to fetch element and DataContext is set to data**
//
// 2. If there is an ItemTemplate:
//    2.1 If data is not a FrameworkElement -> Element is fetched from ElementFactory and DataContext is set to the data**
//    2.2 If data is a FrameworkElement:
//        2.2.1 If Element returned by the ElementFactory is the same as the data -> Element (a.k.a. data) is returned as is
//        2.2.2 If Element returned by the ElementFactory is not the same as the data
//                 -> Element that is fetched from the ElementFactory is returned and
//                    DataContext is set to the data's DataContext (if it exists), otherwise it is set to the data itself**
//
// **data context is set only if no x:Bind was used. ie. No data template component on the root.
winrt::UIElement ViewManager::GetElementFromElementFactory(int index)
{
    // The view generator is the provider of last resort.
    auto const data = m_owner->ItemsSourceView().GetAt(index);

    auto const element = [this, data, index, providedElementFactory = m_owner->ItemTemplateShim()]()
    {
        if (!providedElementFactory)
        {
            if (auto const dataAsElement = data.try_as<winrt::UIElement>())
            {
                return dataAsElement;
            }
        }

        auto const elementFactory = [this, providedElementFactory]()
        {
            if (!providedElementFactory)
            {
                // If no ItemTemplate was provided, use a default
                auto const factory = winrt::XamlReader::Load(L"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'><TextBlock Text='{Binding}'/></DataTemplate>").as<winrt::DataTemplate>();
                m_owner->ItemTemplate(factory);
                return m_owner->ItemTemplateShim();
            }
            return providedElementFactory;
        }();

        auto const args = [this]()
        {
            if (!m_ElementFactoryGetArgs)
            {
                m_ElementFactoryGetArgs = tracker_ref<winrt::ElementFactoryGetArgs>(m_owner, winrt::ElementFactoryGetArgs());
            }
            return m_ElementFactoryGetArgs.get();
        }();

        auto scopeGuard = gsl::finally([args]()
            {
                args.Data(nullptr);
                args.Parent(nullptr);
            });

        args.Data(data);
        args.Parent(*m_owner);
        RepeaterTestHooks::SetElementFactoryElementIndex(index);

        return elementFactory.GetElement(args);
    }();

    auto virtInfo = ItemsRepeater::TryGetVirtualizationInfo(element);
    if (!virtInfo)
    {
        virtInfo = ItemsRepeater::CreateAndInitializeVirtualizationInfo(element);
        ITEMSREPEATER_TRACE_PERF_DBG(L"ElementCreated");
    }
    else
    {
        // View obtained from ElementFactory already has a VirtualizationInfo attached to it
        // which means that the element has been recycled and not created from scratch.
        ITEMSREPEATER_TRACE_PERF_DBG(L"ElementRecycled");
    }
    // Clear flag
    virtInfo->MustClearDataContext(false);

    if (data != element)
    {
        // Prepare the element
        // If we are phasing, run phase 0 before setting DataContext. If phase 0 is not 
        // run before setting DataContext, when setting DataContext all the phases will be
        // run in the OnDataContextChanged handler in code generated by the xaml compiler (code-gen).
        if (auto extension = CachedVisualTreeHelpers::GetDataTemplateComponent(element))
        {
            // Clear out old data. 
            extension.Recycle();
            int nextPhase = VirtualizationInfo::PhaseReachedEnd;
            // Run Phase 0
            extension.ProcessBindings(data, index, 0 /* currentPhase */, nextPhase);

            // Setup phasing information, so that Phaser can pick up any pending phases left.
            // Update phase on virtInfo. Set data and templateComponent only if x:Phase was used.
            virtInfo->UpdatePhasingInfo(nextPhase, nextPhase > 0 ? data : nullptr, nextPhase > 0 ? extension : nullptr);
        }
        else if (auto elementAsFE = element.try_as<winrt::FrameworkElement>())
        {
            // Set data context only if no x:Bind was used. ie. No data template component on the root.
            // If the passed in data is a UIElement and is different from the element returned by 
            // the template factory then we need to propagate the DataContext.
            // Otherwise just set the DataContext on the element as the data.
            auto const elementDataContext = [this, data]()
            {
                if (auto const dataAsElement = data.try_as<winrt::FrameworkElement>())
                {
                    if (auto const dataDataContext = dataAsElement.DataContext())
                    {
                        return dataDataContext;
                    }
                }
                return data;
            }();

            elementAsFE.DataContext(elementDataContext);
            virtInfo->MustClearDataContext(true);
        }
        else
        {
            MUX_ASSERT(L"Element returned by factory is not a FrameworkElement!");
        }
    }

    virtInfo->MoveOwnershipToLayoutFromElementFactory(
        index,
        /* uniqueId: */
        m_owner->ItemsSourceView().HasKeyIndexMapping() ?
        m_owner->ItemsSourceView().KeyFromIndex(index) :
        winrt::hstring{});

    // The view generator is the only provider that prepares the element.
    auto repeater = m_owner;

    // Add the element to the children collection here before raising OnElementPrepared so 
    // that handlers can walk up the tree in case they want to find their IndexPath in the 
    // nested case.
    auto children = repeater->Children();
    if (CachedVisualTreeHelpers::GetParent(element) != static_cast<winrt::DependencyObject>(*repeater))
    {
        children.Append(element);
    }
    
    repeater->TransitionManager().OnElementPrepared(element);

    repeater->OnElementPrepared(element, index);

    if (data != element)
    {
        m_phaser.PhaseElement(element, virtInfo);
    }

    // Update realized indices
    m_firstRealizedElementIndexHeldByLayout = std::min(m_firstRealizedElementIndexHeldByLayout, index);
    m_lastRealizedElementIndexHeldByLayout = std::max(m_lastRealizedElementIndexHeldByLayout, index);

#ifdef DBG
    if (VirtualizationInfo::GetLogItemIndex() == index)
    {
        ITEMSREPEATER_TRACE_INFO(nullptr, TRACE_MSG_METH_PTR_INT, METH_NAME, this, element, index);
    }
#endif // DBG

    return element;
}

#pragma endregion

#pragma region ClearElement handlers

bool ViewManager::ClearElementToUniqueIdResetPool(const winrt::UIElement& element, const winrt::com_ptr<VirtualizationInfo>& virtInfo)
{
    if (m_isDataSourceStableResetPending)
    {
        m_resetPool.Add(element);
        virtInfo->MoveOwnershipToUniqueIdResetPoolFromLayout();
    }

    return m_isDataSourceStableResetPending;
}

bool ViewManager::ClearElementToAnimator(const winrt::UIElement& element, const winrt::com_ptr<VirtualizationInfo>& virtInfo)
{
    const bool cleared = m_owner->TransitionManager().ClearElement(element);
    if (cleared)
    {
        const int clearedIndex = virtInfo->Index();
        virtInfo->MoveOwnershipToAnimator();
        if (m_lastFocusedElement == element)
        {
            // Focused element is going away. Remove the tracked last focused element
            // and pick a reasonable next focus if we can find one within the layout 
            // realized elements.
            MoveFocusFromClearedIndex(clearedIndex);
        }

    }
    return cleared;
}

bool ViewManager::ClearElementToPinnedPool(const winrt::UIElement& element, const winrt::com_ptr<VirtualizationInfo>& virtInfo, bool isClearedDueToCollectionChange)
{
    const bool moveToPinnedPool =
        !isClearedDueToCollectionChange && virtInfo->IsPinned();

    if (moveToPinnedPool)
    {
#ifdef DBG
        if (VirtualizationInfo::GetLogItemIndex() == virtInfo->Index())
        {
            ITEMSREPEATER_TRACE_INFO(nullptr, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"Element added to PinnedPool.", VirtualizationInfo::GetLogItemIndex());
        }
#endif // DBG

#ifdef DBG
        for (size_t i = 0; i < m_pinnedPool.size(); ++i)
        {
            MUX_ASSERT(m_pinnedPool[i].PinnedElement() != element);
        }
#endif
        m_pinnedPool.push_back(PinnedElementInfo(m_owner, element));
        virtInfo->MoveOwnershipToPinnedPool();
    }

    return moveToPinnedPool;
}

#pragma endregion

void ViewManager::UpdateFocusedElement()
{
    auto owner = static_cast<winrt::UIElement>(*m_owner);
    winrt::UIElement focusedElement = nullptr;
    winrt::DependencyObject child = nullptr;

    auto xamlRoot = owner.XamlRoot();
    if (xamlRoot)
    {
        child = winrt::FocusManager::GetFocusedElement(xamlRoot).as<winrt::DependencyObject>();
    }

    if (child)
    {
        auto parent = CachedVisualTreeHelpers::GetParent(child);

        // Find out if the focused element belongs to one of our direct
        // children.
        while (parent)
        {
            auto repeater = parent.try_as<winrt::ItemsRepeater>();
            if (repeater)
            {
                auto element = child.as<winrt::UIElement>();
                if (repeater == owner && ItemsRepeater::GetVirtualizationInfo(element)->IsRealized())
                {
                    focusedElement = element;
                }

                break;
            }

            child = parent;
            parent = CachedVisualTreeHelpers::GetParent(child);
        }
    }

    // If the focused element has changed,
    // we need to unpin the old one and pin the new one.
    if (m_lastFocusedElement != focusedElement)
    {
        if (m_lastFocusedElement)
        {
            UpdatePin(m_lastFocusedElement.get(), false /* addPin */);
        }

        if (focusedElement)
        {
            UpdatePin(focusedElement, true /* addPin */);
        }

        m_lastFocusedElement.set(focusedElement);
    }
}

void ViewManager::OnFocusChanged(const winrt::IInspectable&, const winrt::RoutedEventArgs&)
{
    UpdateFocusedElement();
}

void ViewManager::EnsureEventSubscriptions()
{
    if (!m_gotFocus)
    {
        MUX_ASSERT(!m_lostFocus);
        m_gotFocus = m_owner->GotFocus(winrt::auto_revoke, { this, &ViewManager::OnFocusChanged });
        m_lostFocus = m_owner->LostFocus(winrt::auto_revoke, { this, &ViewManager::OnFocusChanged });
    }
}

void ViewManager::UpdateElementIndex(const winrt::UIElement& element, const winrt::com_ptr<VirtualizationInfo>& virtInfo, int index)
{
    const auto oldIndex = virtInfo->Index();
    if (oldIndex != index)
    {
        virtInfo->UpdateIndex(index);
        m_owner->OnElementIndexChanged(element, oldIndex, index);
    }
}

void ViewManager::InvalidateRealizedIndicesHeldByLayout()
{
    m_firstRealizedElementIndexHeldByLayout = FirstRealizedElementIndexDefault;
    m_lastRealizedElementIndexHeldByLayout = LastRealizedElementIndexDefault;
}

ViewManager::PinnedElementInfo::PinnedElementInfo(const ITrackerHandleManager* owner, const winrt::UIElement& element) :
    m_pinnedElement(owner, element),
    m_virtInfo(owner, ItemsRepeater::GetVirtualizationInfo(element))
{
    ITEMSREPEATER_TRACE_INFO_DBG(nullptr, TRACE_MSG_METH_INT, METH_NAME, this, m_virtInfo.get()->Index());
}
