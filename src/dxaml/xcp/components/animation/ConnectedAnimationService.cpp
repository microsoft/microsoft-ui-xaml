// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ConnectedAnimationService.h"
#include "ConnectedAnimation.h"
#include <CDependencyObject.h>
#include <DependencyObjectTraits.h>
#include <DependencyObjectTraits.g.h>
#include "UIElement.h"
#include "Corep.h"
#include "DOPointerCast.h"
#include "UIElementCollection.h"
#include "EasingFunctions.h"
#include "ConnectedAnimationRoot.h"
#include "WindowRenderTarget.h"
#include "DCompTreeHost.h"
#include "Host.h"
#include <UIThreadScheduler.h>
#include "HWCompNode.h"
#include "HWCompNodeWinRT.h"
#include <UIElementStructs.h>
#include <RuntimeEnabledFeatures.h>
#include <FxCallbacks.h>

static const INT64 s_DefaultDuration = 3000000; // 300ms
static const wfn::Vector2 s_ControlPoint1 = { 0.8f, 0.0f }; // Default control points for easing function
static const wfn::Vector2 s_ControlPoint2 = { 0.2f, 1.0f };

CConnectedAnimationService::CConnectedAnimationService(_In_ CCoreServices *pCore)
    : CDependencyObject(pCore)
{
    m_defaultDuration.Duration = s_DefaultDuration;
}

KnownTypeIndex CConnectedAnimationService::GetTypeIndex() const
{
    return DependencyObjectTraits<CConnectedAnimationService>::Index;
}

_Check_return_ HRESULT CConnectedAnimationService::CreateAnimation(const xstring_ptr_view& key, _Outptr_ CConnectedAnimation** connectedAnimation)
{
    *connectedAnimation = nullptr;
    if (key.IsNullOrEmpty()) IFC_RETURN(E_INVALIDARG);

    auto entry = std::find_if(m_animations.begin(), m_animations.end(), [&](const auto& entry)
    {
        return entry->GetKey() == key;
    });

    if (entry != m_animations.end())
    {
        IFC_RETURN((*entry)->Cancel());
        entry->reset();
        m_animations.erase(entry);
    }

    xref_ptr<CConnectedAnimation> animation;
    xstring_ptr promotedKey;
    IFC_RETURN(key.Promote(&promotedKey));
    animation.attach(new CConnectedAnimation(promotedKey, GetContext(), this));

    m_animations.push_back(animation);

    *connectedAnimation = animation.detach();

    return S_OK;
}

_Check_return_ HRESULT CConnectedAnimationService::CreateCoordinatedAnimation(_In_ CConnectedAnimation * baseAnimation, _Outptr_ CConnectedAnimation ** connectedAnimation)
{
    xref_ptr<CConnectedAnimation> animation;
    animation.attach(new CConnectedAnimation(GetContext(), baseAnimation, this));

    m_animations.push_back(animation);

    *connectedAnimation = animation.detach();

    return S_OK;
}

_Check_return_ HRESULT CConnectedAnimationService::GetAnimation(const xstring_ptr_view& key, _Outptr_opt_ CConnectedAnimation** connectedAnimation)
{
    if (connectedAnimation == nullptr) return S_OK;

    *connectedAnimation = nullptr;

    if (key.IsNullOrEmpty()) IFC_RETURN(E_INVALIDARG);

    auto entry = std::find_if(m_animations.begin(), m_animations.end(), [&](const auto& entry)
    {
        return entry->GetKey() == key;
    });

    if (entry != m_animations.end())
    {
         entry->CopyTo(connectedAnimation);
    }

    return S_OK;
}

// Iterate all the connected animation and determine whether a uielement needs to be retained by any active connected animation.
_Check_return_ HRESULT CConnectedAnimationService::OnUnloadingElement(_In_ CUIElement* unloadingElement, _Out_ bool* shouldRetain)
{
    *shouldRetain = false;

    for (auto& entry : m_animations)
    {
        IFC_RETURN(entry->OnUnloadingElement(unloadingElement, shouldRetain));
        if (*shouldRetain)
        {
            // Add detained element to the unloading storage.
            xref_ptr<CUIElement> element(unloadingElement);
            m_retainedElements.push_back(element);
            auto core = GetContext();
            core->GetConnectedAnimationRoot()->SetNeedsUnloadingHWWalk(true);
            CUIElement::NWSetContentDirty(core->GetConnectedAnimationRoot(), DirtyFlags::Render);
            // We need to render walk the rataining element in order to create the comp node for the source element.
            // However we need to hide the retaining element for the next frame.
            if (!element->HasActiveConnectedAnimation())
            {
                IFC_RETURN(element->SetRequiresComposition(CompositionRequirement::HasConnectedAnimation, IndependentAnimationType::None));
                CUIElement::NWSetContentDirty(element, DirtyFlags::Render); // Force dirty the render to make sure the comp node is created..
            }
            return S_OK;
        }
    }
    return S_OK;
}

_Check_return_ HRESULT CConnectedAnimationService::CleanupRetainedElements()
{
    // Remove all the detained element from unloading storage.
    for (auto& element : m_retainedElements)
    {
        CUIElement* pParent = nullptr;
        bool bWasUnloading = false;

        // unload target
        pParent = do_pointer_cast<CUIElement>(element->GetParentInternal());
        // parent might have been destroyed already. In that case its unloading storage will be cleared in the destructor of its collection.
        if (pParent)
        {
            IFC_RETURN(pParent->GetChildren()->RemoveUnloadedElement(element, UC_REFERENCE_ConnectedAnimation, &bWasUnloading));  // will set the lifecycle to unloaded
        }
    }

    m_retainedElements.clear();

    auto core = GetContext();

    if (core->GetConnectedAnimationRoot())
    {
        core->GetConnectedAnimationRoot()->SetNeedsUnloadingHWWalk(false);
    }
    return S_OK;
}

_Check_return_ HRESULT CConnectedAnimationService::PreCommit()
{
    bool waitForPreviousCommitCompletion = m_waitForCommitCompletion;
    m_waitForCommitCompletion = false;

    for (auto& element : m_retainedElements)
    {
        CUIElement* parent = do_pointer_cast<CUIElement>(element->GetParentInternal());
        if (parent != nullptr)
        {
            // It's time to hide the CompNode we put in unloading storage, but only under certain conditions.
            // There may be multiple reasons why this element is in unloading storage.  If any reasons other
            // then a ConnectedAnimation are present, we must not hide the CompNode as those reasons are legitimately
            // keeping this element visible.
            UINT unloadContext = parent->GetChildren()->GetUnloadingContext(element);
            if (unloadContext == UC_REFERENCE_ConnectedAnimation)
            {
                // Nothing retained by connected animation should be visible in the next frame.
                HWCompTreeNodeWinRT* compNode = static_cast<HWCompTreeNodeWinRT*>(element->GetCompositionPeer());
                ASSERT(compNode);
                compNode->HidePrependVisual();
            }
        }
    }

    auto iterator = m_animations.begin();
    while (iterator != m_animations.end())
    {
        bool removeAnimation = false;
        bool waitForCommit = false;
        IFC_RETURN((*iterator)->UpdatePreCommit(&removeAnimation, &waitForCommit));
        m_waitForCommitCompletion = m_waitForCommitCompletion || waitForCommit;
        if (removeAnimation)
        {
            IFC_RETURN((*iterator)->Cancel());
            iterator->reset();
            iterator = m_animations.erase(iterator);
        }
        else
        {
            iterator++;
        }
    }
    // Don't clean up retained elements here, even if there are no animations left. Cleaning up retained elements can queue
    // compositor tree commands to remove comp nodes and can release DManip data. Those compositor tree commands will remain
    // unexecuted this frame, since by this point we've already processed the compositor tree commands for the frame. Instead,
    // the comp nodes that are queued to be removed could try to access DManip data that have been released and cause a crash.
    // PostCommit will clean up retained elements anyway, so there's no need to do it here.

    if (waitForPreviousCommitCompletion)
    {
        // Due to bug 10646539, if we commit tree changes to the visuals backing a visual
        // surface within the same DWM frame where we attempted to freeze the visual surface
        // it is possible that the visual surface will not be frozen until AFTER the tree
        // changes are processed.  We work around this by having the animation's precommit
        // method tell us if we froze a visual surface or not.  If we did on the previous
        // frame, then we need to block here and wait for that to complete, before we return
        // so that we won't commit the tree changes in the same DWM frame.
        IFC_RETURN(GetContext()->NWGetWindowRenderTarget()->GetDCompTreeHost()->WaitForCommitCompletion());
    }

    // Reset our delay animation start state under the assumption that it will be handled (or requested)
    // prior to the next tick.  Then ensure that the next tick comes.
    if (m_delayStartOfAnimations)
    {
        m_delayStartOfAnimations = false;
        IFC_RETURN(GetContext()->RequestMainDCompDeviceCommit(RequestFrameReason::ConnectedAnimation));
     }

    return S_OK;
}

_Check_return_ HRESULT CConnectedAnimationService::PreRenderWalk()
{
    for (auto& entry : m_animations)
    {
        IFC_RETURN(entry->UpdatePreRenderWalk());
    }
    return S_OK;
}

_Check_return_ HRESULT CConnectedAnimationService::TimeoutAnimations()
{
    UINT64 tickInverval = 0;

    for (auto& entry : m_animations)
    {
        UINT64 requestTickInMS;
        IFC_RETURN(entry->TimeoutIfNecessary(&requestTickInMS));
        if (requestTickInMS > 0)
        {
            if (tickInverval == 0)
            {
                tickInverval = requestTickInMS;
            }
            else
            {
                 // Just need to request a tick for the nearest timeout.
                 tickInverval = std::min(tickInverval, requestTickInMS);
            }
        }
    }

    if (tickInverval > 0)
    {
        ITickableFrameScheduler *pFrameScheduler = GetContext()->GetBrowserHost()->GetFrameScheduler();
        if (pFrameScheduler != NULL)
        {
            IFC_RETURN(pFrameScheduler->RequestAdditionalFrame(static_cast<XUINT32>(tickInverval), RequestFrameReason::ConnectedAnimation));
        }
    }
    return S_OK;
}

_Check_return_ HRESULT CConnectedAnimationService::PostCommit()
{
    return CleanupRetainedElements();
}

// Cancel all animations and reset the default duration/easing function
_Check_return_ HRESULT CConnectedAnimationService::CancelAllAnimationsAndResetDefaults()
{
    for (auto& entry : m_animations)
    {
        IFC_RETURN(entry->Cancel());
    }
    m_animations.clear();
    m_defaultDuration.Duration = s_DefaultDuration;
    m_defaultEasingFunction = nullptr;
    return S_OK;
}

_Check_return_ HRESULT CConnectedAnimationService::GetDefaultEasingFunction(_Outptr_result_maybenull_ WUComp::ICompositionEasingFunction** ppValue)
{
    *ppValue = nullptr;

    if (!m_defaultEasingFunction)
    {
        auto renderTarget = GetContext()->NWGetWindowRenderTarget();
        Microsoft::WRL::ComPtr<WUComp::ICubicBezierEasingFunction> cubicBezier;
        IFCFAILFAST(renderTarget->GetDCompTreeHost()->GetEasingFunctionStatics()->CreateCubicBezierEasingFunction(renderTarget->GetDCompTreeHost()->GetCompositor(), s_ControlPoint1, s_ControlPoint2, &cubicBezier));
        IFCFAILFAST(cubicBezier.As(&m_defaultEasingFunction));
    }
    IFC_RETURN(m_defaultEasingFunction.CopyTo(ppValue))
    return S_OK;
}

_Check_return_ HRESULT CConnectedAnimationService::SetDefaultEasingFunction(_In_opt_ WUComp::ICompositionEasingFunction* pValue)
{
    m_defaultEasingFunction = pValue;
    return S_OK;
}

const wf::TimeSpan CConnectedAnimationService::GetAnimationDuration()
{
    wf::TimeSpan duration = GetDefaultDuration();

    bool isAnimationEnabled = true;
    IGNOREHR(FxCallbacks::FrameworkCallbacks_IsAnimationEnabled(&isAnimationEnabled));

    if (!isAnimationEnabled)
    {
        duration.Duration = 10000; // If animations are disabled in Settings, we still need to fire Completed event with no delay, kick off 1ms animations here.
    }

    return duration;
}

const std::vector<xref_ptr<CUIElement>>& CConnectedAnimationService::GetUnloadingElements()
{
    return m_retainedElements;
}

