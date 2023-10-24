// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "ElementGestureTracker.h"
#include "vector_map.h"

// The InteractionMapKey is used to wrap two common things we do when inserting
// and removing UIElements from the interaction map. Right now, that is pegging/unpegging
// the managed peer.
class InteractionMapKey final
{
public:
    CUIElement* GetInteractionElement() const
    {
        return m_interactionElement.get();
    }

    explicit InteractionMapKey(CUIElement* elem)
        : m_interactionElement(elem)
    {
        m_interactionElement->PegManagedPeer(/*isShutdownException*/true);
    }

    ~InteractionMapKey()
    {
        if (m_interactionElement)
        {
            m_interactionElement->UnpegManagedPeer(/*isShutdownException*/true);
        }
    }

    InteractionMapKey(const InteractionMapKey&) = delete;
    InteractionMapKey& operator=(const InteractionMapKey&) = delete;
    InteractionMapKey(InteractionMapKey&&) = default;
    InteractionMapKey& operator=(InteractionMapKey&& rhs)
    {
        if (this != &rhs)
        {
            if (m_interactionElement)
            {
                m_interactionElement->UnpegManagedPeer(/*isShutdownException*/true);
            }
            m_interactionElement = std::move(rhs.m_interactionElement);
        }
        return *this;
    }

private:
    xref_ptr<CUIElement> m_interactionElement;
};

class CInteractionManager final
{
public:
    CInteractionManager()
    {
    }

    ~CInteractionManager()
    {
        DestroyAllInteractionEngine();
    }

    _Check_return_ HRESULT GetInteractionEngine(
        _In_ CUIElement* pInteractionElement,
        _In_opt_  CUIElement* pActiveInteractionElement,
        _In_ bool fManipulationOnly,
        _Outptr_ ElementGestureTracker** ppInteractionContext);

    void DestroyInteractionIdlingEngine();
    void DestroyAllInteractionEngine();
    void StopInteraction(_In_ CUIElement* element, bool callbackForManipulationCompleted);
    void StopAllInteractions();
    void DestroyInteractionEngine(_In_ CUIElement* pDestroyElement);
    bool IsManipulationInertiaProcessing();
    void ProcessManipulationInertiaInteraction();

private:
    using unique_interactioncontext = std::unique_ptr<ElementGestureTracker, ElementGestureTracker::Deleter>;
    using InteractionMap = containers::vector_map<InteractionMapKey, unique_interactioncontext>;
    unique_interactioncontext GetReusableInteractionEngine(
        _In_ CUIElement* pInteractionElement,
        _In_opt_  CUIElement* pActiveInteractionElement);

    InteractionMap m_mapInteractions;

    static bool IsDisposable(const std::pair<InteractionMapKey, CInteractionManager::unique_interactioncontext>& entry);

    // Specified the max interaction context manager idling count as 10 that base on the 10 finger input
    static const XUINT32 MAX_ICM_IDLING_COUNT  = 10;
};

