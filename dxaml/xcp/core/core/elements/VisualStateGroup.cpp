// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "Storyboard.h"
#include "Animation.h"
#include "TimelineCollection.h"
#include "TimeSpan.h"
#include <VisualTransitionCompletedData.h>
#include <VisualStateGroup.h>
#include <VisualStateCollection.h>

using namespace DirectUI;

CVisualStateGroup::CVisualStateGroup(CCoreServices *pCore)
    : CDependencyObject(pCore)
    , m_pTransitions(nullptr)
    , m_pVisualStates(nullptr)
    , m_pCurrentState(nullptr)
    , m_pEventList(nullptr)
{}

CVisualStateGroup::~CVisualStateGroup()
{
    ReleaseInterface(m_pVisualStates);
    ReleaseInterface(m_pTransitions);
    ReleaseInterface(m_pCurrentState);

    if (m_pEventList)
    {
        m_pEventList->Clean();
        delete m_pEventList;
    }
    m_pEventList = nullptr;

    m_CurrentlyRunningStoryboards.clear();
}

_Check_return_ HRESULT
CVisualStateGroup::EnterImpl(_In_ CDependencyObject *pNamescopeOwner, _In_ EnterParams params)
{
    IFC_RETURN(CDependencyObject::EnterImpl(pNamescopeOwner, params));

    // If there are events registered on this element, ask the
    // EventManager to extract them and a request for every event.
    if (params.fIsLive && m_pEventList)
    {
        CEventManager* pEventManager = GetContext()->GetEventManager();
        ASSERT(pEventManager);
        IFC_RETURN(pEventManager->AddRequestsInOrder(this, m_pEventList));
    }

    return S_OK;
}

_Check_return_ HRESULT
CVisualStateGroup::LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, _In_ LeaveParams params)
{
    if (params.fIsLive)
    {
        IFC_RETURN(CleanupRunningStoryboardsOnLeave());
    }

    IFC_RETURN(CDependencyObject::LeaveImpl(pNamescopeOwner, params));

    if (params.fIsLive && m_pEventList)
    {
        auto pTemp = m_pEventList->GetHead();
        while (pTemp)
        {
            IFC_RETURN(GetContext()->GetEventManager()->RemoveRequest(this, pTemp->m_pData));
            pTemp = pTemp->m_pNext;
        }
    }
    return S_OK;
}

_Check_return_ HRESULT
CVisualStateGroup::AddEventListener(
    _In_ EventHandle hEvent,
    _In_ CValue *pValue,
    _In_ XINT32 iListenerType,
    _Out_opt_ CValue *pResult,
    _In_ bool fHandledEventsToo)
{
    return CEventManager::AddEventListener(this, &m_pEventList, hEvent, pValue, iListenerType, pResult, fHandledEventsToo);
}

_Check_return_ HRESULT
CVisualStateGroup::RemoveEventListener(
    _In_ EventHandle hEvent,
    _In_ CValue *pValue)
{
    return CEventManager::RemoveEventListener(this, m_pEventList, hEvent, pValue);
}

_Check_return_ HRESULT
CVisualStateGroup::GetCurrentVisualState(_Outptr_result_maybenull_ CVisualState **ppVisualState)
{
    if(m_pCurrentState)
    {
        *ppVisualState = m_pCurrentState;
        (*ppVisualState)->AddRef();
    }
    else
    {
        *ppVisualState = nullptr;
    }

    return S_OK;
}

_Check_return_ HRESULT
CVisualStateGroup::SetCurrentVisualState(_In_opt_ CVisualState *pVisualState)
{
    ReleaseInterface(m_pCurrentState);
    m_pCurrentState = pVisualState;

    if (m_pCurrentState)
    {
        m_pCurrentState->AddRef();
    }
    return S_OK;
}

_Check_return_ HRESULT
CVisualStateGroup::NotifyVisualStateEvent(
    _In_ CVisualState *pOldState,
    _In_ CVisualState *pNewState,
    _In_ VisualStateGroupEvent EventState,
    _In_ CControl *pControl)
{
    EventHandle hEvent;

    CEventManager* pEventManager = GetContext()->GetEventManager();
    ASSERT(pEventManager);

    if (!IsActive())
    {
        return S_OK;
    }

    if (VisualStateEventChanged == EventState)
    {
        hEvent.index = KnownEventIndex::VisualStateGroup_CurrentStateChanged;
    }
    else if (VisualStateEventChanging == EventState)
    {
        hEvent.index = KnownEventIndex::VisualStateGroup_CurrentStateChanging;
    }

    if (ShouldRaiseEvent(hEvent))
    {
        xref_ptr<CVisualStateChangedEventArgs> spArgs;
        spArgs.init(new CVisualStateChangedEventArgs(pOldState, pNewState, pControl));

        // To maintain SL5 comptability, the VisualStateChanged/Changing events need to arrive with their sender set
        // to the template root, not the VSG
        xref_ptr<CUIElement> spSenderOverride;
        if (pControl)
        {
            pControl->GetImplementationRoot(spSenderOverride.ReleaseAndGetAddressOf());
        }

        pEventManager->Raise(hEvent, TRUE /* bRefire */, this, spArgs, FALSE /*fRaiseSync */, FALSE /*fInputEvent*/, TRUE /*bAllowErrorFallback*/, spSenderOverride);
    }

    return S_OK;
}

#pragma region Legacy VSM Code
// None of this code is used in XBFv2-based VSM implementations. It is considered dead code
// only kept for strict back-compat reasons. No VSM written with XBFv2 will use these code paths,
// only VSMs created using text parsing or from XBFv1 will use this code.

_Check_return_ HRESULT CVisualStateGroup::CleanupRunningStoryboardsOnLeave()
{
    for (auto& storyboard : m_CurrentlyRunningStoryboards)
    {
        // The data will be used in the handler for the Completed event,
        // so only delete if the event hasn't already been fired
        if (storyboard->IsCompletedEventFired() == FALSE)
        {
            if (storyboard->m_pVisualTransitionCompletedData)
            {
                CVisualTransition *pTransition = storyboard->m_pVisualTransitionCompletedData->GetVisualTransition();
                if (pTransition)
                {
                    IFC_RETURN(storyboard->RemoveEventListener(
                        EventHandle(KnownEventIndex::Timeline_Completed),
                        &storyboard->m_pVisualTransitionCompletedData->m_EventListenerToken));
                }
            }
            storyboard->m_pVisualTransitionCompletedData.reset();
        }

        // Finite animations will complete. Infinite animations will continue to run (this call no-ops).
        IFC_RETURN(storyboard->SkipToFill());
    }

    return S_OK;
}

HRESULT _Check_return_ CVisualStateGroup::FindStateByName(_In_z_ const WCHAR *pStateName, _Inout_ CVisualState **ppVisualState)
{
    HRESULT hr = S_OK;
    CVisualState *pState = NULL;
    XUINT32 nStateCount = 0;
    XUINT32 nStateNameLen = 0;

    *ppVisualState = nullptr;
    IFCEXPECT(m_pVisualStates);

    nStateCount = m_pVisualStates->GetCount();
    nStateNameLen = xstrlen(pStateName);
    for(XUINT32 nStateIndex = 0; nStateIndex < nStateCount; nStateIndex++)
    {
        hr = DoPointerCast(pState, m_pVisualStates->GetItemDOWithAddRef(nStateIndex));
        if(
            (hr == S_OK) &&
            pState &&
            !pState->m_strName.IsNull() &&
            (nStateNameLen == pState->m_strName.GetCount()) &&
            (0 == xstrncmp(pStateName, pState->m_strName.GetBuffer(), nStateNameLen))
            )
            {
                *ppVisualState = pState;
                break; //this will skip our release below and so we'll hold on to the ref
            }
        ReleaseInterface(pState);
    }
Cleanup:
    return hr;
}

HRESULT _Check_return_ CVisualStateGroup::StartNewThenStopOld(
    _In_ CFrameworkElement * pControl,
    _In_ XINT32 cNewStoryboards,
    _In_reads_(cNewStoryboards)CStoryboard** rgNewStoryboards
    )
{
    HRESULT hr = S_OK;
    HRESULT hrTemp = S_OK;
    bool bResetTemporaryNamescopeParent = false;
    int Index = 0;
    CTimeSpan* pZeroTime = NULL;

    CREATEPARAMETERS cp(GetContext());
    IFC(CTimeSpan::Create((CDependencyObject **)&pZeroTime, &cp));

    // Start the new Storyboards
    //
    for (Index = 0; Index < cNewStoryboards; Index++)
    {
        if (! rgNewStoryboards[Index])
        {
            continue;
        }

        // HACK: we wish to redirect namescoping for SL4 release. Code should be removed for SL5.
        CVisualState * pVisualState = do_pointer_cast<CVisualState>(rgNewStoryboards[Index]->GetParent());
        if (pVisualState)
        {
            if (pControl->GetParent())
            {
                // limit to usercontrol scenarios
                CUserControl * pUC = do_pointer_cast<CUserControl>(pControl->GetParent());
                if (pUC)
                {
                    pVisualState->SetTemporaryNamescopeParent(pControl);
                    bResetTemporaryNamescopeParent = TRUE;
                }
            }
        }
        hrTemp = S_OK;
        IFCONTINUE(EnsureStoryboardSafeForStart(pControl, rgNewStoryboards[Index]));
        if (SUCCEEDED(hrTemp))
        {
            IFCONTINUE(rgNewStoryboards[Index]->BeginPrivate(TRUE));
        }
        if (SUCCEEDED(hrTemp))
        {
            IFCONTINUE(rgNewStoryboards[Index]->SeekAlignedToLastTick(pZeroTime));
        }

        if (bResetTemporaryNamescopeParent)
        {
            pVisualState->SetTemporaryNamescopeParent(NULL);
            bResetTemporaryNamescopeParent = FALSE;
        }
        hr = hrTemp;
        if (hr == E_DO_INHERITANCE_CONTEXT_NEEDED)
        {
            IFC_NOTRACE(hr);
        }
        else
        {
            IFC(hr);
        }
    }

    // Stop the old Storyboards
    for (auto& storyboard : m_CurrentlyRunningStoryboards)
    {
        // The data will be used in the handler for the Completed event,
        // so only delete if the event hasn't already been fired
        if (storyboard->IsCompletedEventFired() == FALSE)
        {
            if (storyboard->m_pVisualTransitionCompletedData)
            {
                CVisualTransition *pTransition = storyboard->m_pVisualTransitionCompletedData->GetVisualTransition();
                if (pTransition)
                {
                    IFC(storyboard->RemoveEventListener(
                        EventHandle(KnownEventIndex::Timeline_Completed),
                        &storyboard->m_pVisualTransitionCompletedData->m_EventListenerToken));
                }
            }
            storyboard->m_pVisualTransitionCompletedData.reset();
        }

        IFC(storyboard->StopPrivate());
    }

    // Cleanup the SB list (which removes the references we added to them below)
    m_CurrentlyRunningStoryboards.clear();

    for (Index = 0; Index < cNewStoryboards; Index++)
    {
        if (rgNewStoryboards[Index])
        {
            m_CurrentlyRunningStoryboards.push_back(xref_ptr<CStoryboard>(rgNewStoryboards[Index]));
        }
    }

Cleanup:
    ReleaseInterface(pZeroTime);
    RRETURN(hr);
}

HRESULT
_Check_return_
CVisualStateGroup::EnsureStoryboardSafeForStart(_In_ CFrameworkElement* pControl, _In_ CStoryboard* pSB)
{
    HRESULT hr = S_OK;
    CDependencyObject *pDO = NULL;
    CTimeline *pTimeline = NULL;
    CAnimation *pAnimation = NULL;
    CDependencyObject *pKeyFrame = NULL;

    IFCEXPECT(pControl && pSB);

    //first check to see if the Storyboard is associated
    //if it is, then it's already live, so early out
    if(!pSB->IsAssociated() && !pSB->InheritanceContextWalkForVsmProcessed())
    {
        // If the storyboard is not live, and there is a Binding listening to InheritanceContext changed
        // on one of its animations, or keyframes inside of the animation, GoToState needs to be recalled
        // after InheritanceContext changed is raised synchronously.  It is not safe to raise
        // the event synchronously here so we error out and allow the VisualStateManager
        // to InheritanceContextChanged synchronously from the managed side.
        if (pSB->m_pChild && pSB->m_pChild->GetCount() > 0)
        {
            for (XUINT32 i=0; i<pSB->m_pChild->GetCount(); i++)
            {
                pDO = pSB->m_pChild->GetItemDOWithAddRef(i);
                if (pDO->GetWantsInheritanceContextChanged())
                {
                    IFC_NOTRACE(static_cast<HRESULT>(E_DO_INHERITANCE_CONTEXT_NEEDED));
                }

                // The children of the storyboard are timelines
                // we need to ensure if the timeline is an animation
                // before looking at its keyframes
                pTimeline = do_pointer_cast<CTimeline>(pDO);
                if (pTimeline && pTimeline->IsAnimation())
                {
                    // Can't use metadata checks here because in our metadata
                    // the animations derive from TIMELINE.
                    // At this point, because of the IsAnimation override we know that
                    // this is an animation, since we can't use metadata to do the
                    // conversion we will just use a static cast.
                    pAnimation = static_cast<CAnimation *>(pTimeline);
                    if (pAnimation->m_fUsesKeyFrames)
                    {
                        CDOCollection *pKeyFrames = pAnimation->GetKeyFrameCollection();

                        for (XUINT32 j = 0; j < pKeyFrames->GetCount(); j++)
                        {
                            pKeyFrame = pKeyFrames->GetItemDOWithAddRef(j);

                            if (pKeyFrame->GetWantsInheritanceContextChanged())
                            {
                                // We found that we have an inheritance context
                                IFC_NOTRACE(static_cast<HRESULT>(E_DO_INHERITANCE_CONTEXT_NEEDED));
                            }

                            ReleaseInterface(pKeyFrame);
                        }
                    }
                }

                ReleaseInterface(pDO);
            }
            IFC(hr);
        }
    }
Cleanup:

    // Ensure that we only do the inheritance walk once
    ASSERT(hr == S_OK || hr == E_DO_INHERITANCE_CONTEXT_NEEDED);
    pSB->SetInheritanceContextWalkForVsmProcessed(TRUE);

    ReleaseInterface(pDO);
    ReleaseInterface(pKeyFrame);
    RRETURN(hr);
}
#pragma endregion
