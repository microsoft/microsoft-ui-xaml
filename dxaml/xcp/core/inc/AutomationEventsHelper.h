// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// According to the documentation, because the implementation of StructureChanged events depends on
// the underlying UI framework, UI Automation defines no strict rule governing when a provider must
// switch from sending individual ChildAdded or ChildRemoved events to the bulk equivalent.
// However, the switch typically occurs when two to five child elements are added or removed at
// once. Historically, though, XAML has set the distinction at twenty elements. The bulk events
// help to prevent clients from being flooded by individual ChildAdded and ChildRemoved events.
#define AP_BULK_CHILDREN_LIMIT      20

#include <vector_map.h>
#include <xref_ptr.h>

#include "AutomationPeer.h"

// This class takes care of the way we fire automation events. The StructureChanged automation
// event is raised when the structure of the UI Automation tree changes or, in other words, when UI
// elements are added or removed, or when they become visible or hidden.
class AutomationEventsHelper
{
public:
    enum StructureChangedType
    {
        Added,
        Removed,
        Reordered
    };

private:
    // A simple representation of a given child-parent relationship in the context of
    // AutomationPeers by their runtime IDs.
    class ChildParentKey
    {
    public:
        ChildParentKey(int childId, int parentId)
            : m_childId(childId)
            , m_parentId(parentId)
        {
        }

        bool operator<(const ChildParentKey& other) const
        {
            return m_childId < other.m_childId
                || (m_childId == other.m_childId
                    && m_parentId < other.m_parentId);
        }

    private:
        int m_childId;
        int m_parentId;
    };

    // A simple class to manage the lifetime of AutomationPeers as we queue them up in order to
    // fire automation events on them in the future.
    class PeggedAutomationPeer
    {
    public:
        PeggedAutomationPeer(_In_ CAutomationPeer* automationPeer);
        ~PeggedAutomationPeer();
        PeggedAutomationPeer(const PeggedAutomationPeer&);
        PeggedAutomationPeer& operator=(const PeggedAutomationPeer&);
        PeggedAutomationPeer(PeggedAutomationPeer&& other) noexcept;
        PeggedAutomationPeer& operator=(PeggedAutomationPeer&& other) noexcept;

        const xref_ptr<CAutomationPeer>& GetAutomationPeer() const { return m_automationPeer; }

    private:
        xref_ptr<CAutomationPeer> m_automationPeer;
        bool m_isPegged;
    };

    class StructureChangedRequest
    {
    public:
        StructureChangedRequest(
            _In_ CAutomationPeer* automationPeer,
            _In_ CAutomationPeer* parent,
            StructureChangedType type)
            : m_automationPeer(automationPeer)
            , m_parent(parent)
            , m_type(type)
        {
        }

        // Explicitly deleting copy constructors and copy assignment operators; these do not align
        // with how this class is supposed to be used.
        StructureChangedRequest(const StructureChangedRequest&) = delete;
        StructureChangedRequest& operator=(const StructureChangedRequest&) = delete;

        StructureChangedRequest(StructureChangedRequest&&) noexcept = default;
        StructureChangedRequest& operator=(StructureChangedRequest&&) noexcept = default;

        const xref_ptr<CAutomationPeer>& GetAutomationPeer() const
        {
            return m_automationPeer.GetAutomationPeer();
        }

        const xref_ptr<CAutomationPeer>& GetParent() const
        {
            return m_parent.GetAutomationPeer();
        }

        StructureChangedType GetType() const
        {
            return m_type;
        }

        void SetType(StructureChangedType type) { m_type = type; }

    private:
        PeggedAutomationPeer m_automationPeer;
        PeggedAutomationPeer m_parent;
        StructureChangedType m_type;
    };

    class StructureChangedEventInformation
    {
    public:
        StructureChangedEventInformation(
            _In_ CAutomationPeer* automationPeer,
            _In_ CAutomationPeer* child,
            StructureChangedType type)
            : m_automationPeer(automationPeer)
        {
            RegisterChild(child, type);
        }

        // Explicitly deleting copy constructors and copy assignment operators; these do not align
        // with how this class is supposed to be used.
        StructureChangedEventInformation(const StructureChangedEventInformation&) = delete;
        StructureChangedEventInformation& operator=(const StructureChangedEventInformation&) = delete;

        StructureChangedEventInformation(StructureChangedEventInformation&&) noexcept = default;
        StructureChangedEventInformation& operator=(StructureChangedEventInformation&&) = default;

        const xref_ptr<CAutomationPeer>& GetAutomationPeer() const
        {
            return m_automationPeer;
        }

        void RaiseStructureChangedEvent() const;
        void RegisterChild(_In_ CAutomationPeer* child, StructureChangedType type);

    private:
        xref_ptr<CAutomationPeer> m_automationPeer;
        std::vector<xref_ptr<CAutomationPeer>> m_childrenAdded;
        std::vector<xref_ptr<CAutomationPeer>> m_childrenRemoved;
    };

public:
    AutomationEventsHelper() = default;

    // Explicitly deleting copy constructors and copy assignment operators; these do not align
    // with how this class is supposed to be used.
    AutomationEventsHelper(const AutomationEventsHelper&) = delete;
    AutomationEventsHelper& operator=(const AutomationEventsHelper&) = delete;

    AutomationEventsHelper(AutomationEventsHelper&&) noexcept = default;
    AutomationEventsHelper& operator=(AutomationEventsHelper&&) noexcept = default;

    void CleanUpStructureChangedRequests();

    void RegisterForStructureChangedEvent(
        _In_ CAutomationPeer* automationPeer,
        _In_ CAutomationPeer* parent,
        StructureChangedType type);

    void RaiseStructureChangedEvents();

private:
    // The StructureChanged event is based on a child-parent relationship. For example, a
    // StructureChanged event of the type ChildRemoved, fires on the parent and passes the runtime
    // ID of the child as an argument. Since we should only fire the event at a point in time when
    // visual tree is stable, this means that, even though a given element can enter and leave
    // multiple times while the tree is changing, ultimately what matters is the final state of the
    // element once the tree has stabilized. For example, if an element gets added to the tree,
    // removed, and then added again, in the end a single StructureChanged event of the type
    // ChildAdded should be fired and nothing else. In other words, for a given child-parent
    // relationship, only one StructureChanged event can be fired, and this is the reason why we
    // are keying off a ChildParentKey. In fact, the only reason why we cannot key off just the
    // child ID is because of the scenario where a given child gets removed from the tree and then
    // reparented somewhere else, in which case we must fire two StructureChanged events, one of
    // the type ChildRemoved and the other of the type ChildAdded. Note that the order in which the
    // events are fired is not important.
    containers::vector_map<ChildParentKey, StructureChangedRequest> m_pendingStructureChangedRequests;
    containers::vector_map<ChildParentKey, StructureChangedRequest> m_commitedStructureChangedRequests;
    containers::vector_map<int, StructureChangedEventInformation> m_eventInformation;
};