// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "LinedFlowLayoutItemCollectionTransitionProvider.h"
#include "LinedFlowLayoutItemCollectionTransitionProvider.properties.cpp"

#pragma region IItemCollectionTransitionProviderOverrides

void LinedFlowLayoutItemCollectionTransitionProvider::StartTransitions(
    winrt::IVector<winrt::ItemCollectionTransition> const& transitions)
{
    std::vector<winrt::ItemCollectionTransition> addTransitions;
    std::vector<winrt::ItemCollectionTransition> removeTransitions;
    std::vector<winrt::ItemCollectionTransition> moveTransitions;

    for (auto const& transition : transitions)
    {
        if (transition.Operation() == winrt::ItemCollectionTransitionOperation::Add)
        {
            addTransitions.push_back(transition);
        }
        else if (transition.Operation() == winrt::ItemCollectionTransitionOperation::Remove)
        {
            removeTransitions.push_back(transition);
        }
        else
        {
            MUX_ASSERT(transition.Operation() == winrt::ItemCollectionTransitionOperation::Move);
            moveTransitions.push_back(transition);
        }
    }

    StartAddTransitions(addTransitions, removeTransitions.size() > 0, moveTransitions.size() > 0);
    StartRemoveTransitions(removeTransitions, addTransitions.size() > 0);
    StartMoveTransitions(moveTransitions, removeTransitions.size() > 0);
}

bool LinedFlowLayoutItemCollectionTransitionProvider::ShouldAnimateCore(winrt::ItemCollectionTransition const& transition)
{
    return true;
}

#pragma endregion

void LinedFlowLayoutItemCollectionTransitionProvider::StartAddTransitions(
    std::vector<winrt::ItemCollectionTransition> const& transitions,
    bool hasRemoveTransitions,
    bool hasMoveTransitions)
{
    for (auto const& transition : transitions)
    {
        auto progress = transition.Start();
        auto visual = winrt::ElementCompositionPreview::GetElementVisual(progress.Element());
        auto compositor = visual.Compositor();
        auto animationGroup = compositor.CreateAnimationGroup();

        // There are two options for what a add animation can mean.
        // It can either mean that we're adding all elements for the first time,
        // or it can mean that we're newly adding a new element being added to the collection.
        // We'll interpret a add animation outside of the context of being added to a collection
        // as the former option, and an animation within the context of an add as the latter option.
        if (!WI_IsFlagSet(transition.Triggers(), winrt::ItemCollectionTransitionTriggers::CollectionChangeAdd))
        {
            visual.Scale(ScaleVisible);

            SlideIn(animationGroup, compositor, 0ms);
        }
        else
        {
            // Animations proceed in the order of delete, then move, then add.
            // Since all animations are begun at the same time, we need to delay Add
            // animations until the former two animations will have completed.
            auto delay = 0ms;

            if (hasMoveTransitions)
            {
                delay += DefaultAnimationDuration * 2;
            }

            if (hasRemoveTransitions)
            {
                delay += QuickAnimationDuration;
            }

            visual.CenterPoint(GetCenterPoint(progress.Element()));
            visual.Opacity(1.0f);
            visual.TransformMatrix(winrt::float4x4::identity());

            ScaleIn(animationGroup, compositor, delay, true /* presetInitialValue */);
        }

        auto batch = compositor.CreateScopedBatch(winrt::CompositionBatchTypes::Animation);
        StartAnimationGroup(visual, animationGroup);
        batch.End();
        batch.Completed([strongThis = get_strong(), visual, progress](auto const&, auto const&)
            {
                if (strongThis->m_currentAnimationGroupByVisual.HasKey(visual))
                {
                    strongThis->m_currentAnimationGroupByVisual.Remove(visual);
                }

                progress.Complete();
            });
    }
}

void LinedFlowLayoutItemCollectionTransitionProvider::StartRemoveTransitions(
    std::vector<winrt::ItemCollectionTransition> const& transitions,
    bool hasAddAnimations)
{
    for (auto const& transition : transitions)
    {
        // If the collection has been reset and we're going to play add animations, then we don't want to play delete animations -
        // we'll just immediately delete the elements and then animate them in during the add animations.
        if (WI_IsFlagSet(transition.Triggers(), winrt::ItemCollectionTransitionTriggers::CollectionChangeReset) &&
            hasAddAnimations)
        {
            continue;
        }

        auto progress = transition.Start();
        auto visual = winrt::ElementCompositionPreview::GetElementVisual(progress.Element());
        auto compositor = visual.Compositor();
        auto animationGroup = compositor.CreateAnimationGroup();

        visual.CenterPoint(GetCenterPoint(progress.Element()));
        visual.Opacity(1.0f);
        visual.TransformMatrix(winrt::float4x4::identity());

        ScaleOut(animationGroup, compositor, 0ms);

        auto batch = compositor.CreateScopedBatch(winrt::CompositionBatchTypes::Animation);
        StartAnimationGroup(visual, animationGroup);
        batch.End();

        batch.Completed([strongThis = get_strong(), visual, progress](auto const&, auto const&)
            {
                if (strongThis->m_currentAnimationGroupByVisual.HasKey(visual))
                {
                    strongThis->m_currentAnimationGroupByVisual.Remove(visual);
                }

                progress.Complete();
            });
    }
}

void LinedFlowLayoutItemCollectionTransitionProvider::StartMoveTransitions(
    std::vector<winrt::ItemCollectionTransition> const& transitions,
    bool hasRemoveTransitions)
{
    for (auto const& transition : transitions)
    {
        // Moved can occur in the context of initial add animations with a trigger of LayoutTransition
        // as elements shift to their initial positions, and in that case we don't want to animate the transition.
        if (WI_IsFlagSet(transition.Triggers(), winrt::ItemCollectionTransitionTriggers::LayoutTransition))
        {
            continue;
        }

        auto progress = transition.Start();
        auto oldBounds = transition.OldBounds();
        auto newBounds = transition.NewBounds();

        auto visual = winrt::ElementCompositionPreview::GetElementVisual(progress.Element());
        auto compositor = visual.Compositor();
        auto animationGroup = compositor.CreateAnimationGroup();
        std::function finishedAction = [progress]()
        {
            progress.Complete();
        };

        auto waitBeforeAnimations = hasRemoveTransitions ? QuickAnimationDuration : 0ms;

        // If we're still in the same line, then we'll translate from the old position to the new position.
        // Otherwise, we'll scale out and then scale in for the transition.
        if (std::abs(oldBounds.Y - newBounds.Y) < Epsilon)
        {
            visual.Opacity(1.0f);
            visual.Scale(ScaleVisible);
            Translate(animationGroup, compositor, oldBounds, newBounds, waitBeforeAnimations + QuickAnimationDuration / 2, true /* presetInitialValue */);
        }
        else
        {
            // The animation when changing lines is more complicated.  We want to scale out the visual from
            // its original position, and then scale it back in in its new position.  However, under the covers,
            // the visual has already snapped to its new position, so we need to set its position back to the
            // original position, use its center point in that location as the scaling center point,
            // then set its position to the new position and use its new center point at that location as the
            // scaling center point when scaling it back in.
            winrt::float3 scaleInCenterPoint{ GetCenterPoint(progress.Element()) };
            winrt::float3 scaleOutCenterPoint(
                scaleInCenterPoint.x + (float)(oldBounds.X - newBounds.X),
                scaleInCenterPoint.y + (float)(oldBounds.Y - newBounds.Y),
                scaleInCenterPoint.z);

            winrt::float3 startBounds(
                (float)(oldBounds.X - newBounds.X),
                (float)(oldBounds.Y - newBounds.Y),
                0.0f);

            auto originalTransformMatrix = visual.TransformMatrix();

            visual.CenterPoint(scaleOutCenterPoint);
            visual.Opacity(1.0f);
            visual.TransformMatrix(originalTransformMatrix * winrt::make_float4x4_translation(startBounds));

            ScaleOut(animationGroup, compositor, waitBeforeAnimations + QuickAnimationDuration / 2, true /* presetInitialValue */);

            // We'll wait until the scale-out animation has completed before we begin the scale-in animation;
            // otherwise, the center point can sometimes be very briefly set to the wrong value and cause a stutter effect.
            finishedAction = [this, visual, compositor, scaleInCenterPoint, animationGroup, progress, originalTransformMatrix]()
            {
                visual.CenterPoint(scaleInCenterPoint);
                visual.TransformMatrix(originalTransformMatrix);

                ScaleIn(animationGroup, compositor, 0ms);

                auto batch = compositor.CreateScopedBatch(winrt::CompositionBatchTypes::Animation);
                StartAnimationGroup(visual, animationGroup);
                batch.End();
                batch.Completed([strongThis = get_strong(), visual, progress](auto const&, auto const&)
                    {
                        if (strongThis->m_currentAnimationGroupByVisual.HasKey(visual))
                        {
                            strongThis->m_currentAnimationGroupByVisual.Remove(visual);
                        }

                        progress.Complete();
                    });
            };
        }

        auto batch = compositor.CreateScopedBatch(winrt::CompositionBatchTypes::Animation);
        StartAnimationGroup(visual, animationGroup);
        batch.End();
        batch.Completed([strongThis = get_strong(), visual, finishedAction](auto const&, auto const&)
            {
                if (strongThis->m_currentAnimationGroupByVisual.HasKey(visual))
                {
                    strongThis->m_currentAnimationGroupByVisual.Remove(visual);
                }

                finishedAction();
            });
    }
}

// The slide-in animation is used for the initial load of the content in an ItemsView.
// It has two parts: the elements animate their opacity from 0 to 1, and animate their
// position from (0, +100) to (0, 0), relative to their visual's position.
// This creates the effect of them appearing from nothing while sliding into position.
void LinedFlowLayoutItemCollectionTransitionProvider::SlideIn(winrt::CompositionAnimationGroup const& group, winrt::Compositor const& compositor, std::chrono::milliseconds const& delay)
{
    auto opacityAnimation = compositor.CreateScalarKeyFrameAnimation();
    opacityAnimation.InsertKeyFrame(0.0f, 0.0f);
    opacityAnimation.InsertKeyFrame(1.0f, 1.0f);
    opacityAnimation.Duration(DefaultAnimationDuration);
    opacityAnimation.Target(L"Opacity");

    if (delay > 0ms)
    {
        opacityAnimation.DelayTime(delay);
        opacityAnimation.DelayBehavior(winrt::AnimationDelayBehavior::SetInitialValueBeforeDelay);
    }

    auto translationAnimation = compositor.CreateVector2KeyFrameAnimation();
    translationAnimation.SetVector2Parameter(L"start", winrt::float2(0, SlideInDistance));
    translationAnimation.SetVector2Parameter(L"end", winrt::float2());
    translationAnimation.InsertExpressionKeyFrame(0.0f, L"start");
    translationAnimation.InsertExpressionKeyFrame(1.0f, L"end", GetEaseInFunction(compositor));
    translationAnimation.Duration(DefaultAnimationDuration);
    translationAnimation.Target(L"TransformMatrix._41_42");

    if (delay > 0ms)
    {
        translationAnimation.DelayTime(delay);
        translationAnimation.DelayBehavior(winrt::AnimationDelayBehavior::SetInitialValueBeforeDelay);
    }

    group.Add(opacityAnimation);
    group.Add(translationAnimation);
}

// The scale-in animation is used both when a new element is added to the collection
// and when an element changes which line it's located in.
// It animates the visual's scale from 0 to 1, creating the effect of it coming into
// view from an initial pinpoint position in the center of its position.
void LinedFlowLayoutItemCollectionTransitionProvider::ScaleIn(winrt::CompositionAnimationGroup const& group, winrt::Compositor const& compositor, std::chrono::milliseconds const& delay, bool presetInitialValue)
{
    auto animation = compositor.CreateVector3KeyFrameAnimation();
    animation.InsertKeyFrame(0.0f, ScaleCollapsed);
    animation.InsertKeyFrame(1.0f, ScaleVisible, GetEaseInFunction(compositor));
    animation.Duration(QuickAnimationDuration);
    animation.Target(L"Scale");

    if (delay > 0ms)
    {
        animation.DelayTime(delay);
        animation.DelayBehavior(presetInitialValue ?
            winrt::AnimationDelayBehavior::SetInitialValueBeforeDelay :
            winrt::AnimationDelayBehavior::SetInitialValueAfterDelay);
    }

    group.Add(animation);
}

// The scale-out animation is used both when an element is removed from the collection
// and when an element changes which line it's located in.
// It animates the visual's scale from 1 to 0, creating the effect of it scaling out of
// view to a pinpoint located in what was the center of its position.
void LinedFlowLayoutItemCollectionTransitionProvider::ScaleOut(winrt::CompositionAnimationGroup const& group, winrt::Compositor const& compositor, std::chrono::milliseconds const& delay, bool presetInitialValue)
{
    auto animation = compositor.CreateVector3KeyFrameAnimation();
    animation.InsertKeyFrame(0.0f, ScaleVisible);
    animation.InsertKeyFrame(1.0f, ScaleCollapsed, GetEaseOutFunction(compositor));
    animation.Duration(QuickAnimationDuration);
    animation.Target(L"Scale");

    if (delay > 0ms)
    {
        animation.DelayTime(delay);
        animation.DelayBehavior(presetInitialValue ?
            winrt::AnimationDelayBehavior::SetInitialValueBeforeDelay :
            winrt::AnimationDelayBehavior::SetInitialValueAfterDelay);
    }

    group.Add(animation);
}

// The translate animation is used when an element is moving to a new position within the same line.
// It slides the visual from its old location to its new location.
void LinedFlowLayoutItemCollectionTransitionProvider::Translate(winrt::CompositionAnimationGroup const& group, winrt::Compositor const& compositor, winrt::Rect const& from, winrt::Rect const& to, std::chrono::milliseconds const& delay, bool presetInitialValue)
{
    auto positionAnimation = compositor.CreateVector2KeyFrameAnimation();
    positionAnimation.SetVector2Parameter(L"start", winrt::float2((float)(from.X - to.X), (float)(from.Y - to.Y)));
    positionAnimation.SetVector2Parameter(L"end", winrt::float2());
    positionAnimation.InsertExpressionKeyFrame(0.0f, L"start");
    positionAnimation.InsertExpressionKeyFrame(1.0f, L"end", GetEaseInFunction(compositor));
    positionAnimation.Duration(DefaultAnimationDuration);
    positionAnimation.Target(L"TransformMatrix._41_42");

    if (delay > 0ms)
    {
        positionAnimation.DelayTime(delay);
        positionAnimation.DelayBehavior(presetInitialValue ?
            winrt::AnimationDelayBehavior::SetInitialValueBeforeDelay :
            winrt::AnimationDelayBehavior::SetInitialValueAfterDelay);
    }

    group.Add(positionAnimation);
}

// CenterPoint values are in pixels, and since we want scale transforms to scale around the
// center of the visual, we'll set this CenterPoint value accordingly.
winrt::float3 LinedFlowLayoutItemCollectionTransitionProvider::GetCenterPoint(winrt::UIElement const& element)
{
    return winrt::float3(element.ActualSize().x / 2, element.ActualSize().y / 2, 0.0f);
}

winrt::CubicBezierEasingFunction LinedFlowLayoutItemCollectionTransitionProvider::GetEaseInFunction(winrt::Compositor const& compositor)
{
    if (m_easeInFunction == nullptr)
    {
        m_easeInFunction = compositor.CreateCubicBezierEasingFunction(winrt::float2(0.55f, 0), winrt::float2(0, 1));
    }

    return m_easeInFunction;
}

winrt::CubicBezierEasingFunction LinedFlowLayoutItemCollectionTransitionProvider::GetEaseOutFunction(winrt::Compositor const& compositor)
{
    if (m_easeOutFunction == nullptr)
    {
        m_easeOutFunction = compositor.CreateCubicBezierEasingFunction(winrt::float2(1, 0), winrt::float2(1, 1));
    }

    return m_easeOutFunction;
}

// When we start an animation on a visual, we'll end the existing animation on that visual
// to ensure that each visual has only one ongoing animation at a time.
void LinedFlowLayoutItemCollectionTransitionProvider::StartAnimationGroup(winrt::Visual const& visual, winrt::CompositionAnimationGroup const& animationGroup)
{
    if (m_currentAnimationGroupByVisual.HasKey(visual))
    {
        visual.StopAnimationGroup(m_currentAnimationGroupByVisual.Lookup(visual));
        m_currentAnimationGroupByVisual.Remove(visual);
    }

    m_currentAnimationGroupByVisual.Insert(visual, animationGroup);
    visual.StartAnimationGroup(animationGroup);
}
