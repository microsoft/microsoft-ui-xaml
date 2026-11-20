// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "ItemsRepeater.common.h"
#include "ViewManager.h"
#include "ItemsRepeater.h"
#include "RepeaterTestHooks.h"

ViewManager::ViewManager(ItemsRepeater* owner) :
    m_owner(owner),
    m_resetPool(owner),
    m_lastFocusedElement(owner),
    m_pendingFocusMoveElement(owner),
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

    if (virtInfo->IsInPinnedPool())
    {
        // The element is not held by the layout, so the realized range is not affected.
        // Only a collection change releases a pinned element (layout clears keep it, for example to keep focus).
        // It must not go to the unique id reset pool or the animator, which only take elements owned by the layout.
        if (isClearedDueToCollectionChange)
        {
            const auto it = std::find_if(m_pinnedPool.begin(), m_pinnedPool.end(),
                [&element](const PinnedElementInfo& info) { return info.PinnedElement() == element; });
            MUX_ASSERT(it != m_pinnedPool.end());
            if (it != m_pinnedPool.end())
            {
                m_pinnedPool.erase(it);
            }

            ClearElementToElementFactory(element);
        }
        return;
    }

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

void ViewManager::RecycleWithoutOwner(bool recycleWithoutOwner)
{
    m_recycleWithoutOwner = recycleWithoutOwner;
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
    // Read the index before the ownership move resets it.
    const int clearedIndex = virtInfo->Index();
    const bool isFocusedElement = m_lastFocusedElement == element;
    const bool containsNestedFocus = ContainsNestedFocus(element, virtInfo);
    // Focus moved to this element before the focus events updated the tracked element (see OnFocusedElementCleared).
    const bool isFocusedAfterPendingMove = !isFocusedElement && !containsNestedFocus &&
        m_pendingFocusMoveElement && m_pendingFocusMoveElement.get() != element && GetFocusedChild() == element;
    // Focus is outside the owner, for example in a flyout opened from this element, and can return to it.
    const auto lastFocusedElementBeforeFocusLeft = m_lastFocusedElement ? nullptr : m_lastFocusedElementBeforeFocusLeft.get();
    const bool isLastFocusedElement = isFocusedElement || containsNestedFocus || lastFocusedElementBeforeFocusLeft == element;
    // Read before the element leaves the tree: then the core moves focus to the next tab stop (CControl::LeaveImpl).
    const bool hasFocus = isFocusedAfterPendingMove || ((isFocusedElement || containsNestedFocus) && GetFocusedChild() == element);
    if (isFocusedElement)
    {
        // The focus pins of the ancestor repeaters' elements are released when the pending focus move runs. Releasing
        // them now could let an ancestor repeater clear the element that contains the owner (an element pinned out of
        // view) before the move. The element's own pin is reset by MoveOwnershipToElementFactory; removing it here
        // would invalidate the owner's measure during the collection change, and an UpdateLayout call from an
        // ElementClearing handler would then run the owner's layout inside the change, which throws.
        ++m_pendingAncestorUnpinCount;
    }

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
        // If we are recycling without owner, avoid setting Parent as the parent would be used
        // to set as owner during RecycleElement call.
        if (m_recycleWithoutOwner)
        {
            context.Parent(nullptr);
        }
        else
        {
            context.Parent(*m_owner);
        }

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
    if (isFocusedElement || containsNestedFocus)
    {
        OnFocusedElementCleared(element, clearedIndex);
    }
    else if (isFocusedAfterPendingMove)
    {
        m_pendingFocusMoveElement.set(element);
        m_pendingFocusMoveOnChangeProcessed = IsProcessingRemove();
    }

    if (hasFocus && m_pendingFocusMoveElement)
    {
        // An element factory that removes the element from Children (no ItemTemplate, NavigationViewItemsFactory) makes
        // the core move focus to the next tab stop, which can be outside the owner. That is not a user move.
        m_pendingFocusMoveFocusLeftOwner = !GetFocusedChild();
        if (m_pendingFocusMoveFocusLeftOwner)
        {
            const auto xamlRoot = static_cast<winrt::UIElement>(*m_owner).XamlRoot();
            const auto focused = xamlRoot ? winrt::FocusManager::GetFocusedElement(xamlRoot).try_as<winrt::DependencyObject>() : nullptr;
            m_pendingFocusMoveFocusAfterClear = focused ? winrt::make_weak(focused) : nullptr;
        }
    }

    // When a remove transition completes, the element that was handed to it is cleared again, with no index (it is owned
    // by the animator). Keep the index remembered when it was handed to the transition.
    const bool isRememberedElementWithoutIndex = clearedIndex < 0 && m_clearedFocusedElement.get() == element;
    if (isLastFocusedElement && !isRememberedElementWithoutIndex)
    {
        m_clearedFocusedElement = winrt::make_weak(element);
        m_clearedFocusedElementIndex = AdjustClearedIndexForCollectionChange(clearedIndex);
        m_clearedFocusedElementIndexTracksItem = ClearedIndexTracksItem();
    }

    ITEMSREPEATER_TRACE_PERF_DBG(L"ElementCleared");
}

// UpdateFocusedElement tracks only the elements of the repeater nearest to the focused element. When focus is on an item
// of a nested repeater inside the element, the element is not tracked, but it is pinned (UpdatePin pins the elements of
// the ancestor repeaters). Clearing it must move focus like clearing a tracked element: the nested repeater cannot move
// focus to one of this repeater's items.
bool ViewManager::ContainsNestedFocus(const winrt::UIElement& element, const winrt::com_ptr<VirtualizationInfo>& virtInfo)
{
    return !m_lastFocusedElement && !m_pendingFocusMoveElement && virtInfo->IsPinned() && GetFocusedChild() == element;
}

bool ViewManager::IsFocusMovePendingFor(const winrt::UIElement& element) const
{
    return m_pendingFocusMoveElement && m_pendingFocusMoveElement.get() == element;
}

// The tracked focused element is going away. Stop tracking it, and move focus after the next layout pass:
// during a collection change the other realized elements can be cleared by the same change, and during
// layout the realized range is being updated, so a target picked now could be cleared next.
void ViewManager::OnFocusedElementCleared(const winrt::UIElement& element, int clearedIndex)
{
    // False when focus is in a nested repeater inside the element (ContainsNestedFocus).
    const bool isTrackedElement = m_lastFocusedElement == element;
    m_lastFocusedElement.set(nullptr);
    clearedIndex = AdjustClearedIndexForCollectionChange(clearedIndex);
    const auto focusState = GetFocusStateForMove();

    m_pendingFocusMoveElement.set(element);
    if (const auto focusedChild = GetFocusedChild())
    {
        if (focusedChild != element)
        {
            const auto focusedChildVirtInfo = ItemsRepeater::TryGetVirtualizationInfo(focusedChild);
            if (focusedChildVirtInfo && !focusedChildVirtInfo->IsHeldByLayout())
            {
                // Focus moved to another element before the focus events updated the tracked element, and that
                // element was cleared too. Focus is moved away from it instead.
                m_pendingFocusMoveElement.set(focusedChild);
            }
        }
    }
    m_pendingFocusMoveIndex = clearedIndex;
    m_pendingFocusMoveIndexTracksItem = ClearedIndexTracksItem();
    m_pendingFocusMoveState = focusState;
    m_pendingFocusMoveFocusLeftOwner = false;
    m_pendingFocusMoveFocusAfterClear = nullptr;
    m_pendingFocusMoveInterimElement = nullptr;
    // Also after a Replace of the tracked element. Not when focus is in a nested repeater inside the element: the layout
    // reuses the element for the new item and the nested repeater keeps focus at the same inner position.
    const auto change = m_owner->ProcessingCollectionChange();
    m_pendingFocusMoveOnChangeProcessed = IsProcessingRemove() ||
        (isTrackedElement && change && change.Action() == winrt::NotifyCollectionChangedAction::Replace);
}

bool ViewManager::IsProcessingRemove()
{
    const auto change = m_owner->ProcessingCollectionChange();
    return change && change.Action() == winrt::NotifyCollectionChangedAction::Remove;
}

// True if the cleared index is the position of an item that later changes can shift: the element was cleared by a
// Remove or Replace. After a Reset the index is only a position in a new item list.
bool ViewManager::ClearedIndexTracksItem()
{
    const auto change = m_owner->ProcessingCollectionChange();
    return change &&
        change.Action() != winrt::NotifyCollectionChangedAction::Reset &&
        change.Action() != winrt::NotifyCollectionChangedAction::Move;
}

int ViewManager::AdjustClearedIndexForCollectionChange(int clearedIndex)
{
    if (const auto change = m_owner->ProcessingCollectionChange())
    {
        if (change.Action() == winrt::NotifyCollectionChangedAction::Remove)
        {
            // The items that follow the removed range now start at the first removed index.
            clearedIndex = std::min(clearedIndex, change.OldStartingIndex());
        }
    }
    return clearedIndex;
}

// Returns the index that the item position 'index' has after the change: inserted items before it move it forward,
// removed items before it move it back, and if its item was removed, the first item that followed it takes its place.
int ViewManager::ShiftIndexForCollectionChange(int index, const winrt::NotifyCollectionChangedEventArgs& args)
{
    if (index < 0)
    {
        return index;
    }

    switch (args.Action())
    {
    case winrt::NotifyCollectionChangedAction::Add:
    {
        const int newIndex = args.NewStartingIndex();
        if (newIndex >= 0 && newIndex <= index)
        {
            index += static_cast<int>(args.NewItems().Size());
        }
        break;
    }

    case winrt::NotifyCollectionChangedAction::Remove:
    {
        const int oldIndex = args.OldStartingIndex();
        const int oldCount = static_cast<int>(args.OldItems().Size());
        if (oldIndex >= 0 && oldIndex < index)
        {
            index = std::max(oldIndex, index - oldCount);
        }
        break;
    }

    case winrt::NotifyCollectionChangedAction::Replace:
    {
        const int oldIndex = args.OldStartingIndex();
        const int oldCount = static_cast<int>(args.OldItems().Size());
        if (oldIndex >= 0 && index >= oldIndex + oldCount)
        {
            index += static_cast<int>(args.NewItems().Size()) - oldCount;
        }
        break;
    }

    default:
        // Reset and Move: the item positions are not known.
        break;
    }

    return index;
}

winrt::FocusState ViewManager::GetFocusStateForMove()
{
    if (const auto xamlRoot = static_cast<winrt::UIElement>(*m_owner).XamlRoot())
    {
        if (const auto focusedAsControl = winrt::FocusManager::GetFocusedElement(xamlRoot).try_as<winrt::Control>())
        {
            const auto focusState = focusedAsControl.FocusState();
            if (focusState != winrt::FocusState::Unfocused)
            {
                return focusState;
            }
        }
    }
    return winrt::FocusState::Programmatic;
}

// Focus returned to a cleared element that had focus, for example when a flyout opened from the element closes after
// the element's item was removed (the popup restores focus to the element that had focus when it opened). Move focus
// to the item at the cleared index after the next layout pass, as if focus had stayed on the element. Returns true if
// the move was recorded.
bool ViewManager::TryRecordFocusReturnedToClearedElement(const winrt::UIElement& element)
{
    if (m_pendingFocusMoveElement || m_lastFocusedElement || !m_clearedFocusedElement || m_clearedFocusedElementIndex < 0)
    {
        return false;
    }

    const auto clearedElement = m_clearedFocusedElement.get();
    if (!clearedElement || clearedElement != element || GetFocusedChild() != clearedElement)
    {
        return false;
    }

    const auto virtInfo = ItemsRepeater::TryGetVirtualizationInfo(clearedElement);
    if (!virtInfo || virtInfo->IsRealized())
    {
        return false;
    }

    m_pendingFocusMoveElement.set(clearedElement);
    m_pendingFocusMoveIndex = m_clearedFocusedElementIndex;
    m_pendingFocusMoveIndexTracksItem = m_clearedFocusedElementIndexTracksItem;
    m_pendingFocusMoveState = GetFocusStateForMove();
    m_pendingFocusMoveFocusLeftOwner = false;
    m_pendingFocusMoveFocusAfterClear = nullptr;
    m_pendingFocusMoveInterimElement = nullptr;
    m_pendingFocusMoveOnChangeProcessed = false;
    return true;
}

void ViewManager::ReleaseAncestorPinsOfClearedFocusedElement()
{
    for (; m_pendingAncestorUnpinCount > 0u; --m_pendingAncestorUnpinCount)
    {
        UpdatePin(static_cast<winrt::UIElement>(*m_owner), false /* addPin */);
    }
}

void ViewManager::OnOwnerLayoutUpdated()
{
    ReleaseAncestorPinsOfClearedFocusedElement();

    if (!m_pendingFocusMoveElement)
    {
        return;
    }

    const auto clearedElement = m_pendingFocusMoveElement.get();
    const int clearedIndex = m_pendingFocusMoveIndex;
    const auto focusState = m_pendingFocusMoveState;
    const bool focusLeftOwner = m_pendingFocusMoveFocusLeftOwner;
    const auto focusAfterClear = m_pendingFocusMoveFocusAfterClear.get();
    const auto interimElement = m_pendingFocusMoveInterimElement.get();
    m_pendingFocusMoveElement.set(nullptr);
    m_pendingFocusMoveFocusLeftOwner = false;
    m_pendingFocusMoveFocusAfterClear = nullptr;
    m_pendingFocusMoveInterimElement = nullptr;

    // Only move focus if it is still on a child that is not held by the layout (the cleared element, or another
    // cleared element focused in the meantime), still on the element that focus moved to when the change was processed,
    // or still where the core moved it when the cleared element left the tree. If the app or user moved focus elsewhere,
    // leave it there.
    const auto focusedChild = GetFocusedChild();
    if (!focusedChild)
    {
        const auto xamlRoot = static_cast<winrt::UIElement>(*m_owner).XamlRoot();
        if (!focusLeftOwner || !xamlRoot ||
            winrt::FocusManager::GetFocusedElement(xamlRoot).try_as<winrt::DependencyObject>() != focusAfterClear)
        {
            return;
        }
    }

    const auto focusedChildVirtInfo = focusedChild ? ItemsRepeater::TryGetVirtualizationInfo(focusedChild) : nullptr;
    if (focusedChildVirtInfo && focusedChildVirtInfo->IsHeldByLayout() && focusedChild != clearedElement && focusedChild != interimElement)
    {
        UpdateFocusedElement();
        return;
    }

    if (!TryFocusRealizedChild(clearedIndex, focusState, focusedChild))
    {
        // There is no realized item to move focus to, for example because the source is empty.
        TryMoveFocusOutOfOwner(focusState);
    }

    // A GettingFocus handler may have changed the source and cleared the new target. If focus is now on another
    // element that is not held by the layout, try again after the next layout pass.
    const auto newFocusedChild = GetFocusedChild();
    const auto newFocusedChildVirtInfo = newFocusedChild ? ItemsRepeater::TryGetVirtualizationInfo(newFocusedChild) : nullptr;
    if (newFocusedChildVirtInfo && newFocusedChild != focusedChild && !newFocusedChildVirtInfo->IsHeldByLayout())
    {
        m_pendingFocusMoveElement.set(newFocusedChild);
        m_pendingFocusMoveIndex = clearedIndex;
        m_pendingFocusMoveState = focusState;
        m_pendingFocusMoveOnChangeProcessed = false;
        return;
    }

    UpdateFocusedElement();
}

// The owner processed a change of its items source; this runs outside of the change and of layout. Input that is already
// queued (keys pressed in a row) is delivered before the next layout pass, so focus must not stay on an element that a
// Remove or a Replace cleared until then: move it now to the realized element nearest to the cleared index, among the
// elements that the change kept (after a Remove, the item that follows the removed one has its index). The move after the
// layout pass then moves focus to the item at the cleared index (after a Replace, the new item). Not done for a Reset, or
// when focus is in a nested repeater inside the element cleared by a Replace: the layout reuses the element for the new
// item and the nested repeater keeps focus at the same inner position.
void ViewManager::OnOwnerCollectionChangeProcessed()
{
    if (!m_pendingFocusMoveElement || !m_pendingFocusMoveOnChangeProcessed)
    {
        return;
    }

    // As in OnOwnerLayoutUpdated: only if focus is still on the cleared element, or where the core moved it when the
    // cleared element left the tree.
    if (const auto focusedChild = GetFocusedChild())
    {
        if (focusedChild != m_pendingFocusMoveElement.get())
        {
            return;
        }
    }
    else
    {
        const auto xamlRoot = static_cast<winrt::UIElement>(*m_owner).XamlRoot();
        if (!m_pendingFocusMoveFocusLeftOwner || !xamlRoot ||
            winrt::FocusManager::GetFocusedElement(xamlRoot).try_as<winrt::DependencyObject>() != m_pendingFocusMoveFocusAfterClear.get())
        {
            return;
        }
    }

    bool hasRealizedChild = false;
    const auto children = m_owner->Children();
    for (unsigned i = 0u; i < children.Size() && !hasRealizedChild; ++i)
    {
        const auto virtInfo = ItemsRepeater::TryGetVirtualizationInfo(children.GetAt(i));
        hasRealizedChild = virtInfo && virtInfo->IsHeldByLayout();
    }
    if (!hasRealizedChild)
    {
        // The move after the layout pass runs, once the layout has realized the current items.
        return;
    }

    winrt::UIElement focusedCandidate{ nullptr };
    if (TryFocusRealizedChild(m_pendingFocusMoveIndex, m_pendingFocusMoveState, nullptr /* focusedChild */, &focusedCandidate))
    {
        // The move after the layout pass checks that focus is still on this element. If a GettingFocus handler moved
        // focus to another element, that element is the app's choice.
        const auto newFocusedChild = GetFocusedChild();
        m_pendingFocusMoveInterimElement = newFocusedChild && newFocusedChild == focusedCandidate ? winrt::make_weak(newFocusedChild) : nullptr;
    }
    else
    {
        // Every item refused focus. Move focus out of the owner now, as the move after the layout pass would, instead of
        // trying the same items again then.
        m_pendingFocusMoveElement.set(nullptr);
        TryMoveFocusOutOfOwner(m_pendingFocusMoveState);
    }

    m_pendingFocusMoveFocusLeftOwner = false;
    m_pendingFocusMoveFocusAfterClear = nullptr;
    UpdateFocusedElement();
}

void ViewManager::OnOwnerUnloaded()
{
    ReleaseAncestorPinsOfClearedFocusedElement();
    m_pendingFocusMoveElement.set(nullptr);
    m_pendingFocusMoveFocusLeftOwner = false;
    m_pendingFocusMoveFocusAfterClear = nullptr;
    m_pendingFocusMoveInterimElement = nullptr;
}

// Returns the direct child of the owner that has focus or contains the focused element.
winrt::UIElement ViewManager::GetFocusedChild()
{
    const auto owner = static_cast<winrt::DependencyObject>(*m_owner);
    const auto xamlRoot = static_cast<winrt::UIElement>(*m_owner).XamlRoot();
    if (!xamlRoot)
    {
        return nullptr;
    }

    auto child = winrt::FocusManager::GetFocusedElement(xamlRoot).try_as<winrt::DependencyObject>();
    while (child)
    {
        auto parent = CachedVisualTreeHelpers::GetParent(child);
        if (parent == owner)
        {
            return child.try_as<winrt::UIElement>();
        }
        child = parent;
    }

    return nullptr;
}

// Moves focus to the realized element nearest to the cleared index (a descendant of it if the element is not focusable).
// On a tie, the element that now has the cleared index or follows it wins, because after a removal the next item takes
// the removed item's index. focusedChild (optional) is the child that contains focus; if the layout reused it, focus is
// kept in it. Returns true if focus moved or was kept; focusedCandidate (optional) is then the element that focus was moved to
// or kept in (a GettingFocus handler can have moved focus elsewhere).
bool ViewManager::TryFocusRealizedChild(int clearedIndex, winrt::FocusState focusState, const winrt::UIElement& focusedChild, winrt::UIElement* focusedCandidate)
{
    std::vector<std::pair<int, winrt::UIElement>> candidates;
    const auto children = m_owner->Children();
    for (unsigned i = 0u; i < children.Size(); ++i)
    {
        const auto child = children.GetAt(i);
        const auto virtInfo = ItemsRepeater::TryGetVirtualizationInfo(child);
        if (virtInfo && virtInfo->IsHeldByLayout())
        {
            candidates.emplace_back(virtInfo->Index(), child);
        }
    }

    std::stable_sort(candidates.begin(), candidates.end(),
        [clearedIndex](const auto& left, const auto& right)
        {
            const int64_t leftDistance = std::abs(static_cast<int64_t>(left.first) - clearedIndex);
            const int64_t rightDistance = std::abs(static_cast<int64_t>(right.first) - clearedIndex);
            if (leftDistance != rightDistance)
            {
                return leftDistance < rightDistance;
            }
            return left.first > right.first;
        });

    for (const auto& candidate : candidates)
    {
        // The layout reused the focused element for this item. Keep focus in it.
        if ((focusedChild && candidate.second == focusedChild && TryKeepFocusInReusedElement(focusedChild)) ||
            TryFocusChild(candidate.second, candidate.first < clearedIndex /* focusLastDescendant */, focusState))
        {
            if (focusedCandidate)
            {
                *focusedCandidate = candidate.second;
            }
            return true;
        }
    }

    return false;
}

// The layout reused the element that contains focus for another item. Focus can be on an element that a nested repeater
// inside it cleared when the element got its new item (the element holds a new nested source). Returns true if focus is
// on a realized element of every nested repeater, or if the nested repeater moved focus to one of its realized elements.
bool ViewManager::TryKeepFocusInReusedElement(const winrt::UIElement& element)
{
    const auto xamlRoot = static_cast<winrt::UIElement>(*m_owner).XamlRoot();
    auto current = xamlRoot ? winrt::FocusManager::GetFocusedElement(xamlRoot).try_as<winrt::DependencyObject>() : nullptr;
    while (current && current != static_cast<winrt::DependencyObject>(element))
    {
        const auto parent = CachedVisualTreeHelpers::GetParent(current);
        if (const auto nestedRepeater = parent.try_as<winrt::ItemsRepeater>())
        {
            const auto currentAsElement = current.try_as<winrt::UIElement>();
            const auto virtInfo = currentAsElement ? ItemsRepeater::TryGetVirtualizationInfo(currentAsElement) : nullptr;
            if (virtInfo && !virtInfo->IsRealized())
            {
                return winrt::get_self<ItemsRepeater>(nestedRepeater)->ViewManager().TryMovePendingFocusToRealizedChild();
            }
        }
        current = parent;
    }

    return true;
}

// Called by an ancestor repeater that reused the element containing this repeater while focus is still on an element
// that this repeater cleared. Moving focus out of this repeater is left to the ancestor, which tries its other items.
bool ViewManager::TryMovePendingFocusToRealizedChild()
{
    if (!m_pendingFocusMoveElement)
    {
        return false;
    }

    const int clearedIndex = m_pendingFocusMoveIndex;
    const auto focusState = m_pendingFocusMoveState;
    m_pendingFocusMoveElement.set(nullptr);
    m_pendingFocusMoveFocusLeftOwner = false;
    m_pendingFocusMoveFocusAfterClear = nullptr;
    m_pendingFocusMoveInterimElement = nullptr;
    ReleaseAncestorPinsOfClearedFocusedElement();

    const auto focusedChild = GetFocusedChild();
    const bool focusMoved = TryFocusRealizedChild(clearedIndex, focusState, nullptr /* focusedChild */);

    // As in OnOwnerLayoutUpdated: a GettingFocus handler may have changed the source and cleared the new target. Keep the
    // move pending, so that this repeater moves focus again in its next LayoutUpdated handler, and stop the ancestor.
    const auto newFocusedChild = GetFocusedChild();
    const auto newFocusedChildVirtInfo = newFocusedChild ? ItemsRepeater::TryGetVirtualizationInfo(newFocusedChild) : nullptr;
    if (newFocusedChildVirtInfo && newFocusedChild != focusedChild && !newFocusedChildVirtInfo->IsHeldByLayout())
    {
        m_pendingFocusMoveElement.set(newFocusedChild);
        m_pendingFocusMoveIndex = clearedIndex;
        m_pendingFocusMoveState = focusState;
        m_pendingFocusMoveOnChangeProcessed = false;
        return true;
    }

    UpdateFocusedElement();
    return focusMoved;
}

bool ViewManager::TryFocusChild(const winrt::UIElement& child, bool focusLastDescendant, winrt::FocusState focusState)
{
    if (child.try_as<winrt::Control>() && TryFocus(child, focusState))
    {
        return true;
    }

    // If the container itself is not focusable, find a descendant that is.
    const auto descendant = focusLastDescendant ?
        winrt::FocusManager::FindLastFocusableElement(child) :
        winrt::FocusManager::FindFirstFocusableElement(child);
    if (const auto descendantAsElement = descendant.try_as<winrt::UIElement>())
    {
        return TryFocus(descendantAsElement, focusState);
    }

    return false;
}

// Returns true if focus moved: to the element, or to another element chosen by a GettingFocus handler.
bool ViewManager::TryFocus(const winrt::UIElement& element, winrt::FocusState focusState)
{
    const auto xamlRoot = static_cast<winrt::UIElement>(*m_owner).XamlRoot();
    const auto focusedBefore = xamlRoot ? winrt::FocusManager::GetFocusedElement(xamlRoot).try_as<winrt::DependencyObject>() : nullptr;

    // Since this focus change is due to the focused element getting recycled, don't activate the window.
    if (!element.as<winrt::IUIElementPrivate>().FocusNoActivate(focusState))
    {
        return false;
    }

    // FocusNoActivate also returns true when a GettingFocus or LosingFocus handler cancels the move.
    const auto focusedAfter = xamlRoot ? winrt::FocusManager::GetFocusedElement(xamlRoot).try_as<winrt::DependencyObject>() : nullptr;
    return focusedAfter != focusedBefore || focusedAfter == static_cast<winrt::DependencyObject>(element);
}

// Moves focus to the nearest focusable element outside the owner: the first one that follows the owner in tree order,
// or else the last one that precedes it. The search starts at the owner, not at the focused element: a Tab step from a
// focused element inside a cleared container can stay inside that container. It does not cycle to the start of the
// window, and it stays inside the XamlRoot content, inside the open popup that contains the owner, inside the nearest
// ancestor whose TabFocusNavigation is Cycle, and inside an open SplitView pane that can be light dismissed and has a
// focusable element.
bool ViewManager::TryMoveFocusOutOfOwner(winrt::FocusState focusState)
{
    const auto xamlRoot = static_cast<winrt::UIElement>(*m_owner).XamlRoot();
    if (!xamlRoot)
    {
        return false;
    }

    const auto content = xamlRoot.Content();
    std::vector<winrt::UIElement> popupChildren;
    for (const auto& popup : winrt::VisualTreeHelper::GetOpenPopupsForXamlRoot(xamlRoot))
    {
        if (const auto popupChild = popup.Child())
        {
            popupChildren.push_back(popupChild);
        }
    }

    // The ancestors of the owner up to the search scope, each with its child on the path to the owner.
    std::vector<std::pair<winrt::DependencyObject, winrt::UIElement>> path;
    winrt::UIElement current = *m_owner;
    while (current && current != content && std::find(popupChildren.begin(), popupChildren.end(), current) == popupChildren.end())
    {
        const auto parent = CachedVisualTreeHelpers::GetParent(current);
        if (!parent)
        {
            break;
        }

        // An open SplitView pane that can be light dismissed keeps Tab focus inside it when it has a focusable element
        // (CSplitView::ProcessTabStop), so focus does not leave the pane then.
        if (const auto splitView = parent.try_as<winrt::SplitView>(); splitView && splitView.IsPaneOpen() &&
            (splitView.DisplayMode() == winrt::SplitViewDisplayMode::Overlay || splitView.DisplayMode() == winrt::SplitViewDisplayMode::CompactOverlay))
        {
            const auto pane = splitView.Pane();
            const auto paneLevel = std::find_if(path.begin(), path.end(), [&pane](const auto& level) { return pane && level.second == pane; });
            if (paneLevel != path.end() && winrt::FocusManager::FindFirstFocusableElement(paneLevel->first))
            {
                path.erase(paneLevel + 1, path.end());
                break;
            }
        }

        if (const auto parentRepeater = parent.try_as<winrt::ItemsRepeater>())
        {
            if (winrt::get_self<ItemsRepeater>(parentRepeater)->ViewManager().IsFocusMovePendingFor(current))
            {
                // An ancestor repeater cleared the element that contains the owner, and moves focus to one of its own
                // items after its layout pass. Moving focus from here could leave that repeater although it has items.
                return false;
            }
        }

        path.emplace_back(parent, current);

        // Tab wraps around inside an ancestor whose TabFocusNavigation is Cycle, so focus does not leave it.
        if (const auto parentAsElement = parent.try_as<winrt::UIElement>(); parentAsElement && parentAsElement.TabFocusNavigation() == winrt::KeyboardNavigationMode::Cycle)
        {
            break;
        }

        current = parent.try_as<winrt::UIElement>();
    }

    for (const bool forward : { true, false })
    {
        for (const auto& [parent, child] : path)
        {
            const auto siblings = GetChildrenInTabOrder(parent);
            const auto position = std::find(siblings.begin(), siblings.end(), child);
            if (position == siblings.end())
            {
                continue;
            }

            if (forward)
            {
                for (auto sibling = position + 1; sibling != siblings.end(); ++sibling)
                {
                    if (TryFocusChild(*sibling, false /* focusLastDescendant */, focusState))
                    {
                        return true;
                    }
                }
            }
            else
            {
                for (auto sibling = std::make_reverse_iterator(position); sibling != siblings.rend(); ++sibling)
                {
                    if (TryFocusChild(*sibling, true /* focusLastDescendant */, focusState))
                    {
                        return true;
                    }
                }

                // A focusable parent comes before its children in tab order.
                if (const auto parentAsControl = parent.try_as<winrt::Control>(); parentAsControl && TryFocus(parentAsControl, focusState))
                {
                    return true;
                }
            }
        }
    }

    return false;
}

// The children of the parent in tab order. An ItemsRepeater lists only its realized children, sorted by index.
std::vector<winrt::UIElement> ViewManager::GetChildrenInTabOrder(const winrt::DependencyObject& parent)
{
    std::vector<winrt::UIElement> children;
    if (const auto repeater = parent.try_as<winrt::ItemsRepeater>())
    {
        if (const auto realizedChildren = winrt::get_self<ItemsRepeater>(repeater)->GetChildrenInTabFocusOrder())
        {
            for (const auto& child : realizedChildren)
            {
                if (const auto childAsElement = child.try_as<winrt::UIElement>())
                {
                    children.push_back(childAsElement);
                }
            }
        }
    }
    else
    {
        const int count = winrt::VisualTreeHelper::GetChildrenCount(parent);
        for (int i = 0; i < count; ++i)
        {
            if (const auto childAsElement = winrt::VisualTreeHelper::GetChild(parent, i).try_as<winrt::UIElement>())
            {
                children.push_back(childAsElement);
            }
        }
    }

    return children;
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
    // A focus move recorded by an earlier Remove or Replace targets an item position; keep it on the same position.
    // After a Reset the positions are those of a new item list, so the index is no longer shifted.
    if (m_pendingFocusMoveElement && m_pendingFocusMoveIndexTracksItem)
    {
        m_pendingFocusMoveIndex = ShiftIndexForCollectionChange(m_pendingFocusMoveIndex, args);
    }
    if (m_clearedFocusedElement && m_clearedFocusedElementIndexTracksItem)
    {
        m_clearedFocusedElementIndex = ShiftIndexForCollectionChange(m_clearedFocusedElementIndex, args);
    }
    if (args.Action() == winrt::NotifyCollectionChangedAction::Reset || args.Action() == winrt::NotifyCollectionChangedAction::Move)
    {
        m_pendingFocusMoveIndexTracksItem = false;
        m_clearedFocusedElementIndexTracksItem = false;
    }

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

        // Pinned elements of replaced items show old data.
        ClearPinnedElementsForCollectionChange(oldStartIndex, oldCount);

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

        // Pinned elements of removed items are cleared before the loop below shifts the indices.
        ClearPinnedElementsForCollectionChange(oldStartIndex, oldCount);

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
        // The indices of pinned elements are not valid anymore, and they are looked up by index.
        ClearPinnedElementsForCollectionChange();

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

void ViewManager::ClearPinnedElementsForCollectionChange(int startIndex, int count)
{
    // The layout does not hold pinned elements, so it does not clear them when their items change.
    std::vector<winrt::UIElement> elementsToClear;
    for (const auto& elementInfo : m_pinnedPool)
    {
        const int index = elementInfo.VirtualizationInfo()->Index();
        if (index >= startIndex && index - startIndex < count)
        {
            elementsToClear.push_back(elementInfo.PinnedElement());
        }
    }

    for (const auto& element : elementsToClear)
    {
        // Goes through ClearElement, which removes the element from the pinned pool because a collection change is
        // being processed, and through ViewportManager::OnElementCleared.
        m_owner->ClearElementImpl(element);
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

    if (m_clearedFocusedElement && element == m_clearedFocusedElement.get())
    {
        // Focus returned to the cleared element that is reused now, before the focus events ran (see OnFocusChanged).
        TryRecordFocusReturnedToClearedElement(element);
    }

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
        const bool isFocusedElement = m_lastFocusedElement == element;
        const bool containsNestedFocus = ContainsNestedFocus(element, virtInfo);
        const auto lastFocusedElementBeforeFocusLeft = m_lastFocusedElement ? nullptr : m_lastFocusedElementBeforeFocusLeft.get();
        if (isFocusedElement)
        {
            // The focus pins of the ancestor repeaters' elements are released when the pending focus move runs (see
            // ClearElementToElementFactory). MoveOwnershipToAnimator resets the element's own pin.
            ++m_pendingAncestorUnpinCount;
        }

        virtInfo->MoveOwnershipToAnimator();
        if (isFocusedElement || containsNestedFocus)
        {
            OnFocusedElementCleared(element, clearedIndex);
        }

        if (isFocusedElement || containsNestedFocus || lastFocusedElementBeforeFocusLeft == element)
        {
            m_clearedFocusedElement = winrt::make_weak(element);
            m_clearedFocusedElementIndex = AdjustClearedIndexForCollectionChange(clearedIndex);
            m_clearedFocusedElementIndexTracksItem = ClearedIndexTracksItem();
        }
    }
    return cleared;
}

bool ViewManager::ClearElementToPinnedPool(const winrt::UIElement& element, const winrt::com_ptr<VirtualizationInfo>& virtInfo, bool isClearedDueToCollectionChange)
{
    // Elements that are already in the pinned pool are handled by ClearElement.
    MUX_ASSERT(!virtInfo->IsInPinnedPool());
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
        // Release the pins of a cleared focused element first: an ancestor element that both pin keeps one pin.
        ReleaseAncestorPinsOfClearedFocusedElement();

        if (m_lastFocusedElement)
        {
            UpdatePin(m_lastFocusedElement.get(), false /* addPin */);
            if (!focusedElement)
            {
                m_lastFocusedElementBeforeFocusLeft = winrt::make_weak(m_lastFocusedElement.get());
            }
        }

        if (focusedElement)
        {
            UpdatePin(focusedElement, true /* addPin */);
            m_lastFocusedElementBeforeFocusLeft = nullptr;
        }

        m_lastFocusedElement.set(focusedElement);
    }
}

void ViewManager::OnFocusChanged(const winrt::IInspectable&, const winrt::RoutedEventArgs&)
{
    UpdateFocusedElement();
    if (m_clearedFocusedElement && !m_pendingFocusMoveElement && !m_lastFocusedElement)
    {
        if (const auto focusedChild = GetFocusedChild(); focusedChild && TryRecordFocusReturnedToClearedElement(focusedChild))
        {
            // Make sure that a layout pass follows, so that the move runs in the LayoutUpdated handler.
            static_cast<winrt::UIElement>(*m_owner).InvalidateArrange();
        }
    }
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
