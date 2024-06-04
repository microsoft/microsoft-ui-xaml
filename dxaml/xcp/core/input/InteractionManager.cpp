// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "InteractionManager.h"

// Comparison operators for InteractionMapKey lookups
bool operator<(const CUIElement* lhs, const InteractionMapKey& rhs)
{
    return lhs < rhs.GetInteractionElement();
}
bool operator<(const InteractionMapKey& lhs, const CUIElement* rhs)
{
    return lhs.GetInteractionElement() < rhs;
}
bool operator<(const InteractionMapKey& lhs, const InteractionMapKey& rhs)
{
    return lhs.GetInteractionElement() < rhs.GetInteractionElement();
}

bool operator==(const CUIElement* lhs, const InteractionMapKey& rhs)
{
    return lhs == rhs.GetInteractionElement();
}
bool operator==(const InteractionMapKey& rhs, const CUIElement* lhs)
{
    return lhs == rhs.GetInteractionElement();
}
bool operator==(const InteractionMapKey& lhs, const InteractionMapKey& rhs)
{
    return lhs.GetInteractionElement() == rhs.GetInteractionElement();
}

_Check_return_ HRESULT
CInteractionManager::GetInteractionEngine(
    _In_ CUIElement* pInteractionElement,
    _In_opt_  CUIElement* pActiveInteractionElement,
    _In_ bool fManipulationOnly,
    _Outptr_ ElementGestureTracker** ppInteractionContext)
{
    IFCEXPECT_ASSERT_RETURN(pInteractionElement);
    IFCEXPECT_ASSERT_RETURN(ppInteractionContext);
    *ppInteractionContext = nullptr;

    bool isTapEnabled = fManipulationOnly ? false : pInteractionElement->IsTapEnabled();
    bool isDoubleTapEnabled = fManipulationOnly ? false : pInteractionElement->IsDoubleTapEnabled();
    bool isRightTapEnabled = fManipulationOnly ? false : pInteractionElement->IsRightTapEnabled();
    bool isHoldEnabled = fManipulationOnly ? false : pInteractionElement->IsHoldEnabled();
    auto manipulationMode = CustomManipulationModes(pInteractionElement->GetManipulationMode());

    // Get a ICM engine that associated the current interaction element
    auto iter = m_mapInteractions.find(pInteractionElement);
    if (iter == m_mapInteractions.end())
    {
        // If we don't already have one, find one that we can re-use, which means that either
        // the UIElement associated with it is no longer in the live tree, or the ElementGestureTracker
        // is idle.
        auto interactionIdleContext = GetReusableInteractionEngine(pInteractionElement, pActiveInteractionElement);
        if (interactionIdleContext)
        {
            // Reuse the existing idling ICM 
            IFC_RETURN(interactionIdleContext->ResetAndReinitialize(
                pInteractionElement,
                isTapEnabled,
                isDoubleTapEnabled,
                isRightTapEnabled,
                isHoldEnabled,
                manipulationMode));
        }
        else
        {
            // Create a ICM from the specified interaction element. We use a special destructor because
            // we don't simply just delete this object if it's in the middle of processing a touch interaction
            interactionIdleContext = unique_interactioncontext(new ElementGestureTracker(
                pInteractionElement,
                isTapEnabled,
                isDoubleTapEnabled,
                isRightTapEnabled,
                isHoldEnabled,
                manipulationMode));
        }

        auto entry = m_mapInteractions.emplace(InteractionMapKey(pInteractionElement), std::move(interactionIdleContext));
        iter = entry.first;
    }

    // Reset ICM if it is stopped before
    if (iter->second && iter->second->IsStopped())
    {
        IFC_RETURN(iter->second->ResetAndReinitialize(
            pInteractionElement,
            isTapEnabled,
            isDoubleTapEnabled,
            isRightTapEnabled,
            isHoldEnabled,
            manipulationMode));
    }

    *ppInteractionContext = iter->second.get();

    return S_OK;
}

bool CInteractionManager::IsDisposable(const std::pair<InteractionMapKey, CInteractionManager::unique_interactioncontext>& entry)
{
    // Make sure we don't destroy an ElementGestureTracker while it's busy processing input.  This can happen in re-entrancy
    // scenarios when we're synchronously firing a GestureRecognizer event to the app, and app pumps messages.
    return !entry.second->IsProcessingInput() && 
        (!entry.first.GetInteractionElement()->IsActive() || entry.second->IsIdle());
}

CInteractionManager::unique_interactioncontext
CInteractionManager::GetReusableInteractionEngine(
    _In_ CUIElement* pInteractionElement,
    _In_opt_  CUIElement* pActiveInteractionElement)
{
    unique_interactioncontext reusedContext;

    // Don't try to find a recyclable interaction engine if the interaction element is the active one
    if (pInteractionElement != pActiveInteractionElement)
    {
        // Find a idle ICM engine from the running ICM contexts. Only use one that isn't either the interactionElement
        // or activeInteractionElement
        auto it = std::find_if(m_mapInteractions.begin(), m_mapInteractions.end(), [pInteractionElement, pActiveInteractionElement](const auto& entry) {
            return 
                IsDisposable(entry) &&
                pInteractionElement != entry.first.GetInteractionElement() &&
                pActiveInteractionElement != entry.first.GetInteractionElement();
        });

        if (it != m_mapInteractions.end())
        {
            // Reset the ICM on the idling element. Swap it out of the map because we are re-using
            // it and don't want to destroy it.
            reusedContext.swap(it->second);
            reusedContext->Reset();
            m_mapInteractions.erase(it);
        }
    }
    return reusedContext;
}

void
CInteractionManager::DestroyInteractionIdlingEngine()
{
    // Find the count of idle elements in our map
    auto idleCount = std::count_if(m_mapInteractions.begin(), m_mapInteractions.end(), [](const auto& entry) {
        return IsDisposable(entry);
    });

    auto iter = m_mapInteractions.begin();
    
    // When we "destroy" the idle engine, we want to keep at least MAX_ICM_IDLING_COUNT items in the map to be reused
    // at a later time. So, while we have more than MAX_ICM_IDLING_COUNT that are idle and we haven't reached the end of this map,
    // we will remove them.
    while (idleCount > MAX_ICM_IDLING_COUNT && iter != m_mapInteractions.end())
    {
        if (IsDisposable(*iter))
        {
            // Remove the idle pair in the map and decrement the idle count
            iter = m_mapInteractions.erase(iter);
            --idleCount;
        }
        else
        {
            ++iter;
        }
    }
}

void
CInteractionManager::DestroyAllInteractionEngine()
{
    m_mapInteractions.clear();
}

void
CInteractionManager::DestroyInteractionEngine(_In_ CUIElement* pDestroyElement)
{
    auto iter = m_mapInteractions.find(pDestroyElement);

    // Skip the destruction if the interaction engine was pegged.
    if (iter != m_mapInteractions.end() && !iter->second->IsPegged())
    {
        m_mapInteractions.erase(iter);
    }
}

bool
CInteractionManager::IsManipulationInertiaProcessing()
{
    if (!m_mapInteractions.empty())
    {
        for (const auto& entry : m_mapInteractions)
        {
            if (entry.second->IsInertial())
            {
                return true;
            }
        }
    }

    return false;
}

void
CInteractionManager::ProcessManipulationInertiaInteraction()
{
    for (const auto& entry : m_mapInteractions)
    {
        if (entry.second->IsInertial())
        {
            entry.second->ProcessInertiaInteraction();
        }
    }
}

void
CInteractionManager::StopInteraction(_In_ CUIElement* element, bool callbackForManipulationCompleted)
{
    auto iter = m_mapInteractions.find(element);

    if (iter == m_mapInteractions.end()) return;

    // Win8: bCallbackForManipulationCompleted is always False.
    // WinBlue: bCallbackForManipulationCompleted is True when XCP_POINTERCAPTURECHANGED is handled.
    if (!callbackForManipulationCompleted || iter->second->IsManipulationStarted())
    {
        iter->second->Stop(callbackForManipulationCompleted);
    }
}

void
CInteractionManager::StopAllInteractions()
{
    for (const auto& [key, interactionContext] : m_mapInteractions)
    {
        interactionContext->Stop(false /* callbackForManipulationCompleted */);
    }
}