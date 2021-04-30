// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "ItemsRepeater.common.h"
#include "ElementManager.h"

void ElementManager::SetContext(const winrt::VirtualizingLayoutContext& virtualContext)
{
    m_context = virtualContext;
}

void ElementManager::OnBeginMeasure(const ScrollOrientation& orientation)
{
    if (m_context)
    {
        if (IsVirtualizingContext())
        {
            // We proactively clear elements laid out outside of the realizaton
            // rect so that they are available for reuse during the current
            // measure pass.
            // This is useful during fast panning scenarios in which the realization
            // window is constantly changing and we want to reuse elements from
            // the end that's opposite to the panning direction.
            DiscardElementsOutsideWindow(m_context.RealizationRect(), orientation);
        }
        else
        {
            // If we are initialized with a non-virtualizing context, make sure that
            // we have enough space to hold the bounds for all the elements.
            const int count = m_context.ItemCount();
            if (static_cast<int>(m_realizedElementLayoutBounds.size()) != count)
            {
                // Make sure there is enough space for the bounds.
                // Note: We could optimize when the count becomes smaller, but keeping
                // it always up to date is the simplest option for now.
                m_realizedElementLayoutBounds.resize(count, winrt::Rect());
            }
        }
    }
}

int ElementManager::GetRealizedElementCount() const
{
    return IsVirtualizingContext() ?
        static_cast<int>(m_realizedElements.size()) : m_context.ItemCount();
}

winrt::UIElement ElementManager::GetAt(int realizedIndex)
{
    winrt::UIElement element{ nullptr };
    if (IsVirtualizingContext())
    {
        if (!m_realizedElements[realizedIndex])
        {
            // Sentinel. Create the element now since we need it.
            const int dataIndex = GetDataIndexFromRealizedRangeIndex(realizedIndex);
            REPEATER_TRACE_INFO(L"Creating element for sentinal with data index %d. \n", dataIndex);
            element = m_context.GetOrCreateElementAt(dataIndex, winrt::ElementRealizationOptions::ForceCreate | winrt::ElementRealizationOptions::SuppressAutoRecycle);
            m_realizedElements[realizedIndex] = tracker_ref<winrt::UIElement>{ m_owner, element };
        }
        else
        {
            element = m_realizedElements[realizedIndex].get();
        }
    }
    else
    {
        // realizedIndex and dataIndex are the same (everything is realized)
        element = m_context.GetOrCreateElementAt(realizedIndex, winrt::ElementRealizationOptions::ForceCreate | winrt::ElementRealizationOptions::SuppressAutoRecycle);
    }

    return element;
}

void ElementManager::Add(const winrt::UIElement& element, int dataIndex)
{
    MUX_ASSERT(IsVirtualizingContext());

    if (m_realizedElements.size() == 0)
    {
        m_firstRealizedDataIndex = dataIndex;
    }

    m_realizedElements.emplace_back(tracker_ref<winrt::UIElement>{ m_owner, element });
    m_realizedElementLayoutBounds.emplace_back(winrt::Rect());
}

void ElementManager::Insert(int realizedIndex, int dataIndex, const winrt::UIElement& element)
{
    MUX_ASSERT(IsVirtualizingContext());
    if (realizedIndex == 0)
    {
        m_firstRealizedDataIndex = dataIndex;
    }

    m_realizedElements.insert(m_realizedElements.begin() + realizedIndex, tracker_ref<winrt::UIElement>{ m_owner, element });
    // Set bounds to an invalid rect since we do not know it yet.
    m_realizedElementLayoutBounds.insert(m_realizedElementLayoutBounds.begin() + realizedIndex, winrt::Rect{ -1.f, -1.f, -1.f, -1.f });
}

void ElementManager::ClearRealizedRange(int realizedIndex, int count)
{
    MUX_ASSERT(IsVirtualizingContext());
    for (int i = 0; i < count; i++)
    {
        // Clear from the edges so that ItemsRepeater can optimize on maintaining 
        // realized indices without walking through all the children every time.
        const int index = realizedIndex == 0 ? realizedIndex + i : (realizedIndex + count - 1) - i;
        if (auto elementRef = m_realizedElements[index])
        {
            m_context.RecycleElement(elementRef.get());
        }
    }

    const int endIndex = realizedIndex + count;
    m_realizedElements.erase(m_realizedElements.begin() + realizedIndex, m_realizedElements.begin() + endIndex);
    m_realizedElementLayoutBounds.erase(m_realizedElementLayoutBounds.begin() + realizedIndex, m_realizedElementLayoutBounds.begin() + endIndex);

    if (realizedIndex == 0)
    {
        m_firstRealizedDataIndex =
            m_realizedElements.size() == 0 ?
            -1 :
            m_firstRealizedDataIndex + count;
    }
}

void ElementManager::DiscardElementsOutsideWindow(bool forward, int startIndex)
{
    // Remove layout elements that are outside the realized range.
    if (IsDataIndexRealized(startIndex))
    {
        MUX_ASSERT(IsVirtualizingContext());
        const int rangeIndex = GetRealizedRangeIndexFromDataIndex(startIndex);

        if (forward)
        {
            ClearRealizedRange(rangeIndex, GetRealizedElementCount() - rangeIndex);
        }
        else
        {
            ClearRealizedRange(0, rangeIndex + 1);
        }
    }
}

void ElementManager::ClearRealizedRange()
{
    MUX_ASSERT(IsVirtualizingContext());
    ClearRealizedRange(0, GetRealizedElementCount());
}


winrt::Rect ElementManager::GetLayoutBoundsForDataIndex(int dataIndex) const
{
    const int realizedIndex = GetRealizedRangeIndexFromDataIndex(dataIndex);
    return m_realizedElementLayoutBounds[realizedIndex];
}

void ElementManager::SetLayoutBoundsForDataIndex(int dataIndex, const winrt::Rect& bounds)
{
    const int realizedIndex = GetRealizedRangeIndexFromDataIndex(dataIndex);
    m_realizedElementLayoutBounds[realizedIndex] = bounds;
}


winrt::Rect ElementManager::GetLayoutBoundsForRealizedIndex(int realizedIndex) const
{
    return m_realizedElementLayoutBounds[realizedIndex];
}

void ElementManager::SetLayoutBoundsForRealizedIndex(int realizedIndex, const winrt::Rect& bounds)
{
    m_realizedElementLayoutBounds[realizedIndex] = bounds;
}


bool ElementManager::IsDataIndexRealized(int index) const
{
    if (IsVirtualizingContext())
    {
        const int realizedCount = GetRealizedElementCount();
        return
            realizedCount > 0 &&
            GetDataIndexFromRealizedRangeIndex(0) <= index &&
            GetDataIndexFromRealizedRangeIndex(realizedCount - 1) >= index;
    }
    else
    {
        // Non virtualized - everything is realized
        return IsIndexValidInData(index);
    }
}

bool ElementManager::IsIndexValidInData(int currentIndex) const
{
    return currentIndex >= 0 && currentIndex < m_context.ItemCount();
}


winrt::UIElement ElementManager::GetRealizedElement(int dataIndex)
{
    MUX_ASSERT(IsDataIndexRealized(dataIndex));
    return IsVirtualizingContext() ?
        GetAt(GetRealizedRangeIndexFromDataIndex(dataIndex)) : m_context.GetOrCreateElementAt(dataIndex, winrt::ElementRealizationOptions::ForceCreate | winrt::ElementRealizationOptions::SuppressAutoRecycle);
}

void ElementManager::EnsureElementRealized(bool forward, int dataIndex, const wstring_view& layoutId)
{
    if (IsDataIndexRealized(dataIndex) == false)
    {
        auto element = m_context.GetOrCreateElementAt(dataIndex, winrt::ElementRealizationOptions::ForceCreate | winrt::ElementRealizationOptions::SuppressAutoRecycle);

        if (forward)
        {
            Add(element, dataIndex);
        }
        else
        {
            Insert(0, dataIndex, element);
        }

        MUX_ASSERT(IsDataIndexRealized(dataIndex));
        REPEATER_TRACE_INFO(L"%ls: \tCreated element for index %d. \n", layoutId.data(), dataIndex);
    }
}

// Does the given window intersect the range of realized elements
bool ElementManager::IsWindowConnected(const winrt::Rect& window, const ScrollOrientation& orientation, bool scrollOrientationSameAsFlow) const
{
    MUX_ASSERT(IsVirtualizingContext());
    bool intersects = false;
    if (m_realizedElementLayoutBounds.size() > 0)
    {
        const auto firstElementBounds = GetLayoutBoundsForRealizedIndex(0);
        const auto lastElementBounds = GetLayoutBoundsForRealizedIndex(GetRealizedElementCount() - 1);

        const auto effectiveOrientation = scrollOrientationSameAsFlow ?
            (orientation == ScrollOrientation::Vertical ? ScrollOrientation::Horizontal : ScrollOrientation::Vertical) :
            orientation;


        const auto windowStart = effectiveOrientation == ScrollOrientation::Vertical ? window.Y : window.X;
        const auto windowEnd = effectiveOrientation == ScrollOrientation::Vertical ? window.Y + window.Height : window.X + window.Width;
        const auto firstElementStart = effectiveOrientation == ScrollOrientation::Vertical ? firstElementBounds.Y : firstElementBounds.X;
        const auto lastElementEnd = effectiveOrientation == ScrollOrientation::Vertical ? lastElementBounds.Y + lastElementBounds.Height : lastElementBounds.X + lastElementBounds.Width;

        intersects =
            firstElementStart <= windowEnd &&
            lastElementEnd >= windowStart;
    }

    return intersects;
}

void ElementManager::DataSourceChanged(const winrt::IInspectable& /*source*/, winrt::NotifyCollectionChangedEventArgs const& args)
{
    MUX_ASSERT(IsVirtualizingContext());
    if (m_realizedElements.size() > 0)
    {
        switch (args.Action())
        {
        case winrt::NotifyCollectionChangedAction::Add:
        {
            OnItemsAdded(args.NewStartingIndex(), args.NewItems().Size());
        }
        break;

        case winrt::NotifyCollectionChangedAction::Replace:
        {
            const int oldSize = args.OldItems().Size();
            const int newSize = args.NewItems().Size();
            const int oldStartIndex = args.OldStartingIndex();
            const int newStartIndex = args.NewStartingIndex();

            if (oldSize == newSize &&
                oldStartIndex == newStartIndex &&
                IsDataIndexRealized(oldStartIndex) &&
                IsDataIndexRealized(oldStartIndex + oldSize - 1))
            {
                // Straight up replace of n items within the realization window.
                // Removing and adding might causes us to lose the anchor causing us
                // to throw away all containers and start from scratch.
                // Instead, we can just clear those items and set the element to
                // null (sentinel) and let the next measure get new containers for them.
                const auto startRealizedIndex = GetRealizedRangeIndexFromDataIndex(oldStartIndex);
                for (int realizedIndex = startRealizedIndex; realizedIndex < startRealizedIndex + oldSize; realizedIndex++)
                {
                    if (auto elementRef = m_realizedElements[realizedIndex])
                    {
                        m_context.RecycleElement(elementRef.get());
                        m_realizedElements[realizedIndex] = tracker_ref<winrt::UIElement>{ m_owner, nullptr };
                    }
                }
            }
            else
            {
                OnItemsRemoved(oldStartIndex, oldSize);
                OnItemsAdded(newStartIndex, newSize);
            }
        }
        break;

        case winrt::NotifyCollectionChangedAction::Remove:
        {
            OnItemsRemoved(args.OldStartingIndex(), args.OldItems().Size());
        }
        break;

        case winrt::NotifyCollectionChangedAction::Reset:
            ClearRealizedRange();
            break;

        case winrt::NotifyCollectionChangedAction::Move:
            int size = args.OldItems() != NULL ? args.OldItems().Size() : 1;
            OnItemsRemoved(args.OldStartingIndex(), size);
            OnItemsAdded(args.NewStartingIndex(), size);
            break;
        }
    }
}

int ElementManager::GetElementDataIndex(const winrt::UIElement& suggestedAnchor) const
{
    MUX_ASSERT(suggestedAnchor);
    auto it = std::find(m_realizedElements.cbegin(), m_realizedElements.cend(), suggestedAnchor);
    return
        it != m_realizedElements.cend() ?
        GetDataIndexFromRealizedRangeIndex(static_cast<int>(std::distance(m_realizedElements.cbegin(), it))) :
        -1;
}

int ElementManager::GetDataIndexFromRealizedRangeIndex(int rangeIndex) const
{
    MUX_ASSERT(rangeIndex >= 0 && rangeIndex < GetRealizedElementCount());
    return IsVirtualizingContext() ?
        rangeIndex + m_firstRealizedDataIndex : rangeIndex;
}

int ElementManager::GetRealizedRangeIndexFromDataIndex(int dataIndex) const
{
    MUX_ASSERT(IsDataIndexRealized(dataIndex));
    return IsVirtualizingContext() ?
        dataIndex - m_firstRealizedDataIndex : dataIndex;
}

void ElementManager::DiscardElementsOutsideWindow(const winrt::Rect& window, const ScrollOrientation& orientation)
{
    MUX_ASSERT(IsVirtualizingContext());
    MUX_ASSERT(m_realizedElements.size() == m_realizedElementLayoutBounds.size());

    // The following illustration explains the cutoff indices.
    // We will clear all the realized elements from both ends
    // up to the corresponding cutoff index.
    // '-' means the element is outside the cutoff range.
    // '*' means the element is inside the cutoff range and will be cleared.
    //
    // Window:
    //        |______________________________|
    // Realization range:
    // |*****----------------------------------*********|
    //      |                                  |
    //  frontCutoffIndex                backCutoffIndex
    //
    // Note that we tolerate at most one element outside of the window
    // because the FlowLayoutAlgorithm.Generate routine stops *after*
    // it laid out an element outside the realization window.
    // This is also convenient because it protects the anchor
    // during a BringIntoView operation during which the anchor may
    // not be in the realization window (in fact, the realization window
    // might be empty if the BringIntoView is issued before the first
    // layout pass).

    const int realizedRangeSize = GetRealizedElementCount();
    int frontCutoffIndex = -1;
    int backCutoffIndex = realizedRangeSize;

    for (int i = 0;
        i < realizedRangeSize &&
        !Intersects(window, m_realizedElementLayoutBounds[i], orientation);
        ++i)
    {
        ++frontCutoffIndex;
    }

    for (int i = realizedRangeSize - 1;
        i >= 0 &&
        !Intersects(window, m_realizedElementLayoutBounds[i], orientation);
        --i)
    {
        --backCutoffIndex;
    }

    if (backCutoffIndex < realizedRangeSize - 1)
    {
        ClearRealizedRange(backCutoffIndex + 1, realizedRangeSize - backCutoffIndex - 1);
    }

    if (frontCutoffIndex > 0)
    {
        ClearRealizedRange(0, std::min(frontCutoffIndex, GetRealizedElementCount()));
    }
}

/* static */
bool ElementManager::Intersects(const winrt::Rect& lhs, const winrt::Rect& rhs, const ScrollOrientation& orientation)
{
    const auto lhsStart = orientation == ScrollOrientation::Vertical ? lhs.Y : lhs.X;
    const auto lhsEnd = orientation == ScrollOrientation::Vertical ? lhs.Y + lhs.Height : lhs.X + lhs.Width;
    const auto rhsStart = orientation == ScrollOrientation::Vertical ? rhs.Y : rhs.X;
    const auto rhsEnd = orientation == ScrollOrientation::Vertical ? rhs.Y + rhs.Height : rhs.X + rhs.Width;

    return lhsEnd >= rhsStart && lhsStart <= rhsEnd;
}

void ElementManager::OnItemsAdded(int index, int count)
{
    // Using the old indices here (before it was updated by the collection change)
    // if the insert data index is between the first and last realized data index, we need
    // to insert items.
    const int lastRealizedDataIndex = m_firstRealizedDataIndex + GetRealizedElementCount() - 1;
    const int newStartingIndex = index;
    if (newStartingIndex >= m_firstRealizedDataIndex &&
        newStartingIndex <= lastRealizedDataIndex)
    {
        // Inserted within the realized range
        const int insertRangeStartIndex = newStartingIndex - m_firstRealizedDataIndex;
        for (int i = 0; i < count; i++)
        {
            // Insert null (sentinel) here instead of an element, that way we dont 
            // end up creating a lot of elements only to be thrown out in the next layout.
            const int insertRangeIndex = insertRangeStartIndex + i;
            const int dataIndex = newStartingIndex + i;
            // This is to keep the contiguousness of the mapping
            Insert(insertRangeIndex, dataIndex, nullptr);
        }
    }
    else if (index <= m_firstRealizedDataIndex)
    {
        // Items were inserted before the realized range.
        // We need to update m_firstRealizedDataIndex;
        m_firstRealizedDataIndex += count;
    }
}

void ElementManager::OnItemsRemoved(int index, int count)
{
    const int lastRealizedDataIndex = m_firstRealizedDataIndex + static_cast<int>(m_realizedElements.size()) - 1;
    const int startIndex = std::max(m_firstRealizedDataIndex, index);
    const int endIndex = std::min(lastRealizedDataIndex, index + count - 1);
    const bool removeAffectsFirstRealizedDataIndex = (index <= m_firstRealizedDataIndex);

    if (endIndex >= startIndex)
    {
        ClearRealizedRange(GetRealizedRangeIndexFromDataIndex(startIndex), endIndex - startIndex + 1);
    }

    if (removeAffectsFirstRealizedDataIndex &&
        m_firstRealizedDataIndex != -1)
    {
        m_firstRealizedDataIndex -= count;
    }
}


bool ElementManager::IsVirtualizingContext() const
{
    if (m_context)
    {
        const auto rect = m_context.RealizationRect();
        const bool hasInfiniteSize = (rect.Height == std::numeric_limits<float>::infinity() || rect.Width == std::numeric_limits<float>::infinity());
        return !hasInfiniteSize;
    }
    return false;
}
