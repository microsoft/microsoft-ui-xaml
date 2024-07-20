// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "TimeMgr.h"
#include <TranslateTransform.h>
#include "animation.h"
#include <palcore.h>
#include <XamlTraceLogging.h>
#include <DCompTreeHost.h>
#include <WinRTExpressionConversionContext.h>
#include <RuntimeEnabledFeatures.h>
#include <DependencyLocator.h>
#include <FeatureFlags.h>
#include <DesignMode.h>

bool CTimeManager::s_slowDownAnimations = false;
bool CTimeManager::s_slowDownAnimationsLoaded = false;

bool CTimeManager::s_animationFailFast = false;
bool CTimeManager::s_animationFailFastLoaded = false;

void CTimeManager::CleanupDeviceRelatedResourcesRecursive(_In_ bool cleanupDComp)
{
    TimelineListNode *currentNodeNoRef = m_pTimelineListHead;

    __super::CleanupDeviceRelatedResourcesRecursive(cleanupDComp);

    ResetWUCCompletedEvents();

    while (currentNodeNoRef != nullptr)
    {
        CTimeline *pTimelineNoRef = currentNodeNoRef->m_pTimeline;

        if (pTimelineNoRef != nullptr)
        {
            pTimelineNoRef->CleanupDeviceRelatedResourcesRecursive(cleanupDComp);
        }

        currentNodeNoRef = currentNodeNoRef->m_pNextNoRef;
    }

    for (auto cdo : m_targetDOs)
    {
        cdo->CleanupDeviceRelatedResourcesRecursive(cleanupDComp);
    }
    m_targetDOs.clear();

    s_slowDownAnimationsLoaded = false;
}

//#define DEBUG_TRACE_ANIMATION_HASHTABLE
#ifdef DEBUG_TRACE_ANIMATION_HASHTABLE
void DebugTracePrintAnimation(
    const WCHAR* pMsg,
    _In_ const xref::weakref_ptr<CDependencyObject>& pDO,
    _In_ KnownPropertyIndex nPropertyIndex,
    _In_ const CAnimation* pAnimation)
{
    std::array<WCHAR, 64> animationName = { 0 };
    std::array<WCHAR, 64> targetName = { 0 };

    if (pAnimation && !pAnimation->m_strName.IsNullOrEmpty())
    {
        // Copy into local buffer to ensure it's null-terminated?
        auto nameLength = pAnimation->m_strName.GetCount();
        std::copy_n(
            pAnimation->m_strName.GetBuffer(),
            std::min(animationName.size() - 1, nameLength),
            animationName.begin());
    }

    auto pTarget = pDO.lock();
    if (pTarget && !pTarget->m_strName.IsNullOrEmpty())
    {
        auto nameLength = pTarget->m_strName.GetCount();
        std::copy_n(
            pTarget->m_strName.GetBuffer(),
            std::min(targetName.size() - 1, nameLength),
            targetName.begin());
    }

    TRACE(TraceAlways, L"%s Animation:%p TargetDO:%p TargetDP:%4d TargetName:%s AnimationName:%s", pMsg, pAnimation, pTarget, static_cast<UINT32>(nPropertyIndex), targetName, animationName);
}
#define DEBUG_TRACE_PRINTANIMATION( a,b,c,d ) DebugTracePrintAnimation(a,b,c,d);
#else  // DEBUG_TRACE_ANIMATION_HASHTABLE
#define DEBUG_TRACE_PRINTANIMATION( a,b,c,d ) ;
#endif // DEBUG_TRACE_ANIMATION_HASHTABLE

_Check_return_ HRESULT
CTimeManager::SetAnimationOnProperty(
    _In_ const xref::weakref_ptr<CDependencyObject>& DOWeakRef,
    _In_ KnownPropertyIndex nPropertyIndex,
    _In_ CAnimation *pAnimation)
{
    EnsureTimeStarted();

    IFCEXPECT_RETURN(!DOWeakRef.expired());
    IFCEXPECT_RETURN(nPropertyIndex != KnownPropertyIndex::UnknownType_UnknownProperty);
    ASSERT(pAnimation);

    const auto insertResult = m_hashTable.insert(std::make_pair(HashKey(DOWeakRef, nPropertyIndex), xref_ptr<CAnimation>(pAnimation)));
    const bool inserted = insertResult.second;
    if (inserted)
    {
        DEBUG_TRACE_PRINTANIMATION(L"ADD NEW ...  ", DOWeakRef, nPropertyIndex, pAnimation);
    }
    else
    {
        const auto position = insertResult.first;
        DEBUG_TRACE_PRINTANIMATION(L"REPLACING ...", DOWeakRef, nPropertyIndex, position->second);
        position->second = pAnimation;
        DEBUG_TRACE_PRINTANIMATION(L"STORING ...  ", DOWeakRef, nPropertyIndex, pAnimation);
    }

    return S_OK;
}

void CTimeManager::ClearAnimationOnProperty(
    _In_ const xref::weakref_ptr<CDependencyObject>& DOWeakRef,
    _In_ KnownPropertyIndex nPropertyIndex)
{
    const auto position = m_hashTable.find(HashKey(DOWeakRef, nPropertyIndex));
    if (position != m_hashTable.end())
    {
        DEBUG_TRACE_PRINTANIMATION(L"DELETING ... ", DOWeakRef, nPropertyIndex, position->second.get());
        m_hashTable.erase(position);
    }
}

_Check_return_ xref_ptr<CAnimation>
CTimeManager::GetAnimationOnProperty(
    _In_ const xref::weakref_ptr<CDependencyObject>& DOWeakRef,
    _In_ KnownPropertyIndex nPropertyIndex)
{
    xref_ptr<CAnimation> result;

    EnsureTimeStarted();

    // we should be robust to this condition;
    if (!DOWeakRef || nPropertyIndex == KnownPropertyIndex::UnknownType_UnknownProperty)
    {
        return result;
    }

    auto position = m_hashTable.find(HashKey(DOWeakRef, nPropertyIndex));
    if (position != m_hashTable.end())
    {
        if (DOWeakRef.expired())
        {
            m_hashTable.erase(position);
        }
        else
        {
            result = position->second;
        }
        DEBUG_TRACE_PRINTANIMATION(L"GET >>> ...  ", DOWeakRef, nPropertyIndex, result);
    }

    return result;
}

// Updates the start time and mark as loaded if we are unlocked.
void CTimeManager::EnsureTimeStarted()
{
    if (!m_isLoaded)
    {
        // We do this once for the first trigger
        m_isLoaded = TRUE;
    }

    if (m_unlockRequested)
    {
        // We do this once for the first trigger
        m_rTimeStarted = m_pIClock->GetLastTickTimeInSeconds();
        m_unlockRequested = false;
    }
}

void CTimeManager::SetTimeManagerClockOverrideConstant(double newTime)
{
    m_clockOverride = newTime;
}

HRESULT CTimeManager::OnWUCAnimationCompleted(
    _In_ IInspectable* sender,
    _In_ WUComp::ICompositionBatchCompletedEventArgs* args)
{
    xref_ptr<WUComp::ICompositionScopedBatch> scopedBatch;
    IFCFAILFAST(sender->QueryInterface(IID_PPV_ARGS(scopedBatch.ReleaseAndGetAddressOf())));

    const auto& timelineIterator = m_activeWUCAnimations.find(scopedBatch);

    if (timelineIterator != m_activeWUCAnimations.end())
    {
        xref_ptr<CTimeline> timeline = timelineIterator->second;
        timeline->OnDCompAnimationCompleted();

        // We don't expect completed to fire multiple times. If the animation starts again later, we'll create a new scoped batch for it.
        DetachWUCCompletedHandler(scopedBatch);
    }

    return S_OK;
}

void CTimeManager::MarkWUCAnimationCompleted(
    _In_ CDependencyObject* targetObject,
    _In_ KnownPropertyIndex targetPropertyIndex)
{
    const auto& weakRef = xref::get_weakref(targetObject);
    const auto& xamlAnimation = GetAnimationOnProperty(weakRef, targetPropertyIndex);

    xamlAnimation->OnDCompAnimationCompleted();
}

bool CTimeManager::AddActiveWUCAnimation(
    _In_ CDependencyObject* targetObject,
    _In_ KnownPropertyIndex targetPropertyIndex,
    _In_ WUComp::ICompositionScopedBatch* scopedBatch,
    _In_ WUComp::ICompositionAnimatorPartner* animator)
{
    const auto& weakRef = xref::get_weakref(targetObject);

    if (!weakRef.expired())
    {
        // We should have called EnsureTimeStarted when we first ticked the time manager, so these flags should have already
        // been taken care of.
        ASSERT(m_isLoaded && !m_unlockRequested);

        const auto& xamlAnimation = GetAnimationOnProperty(weakRef, targetPropertyIndex);
        ASSERT(xamlAnimation != nullptr);

        xamlAnimation->SetWUCScopedBatch(scopedBatch);
        xamlAnimation->SetWUCAnimator(animator);

        m_activeWUCAnimations.emplace(xref_ptr<WUComp::ICompositionScopedBatch>(scopedBatch), xamlAnimation);

        auto callback = wrl::Callback<
            wrl::Implements<
                wrl::RuntimeClassFlags<wrl::ClassicCom>,
                wf::ITypedEventHandler<IInspectable*, WUComp::CompositionBatchCompletedEventArgs*>,
                IAgileObject, wrl::FtmBase>>
            (this, &CTimeManager::OnWUCAnimationCompleted);

        IFCFAILFAST(scopedBatch->add_Completed(
            callback.Get(),
            xamlAnimation->GetWUCAnimationCompletedToken()));

        return true;
    }
    else
    {
        return false;
    }
}

void CTimeManager::ResetWUCCompletedEvents()
{
    for (const auto& it : m_activeWUCAnimations)
    {
        const auto& scopedBatch = it.first;
        const auto& timeline = it.second;

        IFCFAILFAST(scopedBatch->remove_Completed(*timeline->GetWUCAnimationCompletedToken()));
        timeline->GetWUCAnimationCompletedToken()->value = 0;
        timeline->SetWUCScopedBatch(nullptr);
    }

    m_activeWUCAnimations.clear();
}

void CTimeManager::DetachWUCCompletedHandler(_In_ WUComp::ICompositionScopedBatch* scopedBatchPtr)
{
    xref_ptr<WUComp::ICompositionScopedBatch> scopedBatch(scopedBatchPtr);

    ASSERT(m_activeWUCAnimations.find(scopedBatch) != m_activeWUCAnimations.end());

    xref_ptr<CTimeline> timeline = m_activeWUCAnimations.at(scopedBatch);

    IFCFAILFAST(scopedBatch->remove_Completed(*timeline->GetWUCAnimationCompletedToken()));

    m_activeWUCAnimations.erase(scopedBatch);
    timeline->GetWUCAnimationCompletedToken()->value = 0;
    timeline->SetWUCScopedBatch(nullptr);
}

void CTimeManager::StartWUCAnimation(
    _In_ WUComp::ICompositor* compositor,
    _In_ WUComp::ICompositionObjectPartner* animatingObject,
    _In_ LPCWSTR propertyName,
    _In_ WUComp::ICompositionAnimation* animation,
    _In_ CDependencyObject* targetObject,
    _In_ KnownPropertyIndex targetPropertyIndex,
    _In_opt_ CTimeManager* timeManager)
{
    bool xamlAnimationFound = true;

    // A WUC::ICompositionAnimatorPartner is returned when we start an animation and has APIs on it for pause/seek/resume.
    // We need to notify the Xaml animation of its animator after starting the WUC animation.
    wrl::ComPtr<WUComp::ICompositionAnimatorPartner> animator;

    // A WUC::CompositionScopedBatch delivers animation completed notifications. We'll put each Xaml animation in its own
    // scoped batch. There's no explicit way to add an animation to a scoped batch, instead a scoped batch will implicitly
    // contain all animations that start after its creation and before End (or Suspend) is called. So we create a scoped
    // batch here when starting the WUC animation, rather than when creating the WUC animation. This guarantees the creation
    // of one scoped batch per animation, and doesn't require us to do additional bookkeeping on an animation to store and
    // look up a scoped batch until its DComp animation is about to start.
    wrl::ComPtr<WUComp::ICompositionScopedBatch> scopedBatch;
    IFCFAILFAST(compositor->CreateScopedBatch(WUComp::CompositionBatchTypes_Animation, &scopedBatch));
    IFCFAILFAST(animatingObject->ConnectAnimation(propertyName, animation, &animator));
    IFCFAILFAST(animator->Start());
    scopedBatch->End();

    // TODO_WinRT : Need a way to mock CTimeManager to allow below call to complete.
    //              Currently, CTransform::GetTimeManager() is stubbed and returns nullptr (see Transform_Stub.cpp).
    //              The null check here is not part of original product code and should be removed.
    if (timeManager != nullptr)
    {
        xamlAnimationFound = timeManager->AddActiveWUCAnimation(targetObject, targetPropertyIndex, scopedBatch.Get(), animator.Get());
    }

    if (!xamlAnimationFound)
    {
        // The Xaml animation corresponding to this WUC animation couldn't be found (e.g. the Xaml target has been released,
        // so the Xaml animation is no longer registered as active). Stop the WUC animation.
        IFCFAILFAST(animator->Stop());
    }
}

void CTimeManager::AddTargetDO(_In_ CDependencyObject* targetDO)
{
    m_targetDOs.push_back(xref_ptr<CDependencyObject>(targetDO));
}

void CTimeManager::UpdateTargetDOs(_In_ WUComp::ICompositor* wucCompositor)
{
    WinRTExpressionConversionContext context(wucCompositor);

    for (auto cdo : m_targetDOs)
    {
        cdo->EnsureWUCAnimationStarted(&context);
    }

    // TODO_WinRT: Transform3D and projections will also need EnsureWUCAnimationStarted implementations.

    m_targetDOs.clear();
}

bool CTimeManager::HasTargetDOs() const
{
    return !m_targetDOs.empty();
}

void CTimeManager::PauseDCompAnimationsOnSuspend()
{
    TimelineListNode *pCurrNode = m_pTimelineListHead;

    while (pCurrNode != nullptr)
    {
        CTimeline *pTimelineNoRef = pCurrNode->m_pTimeline;

        if (pTimelineNoRef != nullptr)
        {
            pTimelineNoRef->PauseDCompAnimationsOnSuspend();
        }

        pCurrNode = pCurrNode->m_pNextNoRef;
    }
}

void CTimeManager::ResumeDCompAnimationsOnResume()
{
    TimelineListNode *pCurrNode = m_pTimelineListHead;

    while (pCurrNode != nullptr)
    {
        CTimeline *pTimelineNoRef = pCurrNode->m_pTimeline;

        if (pTimelineNoRef != nullptr)
        {
            pTimelineNoRef->ResumeDCompAnimationsOnResume();
        }

        pCurrNode = pCurrNode->m_pNextNoRef;
    }
}

bool CTimeManager::ShouldFailFast()
{
    if (!s_animationFailFastLoaded)
    {
        static auto runtimeEnabledFeatureDetector = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector();
        s_animationFailFast = !!runtimeEnabledFeatureDetector->GetFeatureValue(RuntimeFeatureBehavior::RuntimeEnabledFeature::AnimationFailFast, true /* disableCaching */);

        s_animationFailFastLoaded = true;
    }

    return s_animationFailFast && !DesignerInterop::GetDesignerMode(DesignerMode::V2Only);
}
