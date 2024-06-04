// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "LiveReorderHelper.h"

using namespace DirectUI::Components;
using namespace DirectUI::Components::LiveReorderHelper;

// this function returns the items that should be moved back to their original location
// and the items that should be moved to their new location
// the function makes sure that items that have already been moved are left alone
// and are not moved again
// first we go over the items that have been moved and we check to see if they need to be moved back
// then we go over the rest of the new items (if there are any) and move them to their new location
void MovedItems::Update(
    _In_ bool isOrientationVertical,
    _Inout_ const std::vector<MovedItem>& newItems,
    _Inout_ std::vector<MovedItem>& newItemsToMove,
    _Inout_ std::vector<MovedItem>& oldItemsToMoveBack)
{
    const int newItemsSize = static_cast<int>(newItems.size());
    const int movedItemsSize = static_cast<int>(m_movedItems.size());

    // our items should always be ordered from the dragIndex to the dragOverIndex
    const int newItemsStartIndex = newItems[0].sourceIndex;
    const int newItemsEndIndex = newItems[newItemsSize - 1].sourceIndex;
    const int newItemsIncrement = (newItemsStartIndex < newItemsEndIndex) ? 1 : -1;

    // the index we should use to start adding new items
    int startIndexForAddMovedItems = -1;

    newItemsToMove.clear();
    oldItemsToMoveBack.clear();

    if (movedItemsSize > 0)
    {
        const int movedItemsStartIndex = m_movedItems[0].sourceIndex;
        const int movedItemsEndIndex = m_movedItems[movedItemsSize - 1].sourceIndex;
        const int movedItemsIncrement = (movedItemsStartIndex < movedItemsEndIndex) ? 1 : -1;

        // in the same drag motion, the sourceIndex should always be the same
        ASSERT(newItemsStartIndex == movedItemsStartIndex);

        // remove the indexes that are now out of range of new indexes
        RemoveMovedItems((newItemsIncrement == movedItemsIncrement) ? newItemsSize : 1, movedItemsSize - 1, oldItemsToMoveBack);

        // set the starting index for the items
        // basically here, we are subtracting the two ranges of m_movedItems and newItems
        if (newItemsIncrement == movedItemsIncrement)
        {
            // add the new items which should start after the end of the current moved items
            startIndexForAddMovedItems = movedItemsSize;
        }
        else
        {
            // drag index was already added so no need to go over it again
            startIndexForAddMovedItems = 1;
        }
    }
    else
    {
        // this is the first time we enter this function
        // start from the drag index (located at array index 0)
        startIndexForAddMovedItems = 0;
    }

    // add the new moved items
    if (startIndexForAddMovedItems < newItemsSize)
    {
        AddMovedItems(isOrientationVertical, startIndexForAddMovedItems, newItems, newItemsToMove);
    }
}

// this function makes sure that we drag the right elements into the right space
// taking into account the previously dragged elements
// for example, if the drag starts from index 0, index 1 should only move if we are past the halfway point of index 1
// that is, the insertion index is now 2
// if the drag goes back towards index 1, index 1 should only move back only if we are past the halfwary point of its element
int MovedItems::GetDragOverIndex(
    _In_ int closestElementIndex,
    _In_ int insertionIndex,
    _In_ int previousDragOverIndex)
{
    int dragOverIndex = closestElementIndex;

    if (insertionIndex == closestElementIndex)
    {
        if (previousDragOverIndex < insertionIndex)
        {
            --dragOverIndex;
        }
    }
    else
    {
        if (previousDragOverIndex >= insertionIndex)
        {
            ++dragOverIndex;
        }
    }

    return dragOverIndex;
}

void MovedItems::RemoveMovedItems(
    _In_ const int from,
    _In_ const int to,
    _Inout_ std::vector<MovedItem>& oldItemsToMoveBack)
{
    for (int i = to; i >= from; --i)
    {
        oldItemsToMoveBack.push_back(m_movedItems[i]);
        m_movedItems.erase(m_movedItems.begin() + i);
    }
}

void MovedItems::AddMovedItems(
    _In_ bool isOrientationVertical,
    _In_ const int from,
    _In_ const std::vector<MovedItem>& newItems,
    _Inout_ std::vector<MovedItem>& newItemsToMove)
{
    // move the new indexes that have not been moved yet
    for (auto itr = newItems.begin() + from; itr != newItems.end(); ++itr)
    {
        auto newItem = *itr;

        // if moved items is empty, this means that the first item is the dragged item
        // we simply add it to the list
        if (!m_movedItems.empty())
        {
            // find the item whose sourceIndex is the destinationIndex of the new item to be moved
            auto destination = m_movedItems.end() - 1;

            bool forward = (newItem.sourceIndex > newItem.destinationIndex) ? true : false;

            // if the items are of the same size, use the destinations original location
            // else if the destination is the sourceIndex of the first moved item (meaning it's the dragged item), use its original location
            // otherwise, we use the destination's current (moved) location
            if (newItem.sourceRect.Width == destination->sourceRect.Width && newItem.sourceRect.Height == destination->sourceRect.Height)
            {
                newItem.destinationRect.X = destination->sourceRect.X;
                newItem.destinationRect.Y = destination->sourceRect.Y;
            }
            else if (destination->sourceIndex == m_movedItems[0].sourceIndex)
            {
                newItem.destinationRect.X = destination->sourceRect.X;
                newItem.destinationRect.Y = destination->sourceRect.Y;

                // if they're of different sizes, account for the difference of size
                if (!forward)
                {
                    if (isOrientationVertical)
                    {
                        newItem.destinationRect.Y -= (newItem.sourceRect.Height - destination->sourceRect.Height);

                        if (newItem.destinationRect.Y < 0)
                        {
                            newItem.destinationRect.X = newItem.sourceRect.X;
                            newItem.destinationRect.Y = newItem.sourceRect.Y + newItem.sourceRect.Height;
                        }
                    }
                    else
                    {
                        newItem.destinationRect.X -= (newItem.sourceRect.Width - destination->sourceRect.Width);

                        if (newItem.destinationRect.X < 0)
                        {
                            newItem.destinationRect.X = newItem.sourceRect.X + newItem.sourceRect.Width;
                            newItem.destinationRect.Y = newItem.sourceRect.Y;
                        }
                    }
                }
            }
            else
            {
                newItem.destinationRect.X = destination->destinationRect.X;
                newItem.destinationRect.Y = destination->destinationRect.Y;

                // we should offset the location by the size of the destination in case we're moving forward (sourceIndex > destinationIndex)
                // otherwise, we subtract the size of the item itself
                if (isOrientationVertical)
                {
                    if (forward)
                    {
                        newItem.destinationRect.Y += destination->sourceRect.Height;
                    }
                    else
                    {
                        newItem.destinationRect.Y -= newItem.sourceRect.Height;
                    }
                }
                else
                {
                    if (forward)
                    {
                        newItem.destinationRect.X += destination->sourceRect.Width;
                    }
                    else
                    {
                        newItem.destinationRect.X -= newItem.sourceRect.Width;
                    }
                }
            }

            // set the width and height to its original size
            newItem.destinationRect.Width = newItem.sourceRect.Width;
            newItem.destinationRect.Height = newItem.sourceRect.Height;

            // add the item to the lists of items to be moved
            newItemsToMove.push_back(newItem);
        }

        m_movedItems.push_back(newItem);
    }
}