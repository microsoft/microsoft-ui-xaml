// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <VisualTransitionCompletedData.h>

CVisualTransition* VisualTransitionCompletedData::GetVisualTransition() const
{
    return m_pTransitionWeakRef.lock();
}

_Check_return_ HRESULT
VisualTransitionCompletedData::Create(
    _In_ int groupIndex,
    _In_ int stateIndex,
    _In_ CVisualTransition* transition,
    _Outptr_ VisualTransitionCompletedData** transitionCompletedData)
{
    std::unique_ptr<VisualTransitionCompletedData> visualTransitionCompletedData(
        new VisualTransitionCompletedData());

    visualTransitionCompletedData->m_pTransitionWeakRef = xref::get_weakref(transition);

    visualTransitionCompletedData->m_groupIndex = groupIndex;
    visualTransitionCompletedData->m_stateIndex = stateIndex;

    *transitionCompletedData = visualTransitionCompletedData.release();

    return S_OK;
}

#pragma region Legacy VSM Code
// None of this code is used in XBFv2-based VSM implementations. It is considered dead code
// only kept for strict back-compat reasons. No VSM written with XBFv2 will use these code paths,
// only VSMs created using text parsing or from XBFv1 will use this code.
_Check_return_ HRESULT
VisualTransitionCompletedData::Create(
    _In_ CVisualTransition* pTransition,
    _In_ CControl*          pRoot,
    _In_ CFrameworkElement* pImplRoot,
    _In_opt_ CVisualState*  pOldVisualState,
    _In_ CVisualState*      pNewVisualState,
    _In_ CVisualStateGroup* pGroup,
    _Outptr_ VisualTransitionCompletedData** ppVisualTransitionCompletedData)
{
    std::unique_ptr<VisualTransitionCompletedData> visualTransitionCompletedData(
        new VisualTransitionCompletedData());

    // Store weak references to prevent a cycle. For example, VisualTransitionCompletedData
    // is stored in a storyboard that may belong to pGroup. And pRoot, pImplRoot own pGroup.
    visualTransitionCompletedData->m_pTransitionWeakRef = xref::get_weakref(pTransition);
    visualTransitionCompletedData->m_pRootWeakRef = xref::get_weakref(pRoot);
    visualTransitionCompletedData->m_pImplRootWeakRef = xref::get_weakref(pImplRoot);
    visualTransitionCompletedData->m_pOldVisualStateWeakRef = xref::get_weakref(pOldVisualState);
    visualTransitionCompletedData->m_pNewVisualStateWeakRef = xref::get_weakref(pNewVisualState);
    visualTransitionCompletedData->m_pGroupWeakRef = xref::get_weakref(pGroup);

    // Keep alive until the transition completed.
    if (pGroup->HasManagedPeer())
    {
        IFC_RETURN(pGroup->PegManagedPeer(/* isShutdownException */ TRUE));
        visualTransitionCompletedData->m_peggedPeer = true;
    }

    *ppVisualTransitionCompletedData = visualTransitionCompletedData.release();
    return S_OK;
}

CControl* VisualTransitionCompletedData::GetRoot() const
{
    return m_pRootWeakRef.lock();
}

CFrameworkElement* VisualTransitionCompletedData::GetImplRoot() const
{
    return m_pImplRootWeakRef.lock();
}

CVisualState* VisualTransitionCompletedData::GetNewVisualState() const
{
    return m_pNewVisualStateWeakRef.lock();
}

CVisualState* VisualTransitionCompletedData::GetOldVisualState() const
{
    return m_pOldVisualStateWeakRef.lock();
}

CVisualStateGroup* VisualTransitionCompletedData::GetVisualStateGroup() const
{
    return m_pGroupWeakRef.lock();
}

VisualTransitionCompletedData::~VisualTransitionCompletedData()
{
    CVisualStateGroup* pGroup = GetVisualStateGroup();
    if (pGroup && pGroup->HasManagedPeer() && m_peggedPeer)
    {
        pGroup->UnpegManagedPeer(/* isShutdownException */ TRUE);
    }
}
#pragma endregion
