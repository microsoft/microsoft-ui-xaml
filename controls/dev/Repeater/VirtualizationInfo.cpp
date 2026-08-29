// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "ItemsRepeater.common.h"
#include "VirtualizationInfo.h"
#include "ItemsRepeater.h"

#ifdef DBG
int VirtualizationInfo::s_logItemIndexDbg{ -1 };
#endif // DBG

VirtualizationInfo::VirtualizationInfo()
    : m_arrangeBounds(ItemsRepeater::InvalidRect)
{ }

bool VirtualizationInfo::IsPinned() const
{
    return m_pinCounter > 0u;
}

bool VirtualizationInfo::IsHeldByLayout() const
{
    return m_owner == ElementOwner::Layout;
}

bool VirtualizationInfo::IsInPinnedPool() const
{
    return m_owner == ElementOwner::PinnedPool;
}

bool VirtualizationInfo::IsInUniqueIdResetPool() const
{
    return m_owner == ElementOwner::UniqueIdResetPool;
}

bool VirtualizationInfo::IsRealized() const
{
    return IsHeldByLayout() || IsInPinnedPool();
}

void VirtualizationInfo::UpdatePhasingInfo(int phase, const winrt::IInspectable& data, const winrt::IDataTemplateComponent& component)
{
    m_phase = phase;
    m_data = winrt::make_weak(data);
    m_dataTemplateComponent = winrt::make_weak(component);
}

#pragma region Ownership state machine

void VirtualizationInfo::MoveOwnershipToLayoutFromElementFactory(int index, wstring_view uniqueId)
{
#ifdef DBG
    if (m_index != index)
    {
        if (m_index != -1 && m_index == s_logItemIndexDbg)
        {
            ITEMSREPEATER_TRACE_INFO(nullptr, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"Loses log index.", s_logItemIndexDbg);
        }
        else if (index != -1 && index == s_logItemIndexDbg)
        {
            ITEMSREPEATER_TRACE_INFO(nullptr, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"Gains log index.", s_logItemIndexDbg);
        }
    }
#endif // DBG

    MUX_ASSERT(m_owner == ElementOwner::ElementFactory);
    m_owner = ElementOwner::Layout;
    m_index = index;
    m_uniqueId = uniqueId;
}

void VirtualizationInfo::MoveOwnershipToLayoutFromUniqueIdResetPool()
{
#ifdef DBG
    if (m_index == s_logItemIndexDbg)
    {
        ITEMSREPEATER_TRACE_INFO(nullptr, TRACE_MSG_METH_INT, METH_NAME, this, s_logItemIndexDbg);
        MUX_ASSERT(s_logItemIndexDbg != -1);
    }
#endif // DBG

    MUX_ASSERT(m_owner == ElementOwner::UniqueIdResetPool);
    m_owner = ElementOwner::Layout;
}

void VirtualizationInfo::MoveOwnershipToLayoutFromPinnedPool()
{
#ifdef DBG
    if (m_index == s_logItemIndexDbg)
    {
        ITEMSREPEATER_TRACE_INFO(nullptr, TRACE_MSG_METH_INT, METH_NAME, this, s_logItemIndexDbg);
        MUX_ASSERT(s_logItemIndexDbg != -1);
    }
#endif // DBG

    MUX_ASSERT(m_owner == ElementOwner::PinnedPool);
    MUX_ASSERT(IsPinned());
    m_owner = ElementOwner::Layout;
}

void VirtualizationInfo::MoveOwnershipToElementFactory()
{
#ifdef DBG
    if (m_index == s_logItemIndexDbg && !(m_owner == ElementOwner::Animator && m_index == -1))
    {
        ITEMSREPEATER_TRACE_INFO(nullptr, TRACE_MSG_METH_INT, METH_NAME, this, s_logItemIndexDbg);
        MUX_ASSERT(s_logItemIndexDbg != -1);
    }
#endif // DBG

    MUX_ASSERT(m_owner != ElementOwner::ElementFactory);
    m_owner = ElementOwner::ElementFactory;
    m_pinCounter = 0u;
    m_index = -1;
    m_uniqueId.clear();
    m_arrangeBounds = ItemsRepeater::InvalidRect;
}

void VirtualizationInfo::MoveOwnershipToUniqueIdResetPoolFromLayout()
{
#ifdef DBG
    if (m_index == s_logItemIndexDbg)
    {
        ITEMSREPEATER_TRACE_INFO(nullptr, TRACE_MSG_METH_INT, METH_NAME, this, s_logItemIndexDbg);
        MUX_ASSERT(s_logItemIndexDbg != -1);
    }
#endif // DBG

    MUX_ASSERT(m_owner == ElementOwner::Layout);
    m_owner = ElementOwner::UniqueIdResetPool;
    // Keep the pinCounter the same. If the container survives the reset
    // it can go on being pinned as if nothing happened.
}

void VirtualizationInfo::MoveOwnershipToAnimator()
{
#ifdef DBG
    if (m_index == s_logItemIndexDbg)
    {
        ITEMSREPEATER_TRACE_INFO(nullptr, TRACE_MSG_METH_INT, METH_NAME, this, s_logItemIndexDbg);
        MUX_ASSERT(s_logItemIndexDbg != -1);
    }
#endif // DBG

    // During a unique id reset, some elements might get removed.
    // Their ownership will go from the UniqueIdResetPool to the Animator.
    // The common path though is for ownership to go from Layout to Animator.
    MUX_ASSERT(m_owner == ElementOwner::Layout || m_owner == ElementOwner::UniqueIdResetPool);
    m_owner = ElementOwner::Animator;
    m_index = -1;
    m_pinCounter = 0u;
}

void VirtualizationInfo::MoveOwnershipToPinnedPool()
{
#ifdef DBG
    if (m_index == s_logItemIndexDbg)
    {
        ITEMSREPEATER_TRACE_INFO(nullptr, TRACE_MSG_METH_INT, METH_NAME, this, s_logItemIndexDbg);
        MUX_ASSERT(s_logItemIndexDbg != -1);
    }
#endif // DBG

    MUX_ASSERT(m_owner == ElementOwner::Layout);
    m_owner = ElementOwner::PinnedPool;
}

#pragma endregion

unsigned VirtualizationInfo::AddPin()
{
#ifdef DBG
    if (m_index == s_logItemIndexDbg)
    {
        ITEMSREPEATER_TRACE_INFO(nullptr, TRACE_MSG_METH_INT, METH_NAME, this, s_logItemIndexDbg);
        MUX_ASSERT(s_logItemIndexDbg != -1);
    }
#endif // DBG

    if (!IsRealized())
    {
        throw winrt::hresult_error(E_FAIL, L"You can't pin an unrealized element.");
    }

    return ++m_pinCounter;
}

unsigned VirtualizationInfo::RemovePin()
{
#ifdef DBG
    if (m_index == s_logItemIndexDbg)
    {
        ITEMSREPEATER_TRACE_INFO(nullptr, TRACE_MSG_METH_INT, METH_NAME, this, s_logItemIndexDbg);
        MUX_ASSERT(s_logItemIndexDbg != -1);
    }
#endif // DBG

    if (!IsRealized())
    {
        throw winrt::hresult_error(E_FAIL, L"You can't unpin an unrealized element.");
    }

    if (!IsPinned())
    {
        throw winrt::hresult_error(E_FAIL, L"UnpinElement was called more often than PinElement.");
    }

    return --m_pinCounter;
}

void VirtualizationInfo::UpdateIndex(int newIndex)
{
#ifdef DBG
    if (m_index != newIndex)
    {
        if (m_index != -1 && m_index == s_logItemIndexDbg)
        {
            ITEMSREPEATER_TRACE_INFO(nullptr, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"Loses log index.", s_logItemIndexDbg);
        }
        else if (newIndex != -1 && newIndex == s_logItemIndexDbg)
        {
            ITEMSREPEATER_TRACE_INFO(nullptr, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"Gains log index.", s_logItemIndexDbg);
        }
    }
#endif // DBG

    MUX_ASSERT(IsRealized());
    m_index = newIndex;
}
