// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ModernCollectionBasePanel.g.h"

using namespace DirectUI;

// Works around disruptive macros.
#undef min
#undef max

ModernCollectionBasePanel::ContainerContentChangingIterator::ContainerContentChangingIterator()
    : m_pMCBP(nullptr)
    , m_direction(xaml_controls::PanelScrollingDirection::PanelScrollingDirection_None)
    , m_visibleStartInVector(-1)
    , m_visibleEndInVector(-1)
    , m_cacheEndInVector(-1)
    , m_currentIndex(-1)
{
}

_Check_return_ HRESULT 
ModernCollectionBasePanel::ContainerContentChangingIterator::Initialize(_In_ ModernCollectionBasePanel *pPanel)
{
    m_pMCBP = pPanel;
    
    // the cache/visible indicies will be fetched from the panel
    // notice how they are not guaranteed not to be stale: one scenario in which they are 
    // plain and simply wrong is when you collapse/remove a panel from the tree and start 
    // mutating the collection. Arrange will not get a chance to run after the mutation and
    // if we are still registered to do work, we will not be able to fetch the container.

    // translate to array indices
    if (m_pMCBP->m_lastCacheIndexBase > -1)  // -1 means there is nothing.. no visible or cached containers
    {
        IFC_RETURN(m_pMCBP->get_PanningDirectionBase(&m_direction));

        const int firstVisibleIndexBase = m_pMCBP->m_firstVisibleIndexBase;
        const int lastVisibleIndexBase = m_pMCBP->m_lastVisibleIndexBase;
        const int firstCacheIndexBase = m_pMCBP->m_firstCacheIndexBase;
        const int lastCacheIndexBase = m_pMCBP->m_lastCacheIndexBase;

        m_visibleStartInVector = firstVisibleIndexBase > -1 ? firstVisibleIndexBase - firstCacheIndexBase : 0;
        m_visibleEndInVector = lastVisibleIndexBase > -1 ? lastVisibleIndexBase - firstCacheIndexBase : m_visibleStartInVector;
        m_cacheEndInVector = lastCacheIndexBase - firstCacheIndexBase;
    }
    else
    {
        m_direction = xaml_controls::PanelScrollingDirection::PanelScrollingDirection_None;
        m_visibleStartInVector = -1;
        m_visibleEndInVector = -1;
        m_cacheEndInVector = -1;
    }

    return Reset();
}

IFACEMETHODIMP
ModernCollectionBasePanel::ContainerContentChangingIterator::get_Size(_Out_ UINT* pValue) /*override*/
{
    ASSERT(m_pMCBP);
    *pValue = m_cacheEndInVector + 1;    
    return S_OK;
}

IFACEMETHODIMP
ModernCollectionBasePanel::ContainerContentChangingIterator::get_Current(
    _Outptr_result_maybenull_ xaml::IDependencyObject** ppValue) /*override*/
{
    *ppValue = nullptr;
    const int size = m_cacheEndInVector + 1;
    const BOOLEAN forward = m_direction != xaml_controls::PanelScrollingDirection::PanelScrollingDirection_Backward;
    int currentPositionInVector;

    ASSERT(m_pMCBP);
    ASSERT(0 <= m_currentIndex && m_currentIndex < size);

    if (forward)
    {
        currentPositionInVector = m_currentIndex + m_visibleStartInVector;
        
        // We go forward until you reach the end of the forward buffer, and start in the opposite buffer
        // going in the opposite direction
        if (currentPositionInVector > m_cacheEndInVector)
        {
            // processing to the left
            // m_visibleStartInVector - (currentPositionInVector - m_cacheEndInVector) == m_cacheEndInVector - m_currentIndex
            currentPositionInVector = m_cacheEndInVector - m_currentIndex;
        }
    }
    else
    {
        currentPositionInVector = m_visibleEndInVector - m_currentIndex;

        // We go backward until you reach the start of the backward buffer, and start in the opposite buffer
        // going in the opposite direction
        if (currentPositionInVector < 0)
        {
            // processing to the right
            // m_visibleEndInVector + (-currentPositionInVector) == m_currentIndex
            currentPositionInVector = m_currentIndex;
        }
    }

    ASSERT(0 <= currentPositionInVector && currentPositionInVector < size);
    return m_pMCBP->ContainerFromIndex(m_pMCBP->m_firstCacheIndexBase + currentPositionInVector, ppValue);
}

IFACEMETHODIMP
ModernCollectionBasePanel::ContainerContentChangingIterator::MoveNext(
    _Out_ BOOLEAN* pReturnValue) /*override*/
{
    ASSERT(m_pMCBP);

    const int size = m_cacheEndInVector + 1;

    ++m_currentIndex;
    m_currentIndex = std::max(std::min(m_currentIndex, size), -1);
    *pReturnValue = (0 <= m_currentIndex && m_currentIndex < size);   

    return S_OK;
}

IFACEMETHODIMP
ModernCollectionBasePanel::ContainerContentChangingIterator::Reset() /*override*/
{
    ASSERT(m_pMCBP);
    m_currentIndex = -1;
    return S_OK;
}

_Check_return_ HRESULT
ModernCollectionBasePanel::ContainerContentChangingIterator::QueryInterfaceImpl(
    _In_ REFIID iid, 
    _Outptr_ void** ppInterface) /*override*/
{
    if (iid == __uuidof(IContainerContentChangingIterator))
    {
        *ppInterface = static_cast<IContainerContentChangingIterator*>(this);
        AddRefOuter();
        return S_OK;
    }
    return ComBase::QueryInterfaceImpl(iid, ppInterface);
}

_Check_return_ HRESULT
ModernCollectionBasePanel::GetContainersForIncrementalVisualizationImpl(
    _Outptr_ DirectUI::IContainerContentChangingIterator** ppReturnValue) /*override*/
{
    *ppReturnValue = nullptr;

    if (m_lastCacheIndexBase > -1)  // -1 means there is nothing.. no visible or cached containers
    {
        if (m_spContainerContentChangingIterator)
        {
            IFC_RETURN(m_spContainerContentChangingIterator->Initialize(this));
        }
        else
        {
            IFC_RETURN(ctl::make(this, &m_spContainerContentChangingIterator));
        }

        IFC_RETURN(m_spContainerContentChangingIterator.CopyTo(ppReturnValue));
    }
    else
    {
        // Early out. We will return nullptr to signal there isn't any work for CCC to do.
    }

    return S_OK;
}
