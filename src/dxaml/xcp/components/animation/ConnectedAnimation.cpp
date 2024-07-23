// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "ConnectedAnimation.h"
#include "ConnectedAnimationRoot.h"
#include <CDependencyObject.h>
#include <UIElement.h>
#include <DependencyObjectTraits.h>
#include <DependencyObjectTraits.g.h>
#include "UIElement.h"
#include "Corep.h"
#include "UIElementCollection.h"
#include "DOPointerCast.h"
#include "Transforms.h"
#include "Framework.h"
#include "WindowRenderTarget.h"
#include "LayoutTransitionElement.h"
#include "DCompTreeHost.h"
#include "EventMgr.h"
#include "Host.h"
#include "EasingFunctions.h"
#include "HWCompNode.h"
#include "D2d1.h"
#include <MUX-ETWEvents.h>
#include <D3D11Device.h>
#include <WindowsGraphicsDeviceManager.h>
#include <RuntimeEnabledFeatures.h>
#include <DependencyLocator.h>

#pragma warning(push)
#pragma warning(disable:4996)
// Contains interfaces marked as [[deprecated("PrivateAPI")]]
#include <microsoft.ui.composition.internal.h>

#if !defined(abstract) && defined(EXP_CLANG)
  #define abstract
#endif

#include <Microsoft.UI.Composition.Effects_Impl.h>

#pragma warning(pop)

#include <minmax.h>
#include <windowscollections.h>
#include <XamlTraceLogging.h>
#include <ThemeShadow.h>
#include <ErrorHelper.h>
#include "RootScale.h"
#include <FxCallbacks.h>

using namespace xaml_animation;

static const UINT64 s_timeout = 2000; // 2 seconds

CConnectedAnimation::CConnectedAnimation(_In_ xstring_ptr & key, _In_ CCoreServices *pCore, _In_ CConnectedAnimationService* service) :
    CDependencyObject(pCore),
    m_key(key),
    m_connectedAnimationServiceNoRef(service),
    m_state(ConnectedAnimationState::Idle)
{
}

CConnectedAnimation::CConnectedAnimation(_In_ CCoreServices *pCore, _In_ CConnectedAnimation * baseAnimation, _In_ CConnectedAnimationService* service) :
    CDependencyObject(pCore),
    m_connectedAnimationServiceNoRef(service),
    m_state(ConnectedAnimationState::Prepared),
    m_baseAnimation(baseAnimation)
{
}

CConnectedAnimation::~CConnectedAnimation()
{
    VERIFYHR(Reset());
    delete m_pEventList;
}

KnownTypeIndex CConnectedAnimation::GetTypeIndex() const
{
    return DependencyObjectTraits<CConnectedAnimation>::Index;
}

_Check_return_ HRESULT CConnectedAnimation::Prepare(_In_ CUIElement* source)
{
    m_preparedTimestamp = gps->GetCPUMilliseconds();

    IFC_RETURN(Reset());

    // When the source elememt is not in the live tree,
    // The next render walk will not create a comp node for it.
    // Without a comp node, no snapshot can be taken.
    // We throw an error here so the application can catch.
    if (!source->IsActive())
    {
        ::RoOriginateError(E_INVALIDARG, wrl_wrappers::HStringReference(L"Cannot start animation - the source element is not in the element tree.").Get());
        IFC_RETURN(E_INVALIDARG);
    }

    if (source->HasActiveConnectedAnimation())
    {
        // Element is already used in a connected animation.  In the future, we may allow a source element
        // to be used for multiple animations (such as being able to reveal other items while the element
        // is moving).  However, for now, we just don't start the animation, and TryStart() will return false.
        return Cancel();
    }

    m_source.element = source;

    // Ensure the ConnectedAnimationRoot's handoff viusal is created, so that it can be attached to the visual tree in during the next tick's render walk.
    CConnectedAnimationRoot* root = GetConnectedAnimationRootNoRef();
    ASSERT(root); // root should exist.
    Microsoft::WRL::ComPtr<IUnknown> rootVisual;
    IFC_RETURN(root->GetHandOffVisual(&rootVisual));

    IFC_RETURN(source->SetRequiresComposition(CompositionRequirement::HasConnectedAnimation, IndependentAnimationType::None));
    CUIElement::NWSetContentDirty(source, DirtyFlags::Render); // Force dirty the render to make sure the comp node is created.

    m_state = ConnectedAnimationState::Initialized; // We will prepare on the tick.

   // Get the transform info now since the user may modify the tree.

    IFC_RETURN(GetSnapshotTransformInfo(m_source));

    return S_OK;
}

_Check_return_ HRESULT CConnectedAnimation::TryStart(_In_ CUIElement* destination, _Out_ BOOLEAN* result)
{
    *result = false;

    if (destination == m_source.element.get())
    {
        return Cancel();
    }

    if (destination->HasActiveConnectedAnimation())
    {
        // Element is already used in a connected animation.  In the future, we may allow a source element
        // to be used for multiple animations (such as being able to reveal other items while the element
        // is moving).  However, for now, we just don't start the animation.
        return Cancel();
    }

    // No-op, return failed, if the animation has not initialized, or the animation has started.
    if (m_state == ConnectedAnimationState::Started ||
        m_state == ConnectedAnimationState::Running ||
        m_state == ConnectedAnimationState::Canceled ||
        m_state == ConnectedAnimationState::Complete ||
        m_state == ConnectedAnimationState::ReadyToComplete ||
        m_state == ConnectedAnimationState::Idle)
    {
        return S_OK;
    }

    IFC_RETURN(destination->SetRequiresComposition(CompositionRequirement::HasConnectedAnimation, IndependentAnimationType::None));
    CUIElement::NWSetContentDirty(destination, DirtyFlags::Render); // Force dirty the render to make sure the comp node is created.
    m_destination.element = destination;
    m_state = ConnectedAnimationState::Started;

    // isAnimationEnabled is used to check if the animation is enabled to make a distinction between
    // zero and non-zero length animation
    bool isAnimationEnabled = true;

    // if the call to IsAnimationEnabled fails, it will not set any value, so we will default to TRUE
    IGNOREHR(FxCallbacks::FrameworkCallbacks_IsAnimationEnabled(&isAnimationEnabled));

    // Log "ConnectedAnimation_TryStart" Event
    TraceLoggingWrite(g_hTraceProvider,
         "ConnectedAnimation_TryStart",
         TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
         TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES),
         TraceLoggingBool(isAnimationEnabled, "isAnimationEnabled"));

    *result = true;
    return S_OK;
}

_Check_return_ HRESULT CConnectedAnimation::AnimateCoordinatedEntrance(_In_ CUIElement* pElement)
{
    xref_ptr<CConnectedAnimation> coordinatedAnimation;

    // Validate that this animation is in a state that we can start it.  Because the destination page may not know what
    // has happened back on the source page, we don't error here, but return.
    if (this->m_state != ConnectedAnimationState::Initialized && this->m_state != ConnectedAnimationState::Prepared && this->m_state != ConnectedAnimationState::Started)
    {
        return S_OK;
    }

    if (pElement->HasActiveConnectedAnimation())
    {
        return S_OK;
    }

    IFC_RETURN(m_connectedAnimationServiceNoRef->CreateCoordinatedAnimation(this, coordinatedAnimation.ReleaseAndGetAddressOf()));
    BOOLEAN started;
    IFC_RETURN(coordinatedAnimation->TryStart(pElement, &started));
    return started ? S_OK : E_FAIL;
}

_Check_return_ HRESULT CConnectedAnimation::Cancel()
{
    IFC_RETURN(Reset());
    m_state = ConnectedAnimationState::Canceled;
    return S_OK;
}

_Check_return_ HRESULT CConnectedAnimation::SetAnimationComponent(_In_ ConnectedAnimationComponent component, _In_opt_ WUComp::ICompositionAnimationBase *animation)
{
    int i = static_cast<int>(component);
    if (i < 0 || i >= ARRAYSIZE(m_componentAnimation))
    {
        IFC_RETURN(E_INVALIDARG);
    }

        if (animation == nullptr)
        {
            m_componentAnimation[i] = nullptr;
        }
        else
        {
        IFC_RETURN(animation->QueryInterface(IID_PPV_ARGS(&m_componentAnimation[i])));
        }
    return S_OK;
}

void CConnectedAnimation::GetScaleAnimationEnabled(_Out_ BOOLEAN * enabled) const
{
    *enabled = m_isScaleAnimationEnabled;
}

void CConnectedAnimation::PutScaleAnimationEnabled(_In_ BOOLEAN enabled)
{
    m_isScaleAnimationEnabled = enabled;
}

void CConnectedAnimation::GetConfiguration(_Out_ IConnectedAnimationCoreConfiguration** configuration)
{
    Microsoft::WRL::ComPtr<IConnectedAnimationCoreConfiguration> config = m_configuration;
    *configuration = config.Detach();
}
void CConnectedAnimation::PutConfiguration(_In_ IConnectedAnimationCoreConfiguration* configuration)
{
    m_configuration = configuration;
}


_Check_return_ HRESULT CConnectedAnimation::Reset()
{
    // TODO: Stop any animations that are running

    // Clean up any element based data
    auto connectedAnimationRoot = GetConnectedAnimationRootNoRef();
    IFC_RETURN(ClearElementInfo(m_source));
    IFC_RETURN(ClearElementInfo(m_destination));

    // Remove the host visual from the tree
    if (m_hostVisual != nullptr)
    {
        GetContext()->NWGetWindowRenderTarget()->GetDCompTreeHost()->GetProjectedShadowManager()->UpdateCasterStatus(
            GetContext(),
            SourceType::Visual,
            m_hostVisual.Get(),       // visual
            nullptr,                  // uielement
            nullptr,                  // themeShadow
            true,                     // addToGlobalScene
            nullptr,                  // containingPopup
            GetXamlIslandRootNoRef()
            );

        IFC_RETURN(connectedAnimationRoot->RemoveConnectedAnimationVisual(m_hostVisual.Get()));
        m_hostVisual = nullptr;
    }

    if (m_scopedAnimationBatch != nullptr)
    {
        IFC_RETURN(m_scopedAnimationBatch->remove_Completed(m_scopedAnimationBatchCompletedToken));
        m_scopedAnimationBatch.Reset();
    }

    return S_OK;
}

_Check_return_ HRESULT CConnectedAnimation::UpdatePreRenderWalk()
{
    // See if we can complete the animation
    if (m_state == ConnectedAnimationState::ReadyToComplete)
    {
        IFC_RETURN(TryToCompleteAnimation());
    }

    // If our destination element needs to be re-rendered, then we will take a
    // new snapshot.  Note:  It is possible that due to reparenting (such as when using
    // LTEs), it is possible that the composition node is cleared, but no dirty flags
    // are set.  The node will get recreated in EnsureCompositionNode, so we also
    // need to ensure our snapshot gets updated.
    if (m_state != ConnectedAnimationState::Complete && m_destination.element != nullptr)
    {
        m_destination.refreshSnapshot = m_destination.element->GetCompositionPeer() == nullptr || m_destination.element->NWNeedsRendering();
    }

    return S_OK;
}

_Check_return_ HRESULT CConnectedAnimation::TimeoutIfNecessary(_Out_ UINT64* requestTickInverval)
{
    *requestTickInverval = 0;
    if (m_state == ConnectedAnimationState::Prepared)
    {
        auto currentTime = gps->GetCPUMilliseconds();
        if(currentTime - m_preparedTimestamp >= s_timeout)
        {
             IFC_RETURN(Cancel());
        }
        else
        {
            // request for another tick later.
            *requestTickInverval = m_preparedTimestamp + s_timeout - currentTime;
        }
    }
    return S_OK;
}

_Check_return_ HRESULT CConnectedAnimation::UpdatePreCommit(_Out_ bool * remove, _Out_ bool * waitForCommitCompletion)
{
    // We only do work if we are Initialized or Started.  But we still want to keep the animation
    // around if we are Prepared or Running.
    *remove = false;
    *waitForCommitCompletion = false;

    // Ensure that a coordinated animation is consistent with its base.
    if (m_baseAnimation != nullptr && m_baseAnimation->m_state == ConnectedAnimationState::Canceled)
    {
        IFC_RETURN(Cancel());
    }

    switch (m_state)
    {
    case ConnectedAnimationState::Initialized:
    case ConnectedAnimationState::Unloaded:
        break;
    case ConnectedAnimationState::Started:
        // We can't officially start a coordinated animation until its base has started
        if (m_baseAnimation != nullptr && m_baseAnimation->m_state != ConnectedAnimationState::Running)
        {
            return S_OK;
        }
        break;
    case ConnectedAnimationState::Prepared:
        return S_OK;
    case ConnectedAnimationState::Running:
        // We may have cleared the destination snapshot, if so we need to fall
        // through and rebuild it.
        if (!m_destination.refreshSnapshot)
        {
            // The destination element's prepend visual could have changed, make sure it is still invisible.
            HWCompTreeNode* compNode = m_destination.element->GetCompositionPeer();
            // while we don't expect the compnode to be null, it could be if the application has chosen a
            // destination element that won't get a comp node, even if we requested it (e.g. it is sitting
            // outside a clipped area or a child of an LTE), so don't crash.
            ASSERT(compNode != nullptr);
            if (compNode != nullptr)
            {
                IFC_RETURN(compNode->SetConnectedAnimationRunning(true));
            }
            return S_OK;
        }
        break;
    case ConnectedAnimationState::ReadyToComplete:
        // We will check on the next prerenderwalk if we can complete this.  If we were
        // to do it here, we could remove our placeholder (if we can complete), but
        // the real element won't be in the scene until the next tick which could cause
        // it to flicker.
        return S_OK;
    default:
        *remove = true;
        return S_OK;
    }

    // Make sure we have a snapshot visual for our source (if we are doing a cooordinated animation
    // we don't need the actual snapshot).  If we have created the host visual assume that
    // we have created the source visual
    if (m_hostVisual == nullptr && m_baseAnimation == nullptr)
    {
        IFC_RETURN(CreateSnapshotBrush(m_source, false /* createClippedBrush */, true /* freezeBrush */));

        // Due to an issue with DComp (Bug 10646539), if we attempt to remove the visual backing a visual
        // surface within the same DWM frame that we froze the brush, the brush gets re-visualized after the
        // visual is gone (and is then frozen).  So, if we freeze a brush, we need to make sure that we wait
        // for the current commit to complete before we can commit any tree updates that might
        // affect the visual surface.
        *waitForCommitCompletion = true;

        m_source.refreshSnapshot = false;

        if (m_state == ConnectedAnimationState::Initialized || m_state == ConnectedAnimationState::Unloaded)
        {
            // This is all we need to do unless we have started the animation
            m_state = ConnectedAnimationState::Prepared;
            return S_OK;
        }
    }

    // Remove the animation if the destination element is not in the live tree.
    // When user calls PrepareAnimation, the state will go to Initialized,
    // and if before user calling TryStart() we ticked and  UpdatePreCommit is called,
    // then we need the above if block to properly prepare the source snapshot and change state to Prepared.
    // We cannot move this before the previous if block,
    // because if we do so, this could be called before user calls TryStart(),
    // and destination element is null before TryStart().
    if (!m_destination.element->IsActive())
    {
        *remove = true;
        return S_OK;
    }

    // Compute the transforms for the destination and create our snapshot visual for it.
    IFC_RETURN(GetSnapshotTransformInfo(m_destination));

    // If we are a coordindated animation (m_baseAnimation specified), then use the
    // the base animation to compute the source information
    if (m_baseAnimation != nullptr)
    {
        // If our destination has zero height or width, then it doesn't make any sense
        // to actually animate it since it will never be seen.
        if (m_baseAnimation->m_destination.clippedRect.Width == 0 || m_baseAnimation->m_destination.clippedRect.Height == 0)
        {
            IFC_RETURN(Cancel());
        }
        else
        {
            GetBaseAnimationSourceInfo();
        }
    }

    if (m_state == ConnectedAnimationState::Canceled)
    {
        *remove = true;
        return S_OK;
    }

    // If we are not animating scale then we need to convert our source brush (which is always created as unclipped)
    // to a clipped brush and update our host visual's size and position.  Note: if we are a coordinated animation, then
    // we don't have a source brush, so do nothing.
    if (!m_isScaleAnimationEnabled && m_baseAnimation == nullptr)
    {
        // If we have already converted or we don't have a source brush, we don't need to do this.
        // If we already have our temp visual (the souce of the clipped source brush), then we
        // already have a clipped brush.

        if (!m_isSourceBrushClipped && m_source.surfaceBrush != nullptr)
        {
            IFC_RETURN(ConvertSourceBrushToClipped());
        }

        IFC_RETURN(UpdateVisualRectForClippedBrushes());
    }

    // REVIEW: We shouldn't need to refresh the snap shot if we are a brush, that should happen automatically
    // since we aren't frozen.  We need to confirm this before removing this code.
    if (m_destination.visual == nullptr || m_destination.refreshSnapshot)
    {
        IFC_RETURN(CreateSnapshotBrush(m_destination, !m_isScaleAnimationEnabled /* createClippedBrush */, false /* freezeBrush */));
        m_destination.refreshSnapshot = false;
    }

    // If we aren't being requested to start the animate (e.g. we are already running), then we are done.
    // If we need to delay the start so a BringIntoView can process, then we are done (until the next tick)
    if (m_state != ConnectedAnimationState::Started || m_connectedAnimationServiceNoRef->AnimationStartsAreDelayed())
    {
        return S_OK;
    }

    WUComp::ICompositor * compositorNoRef = GetContext()->NWGetWindowRenderTarget()->GetDCompTreeHost()->GetCompositor();

    // Create a scoped batch
    IFC_RETURN(compositorNoRef->CreateScopedBatch(WUComp::CompositionBatchTypes_Animation, &m_scopedAnimationBatch));

    // if we don't have a host visual that means we were unable to get either a source or destination snapshot.  Rather than
    // crashing, we will just complete the animation.
    if (m_hostVisual == nullptr)
    {
        m_state = ConnectedAnimationState::Complete;
        FireCompletedEvent();
        IFC_RETURN(Reset());
        return S_OK;
    }

    IFC_RETURN(StartSpriteAnimations());

    // End the batch and register for the completed event
    m_scopedAnimationBatch->End();
    auto callback = wrl::Callback <
        wrl::Implements <
        wrl::RuntimeClassFlags<wrl::ClassicCom>,
        wf::ITypedEventHandler<IInspectable*, WUComp::CompositionBatchCompletedEventArgs*>,
        wrl::FtmBase >>
        (this, &CConnectedAnimation::OnAnimationCompleted);

    IFC_RETURN(m_scopedAnimationBatch->add_Completed(callback.Get(), &m_scopedAnimationBatchCompletedToken));

    m_state = ConnectedAnimationState::Running;

    return S_OK;
}

_Check_return_ HRESULT CConnectedAnimation::StartSpriteAnimations()
{
    wf::TimeSpan duration;
    Microsoft::WRL::ComPtr<WUComp::ICompositionEasingFunction> easingFunction;
    Microsoft::WRL::ComPtr<IConnectedAnimationCoreConfiguration> configuration = m_configuration;

    if (configuration == nullptr)
    {
        IFC_RETURN(ConnectedAnimationConfiguration_GetDefault(&configuration));
    }

    if (configuration == nullptr)
    {
        // We didn't have a configuration (or they were disabled), so fall back to the RS4 behavior.
        duration = m_connectedAnimationServiceNoRef->GetAnimationDuration();
        IFC_RETURN(m_connectedAnimationServiceNoRef->GetDefaultEasingFunction(&easingFunction));
    }
    else
    {
        IFC_RETURN(configuration->GetEffectiveDuration(&duration));
        IFC_RETURN(configuration->GetEffectiveEasingFunction(&easingFunction));
    }

    DCompTreeHost* dcompTreeHost = GetContext()->NWGetWindowRenderTarget()->GetDCompTreeHost();
    WUComp::ICompositor * compositorNoRef = dcompTreeHost->GetCompositor();

    Microsoft::WRL::ComPtr<WUComp::ICompositionObject> hostVisualObject;
    IFC_RETURN(m_hostVisual.As(&hostVisualObject));

    // Compute our target scale and offsets
    wfn::Vector3 scale = { 1, 1, 1 };
    wfn::Vector3 sourceOffset;
    wfn::Vector3 destOffset;
    ComputeAnimationOffset(m_source, m_destination, sourceOffset);
    ComputeAnimationOffset(m_destination, m_source, destOffset);
    if (m_isScaleAnimationEnabled && m_source.unclippedRect.Width > 0 && m_source.unclippedRect.Height > 0)
    {
        scale.X = m_destination.unclippedRect.Width / m_source.unclippedRect.Width;
        scale.Y = m_destination.unclippedRect.Height / m_source.unclippedRect.Height;
    }

    // Allow the configuration to provide an animation effect.
    WUComp::ICompositionObject* offsetAnimationObjectNoRef = hostVisualObject.Get();
    WUComp::ICompositionObject* scaleAnimationObjectNoRef = hostVisualObject.Get();
    if (configuration != nullptr)
    {
        IFC_RETURN(configuration->GetEffectPropertySet(scale, &m_effectPropertySet));

        // If the configuration returned a property set then we need to combine this with our primary animation.  We
        // accomplish this by...
        //    1. Creating a primary animation property set
        //    2. Creating an expression animation that combines the effectPropertySet (returned from the configuration)
        //       and the primaryPropertySet and set that on the host visual.
        //    3. Animate the primaryPropertySet values.
        if (m_effectPropertySet != nullptr)
        {
            IFC_RETURN(compositorNoRef->CreatePropertySet(&m_primaryPropertySet));
            Microsoft::WRL::ComPtr<WUComp::ICompositionObject> primaryPSObject;
            Microsoft::WRL::ComPtr<WUComp::ICompositionObject> effectPSObject;
            IFC_RETURN(m_primaryPropertySet.As(&primaryPSObject));
            IFC_RETURN(m_effectPropertySet.As(&effectPSObject));
            wfn::Vector3 vector;
            WUComp::CompositionGetValueStatus getValueStatus;
            IFC_RETURN(m_effectPropertySet->TryGetVector3(wrl_wrappers::HStringReference(L"Offset").Get(), &vector, &getValueStatus));
            if (getValueStatus == WUComp::CompositionGetValueStatus_Succeeded)
            {
                wrl::ComPtr<WUComp::IExpressionAnimation> expressionAnimation;
                Microsoft::WRL::ComPtr<WUComp::ICompositionAnimation> compositionAnimation;
                IFC_RETURN(m_primaryPropertySet->InsertVector3(wrl_wrappers::HStringReference(L"Offset").Get(), sourceOffset));
                IFC_RETURN(compositorNoRef->CreateExpressionAnimationWithExpression(wrl_wrappers::HStringReference(L"PrimaryPS.Offset + EffectPS.Offset").Get(), &expressionAnimation));
                IFC_RETURN(expressionAnimation.As(&compositionAnimation));
                IFC_RETURN(compositionAnimation->SetReferenceParameter(wrl_wrappers::HStringReference(L"PrimaryPS").Get(), primaryPSObject.Get()));
                IFC_RETURN(compositionAnimation->SetReferenceParameter(wrl_wrappers::HStringReference(L"EffectPS").Get(), effectPSObject.Get()));
                IFC_RETURN(hostVisualObject->StartAnimation(wrl_wrappers::HStringReference(L"Offset").Get(), compositionAnimation.Get()));
                offsetAnimationObjectNoRef = primaryPSObject.Get();
            }
            IFC_RETURN(m_effectPropertySet->TryGetVector3(wrl_wrappers::HStringReference(L"Scale").Get(), &vector, &getValueStatus));
            if (getValueStatus == WUComp::CompositionGetValueStatus_Succeeded)
            {

                wfn::Vector3 naturalScale = { 1, 1, 1 };
                wrl::ComPtr<WUComp::IExpressionAnimation> expressionAnimation;
                Microsoft::WRL::ComPtr<WUComp::ICompositionAnimation> compositionAnimation;
                IFC_RETURN(m_primaryPropertySet->InsertVector3(wrl_wrappers::HStringReference(L"Scale").Get(), naturalScale));

                IFC_RETURN(compositorNoRef->CreateExpressionAnimationWithExpression(wrl_wrappers::HStringReference(L"PrimaryPS.Scale * EffectPS.Scale").Get(), &expressionAnimation));
                IFC_RETURN(expressionAnimation.As(&compositionAnimation));
                IFC_RETURN(compositionAnimation->SetReferenceParameter(wrl_wrappers::HStringReference(L"PrimaryPS").Get(), primaryPSObject.Get()));
                IFC_RETURN(compositionAnimation->SetReferenceParameter(wrl_wrappers::HStringReference(L"EffectPS").Get(), effectPSObject.Get()));
                IFC_RETURN(hostVisualObject->StartAnimation(wrl_wrappers::HStringReference(L"Scale").Get(), compositionAnimation.Get()));
                scaleAnimationObjectNoRef = primaryPSObject.Get();
            }
        }
    }

    // Offset animation for the host visual.
    {
        if (m_componentAnimation[ConnectedAnimationComponent::ConnectedAnimationComponent_OffsetX] == nullptr && m_componentAnimation[ConnectedAnimationComponent::ConnectedAnimationComponent_OffsetY] == nullptr)
        {
            IFC_RETURN(StartVector3Animation(offsetAnimationObjectNoRef, L"Offset", sourceOffset, destOffset, duration, easingFunction.Get(), compositorNoRef));
        }
        else
        {
            IFC_RETURN(StartCustomAnimation(offsetAnimationObjectNoRef, L"Offset.x", ConnectedAnimationComponent::ConnectedAnimationComponent_OffsetX, sourceOffset.X, destOffset.X, duration, easingFunction.Get(), compositorNoRef));
            IFC_RETURN(StartCustomAnimation(offsetAnimationObjectNoRef, L"Offset.y", ConnectedAnimationComponent::ConnectedAnimationComponent_OffsetY, sourceOffset.Y, destOffset.Y, duration, easingFunction.Get(), compositorNoRef));
        }
    }

    // Scale animation for source snapshot viusal
    if (scale.X != 1.0f || scale.Y != 1.0f)
    {
        if (m_componentAnimation[ConnectedAnimationComponent::ConnectedAnimationComponent_Scale] == nullptr)
        {
            wfn::Vector3 naturalScale = { 1, 1, 1 };
            IFC_RETURN(StartVector3Animation(scaleAnimationObjectNoRef, L"Scale", naturalScale, scale, duration, easingFunction.Get(), compositorNoRef));
        }
        else
        {
            IFC_RETURN(StartCustomAnimation(scaleAnimationObjectNoRef, L"Scale.x", ConnectedAnimationComponent::ConnectedAnimationComponent_Scale, 1, scale.X, duration, easingFunction.Get(), compositorNoRef));
            IFC_RETURN(StartCustomAnimation(scaleAnimationObjectNoRef, L"Scale.y", ConnectedAnimationComponent::ConnectedAnimationComponent_Scale, 1, scale.Y, duration, easingFunction.Get(), compositorNoRef));
        }
    }

    // Start the clip animation
    if (m_insetClip != nullptr)
    {
        XRECTF_RB sourceClip = {
            m_source.clippedRect.X - m_source.unclippedRect.X,
            m_source.clippedRect.Y - m_source.unclippedRect.Y,
            m_source.unclippedRect.Right() - m_source.clippedRect.Right(),
            m_source.unclippedRect.Bottom() - m_source.clippedRect.Bottom()
        };

        XRECTF_RB destinationClip = {
            (m_destination.clippedRect.X - m_destination.unclippedRect.X) / scale.X,
            (m_destination.clippedRect.Y - m_destination.unclippedRect.Y) / scale.Y,
            (m_destination.unclippedRect.Right() - m_destination.clippedRect.Right()) / scale.X,
            (m_destination.unclippedRect.Bottom() - m_destination.clippedRect.Bottom()) / scale.Y
        };


        Microsoft::WRL::ComPtr<WUComp::ICompositionObject> clipObject;
        IFC_RETURN(m_insetClip.As(&clipObject));

        IFC_RETURN(StartCustomAnimation(clipObject.Get(), L"LeftInset", ConnectedAnimationComponent::ConnectedAnimationComponent_CrossFade, sourceClip.left, destinationClip.left, duration, easingFunction.Get(), compositorNoRef));
        IFC_RETURN(StartCustomAnimation(clipObject.Get(), L"TopInset", ConnectedAnimationComponent::ConnectedAnimationComponent_CrossFade, sourceClip.top, destinationClip.top, duration, easingFunction.Get(), compositorNoRef));
        IFC_RETURN(StartCustomAnimation(clipObject.Get(), L"RightInset", ConnectedAnimationComponent::ConnectedAnimationComponent_CrossFade, sourceClip.right, destinationClip.right, duration, easingFunction.Get(), compositorNoRef));
        IFC_RETURN(StartCustomAnimation(clipObject.Get(), L"BottomInset", ConnectedAnimationComponent::ConnectedAnimationComponent_CrossFade, sourceClip.bottom, destinationClip.bottom, duration, easingFunction.Get(), compositorNoRef));
    }

    // Our opacity animation is different depending on what brush/brushes we have
    //    CrossFadeEffect Brush - animate Crossfade.Weight from 0 to 1.
    //    Source brush - animate opacity from 1 to 0
    //    Destination brush - animate opacity from 1 to 0
    Microsoft::WRL::ComPtr<WUComp::ICompositionBrush> compositionBrush;
    Microsoft::WRL::ComPtr<WUComp::ICompositionEffectBrush> effectBrush;
    Microsoft::WRL::ComPtr<WUComp::ISpriteVisual> spriteVisual;

    IFC_RETURN(m_hostVisual.As(&spriteVisual));
    IFC_RETURN(spriteVisual->get_Brush(&compositionBrush));
    if (SUCCEEDED(compositionBrush.As(&effectBrush)))
    {
        Microsoft::WRL::ComPtr<WUComp::ICompositionObject> targetObject;
        Microsoft::WRL::ComPtr<WUComp::ICompositionPropertySet> propertySet;
        Microsoft::WRL::ComPtr<WUComp::ICompositionAnimation> animation;
        Microsoft::WRL::ComPtr<WUComp::ICompositionObject> compositionObject;

        IFC_RETURN(compositionBrush.As(&compositionObject));
        IFC_RETURN(compositionObject->get_Properties(&propertySet));
        IFC_RETURN(propertySet.As(&targetObject));
        IFC_RETURN(StartCustomAnimation(targetObject.Get(), L"Crossfade.Weight", ConnectedAnimationComponent::ConnectedAnimationComponent_CrossFade, 0, 1, duration, easingFunction.Get(), compositorNoRef));
    }
    else
    {
        float startValue;
        IFC_RETURN(m_hostVisual->get_Opacity(&startValue));
        IFC_RETURN(StartCustomAnimation(hostVisualObject.Get(), L"Opacity", ConnectedAnimationComponent::ConnectedAnimationComponent_CrossFade, startValue, startValue == 0 ? 1.0f : 0.0f, duration, easingFunction.Get(), compositorNoRef));
    }

    // The ThemeShadow feature doesn't do the runtime check and we don't want to
    // call if it is disabled.
    // If the configuration requests a shadow, then make our host visual a caster.
    if (configuration != nullptr && configuration->ShouldShadowBeCast())
    {
        CREATEPARAMETERS cp(GetContext());
        xref_ptr<CThemeShadow> shadow;
        IFC_RETURN(CThemeShadow::Create((CDependencyObject**)shadow.ReleaseAndGetAddressOf(), &cp));
        shadow->SetMask(compositionBrush.Get());
        dcompTreeHost->GetProjectedShadowManager()->UpdateCasterStatus(
            GetContext(),
            SourceType::Visual,
            m_hostVisual.Get(),       // visual
            nullptr,                  // uielement
            shadow.get(),             // themeShadow
            true,                     // addToGlobalScene
            nullptr,                  // containingPopup
            GetXamlIslandRootNoRef()
            );
    }

    return S_OK;
}

void CConnectedAnimation::ComputeAnimationOffset(_In_ ConnectedAnimationElementInfo& info, _In_ ConnectedAnimationElementInfo& info2, wfn::Vector3& offset)
{
    // currently z is always zero
    offset.Z = 0;

    // If we are animating scale, then it is just the unclipped offset of the element (regardless of whether it
    // is a coordinated animation or not).
    if (m_isScaleAnimationEnabled)
    {
        offset.X = info.unclippedRect.X;
        offset.Y = info.unclippedRect.Y;
        return;
    }

    // Since we are not scaling, we need to adjust the element's offset based upon the relative sizes of the
    // source and destination.

    // The default offset is the natural position of the primary rectangle.
    offset.X = info.clippedRect.X;
    offset.Y = info.clippedRect.Y;

    // If we are not a coordinated animation, then our adjustment needs to be  based up our base animation
    // rects, not our own.  If our primary width is less than the secondary width, we adjust the offset
    // by half the difference to account for the fact that we are rendering the union of both the source
    // and the destination rectangles.  Similarly with height.
    if (m_baseAnimation == nullptr)
    {
        if (info.clippedRect.Width < info2.clippedRect.Width)
        {
            offset.X -= (info2.clippedRect.Width - info.clippedRect.Width) / 2;
        }
        if (info.clippedRect.Height < info2.clippedRect.Height)
        {
            offset.Y -= (info2.clippedRect.Height - info.clippedRect.Height) / 2;
        }
        return;
    }

    // The destination location for a coordinated animation is simply its natural position.
    if (&info == &m_destination) return;

    // The source location for a coordinated animation is offset based upon the position of the base destination image in
    // relation to the source.  Since they are centered over each other we adjust for any offset that might exist
    offset.X += (m_baseAnimation->m_source.clippedRect.Width - m_baseAnimation->m_destination.clippedRect.Width) / 2;
    offset.Y += (m_baseAnimation->m_source.clippedRect.Height - m_baseAnimation->m_destination.clippedRect.Height) / 2;
}

void CConnectedAnimation::GetBaseAnimationSourceInfo()
{
    // Coordinated connected animations don't have a souce snapshot, so we create a rectangle for the source
    // that is basically, "compute where a source element would be if it had the same layout relationship to
    // the source as the coordinated element has to the destination".  We will then animation from the
    // placeholder rectangle to the coordinated element's natural position.
    m_source.transform = m_baseAnimation->m_destination.transform;

    // If our base animations is not scaling then neither should we
    m_isScaleAnimationEnabled = m_baseAnimation->m_isScaleAnimationEnabled;

    // We should never get here if our clipped rect has a zero size component and the unclipped should be
    // always be greater than the clipped.
    ASSERT(m_baseAnimation->m_destination.unclippedRect.Width > 0);
    ASSERT(m_baseAnimation->m_destination.unclippedRect.Height > 0);

    // If we are animating scale the we need to use the unlcipped rectangles of the base animaton as that is what
    // is getting scaled.  However, of we are suppressing the scale animation, then we use clipped size.
    XRECTF_WH sourceRectBase = m_isScaleAnimationEnabled ? m_baseAnimation->m_source.unclippedRect : m_baseAnimation->m_source.clippedRect;
    XRECTF_WH destinationRectBase = m_isScaleAnimationEnabled ? m_baseAnimation->m_destination.unclippedRect : m_baseAnimation->m_destination.clippedRect;
    XRECTF_WH destinationRect = m_isScaleAnimationEnabled ? m_destination.unclippedRect : m_destination.clippedRect;

    // Match the scaling factor to the base
    float scaleX = m_isScaleAnimationEnabled ? sourceRectBase.Width / destinationRectBase.Width : 1.0f;
    float scaleY = m_isScaleAnimationEnabled ? sourceRectBase.Height / destinationRectBase.Height : 1.0f;

    // Determine the offset of the coordinated element to the base
    float offsetX = destinationRect.X - destinationRectBase.X;
    float offsetY = destinationRect.Y - destinationRectBase.Y;

    // Coordinated animation sources (since they don't really exist) always have the same clipped and
    // unclipped rects, so construct one and set the other.  Be sure to account for the scale factor
    // in determining the corresponding source position.
    m_source.clippedRect.X = sourceRectBase.X + (offsetX * scaleX);
    m_source.clippedRect.Y = sourceRectBase.Y + (offsetY * scaleY);
    m_source.clippedRect.Width = destinationRect.Width * scaleX;
    m_source.clippedRect.Height = destinationRect.Height * scaleY;

    m_source.unclippedRect = m_source.clippedRect;
}

_Check_return_ HRESULT CConnectedAnimation::CreateSnapshotBrush(_In_ ConnectedAnimationElementInfo & info, bool createClipped, bool freezeBrush) noexcept
{
    CWindowRenderTarget* pRenderTargetNoRef = GetContext()->NWGetWindowRenderTarget();
    DCompTreeHost* pDCompTreeHostNoRef = pRenderTargetNoRef->GetDCompTreeHost();

    HWCompTreeNode* compNode = info.element->GetCompositionPeer();

    if (compNode == nullptr)
    {
        // Our element has no visibility so we won't do anything with it.  The code at the
        // end of this function handles cases where we don't have a destination or a source.
        return S_OK;
    }

    // A clippable brush encompasses the entire visual, while an unclippable one, only
    // encompasses the part of the visual that isn't clipped (i.e. is it already clipped)
    wfn::Vector2 realizationSize = {
        createClipped ? info.clippedRect.Width : info.unclippedRect.Width,
        createClipped ? info.clippedRect.Height : info.unclippedRect.Height
    };
    // Dcomp brushes sometimes don't like sizes that are less than half a pixel, so if either height or width is less than half a pixel
    // treat it as if we didn't have a comp node.
    if (realizationSize.X < 0.5f || realizationSize.Y < 0.5f)
    {
        return S_OK;
    }

    Microsoft::WRL::ComPtr<WUComp::IVisual> primaryVisual;
    IFC_RETURN(compNode->SetConnectedAnimationRunning(true, &primaryVisual));

    // Create our visual surface
    wrl::ComPtr<WUComp::ICompositionVisualSurface> visualSurface;

    Microsoft::WRL::ComPtr<WUComp::ICompositorWithVisualSurface> compositorVisualSurface;
    IFC_RETURN(pDCompTreeHostNoRef->GetCompositor()->QueryInterface(IID_PPV_ARGS(&compositorVisualSurface)));
    IFC_RETURN(compositorVisualSurface->CreateVisualSurface(&visualSurface));

    wrl::ComPtr<WUComp::Internal::ICompositionVisualSurfacePartner> visualSurfacePartner;
    IFC_RETURN(visualSurface.As(&visualSurfacePartner));

    // Assign our source visual to it and set its properties.
    {
        IFC_RETURN(visualSurface->put_SourceVisual(primaryVisual.Get()));

        IFC_RETURN(visualSurfacePartner->put_RealizationSize(realizationSize));

        float scaleX;
        float scaleY;
        info.transform.GetScaleDimensions(&scaleX, &scaleY);

        float left = info.transform.GetDx();
        float top = info.transform.GetDy();

        // The element's offset is included in our transform, but that shouldn't be included in our actual
        // snapshot bounds, so back it out.
        {
            left -= info.element->GetActualOffsetX();
            top -= info.element->GetActualOffsetY();
        }

        // If we are creating a clipped brush, then our surface rectangle will be
        // offset in the visual's area by the amount clipped.
        if (createClipped)
        {
            left += info.clippedRect.X - info.unclippedRect.X;
            top += info.clippedRect.Y - info.unclippedRect.Y;
        }

        // Right and Bottom must account for the offset, but also needs to reduce itself by
        // the parent scaling that has occurred.  This gets 're-applied" when we stretch
        // the source into the realization sized surface.
        float right = (realizationSize.X / scaleX) + left;
        float bottom = (realizationSize.Y / scaleY) + top;

        wfn::Vector2 sourceOffset = { left, top };
        wfn::Vector2 sourceSize = { right - left, bottom - top };

        visualSurface->put_SourceOffset(sourceOffset);
        visualSurface->put_SourceSize(sourceSize);
        visualSurfacePartner->put_Stretch(createClipped ? WUComp::CompositionStretch_None : WUComp::CompositionStretch_Fill);
    }

    // Create a surface brush from the surface
    {
        Microsoft::WRL::ComPtr<WUComp::ICompositionSurfaceBrush> surfaceBrush;
        Microsoft::WRL::ComPtr<WUComp::ICompositionSurface> surface;
        IFC_RETURN(visualSurface.As(&surface));
        IFC_RETURN(pDCompTreeHostNoRef->GetCompositor()->CreateSurfaceBrushWithSurface(surface.Get(), &surfaceBrush));
        IFC_RETURN(surfaceBrush.As(&info.surfaceBrush));
        surfaceBrush->put_Stretch(createClipped ? WUComp::CompositionStretch_None : WUComp::CompositionStretch_Fill);
        // We need to account for parent flip if necessary
        wfn::Vector2 scale = { info.transform.GetM11() < 0 ? -1.0f : 1.0f, info.transform.GetM22() < 0 ? -1.0f : 1.0f };
        if (scale.X != 1 || scale.Y != 1)
        {
            // Note: It seems that the scale and centerpoint properties on a brush are actually applied
            //       to the visual that the brush is applied to, not the underlying composition surface
            //       as the brush is applied.  This means that our center point computation is not the
            //       the center point of the composition surface backing the brush, but the center point
            //       of the visual the brush will be applied to.
            Microsoft::WRL::ComPtr<WUComp::ICompositionSurfaceBrush2> surfaceBrush2;
            IFC_RETURN(surfaceBrush.As(&surfaceBrush2));
            IFC_RETURN(surfaceBrush2->put_Scale(scale));

            // The centerpoint will always be the center of the visual which in the clipped scenario is the maximum of the
            // the source size and the destination.
            if (createClipped)
            {
                // Since we can't create a clippped brush until we are handling the destination, we don't need to worry about
                // that not being initialized yet.
                wfn::Vector2 centerPoint = {
                    centerPoint.X = max(m_source.unclippedRect.Width, m_destination.unclippedRect.Width) / 2,
                    centerPoint.Y = max(m_source.unclippedRect.Height, m_destination.unclippedRect.Height) / 2
                };
                IFC_RETURN(surfaceBrush2->put_CenterPoint(centerPoint));
            }
            else
            {
                wfn::Vector2 centerPoint = { m_source.unclippedRect.Width / 2,  m_source.unclippedRect.Height / 2 };
                IFC_RETURN(surfaceBrush2->put_CenterPoint(centerPoint));
            }
        }
    }

    // Update the host visual with the new brush

    // Ensure we have our animation root and host visual
    if (m_hostVisual == nullptr)
    {
        IFC_RETURN(GetConnectedAnimationRootNoRef()->CreateConnectedAnimationVisual(&m_hostVisual));

        // Although we may trigger the creation of the host visual via either the source or destination,
        // we always give it an initial position and size based upon the source.  Note, this size may change
        // based upon whether we are animating scale or not, but at this point we just assume that we are.
        wfn::Vector3 offset = { m_source.unclippedRect.X, m_source.unclippedRect.Y, 0.0f };
        IFC_RETURN(m_hostVisual->put_Offset(offset));

        wfn::Vector2 size = { m_source.unclippedRect.Width, m_source.unclippedRect.Height };
        IFC_RETURN(m_hostVisual->put_Size(size));
    }

    // Freeze the Brush/Visual Surface if requested to do so
    if (freezeBrush)
    {
        IFC_RETURN(visualSurfacePartner->Freeze());
     }

    Microsoft::WRL::ComPtr<WUComp::ISpriteVisual> spriteVisual;
    IFC_RETURN(m_hostVisual.As(&spriteVisual));

    // If we need an inset clip create it.  Note: We require the clip if either the source
    // or the destination need a clip (non zero insets), but always initialize it to the
    // the source clip.  Here again, our assumption is that we will be animating scale.  We
    // will make adjustments later, if we need to.
    if (!createClipped && m_insetClip == nullptr && info.clippedRect != info.unclippedRect)
    {
        IFC_RETURN(pDCompTreeHostNoRef->GetCompositor()->CreateInsetClipWithInsets(
            m_source.clippedRect.X - m_source.unclippedRect.X,
            m_source.clippedRect.Y - m_source.unclippedRect.Y,
            m_source.unclippedRect.X + m_source.unclippedRect.Width - m_source.clippedRect.X - m_source.clippedRect.Width,
            m_source.unclippedRect.Y + m_source.unclippedRect.Height - m_source.clippedRect.Y - m_source.clippedRect.Height,
            &m_insetClip
        ));

        Microsoft::WRL::ComPtr<WUComp::ICompositionClip> clip;
        IFC_RETURN(m_insetClip.As(&clip));
        IFC_RETURN(m_hostVisual->put_Clip(clip.Get()));
    }

    // Add the brush to the host visual.  How we put the brush on the visual depends on which brushes we have

    if (m_destination.surfaceBrush == nullptr)
    {
        // We only have a source brush, so we will fade out the visual.
        ASSERT(m_source.surfaceBrush != nullptr);
        if (m_state != ConnectedAnimationState::Running)
        {
            IFC_RETURN(m_hostVisual->put_Opacity(1));
        }
        IFC_RETURN(spriteVisual->put_Brush(m_source.surfaceBrush.Get()));
        return S_OK;
    }

    if (m_source.surfaceBrush == nullptr)
    {
        // We only have a destination brush, so we will fade in the visual.
        if (m_state != ConnectedAnimationState::Running)
        {
            IFC_RETURN(m_hostVisual->put_Opacity(0));
        }
        IFC_RETURN(spriteVisual->put_Brush(m_destination.surfaceBrush.Get()));
        return S_OK;
    }

    // We have both brushes so we want to crossfade.  If we already have a crossfade
    // brush, then simply update the destination brush in it (we never update the
    // source brush as we can only generate it once).
    Microsoft::WRL::ComPtr<WUComp::ICompositionBrush> sourceBrush;
    IFC_RETURN(spriteVisual->get_Brush(&sourceBrush));
    if (sourceBrush != nullptr)
    {
        Microsoft::WRL::ComPtr<WUComp::ICompositionEffectBrush> effectBrush;
        if (SUCCEEDED(sourceBrush.As(&effectBrush)))
        {
            IFC_RETURN(effectBrush->SetSourceParameter(wrl_wrappers::HStringReference(L"source2").Get(), m_destination.surfaceBrush.Get()));
            return S_OK;
        }
    }

    // Create a new crossfade effect brush
    Microsoft::WRL::ComPtr<WUComp::ICompositionBrush> crossfadeBrush;

    // Effect brushes are created by creating a "property bag" which is used to define the type of effect
    // and parameters for that effect and then creating an effect factory for that type of effect brush.
    Microsoft::WRL::ComPtr<XAML_ABI_PARAMETER(Microsoft::UI::Composition::Effects::CrossFadeEffect)> effect;
    IFC_RETURN(Microsoft::WRL::MakeAndInitialize<XAML_ABI_PARAMETER(Microsoft::UI::Composition::Effects::CrossFadeEffect)>(&effect));

    IFC_RETURN(effect->put_Weight(0));
    effect->put_Name(wrl_wrappers::HStringReference(L"Crossfade").Get());

    Microsoft::WRL::ComPtr<WUComp::ICompositionEffectSourceParameterFactory> effectSourceFactory;
    IFC_RETURN(wf::GetActivationFactory(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Composition_CompositionEffectSourceParameter).Get(), &effectSourceFactory));

    // Create two graphics source parameters that we will use to set our brushes into the effect.
    {
        Microsoft::WRL::ComPtr<WUComp::ICompositionEffectSourceParameter> effectSourceParameter;
        IFC_RETURN(effectSourceFactory->Create(wrl_wrappers::HStringReference(L"source1").Get(), &effectSourceParameter));
        Microsoft::WRL::ComPtr<wgr::Effects::IGraphicsEffectSource> graphicsSource;
        IFC_RETURN(effectSourceParameter.As(&graphicsSource));
        IFC_RETURN(effect->put_Source1(graphicsSource.Get()));
    }
    {
        Microsoft::WRL::ComPtr<WUComp::ICompositionEffectSourceParameter> effectSourceParameter;
        IFC_RETURN(effectSourceFactory->Create(wrl_wrappers::HStringReference(L"source2").Get(), &effectSourceParameter));
        Microsoft::WRL::ComPtr<wgr::Effects::IGraphicsEffectSource> graphicsSource;
        IFC_RETURN(effectSourceParameter.As(&graphicsSource));
        IFC_RETURN(effect->put_Source2(graphicsSource.Get()));
    }

    // Create the effect factory.  When we do this we need to identify what properties we are going to want to animate.
    Microsoft::WRL::ComPtr<WUComp::ICompositionEffectFactory> effectFactory;

    Microsoft::WRL::ComPtr<wfci_::Vector<HSTRING>> animatableProperties;
    IFC_RETURN(wfci_::Vector<HSTRING>::Make(&animatableProperties));
    IFC_RETURN(animatableProperties->Append(wrl_wrappers::HStringReference(L"Crossfade.Weight").Get()));

    IFC_RETURN(pDCompTreeHostNoRef->GetCompositor()->CreateEffectFactoryWithProperties(effect.Get(), animatableProperties.Get(), &effectFactory));

    // Use the factory to create a brush and set our two source parameters.
    Microsoft::WRL::ComPtr<WUComp::ICompositionEffectBrush> crossfadeEffectBrush;
    IFC_RETURN(effectFactory->CreateBrush(&crossfadeEffectBrush));
    IFC_RETURN(crossfadeEffectBrush->SetSourceParameter(wrl_wrappers::HStringReference(L"source1").Get(), m_source.surfaceBrush.Get()));
    IFC_RETURN(crossfadeEffectBrush->SetSourceParameter(wrl_wrappers::HStringReference(L"source2").Get(), m_destination.surfaceBrush.Get()));

    // Assign the brush to the visual.
    Microsoft::WRL::ComPtr<WUComp::ICompositionBrush> brush;
    IFC_RETURN(crossfadeEffectBrush.As(&brush));
    IFC_RETURN(spriteVisual->put_Brush(brush.Get()));

    return S_OK;
}

HRESULT CConnectedAnimation::ConvertSourceBrushToClipped()
{
    // If we are not animation the scale factor between the source and destination, then we use clipped
    // brushes (brushes that only include the unclipped part of the visuals).  However, we don't know for
    // sure whether we need a clipped or unclipped brush until after the source element has been removed
    // from the tree.  Since we can create a clipped brush from an unclipped one, but not the reverse, we
    // always create the source brush initially as an unclipped brush.

    // This function will convert the unclipped brush to a clipped one, by rendering the clipped brush to
    // another visual and then creating a composition brush from that visual.

    WUComp::ICompositor * compositorNoRef = GetContext()->NWGetWindowRenderTarget()->GetDCompTreeHost()->GetCompositor();

    // Create a Visual of the full size and apply our current brush to it.  This will give us something we
    // can use to create a new visual surface from.
    Microsoft::WRL::ComPtr<WUComp::IVisual> visual;
    {
        Microsoft::WRL::ComPtr<WUComp::ISpriteVisual> tempVisual;
        IFC_RETURN(compositorNoRef->CreateSpriteVisual(&tempVisual));
        IFC_RETURN(tempVisual.As(&visual));
        wfn::Vector2 visualSize = { m_source.unclippedRect.Width, m_source.unclippedRect.Height };
        IFC_RETURN(visual->put_Size(visualSize));
        IFC_RETURN(tempVisual->put_Brush(m_source.surfaceBrush.Get()));

    }

    // Create our visual surface using the previously created visual, but limit it
    // to the clipped area.
    Microsoft::WRL::ComPtr<WUComp::ICompositionVisualSurface> visualSurface;
    Microsoft::WRL::ComPtr<WUComp::Internal::ICompositionVisualSurfacePartner> visualSurfacePartner;
    {
        Microsoft::WRL::ComPtr<WUComp::ICompositorWithVisualSurface> compositorVisualSurface;
        IFC_RETURN(compositorNoRef->QueryInterface(IID_PPV_ARGS(&compositorVisualSurface)));
        IFC_RETURN(compositorVisualSurface->CreateVisualSurface(&visualSurface));
        IFC_RETURN(visualSurface.As(&visualSurfacePartner));
        IFC_RETURN(visualSurface->put_SourceVisual(visual.Get()));

        wfn::Vector2 realizationSize = { m_source.clippedRect.Width, m_source.clippedRect.Height };
        IFC_RETURN(visualSurfacePartner->put_RealizationSize(realizationSize));

        float left = m_source.clippedRect.X - m_source.unclippedRect.X;
        float top = m_source.clippedRect.Y - m_source.unclippedRect.Y;

        wfn::Vector2 sourceOffset = { left, top };
        visualSurface->put_SourceOffset(sourceOffset);
        visualSurface->put_SourceSize(realizationSize);

        visualSurfacePartner->put_Stretch(WUComp::CompositionStretch_Fill);
    }

    // Create a surface brush from the surface
    Microsoft::WRL::ComPtr<WUComp::ICompositionSurfaceBrush> surfaceBrush;
    {
        Microsoft::WRL::ComPtr<WUComp::ICompositionSurface> surface;
        IFC_RETURN(visualSurface.As(&surface));
        IFC_RETURN(compositorNoRef->CreateSurfaceBrushWithSurface(surface.Get(), &surfaceBrush));
        surfaceBrush->put_Stretch(WUComp::CompositionStretch_None);

        // Replace our current brush with this new one.
        m_source.surfaceBrush = nullptr;
        IFC_RETURN(surfaceBrush.As(&m_source.surfaceBrush));
    }

    // Reset our inset clip
    m_insetClip = nullptr;
    m_hostVisual->put_Clip(nullptr);

    m_isSourceBrushClipped = true;

    return S_OK;
}

HRESULT CConnectedAnimation::UpdateVisualRectForClippedBrushes()
{
    // Our visual will be based upon the greater of the size of the source and destination
    wfn::Vector2 spriteSize = {
        max(m_source.clippedRect.Width, m_destination.clippedRect.Width),
        max(m_source.clippedRect.Height, m_destination.clippedRect.Height)
    };

    IFC_RETURN(m_hostVisual->put_Size(spriteSize));

    // Since the source brush may be smaller that the visual, we need to adjust
    // the visual's position so that it appears in the right spot.
    wfn::Vector3 offset = {
        m_source.clippedRect.X - (spriteSize.X - m_source.clippedRect.Width),
        m_source.clippedRect.Y - (spriteSize.Y - m_source.clippedRect.Height),
        0
    };
    IFC_RETURN(m_hostVisual->put_Offset(offset));

    return S_OK;
}

// Return true if unloading element is the animation element or one of its ancestors
bool CConnectedAnimation::IsUnloadingElementAncestor(_In_ CUIElement* unloadingElement, _In_ ConnectedAnimationElementInfo& animationElementInfo)
{
    bool isAncestor = false;
    if (animationElementInfo.element != nullptr)
    {
        if (unloadingElement == animationElementInfo.element)
        {
            if (m_state == ConnectedAnimationState::Initialized)
            {
                m_state = ConnectedAnimationState::Unloaded;
            }
            return true;
        }

        isAncestor = unloadingElement->IsAncestorOf(animationElementInfo.element);
    }

    return isAncestor;
}

_Check_return_ HRESULT CConnectedAnimation::OnUnloadingElement(_In_ CUIElement* unloadingElement, _Out_ bool* retainedToUnloadingStorage)
{
    *retainedToUnloadingStorage = false;

    bool isAncestor = IsUnloadingElementAncestor(unloadingElement, m_destination);
    if (isAncestor)
    {
        IFC_RETURN(Cancel());
    }

    if (m_source.refreshSnapshot)
    {
        isAncestor = IsUnloadingElementAncestor(unloadingElement, m_source);

        if (isAncestor)
        {
            *retainedToUnloadingStorage = true;
        }
    }
    return S_OK;
}

_Check_return_ HRESULT CConnectedAnimation::GetSnapshotTransformInfo(_In_ ConnectedAnimationElementInfo & info)
{
    // This function computes a transform for use in either taking a snapshot (old api) or creating a composition
    // surface brush (new api).
    //
    // For the snapshot API, it takes the primary visual of the UIElement which contains only the local transform,
    // together with a transform specifying how this primary visual is rendered.
    // From testing, it seems the API does this [local][snapshotTransform]
    // Hence we need to compute the transform to be used for creating the snapshot
    // The goal is to snapshot the Visual with world scale but without offset/rotation/skew.
    // So that the bitmap generated will be exactly the size of the Visual on screen.
    // To calculate this, we need to take the inverse of the local transform, multiply it with the world scale transform.
    // So we end up with this: [local][inverse local][world scale]
    //
    // For the composition surface brush API, we use the offset to determine the origin of the rectangle on
    // the composition surface from which the brush should take its data and the scale information to determine
    // whether we need to flip the image (e.g. if a parent has a flip transform applied).

    // For now we don't support 3D.  BUT.... The use of elevation in popups now means that anything that
    // is under a popup thinks it is in a 3D scene.
    //ASSERT(!CUIElement::IsInTransform3DSubtree(info.element, nullptr));

    // Get our clipped and unclipped rectangles
    XRECTF_RB bounds;
    IFC_RETURN(info.element->GetTightGlobalBounds(&bounds, false /*do not ignore clipping*/, true /* useTargetInformation - use manipulation target */));
    info.clippedRect.X = bounds.left;
    info.clippedRect.Y = bounds.top;
    info.clippedRect.Width = bounds.right - bounds.left;
    info.clippedRect.Height = bounds.bottom - bounds.top;

    TraceDCompSnapshotBoundsInfo(
        reinterpret_cast<uint64_t>(this),
        info.clippedRect.X,
        info.clippedRect.Y,
        info.clippedRect.Width,
        info.clippedRect.Height
    );

    IFC_RETURN(info.element->GetTightGlobalBounds(&bounds, true /*ignore clipping*/, true /* useTargetInformation - use manipulation target */));
    info.unclippedRect.X = bounds.left;
    info.unclippedRect.Y = bounds.top;
    info.unclippedRect.Width = bounds.right - bounds.left;
    info.unclippedRect.Height = bounds.bottom - bounds.top;

    // Get the global and local transforms
    xref_ptr<ITransformer> pTransformer;
    IFC_RETURN(info.element->TransformToRoot(pTransformer.ReleaseAndGetAddressOf()));
    CMILMatrix globalTransform = pTransformer->Get2DMatrixIgnore3D();

    CMILMatrix localTransform;
    // When rendering, we don't care about properties set by composition on the handoff visual. That will prevent problems like
    // creating a tiny mask because there's a WUC scale animation going on.
    (void)(info.element->GetLocalTransform(TransformRetrievalOptions::None, &localTransform));

    // The plateau scale isn't applied by Xaml. Instead it's applied in the ICoreWindowSiteBridge that hosts our
    // visual tree, or by the content bridge in the Xaml island. We won't pick it up by walking the UIElement tree, so
    // account for it explicitly.
    float rasterizationScale = RootScale::GetRasterizationScaleForElement(info.element);

    // Ignore any 3D transforms.
    // Take the global scale and adjust it to parent scale.
    float parentScaleX = rasterizationScale * globalTransform.GetM11() / localTransform.GetM11();
    float parentScaleY = rasterizationScale * globalTransform.GetM22() / localTransform.GetM22();

    // Determine the inner bounds and adjust as needed for RTL
    XRECTF_RB innerBounds;
    IFC_RETURN(info.element->GetTightInnerBounds(&innerBounds));

    if (info.element->IsRightToLeft())
    {
        // BUG 10626859:  In RTL mode, it seems that the GetTightGlobalBounds does not account
        // for the flip when the content is smaller than the element.  Thus the value returned
        // is the LTR position of the item and we need to maunally adjust this to get the right
        // value.
        info.clippedRect.X -= (innerBounds.left * globalTransform.GetM11());
        info.unclippedRect.X -= (innerBounds.left * globalTransform.GetM11());

        // In addition, the inner bounds are reversed so we need to adjust them.
        float boundsWidth = innerBounds.right - innerBounds.left;
        innerBounds.left = info.element->GetActualWidth() - innerBounds.right;
        innerBounds.right = innerBounds.left + boundsWidth;
    }

    // We take snapshot based on its content bounds, so determine the position of the
    // the content in respect to the local transform.
    float localScaleX = localTransform.GetM11();
    float localScaleY = localTransform.GetM22();
    float localOffsetX = localTransform.GetDx();
    float localOffsetY = localTransform.GetDy();

    if (localScaleX >= 0)
    {
        localOffsetX += (innerBounds.left * localScaleX);
    }
    else
    {
        localScaleX *= -1;
        localOffsetX -= ((innerBounds.right - innerBounds.left) * localScaleX);
    }

    if (localScaleY >= 0)
    {
        localOffsetY += (innerBounds.top * localScaleY);
    }
    else
    {
        localScaleY *= -1;
        localOffsetY -= ((innerBounds.bottom - innerBounds.top) * localScaleY);
    }

    // Contruct our transform that we will pass into the snapshot or brush routines.  This transform
    // accounts for the parent scale and the corresponding offset relative to the local transform.
    info.transform.SetToIdentity();
    info.transform.SetM11(parentScaleX);
    info.transform.SetM22(parentScaleY);
    info.transform.SetDx(localOffsetX);
    info.transform.SetDy(localOffsetY);

    return S_OK;
}

_Check_return_ HRESULT CConnectedAnimation::ClearElementInfo(_In_ ConnectedAnimationElementInfo & info)
{
    if (info.element == nullptr) return S_OK;

    if (info.element->HasActiveConnectedAnimation())
    {
        info.element->UnsetRequiresComposition(CompositionRequirement::HasConnectedAnimation, IndependentAnimationType::None);
        CUIElement::NWSetContentDirty(info.element, DirtyFlags::Render); // Force dirty the render to make sure the comp node is cleared..
    }

    // If we have a compnode then we need to fix up the comp tree.  Note that it is possible that the
    // compnode went away when we reset the HasActiveConnectedAnimation flag.  This is OK.
    HWCompTreeNode* compNode = info.element->GetCompositionPeer();
    if (compNode != nullptr)
    {
        IFC_RETURN(compNode->SetConnectedAnimationRunning(false));
    }

    info.element = nullptr;
    info.visual = nullptr;
    return S_OK;
}

void CConnectedAnimation::FireCompletedEvent()
{
    if (m_pEventList)
    {
        CEventManager *pEventManager = GetContext()->GetEventManager();
        if (pEventManager)
        {
            pEventManager->Raise(EventHandle(KnownEventIndex::ConnectedAnimation_Completed), true, this, nullptr);
        }
    }
}

_Check_return_ HRESULT CConnectedAnimation::AddEventListener(
    _In_ EventHandle hEvent,
    _In_ CValue *pValue,
    _In_ XINT32 iListenerType,
    _Out_opt_ CValue *pResult,
    _In_ bool fHandledEventsToo)
{
    return CEventManager::AddEventListener(this, &m_pEventList, hEvent, pValue, iListenerType, pResult, fHandledEventsToo);
}


_Check_return_ HRESULT CConnectedAnimation::RemoveEventListener(
    _In_ EventHandle hEvent,
    _In_ CValue *pValue)
{
    return CEventManager::RemoveEventListener(this, m_pEventList, hEvent, pValue);
}

_Check_return_ HRESULT CConnectedAnimation::StartCustomAnimation(
    _In_ WUComp::ICompositionObject * compositionObject,
    _In_ LPCWSTR propertyName,
    _In_ ConnectedAnimationComponent animationComponent,
    _In_ float from,
    _In_ float to,
    _In_ wf::TimeSpan ts,
    _In_ WUComp::ICompositionEasingFunction * easingFunction,
    _In_ WUComp::ICompositor * compositor)
{
    int i = (int)animationComponent;
    if (i < 0 || i >= ARRAYSIZE(m_componentAnimation))
    {
        IFC_RETURN(E_INVALIDARG);
    }

    Microsoft::WRL::ComPtr<WUComp::ICompositionAnimation> animation = m_componentAnimation[i];
    if (animation == nullptr)
    {
        // It wasn't really a custom animation at all.  Probably one part of a property (e.g. offset.x) was custom.
        return StartScalarAnimation(compositionObject, propertyName, from, to, ts, easingFunction, compositor);
    }

    IFC_RETURN(animation->SetScalarParameter(wrl_wrappers::HStringReference(L"StartingValue").Get(), from));
    IFC_RETURN(animation->SetScalarParameter(wrl_wrappers::HStringReference(L"FinalValue").Get(), to));
    HRESULT hr = compositionObject->StartAnimation(wrl_wrappers::HStringReference(propertyName).Get(), animation.Get());
    if (FAILED(hr))
    {
        WCHAR errorText[256];
        swprintf_s(errorText, _countof(errorText), L"Cannot start animation - the custom animation for %s:%s failed.", m_key.GetBuffer(), propertyName);
        ::RoOriginateError(hr, wrl_wrappers::HStringReference(errorText).Get());
        return hr;
    }

    return S_OK;
}

_Check_return_ HRESULT CConnectedAnimation::StartScalarAnimation(
    _In_ WUComp::ICompositionObject * compositionObject,
    _In_ LPCWSTR propertyName,
    _In_ float from,
    _In_ float to,
    _In_ wf::TimeSpan ts,
    _In_ WUComp::ICompositionEasingFunction * easingFunction,
    _In_ WUComp::ICompositor * compositor)
{
    // Since we use a simple easing function, we can skip this if the values aren't different
    if (from == to) return S_OK;

    Microsoft::WRL::ComPtr<WUComp::ICompositionAnimation> animation;
    Microsoft::WRL::ComPtr<WUComp::IScalarKeyFrameAnimation> scalarAnimation;
    Microsoft::WRL::ComPtr<WUComp::IKeyFrameAnimation> keyFrameAnimation;

    IFC_RETURN(compositor->CreateScalarKeyFrameAnimation(scalarAnimation.GetAddressOf()));

    IFC_RETURN(scalarAnimation.As(&keyFrameAnimation));
    IFCFAILFAST(keyFrameAnimation->put_Duration(ts));

    scalarAnimation->InsertKeyFrame(0.0f, from);

    IFC_RETURN(scalarAnimation->InsertKeyFrameWithEasingFunction(1.0f, to, easingFunction));

    IFC_RETURN(scalarAnimation.Get()->QueryInterface(IID_PPV_ARGS(&animation)));
    IFC_RETURN(compositionObject->StartAnimation(wrl_wrappers::HStringReference(propertyName).Get(), animation.Get()));
    return S_OK;
}

_Check_return_ HRESULT CConnectedAnimation::StartVector3Animation(
    _In_ WUComp::ICompositionObject * compositionObject,
    _In_ LPCWSTR propertyName,
    _In_ wfn::Vector3 from,
    _In_ wfn::Vector3 to,
    _In_ wf::TimeSpan ts,
    _In_ WUComp::ICompositionEasingFunction * easingFunction,
    _In_ WUComp::ICompositor * compositor)
{
    // Since we use a simple easing function, we can skip this if the values aren't different
    if (from.X == to.X && from.Y == to.Y) return S_OK;

    Microsoft::WRL::ComPtr<WUComp::ICompositionAnimation> animation;
    Microsoft::WRL::ComPtr<WUComp::IVector3KeyFrameAnimation> vector3Animation;
    Microsoft::WRL::ComPtr<WUComp::IKeyFrameAnimation> keyFrameAnimation;

    IFC_RETURN(compositor->CreateVector3KeyFrameAnimation(vector3Animation.GetAddressOf()));

    IFC_RETURN(vector3Animation.As(&keyFrameAnimation));
    IFCFAILFAST(keyFrameAnimation->put_Duration(ts));

    vector3Animation->InsertKeyFrame(0.0f, from);

    IFC_RETURN(vector3Animation->InsertKeyFrameWithEasingFunction(1.0f, to, easingFunction));

    IFC_RETURN(vector3Animation.Get()->QueryInterface(IID_PPV_ARGS(&animation)));
    IFC_RETURN(compositionObject->StartAnimation(wrl_wrappers::HStringReference(propertyName).Get(), animation.Get()));
    return S_OK;
}

_Check_return_ HRESULT CConnectedAnimation::OnAnimationCompleted(
    _In_ IInspectable* sender,
    _In_ WUComp::ICompositionBatchCompletedEventArgs* args)
{
    return TryToCompleteAnimation();
}

_Check_return_ HRESULT CConnectedAnimation::TryToCompleteAnimation()
{
    // It is possible that we are animating (due to an LTE) and that animation is still
    // running.  If that is the case, we have animated the connected animation to the ending
    // point and if we complete the animation the target element will jump to where ever the
    // animation is now.  In this case we want to wait for the other animation to complete
    // before we complete the connected animation.
    bool canComplete = true;

    for (CUIElement* pElement = m_destination.element;
        pElement != nullptr && canComplete;
        pElement = pElement->GetUIElementAdjustedParentInternal(false))
    {
        if (pElement == nullptr)
        {
            break;
        }

        if (pElement->IsHiddenForLayoutTransition())
        {
            auto layoutElements = pElement->GetLayoutTransitionElements();
            XUINT32 size = layoutElements->size();
            // Note: if size != 1, then we're not handling this correctly. The element is showing up in
            // multiple different places, so it's non-trivial to pass through the LTEs.
            if (size == 1)
            {
                CLayoutTransitionElement* pLTE = nullptr;
                IFC_RETURN(layoutElements->get_item(0, pLTE));
                ASSERT(pLTE != NULL);

                if (pLTE->IsIndependentlyAnimating())
                {
                    canComplete = false;
                }
            }
        }
     }

    if (canComplete)
    {
        m_state = ConnectedAnimationState::Complete;
        IFC_RETURN(Reset());

        // Raise the Completed event.
        FireCompletedEvent();
    }
    else
    {
        m_state = ConnectedAnimationState::ReadyToComplete;
        // Schedule a tick to so we can try to complete it on the next frame.
        ITickableFrameScheduler *pFrameScheduler = GetContext()->GetBrowserHost()->GetFrameScheduler();
        if (pFrameScheduler != NULL)
        {
            IFC_RETURN(pFrameScheduler->RequestAdditionalFrame(0, RequestFrameReason::ConnectedAnimation));
        }

    }

    return S_OK;
}

CXamlIslandRoot* CConnectedAnimation::GetXamlIslandRootNoRef()
{
    CXamlIslandRoot* sourceXamlIslandRoot = m_source.element ? VisualTree::GetContentRootForElement(m_source.element.get())->GetXamlIslandRootNoRef() : nullptr;
    CXamlIslandRoot* destinationXamlIslandRoot = m_destination.element ? VisualTree::GetContentRootForElement(m_destination.element.get())->GetXamlIslandRootNoRef() : nullptr;

    if (sourceXamlIslandRoot && destinationXamlIslandRoot && sourceXamlIslandRoot != destinationXamlIslandRoot)
    {
        IFCFAILFAST(DirectUI::ErrorHelper::OriginateErrorUsingResourceID(E_INVALIDARG, ERROR_CONNECTED_ANIMATIONS_BETWEEN_ELEMENTS_IN_DIFFERENT_XAMLROOTS_ARE_NOT_SUPPORTED));
    }

    return sourceXamlIslandRoot ? sourceXamlIslandRoot : destinationXamlIslandRoot;
}

CConnectedAnimationRoot* CConnectedAnimation::GetConnectedAnimationRootNoRef()
{
    VisualTree* sourceVisualTree = m_source.element ? VisualTree::GetForElementNoRef(m_source.element.get()) : nullptr;
    VisualTree* destinationVisualTree = m_destination.element ? VisualTree::GetForElementNoRef(m_destination.element.get()) : nullptr;

    if (sourceVisualTree && destinationVisualTree && sourceVisualTree != destinationVisualTree)
    {
        IFCFAILFAST(DirectUI::ErrorHelper::OriginateErrorUsingResourceID(E_INVALIDARG, ERROR_CONNECTED_ANIMATIONS_BETWEEN_ELEMENTS_IN_DIFFERENT_XAMLROOTS_ARE_NOT_SUPPORTED));
    }

    VisualTree* visualTree = sourceVisualTree ? sourceVisualTree : destinationVisualTree;

    if (visualTree)
    {
        return visualTree->GetConnectedAnimationRoot();
    }
    else
    {
        return GetContext()->GetConnectedAnimationRoot();
    }
}