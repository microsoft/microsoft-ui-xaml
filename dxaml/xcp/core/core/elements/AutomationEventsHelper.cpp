// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "CValue.h"

#pragma region PeggedAutomationPeer
AutomationEventsHelper::PeggedAutomationPeer::PeggedAutomationPeer(
    _In_ CAutomationPeer* automationPeer)
    : m_automationPeer(automationPeer)
{
    bool isPendingDelete = false;

    automationPeer->TryPegPeer(&m_isPegged, &isPendingDelete);
}

AutomationEventsHelper::PeggedAutomationPeer::~PeggedAutomationPeer()
{
    if (m_isPegged)
    {
        m_automationPeer->UnpegManagedPeer();
        m_isPegged = false;
    }
}

AutomationEventsHelper::PeggedAutomationPeer::PeggedAutomationPeer(
    const PeggedAutomationPeer& other)
    : m_automationPeer(other.m_automationPeer)
{
    bool isPendingDelete = false;

    other.m_automationPeer->TryPegPeer(&m_isPegged, &isPendingDelete);
}

AutomationEventsHelper::PeggedAutomationPeer& AutomationEventsHelper::PeggedAutomationPeer::operator=(
    const PeggedAutomationPeer& other)
{
    if (this != &other)
    {
        if (m_isPegged)
        {
            m_automationPeer->UnpegManagedPeer();
        }

        m_automationPeer.reset();
        m_isPegged = false;

        bool isPendingDelete = false;

        other.m_automationPeer->TryPegPeer(&m_isPegged, &isPendingDelete);
        m_automationPeer = other.m_automationPeer;
    }

    return *this;
}

AutomationEventsHelper::PeggedAutomationPeer::PeggedAutomationPeer(
    PeggedAutomationPeer&& other) noexcept
    : m_automationPeer(nullptr)
    , m_isPegged(false)
{
    m_automationPeer = other.m_automationPeer;
    m_isPegged = other.m_isPegged;

    other.m_automationPeer.reset();
    other.m_isPegged = false;
}

AutomationEventsHelper::PeggedAutomationPeer& AutomationEventsHelper::PeggedAutomationPeer::operator=(
    PeggedAutomationPeer&& other) noexcept
{
    if (this != &other)
    {
        if (m_isPegged)
        {
            m_automationPeer->UnpegManagedPeer();
        }

        m_automationPeer.reset();
        m_isPegged = false;

        m_automationPeer = other.m_automationPeer;
        m_isPegged = other.m_isPegged;

        other.m_automationPeer.reset();
        other.m_isPegged = false;
    }

    return *this;
}
#pragma endregion

#pragma region StructureChangedEventInformation
void AutomationEventsHelper::StructureChangedEventInformation::RaiseStructureChangedEvent() const
{
    // According to the documentation, except for ChildAdded, StructureChanged events are always
    // associated with the container element that holds the children. The ChildAdded event, in
    // particular, is associated with the element that was just added. Additionally, when it comes
    // to the runtime IDs that are passed when raising the event, these correspond to the child
    // elements of the provider node where the tree change occurred, but this parameter is used
    // only in the case of a ChildRemoved event, and it is supposed to be null for all other types
    // of StructureChanged events.
    CAutomationPeer * const automationPeer = GetAutomationPeer();
    CValue runtimeIds;

    if ((m_childrenAdded.size() + m_childrenRemoved.size()) > AP_BULK_CHILDREN_LIMIT)
    {
        // StructureChangeType_ChildrenInvalidated:
        // Child elements were invalidated in the UI Automation element tree. This might mean that
        // one or more child elements were added or removed, or a combination of both. This value
        // can also indicate that one subtree in the UI was substituted for another. For example,
        // the entire contents of a dialog box changed at once, or the view of a list changed
        // because an Explorer-type application navigated to another location. The exact meaning
        // depends on the UI Automation provider implementation.
        runtimeIds.SetNull();
        automationPeer->RaisePropertyChangedEvent(
            UIAXcp::APStructureChangeType_ChildrenInvalidatedProperty,
            runtimeIds,
            runtimeIds);
    }
    else
    {
        if (m_childrenAdded.size() >= AP_BULK_CHILDREN_LIMIT)
        {
            // StructureChangeType_ChildrenBulkAdded:
            // Child elements were added in bulk to the UI Automation element tree.
            runtimeIds.SetNull();
            automationPeer->RaisePropertyChangedEvent(
                UIAXcp::APStructureChangeType_ChildrenBulkAddedProperty,
                runtimeIds,
                runtimeIds);
        }
        else
        {
            // StructureChangeType_ChildAdded:
            // A child element was added to the UI Automation element tree.
            for (auto& child : m_childrenAdded)
            {
                runtimeIds.SetNull();
                child->RaisePropertyChangedEvent(
                    UIAXcp::APStructureChangeType_ChildAddedProperty,
                    runtimeIds,
                    runtimeIds);
            }
        }

        if (m_childrenRemoved.size() >= AP_BULK_CHILDREN_LIMIT)
        {
            // StructureChangeType_ChildrenBulkRemoved:
            // Child elements were removed in bulk from the UI Automation element tree.
            runtimeIds.SetNull();
            automationPeer->RaisePropertyChangedEvent(
                UIAXcp::APStructureChangeType_ChildrenBulkRemovedProperty,
                runtimeIds,
                runtimeIds);
        }
        else
        {
            // StructureChangeType_ChildRemoved:
            // A child element was removed from the UI Automation element tree.
            for (auto& child : m_childrenRemoved)
            {
                const int id = child->GetRuntimeId();
                runtimeIds.WrapSignedArray(1, &id);
                automationPeer->RaisePropertyChangedEvent(
                    UIAXcp::APStructureChangeType_ChildRemovedProperty,
                    runtimeIds,
                    runtimeIds);
            }
        }
    }
}

void AutomationEventsHelper::StructureChangedEventInformation::RegisterChild(
    _In_ CAutomationPeer* child,
    StructureChangedType type)
{
    switch (type)
    {
    case StructureChangedType::Added:
        // We do not need to register more children past the limit used to fire the event in bulk.
        if (m_childrenAdded.size() < AP_BULK_CHILDREN_LIMIT)
        {
            m_childrenAdded.emplace_back(child);
        }
        break;
    case StructureChangedType::Removed:
        // We do not need to register more children past the limit used to fire the event in bulk.
        if (m_childrenRemoved.size() < AP_BULK_CHILDREN_LIMIT)
        {
            m_childrenRemoved.emplace_back(child);
        }
        break;
    case StructureChangedType::Reordered:
        // Historically, XAML has never fired StructureChanged events in response to reorder
        // operations.
        break;
    default:
        IFCFAILFAST(E_INVALIDARG);
    }
}
#pragma endregion

#pragma region AutomationEventsHelper
void AutomationEventsHelper::CleanUpStructureChangedRequests()
{
    m_pendingStructureChangedRequests.clear();
    m_commitedStructureChangedRequests.clear();
    m_eventInformation.clear();

    m_pendingStructureChangedRequests.shrink_to_fit();
    m_commitedStructureChangedRequests.shrink_to_fit();
    m_eventInformation.shrink_to_fit();
}

void AutomationEventsHelper::RaiseStructureChangedEvents()
{
    if (m_pendingStructureChangedRequests.size() > 0)
    {
        for (const auto& request : m_pendingStructureChangedRequests)
        {
            auto parent = request.second.GetParent();
            auto child = request.second.GetAutomationPeer();
            auto type = request.second.GetType();

            const int parentId = parent->GetRuntimeId();
            auto it = m_eventInformation.find(parentId);

            if (it != m_eventInformation.end())
            {
                it->second.RegisterChild(child, type);
            }
            else
            {
                m_eventInformation.emplace(
                    std::piecewise_construct,
                    std::forward_as_tuple(parentId),
                    std::forward_as_tuple(parent, child, type));
            }
        }

        // At this point, we can consider the current requests as committed, and we must now clear the
        // container of requests. Doing so keeps us open to the possibility that new requests might
        // come as we are firing the ones that have been already committed. Given that the requests are
        // the owners of the pegs though, we will keep these objects around for the rest of this
        // function for lifetime purposes. We achieve all of this by swapping the container with an
        // empty one. A swap operation on the container does not invoke any move, copy, or swap
        // operations on individual elements.
        m_pendingStructureChangedRequests.swap(m_commitedStructureChangedRequests);

        for (const auto& info : m_eventInformation)
        {
            info.second.RaiseStructureChangedEvent();
        }

        m_eventInformation.clear();
        m_commitedStructureChangedRequests.clear();
    }
}

void AutomationEventsHelper::RegisterForStructureChangedEvent(
    _In_ CAutomationPeer* automationPeer,
    _In_ CAutomationPeer* parent,
    StructureChangedType type)
{
    const int automationPeerId = automationPeer->GetRuntimeId();
    const int parentId = parent->GetRuntimeId();
    bool isNewEntry = true;

    if (m_pendingStructureChangedRequests.size() > 0)
    {
        auto it = m_pendingStructureChangedRequests.find(ChildParentKey(automationPeerId, parentId));

        if (it != m_pendingStructureChangedRequests.end())
        {
            isNewEntry = false;

            // If the element is already registered, we might need to update the type of
            // StructureChanged event we will fire for it.
            switch (it->second.GetType())
            {
            case StructureChangedType::Added:
                // If the element was previously marked as Added, but got Removed before we could
                // fire the event, the StructureChanged event should not be fired.
                if (type == StructureChangedType::Removed)
                {
                    m_pendingStructureChangedRequests.erase(it);
                }
                break;
            case StructureChangedType::Removed:
                // If the element was previously marked as Removed, but got Added before we could
                // fire the event, the StructureChanged event should not be fired.
                if (type == StructureChangedType::Added)
                {
                    m_pendingStructureChangedRequests.erase(it);
                }
                break;
            case StructureChangedType::Reordered:
                it->second.SetType(type);
                break;
            default:
                IFCFAILFAST(E_INVALIDARG);
            }
        }
    }

    if (isNewEntry)
    {
        // If the element hasn't been registered yet, do so.
        m_pendingStructureChangedRequests.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(automationPeerId, parentId),
            std::forward_as_tuple(automationPeer, parent, type));
    }
}
#pragma endregion
