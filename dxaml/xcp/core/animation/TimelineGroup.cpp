// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "TimelineGroup.h"
#include "TimelineCollection.h"
#include "Storyboard.h"
#include "DynamicTimeline.h"
#include "ParallelTimeline.h"

CTimelineGroup::~CTimelineGroup()
{
    DetachChildren();
}

bool CTimelineGroup::HasChildren()
{
   return (m_pChild != NULL) && (m_pChild->GetCount() != 0);
}

_Check_return_ HRESULT CTimelineGroup::Create(
    _Outptr_ CDependencyObject **ppObject,
    _In_ CREATEPARAMETERS *pCreate)
{
    return E_FAIL;
}

// Releases the children collection and ensures that children's back pointers are released.
void CTimelineGroup::DetachChildren()
{
   // Remove our back pointers
   if (m_pChild)
   {
       XUINT32 nCount = m_pChild->GetCount();
       for (XUINT32 i = 0; i < nCount; i++)
       {
           CTimeline *pTimeline = (CTimeline *)m_pChild->GetItemWithAddRef(i);
           if ( pTimeline )
           {
               pTimeline->SetTimingParent( NULL );
           }
           ReleaseInterface(pTimeline);
       }

       // clear timing owner
       m_pChild->SetTimingOwner(NULL);

       if (m_pChild->GetParentInternal(false) == this)
       {
           IGNOREHR(m_pChild->RemoveParent(this));
       }
   }
   // Release collection reference
   ReleaseInterface(m_pChild);
}

// Override to base SetValue to allow adding a child timeline
_Check_return_ HRESULT CTimelineGroup::SetValue(_In_ const SetValueParams& args)
{
    // See if it is of the correct type to add it to our list of children.

    if (args.m_pDP->GetIndex() == KnownPropertyIndex::Storyboard_Children || args.m_pDP->GetIndex() == KnownPropertyIndex::DynamicTimeline_Children)
    {
        if (args.m_value.GetType() != valueObject)
        {
            IFC_RETURN(E_INVALIDARG);
        }

        ASSERT(args.m_value.AsObject());

        CTimelineCollection* pTimelineCollection;
        DetachChildren();
        IFC_RETURN(CTimeline::SetValue(args));
        IFC_RETURN(DoPointerCast(pTimelineCollection, args.m_value.AsObject()));
        pTimelineCollection->SetTimingOwner(this);
    }
    else
    {
        IFC_RETURN(CTimeline::SetValue(args));
    }

    return S_OK;
}

_Check_return_ HRESULT CTimelineGroup::AddChild(_In_ CTimeline *pChild)
{
    if (pChild == NULL)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    if (pChild->IsAssociated())
    {
        // We cannot set a new value that is already associated to another element
        IFC_RETURN(E_INVALIDARG);
    }

    if (m_pChild == NULL)
    {
        CREATEPARAMETERS cp(GetContext());
        IFC_RETURN(CTimelineCollection::Create((CDependencyObject **) &m_pChild, &cp));
        IFCPTR_RETURN(m_pChild);
        m_pChild->SetTimingOwner(this);

        // Be the timeline's parent, so that the managed lifetime code
        // can track it.
        IFC_RETURN(m_pChild->AddParent(this, FALSE));
    }

    IFC_RETURN(m_pChild->Append(pChild));

    // Keep a parent reference in the DO parent pointer - this is safe
    // since timing nodes are never part of the visual tree.
    pChild->SetTimingParent(this);

    return S_OK;
}

_Check_return_ HRESULT CTimelineGroup::RemoveChild(_In_ CTimeline *pChild)
{

    if (pChild == NULL)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    IFCPTR_RETURN(m_pChild);
    xref_ptr<CTimeline> ptrItem;
    ptrItem.attach((CTimeline *)m_pChild->Remove(pChild));
    IFCPTR_RETURN(ptrItem);
    ptrItem->SetTimingParent(NULL);


    return S_OK;
}

_Check_return_ HRESULT CTimelineGroup::ComputeStateImpl(
    _In_ const ComputeStateParams &parentParams,
    _Inout_ ComputeStateParams &myParams,
    _Inout_opt_ bool *pHasNoExternalReferences,
    bool hadIndependentAnimationLastTick,
    _Out_ bool *pHasIndependentAnimation
    )
{
    DirectUI::ClockState previousState = m_clockState;

    // Compute local state
    bool hasIndependentAnimation;
    IFC_RETURN(CTimeline::ComputeStateImpl(
        parentParams,
        myParams,
        pHasNoExternalReferences,
        hadIndependentAnimationLastTick,
        &hasIndependentAnimation
        ));

    // Compute child timelines
    if (m_pChild)
    {
        // Compute the state of the timing children when:
        //   - the timeline hasn't started yet
        //      (e.g. We need to compute children to clone them for the composition thread in case they are independent animations with BeginTimes set)
        //   - the timeline is active
        //      (e.g. The children animations need to update their targets' values)
        //   - the timeline has transitioned to filling
        //      (e.g. The children timelines need to update their values one last time and raise Completed events)
        //   - a child needs a pending theme change to be processed
        //   - this is the final tick of the entire timing tree
        //      (see CAnimation::ShouldTickOnUIThread for detailed comment)
        const bool shouldComputeChildState = m_clockState == DirectUI::ClockState::NotStarted
            || m_clockState == DirectUI::ClockState::Active
            || (m_clockState == DirectUI::ClockState::Filling && previousState != DirectUI::ClockState::Filling)
                                           || HasPendingThemeChange()
                                           || !GetRootTimingParent()->IsInActiveState();

        // The current time of this timeline is used as the baseline for each child timeline to calculate its relative progress.
        // If this timeline hasn't started, the current time is not valid and there's no meaningful way for children to calculate
        // relative progress either. This is okay since all the children will be in the 'not started' state as well.
        // If the root storyboard has stopped, then its current time has been reset to 0 by FinalizeIteration and is not valid. Its
        // children should not be active, so tick them with no parent time.
        // Note: A child with a negative BeginTime will not actually start sooner - it will be limited by its parent starting first.
        //       The negative BeginTime will just adjust the progress ahead once the animation has started.
        // Note: We check the state of the root storyboard explicitly. If this is a nested storyboard, it could be Filling because
        //       the Stopped root ticked it at time infinity. We also check the clock state explicitly rather than use IsInStoppedState(),
        //       because a storyboard that naturally expires with FillBehavior.Stop will not be IsInStoppedState.
        myParams.hasTime =
            m_clockState != DirectUI::ClockState::NotStarted
            && GetRootTimingParent()->m_clockState != DirectUI::ClockState::Stopped;
        if (myParams.hasTime)
        {
            myParams.time = m_rCurrentTime;
        }

        for (auto& pDOChild : m_pChild->GetCollection())
        {
            auto pTimelineNoRef = static_cast<CTimeline*>(pDOChild);

            if (shouldComputeChildState)
            {
                IFC_RETURN(pTimelineNoRef->ComputeState(myParams, pHasNoExternalReferences));
            }

            // A TimelineGroup has independent animations if any of its children does.
            if (pTimelineNoRef->HasIndependentAnimation())
            {
                hasIndependentAnimation = TRUE;
            }
        }
    }

    *pHasIndependentAnimation = hasIndependentAnimation;

    return S_OK;
}

_Check_return_ HRESULT CTimelineGroup::FinalizeIteration()
{
    if (m_clockState != DirectUI::ClockState::Stopped)
    {
        m_clockState = DirectUI::ClockState::Stopped;
    }


// Notify children
    if (m_pChild)
    {
        XUINT32 nCount = m_pChild->GetCount();
        for (XUINT32 i = 0; i < nCount; i++)
        {
            CTimeline *pTimeline = (CTimeline *)m_pChild->GetItemWithAddRef(i);
            
            // REVIEW:  What is the desired behavior here?
            //
            //          The original comment on this was that we wanted to process all the children
            //          before returning an error, however, because we process an IFC just before
            //          the cleanup, we never end up returning that error as hr gets tromped on.
            //
            //          If this is the correct behavior, then we can just ignore the HR and maintain 
            //          compatibility with prior versions.
            //
            //          However, if we want to do what the comment suggested (and be consistent with
            //          other functions in this module), then we should be returning a failed HR if any
            //          one of the FinalizeIteration calls return a failed HR.  If this is desired, 
            //          then we need to decided if ReleaseTarget and CTimeline::FinalizeIteration 
            //          should be called if there was an error in the loop.
            IGNOREHR(pTimeline->FinalizeIteration());
            ReleaseInterface(pTimeline);
        }
    }

    // Release the target object.  In the case of an explicitly set target,
    // we have another pointer through m_pManualTargetObjectWeakRef.
    ReleaseTarget();

    IFC_RETURN(CTimeline::FinalizeIteration());

    return S_OK;
}

_Check_return_ HRESULT CTimelineGroup::OnAddToTimeManager()
{
    IFC_RETURN( CTimeline::OnAddToTimeManager() );

    // Notify children
    if (m_pChild)
    {
        XUINT32 nCount = m_pChild->GetCount();
        HRESULT hrError  = S_OK;
        for (XUINT32 i = 0; i < nCount; i++)
        {
            CTimeline *pTimeline = (CTimeline *)m_pChild->GetItemWithAddRef(i);
            // Process all the children before returning an error
            HRESULT hrChild = pTimeline->OnAddToTimeManager();
            if ( FAILED(hrChild) )
            {
                hrError = hrChild;
            }
            ReleaseInterface(pTimeline);
        }
        IFC_RETURN(hrError);
    }

    return S_OK;
}

_Check_return_ HRESULT CTimelineGroup::OnRemoveFromTimeManager()
{
    IFC_RETURN( CTimeline::OnRemoveFromTimeManager() );

    // Notify children
    if (m_pChild)
    {
        XUINT32 nCount = m_pChild->GetCount();
        HRESULT hrError = S_OK;
        for (XUINT32 i = 0; i < nCount; i++)
        {
            CTimeline *pTimeline = (CTimeline *)m_pChild->GetItemWithAddRef(i);
            // Process all the children before returning an error
            HRESULT hrChild = pTimeline->OnRemoveFromTimeManager();
            if ( FAILED(hrChild) )
            {
                hrError = hrChild;
            }
            ReleaseInterface(pTimeline);
        }
        IFC_RETURN(hrError);
    }

    return S_OK;
}

bool CTimelineGroup::IsFinite()
{
    bool isFiniteAnimation = true;

    if (!CTimeline::IsFinite())
    {
        return false;
    }

    // If we have children, and we're finite, then this timeline is finite only if all children are finite.
    if (m_pChild)
    {
        XUINT32 nCount = m_pChild->GetCount();
        for (XUINT32 i = 0; i < nCount && isFiniteAnimation; ++i)
        {
            auto pTimeline = static_cast<CTimeline *>(m_pChild->GetItemDOWithAddRef(i));

            if (!pTimeline->IsFinite())
            {
                isFiniteAnimation = FALSE;
            }

            ReleaseInterface(pTimeline);
        }
    }

    return isFiniteAnimation;
}
