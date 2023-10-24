// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ListViewBaseItemPresenter.g.h"
#include "vsanimation.h"
#include "Storyboard.g.h"
#include "SwipeHintThemeAnimation.g.h"
#include "PointerDownThemeAnimation.g.h"
#include "PointerUpThemeAnimation.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

using namespace Microsoft::WRL;

// Define as 1 (i.e. XCP_TRACE_OUTPUT_MSG) to get ListViewBaseItemPresenter debug outputs, and 0 otherwise
#define LVBIP_DBG 0
//#define LVBIP_DEBUG

#define REORDER_ANIMATION_DURATION_MSEC 240

_Check_return_ HRESULT
ListViewBaseItemPresenter::ProcessAnimationCommands()
{
    HRESULT hr = S_OK;
    ListViewBaseItemAnimationCommand* pCommand = nullptr;
    CListViewBaseItemChrome* pChrome = static_cast<CListViewBaseItemChrome*>(GetHandle());

    IFC(pChrome->GetNextPendingAnimation(&pCommand));
    while (pCommand != nullptr)
    {
        IFC(pCommand->Accept(this));
        IFC(pChrome->GetNextPendingAnimation(&pCommand));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ListViewBaseItemPresenter::VisitAnimationCommand(_In_ ListViewBaseItemAnimationCommand_Pressed* pCommand)
{
#ifdef LVBIP_DEBUG
    if (LVBIP_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_LISTVIEWBASEITEMCHROME) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_LISTVIEWBASEITEMCHROME | LVBIP_DBG) /*traceType*/,
            L"LVBIP[0x%p]: VisitAnimationCommand - entry for ListViewBaseItemAnimationCommand_Pressed, command=0x%p, pressed=%d, isStarting=%d, steadyStateOnly=%d.",
            this,
            pCommand,
            pCommand->m_pressed,
            pCommand->m_isStarting,
            pCommand->m_steadyStateOnly));
    }
#endif // LVBIP_DEBUG
    HRESULT hr = S_OK;
    CListViewBaseItemChrome* pChrome = static_cast<CListViewBaseItemChrome*>(GetHandle());

    if (pCommand->m_isStarting)
    {
        ctl::ComPtr<Storyboard> spStoryboard;

        {
            bool shouldProceed = false;

            IFC(pChrome->LockLayersForAnimation(pCommand, &shouldProceed));

            if (shouldProceed)
            {
                auto pAnimationTarget = pCommand->m_pAnimationTarget.lock();

                if (pAnimationTarget)
                {
                    ctl::ComPtr<DependencyObject> spTarget;
                    ctl::ComPtr<wfc::IVector<xaml_animation::Timeline*>> spTimelineCollection;

                    IFC(ctl::make<Storyboard>(&spStoryboard));
                    IFC(spStoryboard->get_Children(&spTimelineCollection));

                    IFC(DXamlCore::GetCurrent()->GetPeer(pAnimationTarget, &spTarget));

                    if (pCommand->m_pressed)
                    {
                        ctl::ComPtr<PointerDownThemeAnimation> spPointerDownThemeAnimation;

                        IFC(ctl::make<PointerDownThemeAnimation>(&spPointerDownThemeAnimation));
                        spPointerDownThemeAnimation->SetAnimationTarget(spTarget.Get());
                        IFC(spPointerDownThemeAnimation->CreateTimelines(!!pCommand->m_steadyStateOnly, spTimelineCollection.Get()));
                    }
                    else
                    {
                        ctl::ComPtr<PointerUpThemeAnimation> spPointerUpThemeAnimation;

                        IFC(ctl::make<PointerUpThemeAnimation>(&spPointerUpThemeAnimation));
                        spPointerUpThemeAnimation->SetAnimationTarget(spTarget.Get());
                        IFC(spPointerUpThemeAnimation->CreateTimelines(!!pCommand->m_steadyStateOnly, spTimelineCollection.Get()));
                    }

                    IFC(spStoryboard->Begin());
                }
            }
        }

        // Clear the current animation to start the new one. It is important
        // to call Stop on the current animation only after we have called
        // Begin on the new one, otherwise the animated value won't be handed
        // off correctly. For this specific case, consider the
        // PointerDownThemeAnimation handing off the animated value to the
        // PointerUpThemeAnimation.
        if (m_pointerPressedAnimation.tpStoryboard)
        {
            IFC(ClearAnimation(&m_pointerPressedAnimation));
        }

        if (spStoryboard)
        {
            SetPtrValue(m_pointerPressedAnimation.tpStoryboard, spStoryboard);
            m_pointerPressedAnimation.pCommand = pCommand;
            pCommand = nullptr;
        }
    }
    else
    {
        if (pCommand == m_pointerPressedAnimation.pCommand)
        {
            pCommand = nullptr;
        }

        IFC(ClearAnimation(&m_pointerPressedAnimation));
    }

Cleanup:
    pChrome->UnlockLayersForAnimationAndDisposeCommand(pCommand);
    RRETURN(hr);
}

_Check_return_ HRESULT
ListViewBaseItemPresenter::VisitAnimationCommand(_In_ ListViewBaseItemAnimationCommand_ReorderHint* pCommand)
{
    HRESULT hr = S_OK;

    CListViewBaseItemChrome* pChrome = static_cast<CListViewBaseItemChrome*>(GetHandle());
    if (pCommand->m_isStarting)
    {
        if (!m_reorderHintAnimation.tpStoryboard)
        {
            bool shouldProceed = false;

            IFC(pChrome->LockLayersForAnimation(pCommand, &shouldProceed));

            if (shouldProceed)
            {
                auto pAnimationTarget = pCommand->m_pAnimationTarget.lock();

                if (pAnimationTarget)
                {
                    BOOLEAN onlySteadyState = !!pCommand->m_steadyStateOnly;
                    wf::Point destinationOffset = { pCommand->m_offsetX, pCommand->m_offsetY };
                    wf::Point startOffset = {0, 0};
                    wrl_wrappers::HString strTargetName; // No target name, passing target via ref.
                    ctl::ComPtr<wfc::IVector<xaml_animation::Timeline*>> spTimelineCollection;
                    ctl::ComPtr<Storyboard> spStoryboard;
                    ctl::ComPtr<DependencyObject> spTarget;

                    IFC(DXamlCore::GetCurrent()->GetPeer(pAnimationTarget, &spTarget));

                    IFC(ctl::make<Storyboard>(&spStoryboard));
                    IFC(spStoryboard->get_Children(&spTimelineCollection));

                    __if_exists(TAS_DRAGENTER)
                    {
                        IFC(ThemeGenerator::AddTimelinesForThemeAnimation(TAS_DRAGENTER, TA_DRAGENTER_TARGET, strTargetName, spTarget.Get(), onlySteadyState, startOffset, destinationOffset, spTimelineCollection.Get()));
                    }
                    IFC(ThemeGenerator::AddTimelinesForThemeAnimation(TAS_DRAGBETWEENENTER, TA_DRAGBETWEENENTER_AFFECTED, strTargetName, spTarget.Get(), onlySteadyState, startOffset, destinationOffset, spTimelineCollection.Get()));


                    IFC(spStoryboard->Begin());

                    SetPtrValue(m_reorderHintAnimation.tpStoryboard, spStoryboard);
                    m_reorderHintAnimation.pCommand = pCommand;
                    pCommand = nullptr;
                }
            }
        }
    }
    else
    {
        if (m_reorderHintAnimation.tpStoryboard)
        {
            bool shouldProceed = false;

            IFC(pChrome->LockLayersForAnimation(pCommand, &shouldProceed));

            if (shouldProceed)
            {
                auto pAnimationTarget = pCommand->m_pAnimationTarget.lock();

                if (pAnimationTarget)
                {
                    bool onlySteadyState = pCommand->m_steadyStateOnly;

                    if (onlySteadyState)
                    {
                        // Usually we'd create a blank animation even for steady state only, but in this case we want to clear out the
                        // animation immediately as we're about to create a new one in the next command.

                        if (pCommand == m_reorderHintAnimation.pCommand)
                        {
                            pCommand = nullptr;
                        }

                        IFC(ClearAnimation(&m_reorderHintAnimation));
                    }
                    else
                    {
                        wf::Point startOffset = { pCommand->m_offsetX, pCommand->m_offsetY };
                        wf::Point destinationOffset = {0, 0};
                        wrl_wrappers::HString strTargetName; // No target name, passing target via ref.
                        ctl::ComPtr<wfc::IVector<xaml_animation::Timeline*>> spTimelineCollection;
                        ctl::ComPtr<Storyboard> spStoryboard;
                        ctl::ComPtr<DependencyObject> spTarget;

                        IFC(DXamlCore::GetCurrent()->GetPeer(pAnimationTarget, &spTarget));

                        IFC(ctl::make<Storyboard>(&spStoryboard));
                        IFC(spStoryboard->get_Children(&spTimelineCollection));

                        IFC(ThemeGenerator::AddTimelinesForThemeAnimation(TAS_DRAGBETWEENLEAVE, TA_DRAGBETWEENLEAVE_AFFECTED, strTargetName, spTarget.Get(), !!onlySteadyState, startOffset, destinationOffset, spTimelineCollection.Get()));

                        // When this animation completes, we don't need to lock the layers anymore. So, we add a handler
                        // to unlock the layers at the correct time. Not doing so would unnecessarily block other visual
                        // states from working.
                        ctl::ComPtr<wf::IEventHandler<IInspectable*>> spCompletedHandler;
                        spCompletedHandler.Attach(
                            new ClassMemberEventHandler<
                                ListViewBaseItemPresenter,
                                IContentPresenter,
                                wf::IEventHandler<IInspectable*>,
                                IInspectable,
                                IInspectable>(this, &ListViewBaseItemPresenter::OnReorderHintReturnCompleted));
                        IFC(spStoryboard.Cast<Storyboard>()->add_Completed(spCompletedHandler.Get(), &m_reorderHintBackCompletedToken));

                        IFC(spStoryboard->Begin());

                        IFC(ClearAnimation(&m_reorderHintAnimation));
                        SetPtrValue(m_reorderHintAnimation.tpStoryboard, spStoryboard);
                        m_reorderHintAnimation.pCommand = pCommand;
                        pCommand = nullptr;
                    }
                }
            }
        }
        else
        {
            if (pCommand == m_reorderHintAnimation.pCommand)
            {
                pCommand = nullptr;
            }

            IFC(ClearAnimation(&m_reorderHintAnimation));
        }
    }

Cleanup:
    pChrome->UnlockLayersForAnimationAndDisposeCommand(pCommand);
    RRETURN(hr);
}

_Check_return_ HRESULT
ListViewBaseItemPresenter::OnReorderHintReturnCompleted(
            _In_opt_ IInspectable* pUnused1,
            _In_opt_ IInspectable* pUnused2)
{
    HRESULT hr = S_OK;
    if (m_reorderHintAnimation.tpStoryboard)
    {
        IFC(m_reorderHintAnimation.tpStoryboard.Cast<Storyboard>()->remove_Completed(m_reorderHintBackCompletedToken));
    }

    IFC(ClearAnimation(&m_reorderHintAnimation));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ListViewBaseItemPresenter::VisitAnimationCommand(_In_ ListViewBaseItemAnimationCommand_DragDrop* pCommand)
{
    HRESULT hr = S_OK;
    CListViewBaseItemChrome* pChrome = static_cast<CListViewBaseItemChrome*>(GetHandle());

    if (pCommand->m_isStarting)
    {
        if (!m_dragDropAnimation.tpStoryboard)
        {
            bool shouldProceed = false;

            IFC(pChrome->LockLayersForAnimation(pCommand, &shouldProceed));

            if (shouldProceed)
            {
                auto pBaseAnimationTarget = pCommand->m_pBaseAnimationTarget.lock();
                auto pFadeOutAnimationTarget = pCommand->m_pFadeOutAnimationTarget.lock();

                if (pFadeOutAnimationTarget && pBaseAnimationTarget)
                {

                    BOOLEAN onlySteadyState = !!pCommand->m_steadyStateOnly;
                    wf::Point destinationOffset = {0, 0};
                    wf::Point startOffset = {0, 0};
                    wrl_wrappers::HString strTargetName; // No target name, passing target via ref.
                    ctl::ComPtr<wfc::IVector<xaml_animation::Timeline*>> spTimelineCollection;
                    ctl::ComPtr<Storyboard> spStoryboard;

                    ctl::ComPtr<DependencyObject> spBaseTarget;
                    ctl::ComPtr<DependencyObject> spFadeTarget;

                    IFC(DXamlCore::GetCurrent()->GetPeer(pBaseAnimationTarget, &spBaseTarget));
                    IFC(DXamlCore::GetCurrent()->GetPeer(pFadeOutAnimationTarget, &spFadeTarget));

                    IFC(ctl::make<Storyboard>(&spStoryboard));
                    IFC(spStoryboard->get_Children(&spTimelineCollection));

                    switch(pCommand->m_state)
                    {
                        case ListViewBaseItemAnimationCommand_DragDrop::DragDropState_SinglePrimary:
                        {
                            IFC(ThemeGenerator::AddTimelinesForThemeAnimation(
                                TAS_DRAGSOURCESTART,
                                TA_DRAGSOURCESTART_DRAGSOURCE,
                                strTargetName,
                                spBaseTarget.Get(),
                                onlySteadyState,
                                startOffset,
                                destinationOffset,
                                spTimelineCollection.Get()));
                            IFC(ThemeGenerator::AddTimelinesForThemeAnimation(
                                TAS_FADEOUT,
                                TA_FADEOUT_HIDDEN,
                                strTargetName,
                                spFadeTarget.Get(),
                                onlySteadyState,
                                startOffset,
                                destinationOffset,
                                spTimelineCollection.Get()));
                            break;
                        }
                        case ListViewBaseItemAnimationCommand_DragDrop::DragDropState_MultiPrimary:
                        {
                            IFC(ThemeGenerator::AddTimelinesForThemeAnimation(
                                TAS_DRAGSOURCESTART,
                                TA_DRAGSOURCESTART_DRAGSOURCE,
                                strTargetName,
                                spBaseTarget.Get(),
                                onlySteadyState,
                                startOffset,
                                destinationOffset,
                                spTimelineCollection.Get()));
                            IFC(ThemeGenerator::AddTimelinesForThemeAnimation(
                                TAS_FADEOUT,
                                TA_FADEOUT_HIDDEN,
                                strTargetName,
                                spFadeTarget.Get(),
                                onlySteadyState,
                                startOffset,
                                destinationOffset,
                                spTimelineCollection.Get()));
                            break;
                        }
                        case ListViewBaseItemAnimationCommand_DragDrop::DragDropState_ReorderingSinglePrimary:
                        case ListViewBaseItemAnimationCommand_DragDrop::DragDropState_ReorderingMultiPrimary:
                        {
                            // Query Reorder item's opacity from resource dictionary
                            double reorderOpacity = 1;
                            IFC(GetValueFromThemeResources(wrl_wrappers::HStringReference(L"ListViewItemReorderThemeOpacity"), &reorderOpacity));

                            ThemeGeneratorHelper theme(startOffset, destinationOffset, strTargetName, spBaseTarget.Get(), onlySteadyState, spTimelineCollection.Get());
                            IFC(theme.Initialize());
                            TimingFunctionDescription easing = TimingFunctionDescription();
                            IFC(theme.RegisterKeyFrame(theme.GetOpacityPropertyName(), reorderOpacity, 0, static_cast<INT64>(REORDER_ANIMATION_DURATION_MSEC), &easing));
                            IFC(ThemeGenerator::AddTimelinesForThemeAnimation(
                                TAS_FADEOUT,
                                TA_FADEOUT_HIDDEN,
                                strTargetName,
                                spFadeTarget.Get(),
                                onlySteadyState,
                                startOffset,
                                destinationOffset,
                                spTimelineCollection.Get()));
                            break;
                        }
                        case ListViewBaseItemAnimationCommand_DragDrop::DragDropState_ReorderingTarget:
                        {
                            // Query ReorderTarget's opacity and scale from resource dictionary
                            double reorderTargetOpacity = 1;
                            IFC(GetValueFromThemeResources(wrl_wrappers::HStringReference(L"ListViewItemReorderTargetThemeOpacity"), &reorderTargetOpacity));
                            double reorderTargetScale = 1;
                            IFC(GetValueFromThemeResources(wrl_wrappers::HStringReference(L"ListViewItemReorderTargetThemeScale"), &reorderTargetScale));

                            ThemeGeneratorHelper theme(startOffset, destinationOffset, strTargetName, spBaseTarget.Get(), onlySteadyState, spTimelineCollection.Get());
                            IFC(theme.Initialize());
                            TimingFunctionDescription easing = TimingFunctionDescription();
                            IFC(theme.RegisterKeyFrame(theme.GetOpacityPropertyName(), reorderTargetOpacity, 0, static_cast<INT64>(REORDER_ANIMATION_DURATION_MSEC), &easing));
                            IFC(theme.Set2DTransformOriginValues({ 0.5f, 0.5f }));
                            IFC(theme.RegisterKeyFrame(theme.GetScaleXPropertyName(), reorderTargetScale, 0, static_cast<INT64>(REORDER_ANIMATION_DURATION_MSEC), &easing));
                            IFC(theme.RegisterKeyFrame(theme.GetScaleYPropertyName(), reorderTargetScale, 0, static_cast<INT64>(REORDER_ANIMATION_DURATION_MSEC), &easing));
                            break;
                        }
                        case ListViewBaseItemAnimationCommand_DragDrop::DragDropState_ReorderedPlaceholder:
                        {
                            IFC(ThemeGenerator::AddTimelinesForThemeAnimation(
                                TAS_FADEOUT,
                                TA_FADEOUT_HIDDEN,
                                strTargetName,
                                spFadeTarget.Get(),
                                onlySteadyState,
                                startOffset,
                                destinationOffset,
                                spTimelineCollection.Get()));
                            break;
                        }
                        case ListViewBaseItemAnimationCommand_DragDrop::DragDropState_DragOver:
                        {
                            IFC(ThemeGenerator::AddTimelinesForThemeAnimation(
                                TAS_DRAGSOURCESTART,
                                TA_DRAGSOURCESTART_AFFECTED,
                                strTargetName,
                                spBaseTarget.Get(),
                                onlySteadyState,
                                startOffset,
                                destinationOffset,
                                spTimelineCollection.Get()));
                            break;
                        }
                    }

                    IFC(spStoryboard->Begin());

                    SetPtrValue(m_dragDropAnimation.tpStoryboard, spStoryboard);
                    m_dragDropAnimation.pCommand = pCommand;
                    pCommand = nullptr;
                }
            }
        }
    }
    else
    {
        if (pCommand == m_dragDropAnimation.pCommand)
        {
            pCommand = nullptr;
        }

        IFC(ClearAnimation(&m_dragDropAnimation));
    }

Cleanup:
    pChrome->UnlockLayersForAnimationAndDisposeCommand(pCommand);
    RRETURN(hr);
}

_Check_return_ HRESULT
ListViewBaseItemPresenter::VisitAnimationCommand(_In_ ListViewBaseItemAnimationCommand_MultiSelect* pCommand)
{
#ifdef LVBIP_DEBUG
    if (LVBIP_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_LISTVIEWBASEITEMCHROME) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_LISTVIEWBASEITEMCHROME | LVBIP_DBG) /*traceType*/,
            L"LVBIP[0x%p]: VisitAnimationCommand - entry for ListViewBaseItemAnimationCommand_MultiSelect, command=0x%p, isRoundedChromeEnabled=%d, entering=%d, isStarting=%d, steadyStateOnly=%d, inline=%d, checkBoxTranslationX=%lf, contentTranslationX=%lf.",
            this,
            pCommand,
            pCommand->m_isRoundedListViewBaseItemChromeEnabled,
            pCommand->m_entering,
            pCommand->m_isStarting,
            pCommand->m_steadyStateOnly,
            pCommand->m_checkMode == DirectUI::ListViewItemPresenterCheckMode::Inline,
            pCommand->m_checkBoxTranslationX,
            pCommand->m_contentTranslationX));
    }
#endif // LVBIP_DEBUG

    HRESULT hr = S_OK;
    std::unique_ptr<ThemeGeneratorHelper> pSupplier;
    CListViewBaseItemChrome* pChrome = static_cast<CListViewBaseItemChrome*>(GetHandle());

    if (pCommand->m_isStarting)
    {
        // if an animation was already playing, we clear it
        // not clearing will cause the following cases

        // Case 1: Single -> Multiple -> Single
        // Switching to multiple creates the CheckBox and starts the animation from left to right to push the content.
        // Switching to single will not do anything since we will still be animating to multiple.
        // The single animation is responsible for deleting the CheckBox at the end of its animation.
        // Not playing the single animation will leave in Single mode with a CheckBox on top of the content.

        // Case 2: Multiple -> Single -> Single
        // Switching to single animates to the left and delete the CheckBox at the end.
        // Switching for multiple will not do anything since the Single animation is still playing.
        // At the end of the single animation, the CheckBox will be deleted.
        // Current state will be MultiSelect mode without a CheckBox which might cause a crash in certain cases where
        // we make the assumption that a CheckBox has been created.
        if (m_multiSelectAnimation.tpStoryboard)
        {
            IFC_RETURN(m_multiSelectAnimation.tpStoryboard.Cast<Storyboard>()->remove_Completed(m_multiSelectCompletedToken));

            IFC_RETURN(ClearAnimation(&m_multiSelectAnimation));
        }

        // Create the animation and its storyboard
        {
            bool shouldProceed = false;

            IFC(pChrome->LockLayersForAnimation(pCommand, &shouldProceed));

            if (shouldProceed)
            {
                const wf::Point nullPoint = {};
                INT64 animationDuration; // in milliseconds
                ctl::ComPtr<wfc::IVector<xaml_animation::Timeline*>> spTimelineCollection;
                ctl::ComPtr<Storyboard> spStoryboard;
                ctl::ComPtr<DependencyObject> spTarget;
                TimingFunctionDescription easing = TimingFunctionDescription();

                if (pCommand->m_isRoundedListViewBaseItemChromeEnabled)
                {
                    // pCommand->m_contentTranslationX is 28px for None/Multiple transitions, and 19px for Single/Multiple & Extended/Multiple transitions.
                    if (pCommand->m_entering)
                    {
                        // Using 250ms for 28px and 169ms for 19px.
                        animationDuration = static_cast<INT64>(250 * pCommand->m_contentTranslationX / 28);
                        easing.cp2.X = 0.0f;
                        easing.cp3.X = 0.0f;
                    }
                    else
                    {
                        // Using 167ms for 28px and 113ms for 19px.
                        animationDuration = static_cast<INT64>(167 * pCommand->m_contentTranslationX / 28);
                        easing.cp2.X = 1.0f;
                        easing.cp3.X = 1.0f;
                    }
                    easing.cp2.Y = 0.0f;
                }
                else
                {
                    animationDuration = 333;
                    easing.cp2.X = 0.1f;
                    easing.cp2.Y = 0.9f;
                    easing.cp3.X = 0.2f;
                }
                easing.cp3.Y = 1.0f;

                // checkbox animation
                {
                    auto pAnimationTarget = pCommand->m_multiSelectCheckBox.lock();

                    if (pAnimationTarget)
                    {
                        IFC(DXamlCore::GetCurrent()->GetPeer(pAnimationTarget, &spTarget));

                        IFC(ctl::make<Storyboard>(&spStoryboard));

                        IFC(spStoryboard->get_Children(&spTimelineCollection));

                        if (pCommand->m_checkMode == DirectUI::ListViewItemPresenterCheckMode::Inline)
                        {
                            if (!pCommand->m_steadyStateOnly)
                            {
                                const double checkBoxTranslationValue = pCommand->m_entering ? -pCommand->m_checkBoxTranslationX : pCommand->m_checkBoxTranslationX;

                                pSupplier.reset(new ThemeGeneratorHelper(nullPoint, nullPoint, nullptr, spTarget.Get(), FALSE, spTimelineCollection.Get()));
                                IFC(pSupplier->Initialize());

                                if (pCommand->m_entering)
                                {
                                    IFC(pSupplier->RegisterKeyFrame(pSupplier->GetTranslateXPropertyName(), checkBoxTranslationValue, 0, 0, &easing));
                                    IFC(pSupplier->RegisterKeyFrame(pSupplier->GetTranslateXPropertyName(), 0, 0, animationDuration, &easing));

                                    IFC(pSupplier->RegisterKeyFrame(pSupplier->GetClipTranslateXPropertyName(), -checkBoxTranslationValue, 0, 0, &easing));
                                    IFC(pSupplier->RegisterKeyFrame(pSupplier->GetClipTranslateXPropertyName(), 0, 0, animationDuration, &easing));
                                }
                                else
                                {
                                    IFC(pSupplier->RegisterKeyFrame(pSupplier->GetTranslateXPropertyName(), 0, 0, 0, &easing));
                                    IFC(pSupplier->RegisterKeyFrame(pSupplier->GetTranslateXPropertyName(), -checkBoxTranslationValue, 0, animationDuration, &easing));

                                    IFC(pSupplier->RegisterKeyFrame(pSupplier->GetClipTranslateXPropertyName(), 0, 0, 0, &easing));
                                    IFC(pSupplier->RegisterKeyFrame(pSupplier->GetClipTranslateXPropertyName(), checkBoxTranslationValue, 0, animationDuration, &easing));
                                }
                            }
                        }
                        else if (pCommand->m_checkMode == DirectUI::ListViewItemPresenterCheckMode::Overlay)
                        {
                            if (pCommand->m_entering)
                            {
                                // FadeIn
                                IFC(ThemeGenerator::AddTimelinesForThemeAnimation(TAS_FADEIN, TA_FADEIN_SHOWN, nullptr, spTarget.Get(), !!pCommand->m_steadyStateOnly, nullPoint, nullPoint, spTimelineCollection.Get()));
                            }
                            else
                            {
                                // FadeOut
                                IFC(ThemeGenerator::AddTimelinesForThemeAnimation(TAS_FADEOUT, TA_FADEOUT_HIDDEN, nullptr, spTarget.Get(), !!pCommand->m_steadyStateOnly, nullPoint, nullPoint, spTimelineCollection.Get()));
                            }
                        }
                    }
                }

                // content presenter animation
                {
                    auto pAnimationTarget = pCommand->m_contentPresenter.lock();

                    if (pAnimationTarget)
                    {
                        IFC(DXamlCore::GetCurrent()->GetPeer(pAnimationTarget, &spTarget));

                        if (!spStoryboard)
                        {
                            IFC(ctl::make<Storyboard>(&spStoryboard));
                            IFC(spStoryboard->get_Children(&spTimelineCollection));
                        }

                        if (!pCommand->m_steadyStateOnly)
                        {
                            const double contentTranslationValue = pCommand->m_entering ? -pCommand->m_contentTranslationX : pCommand->m_contentTranslationX;

                            pSupplier.reset(new ThemeGeneratorHelper(nullPoint, nullPoint, nullptr, spTarget.Get(), FALSE, spTimelineCollection.Get()));
                            IFC(pSupplier->Initialize());

                            if (pCommand->m_checkMode == DirectUI::ListViewItemPresenterCheckMode::Inline)
                            {
                                IFC(pSupplier->RegisterKeyFrame(pSupplier->GetTranslateXPropertyName(), contentTranslationValue, 0, 0, &easing));
                                IFC(pSupplier->RegisterKeyFrame(pSupplier->GetTranslateXPropertyName(), 0, 0, animationDuration, &easing));
                            }
                        }
                    }
                }

                if (spStoryboard)
                {
                    // When this animation completes, we clear the animation so we can use it again
                    ctl::ComPtr<wf::IEventHandler<IInspectable*>> spCompletedHandler;
                    spCompletedHandler.Attach(
                        new ClassMemberEventHandler<
                        ListViewBaseItemPresenter,
                        IContentPresenter,
                        wf::IEventHandler<IInspectable*>,
                        IInspectable,
                        IInspectable>(this, &ListViewBaseItemPresenter::OnMultiSelectCompleted));

                    IFC(spStoryboard.Cast<Storyboard>()->add_Completed(spCompletedHandler.Get(), &m_multiSelectCompletedToken));

                    IFC(spStoryboard->Begin());

                    SetPtrValue(m_multiSelectAnimation.tpStoryboard, spStoryboard);
                    m_multiSelectAnimation.pCommand = pCommand;
                    pCommand = nullptr;
                }
            }
        }
    }
    else
    {
        if (pCommand == m_multiSelectAnimation.pCommand)
        {
            pCommand = nullptr;
        }

        IFC(ClearAnimation(&m_multiSelectAnimation));
    }

Cleanup:
    pChrome->UnlockLayersForAnimationAndDisposeCommand(pCommand);
    return hr;
}

_Check_return_ HRESULT
ListViewBaseItemPresenter::OnMultiSelectCompleted(
    _In_opt_ IInspectable* pUnused1,
    _In_opt_ IInspectable* pUnused2)
{
#ifdef LVBIP_DEBUG
    if (LVBIP_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_LISTVIEWBASEITEMCHROME) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_LISTVIEWBASEITEMCHROME | LVBIP_DBG) /*traceType*/, L"LVBIP[0x%p]: OnMultiSelectCompleted - entry. command=0x%p",
            this, m_multiSelectAnimation.pCommand));
    }
#endif // LVBIP_DEBUG

    ASSERT(m_multiSelectAnimation.pCommand);

    if (!static_cast<ListViewBaseItemAnimationCommand_MultiSelect*>(m_multiSelectAnimation.pCommand)->m_entering)
    {
        CListViewBaseItemChrome* pChrome = static_cast<CListViewBaseItemChrome*>(GetHandle());

        if (pChrome)
        {
            IFC_RETURN(pChrome->RemoveMultiSelectCheckBox());
        }
    }

    if (m_multiSelectAnimation.tpStoryboard)
    {
        IFC_RETURN(m_multiSelectAnimation.tpStoryboard.Cast<Storyboard>()->remove_Completed(m_multiSelectCompletedToken));
    }

    IFC_RETURN(ClearAnimation(&m_multiSelectAnimation));

    return S_OK;
}

_Check_return_ HRESULT
ListViewBaseItemPresenter::VisitAnimationCommand(_In_ ListViewBaseItemAnimationCommand_IndicatorSelect* pCommand)
{
#ifdef LVBIP_DEBUG
    if (LVBIP_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_LISTVIEWBASEITEMCHROME) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_LISTVIEWBASEITEMCHROME | LVBIP_DBG) /*traceType*/,
            L"LVBIP[0x%p]: VisitAnimationCommand - entry for ListViewBaseItemAnimationCommand_IndicatorSelect, command=0x%p, entering=%d, isStarting=%d, steadyStateOnly=%d, inline=%d, translationX=%lf.",
            this,
            pCommand,
            pCommand->m_entering,
            pCommand->m_isStarting,
            pCommand->m_steadyStateOnly,
            pCommand->m_selectionIndicatorMode == DirectUI::ListViewItemPresenterSelectionIndicatorMode::Inline,
            pCommand->m_translationX));
    }
#endif // LVBIP_DEBUG

    HRESULT hr = S_OK;
    std::unique_ptr<ThemeGeneratorHelper> supplier;
    CListViewBaseItemChrome* chrome = static_cast<CListViewBaseItemChrome*>(GetHandle());

    if (pCommand->m_isStarting)
    {
        // Create the animation and its storyboard
        {
            bool shouldProceed = false;

            IFC(chrome->LockLayersForAnimation(pCommand, &shouldProceed));

            if (shouldProceed)
            {
                const double translationValue = pCommand->m_entering ? -pCommand->m_translationX : pCommand->m_translationX;
                const wf::Point nullPoint = {};
                const INT64 animationDuration = 83; // in milliseconds
                ctl::ComPtr<wf::Collections::IVector<xaml_animation::Timeline*>> timelineCollection;
                ctl::ComPtr<Storyboard> storyboard;
                ctl::ComPtr<DependencyObject> target;
                TimingFunctionDescription easing = TimingFunctionDescription();

                if (pCommand->m_entering)
                {
                    easing.cp2.X = 0.0f;
                    easing.cp3.X = 0.0f;
                }
                else
                {
                    easing.cp2.X = 1.0f;
                    easing.cp3.X = 1.0f;
                }
                easing.cp2.Y = 0.0f;
                easing.cp3.Y = 1.0f;

                // Selection indicator animation
                {
                    auto animationTarget = pCommand->m_selectionIndicator.lock();

                    if (animationTarget)
                    {
                        IFC(DXamlCore::GetCurrent()->GetPeer(animationTarget, &target));

                        IFC(ctl::make<Storyboard>(&storyboard));

                        IFC(storyboard->get_Children(&timelineCollection));

                        if (pCommand->m_selectionIndicatorMode == DirectUI::ListViewItemPresenterSelectionIndicatorMode::Inline)
                        {
                            if (!pCommand->m_steadyStateOnly)
                            {
                                supplier.reset(new ThemeGeneratorHelper(nullPoint, nullPoint, nullptr, target.Get(), FALSE, timelineCollection.Get()));
                                IFC(supplier->Initialize());

                                if (pCommand->m_entering)
                                {
                                    IFC(supplier->RegisterKeyFrame(supplier->GetTranslateXPropertyName(), translationValue, 0, 0, &easing));
                                    IFC(supplier->RegisterKeyFrame(supplier->GetTranslateXPropertyName(), 0, 0, animationDuration, &easing));

                                    IFC(supplier->RegisterKeyFrame(supplier->GetClipTranslateXPropertyName(), -translationValue, 0, 0, &easing));
                                    IFC(supplier->RegisterKeyFrame(supplier->GetClipTranslateXPropertyName(), 0, 0, animationDuration, &easing));
                                }
                                else
                                {
                                    IFC(supplier->RegisterKeyFrame(supplier->GetTranslateXPropertyName(), 0, 0, 0, &easing));
                                    IFC(supplier->RegisterKeyFrame(supplier->GetTranslateXPropertyName(), -translationValue, 0, animationDuration, &easing));

                                    IFC(supplier->RegisterKeyFrame(supplier->GetClipTranslateXPropertyName(), 0, 0, 0, &easing));
                                    IFC(supplier->RegisterKeyFrame(supplier->GetClipTranslateXPropertyName(), translationValue, 0, animationDuration, &easing));
                                }
                            }
                        }
                    }
                }

                // content presenter animation
                {
                    auto animationTarget = pCommand->m_contentPresenter.lock();

                    if (animationTarget)
                    {
                        IFC(DXamlCore::GetCurrent()->GetPeer(animationTarget, &target));

                        if (!storyboard)
                        {
                            IFC(ctl::make<Storyboard>(&storyboard));
                            IFC(storyboard->get_Children(&timelineCollection));
                        }

                        if (!pCommand->m_steadyStateOnly)
                        {
                            supplier.reset(new ThemeGeneratorHelper(nullPoint, nullPoint, nullptr, target.Get(), FALSE, timelineCollection.Get()));
                            IFC(supplier->Initialize());

                            if (pCommand->m_selectionIndicatorMode == DirectUI::ListViewItemPresenterSelectionIndicatorMode::Inline)
                            {
                                IFC(supplier->RegisterKeyFrame(supplier->GetTranslateXPropertyName(), translationValue, 0, 0, &easing));
                                IFC(supplier->RegisterKeyFrame(supplier->GetTranslateXPropertyName(), 0, 0, animationDuration, &easing));
                            }
                        }
                    }
                }

                if (storyboard)
                {
                    // When this animation completes, we clear the animation so we can use it again
                    ctl::ComPtr<wf::IEventHandler<IInspectable*>> completedHandler;
                    completedHandler.Attach(
                        new ClassMemberEventHandler<
                        ListViewBaseItemPresenter,
                        IContentPresenter,
                        wf::IEventHandler<IInspectable*>,
                        IInspectable,
                        IInspectable>(this, &ListViewBaseItemPresenter::OnIndicatorSelectCompleted));

                    IFC(storyboard.Cast<Storyboard>()->add_Completed(completedHandler.Get(), &m_indicatorSelectCompletedToken));

                    IFC(storyboard->Begin());

                    SetPtrValue(m_indicatorSelectAnimation.tpStoryboard, storyboard);
                    m_indicatorSelectAnimation.pCommand = pCommand;
                    pCommand = nullptr;
                }
            }
        }
    }
    else
    {
        if (pCommand == m_indicatorSelectAnimation.pCommand)
        {
            pCommand = nullptr;
        }

        IFC(ClearAnimation(&m_indicatorSelectAnimation));
    }

Cleanup:
    chrome->UnlockLayersForAnimationAndDisposeCommand(pCommand);
    return hr;
}

_Check_return_ HRESULT
ListViewBaseItemPresenter::OnIndicatorSelectCompleted(
    _In_opt_ IInspectable* unused1,
    _In_opt_ IInspectable* unused2)
{
#ifdef LVBIP_DEBUG
    if (LVBIP_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_LISTVIEWBASEITEMCHROME) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_LISTVIEWBASEITEMCHROME | LVBIP_DBG) /*traceType*/, L"LVBIP[0x%p]: OnIndicatorSelectCompleted - entry. command=0x%p",
            this, m_indicatorSelectAnimation.pCommand));
    }
#endif // LVBIP_DEBUG

    ASSERT(m_indicatorSelectAnimation.pCommand);

    if (m_indicatorSelectAnimation.tpStoryboard)
    {
        IFC_RETURN(m_indicatorSelectAnimation.tpStoryboard.Cast<Storyboard>()->remove_Completed(m_indicatorSelectCompletedToken));
    }

    IFC_RETURN(ClearAnimation(&m_indicatorSelectAnimation));

    return S_OK;
}

_Check_return_ HRESULT
ListViewBaseItemPresenter::VisitAnimationCommand(_In_ ListViewBaseItemAnimationCommand_SelectionIndicatorVisibility* pCommand)
{
#ifdef LVBIP_DEBUG
    if (LVBIP_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_LISTVIEWBASEITEMCHROME) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_LISTVIEWBASEITEMCHROME | LVBIP_DBG) /*traceType*/,
            L"LVBIP[0x%p]: VisitAnimationCommand - entry for ListViewBaseItemAnimationCommand_SelectionIndicatorVisibility, command=0x%p, selected=%d, fromScale=%lf, isStarting=%d, steadyStateOnly=%d.",
            this,
            pCommand,
            pCommand->m_selected,
            pCommand->m_fromScale,
            pCommand->m_isStarting,
            pCommand->m_steadyStateOnly));
    }
#endif // LVBIP_DEBUG

    HRESULT hr = S_OK;
    CListViewBaseItemChrome* chrome = static_cast<CListViewBaseItemChrome*>(GetHandle());

    if (pCommand->m_isStarting)
    {
        bool isAnimating = m_selectionIndicatorAnimation.pCommand != nullptr;

        // Create the animation and its storyboard
        bool shouldProceed = false;

        IFC(chrome->LockLayersForAnimation(pCommand, &shouldProceed));

        if (shouldProceed)
        {
            ctl::ComPtr<Storyboard> storyboard;

            // Selection indicator animation
            {
                auto animationTarget = pCommand->m_selectionIndicator.lock();

                if (animationTarget)
                {
                    ctl::ComPtr<DependencyObject> target;

                    IFC(DXamlCore::GetCurrent()->GetPeer(animationTarget, &target));
                    IFC(ctl::make<Storyboard>(&storyboard));

                    ctl::ComPtr<wf::Collections::IVector<xaml_animation::Timeline*>> timelineCollection;

                    IFC(storyboard->get_Children(&timelineCollection));
                        
                    const wf::Point nullPoint = {};
                    std::unique_ptr<ThemeGeneratorHelper> supplier;

                    supplier.reset(new ThemeGeneratorHelper(nullPoint /*startOffset*/, nullPoint /*destinationOffset*/, nullptr /*targetName*/, target.Get(), !!pCommand->m_steadyStateOnly, timelineCollection.Get()));
                    IFC(supplier->Initialize());

                    const INT64 opacityAnimationDuration = 83; // in milliseconds
                    TimingFunctionDescription easingOpacity = TimingFunctionDescription();

                    // Opacity animation to fade in or out
                    if (pCommand->m_selected && !isAnimating)
                    {
                        // Start from 0.0 opacity unless there is already an animation
                        IFC(supplier->RegisterKeyFrame(supplier->GetOpacityPropertyName(), 0.0, 0, 0, &easingOpacity));
                    }

                    IFC(supplier->RegisterKeyFrame(supplier->GetOpacityPropertyName(), pCommand->m_selected ? 1.0 : 0.0, 0, opacityAnimationDuration, &easingOpacity));

                    if (pCommand->m_selected)
                    {
                        const INT64 scaleAnimationDuration = 167; // in milliseconds
                        TimingFunctionDescription easingScale = TimingFunctionDescription();

                        if (pCommand->m_fromScale != 0.0)
                        {
                            easingScale.cp2.X = 0.0f;
                            easingScale.cp2.Y = 0.0f;
                        }
                        else
                        {
                            easingScale.cp2.X = 0.167f;
                            easingScale.cp2.Y = 0.167f;
                        }
                        easingScale.cp3.X = 0.0f;
                        easingScale.cp3.Y = 1.0f;

                        IFC(supplier->Set2DTransformOriginValues({ 0.5f, 0.5f }));

                        if (!isAnimating)
                        {
                            IFC(supplier->RegisterKeyFrame(supplier->GetScaleYPropertyName(), pCommand->m_fromScale, 0, 0, &easingScale));
                        }

                        IFC(supplier->RegisterKeyFrame(supplier->GetScaleYPropertyName(), 1.0, 0, scaleAnimationDuration, &easingScale));
                    }
                }
            }

            if (storyboard)
            {
                if (m_selectionIndicatorAnimation.tpStoryboard)
                {
                    ASSERT(isAnimating);
                    // Unhook completion event for ongoing animation
                    IFC(m_selectionIndicatorAnimation.tpStoryboard.Cast<Storyboard>()->remove_Completed(m_selectionIndicatorCompletedToken));

                    IFC(ClearAnimation(&m_selectionIndicatorAnimation));
                }

                // When this animation completes, we clear the animation so we can use it again
                ctl::ComPtr<wf::IEventHandler<IInspectable*>> completedHandler;
                completedHandler.Attach(
                    new ClassMemberEventHandler<
                    ListViewBaseItemPresenter,
                    IContentPresenter,
                    wf::IEventHandler<IInspectable*>,
                    IInspectable,
                    IInspectable>(this, &ListViewBaseItemPresenter::OnSelectionIndicatorCompleted));

                IFC(storyboard.Cast<Storyboard>()->add_Completed(completedHandler.Get(), &m_selectionIndicatorCompletedToken));

                IFC(storyboard->Begin());

                SetPtrValue(m_selectionIndicatorAnimation.tpStoryboard, storyboard);
                m_selectionIndicatorAnimation.pCommand = pCommand;
                pCommand = nullptr;
            }
        }
    }
    else
    {
        if (pCommand == m_selectionIndicatorAnimation.pCommand)
        {
            pCommand = nullptr;
        }

        IFC(ClearAnimation(&m_selectionIndicatorAnimation));
    }

Cleanup:
    chrome->UnlockLayersForAnimationAndDisposeCommand(pCommand);
    return hr;
}

_Check_return_ HRESULT
ListViewBaseItemPresenter::OnSelectionIndicatorCompleted(
    _In_opt_ IInspectable* unused1,
    _In_opt_ IInspectable* unused2)
{
#ifdef LVBIP_DEBUG
    if (LVBIP_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_LISTVIEWBASEITEMCHROME) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_LISTVIEWBASEITEMCHROME | LVBIP_DBG) /*traceType*/, L"LVBIP[0x%p]: OnSelectionIndicatorCompleted - entry. command=0x%p",
            this, m_selectionIndicatorAnimation.pCommand));
    }
#endif // LVBIP_DEBUG

    ASSERT(m_selectionIndicatorAnimation.pCommand);

    if (!static_cast<ListViewBaseItemAnimationCommand_SelectionIndicatorVisibility*>(m_selectionIndicatorAnimation.pCommand)->m_selected)
    {
        CListViewBaseItemChrome* chrome = static_cast<CListViewBaseItemChrome*>(GetHandle());

        if (chrome)
        {
            IFC_RETURN(chrome->RemoveSelectionIndicator());
        }
    }

    if (m_selectionIndicatorAnimation.tpStoryboard)
    {
        IFC_RETURN(m_selectionIndicatorAnimation.tpStoryboard.Cast<Storyboard>()->remove_Completed(m_selectionIndicatorCompletedToken));
    }

    IFC_RETURN(ClearAnimation(&m_selectionIndicatorAnimation));

    return S_OK;
}

_Check_return_ HRESULT
ListViewBaseItemPresenter::ClearAnimation(_In_ AnimationState* pAnimation)
{
    HRESULT hr = S_OK;

    if (pAnimation->tpStoryboard)
    {
        CListViewBaseItemChrome* pChrome = static_cast<CListViewBaseItemChrome*>(GetHandle());

        pChrome->UnlockLayersForAnimationAndDisposeCommand(pAnimation->pCommand);
        pAnimation->pCommand = nullptr;

        IFC(pAnimation->tpStoryboard->Stop());
        pAnimation->tpStoryboard.Clear();
    }

Cleanup:
    RRETURN(hr);
}

// Gets the value of a resource by querying the ThemeResource dictionary
_Check_return_ HRESULT
ListViewBaseItemPresenter::GetValueFromThemeResources(
    _In_ const wrl_wrappers::HStringReference& resourceKey,
    _Out_ double* pValue)
{
    ctl::ComPtr<xaml::IResourceDictionary> resources;
    IFC_RETURN(get_Resources(&resources));
    ctl::ComPtr<wfc::IMap<IInspectable*, IInspectable*>> resourcesMap;
    IFC_RETURN(resources.As(&resourcesMap));

    ctl::ComPtr<IInspectable> boxedResourceKey;
    IFC_RETURN(PropertyValue::CreateFromString(resourceKey.Get(), &boxedResourceKey));
    BOOLEAN doesResourceExist = FALSE;
    IFC_RETURN(resourcesMap->HasKey(boxedResourceKey.Get(), &doesResourceExist));
    if (doesResourceExist)
    {
        ctl::ComPtr<IInspectable> boxedResource;
        IFC_RETURN(resourcesMap->Lookup(boxedResourceKey.Get(), &boxedResource));
        IFCPTR_RETURN(boxedResource);

        auto doubleReference = ctl::query_interface_cast<wf::IReference<double>>(boxedResource.Get());
        IFC_RETURN(doubleReference->get_Value(pValue));
    }

    return S_OK;
}
