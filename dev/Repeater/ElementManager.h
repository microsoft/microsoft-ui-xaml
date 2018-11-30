// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "OrientationBasedMeasures.h"

// Internal component for layout to keep track of elements and
// help with collection changes.
class ElementManager final
{

public:
    ElementManager(const ITrackerHandleManager* owner) : m_owner(owner) { }

    void SetContext(const winrt::VirtualizingLayoutContext& virtualContext);
    
    void OnBeginMeasure(const ScrollOrientation& orientation);

    int GetRealizedElementCount() const ;
    winrt::UIElement GetAt(int realizedIndex);

    void Add(const winrt::UIElement& element, int dataIndex);
    void Insert(int realizedIndex, int dataIndex, const winrt::UIElement& element);
    void ClearRealizedRange(int realizedIndex, int count);
    void DiscardElementsOutsideWindow(bool forward, int startIndex);
    void ClearRealizedRange();

    winrt::Rect GetLayoutBoundsForDataIndex(int dataIndex) const;
    void SetLayoutBoundsForDataIndex(int dataIndex, const winrt::Rect& bounds);

    winrt::Rect GetLayoutBoundsForRealizedIndex(int realizedIndex) const;
    void SetLayoutBoundsForRealizedIndex(int realizedIndex, const winrt::Rect& bounds);

    bool IsDataIndexRealized(int index) const;
    bool IsIndexValidInData(int currentIndex) const;

    winrt::UIElement GetRealizedElement(int dataIndex);
    void EnsureElementRealized(bool forward, int dataIndex, const wstring_view& layoutId);

    bool IsWindowConnected(const winrt::Rect& window, const ScrollOrientation& orientation) const;
    void DataSourceChanged(const winrt::IInspectable& source, winrt::NotifyCollectionChangedEventArgs const& args);
    
    // we do not want copies of this type
    ElementManager(const ElementManager& that) = delete;
    ElementManager& ElementManager::operator=(const ElementManager& other) = delete;

    int GetElementDataIndex(const winrt::UIElement& suggestedAnchor) const;
    int GetDataIndexFromRealizedRangeIndex(int rangeIndex) const;

private:
    int GetRealizedRangeIndexFromDataIndex(int dataIndex) const;

    void DiscardElementsOutsideWindow(const winrt::Rect& window, const ScrollOrientation& orientation);
    static bool Intersects(const winrt::Rect& lhs, const winrt::Rect& rhs, const ScrollOrientation& orientation);

    void OnItemsAdded(int index, int count);
    void OnItemsRemoved(int index, int count);

    bool IsVirtualizingContext() const;

    const ITrackerHandleManager* m_owner;

    std::vector<tracker_ref<winrt::UIElement>> m_realizedElements;
    std::vector<winrt::Rect> m_realizedElementLayoutBounds;
    int m_firstRealizedDataIndex{ -1 };
    winrt::VirtualizingLayoutContext m_context{ nullptr };
};