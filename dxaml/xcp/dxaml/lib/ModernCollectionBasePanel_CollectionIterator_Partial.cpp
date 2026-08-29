// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ModernCollectionBasePanel.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

// Initializes the iterator at the beginning.
ModernCollectionBasePanel::CollectionIterator::CollectionIterator(
    _In_ const CacheManager& cache)
    : m_cache(cache)
{
    Init(0, cache.IsGrouping() ? xaml_controls::ElementType_GroupHeader : xaml_controls::ElementType_ItemContainer);
}

// Initializes the iterator at a location in the collection.
ModernCollectionBasePanel::CollectionIterator::CollectionIterator(
    _In_ const CacheManager& cache, 
    _In_ INT index, 
    _In_ xaml_controls::ElementType type)
    : m_cache(cache)
{
    Init(index, type);
}

// Used to initializes or re-initializes the iterator.
void ModernCollectionBasePanel::CollectionIterator::Init(
    _In_ INT index, 
    _In_ xaml_controls::ElementType type)
{
    m_current.type = type;

    if(m_current.isHeader())
    {
        // Don't use this to iterate over an empty collection.
        ASSERT(index >= 0 && index < m_cache.GetTotalGroupCount());

        m_current.groupIndex = index;
        UpdateCurrentGroupInformation();
    }
    // isHeader is false if we are starting at a container or if we are
    // iterating over a non grouped collection.
    else
    {
        // Don't use this to iterate over an empty collection.
        ASSERT(index >= 0 && index < m_cache.GetTotalItemCount());

        m_current.itemIndex = index;

        if(m_cache.IsGrouping())
        {
            INT indexInsideGroup = 0;

            VERIFYHR(m_cache.GetGroupInformationFromItemIndex(
                        m_current.itemIndex,
                        &m_current.groupIndex,
                        &indexInsideGroup,
                        &m_current.itemCountInGroup));

            m_current.startItemIndex = m_current.itemIndex - indexInsideGroup;
        }
        // Non grouped collections are treated as a big group, except we stop at the first container
        // when iterating backward.
        else
        {
            m_current.groupIndex = 0;
            m_current.startItemIndex = 0;
            m_current.itemCountInGroup = m_cache.GetTotalItemCount();
        }
    }
}

// Try moving to the next element. Retruns true if it succeeds.
BOOLEAN ModernCollectionBasePanel::CollectionIterator::MoveNext()
{
    BOOLEAN canMove = TRUE;

    if(m_current.isHeader())
    {
        // If the current element is a header, the next element is a
        // container if the current group is not empty
        if(m_current.itemCountInGroup > 0)
        {
            // m_current.itemIndex is already set.
            m_current.type = xaml_controls::ElementType_ItemContainer;
        }
        // if the current group is empty, then the next element is a
        // header assuming there is one.
        else if(m_current.groupIndex < m_cache.GetTotalGroupCount() - 1)
        {
            // m_current.itemIndex won't need to change for an empty group.
            ++m_current.groupIndex;
            UpdateCurrentGroupInformation();
        }
        // Otherwise, we are at the end.
        else
        {
            canMove = FALSE;
        }
    }
    else
    {
        // If the current element is a container, the next element is also
        // a container if there are more items in the current group.
        if(m_current.IndexInGroup() < m_current.itemCountInGroup - 1)
        {
            ++m_current.itemIndex;
        }
        // If we are the last container of the current group, the next element
        // is a header assuming we are not at the end already.
        else if(m_current.groupIndex < m_cache.GetTotalGroupCount() - 1)
        {
            m_current.type = xaml_controls::ElementType_GroupHeader;
            ++m_current.groupIndex;
            UpdateCurrentGroupInformation();
        }
        // Otherwise, we are at the end.
        else
        {
            canMove = FALSE;
        }
    }

    return canMove;
}

// Try moving to the previous element. Retruns true if it succeeds.
BOOLEAN ModernCollectionBasePanel::CollectionIterator::MovePrevious()
{
    BOOLEAN canMove = TRUE;

    if(m_current.isHeader())
    {
        // If we are already at the beginning, there is nowhere to move.
        if(m_current.groupIndex == 0)
        {
            canMove = FALSE;
        }
        // Otherwise, retrieve information about the previous group.
        else
        {
            --m_current.groupIndex;
            UpdateCurrentGroupInformation();

            // If the previous (now current!) group is empty, we are done.
            // Otherwise, if it's not empty, we will move to the last container.
            if(m_current.itemCountInGroup > 0)
            {
                m_current.type = xaml_controls::ElementType_ItemContainer;
                m_current.itemIndex = m_current.startItemIndex + m_current.itemCountInGroup - 1;
            }
        }
    }
    else
    {
        // If we are a container, the previous element is also a container if
        // we are not the first container of the group.
        if(m_current.IndexInGroup() > 0)
        {
            --m_current.itemIndex;
        }
        // Otherwise, it's the header of the current group if we are iterating over a 
        // grouped collection.
        else if(m_cache.IsGrouping())
        {
            m_current.type = xaml_controls::ElementType_GroupHeader;
        }
        else
        {
            canMove = FALSE;
        }

    }

    return canMove;
}

