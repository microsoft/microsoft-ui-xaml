// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ModernCollectionBasePanel.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

// Work around disruptive max/min macros
#undef max
#undef min

const DWORD ModernCollectionBasePanel::TransitionContextManager::incrementalLoadingDuration = 500;
const DWORD ModernCollectionBasePanel::TransitionContextManager::fastmutationThreshold = 1000;

_Check_return_ HRESULT ModernCollectionBasePanel::GetChildTransitionContext(
    _In_ xaml::IUIElement* element,
    _In_ INT layoutTickId,
    _Out_ ThemeTransitionContext* returnValue)
{
    HRESULT hr = S_OK;
    BOOLEAN isWrapping = FALSE;

    bool isHeader = static_cast<UIElement*>(element)->GetVirtualizationInformation()->GetIsHeader();
    if (!isHeader)
    {
        IFC(m_spLayoutStrategy->GetIsWrappingStrategy(&isWrapping));
    }

    IFC(m_transitionContextManager.GetTransitionContext(layoutTickId, isWrapping, returnValue));

Cleanup:
    RRETURN(hr);
}


// determines if mutations are going fast
IFACEMETHODIMP ModernCollectionBasePanel::IsCollectionMutatingFast(
    _Out_ BOOLEAN* pReturnValue)
{
    RRETURN(m_transitionContextManager.IsCollectionMutatingFast(pReturnValue));
}

_Check_return_ HRESULT ModernCollectionBasePanel::GetChildTransitionBounds(
    _In_ xaml::IUIElement* element,
    _Out_ wf::Rect* returnValue)
{
    HRESULT hr = S_OK;
    INT itemIndex = -1;
    ASSERT(!GetVirtualizationInformationFromElement(element)->GetIsHeader());
    IFC(m_containerManager.IICM_IndexFromContainer(static_cast<UIElement*>(element), &itemIndex));

    if (m_cacheManager.IsGrouping())
    {
        INT groupIndex = -1;

        IFC(m_cacheManager.GetGroupInformationFromItemIndex(itemIndex, &groupIndex, nullptr /* pIndexInGroup */, nullptr /* pItemCountInGroup */));

        IFC(m_spLayoutStrategy->GetElementTransitionsBounds(
            xaml_controls::ElementType_GroupHeader,
            groupIndex,
            m_windowState.GetRealizationWindow(),
            returnValue));
    }
    else
    {
        IFC(m_spLayoutStrategy->GetElementTransitionsBounds(
            xaml_controls::ElementType_ItemContainer,
            itemIndex,
            m_windowState.GetRealizationWindow(),
            returnValue));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ModernCollectionBasePanel::NotifyOfItemsReorderedImpl(
    _In_ UINT nCount)
{
    RRETURN(m_transitionContextManager.NotifyOfItemsReordered(nCount));
}


ModernCollectionBasePanel::TransitionContextManager::TransitionContextManager(_In_ ModernCollectionBasePanel* owner)
    : m_owner(owner)    // no ref needed, TransitionContextManager is not at all exposed to the outside world
    , m_elementCountAddedThisLayoutTick(0)
    , m_elementCountRemovedThisLayoutTick(0)
    , m_elementCountReorderedThisLayoutTick(0)
    , m_resetItemsThisLayoutTick(FALSE)
    , m_shouldIncrementallyLoad(FALSE)
    , m_isLoadingStarted(FALSE)
    , m_isLoading(FALSE)
    , m_currentTickCounterId(0)
    , m_startIncrementallyLoadingTime(0)
    , m_currentMutationTime(0)
    , m_previousMutationTime(0)
{
}

_Check_return_ HRESULT
ModernCollectionBasePanel::TransitionContextManager::NotifyOfItemsChanging(
    _In_ wfc::CollectionChange action)
{
    HRESULT hr = S_OK;
    IFC(UpdateContextCounters(true /* isUpdateDueToMutation */));

    switch (action)
    {
        case wfc::CollectionChange_Reset:
        {
            m_resetItemsThisLayoutTick = TRUE;
            break;
        }
        case wfc::CollectionChange_ItemInserted:
        {
            m_elementCountAddedThisLayoutTick += 1;
            // incremental loading should show Entrance transitions.
            m_isLoading = m_shouldIncrementallyLoad;
            break;
        }
        case wfc::CollectionChange_ItemRemoved:
        {
            m_elementCountRemovedThisLayoutTick += 1;
            break;
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ModernCollectionBasePanel::TransitionContextManager::NotifyOfItemsChanged(
    _In_ wfc::CollectionChange action)
{
    HRESULT hr = S_OK;
    RRETURN(hr);
}

_Check_return_ HRESULT
ModernCollectionBasePanel::TransitionContextManager::NotifyOfGroupsChanged(
    _In_ wfc::CollectionChange action)
{
    HRESULT hr = S_OK;
    IFC(UpdateContextCounters(true /* isUpdateDueToMutation */));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ModernCollectionBasePanel::TransitionContextManager::NotifyOfItemsReordered(
    _In_ UINT nCount)
{
    HRESULT hr = S_OK;
    IFC(UpdateContextCounters(true /* isUpdateDueToMutation */));
    m_elementCountReorderedThisLayoutTick += nCount;

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ModernCollectionBasePanel::TransitionContextManager::IsCollectionMutatingFast(_Out_ BOOLEAN* pFast)
{
    HRESULT hr = S_OK;
    DWORD currentTime = ::GetTickCount();

    *pFast = m_previousMutationTime != 0 && currentTime - m_previousMutationTime < fastmutationThreshold;

    RRETURN(hr);
}

_Check_return_ HRESULT
ModernCollectionBasePanel::TransitionContextManager::UpdateContextCounters(bool isUpdateDueToMutation)
{
    HRESULT hr = S_OK;
    XINT16 currentTickCounterId = 0;
    DWORD currentTime = ::GetTickCount();

    // load should no longer come into play
    m_isLoading = FALSE;

    if (m_shouldIncrementallyLoad)
    {
        DWORD timeDelta = 0;

        if (currentTime < m_startIncrementallyLoadingTime)
        {
            // we pass 49.7 days and started all over again
            timeDelta = std::numeric_limits<DWORD>::max() - m_startIncrementallyLoadingTime + currentTime;
        }
        else
        {
            timeDelta = currentTime - m_startIncrementallyLoadingTime;
        }

        if (timeDelta > incrementalLoadingDuration)
        {
            m_shouldIncrementallyLoad = FALSE;
        }
    }

    IFC(CoreImports::LayoutManager_GetLayoutTickForTransition(static_cast<CUIElement*>(m_owner->GetHandle()), &currentTickCounterId));

    // update timers
    if (isUpdateDueToMutation && currentTickCounterId != m_currentTickCounterId)
    {
        // The reason we only do it when it is on separate ticks is because changes get
        // coalesced into multi-add or multi-delete contexts, which we do have animations for.
        // It really is only a problem if they happen in separate ticks.

        // different tick, store time and move current to previous
        m_previousMutationTime = m_currentMutationTime;
        m_currentMutationTime = currentTime;
    }

    if (!m_shouldIncrementallyLoad)
    {
        if (currentTickCounterId != m_currentTickCounterId)
        {
            m_elementCountAddedThisLayoutTick = 0;
            m_elementCountRemovedThisLayoutTick = 0;
            m_elementCountReorderedThisLayoutTick = 0;
            m_resetItemsThisLayoutTick = FALSE;
        }

        m_currentTickCounterId = currentTickCounterId;
    }


Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ModernCollectionBasePanel::TransitionContextManager::GetTransitionContext(
    _In_ INT32 layoutTickId,
    _In_ BOOLEAN isWrapping,
    _Out_ ThemeTransitionContext* returnValue)
{
    HRESULT hr = S_OK;
    ThemeTransitionContext themeTransitionContext = ThemeTransitionContext::None;

    // loaded is an async event, so the tick might not come in until later
    if (!m_isLoadingStarted)
    {
        m_currentTickCounterId = layoutTickId;
        m_isLoadingStarted = TRUE;
        m_isLoading = TRUE;
        if (m_shouldIncrementallyLoad)
        {
            m_startIncrementallyLoadingTime = ::GetTickCount();
        }
    }
    else
    {
        if (m_currentTickCounterId != layoutTickId)
        {
            IFC(UpdateContextCounters(false /* isUpdateDueToMutation */));
            m_isLoading = m_shouldIncrementallyLoad;
        }
    }

    // If layoutTick is same as the one during Load, call EntraceTransition
    if (m_isLoading)
    {
        // Entrance Transition should happen only on First time, this animation runs only first time
        themeTransitionContext = ThemeTransitionContext::Entrance;
    }
    else
    {
        if (m_resetItemsThisLayoutTick)
        {
            // if content is changed, the colelction is reset or dataSource is changed, this animation runs
            themeTransitionContext = ThemeTransitionContext::ContentTransition;
        }
        // Reorder Ticks checks are above add/remove because if there is reorder, it will update the ItemsSource
        // which eventually updates m_elementCountAddedThisLayoutTick/m_elementCountRemovedThisLayoutTick
        else if (m_elementCountReorderedThisLayoutTick == 1)
        {
            // if single reorder happens
            themeTransitionContext = isWrapping ? ThemeTransitionContext::SingleReorderGrid : ThemeTransitionContext::SingleReorderList;
        }
        else if (m_elementCountReorderedThisLayoutTick > 1)
        {
            // if multiple reorder happens
            themeTransitionContext = isWrapping ? ThemeTransitionContext::MultipleReorderGrid : ThemeTransitionContext::MultipleReorderList;
        }
        else if (m_elementCountAddedThisLayoutTick == 1 && m_elementCountRemovedThisLayoutTick == 0)
        {
            // if only 1 item gets added or removed
            themeTransitionContext = isWrapping ? ThemeTransitionContext::SingleAddGrid : ThemeTransitionContext::SingleAddList;
        }
        else if (m_elementCountAddedThisLayoutTick == 0 && m_elementCountRemovedThisLayoutTick == 1)
        {
            themeTransitionContext = isWrapping ? ThemeTransitionContext::SingleDeleteGrid : ThemeTransitionContext::SingleDeleteList;
        }
        else if (m_elementCountAddedThisLayoutTick >= 1 && m_elementCountRemovedThisLayoutTick >= 1)
        {
            // If there is a mix
            themeTransitionContext = isWrapping ? ThemeTransitionContext::MixedOperationsGrid : ThemeTransitionContext::MixedOperationsList;
        }
        else if (m_elementCountAddedThisLayoutTick > 0)
        {
            // if multiple items gets added/removed
            themeTransitionContext = isWrapping ? ThemeTransitionContext::MultipleAddGrid : ThemeTransitionContext::MultipleAddList;
        }
        else if (m_elementCountAddedThisLayoutTick == 0 && m_elementCountRemovedThisLayoutTick > 0)
        {
            // if multiple items gets added/removed
            themeTransitionContext = isWrapping ? ThemeTransitionContext::MultipleDeleteGrid : ThemeTransitionContext::MultipleDeleteList;
        }
    }

    *returnValue = themeTransitionContext;

Cleanup:
    RRETURN(hr);
}
