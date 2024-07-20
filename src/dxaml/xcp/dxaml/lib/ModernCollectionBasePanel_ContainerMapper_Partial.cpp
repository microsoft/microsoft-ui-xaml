// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ModernCollectionBasePanel.g.h"
#include "UIElementCollection.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

#undef min
#undef max

ModernCollectionBasePanel::ContainerManager::ContainerManager(_In_ ModernCollectionBasePanel* owner)
    : m_owner(owner)    // no ref needed, containermapper is not at all exposed to the outside world
    , m_focusedContainer()
    , m_focusedHeaderContainer()
    , m_unlinkedFillers()
    , m_preparationHoldingArea()
    , m_indexOfGroupBeingReset(-1)
    , m_wasLastScrollIntoViewForHeader(false)
    , m_isScrollIntoViewPending (false)
    , m_isScrollIntoViewInProgress(false)
{
    m_indicesOfFirstValidElement.fill(-1);
    m_lastFoundIndexElements.fill(-1);
}

ModernCollectionBasePanel::ContainerManager::~ContainerManager()
{

}

// naming confusion: for long we have shipped with a Refresh that did the work of what I would consider
// a reset. This has seeped into public api, and therefore I can't rectify that. So understand these definitions:
// *******    Refresh:   destructive, time consuming
// *******    Reset:     maintain state as much as possible. We can't trust our concept of indices anymore and want to redo them
_Check_return_ HRESULT ModernCollectionBasePanel::ContainerManager::Refresh()
{
    // Invalidate all the container offets, as if we were recycling all
    IFC_RETURN(RemoveAllValidContainers());
    IFC_RETURN(RemoveAllValidHeaders());

    ClearPinAndFocus();

    return S_OK;
}

// immediately clears pin and focused container without prejudice
void ModernCollectionBasePanel::ContainerManager::ClearPinAndFocus()
{
    // Remove the pinned containers
    m_pinnedElements[xaml_controls::ElementType_GroupHeader].clear();
    m_pinnedElements[xaml_controls::ElementType_ItemContainer].clear();

    // we remember when we want to recycle a focused container
    // but we start off without one.
    m_focusedContainer = PinnedElementInfo();
}

// naming confusion: for long we have shipped with a Refresh that did the work of what I would consider
// a reset. This has seeped into public api, and therefore I can't rectify that. So understand these definitions:
// *******    Refresh:   destructive, time consuming
// *******    Reset:     maintain state as much as possible. We can't trust our concept of indices anymore and want to redo them
_Check_return_ HRESULT ModernCollectionBasePanel::ContainerManager::Reset()
{
    HRESULT hr = S_OK;

    // a refresh really means that indices are no longer valid. but that does not mean that the link between
    // container and item needs to be lost.

    bool isGroupHeader = false;
    bool found = false;
    unsigned int newItemIndex = 0;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;

    PinnedElementInfo focusedContainerInfo = m_focusedContainer;
    auto focusedContainer = m_focusedContainer.GetElement();
    // temporary, so that the pinned containers do not also include m_focusedContainer when we ask for 'isPinned'
    m_focusedContainer = PinnedElementInfo();

    IFC(m_owner->m_cacheManager.GetChildren(&spChildren));

    // if we ever get a a new elementtype, we probably need to update this code
    ASSERT(ElementType_Count == 2);
    for (int typeindex = 0; typeindex < ElementType_Count; ++typeindex)
    {
        xaml_controls::ElementType elementType = static_cast<xaml_controls::ElementType>(typeindex);

        auto& pinnedContainers = m_pinnedElements[elementType];
        auto iter = pinnedContainers.begin();

        // update and remove as we go
        while (iter != pinnedContainers.end())
        {
            PinnedElementInfo info = *iter;
            ctl::ComPtr<IInspectable> spDataItem;
            ctl::ComPtr<IUIElement> oldContainer = info.GetElement();
            ctl::ComPtr<IUIElement> newContainer;

            const bool isPinnedContainerAlsoFocused = (oldContainer == focusedContainer);
            if (isPinnedContainerAlsoFocused)
            {
                // Clear focusedContainerInfo to avoid a double reset on this container.
                focusedContainerInfo = PinnedElementInfo();
            }

            IFC(Interface_DataItemFromElement(elementType, oldContainer.Cast<UIElement>(), &spDataItem));
            IFC(m_owner->m_cacheManager.GetIndex(spDataItem.Get(), &newItemIndex, &isGroupHeader, &found));
            if (found)
            {
                ctl::WeakRefPtr wrElement;
                wf::Size measureSize;

                // get rid of it since recycling will not work if we are still pinned
                pinnedContainers.erase(iter);
                IFC(m_owner->m_spLayoutStrategy->GetElementMeasureSize(
                    elementType,
                    newItemIndex,
                    m_owner->m_windowState.GetRealizationWindow(),
                    &measureSize));

                if (isGroupHeader)
                {
                    IFC(m_owner->RecycleHeaderImpl(info.GetElement().Get()));
                }
                else
                {
                    IFC(m_owner->RecycleContainerImpl(info.GetElement().Get()));
                }

                IFC(m_owner->GenerateElementAtDataIndex(elementType, newItemIndex, &newContainer));

                info.UpdateIndex(newItemIndex);
                if (info.GetElement().Get() != newContainer.Get())
                {
                    // new container
                    IFC(ctl::ComPtr<IUIElement>(newContainer).AsWeak(&wrElement)); // copy because AsWeak() isn't const-correct
                    info.UpdateElement(std::move(wrElement));

                    // items need to be in the visual tree to be measured correctly
                    // however, this does not mean they have to be in the valid part.
                    IFC(PlaceInGarbageRegionIfNotInChildren(newContainer));
                }

                IFC(m_owner->PrepareElementViaItemsHost(elementType, newItemIndex, measureSize, newContainer));

                // reinsert the pin at the same location
                pinnedContainers.insert(iter, info);

                if (isPinnedContainerAlsoFocused)
                {
                    // We finished the reset of the pinned container.
                    // It's present in the new data source.
                    // Make sure we also update m_focusedContainer if
                    // this pinned container is also focused.
                    m_focusedContainer = std::move(info);
                }

                ++iter;
            }
            else
            {
                // in new datasource, apparently this doesn't exist anymore
                iter = pinnedContainers.erase(iter);

                IFC(ElementUnpinned(
                    elementType,
                    info.GetIndex(),
                    info.GetElement(),
                    TRUE /* removeNonGeneratedContainer */));

                // let it recycle
                if (elementType == xaml_controls::ElementType::ElementType_GroupHeader)
                {
                    IFC(m_owner->RecycleHeaderImpl(info.GetElement().Get()));
                }
                else
                {
                    IFC(m_owner->RecycleContainerImpl(info.GetElement().Get()));
                }
            }
        }
    }

    // focused containers are not necessarily in the pinned container collection, but do act as though they are pinned
    if (focusedContainerInfo.GetIndex() > -1)
    {
        ctl::ComPtr<IInspectable> spDataItem;
        IFC(Interface_DataItemFromElement(xaml_controls::ElementType_ItemContainer, focusedContainerInfo.GetElement().AsOrNull<IDependencyObject>().Get(), &spDataItem));
        IFC(m_owner->m_cacheManager.GetIndex(spDataItem.Get(), &newItemIndex, &isGroupHeader, &found));
        if (found)
        {
            if (GetIsContainerPinned(focusedContainerInfo.GetElement().Get()))
            {
                // Focused container is also the pinned container. We already took care
                // of pinned containers, so just update the index of the focused container.
                focusedContainerInfo.UpdateIndex(newItemIndex);
            }
            else
            {
                ctl::WeakRefPtr wrElement;
                ctl::ComPtr<IUIElement> spContainer;
                wf::Size measureSize;

                IFC(m_owner->m_spLayoutStrategy->GetElementMeasureSize(
                    xaml_controls::ElementType_ItemContainer,
                    newItemIndex,
                    m_owner->m_windowState.GetRealizationWindow(),
                    &measureSize));

                IFC(m_owner->RecycleLinkedContainer(focusedContainerInfo.GetElement().Get()));
                IFC(m_owner->GenerateContainerAtIndex(newItemIndex, &spContainer));
                focusedContainerInfo.UpdateIndex(newItemIndex);

                if (focusedContainerInfo.GetElement().Get() != spContainer.Get())
                {
                    // new container
                    IFC(ctl::ComPtr<IUIElement>(spContainer).AsWeak(&wrElement)); // copy because AsWeak() isn't const-correct
                    focusedContainerInfo.UpdateElement(std::move(wrElement));

                    // items need to be in the visual tree to be measured correctly
                    // however, this does not mean they have to be in the valid part.
                    IFC(PlaceInGarbageRegionIfNotInChildren(spContainer));
                }

                IFC(m_owner->PrepareElementViaItemsHost(xaml_controls::ElementType_ItemContainer, newItemIndex, measureSize, spContainer));
            }

            m_focusedContainer = focusedContainerInfo;
        }
        else
        {
            // in new datasource, apparently this doesn't exist anymore
            IFC(m_owner->RecycleLinkedContainer(focusedContainerInfo.GetElement().Get()));
        }
    }

Cleanup:
    RRETURN(hr);
}

// Given an the index of a valid element (i.e., 0 is always the first container in the valid range), return the element.
_Check_return_ HRESULT ModernCollectionBasePanel::ContainerManager::GetElementAtValidIndex(
    _In_ xaml_controls::ElementType type,
    _In_ INT32 indexInValidElements,
    _Out_ ctl::ComPtr<xaml::IUIElement>* pspElement) const
{
    ctl::WeakRefPtr temp; // Much of COM isn't thread safe, so we have to create a temp to resolve

    ASSERT(IsValidElementIndexWithinBounds(type, indexInValidElements));

    temp = m_validElements[type][indexInValidElements];
    RRETURN(temp.As(pspElement));
}

_Check_return_ HRESULT ModernCollectionBasePanel::ContainerManager::GetElementAtDataIndex(
    _In_ xaml_controls::ElementType type,
    _In_ INT32 dataIndex,
    _Out_ ctl::ComPtr<xaml::IUIElement>* pspElement) const
{
    ctl::WeakRefPtr temp; // Much of COM isn't thread safe, so we have to create a temp to resolve

    const INT32 validIndex = GetValidElementIndexFromDataIndex(type, dataIndex);
    if (IsValidElementIndexWithinBounds(type, validIndex))
    {
        temp = m_validElements[type][validIndex];
    }

    RRETURN(temp.As(pspElement));
}

// Returns True if there is a realized item in the valid collection for the provided index.
bool ModernCollectionBasePanel::ContainerManager::IsItemConnected(_In_ UINT index) const
{
    if (GetValidContainerCount() > 0 &&
        index >= static_cast<UINT>(GetItemIndexFromValidIndex(0)) &&
        index <= static_cast<UINT>(GetItemIndexFromValidIndex(GetValidContainerCount() - 1)))
    {
        ctl::ComPtr<IUIElement> spContainer;
        IFCFAILFAST(GetContainerAtItemIndex(index, &spContainer));
        return spContainer != nullptr;
    }
    return false;
}

// Returns True if the group header for the provided group index is connected.
// neighboringItemIndex is either the last item of the previous visible group or the first item of the provided group.
// It depends on whether the known connected area is above or below the provided group index.
bool ModernCollectionBasePanel::ContainerManager::IsGroupHeaderConnected(_In_ UINT groupIndex, _In_ UINT neighboringItemIndex) const
{
    return IsGroupHeaderRealized(groupIndex) && IsItemConnected(neighboringItemIndex);
}

// Returns True if there is a realized group header for the provided index.
bool ModernCollectionBasePanel::ContainerManager::IsGroupHeaderRealized(_In_ UINT groupIndex) const
{
    // Deal with empty groups
    groupIndex = static_cast<UINT>(m_owner->m_cacheManager.DataIndexToLayoutIndex(xaml_controls::ElementType_GroupHeader, groupIndex));
    groupIndex = static_cast<UINT>(m_owner->m_cacheManager.LayoutIndexToDataIndex(xaml_controls::ElementType_GroupHeader, groupIndex));

    // Find out if we've already realized this header, and if not, who the closest header is
    if (GetValidHeaderCount() > 0)
    {
        UINT firstValidGroupIndex = static_cast<UINT>(GetGroupIndexFromValidIndex(0));
        UINT lastValidGroupIndex = static_cast<UINT>(GetGroupIndexFromValidIndex(GetValidHeaderCount() - 1));

        if (groupIndex < firstValidGroupIndex || groupIndex > lastValidGroupIndex)
        {
            return false;
        }
        else
        {
            ctl::ComPtr<IUIElement> spHeader;
            IFCFAILFAST(GetHeaderAtGroupIndex(groupIndex, &spHeader));
            return spHeader != nullptr;
        }
    }
    return false;
}

// Inform the ContainerManager that the index of the first realized element has changed
// to the given value.
void ModernCollectionBasePanel::ContainerManager::UpdateDataIndexForFirstValidElement(_In_ xaml_controls::ElementType type, _In_ INT32 dataIndexOfFirstValidElement)
{
    // in cases where we insert or remove a data item before the collection, the dataIndexes need to reflect the change
    const INT32 difference = dataIndexOfFirstValidElement - m_indicesOfFirstValidElement[type];
    m_indicesOfFirstValidElement[type] = dataIndexOfFirstValidElement;
    std::for_each(begin(m_elementIndicesForChildren[type]), end(m_elementIndicesForChildren[type]),
        [difference](INT32& value)
    {
        value += difference;
    });
}

// Given the element and its data index in the Items(Source), add the element (or move, if it's)
// to the proper place in the Children collection. Also updates the elements's virtualization info to reflect
// that it is realized.
_Check_return_ HRESULT ModernCollectionBasePanel::ContainerManager::PlaceInValidElements(_In_ xaml_controls::ElementType type, _In_ INT32 dataIndex, _In_ const ctl::ComPtr<IUIElement>& spElement)
{
    TracePlaceElementBegin(dataIndex);

    auto guard = wil::scope_exit([]()
    {
        TracePlaceElementEnd();
    });

    ctl::WeakRefPtr weakRef;
    IFC_RETURN(ctl::ComPtr<IUIElement>(spElement).AsWeak(&weakRef));

    // Sanity-check the index so we don't end up corrupting memory
    // Our item index can be up to 1 element beyond the valid range.
    FAIL_FAST_ASSERT(m_validElements[type].empty() ||
        (m_indicesOfFirstValidElement[type] - 1 <= dataIndex && dataIndex <= m_indicesOfFirstValidElement[type] + GetValidElementCount(type)));

    // Figure out if we are inserting a new element in the valid range, or just replacing an existing one
    const bool isValidRangeInsertion =
        m_validElements[type].empty() || // Realizing our first element
        dataIndex == m_indicesOfFirstValidElement[type] - 1 || // Placing at beginning of the range
        dataIndex == m_indicesOfFirstValidElement[type] + GetValidElementCount(type); // Placing at end of the range

    if (isValidRangeInsertion)
    {
        const INT32 indexInValidElements = m_validElements[type].empty() ? 0 : dataIndex - m_indicesOfFirstValidElement[type];

        // If we're inserting just before the valid range, use the "begin" iterator
        auto insertIterator = begin(m_validElements[type]) + std::max(0, indexInValidElements);

        // Note: we are allowed to place a null on the edge here during generation, under the assumption that
        // the panel's generate will clean this sentinel up if another non-sentinel isn't placed after it.
        // However, it doesn't do us any good to have a valid range consisting entirely of a sentinel, so no-op
        if (weakRef || !m_validElements[type].empty())
        {
            m_validElements[type].insert(insertIterator, weakRef);

            // We inserted in front of the range, so update the first item index
            if (indexInValidElements <= 0)
            {
                m_indicesOfFirstValidElement[type] = dataIndex;
            }
        }
    }
    else
    {
        // Just replacing an existing element
        const INT32 indexInValidElements = dataIndex - m_indicesOfFirstValidElement[type];
        ASSERT(0 <= indexInValidElements && indexInValidElements < GetValidElementCount(type));

        ASSERT(!m_validElements[type][indexInValidElements] || !weakRef);

        if (m_validElements[type][indexInValidElements])
        {
            // If we're replacing an existing element with a sentinel, let's pull the old one out of the visual collection
            IFC_RETURN(RemoveFromVisualChildren(
                type,
                dataIndex,
                m_validElements[type][indexInValidElements].AsOrNull<IUIElement>(),
                FALSE /* isForElementRemoval */));
        }

        if (m_validElements[type].size() > 1 || weakRef)
        {
            // If we're placing a valid, non-sentinel, or we have other elements in the valid range
            // Go ahead and place it
            m_validElements[type][indexInValidElements] = weakRef;
        }
        else
        {
            // If we're replacing the only element in the range with a sentinel, let's just blow away the range
            ASSERT(m_validElements[type].size() == 1 && !weakRef);
            ASSERT(m_elementIndicesForChildren[type].empty());
            m_validElements[type].clear();
            m_indicesOfFirstValidElement[type] = -1;
        }
    }

    IFC_RETURN(AddToVisualChildren(type, dataIndex, spElement));

    // Make sure that we either have an empty collection (adding a sentinel to an empty collection should remain empty)
    // or that our range is contiguous and valid
    ASSERT((!spElement && m_validElements[type].empty() && m_elementIndicesForChildren[type].empty()) ||
        (m_indicesOfFirstValidElement[type] <= dataIndex) && (dataIndex < m_indicesOfFirstValidElement[type] + GetValidElementCount(type))); // test for contiguousness

    return S_OK;
}

// Place a sentinel object into the valid element block
_Check_return_ HRESULT ModernCollectionBasePanel::ContainerManager::InsertSentinelElement(_In_ xaml_controls::ElementType type, _In_ INT32 dataIndex)
{
    HRESULT hr = S_OK;

    // We should only be doing this if it needs to go into the interior or edge of the valid block
    ASSERT(m_indicesOfFirstValidElement[type] <= dataIndex && dataIndex <= m_indicesOfFirstValidElement[type] + GetValidElementCount(type));

    m_validElements[type].insert(begin(m_validElements[type]) + GetValidElementIndexFromDataIndex(type, dataIndex), ctl::WeakRefPtr());

    RRETURN(hr);//RRETURN_REMOVAL
}

// Removes an element from the valid range. It is now considered to be junked and we are probably calling this because we are recycling.
// In some cases we are removing a item/group from the itemcollection and we do not want to update the dataindexFromFirstValidElement (itemBeingRemoved).
// Generated elements get moved to the junk sections; ungenerated elements get removed from the children.
// If specified, gateIndex represents the limit of ungenerated elements removal: it is used to ensure that the element being realized
// does not get disjoint from the range of elements after this cleanup phase
_Check_return_ HRESULT ModernCollectionBasePanel::ContainerManager::RemoveFromValidElements(
    _In_ xaml_controls::ElementType type,
    _In_ INT32 indexInValidElements,
    _In_ BOOLEAN isForDataRemoval,
    _In_ INT32 gateIndex)
{
    HRESULT hr = S_OK;

    const INT32 dataIndex = GetDataIndexFromValidIndex(type, indexInValidElements);

    ctl::ComPtr<IUIElement> spElement;

    IFC(GetElementAtValidIndex(type, indexInValidElements, &spElement));
    IFC(RemoveFromVisualChildren(type, dataIndex, spElement, isForDataRemoval));

    // Remove the slot from our realized containers deque
    // Careful now... If we remove from an end, we'll also clean up any nulls left over on that end
    // While cleaning the nulls left over, ensure that we are not trimming too much
    // and making the element being realized and the range disjoints
    // But we're not going to clean up the other end, because it's probably in the middle of generating things
    if (indexInValidElements == 0)
    {
        m_validElements[type].pop_front();
        if (!isForDataRemoval)
        {
            // Only do this check on the first removal
            // The others are placeholders that we are popping off the range
            ++m_indicesOfFirstValidElement[type];
        }
        while (!m_validElements[type].empty() && !m_validElements[type].front()
               && ((gateIndex == -1) || (m_indicesOfFirstValidElement[type] < gateIndex)))
        {
            m_validElements[type].pop_front();
            ++m_indicesOfFirstValidElement[type];
        }
    }
    else if (indexInValidElements == static_cast<INT32>(m_validElements[type].size()) - 1)
    {
        do
        {
            m_validElements[type].pop_back();
        } while (!m_validElements[type].empty() && !m_validElements[type].back()
               && ((gateIndex == -1) || (m_indicesOfFirstValidElement[type] + static_cast<INT32>(m_validElements[type].size() - 1) > gateIndex)));
    }
    else
    {
        m_validElements[type].erase(begin(m_validElements[type]) + indexInValidElements);
    }

    // Update the following indices in the child map
    if (isForDataRemoval)
    {
        std::for_each(
            begin(m_elementIndicesForChildren[type]),
            end(m_elementIndicesForChildren[type]),
            [dataIndex](INT32& element)
        {
            ASSERT(element != dataIndex);
            if (dataIndex < element)
            {
                --element;
            }
        });
    }

    // Since our set of elements is empty, let's make sure our other data is consistent with this state
    if (m_validElements[type].empty())
    {
        m_indicesOfFirstValidElement[type] = -1;
        ASSERT(m_elementIndicesForChildren[type].empty());
    }

Cleanup:
    RRETURN(hr);
}

// Mark all valid containers as junked
_Check_return_ HRESULT ModernCollectionBasePanel::ContainerManager::RemoveAllValidElements(_In_ xaml_controls::ElementType type)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;
    ctl::ComPtr<IUIElement> spChild;

    IFC(m_owner->m_cacheManager.GetChildren(&spChildren));

    // notify that it is no longer of interest
    while (SizeOfVisualSection(type) > 0)
    {
        INT32 childCollectionIndex = StartOfVisualSection(type) + SizeOfVisualSection(type) - 1;
        IFC(spChildren->GetAt(childCollectionIndex, &spChild));
        ModernCollectionBasePanel::SetElementIsRealized(spChild, false);

        bool forceRemove = false;
        if (!ModernCollectionBasePanel::GetElementIsGenerated(spChild))
        {
            forceRemove = !GetIsElementPinned(type, spChild);
        }

        if (type == xaml_controls::ElementType_ItemContainer && forceRemove)
        {
            IFC(spChildren->RemoveAt(childCollectionIndex));
        }
        else
        {
            IFC(spChildren.Cast<UIElementCollection>()->MoveInternal(childCollectionIndex, StartOfGarbageSection()));
        }
        m_elementIndicesForChildren[type].pop_back();
    }

    m_indicesOfFirstValidElement[type] = -1;
    m_validElements[type].clear();
    ASSERT(m_elementIndicesForChildren[type].empty());

Cleanup:
    RRETURN(hr);
}

// Place the container in the garbage section
_Check_return_ HRESULT ModernCollectionBasePanel::ContainerManager::PlaceInGarbageRegionIfNotInChildren(_In_ const ctl::ComPtr<IUIElement>& spElement)
{
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;
    UINT foundIndex;
    boolean found;

    IFC_RETURN(m_owner->m_cacheManager.GetChildren(&spChildren));
    IFC_RETURN(spChildren->IndexOf(spElement.Get(), &foundIndex, &found));
    if (!found)
    {
        IFC_RETURN(spChildren->Append(spElement.Get()));
    }

    return S_OK;
}

// NOTE: This method is much slower than getting elements from the valid ranges, because this is going to access the collection of visual children.
_Check_return_ HRESULT ModernCollectionBasePanel::ContainerManager::GetVisualChild(_In_ INT visualChildIndex, _Out_ ctl::ComPtr<xaml::IUIElement>* pspElement)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;
    ctl::ComPtr<IUIElement> spChild;

    IFC(m_owner->m_cacheManager.GetChildren(&spChildren));
    IFC(spChildren->GetAt(visualChildIndex, &spChild));

    *pspElement = std::move(spChild);

Cleanup:
    RRETURN(hr);
}

// Trim any leftover edges for both headers and containers
// Wrapped up so clients don't have to manually handle both types
void ModernCollectionBasePanel::ContainerManager::TrimEdgeSentinels()
{
    TrimEdgeSentinels(xaml_controls::ElementType_GroupHeader);
    TrimEdgeSentinels(xaml_controls::ElementType_ItemContainer);
}

// Cleans up remaining edge sentinels for one type of element
void ModernCollectionBasePanel::ContainerManager::TrimEdgeSentinels(_In_ xaml_controls::ElementType type)
{
    // All we're doing here is searching the vectors and erasing elements
    // None of these operations allocate any memory, so we're pretty safe from exceptions

    // Trim the far end
    m_validElements[type].erase(
        // Search backwards from the end of the range, looking for the first non-null ref
        std::find_if(
        m_validElements[type].rbegin(), m_validElements[type].rend(),
        [](const ctl::WeakRefPtr& arg) -> bool { return arg != nullptr; }
    // And erase everything from the element after it to the end
    ).base(), end(m_validElements[type]));

    // Now, let's trim the front ends
    // Find the position of the first non-null element
    auto firstValid = std::find_if(begin(m_validElements[type]), end(m_validElements[type]),
        [](const ctl::WeakRefPtr& arg) -> bool {return arg != nullptr;});
    // Calculate the distance so we can update our first valid index
    auto removalCount = firstValid - begin(m_validElements[type]);
    // Erase the nulls
    m_validElements[type].erase(begin(m_validElements[type]), firstValid);
    // Increment the first valid index
    m_indicesOfFirstValidElement[type] += static_cast<INT32>(removalCount);
}

// Helper to remove a realized element from the visual children
_Check_return_ HRESULT ModernCollectionBasePanel::ContainerManager::RemoveFromVisualChildren(
    _In_ xaml_controls::ElementType type,
    _In_ INT32 index,
    _In_ const ctl::ComPtr<IUIElement>& spElement,
    _In_ BOOLEAN isForElementRemoval)
{
    HRESULT hr = S_OK;

    if (spElement)
    {
        IndexMap& currentMap = m_elementIndicesForChildren[type];
        std::deque<INT32>::const_iterator childCollectionIterator;
        BOOLEAN removed = FALSE;

        // Optimize for the panning case (removing from ends) to avoid unneeded searches
        ASSERT(!currentMap.empty());
        if (index == currentMap.front())
        {
            childCollectionIterator = begin(currentMap);
        }
        else if (index == currentMap.back())
        {
            childCollectionIterator = end(currentMap) - 1;
        }
        else
        {
            childCollectionIterator = std::lower_bound(
                begin(currentMap),
                end(currentMap),
                index);
        }

        // If we have a valid element, this index should exist in the map
        ASSERT(childCollectionIterator != end(currentMap) &&
            *childCollectionIterator == index);
        const INT32 childIndex = StartOfVisualSection(type) + static_cast<INT32>(childCollectionIterator - begin(currentMap));

        ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;
        IFC(m_owner->m_cacheManager.GetChildren(&spChildren));

#ifdef DBG
        {
            // If our maps get out of sync, we are in big trouble, so let's do some generous sanity-checking here against the actual visual children
            UINT32 foundIndex = 0;
            BOOLEAN found = FALSE;
            IFC(spChildren->IndexOf(spElement.Get(), &foundIndex, &found));
            ASSERT(found);
            ASSERT(StartOfVisualSection(type) <= static_cast<INT32>(foundIndex) &&
                static_cast<INT32>(foundIndex) < EndOfVisualSection(type));
            ASSERT(foundIndex == childIndex);
        }
#endif

        auto isPinned = GetIsElementPinned(type, spElement);

        // Handle non-generated containers differently.
        // If it is not pinned nor focused, remove it from the tree.
        // Otherwise put it in garbage section.  It will be removed from visual tree when it is unpinned/loses focus.

        // Containers have a special case for transitions and items that are their own container
        // Item containers are removed from the children collection to show their remove transition.
        bool removeItemContainerFromChildrenCollection =
            type == xaml_controls::ElementType_ItemContainer &&
            (isForElementRemoval || (!ModernCollectionBasePanel::GetElementIsGenerated(spElement) && !isPinned));

        // As for group headers, to avoid infinite growth of the garbage section, we limit the number of headers in there to 16
        // or 4 times the CacheLength, whichever is larger (note that the default CacheLength is 4).
        // Applications that consistently provide brand new headers via the ChoosingGroupHeaderContainer event will thus not add
        // an ever growing number of visuals in the garbage section for instance.
        bool removeGroupHeaderFromChildrenCollection =
            type == xaml_controls::ElementType_GroupHeader &&
            ModernCollectionBasePanel::GetElementIsGenerated(spElement) && !isPinned;

        if (removeGroupHeaderFromChildrenCollection)
        {
            const UINT maxHeadersInGarbageSection = std::max(16, static_cast<int>(m_owner->m_cacheLength * 4));
            UINT headersInGarbageSection = 0;
            UINT garbageStart = StartOfGarbageSection();
            UINT childrenCount = 0;

            IFC(spChildren->get_Size(&childrenCount));

            for (UINT garbageIndex = garbageStart; garbageIndex < childrenCount; ++garbageIndex)
            {
                ctl::ComPtr<IUIElement> currentElement;

                IFC(spChildren->GetAt(garbageIndex, &currentElement));

                if (GetElementIsHeader(currentElement) == xaml_controls::ElementType_GroupHeader)
                {
                    headersInGarbageSection++;

                    if (headersInGarbageSection == maxHeadersInGarbageSection)
                    {
                        break;
                    }
                }
            }

            removeGroupHeaderFromChildrenCollection = headersInGarbageSection == maxHeadersInGarbageSection;
        }

        if (removeItemContainerFromChildrenCollection || removeGroupHeaderFromChildrenCollection)
        {
            IFC(spChildren.Cast<UIElementCollection>()->RemoveAt(childIndex));
            removed = TRUE;
        }
        else
        {
            // Move it to the junk section
            IFC(spChildren.Cast<UIElementCollection>()->MoveInternal(childIndex, StartOfGarbageSection()));
        }

        if (!isPinned ||
            removed)
        {
            // Keep pinned elements which are still in visual tree realized until they are unpinned.
            // This is to keep rendering Chrome when dragging an element.
            ModernCollectionBasePanel::SetElementIsRealized(spElement, false);
        }

        // Remove the entry from the child index map
        currentMap.erase(childCollectionIterator);
    }

Cleanup:
    RRETURN(hr);
}

// Perform cleanup once element is unpinned and outside of valid window.
_Check_return_ HRESULT ModernCollectionBasePanel::ContainerManager::ElementUnpinned(
    _In_ xaml_controls::ElementType type,
    _In_ INT32 index,
    _In_ const ctl::ComPtr<IUIElement>& spElement,
    _In_ BOOLEAN removeNonGeneratedContainer)
{
    HRESULT hr = S_OK;

    ASSERT(spElement.Get());

    // Element can still be pinned since there are two reasons for pinning (focus + explicit pin).
    if (!GetIsElementPinned(type, spElement) &&
        m_owner->m_windowState.validWindowCalculation &&
        !IsValidElementIndexWithinBounds(type, GetValidElementIndexFromDataIndex(type, index)))
    {
        if (removeNonGeneratedContainer &&
            !ModernCollectionBasePanel::GetElementIsGenerated(spElement))
        {
            // This element was moved to garbage area of visual tree to keep its focus.
            // Now it's time to remove it.
            ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;
            UINT32 foundIndex = 0;
            BOOLEAN found = FALSE;

            IFC(m_owner->m_cacheManager.GetChildren(&spChildren));
            IFC(spChildren->IndexOf(spElement.Get(), &foundIndex, &found));
            ASSERT(found);
            IFC(spChildren.Cast<UIElementCollection>()->RemoveAt(foundIndex));

            // to avoid re-parenting we should mark NonGeneratedElement that it did't leave the tree in current layout cycle.
            IFC(CoreImports::UIElement_SetIsLeaving(static_cast<CUIElement*>(spElement.Cast<UIElement>()->GetHandle()), FALSE));
        }

        // For all cases, disable rendering Chrome on once it's not within valid range.
        ModernCollectionBasePanel::SetElementIsRealized(spElement, false);
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ModernCollectionBasePanel::ContainerManager::AddToVisualChildren(
    _In_ xaml_controls::ElementType type,
    _In_ INT32 index,
    _In_ const ctl::ComPtr<IUIElement>& spElement)
{
    HRESULT hr = S_OK;

    if (spElement)
    {
        IndexMap& currentMap = m_elementIndicesForChildren[type];
        std::deque<INT32>::const_iterator childCollectionIterator;

        // Optimize for the panning case (adding to ends) to avoid unneeded searches
        if (currentMap.empty() || index < currentMap.front())
        {
            childCollectionIterator = begin(currentMap);
        }
        else if (index > currentMap.back())
        {
            childCollectionIterator = end(currentMap);
        }
        else
        {
            childCollectionIterator = std::lower_bound(
                begin(currentMap), end(currentMap), index);
        }

        // If we aren't processing a newly-inserted element, then we shouldn't already have an element for this index
        ASSERT(childCollectionIterator == end(currentMap) || *childCollectionIterator != index);

        const INT32 destinationChildIndex = StartOfVisualSection(type) + static_cast<INT32>(childCollectionIterator - begin(currentMap));

        UINT32 foundIndex = 0;
        BOOLEAN found = FALSE;
        ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;

        IFC(m_owner->m_cacheManager.GetChildren(&spChildren));

        // TODO: This is a relatively slow linear search. This could be faster if we had knowledge
        // of the recycle queue's contents
        IFC(spChildren->IndexOf(spElement.Get(), &foundIndex, &found));

        // Make sure our caller isn't trying to reinsert an existing container
        ASSERT(!found || foundIndex >= static_cast<UINT>(StartOfGarbageSection()));

        // mark as interesting
        ModernCollectionBasePanel::SetElementIsRealized(spElement, true);

        if (!found)
        {
            IFC(spChildren->InsertAt(destinationChildIndex, spElement.Get()));
        }
        else
        {
            IFC(spChildren.Cast<UIElementCollection>()->MoveInternal(foundIndex, destinationChildIndex));
            IFC(CoreImports::UIElement_SetIsEntering(static_cast<CUIElement*>(spElement.Cast<UIElement>()->GetHandle()), TRUE));
        }
        currentMap.insert(childCollectionIterator, index);
    }

Cleanup:
    RRETURN(hr);
}

//////////////////////////////////////////////////////////////////////////

ModernCollectionBasePanel::ContainerManager::PinnedElementInfo::PinnedElementInfo(
    _Inout_ ctl::WeakRefPtr&& wrElement, _In_ INT index)
    : m_wrElement(std::move(wrElement))
    , m_index(index)
{
}

ModernCollectionBasePanel::ContainerManager::PinnedElementInfo::PinnedElementInfo(
    _In_ const ctl::WeakRefPtr& wrElement, _In_ INT index)
    : m_wrElement(wrElement)
    , m_index(index)
{
}

//////////////////////////////////////////////////////////////////////////
_Check_return_ HRESULT
    ModernCollectionBasePanel::ContainerManager::Interface_DataItemFromElement(
    _In_ xaml_controls::ElementType type,
    _In_ xaml::IDependencyObject* element,
    _Outptr_ IInspectable** ppReturnValue )
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IUIElement> spElementAsUI = ctl::query_interface_cast<IUIElement>(element);

    IFCPTR(ppReturnValue);
    *ppReturnValue = nullptr;

    // Convert to UIElement and inspect its virtualization information
    if (spElementAsUI)
    {
        // Check the VirtualizationInfo. If somebody hands us a container we had nothing to do with
        // we should probably just return nullptr, rather than AV.
        UIElement::VirtualizationInformation* pInfo = GetVirtualizationInformationFromElement(spElementAsUI);
        if (pInfo)
        {
            IFC(pInfo->GetItem().MoveTo(ppReturnValue));
        }
    }
Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ModernCollectionBasePanel::ContainerManager::Interface_IndexFromElement(
    _In_ xaml_controls::ElementType type,
    _In_ xaml::IDependencyObject* element,
    _In_ BOOLEAN excludeHiddenEmptyGroups,
    _Out_ INT* pReturnValue )
{
    INT returnValue = -1;

    auto forward_iterator = m_validElements[type].begin();   // not using begin(m_valid..) notation for symmetry
    auto reverse_iterator = m_validElements[type].rbegin();

    IFCPTR_RETURN(pReturnValue);
    *pReturnValue = -1;

    // First, check the preparation holding area, as we expect this to match frequently, and it saves us from
    // doing a linear search on the realized element set
    if (m_preparationHoldingArea.Valid() &&
        type == m_preparationHoldingArea.m_type &&
        element == m_preparationHoldingArea.m_element.AsOrNull<IDependencyObject>().Get())
    {
        returnValue = m_preparationHoldingArea.m_index;
    }
    else
    {
        // perf optimization: we usually call index from element on elements adjacent to eachother.
        // so do a 'fanning' out search.

        INT32 lastFoundAtIndex = m_lastFoundIndexElements[type];
        INT64 foundAtIndex = -1;
        if (lastFoundAtIndex < 0 || lastFoundAtIndex >= static_cast<INT32>(m_validElements[type].size()))
        {
            // nothing of use here
            lastFoundAtIndex = 0;
        }

        forward_iterator += lastFoundAtIndex;
        reverse_iterator += m_validElements[type].size() - lastFoundAtIndex;

        while (forward_iterator != m_validElements[type].end() || reverse_iterator != m_validElements[type].rend())
        {
            if (forward_iterator != m_validElements[type].end())
            {
                if ((*forward_iterator).AsOrNull<IDependencyObject>().Get() == element)
                {
                    foundAtIndex = forward_iterator - m_validElements[type].begin();
                    break;
                }
                ++forward_iterator;
            }
            if (reverse_iterator != m_validElements[type].rend())
            {
                if ((*reverse_iterator).AsOrNull<IDependencyObject>().Get() == element)
                {
                    foundAtIndex = m_validElements[type].rend() - reverse_iterator -1;
                    break;
                }
                ++reverse_iterator;
            }
        }

        m_lastFoundIndexElements[type] = static_cast<INT32>(foundAtIndex);

        if (foundAtIndex > -1)
        {
            // in the regular valid elements section
            returnValue = GetDataIndexFromValidIndex(type, static_cast<INT32>(foundAtIndex));
        }
        else
        {
            // could be in the pinned containers collection
            auto foundPinnedElementInfo = std::find_if(begin(m_pinnedElements[type]), end(m_pinnedElements[type]),
                [element] (const PinnedElementInfo& elem)
            {
                return elem.GetElement().AsOrNull<IDependencyObject>().Get() == element;
            });

            if (foundPinnedElementInfo != end(m_pinnedElements[type]))
            {
                returnValue = foundPinnedElementInfo->GetIndex();
            }

            // maybe through focused?
            if (type == xaml_controls::ElementType_ItemContainer)
            {
                if(m_focusedContainer.GetIndex() > -1 && m_focusedContainer.GetElement().AsOrNull<IDependencyObject>().Get() == element)
                {
                    returnValue = m_focusedContainer.GetIndex();
                }
            }
        }
    }

    if (excludeHiddenEmptyGroups)
    {
        returnValue = static_cast<UINT>(m_owner->m_cacheManager.DataIndexToLayoutIndex(type, returnValue));
    }

    *pReturnValue = returnValue;

    return S_OK;
}

_Check_return_ HRESULT
    ModernCollectionBasePanel::ContainerManager::Interface_ElementFromDataIndex(
    _In_ xaml_controls::ElementType type,
    _In_ INT index,
    _Outptr_ xaml::IDependencyObject** ppReturnValue )
{
    ctl::ComPtr<xaml::IUIElement> spElement;
    IFCPTR_RETURN(ppReturnValue);
    *ppReturnValue = nullptr;

    INT32 elementIndex = GetValidElementIndexFromDataIndex(type, index);
    if (IsValidElementIndexWithinBounds(type, elementIndex))
    {
        IFC_RETURN(GetElementAtValidIndex(type, elementIndex, &spElement));
    }
    else
    {
        if (m_preparationHoldingArea.Valid() &&
            type == m_preparationHoldingArea.m_type &&
            index == m_preparationHoldingArea.m_index)
        {
            spElement = m_preparationHoldingArea.m_element;
        }
        else
        {
            IFC_RETURN(GetElementFromPinnedElements(type, index, &spElement));
        }
    }

    IFC_RETURN(spElement.CopyTo(ppReturnValue));

    return S_OK;
}

_Check_return_ HRESULT
    ModernCollectionBasePanel::ContainerManager::Interface_ElementFromDataItem(
    _In_ xaml_controls::ElementType type,
    _In_opt_ IInspectable* dataItem,
    _Outptr_ xaml::IDependencyObject** ppReturnValue )
{
    HRESULT hr = S_OK;
    bool itemMatch = false;

    IFCPTR(ppReturnValue);
    *ppReturnValue = nullptr;

    // We have an api on IGeneratorHost that allows a listview to give us a suggestion.
    // Let's use it! probably gives us null anyway.
    if (type == xaml_controls::ElementType_ItemContainer)
    {
        ctl::ComPtr<IGeneratorHost> spIHost;
        IFC(m_owner->m_cacheManager.GetItemsHost(&spIHost));
        if (spIHost)
        {
            ctl::ComPtr<IDependencyObject> spSuggested;
            IFC(spIHost->SuggestContainerForContainerFromItemLookup(&spSuggested));
            if (spSuggested && ctl::is<IUIElement>(spSuggested))
            {
                IFC(PropertyValue::AreEqual(GetItemFromElement(spSuggested.AsOrNull<IUIElement>()).Get(), dataItem, &itemMatch));

                if (itemMatch)
                {
                    IFC(spSuggested.CopyTo(ppReturnValue));
                }
            }
        }
    }

    // Look in the preparation holding area first, as this will save us a potentially expensive search
    if (!itemMatch && m_preparationHoldingArea.Valid() &&
        type == m_preparationHoldingArea.m_type)
    {
        IFC(PropertyValue::AreEqual(GetItemFromElement(m_preparationHoldingArea.m_element).Get(), dataItem, &itemMatch));
        if (itemMatch)
        {
            IFC(m_preparationHoldingArea.m_element.CopyTo(ppReturnValue));
        }
    }

    if (!itemMatch)
    {
        // fallback logic (slower)
        auto elementIterator = std::find_if(begin(m_validElements[type]), end(m_validElements[type]),
            [dataItem] (ctl::WeakRefPtr weakRef)
        {
            ctl::ComPtr<IUIElement> spCandidateElement = weakRef.AsOrNull<IUIElement>();
            if (spCandidateElement)
            {
                bool areEqual = false;
                VERIFYHR(PropertyValue::AreEqual(GetItemFromElement(spCandidateElement).Get(), dataItem, &areEqual));
                return areEqual;
            }
            return false;
        });

        if (end(m_validElements[type]) != elementIterator)
        {
            IFC((*elementIterator).CopyTo(ppReturnValue));
        }
        else
        {
            // nothing found, let's not forget the pinned headers
            auto foundPinnedElementInfo = std::find_if(begin(m_pinnedElements[type]), end(m_pinnedElements[type]),
                [dataItem](const PinnedElementInfo& pinnedElement)
            {
                ctl::ComPtr<xaml::IUIElement> element = pinnedElement.GetElement();
                if (element)
                {
                    bool areEqual = false;
                    VERIFYHR(PropertyValue::AreEqual(GetItemFromElement(element).Get(), dataItem, &areEqual));
                    return areEqual;
                }
                else
                {
                    return false;
                }
            });

            if (foundPinnedElementInfo != end(m_pinnedElements[type]))
            {
                IFC(foundPinnedElementInfo->GetElement().CopyTo(ppReturnValue));
            }
            else if (type == xaml_controls::ElementType_ItemContainer)
            {
                if (m_focusedContainer.GetIndex() > -1)
                {
                    ctl::ComPtr<xaml::IUIElement> element = m_focusedContainer.GetElement();
                    if (element)
                    {
                        bool areEqual = false;
                        IFC(PropertyValue::AreEqual(GetItemFromElement(element).Get(), dataItem, &areEqual));
                        if (areEqual)
                        {
                            IFC(element.CopyTo(ppReturnValue));
                        }
                    }
                }
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ModernCollectionBasePanel::ContainerManager::Interface_GroupHeaderContainerFromItemContainerImpl(
    _In_ xaml::IDependencyObject* pItemContainer,
    _Outptr_result_maybenull_ xaml::IDependencyObject** ppReturnValue)
{
    HRESULT hr = S_OK;

    IFCPTR(ppReturnValue);
    *ppReturnValue = nullptr;

    // make sure we are grouped
    if (m_owner->m_cacheManager.IsGrouping())
    {
        INT index = -1;
        INT indexOfGroup = -1;
        INT indexInsideGroup = -1;
        INT countInGroup = -1;

        // get index of container
        IFC(IICM_IndexFromContainer(pItemContainer, &index));

        if (index >= 0)
        {
            // get container index to group index
            IFC(m_owner->m_cacheManager.GetGroupInformationFromItemIndex(index, &indexOfGroup, &indexInsideGroup, &countInGroup));

            // if container exists group must exist
            ASSERT(indexOfGroup >= 0);

            // get group header from group index
            IFC(IGHM_HeaderFromIndex(indexOfGroup, ppReturnValue));

            // if container exists group header must exist
            ASSERT(*ppReturnValue);
        }
    }

Cleanup:
    RRETURN(hr);
}

// returns an array of indices of all pinned elements by type
_Check_return_ HRESULT ModernCollectionBasePanel::ContainerManager::GetPinnedElementsIndexVector(_In_ xaml_controls::ElementType type, _Out_ std::vector<unsigned int>* pReturnValue)
{
    HRESULT hr = S_OK;

    pReturnValue->clear();

    for (auto element : m_pinnedElements[type])
    {
        pReturnValue->push_back(static_cast<unsigned int>(element.GetIndex()));
    }

    RRETURN(hr);//RRETURN_REMOVAL
}

// Given an element index in the Items(Source), grab the actual pinned element (if it is actually pinned).
_Check_return_ HRESULT ModernCollectionBasePanel::ContainerManager::GetElementFromPinnedElements(_In_ xaml_controls::ElementType type, _In_ INT index, _Out_ ctl::ComPtr<xaml::IUIElement>* pspElement) const
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IUIElement> spElement;

    // focused containers are not necessarily in the pinned container collection, but do act as though they are pinned
    if (type == xaml_controls::ElementType_ItemContainer && m_focusedContainer.GetIndex() > -1 && m_focusedContainer.GetIndex() == index)
    {
        spElement = m_focusedContainer.GetElement();
        ASSERT(spElement);
    }
    else if (type == xaml_controls::ElementType_GroupHeader && m_focusedHeaderContainer.GetIndex() > -1 && m_focusedHeaderContainer.GetIndex() == index)
    {
        spElement = m_focusedHeaderContainer.GetElement();
        ASSERT(spElement);
    }
    else
    {
        // regular pinned
        auto foundPinnedElementInfo = std::find_if(begin(m_pinnedElements[type]), end(m_pinnedElements[type]),
            [&index](const PinnedElementInfo& pinnedElement) -> bool
        {
            return pinnedElement.GetIndex() == index;
        });

        if (foundPinnedElementInfo != end(m_pinnedElements[type]))
        {
            spElement = foundPinnedElementInfo->GetElement();
            ASSERT(spElement);
        }
    }

    *pspElement = std::move(spElement);

    RRETURN(hr);
}

// Given a data index in the Items(Source) and the element, add it to the pinned elements.
_Check_return_ HRESULT ModernCollectionBasePanel::ContainerManager::RegisterPinnedElement(_In_ xaml_controls::ElementType type, _In_ INT index, _In_ const ctl::ComPtr<xaml::IUIElement>& spElement)
{
    HRESULT hr = S_OK;
    ctl::WeakRefPtr wrElement;

    // NOTE: We can't assert that the element is realized because we temporarily pin a partially realized
    // element during element generation, but we can assert that the element isn't a sentinel.
    ASSERT(spElement);

    // Let's also verify that we aren't re-pinning an existing index
#if DBG
    auto it = std::find_if(begin(m_pinnedElements[type]), end(m_pinnedElements[type]),
        [index](const PinnedElementInfo& info)
    {
        return info.GetIndex() == index;
    });
    ASSERT(it == end(m_pinnedElements[type]));
#endif

    IFC(ctl::ComPtr<IUIElement>(spElement).AsWeak(&wrElement)); // copy because AsWeak() isn't const-correct

    m_pinnedElements[type].push_back(
        ModernCollectionBasePanel::ContainerManager::PinnedElementInfo(std::move(wrElement), index)
        );

Cleanup:
    RRETURN(hr);
}

// Given a data index in the Items(Source), remove the corresponding element from the pinned elements (if present).
_Check_return_ HRESULT ModernCollectionBasePanel::ContainerManager::UnregisterPinnedElement(_In_ xaml_controls::ElementType type, _In_ INT index)
{
    // optimize for elements that have _just_ been added
    HRESULT hr = S_OK;

    if (!m_pinnedElements[type].empty())
    {
        ctl::ComPtr<IUIElement> spRemovedContainer;

        if (m_pinnedElements[type].back().GetIndex() == index)
        {
            spRemovedContainer = m_pinnedElements[type].back().GetElement();
            m_pinnedElements[type].pop_back();
        }
        else
        {
            auto foundPinnedElementInfo = std::find_if(begin(m_pinnedElements[type]), end(m_pinnedElements[type]),
                [&index](const PinnedElementInfo& pinnedElement) -> bool
            {
                return pinnedElement.GetIndex() == index;
            });

            if (foundPinnedElementInfo != end(m_pinnedElements[type]))
            {
                spRemovedContainer = foundPinnedElementInfo->GetElement();
                m_pinnedElements[type].erase(foundPinnedElementInfo);
            }
        }

        if (spRemovedContainer)
        {
            IFC(ElementUnpinned(
                type,
                index,
                spRemovedContainer,
                TRUE /* removeNonGeneratedContainer */ ));
        }
    }

Cleanup:
    RRETURN(hr);
}

// Keep only the pinned containers that are approved by the given filter function.
// The function takes the index and container as arguments, and should return TRUE for pinned containers to keep.
_Check_return_ HRESULT ModernCollectionBasePanel::ContainerManager::FilterPinnedContainers(_In_ std::function<HRESULT (INT, xaml::IUIElement*, BOOLEAN*)> filterFunction)
{
    HRESULT hr = S_OK;

    auto& pinnedContainers = m_pinnedElements[xaml_controls::ElementType_ItemContainer];
    auto iter = pinnedContainers.begin();

    while(iter != pinnedContainers.end())
    {
        PinnedElementInfo info = *iter;
        ctl::ComPtr<IUIElement> spContainer = info.GetElement();
        INT index = info.GetIndex();
        BOOLEAN keep = FALSE;

        IFC(filterFunction(index, spContainer.Get(), &keep));

        if (!keep)
        {
            iter = pinnedContainers.erase(iter);

            IFC(ElementUnpinned(
                xaml_controls::ElementType_ItemContainer,
                index,
                spContainer,
                TRUE /* removeNonGeneratedContainer */ ));
        }
        else
        {
            ++iter;
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ModernCollectionBasePanel::ContainerManager::EnsureContainerPinned(_In_ INT index, _In_ const ctl::ComPtr<xaml::IUIElement>& spContainer)
{
    HRESULT hr = S_OK;

    auto& pinnedContainers = m_pinnedElements[xaml_controls::ElementType_ItemContainer];
    auto foundPinnedContainerInfo = std::find_if(
        begin(pinnedContainers),
        end(pinnedContainers),
        [&spContainer](const PinnedElementInfo& pinnedContainer) -> bool
    {
        return pinnedContainer.GetElement() == spContainer;
    });

    if (foundPinnedContainerInfo == end(pinnedContainers))
    {
        IFC(RegisterPinnedContainer(index, spContainer));
    }

Cleanup:
    RRETURN(hr);
}

// Is the given element pinned?
bool ModernCollectionBasePanel::ContainerManager::GetIsElementPinned(
    _In_ xaml_controls::ElementType type,
    _In_ const ctl::ComPtr<xaml::IUIElement>& spElement,
    _Out_opt_ INT* pPinnedIndex) const
{
    // todo: if this starts becoming a perf issue, cache a boolean on pVirtualizationInformation on UIElement
    INT pinnedIndex = -1;

    bool isPinned = false;

    // focused containers are not necessarily in the pinned container collection, but do act as though they are pinned
    if (type == xaml_controls::ElementType_ItemContainer && m_focusedContainer.GetElement() && m_focusedContainer.GetElement() == spElement)
    {
        isPinned = true;
        pinnedIndex = m_focusedContainer.GetIndex();
    }
    else if (type == xaml_controls::ElementType_GroupHeader && m_focusedHeaderContainer.GetElement() && m_focusedHeaderContainer.GetElement() == spElement)
    {
        isPinned = true;
        pinnedIndex = m_focusedHeaderContainer.GetIndex();
    }
    else
    {
        auto foundPinnedElementInfo = std::find_if(begin(m_pinnedElements[type]), end(m_pinnedElements[type]),
            [&spElement](const PinnedElementInfo& pinnedElement) -> bool
        {
            return pinnedElement.GetElement() == spElement;
        });

        if (foundPinnedElementInfo != end(m_pinnedElements[type]))
        {
            isPinned = true;
            pinnedIndex = foundPinnedElementInfo->GetIndex();
        }

        // We shouldn't ever find a sentinel pinned element
        ASSERT(foundPinnedElementInfo == end(m_pinnedElements[type]) || (*foundPinnedElementInfo).GetElement());
    }

    if (pPinnedIndex != nullptr)
    {
        *pPinnedIndex = pinnedIndex;
    }

    return isPinned;
}

bool ModernCollectionBasePanel::ContainerManager::GetIsIndexPinned(_In_ xaml_controls::ElementType type, _In_ INT index) const
{
    const auto& pinnedElements = m_pinnedElements[type];
    auto pinnedIterator = std::find_if(begin(pinnedElements), end(pinnedElements),
        [index](const PinnedElementInfo& element)
    {
        return index == element.GetIndex();
    });

    return pinnedIterator != end(pinnedElements);
}

// Called when an item is added to Items(Source). We need to know about this so we can update various internal indexes.
_Check_return_ HRESULT ModernCollectionBasePanel::ContainerManager::OnDataItemAdded(_In_ xaml_controls::ElementType type, _In_ INT index)
{
    HRESULT hr = S_OK;

    // First, if this new item is in front of our valid block, let's update the first index
    if (GetValidElementCount(type) > 0 && index <= m_indicesOfFirstValidElement[type])
    {
        ++m_indicesOfFirstValidElement[type];
    }

    // If the item was added at the end, there's no existing indices to update.
    if (!m_elementIndicesForChildren[type].empty() && index <= m_elementIndicesForChildren[type].back())
    {
        // Update the indices of any entry in the child collection that comes after the new insert
        std::for_each(begin(m_elementIndicesForChildren[type]), end(m_elementIndicesForChildren[type]),
            [index](INT32& element)
        {
            if (index <= element)
            {
                ++element;
            }
        });
    }

    // focused containers are not necessarily in the pinned container collection, but do act as though they are pinned
    if (type == xaml_controls::ElementType_ItemContainer && m_focusedContainer.GetIndex() >= index)
    {
        m_focusedContainer.UpdateIndex(m_focusedContainer.GetIndex() + 1);
    }

    if (type == xaml_controls::ElementType_GroupHeader && m_focusedHeaderContainer.GetIndex() >= index)
    {
        m_focusedHeaderContainer.UpdateIndex(m_focusedHeaderContainer.GetIndex() + 1);
    }

    std::for_each(begin(m_pinnedElements[type]), end(m_pinnedElements[type]),
        [&index](PinnedElementInfo& pinnedElement) -> void
    {
        if (pinnedElement.GetIndex() >= index)
        {
            pinnedElement.UpdateIndex(pinnedElement.GetIndex() + 1);
        }
    });

    RRETURN(hr);
}

// Called when an item is removed from Items(Source). We need to know about this so we can update various internal indexes.
_Check_return_ HRESULT ModernCollectionBasePanel::ContainerManager::OnDataItemRemoved(_In_ xaml_controls::ElementType type, _In_ INT index)
{
    HRESULT hr = S_OK;

    IFC(UnregisterPinnedElement(type, index));

    // focused containers are not necessarily in the pinned container collection, but do act as though they are pinned
    if (type == xaml_controls::ElementType_ItemContainer)
    {
        if (m_focusedContainer.GetIndex() > -1 &&  m_focusedContainer.GetIndex() > index)
        {
            m_focusedContainer.UpdateIndex(m_focusedContainer.GetIndex() -1);
        }
        else if (m_focusedContainer.GetIndex() == index)
        {
            // reset the focusedContainer. Unregistering a pinnedcontainer should not do that for you
            // because there could be a different reason a focusedcontainer was also pinned
            IFC(ResetFocusedContainer(TRUE));
        }
    }

    if (type == xaml_controls::ElementType_GroupHeader)
    {
        if (m_focusedHeaderContainer.GetIndex() > -1 && m_focusedHeaderContainer.GetIndex() > index)
        {
            m_focusedHeaderContainer.UpdateIndex(m_focusedHeaderContainer.GetIndex() - 1);
        }
        else if (m_focusedHeaderContainer.GetIndex() == index)
        {
            // reset the focusedContainer. Unregistering a pinnedcontainer should not do that for you
            // because there could be a different reason a focusedcontainer was also pinned
            IFC(ResetFocusedHeaderContainer(TRUE));
        }
    }

    std::for_each(begin(m_pinnedElements[type]), end(m_pinnedElements[type]),
        [&index](PinnedElementInfo& pinnedElement) -> void
    {
        if (pinnedElement.GetIndex() > index)
        {
            pinnedElement.UpdateIndex(pinnedElement.GetIndex() - 1);
        }
    });

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ModernCollectionBasePanel::ContainerManager::OnDataItemReplaced(
    _In_ xaml_controls::ElementType type,
    _In_ INT index,
    _In_ xaml::IUIElement* pNewElement)
{
    ctl::WeakRefPtr newWeakRef;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;
    IFC_RETURN(ctl::AsWeak(pNewElement, &newWeakRef));
    INT32 childIndex = StartOfGarbageSection();

    IFC_RETURN(m_owner->m_cacheManager.GetChildren(&spChildren));

    const INT validIndex = GetValidElementIndexFromDataIndex(type, index);
    if (0 <= validIndex && validIndex < static_cast<INT>(m_validElements[type].size()))
    {
        // Grab the old element
        ctl::ComPtr<IUIElement> spOldElement = m_validElements[type][validIndex].AsOrNull<IUIElement>();

        // Find the location of this element in the visual children collection
        auto& currentIndexMap = m_elementIndicesForChildren[type];
        auto indexMapIterator = std::lower_bound(begin(currentIndexMap), end(currentIndexMap), index);
        ASSERT(indexMapIterator != end(currentIndexMap));
        childIndex = StartOfVisualSection(type) + static_cast<INT32>(indexMapIterator - begin(currentIndexMap));

        // Sanity check the index lookup
#ifdef DBG
        {
            UINT32 foundIndex = -1;
            BOOLEAN found = FALSE;
            IFC_RETURN(spChildren->IndexOf(spOldElement.Get(), &foundIndex, &found));
            ASSERT(found);
            ASSERT(StartOfVisualSection(type) <= static_cast<INT32>(foundIndex) &&
                static_cast<INT32>(foundIndex) < EndOfVisualSection(type));
            ASSERT(foundIndex == childIndex);
        }
#endif

        // Remove the old element
        if (type == xaml_controls::ElementType_ItemContainer && (!ModernCollectionBasePanel::GetElementIsGenerated(spOldElement)))
        {
            // Containers have a special case for items that are their own container
            // Rip it right out of the children. For item containers, we do this to show the remove transition
            IFC_RETURN(spChildren.Cast<UIElementCollection>()->RemoveAt(childIndex));
        }
        else
        {
            // Move it to the junk section
            IFC_RETURN(spChildren.Cast<UIElementCollection>()->MoveInternal(childIndex, StartOfGarbageSection()));
        }
        ModernCollectionBasePanel::SetElementIsRealized(spOldElement, false);

        // Replace the valid element entry with the new element
        m_validElements[type][validIndex] = newWeakRef;
    }

    // Place the new element
    UINT32 foundIndexOfNewElement = -1;
    BOOLEAN foundNewElement = FALSE;
    IFC_RETURN(spChildren->IndexOf(pNewElement, &foundIndexOfNewElement, &foundNewElement));

    // Make sure we're not reinserting an existing container
    ASSERT(!foundNewElement || foundIndexOfNewElement >= static_cast<UINT>(StartOfGarbageSection()));
    if (!foundNewElement)
    {
        IFC_RETURN(spChildren->InsertAt(childIndex, pNewElement));
    }
    else
    {
        IFC_RETURN(spChildren.Cast<UIElementCollection>()->MoveInternal(foundIndexOfNewElement, childIndex));
    }

    ModernCollectionBasePanel::SetElementIsRealized(pNewElement, true);

    return S_OK;
}

_Check_return_ HRESULT ModernCollectionBasePanel::ContainerManager::SetFocusedContainer(_In_ INT itemIndex, _In_ const ctl::ComPtr<xaml::IUIElement>& spContainer)
{
    HRESULT hr = S_OK;
    ctl::WeakRefPtr wrContainer;

    ASSERT((itemIndex >= 0 && spContainer) || (itemIndex == -1 && !spContainer)); // either we have a valid pair or an 'empty' pair (=reset)

    if(itemIndex != -1 && m_focusedContainer.GetIndex() > -1 && itemIndex != m_focusedContainer.GetIndex())  // if this is not a reset action (-1) then we should currently already be empty
    {
        // we should always have tested the current focused container and removed it if possible
        IFC(E_FAIL);
    }

    IFC(ResetFocusedContainer(TRUE));

    IFC(ctl::ComPtr<IUIElement>(spContainer).AsWeak(&wrContainer)); // Copy the ComPtr because QI isn't a const method

    m_focusedContainer = ModernCollectionBasePanel::ContainerManager::PinnedElementInfo(std::move(wrContainer), itemIndex);

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ModernCollectionBasePanel::ContainerManager::GetFocusedContainer(_Out_ INT* pItemIndex, _Out_ ctl::ComPtr<xaml::IUIElement>* pspContainer) const
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IUIElement> spContainer;
    INT index = m_focusedContainer.GetIndex();

    if(index > -1)
    {
        spContainer = m_focusedContainer.GetElement();
    }

    *pspContainer = spContainer;
    *pItemIndex = index;

    RRETURN(hr);
}

_Check_return_ HRESULT ModernCollectionBasePanel::ContainerManager::ResetFocusedContainer(
    _In_ BOOLEAN removeNonGeneratedContainer)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IUIElement> spPreviousFocusedElement = m_focusedContainer.GetElement();
    INT32 previousFocusedIndex = m_focusedContainer.GetIndex();

    m_focusedContainer = ModernCollectionBasePanel::ContainerManager::PinnedElementInfo();

    if (spPreviousFocusedElement)
    {
        IFC(ElementUnpinned(
            xaml_controls::ElementType_ItemContainer,
            previousFocusedIndex,
            spPreviousFocusedElement,
            removeNonGeneratedContainer));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ModernCollectionBasePanel::ContainerManager::SetFocusedHeaderContainer(_In_ INT groupIndex, _In_ const ctl::ComPtr<xaml::IUIElement>& spContainer)
{
    HRESULT hr = S_OK;
    ctl::WeakRefPtr wrContainer;

    ASSERT((groupIndex >= 0 && spContainer) || (groupIndex == -1 && !spContainer)); // either we have a valid pair or an 'empty' pair (=reset)

    if (groupIndex != -1 && m_focusedHeaderContainer.GetIndex() > -1 && groupIndex != m_focusedHeaderContainer.GetIndex())  // if this is not a reset action (-1) then we should currently already be empty
    {
        // we should always have tested the current focused container and removed it if possible
        IFC(E_FAIL);
    }

    IFC(ResetFocusedHeaderContainer(TRUE));

    IFC(ctl::ComPtr<IUIElement>(spContainer).AsWeak(&wrContainer)); // Copy the ComPtr because QI isn't a const method

    m_focusedHeaderContainer = ModernCollectionBasePanel::ContainerManager::PinnedElementInfo(std::move(wrContainer), groupIndex);

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ModernCollectionBasePanel::ContainerManager::GetFocusedHeaderContainer(_Out_ INT* pGroupIndex, _Out_ ctl::ComPtr<xaml::IUIElement>* pspContainer) const
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IUIElement> spContainer;
    INT index = m_focusedHeaderContainer.GetIndex();

    if (index > -1)
    {
        spContainer = m_focusedHeaderContainer.GetElement();
    }

    *pspContainer = spContainer;
    *pGroupIndex = index;

    RRETURN(hr);
}

_Check_return_ HRESULT ModernCollectionBasePanel::ContainerManager::ResetFocusedHeaderContainer(
    _In_ BOOLEAN removeNonGeneratedContainer)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IUIElement> spPreviousFocusedElement = m_focusedHeaderContainer.GetElement();
    INT32 previousFocusedIndex = m_focusedHeaderContainer.GetIndex();

    m_focusedHeaderContainer = ModernCollectionBasePanel::ContainerManager::PinnedElementInfo();

    if (spPreviousFocusedElement)
    {
        IFC(ElementUnpinned(
            xaml_controls::ElementType_GroupHeader,
            previousFocusedIndex,
            spPreviousFocusedElement,
            removeNonGeneratedContainer));
    }

Cleanup:
    RRETURN(hr);
}

// Temporary holding for elements being prepared
void ModernCollectionBasePanel::ContainerManager::HoldForPrepare(
    _In_ ctl::ComPtr<IUIElement> spElement,
    _In_ xaml_controls::ElementType type,
    _In_ INT index)
{
    // Somebody is goofed up if this didn't happen
    ASSERT(m_preparationHoldingArea.m_index == -1 && !m_preparationHoldingArea.m_element);
    m_preparationHoldingArea.m_element = std::move(spElement);
    m_preparationHoldingArea.m_type = type;
    m_preparationHoldingArea.m_index = index;
}

void ModernCollectionBasePanel::ContainerManager::ResetAfterPrepare()
{
    m_preparationHoldingArea.Reset();
}
