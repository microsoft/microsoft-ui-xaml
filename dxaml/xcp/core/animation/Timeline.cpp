// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Timeline.h"
#include "Timemgr.h"
#include "RepeatBehavior.h"
#include "PointerAnimationUsingKeyFrames.h"
#include <GraphicsUtility.h>
#include <UIThreadScheduler.h>
#include "TimeSpan.h"

using namespace DirectUI;

bool CTimeline::s_allowDependentAnimations = TRUE;

XFLOAT CTimeline::s_timeTolerance = 0.0005f; // 0.5 milliseconds

namespace CoreImports
{
    DirectUI::ClockState Timeline_GetClockState(_In_ CTimeline *timeline)
    {
        // Determine what the clock state is.
        // The internal state NotStarted maps to the external state DirectUI::ClockState::Active.
        if (timeline->m_clockState == DirectUI::ClockState::NotStarted)
        {
            return DirectUI::ClockState::Active;
        }
        else
        {
            return timeline->m_clockState;
        }
    }

    // Returns the current time in seconds
    double Timeline_GetCurrentTime(_In_ CTimeline *timeline)
    {
        return timeline->m_rCurrentTime;
    }

    // Resolves an object name to the object that it refers to.
    xref_ptr<CDependencyObject> Timeline_ResolveName(
        _In_ CTimeline *timeline,
        _In_ const xstring_ptr& strName)
    {
        xref_ptr<CDependencyObject> resolved;
        timeline->ResolveName(strName, nullptr /* pParentTimeline */, resolved.ReleaseAndGetAddressOf());
        return resolved;
    }
}

CTimeline::~CTimeline()
{
    ReleaseInterface(m_pBeginTime);

    if (m_wucAnimator)
    {
        // Manually call Stop() on the WUC animator to unbind it from the target property. Otherwise, WUC's binding manager
        // could keep the WUC animation alive and playing until someone else sets a different value on the target property.
        // This can lead to infinite animations looping forever even after they've been released by Xaml.
        IGNOREHR(m_wucAnimator->Stop());
    }

    ReleaseTarget();

    ReleaseInterface(m_pDynamicTimelineParent);

    if (m_pEventList)
    {
        // Remove existing events...
        m_pEventList->Clean();
        delete m_pEventList;
        m_pEventList = NULL;
    }
}

KnownTypeIndex CTimeline::GetTypeIndex() const
{
    return DependencyObjectTraits<CTimeline>::Index;
}

// Releases the WeakRef to the target object.
void CTimeline::ReleaseTarget()
{
    DetachDCompAnimationInstancesFromTarget();

    m_targetObjectWeakRef.reset();
    m_pTargetDependencyProperty = nullptr;
}

_Check_return_ HRESULT CTimeline::InitInstance()
{
    auto core = GetContext();
    m_duration = DurationVOHelper::Create(core);
    m_repeatBehavior = RepeatBehaviorVOHelper::Create(core);
    return S_OK;
}

_Check_return_ HRESULT CTimeline::SetValue(_In_ const SetValueParams& args)
{
    // These properties can't be modified unless the root Storyboard is stopped.
    if (args.m_pDP->GetIndex() == KnownPropertyIndex::Storyboard_TargetName ||
        args.m_pDP->GetIndex() == KnownPropertyIndex::Storyboard_TargetProperty)
    {
        IFC_RETURN(CheckCanBeModified());
    }

    IFC_RETURN(CDependencyObject::SetValue(args));

    // Validate parameters
    if (m_rSpeedRatio <= 0.0f)
    {
        m_rSpeedRatio = 1.0f;
        IFC_RETURN(E_INVALIDARG);
    }

    return S_OK;
}

_Check_return_ HRESULT CTimeline::AddEventListener(
    _In_ EventHandle hEvent,
    _In_ CValue *pValue,
    _In_ XINT32 iListenerType,
    _Out_opt_ CValue *pResult,
    _In_ bool fHandledEventsToo)
{
    if (KnownEventIndex::Timeline_Completed == hEvent.index)
    {
        m_completedHandlerRegisteredCount++;
    }

    return CEventManager::AddEventListener(this, &m_pEventList, hEvent, pValue, iListenerType, pResult, fHandledEventsToo);
}

_Check_return_ HRESULT CTimeline::RemoveEventListener(
    _In_ EventHandle hEvent,
    _In_ CValue *pValue)
{
    if (KnownEventIndex::Timeline_Completed == hEvent.index)
    {
        m_completedHandlerRegisteredCount--;
    }

    return CEventManager::RemoveEventListener(this, m_pEventList, hEvent, pValue);
}

_Check_return_ HRESULT CTimeline::ComputeState(
    _In_ const ComputeStateParams &parentParams,
    _Inout_opt_ bool *pHasNoExternalReferences
    )
{
    ComputeStateParams myParams(parentParams);
    myParams.speedRatio *= m_rSpeedRatio;
    myParams.isAncestorWaitingForDCompAnimationCompleted &= m_isWaitingForDCompAnimationCompleted;
    myParams.cannotConvertToCompositionAnimation |= (m_conversionResult != CompositionAnimationConversionResult::Success);

    // Reset independent animation state.  It will be recalculated by this method each tick.
    const bool hadIndependentAnimationLastTick = m_hasIndependentAnimation;
    m_hasIndependentAnimation = FALSE;

    bool hasIndependentAnimationThisTick;
    IFC_RETURN(ComputeStateImpl(
        parentParams,
        myParams,
        pHasNoExternalReferences,
        !!hadIndependentAnimationLastTick,
        &hasIndependentAnimationThisTick
        ));

    m_hasIndependentAnimation = hasIndependentAnimationThisTick;

    // If this timeline has changed from dependent to independent or vice versa, notify the time manager.
    if (hadIndependentAnimationLastTick != hasIndependentAnimationThisTick)
    {
        GetContext()->GetTimeManager()->NotifyIndependentAnimationChange();
    }

    m_hasPendingThemeChange = FALSE;

    return S_OK;
}

_Check_return_ HRESULT CTimeline::ComputeStateImpl(
    _In_ const ComputeStateParams &parentParams,
    _Inout_ ComputeStateParams &myParams,
    _Inout_opt_ bool *pHasNoExternalReferences,
    bool hadIndependentAnimationLastTick,
    _Out_ bool *pHasIndependentAnimation
    )
{
    bool isLocallyReversed = false;

    const XDOUBLE rBeginTime = m_pBeginTime ? m_pBeginTime->m_rTimeSpan : 0.0;

    // At this point the timeline computes all timing computations based off the parent time

    // NotStarted until we gather enough data to determine we are active
    m_clockState = DirectUI::ClockState::NotStarted;
    m_nIteration = 0;

    DurationType durationType;
    XFLOAT rDurationValue;
    GetNaturalDuration(&durationType, &rDurationValue);

    // Compute the expiration time
    COptionalDouble expirationTime;
    ComputeExpirationTime(
        rBeginTime,
        durationType,
        rDurationValue,
        m_rSpeedRatio,  // we need to use our local speed ratio
        &expirationTime);

    // The parent time can be modified by ComputeCurrentClockState, so copy it on the stack if it's valid.
    ComputeStateParams clampedParentParams(parentParams);

    // Determines whether the timeline has started, is running, or has finished.
    bool shouldComputeProgress;
    ComputeCurrentClockState(
        clampedParentParams,
        rBeginTime,
        &expirationTime,
        hadIndependentAnimationLastTick,
        &shouldComputeProgress
        );

    if (IsDurationForProgressDifferent())
    {
        GetDurationForProgress(&durationType, &rDurationValue);
    }

    if (shouldComputeProgress)
    {
        // Compute local progress accounting for iteration count.
        ASSERT(clampedParentParams.hasTime);
        ComputeLocalProgressAndTime(
            rBeginTime,
            clampedParentParams.time,
            durationType,
            rDurationValue,
            &expirationTime,
            hadIndependentAnimationLastTick,
            &isLocallyReversed
            );

        // Fire per-iteration initialization.
        if (!m_fInitialized)
        {
            InitializeIteration();
        }
    }
    else
    {
        // If we haven't yet gone into active mode, skip progress computation.
        ASSERT(m_clockState == DirectUI::ClockState::NotStarted);
    }

    // Update animated values and update the next tick time.
    myParams.isReversed = parentParams.isReversed ^ !!isLocallyReversed;
    bool isIndependentAnimation;
    IFC_RETURN(UpdateAnimation(
        parentParams, // NOTE: the actual parent time is used for scheduling purposes, not the clamped one
        myParams,
        rBeginTime,
        durationType,
        rDurationValue,
        expirationTime,
        &isIndependentAnimation
        ));

    // Check external references
    CheckHasNoExternalRefs(pHasNoExternalReferences);

    *pHasIndependentAnimation = isIndependentAnimation;

    return S_OK;
}

_Check_return_ HRESULT CTimeline::FinalizeIteration()
{
    m_fInitialized = FALSE;
    m_rCurrentTime = 0.0;

    return S_OK;
}

_Check_return_ HRESULT CTimeline::OnAddToTimeManager()
{
    m_fIsInTimeManager = TRUE;
    CEventManager * pEventManager = NULL;

    // If there are events registered on this element, ask the
    // EventManager to extract them and a request for every event.
    if (m_pEventList)
    {
        auto core = GetContext();
        // Get the event manager.
        IFCPTR_RETURN(core);
        pEventManager = core->GetEventManager();
        IFCPTR_RETURN(pEventManager);
        IFC_RETURN(pEventManager->AddRequestsInOrder(this, m_pEventList));
    }

    // If we have a managed peer, then the time manager is now acting
    // as a root reference to it; we must prevent it from being GC-ed.
    if ( HasManagedPeer() )
    {
        IFC_RETURN(PegManagedPeer(TRUE /* isShutdownException */));
        m_fReleaseManagedPeer = TRUE;
    }

    return S_OK;
}

_Check_return_ HRESULT CTimeline::OnRemoveFromTimeManager()
{
    m_fIsInTimeManager = FALSE;

    // This flag should only be set when being ticked in the time manager.
    m_hasIndependentAnimation = FALSE;

    // If we are becoming Inactive and there are events.
    if (m_pEventList)
    {
        // Remove the events...
        CXcpList<REQUEST>::XCPListNode *pTemp = m_pEventList->GetHead();
        while (pTemp)
        {
            REQUEST * pRequest = (REQUEST *)pTemp->m_pData;
            IFC_RETURN( GetContext()->GetEventManager()->RemoveRequest(this, pRequest));
            pTemp = pTemp->m_pNext;
        }
    }

    // If we have a managed peer, then we can turn its lifetime management
    // over to its parent; the time manager is no longer acting as a root
    // reference to it.
    if (m_fReleaseManagedPeer)
    {
        UnpegManagedPeer(TRUE /* isShutdownException */);
        m_fReleaseManagedPeer = FALSE;
    }

    return S_OK;
}

// Perform some managed peer lifetime maintenance when a managed peer is created.
_Check_return_ HRESULT CTimeline::OnManagedPeerCreated(XUINT32 fIsCustomDOType, XUINT32 fIsGCRoot)
{
    // Perform the base behavior
    IFC_RETURN(CDependencyObject::OnManagedPeerCreated(fIsCustomDOType, fIsGCRoot));

    // As long as we're in the time manager, we're "rooted", and so
    // the managed peer should not be allowed to be GC-ed.

    if (m_fIsInTimeManager && !m_fReleaseManagedPeer)
    {
        // TODO: Merge: Why does this happen? It seems unnecessary given that the managed
        // peer would be immediately unpegged by CheckHasNoExternalRefs if the only things
        // with references to this timeline are the time manager and the managed peer.
        IFC_RETURN(PegManagedPeer(TRUE /* isShutdownException */));
        m_fReleaseManagedPeer = TRUE;
    }

    return S_OK;
}

// Encapsulates the lifetime checks for active Timelines.
void CTimeline::CheckHasNoExternalRefs(_Inout_opt_ bool *pbHasNoExternalReferences)
{
    // if this is part of a TimeManager tick, check whether we have any external refs
    if (pbHasNoExternalReferences)
    {
        //
        // There might be references from Completed handlers. If this timeline runs forever, then
        // the Completed handler never gets called and should not contribute to our reference
        // count. They should be ignored, otherwise this timeline will leak.
        //
        const bool runsForever =
            m_duration && m_duration->Value().GetDurationType() == DirectUI::DurationType::Forever ||
            m_repeatBehavior && m_repeatBehavior->Value().GetRepeatBehaviorType() == DirectUI::RepeatBehaviorType::Forever;

        XUINT32 completedReferencesToIgnore = 0;

        // This CTimeline should be in the time manager, otherwise the event manager won't actually
        // have m_completedHandlerRegisteredCount references on this object.
        ASSERT(m_fIsInTimeManager);

        if (runsForever)
        {
            completedReferencesToIgnore = m_completedHandlerRegisteredCount;
        }

        //
        // If we have a managed peer and two references (not counting the ignored references from
        // the Completed handlers of CTimelines that run forever), then one reference is from the
        // CTimeManager, and the other is from the managed peer itself. Weaken the ref on the
        // managed peer. If the managed peer ends up being GC'd, then this will cause the number
        // of references on this CTimeline to go down to 1.
        //
        if (HasManagedPeer() && (GetRefCount() - completedReferencesToIgnore == 2))
        {
            if (m_fReleaseManagedPeer)
            {
                UnpegManagedPeer(TRUE /* isShutdownException */);
                m_fReleaseManagedPeer = FALSE;
            }
        }

        // if this Timeline only has one ref, and we are in the TimeManager, then
        // this animation is ok with the Storyboard tree being deleted.
        *pbHasNoExternalReferences &= (GetRefCount() - completedReferencesToIgnore == 1);
    }
}

// Resolve target name and target property
_Check_return_ HRESULT CTimeline::ResolveLocalTarget(
    _In_ CCoreServices *pCoreServices,
    _In_opt_ CTimeline *pParentTimeline )
{
    xref::weakref_ptr<CDependencyObject> targetObjectWeakRef;
    const CDependencyProperty* pTargetDependencyProperty = nullptr;
    xstring_ptr strTargetName = m_strTargetName;
    xstring_ptr strTargetProperty = m_strTargetProperty;
    CTimeline* pParent = nullptr;
    CTimeline* pResolveProxy = nullptr;

    // Always clean our resolved target
    ReleaseTarget();

    // if we have a manual target object, it takes precedence
    if (!m_manualTargetObjectWeakRef.expired())
    {
        targetObjectWeakRef = m_manualTargetObjectWeakRef;
    }

    // if we have a manual target DP (internal only), it takes precedence
    if (m_pManualTargetDependencyProperty)
    {
        pTargetDependencyProperty = m_pManualTargetDependencyProperty;
    }

    if ( pParentTimeline != nullptr )
    {
        // get targeting information from the parent chain
        pParent = pParentTimeline;
        while( pParent )
        {
            // timelines under a dynamictimeline should use it as its proxy for resolving through templatescope.
            if (!pResolveProxy && pParent->OfTypeByIndex<KnownTypeIndex::DynamicTimeline>())
            {
                pResolveProxy = pParent;
            }

            // attempt to get a target name
            if (strTargetName.IsNullOrEmpty())
            {
                strTargetName = pParent->m_strTargetName;
            }

            // attempt to get a property path
            if (strTargetProperty.IsNullOrEmpty())
            {
                strTargetProperty = pParent->m_strTargetProperty;
            }

            // attempt to get a manually set target object
            if (targetObjectWeakRef.expired())
            {
                targetObjectWeakRef = pParent->m_manualTargetObjectWeakRef;
            }

            // attempt to get a manually set target property
            if (!pTargetDependencyProperty)
            {
                pTargetDependencyProperty = m_pManualTargetDependencyProperty;
            }

            pParent = pParent->GetTimingParent();
        }
    }


    // If we don't have a target object yet, try to resolve it using name
    if ( targetObjectWeakRef.expired())
    {
        CDependencyObject *pTargetObject;

        // if we don't have a name specified, nothing we can do at this point
        if (strTargetName.IsNullOrEmpty())
        {
            return S_OK;
        }


        if (!pResolveProxy)
        {
            // if no proxy is being used, possibly dynamictimeline needs to be used
            if (m_pDynamicTimelineParent)
            {
                pResolveProxy = m_pDynamicTimelineParent;
            }
            else
            {
                // still nothing than (this) is the correct object to use
                pResolveProxy = this;
            }
        }

        //
        // If the timeline was parsed as part of a ControlTemplate, then m_templatedParent will be set, as
        // will IsTemplateNamescopeMember.  If that's the case, then we use the TemplatedParent as the NamescopeOwner
        // that we pass to GetNamedObject()
        //
        pTargetObject = GetContext()->GetNamedObject(strTargetName,
            pResolveProxy->IsTemplateNamescopeMember() ? pResolveProxy->GetTemplatedParent() : pResolveProxy->GetStandardNameScopeOwner(),
            pResolveProxy->IsTemplateNamescopeMember() ?
                Jupiter::NameScoping::NameScopeType::TemplateNameScope :
                Jupiter::NameScoping::NameScopeType::StandardNameScope);


        // if we have a name, but it can't be found, set error with details.
        if (pTargetObject == nullptr)
        {
            HRESULT hrToOriginate = E_NER_INVALID_OPERATION;
            xephemeral_string_ptr parameters[1];

            strTargetName.Demote(&parameters[0]);

            // Don't fail fast - VSM can catch this error and handle it
            IGNOREHR(SetAndOriginateError(hrToOriginate, RuntimeError, AG_E_RUNTIME_SB_BEGIN_INVALID_TARGET, 1, parameters));
            IFC_RETURN(hrToOriginate);
        }

        // First get the named object from the value store.  This addrefs the weakref object.
        targetObjectWeakRef = xref::get_weakref(pTargetObject);
        pTargetObject = nullptr;
    }

    // By this point we know we have a target object - need to resolve target property.
    auto resolvedTargetObject = targetObjectWeakRef.lock();
    if (resolvedTargetObject)
    {
        // if we don't yet have a target DP, need to parse the property path
        if (!pTargetDependencyProperty)
        {
            // if we don't have a property path, nothing we can do at this point
            if (strTargetProperty.IsNullOrEmpty())
            {
                return S_OK;
            }

            // Now parse the property path.  Note that this may change what we consider the
            // target object so we won't take a reference unless we succeed at parsing the
            // property path.  The refcount in the inout parameter pResolvedTargetObject should
            // stay the same.
            auto tempTargetObject = resolvedTargetObject.get();
            HRESULT hr = GetContext()->ParsePropertyPath(
                    &tempTargetObject,
                    &pTargetDependencyProperty,
                    strTargetProperty);
            resolvedTargetObject = tempTargetObject;

            // It is an error NOT to find the target property
            if (FAILED(hr) || !pTargetDependencyProperty)
            {
                HRESULT hrToOriginate = E_NER_INVALID_OPERATION;
                xephemeral_string_ptr parameters[1];

                strTargetProperty.Demote(&parameters[0]);

                if (CTimeManager::ShouldFailFast())
                {
                    IFCFAILFAST(hrToOriginate);
                }
                else
                {
                    IGNOREHR(SetAndOriginateError(hrToOriginate, RuntimeError, AG_E_RUNTIME_SB_BEGIN_INVALID_PROP, 1, parameters));
                    IFC_RETURN(hrToOriginate);
                }
            }
        }

        // By this point, we have the final resolved target object and the DP

        // Store the weak ref to the target object (add refs the final weak ref object).
        // pTargetObjectWeakRef still needs to be released, which happens in the cleanup section.
        m_targetObjectWeakRef = resolvedTargetObject;
        m_pTargetDependencyProperty = pTargetDependencyProperty;
    }
    else
    {
        // We have no target in the current tree - ignore
    }

    return S_OK;
}

void CTimeline::ResolveName(
    _In_ const xstring_ptr& strName,
    _In_opt_ CTimeline *pParentTimeline,
    _Outptr_ CDependencyObject **ppObject
    )
{
    CTimeline* pResolveProxyNoRef = NULL;
    CDependencyObject *pObject = NULL;

    if (pParentTimeline != NULL)
    {
        CTimeline* pParent = pParentTimeline;

        while (pParent != NULL)
        {
            // Timelines under a dynamictimeline should use it as its proxy for resolving through templatescope.
            if (!pResolveProxyNoRef && pParent->OfTypeByIndex<KnownTypeIndex::DynamicTimeline>())
            {
                pResolveProxyNoRef = pParent;
            }

            pParent = pParent->GetTimingParent();
        }
    }

    if (!pResolveProxyNoRef)
    {
        // If we have a dynamic timeline parent, we'll use that; otherwise, we'll just use this.
        if (m_pDynamicTimelineParent)
        {
            pResolveProxyNoRef = m_pDynamicTimelineParent;
        }
        else
        {
            pResolveProxyNoRef = this;
        }
    }

    //
    // If the timeline was parsed as part of a ControlTemplate, then m_pTemplatedParent will be set, as
    // will IsTemplateNamescopeMember.  If that's the case, then we use the TemplatedParent as the NamescopeOwner
    // that we pass to GetNamedObject()
    //
    pObject =
        GetContext()->GetNamedObject(
            strName,
            pResolveProxyNoRef->IsTemplateNamescopeMember() ? pResolveProxyNoRef->GetTemplatedParent() : pResolveProxyNoRef->GetStandardNameScopeOwner(),
            pResolveProxyNoRef->IsTemplateNamescopeMember() ?
                Jupiter::NameScoping::NameScopeType::TemplateNameScope :
                Jupiter::NameScoping::NameScopeType::StandardNameScope);

    AddRefInterface(pObject);
    *ppObject = pObject;
}

_Check_return_ HRESULT CTimeline::RequestTickForPendingThemeChange()
{
    CTimeline *pTimingParent = NULL;

    CTimeManager* pTimeManager = GetContext()->GetTimeManager();
    if (!pTimeManager)
    {
        return S_OK;
    }
    pTimingParent = GetTimingParent();

    if (pTimingParent && (pTimingParent != pTimeManager->GetRootTimeline()))
    {
        m_hasPendingThemeChange = TRUE;

        // This is not the top-level timeline, so bubble up request for new tick
        pTimingParent->RequestTickForPendingThemeChange();
    }
    else
    {
        // This is the top-level timeline.

        IXcpBrowserHost *pBrowserHost = GetContext()->GetBrowserHost();
        if (!pBrowserHost)
        {
           return S_OK;
        }
        ITickableFrameScheduler *pFrameScheduler = pBrowserHost->GetFrameScheduler();
        if (!pFrameScheduler)
        {
           return S_OK;
        }

        // Add timeline to TimeManager if needed
        if (!pTimingParent)
        {
            IFC_RETURN(pTimeManager->AddTimeline(this));
        }

        // Schedule new tick to process Animation's new theme
        IFC_RETURN(pFrameScheduler->RequestAdditionalFrame(0 /* immediate */, RequestFrameReason::ThemeChange));

        m_hasPendingThemeChange = TRUE;
    }

    return S_OK;
}

bool CTimeline::IsFinite()
{
    bool isFiniteAnimation = true;
    CTimeline *pCurrentTimeline = this;

    while (pCurrentTimeline != NULL)
    {
        if ((pCurrentTimeline->m_duration && pCurrentTimeline->m_duration->Value().GetDurationType() == DirectUI::DurationType::Forever) ||
            (pCurrentTimeline->m_repeatBehavior && pCurrentTimeline->m_repeatBehavior->Value().GetRepeatBehaviorType() == DirectUI::RepeatBehaviorType::Forever))
        {
            isFiniteAnimation = FALSE;
            break;
        }

        pCurrentTimeline = pCurrentTimeline->GetTimingParent();
    }

    return isFiniteAnimation;
}

// This returns the effective duration of a clock.  The effective duration is basically the
// length of the clock's active period, taking into account speed ratio, repeat, and autoreverse.
// Null is used to represent an infinite effective duration.
void CTimeline::ComputeEffectiveDuration(
    _In_ DurationType durationType,
    _In_ XFLOAT       rDurationValue,
    _In_ XFLOAT       rAppliedSpeedRatio,
    _Out_ COptionalDouble *poptEffectiveDuration
    )
{
    ASSERT(durationType != DirectUI::DurationType::Automatic);
    ASSERT(rAppliedSpeedRatio > 0.0f);
    ASSERT(m_repeatBehavior); // this is always present and can be default

    XFLOAT rRepeatBehavior = 1.0f;
    XDOUBLE rLimitingTime = XDOUBLE_MAX;

    if (m_repeatBehavior->Value().GetRepeatBehaviorType() == DirectUI::RepeatBehaviorType::Duration)
    {
        rLimitingTime = m_repeatBehavior->Value().GetDurationInSec();
    }
    else
    {
        // Repeat is either forever or automatic, so look at the count field
        rRepeatBehavior *= m_repeatBehavior->Value().GetCount();
    }

    // We have a zero or negative duration or repeat, set the effective duration to ZERO
    if ((durationType == DirectUI::DurationType::TimeSpan && rDurationValue <= 0.0f)
      || (rRepeatBehavior <= 0.0f))
    {
        poptEffectiveDuration->m_fHasValue = TRUE;
        poptEffectiveDuration->m_rValue = 0.0;
    }
    // We repeat forever or we last forever without any clippling RepeatBehavior
    else if ((m_repeatBehavior->Value().GetRepeatBehaviorType() == DirectUI::RepeatBehaviorType::Forever)
        || (durationType == DirectUI::DurationType::Forever && m_repeatBehavior->Value().GetRepeatBehaviorType() == DirectUI::RepeatBehaviorType::Count))
    {
        poptEffectiveDuration->m_fHasValue = FALSE;  // no value == forever
        poptEffectiveDuration->m_rValue = 0.0;
    }
    // We last forever, but we have a clipping RepeatBehavior
    else if (durationType == DirectUI::DurationType::Forever)
    {
        poptEffectiveDuration->m_fHasValue = TRUE;
        if (m_repeatBehavior->Value().GetRepeatBehaviorType() == DirectUI::RepeatBehaviorType::Duration)
        {
            poptEffectiveDuration->m_rValue = rLimitingTime;
        }
        else
        {
            poptEffectiveDuration->m_rValue = rRepeatBehavior / rAppliedSpeedRatio;
        }
    }
    // We repeat for a specific number of iterations
    else
    {
        XDOUBLE rScalingFactor = 0;

        if (m_repeatBehavior->Value().GetRepeatBehaviorType() == DirectUI::RepeatBehaviorType::Duration)
        {
            rScalingFactor = rLimitingTime / rAppliedSpeedRatio;
        }
        else
        {
            rScalingFactor = rRepeatBehavior / rAppliedSpeedRatio;
        }

        if (m_fAutoReverse)
        {
            rScalingFactor *= 2.0;
        }

        poptEffectiveDuration->m_fHasValue = TRUE;
        poptEffectiveDuration->m_rValue = MIN( rScalingFactor * rDurationValue, rLimitingTime );
    }
}

void CTimeline::ComputeExpirationTime(
    _In_ XDOUBLE      rBeginTime,
    _In_ DurationType durationType,
    _In_ XFLOAT       rDurationValue,
    _In_ XFLOAT       rAppliedSpeedRatio,
    _Out_ COptionalDouble *poptExpirationTime
    )
{
    ASSERT(durationType != DirectUI::DurationType::Automatic);
    COptionalDouble effectiveDuration;

// Get the effective duration and compute the expiration time
    ComputeEffectiveDuration(
        durationType,
        rDurationValue,
        rAppliedSpeedRatio,
        &effectiveDuration
        );

    if (effectiveDuration.m_fHasValue)
    {
        poptExpirationTime->m_fHasValue = TRUE;
        poptExpirationTime->m_rValue = rBeginTime + effectiveDuration.m_rValue;
    }
    else
    {
        poptExpirationTime->m_fHasValue = FALSE; // indicates infinite expiration
    }
}

bool CTimeline::IsInterestingForAnimationTracking()
{
    // If the timeline is delayed, ignore it: e.g. it may be speculative
    // animation scheduled into the future to auto-hide etc.
    if (m_pBeginTime && ((m_pBeginTime->m_rTimeSpan * 1000) > c_AnimationTrackingMaxBeginTimeInMs))
    {
        return false;
    }

    // If the timeline is set to run for ever or has zero duration, ignore it.
    // Note that since parallel timeline overrides this routine, we don't expect
    // to be calling GetNaturalDuration in an expensive recursive manner.
    DurationType durationType;
    XFLOAT rDurationValue;
    GetNaturalDuration(&durationType, &rDurationValue);
    if ((durationType == DirectUI::DurationType::Forever) ||
        ((durationType == DirectUI::DurationType::TimeSpan) && (rDurationValue <= 0)))
    {
        return false;
    }

    // If there is no target, or the target is not visible etc, ignore it.
    // This happens frequently for the scrollbar state transitions that get
    // fired even though no scrollbars or parts are visible.
    // Note that this a 'best effort' check - there are other animations that
    // might not be interesting for telemetry that aren't ruled out here, e.g.
    // target could be clipped out, off-screen, etc.
    bool hasValidUITarget = false;
    auto target = GetTargetObjectWeakRef().lock();
    if (target)
    {
        CUIElement* pUITarget = target->GetNamedSelfOrParent<CUIElement>();
        if (pUITarget &&
            pUITarget->IsVisible() &&
            pUITarget->AreAllAncestorsVisible())
        {
            // We do not want to force the bounds calculation in this path,
            // but we will check it if it is already calculated.
            if (!pUITarget->AreBoundsDirty())
            {
                XRECTF_RB bounds;
                if (SUCCEEDED(pUITarget->GetOuterBounds(nullptr /*hitTestParams*/, &bounds)))
                {
                    if (!IsEmptyRectF(bounds))
                    {
                        hasValidUITarget = TRUE;
                    }
                }
            }
            else
            {
                hasValidUITarget = TRUE;
            }
        }
    }

    if (!hasValidUITarget)
    {
        return false;
    }

    return true;
}

// Collect target and other information for animation tracking.
void CTimeline::AnimationTrackingCollectInfoNoRef(
    _Inout_ CDependencyObject** ppTarget,
    _Inout_ CDependencyObject** ppDynamicTimeline
    )
{
    if (!*ppTarget)
    {
        auto target = GetTargetObjectWeakRef().lock();
        if (target)
        {
            *ppTarget = target.get();
        }
    }
}

// Determine if we are active, filling, or off
// pParentTime is clamped if it is inside the postfill zone.
// We have to handle reversed parent differently because we have closed-open intervals in global time
void CTimeline::ComputeCurrentClockState(
    _Inout_ ComputeStateParams &clampedParentParams,
    _In_ XDOUBLE rBeginTime,
    _In_ COptionalDouble *poptExpirationTime,
    bool hadIndependentAnimationLastTick,
    _Out_ bool *pShouldComputeProgress
    )
{
    // Check if the parent hadn't started yet, or if we are before the begin time.
    if (!clampedParentParams.hasTime || clampedParentParams.time < rBeginTime)
    {
        m_clockState = DirectUI::ClockState::NotStarted;
        m_rCurrentProgress = 0.0f;
        *pShouldComputeProgress = FALSE;
    }
    else
    {
        // If we have an expiration time, check fill behavior.
        if (IsExpired(poptExpirationTime, clampedParentParams.time))
        {
            // If we're running with DComp animations, then we'll get notifications back from DComp when the animation
            // expires, at which time we'll schedule another tick. Before we get that notification, treat the animation
            // as active; even if Xaml determines that it should have completed, it might still be finishing up in the
            // DWM.
            //
            // Not all animations need to wait on DComp's completed event. Specifically, dependent animations don't have
            // a DComp animation, so they have nothing to wait on. There's a slight chicken-and-egg problem here, since
            // an animation can't be determined to be independent until the clock state is computed here (we need to know
            // whether the root storyboard is still active). As a workaround, we check whether this animation was independent
            // on the previous frame. This won't be accurate on the first frame, where all animation are considered dependent
            // here. So we could incorrectly complete an independent animation on the first frame, but that means the animation
            // was instant and shouldn't have generated a DComp animation to wait on in the first place.
            //
            // Animations that were seeked to the end on their first frame by the VSM will also not generate a DComp animation,
            // but that means they're not waiting on a completed notification either and are free to complete immediately.
            if (!hadIndependentAnimationLastTick
                || !IsWaitingForDCompAnimationCompleted()
                || !clampedParentParams.isAncestorWaitingForDCompAnimationCompleted
                || (poptExpirationTime->m_fHasValue && poptExpirationTime->m_rValue == 0)
                // DComp time events aren't firing on the phone right now. This is tracked by bug 1548785 <Time events don't fire on the phone>
                // In the meantime, only wait for DComp animation completed events on the desktop. AllHwndFeatures_NotSupported is true everywhere
                // except desktop.
                || IsXamlBehaviorEnabledForCurrentSku(AllHwndFeatures_NotSupported)
                )
            {
                // Clamp the effective parent time and the current time to the expiration time.
                if (m_fillBehavior == DirectUI::FillBehavior::HoldEnd)
                {
                    m_clockState = DirectUI::ClockState::Filling;
                    m_rCurrentTime = poptExpirationTime->m_rValue;
                    clampedParentParams.time = m_rCurrentTime;
                }
                else
                {
                    m_clockState = DirectUI::ClockState::Stopped;
                }
            }
            else
            {
                // We've expired while waiting for a DComp animation completed notification to come in. Don't request any more
                // ticks. We'll request a tick after that notification comes in.
                m_clockState = DirectUI::ClockState::Active;
                m_isExpiredWhileWaitingForDCompAnimationCompleted = true;
            }
        }
        else
        {
            m_clockState = DirectUI::ClockState::Active;
        }

        *pShouldComputeProgress = TRUE;
    }
}

void CTimeline::ComputeLocalProgressAndTime(
    XDOUBLE rBeginTime,
    XDOUBLE rParentTime,
    DurationType durationType,
    XFLOAT rDurationValue,
    _In_ COptionalDouble *poptExpirationTime,
    bool hadIndependentAnimationLastTick,
    _Out_ bool *pIsInReverseSegment
    )
{
    ASSERT(rParentTime >= rBeginTime);

    bool isInReverseSegment = false;

    // We compute our current time relative to our parent time and take into account our own speed.
    // This is always relative to the parent's frame of reference.
    m_rCurrentTime = (rParentTime - rBeginTime) * m_rSpeedRatio;

    if (durationType == DirectUI::DurationType::TimeSpan)
    {
        if (hadIndependentAnimationLastTick
            && IsExpired(poptExpirationTime, rParentTime)
            // DComp time events aren't firing on the phone right now. This is tracked by bug 1548785 <Time events don't fire on the phone>
            // In the meantime, only wait for DComp animation completed events on the desktop. AllHwndFeatures_NotSupported is true everywhere
            // except desktop.
            && !IsXamlBehaviorEnabledForCurrentSku(AllHwndFeatures_NotSupported)
            )
        {
            // If DComp animations are on, then we're waiting for a completed notification from the DComp animation. In
            // the meantime, Xaml is ticking the animation itself. Xaml is expected to reach the end before the DComp
            // animation ends (since the DComp animation starts after the Xaml animation due to frame submission, and it
            // takes time for the notification to come back from the DWM). In the period of time where the Xaml animation
            // completed but the DComp one didn't, hold the time of the animation in its expiration time, but don't fire
            // the completed event or go inactive until we get the notification back from DComp.
            // Note: the expiration time has the begin time and speed ratio factored in. See ComputeExpirationTime and
            // ComputeEffectiveDuration.
            //
            // Not all animations need to wait on DComp's completed event. Specifically, dependent animations don't have
            // a DComp animation, so they have nothing to wait on. There's a slight chicken-and-egg problem here, since
            // an animation can't be determined to be independent until the clock state is computed here (we need to know
            // whether the root storyboard is still active). As a workaround, we check whether this animation was independent
            // on the previous frame. This won't be accurate on the first frame, where all animation are considered dependent
            // here, but that's not a problem, since the only animations that can complete on their first frame are zero-
            // duration animations, and those won't need a DComp animation anyway.
            m_rCurrentTime = (poptExpirationTime->m_rValue - rBeginTime) * m_rSpeedRatio;
        }

        if (rDurationValue <= 0.0f)
        {
            // There are scenarios for ZERO duration animations
            // In those cases we want to hold the END value
            m_rCurrentTime = 0.0;
            m_rCurrentProgress = 1.0f;
        }
        else
        {
            XFLOAT rIterations = static_cast<XFLOAT>(m_rCurrentTime / rDurationValue);
            m_nIteration = XcpFloor(rIterations);
            m_rCurrentProgress = rIterations - XFLOAT(m_nIteration);

        // If we have done more than one iteration, the boundary case indicates
        // full progress so that hold behaviors works at the end

        // Note that there are some tricky precision issues here. In particular,
        // our computation includes:
        //
        // rParentTime = rBeginTime + rDuration;    <-- drops precision to rBeginTime range
        // rCurrentTime = rParentTime - rBeginTime; <-- still in rBeginTime range
        //
        // However, rDuration hasn't had truncated precision, so we can't do an
        // exact test with m_rCurrentProgress == 0.
        //
        // Worse yet, the precision drop depends on whether or not we stay internal
        // to the FPU or whether we write to memory (and drop precision), so we can't
        // reliably drop precision here without forcing external memory writes.
        //
        // So, instead of the zero test, we do a current progress < 0.1% test for
        // this boundary case so we can fill last value.
        //
        // TODO: Think about how to avoid precision loss here more reliably.

            // We have an expiration time, so we are clipping.  Only check for the boundary
            // case
            if ((rIterations >= 1.0f && m_rCurrentProgress <= 0.001f) || (m_rCurrentProgress >= 0.999f))
            {
                if (IsExpired(poptExpirationTime, rParentTime))
                {
                    // If we are near a complete cycle, and have autorepeat,
                    // we must hold at the beginning and not the end ...
                    if ( m_fAutoReverse && (m_nIteration % 2) == 0)
                    {
                        m_rCurrentProgress = 0.0f;
                    }
                    else
                    {
                        m_rCurrentProgress = 1.0f;
                    }
                }
            }

            // Invert the odd intervals for autoreverse
            if (m_fAutoReverse && (m_nIteration % 2) == 1)
            {
                ASSERT(m_rCurrentProgress <= 1.0f);
                m_rCurrentProgress = 1.0f - m_rCurrentProgress;
                isInReverseSegment = TRUE;
            }

            // Set the time we will report to our children timelines
            m_rCurrentTime = m_rCurrentProgress * rDurationValue;
        }
    }
    else // Duration is Forever
    {
        m_rCurrentProgress = 0.0f;
    }

    *pIsInReverseSegment = isInReverseSegment;
}

bool CTimeline::IsExpired(
    _In_ const COptionalDouble *poptExpirationTime,
    XDOUBLE rParentTime
    ) const
{
    //
    // When there's a pending seek on the first tick, the start delta for the storyboard is calculated from
    // the global time and the pending seek time. The current time is then calculated from the global time
    // and the start delta. Floating point error can cause the current time to be slightly before the seek
    // time, so include a tolerance in this comparison.
    //
    return (poptExpirationTime->m_fHasValue
        && poptExpirationTime->m_rValue <= rParentTime + s_timeTolerance);
}

// Compute the duration when one is not specified in the markup.
// Animations override this method to provide their own default.
void CTimeline::GetNaturalDuration(
    _Out_ DurationType* pDurationType,
    _Out_ XFLOAT* pDurationValue
    )
{
    // Timelines defaults infinite animation
    *pDurationType = DirectUI::DurationType::Automatic;
    *pDurationValue = 0.0f;
}

HRESULT CTimeline::FireCompletedEvent()
{
    ASSERT(!IsCompletedEventFired());

    HRESULT hr = S_OK;
    // See if we have an event handler for "Completed"
    if (m_pEventList)
    {
        CEventManager *pEventManager = GetContext()->GetEventManager();
        if (pEventManager)
        {
            // Events MAY BE synchronous, so if anything RESETS this state inside the event
            //   this value will be incorrect if we set this later
            m_IsCompletedEventFired = TRUE;

            // ... And fire it!
            pEventManager->Raise(EventHandle(KnownEventIndex::Timeline_Completed), TRUE, this, NULL);
        }
    }

    RRETURN(hr);
}

// Returns the root Timeline that is a parent of this one, which could be itself.
CTimeline* CTimeline::GetRootTimingParent() const
{
    CTimeline *pRoot = NULL;
    const CTimeline *pParent = this;
    auto core = GetContext();

    if (core && core->GetTimeManager())
    {
        pRoot = core->GetTimeManager()->GetRootTimeline();
    }

    // we found the root if our parent's parent is the TimeManager or NULL
    while ((pParent->GetTimingParent() != pRoot) && (pParent->GetTimingParent() != NULL))
    {
        pParent = pParent->GetTimingParent();
    }

    return const_cast<CTimeline *>(pParent);
}

// Returns S_OK in case this Timeline is in a state where it can be
// safely modified.  Returns an appropriate error message otherwise.
_Check_return_ HRESULT CTimeline::CheckCanBeModified()
{
    if (!CanBeModified())
    {
        if (CTimeManager::ShouldFailFast())
        {
            IFCFAILFAST(static_cast<HRESULT>(E_NER_INVALID_OPERATION));
        }
        else
        {
            IFC_RETURN(SetAndOriginateError(E_NER_INVALID_OPERATION, RuntimeError, AG_E_RUNTIME_SB_MODIFY_ACTIVE_ANIMATION));
        }
    }
    return S_OK;
}

bool CTimeline::CanBeModified() const
{
    CTimeline *pRootStoryboard = GetRootTimingParent();
    if (pRootStoryboard && !pRootStoryboard->IsInStoppedState())
    {
        return false;

    }
    return true;
}

_Check_return_ HRESULT CTimeline::SetTargetObject(_In_ CDependencyObject *pTarget)
{
    // we can only do this if we can be modified
    IFC_RETURN(CheckCanBeModified());

    m_manualTargetObjectWeakRef = xref::get_weakref(pTarget);

    return S_OK;
}

CDependencyObject* CTimeline::GetTargetObjectNoRef() const
{
    return m_manualTargetObjectWeakRef.lock_noref();
}

xref_ptr<CDependencyObject> CTimeline::GetTargetObject() const
{
    return xref_ptr<CDependencyObject>(GetTargetObjectNoRef());
}

const CDependencyProperty* CTimeline::GetTargetProperty()
{
    return m_pManualTargetDependencyProperty;
}

// Requests a tick for the time remaining until the beginning of this Timeline's active
// period, if required.
_Check_return_ HRESULT CTimeline::RequestTickToActivePeriodBegin(
    _In_ const ComputeStateParams &parentParams,
    _In_ const ComputeStateParams &myParams,
    XDOUBLE beginTime
    ) const
{
    ASSERT(m_clockState == DirectUI::ClockState::NotStarted);

    // This timeline will be 'NotStarted' but have a valid parent time if it had a BeginTime.
    if (parentParams.hasTime)
    {
        ASSERT(parentParams.time < beginTime);

        // This animation needs to schedule a tick for the time it starts when the parent time is running forwards.
        //
        // When running backwards in this state, this isn't necessary since this animation won't tick again
        // (from its perspective). If it ends up changing direction and running forwards again, it's because
        // the parent started a new iteration. The parent guarantees a tick at the beginning of each iteration
        // to notify all its children that they are running again in a new state or direction.
        //
        // Note that it isn't necessary to check whether the timeline is locally reversed, since a timeline will not
        // run backwards from Active into NotStarted again - it will loop into a forward iteration again.
        if (!parentParams.isReversed)
        {
            IFC_RETURN(parentParams.pFrameSchedulerNoRef->RequestAdditionalFrame(
                XcpCeiling((beginTime - parentParams.time) * 1000.0 / myParams.speedRatio),
                RequestFrameReason::AnimationTick));
        }
    }
    // Otherwise, it hasn't started because its parent hasn't started (which is why it has no parent time),
    // or the root timing parent is stopped and didn't want to any of its children to be active.
    // Note: Check the root storyboard for Stopped. In the case of nested storyboard, the root could be Stopped
    //       while all nested storyboards are Filling at time infinity.
    else
    {
        ASSERT(GetTimingParent() != NULL &&
            (GetTimingParent()->m_clockState == DirectUI::ClockState::NotStarted || GetRootTimingParent()->m_clockState == DirectUI::ClockState::Stopped)
            );
    }

    return S_OK;
}

// Requests a tick for the time remaining until the end of this Timeline's active period,
// if required.
_Check_return_ HRESULT CTimeline::RequestTickToActivePeriodEnd(
    _In_ const ComputeStateParams &parentParams,
    _In_ const ComputeStateParams &myParams,
    _In_ const COptionalDouble &expirationTime
    ) const
{
    ASSERT(m_clockState == DirectUI::ClockState::Stopped || m_clockState == DirectUI::ClockState::Filling);

    // If the parent time isn't provided, it means the parent hasn't started yet. In that situation,
    // the animation should not have started either.
    ASSERT(parentParams.hasTime);

    // If the animation is stopped or filling, it must have ticked past its expiration time.
    ASSERT(IsExpired(&expirationTime, parentParams.time));

    // This animation needs to schedule a tick for the time it becomes active again, its
    // expiration time, when the parent time is running in reverse. This is analogous to ticking to the
    // beginning of its active period when running forwards (see RequestTickToActivePeriodBegin).
    //
    // When running forwards in the Stopped or Filling state, the animation will not tick again
    // (from its perspective). If it ends up changing direction and running backwards again, it's
    // because the parent started a new iteration. The parent guarantees a tick at the beginning of
    // each iteration to notify all its children that they are running again in a new state or direction.
    //
    // Note that it isn't necessary to check whether the timeline is locally reversed, since a timeline will not
    // run forwards from Active to Filling or Stopped if AutoReverse="true" - it will reverse direction instead.
    if (parentParams.isReversed)
    {
        IFC_RETURN(parentParams.pFrameSchedulerNoRef->RequestAdditionalFrame(
            XcpCeiling((parentParams.time - expirationTime.m_rValue) * 1000.0 / myParams.speedRatio),
            RequestFrameReason::AnimationTick));
    }

    return S_OK;
}

void CTimeline::RequestAdditionalFrame()
{
    // The browser host and/or frame scheduler can be NULL during shutdown.
    IXcpBrowserHost *pBH = GetContext()->GetBrowserHost();
    if (pBH != NULL)
    {
        ITickableFrameScheduler *pFrameScheduler = pBH->GetFrameScheduler();

        if (pFrameScheduler != NULL)
        {
            VERIFYHR(pFrameScheduler->RequestAdditionalFrame(0 /* immediate */, RequestFrameReason::AnimationTick));
        }
    }
}
