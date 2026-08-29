// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace DirectUI { namespace Components { namespace LiveReorderHelper {

    struct MovedItem
    {
        int sourceIndex;
        int destinationIndex;
        wf::Rect sourceRect;
        wf::Rect destinationRect;

        MovedItem(int sourceIdx, int destinationIdx, wf::Rect sourceRt, wf::Rect destinationRt)
        {
            sourceIndex = sourceIdx;
            destinationIndex = destinationIdx;
            sourceRect = sourceRt;
            destinationRect = destinationRt;
        }
    };

    class MovedItems
    {
    public:
        // this function returns the items that should be moved back to their original location
        // and the items that should be moved to their new location
        void Update(
            _In_ bool isOrientationVertical,
            _Inout_ const std::vector<MovedItem>& newItems,
            _Inout_ std::vector<MovedItem>& newItemsToMove,
            _Inout_ std::vector<MovedItem>& oldItemsToMoveBack);

        // this function makes sure that we drag the right elements into the right space
        // taking into account the previously dragged elements
        static int GetDragOverIndex(
            _In_ int closestElementIndex,
            _In_ int insertionIndex,
            _In_ int previousDragOverIndex);

        // used to access the private moved indexes tuple
        std::vector<MovedItem>::iterator begin()
        {
            return m_movedItems.begin();
        }

        std::vector<MovedItem>::iterator end()
        {
            return m_movedItems.end();
        }

        unsigned int size()
        {
            return static_cast<unsigned int>(m_movedItems.size());
        }

        void clear()
        {
            m_movedItems.clear();
        }

    private:
        // stores the indexes of the items that have been moved
        // and their original layout information
        std::vector<MovedItem> m_movedItems;

        void RemoveMovedItems(
            _In_ const int from,
            _In_ const int to,
            _Inout_ std::vector<MovedItem>& oldItemsToMoveBack);

        void AddMovedItems(
            _In_ bool isOrientationVertical,
            _In_ const int from,
            _In_ const std::vector<MovedItem>& newItems,
            _Inout_ std::vector<MovedItem>& newItemsToMove);
    };

} } }