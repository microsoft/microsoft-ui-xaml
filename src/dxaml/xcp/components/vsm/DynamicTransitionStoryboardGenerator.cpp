// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <CControl.h>
#include <CDependencyObject.h>
#include <framework.h>
#include <DOPointerCast.h>
#include <VisualState.h>
#include <Timeline.h>
#include <DynamicTimeline.h>
#include <storyboard.h>
#include <TimelineCollection.h>
#include <Indexes.g.h>
#include <Timeline.h>
#include <duration.h>
#include <animation.h>
#include <TypeTableStructs.h>
#include <VisualTransition.h>
#include <MetadataAPI.h>
#include <corep.h>
#include <weakref_ptr.h>
#include <VisualStateSetterHelper.h>
#include "TimeSpan.h"

#include "TimelineLookupList.h"
#include "DynamicTransitionStoryboardGenerator.h"

namespace {
    xref_ptr<DurationVO::Wrapper> GetDurationFromTransition(_In_ CVisualTransition* transition)
    {
        if (transition->m_duration)
        {
            return transition->m_duration;
        }
        else
        {
            return DurationVOHelper::Create(transition->GetContext());
        }
    }

    bool HasMatchingDynamicTimeline(_In_ const TimelineLookupList& transitionAnimations,
        _In_ const TimelineLookupList::Node& dynamicTimelineNode)
    {
        for (const auto& transitionNode : transitionAnimations)
        {
            if (transitionNode.m_dynamicTimeline &&
                dynamicTimelineNode == transitionNode &&
                dynamicTimelineNode.m_dynamicTimeline->GetTypeIndex() == transitionNode.m_dynamicTimeline->GetTypeIndex())
            {
                return true;
            }
        }
        return false;
    }

    _Check_return_ HRESULT GenerateThemeAnimationTransitions(
        _In_ const TimelineLookupList& newStateAnimations,
        _In_ const TimelineLookupList& transitionAnimations,
        _In_ CStoryboard* transitionStoryboard)
    {
        // The set of newStateAnimations will contain EVERY storyboard child of
        // an expanded dynamic timeline. This means that there will be multiple
        // TimelineLookupList nodes with separate m_timeline pointers, but that all
        // have the same m_dynamicTimeline pointer. This method is purposed with taking
        // all the dynamic timelines in the newStateAnimations list and expanding
        // them into the transition storyboard. Expanding a given dynamic timeline
        // more than once would be to create multiple sets of storyboards that all
        // target the same property, an action that generates a runtime error at the
        // commencement of the animations.
        //
        // To avoid this we need to make sure that every DynamicTimeline is only seen once, we
        // do this by exploiting the fact that DynamicTimeline children are added in sequence,
        // and simply tracking the last seen DynamicTimeline pointer. If we encounter a DynamicTimeline
        // that matches this value, we skip it, as we're in a run of expanded storyboards.
        CDynamicTimeline* lastSeen = nullptr;

        for (const auto& node : newStateAnimations)
        {
            if (node.m_dynamicTimeline &&
                lastSeen != node.m_dynamicTimeline &&
                !HasMatchingDynamicTimeline(transitionAnimations, node))
            {
                lastSeen = node.m_dynamicTimeline;
                xref_ptr<CTimelineCollection> generatedChildren;
                IFC_RETURN(node.m_dynamicTimeline->GenerateChildren(
                    DynamicTimelineGenerationMode::Transition, generatedChildren.ReleaseAndGetAddressOf()));
                while (!generatedChildren->empty())
                {
                    auto childDO = generatedChildren->back();
                    generatedChildren->pop_back();
                    IFC_RETURN(transitionStoryboard->AddChild(static_cast<CTimeline*>(childDO.get())));
                }
            }
        }

        return S_OK;
    }

    _Check_return_ HRESULT SetTargetValues(
        _In_ CFrameworkElement* root,
        _In_ CTimeline* source,
        _In_ CAnimation* destination)
    {
        // Get both the target name and object-based target from
        // the source timeline first.
        CValue targetName;
        xref_ptr<CDependencyObject> target;

        IFC_RETURN(source->GetValueByIndex(KnownPropertyIndex::Storyboard_TargetName, &targetName));
        target = source->GetTargetObject();
        // todo: the above does not take the TargetObjectWeakRef into account, only manual
        // find out if that is for a reason
        // META-TODO: Figure out what the above TODO actually means...

        // If the source doesn't have either a target OR a target name, we query for its timing parent, which might
        // be a dynamic timeline. They do things differently and we look at their target name and target additionally
        // for clues as to what we're suppose to do here.
        if (!target && targetName.IsNull())
        {
            CDynamicTimeline* dynamicTimeline =
                do_pointer_cast<CDynamicTimeline>(source->GetTimingParent());
            if (dynamicTimeline)
            {
                IFC_RETURN(dynamicTimeline->GetValueByIndex(KnownPropertyIndex::Storyboard_TargetName, &targetName));
                target = dynamicTimeline->GetTargetObject();
            }
        }
        else if (!target)
        {
            xstring_ptr strTargetName;
            if (targetName.GetType() == valueString)
            {
                strTargetName = targetName.AsString();
            }
            else if (targetName.GetType() == valueObject)
            {
                CDependencyObject* targetNameAsObject = targetName.AsObject();

                if (targetNameAsObject &&
                    targetNameAsObject->GetTypeIndex() == KnownTypeIndex::String)
                {
                    CValue strFromObject;
                    IFC_RETURN(targetNameAsObject->GetValue(
                        targetNameAsObject->GetContentProperty(), &strFromObject));
                    strTargetName = strFromObject.AsString();
                }
            }

            // This is inconsistent behavior- If the target name is empty
            // we silently continue, otherwise we fail if the element can't be resolved.
            // Ideally we could rely entirely on the namescope component to validate the target
            // name, and always fail on empty strings.
            if (!strTargetName.IsNullOrEmpty())
            {
                xref_ptr<CDependencyObject> targetAsInterface =
                    source->GetContextInterface()->TryGetElementByName(strTargetName, root);
                if (targetAsInterface.get() == nullptr)
                {
                    IFC_RETURN(E_FAIL);
                }
                target = static_sp_cast<CDependencyObject>(std::move(targetAsInterface));
            }
        }

        IFC_RETURN(destination->SetValueByIndex(KnownPropertyIndex::Storyboard_TargetName, targetName));
        IFC_RETURN(destination->SetTargetObject(target.get()));

        // Clone the Storyboard.TargetProperty attached property
        CValue targetProperty;
        IFC_RETURN(source->GetValueByIndex(KnownPropertyIndex::Storyboard_TargetProperty, &targetProperty));
        IFC_RETURN(destination->SetValueByIndex(KnownPropertyIndex::Storyboard_TargetProperty, targetProperty));

        // Clone the target property reference
        destination->SetTargetProperty(source->GetTargetProperty());

        return S_OK;
    }

    _Check_return_ HRESULT GenerateToAnimation(
        _In_ CFrameworkElement* rootForLookup,
        _In_ CTimeline* timeline,
        _Out_ xref_ptr<CAnimation>& ppToAnimation)
    {
        KnownTypeIndex indexToCreate = KnownTypeIndex::UnknownType;
        KnownPropertyIndex propertyToSet = KnownPropertyIndex::UnknownType_UnknownProperty;
        KnownPropertyIndex propertyFromKeyFrame = KnownPropertyIndex::UnknownType_UnknownProperty;
        bool usingKeyFrameValue = false;
        const CDependencyProperty* toProperty = nullptr;
        const CDependencyProperty* fromProperty = nullptr;

        ppToAnimation = nullptr;

        switch (timeline->GetTypeIndex())
        {
        case KnownTypeIndex::DoubleAnimation:
            indexToCreate = KnownTypeIndex::DoubleAnimation;
            propertyToSet = KnownPropertyIndex::DoubleAnimation_To;
            fromProperty = timeline->GetPropertyByIndexInline(KnownPropertyIndex::DoubleAnimation_From);
            toProperty = timeline->GetPropertyByIndexInline(KnownPropertyIndex::DoubleAnimation_To);
            break;

        case KnownTypeIndex::PointAnimation:
            indexToCreate = KnownTypeIndex::PointAnimation;
            propertyToSet = KnownPropertyIndex::PointAnimation_To;
            fromProperty = timeline->GetPropertyByIndexInline(KnownPropertyIndex::PointAnimation_From);
            toProperty = timeline->GetPropertyByIndexInline(KnownPropertyIndex::PointAnimation_To);
            break;

        case KnownTypeIndex::ColorAnimation:
            indexToCreate = KnownTypeIndex::ColorAnimation;
            propertyToSet = KnownPropertyIndex::ColorAnimation_To;
            fromProperty = timeline->GetPropertyByIndexInline(KnownPropertyIndex::ColorAnimation_From);
            toProperty = timeline->GetPropertyByIndexInline(KnownPropertyIndex::ColorAnimation_To);
            break;

        case KnownTypeIndex::DoubleAnimationUsingKeyFrames:
            indexToCreate = KnownTypeIndex::DoubleAnimation;
            propertyToSet = KnownPropertyIndex::DoubleAnimation_To;

            usingKeyFrameValue = true;
            fromProperty = timeline->GetPropertyByIndexInline(KnownPropertyIndex::DoubleAnimationUsingKeyFrames_KeyFrames);
            propertyFromKeyFrame = KnownPropertyIndex::DoubleKeyFrame_Value;
            break;

        case KnownTypeIndex::PointAnimationUsingKeyFrames:
            indexToCreate = KnownTypeIndex::PointAnimation;
            propertyToSet = KnownPropertyIndex::PointAnimation_To;

            usingKeyFrameValue = true;
            fromProperty = timeline->GetPropertyByIndexInline(KnownPropertyIndex::PointAnimationUsingKeyFrames_KeyFrames);
            propertyFromKeyFrame = KnownPropertyIndex::PointKeyFrame_Value;
            break;

        case KnownTypeIndex::ColorAnimationUsingKeyFrames:
            indexToCreate = KnownTypeIndex::ColorAnimation;
            propertyToSet = KnownPropertyIndex::ColorAnimation_To;

            usingKeyFrameValue = true;
            fromProperty = timeline->GetPropertyByIndexInline(KnownPropertyIndex::ColorAnimationUsingKeyFrames_KeyFrames);
            propertyFromKeyFrame = KnownPropertyIndex::ColorKeyFrame_Value;
            break;
        // If this isn't a property we know how to generate a timeline for
        // we return early.
        default:
            return S_OK;
        }

        CValue previousValue;
        if (!usingKeyFrameValue)
        {
            IFC_RETURN(timeline->GetValue(fromProperty, &previousValue));
            if (previousValue.IsNull())
            {
                IFC_RETURN(timeline->GetValue(toProperty, &previousValue));
            }
        }
        else
        {
            CValue keyFramesValue;
            IFC_RETURN(timeline->GetValue(fromProperty, &keyFramesValue));
            xref_ptr<CDOCollection> keyframes(static_cast<CDOCollection*>(keyFramesValue.AsObject()));
            if (!keyframes || keyframes->empty())
            {
                return S_OK;
            }
            IFC_RETURN((*keyframes)[0]->GetValueByIndex(propertyFromKeyFrame, &previousValue));
        }

        // GetValue could have returned without an error, but if there wasn't any value set, simply skip this item
        if (previousValue.GetType() == valueAny || previousValue.IsNull())
        {
            return S_OK;
        }

        // create the new object and set the desired property
        CREATEPARAMETERS cp(timeline->GetContext());
        xref_ptr<CDependencyObject> newTimeline;
        IFC_RETURN(DirectUI::MetadataAPI::GetClassInfoByIndex(indexToCreate)->GetCoreConstructor()(
            newTimeline.ReleaseAndGetAddressOf(), &cp));
        IFC_RETURN(newTimeline->SetValueByIndex(propertyToSet, previousValue));

        IFC_RETURN(SetTargetValues(rootForLookup, timeline, static_cast<CAnimation*>(newTimeline.get())));
        ppToAnimation = static_sp_cast<CAnimation>(std::move(newTimeline));
        return S_OK;
    }

    _Check_return_ HRESULT GenerateFromAnimation(
        _In_ CFrameworkElement* rootForLookup,
        _In_ CTimeline* timeline,
        _Out_ xref_ptr<CAnimation>& ppFromAnimation)
    {
        KnownTypeIndex indexToCreate = KnownTypeIndex::UnknownType;
        ppFromAnimation = nullptr;

        switch (timeline->GetTypeIndex())
        {
        case KnownTypeIndex::DoubleAnimation:
        case KnownTypeIndex::DoubleAnimationUsingKeyFrames:
            indexToCreate = KnownTypeIndex::DoubleAnimation;
            break;

        case KnownTypeIndex::PointAnimation:
        case KnownTypeIndex::PointAnimationUsingKeyFrames:
            indexToCreate = KnownTypeIndex::PointAnimation;
            break;

        case KnownTypeIndex::ColorAnimation:
        case KnownTypeIndex::ColorAnimationUsingKeyFrames:
            indexToCreate = KnownTypeIndex::ColorAnimation;
            break;

        default:
            return S_OK;
        }

        // create the new object and set the desired property
        CREATEPARAMETERS cp(timeline->GetContext());
        xref_ptr<CDependencyObject> newTimeline;
        IFC_RETURN(DirectUI::MetadataAPI::GetClassInfoByIndex(indexToCreate)->GetCoreConstructor()(
            newTimeline.ReleaseAndGetAddressOf(), &cp));
        IFC_RETURN(SetTargetValues(rootForLookup, timeline, static_cast<CAnimation*>(newTimeline.get())));
        ppFromAnimation = static_sp_cast<CAnimation>(std::move(newTimeline));

        return S_OK;
    }

    _Check_return_ HRESULT GenerateToAnimation(
        _In_ CFrameworkElement* rootForLookup,
        _In_ const VisualStateSetterHelper::ResolvedVisualStateSetter* setter,
        _Out_ xref_ptr<CAnimation>& ppToAnimation)
    {
        KnownTypeIndex indexToCreate = KnownTypeIndex::UnknownType;
        KnownPropertyIndex propertyToSet = KnownPropertyIndex::UnknownType_UnknownProperty;

        ppToAnimation = nullptr;
        switch (setter->get_Value().GetType())
        {
        case valueFloat:
        case valueDouble:
            indexToCreate = KnownTypeIndex::DoubleAnimation;
            propertyToSet = KnownPropertyIndex::DoubleAnimation_To;
            break;

        case valuePoint:
            indexToCreate = KnownTypeIndex::PointAnimation;
            propertyToSet = KnownPropertyIndex::PointAnimation_To;
            break;

        case valueColor:
            indexToCreate = KnownTypeIndex::ColorAnimation;
            propertyToSet = KnownPropertyIndex::ColorAnimation_To;
            break;

            // If this isn't a property we know how to generate a timeline for
            // we return early.
        default:
            return S_OK;
        }

        auto targetObject = setter->get_TargetObject().lock();

        // Create the new object and set the desired property.
        CREATEPARAMETERS cp(rootForLookup->GetContext());
        xref_ptr<CDependencyObject> newTimeline;
        IFC_RETURN(DirectUI::MetadataAPI::GetClassInfoByIndex(indexToCreate)->GetCoreConstructor()(
            newTimeline.ReleaseAndGetAddressOf(), &cp));
        IFC_RETURN(newTimeline->SetValueByIndex(propertyToSet, setter->get_Value()));

        auto animation = static_cast<CAnimation*>(newTimeline.get());
        IFC_RETURN(animation->SetTargetObject(targetObject.get()));
        animation->SetTargetProperty(setter->get_TargetProperty());
        ppToAnimation = static_sp_cast<CAnimation>(std::move(newTimeline));
        return S_OK;
    }

    _Check_return_ HRESULT GenerateFromAnimation(
        _In_ CFrameworkElement* rootForLookup,
        _In_ const VisualStateSetterHelper::ResolvedVisualStateSetter* setter,
        _Out_ xref_ptr<CAnimation>& ppFromAnimation)
    {
        KnownTypeIndex indexToCreate = KnownTypeIndex::UnknownType;
        KnownPropertyIndex propertyToSet = KnownPropertyIndex::UnknownType_UnknownProperty;

        ppFromAnimation = nullptr;
        switch (setter->get_Value().GetType())
        {
        case valueFloat:
        case valueDouble:
            indexToCreate = KnownTypeIndex::DoubleAnimation;
            propertyToSet = KnownPropertyIndex::DoubleAnimation_To;
            break;

        case valuePoint:
            indexToCreate = KnownTypeIndex::PointAnimation;
            propertyToSet = KnownPropertyIndex::PointAnimation_To;
            break;

        case valueColor:
            indexToCreate = KnownTypeIndex::ColorAnimation;
            propertyToSet = KnownPropertyIndex::ColorAnimation_To;
            break;

            // If this isn't a property we know how to generate a timeline for
            // we return early.
        default:
            return S_OK;
        }

        auto targetObject = setter->get_TargetObject().lock();

        // Create the new object and set the desired property.
        CREATEPARAMETERS cp(rootForLookup->GetContext());
        xref_ptr<CDependencyObject> newTimeline;
        IFC_RETURN(DirectUI::MetadataAPI::GetClassInfoByIndex(indexToCreate)->GetCoreConstructor()(
            newTimeline.ReleaseAndGetAddressOf(), &cp));

        // If we are here, that means that we are transitioning from a state with a setter,
        // to a state where there is no setter for this target object and property. In other
        // words, we are transitioning to the base value, so we will use that as the To value
        // for the visual transition.
        CValue value;
        IFC_RETURN(targetObject->GetAnimationBaseValue(setter->get_TargetProperty(), &value));
        IFC_RETURN(newTimeline->SetValueByIndex(propertyToSet, value));

        auto animation = static_cast<CAnimation*>(newTimeline.get());
        IFC_RETURN(animation->SetTargetObject(targetObject.get()));
        animation->SetTargetProperty(setter->get_TargetProperty());
        ppFromAnimation = static_sp_cast<CAnimation>(std::move(newTimeline));
        return S_OK;
    }

    // Special case for transform origin of transition target.
    // For independent, FROM animations play them in reverse order, scaled to transition duration.
    // This guarantees that transform origin is held for duration of transition at the set value.
    // The scenarios below illustrate how independent FROM animations of transition target's
    // transform origin will return to original values (x indicates when value is set):
    //   |-- time -->
    //   |x----------| => |----------x|
    //   |----------x| => |x----------|
    //   |-----x-----| => |--x--|       - different, finite duration
    //   |x-----...    => |----------x| - different, infinite duration, begin time = 0
    //   |----x-...    => |----------x| - different, infinite duration, begin time > 0
    // x |-----------| => |----------x| - begin time < 0
    //
    // FUTURE: I have no idea what this actually does or why it does it.
    _Check_return_ HRESULT ApplyTransformOrginWorkaround(CAnimation* newAnimation, const DurationVO::Wrapper* duration, const TimelineLookupList::Node& node)
    {
        xref_ptr<CDependencyObject> resolvedTarget;
        const CDependencyProperty* property = nullptr;
        {
            xref::weakref_ptr<CDependencyObject> weakRef;
            IFC_RETURN(newAnimation->GetAnimationTargets(
                &weakRef, &property));
            resolvedTarget = weakRef.lock();
            ASSERT(resolvedTarget);
        }
        bool isIndependentAnimation = false;
        IFC_RETURN(newAnimation->IsIndependentAnimation(resolvedTarget.get(), &isIndependentAnimation));

        if (property->GetIndex() == KnownPropertyIndex::TransitionTarget_TransformOrigin &&
            (resolvedTarget && resolvedTarget->OfTypeByIndex<KnownTypeIndex::TransitionTarget>()) &&
            !isIndependentAnimation)
        {
            XFLOAT normalizedBeginTime = 1.0f;

            if (node.m_timeline && node.m_timeline->m_pBeginTime && node.m_dynamicTimeline)
            {
                float beginTime = static_cast<float>(node.m_timeline->m_pBeginTime->m_rTimeSpan);
                if (beginTime > 0.0f)
                {
                    // In case if begin time is non-zero, scale it to duration of storyboards (only if their duration is non-infinite).
                    DirectUI::DurationType durationType = DirectUI::DurationType::Automatic;
                    float naturalDuration = 0.0f;
                    node.m_dynamicTimeline->GetNaturalDuration(&durationType, &naturalDuration);

                    if ((durationType == DirectUI::DurationType::TimeSpan || durationType == DirectUI::DurationType::Automatic)
                        && naturalDuration > 0.0f)
                    {
                        normalizedBeginTime = 1.0f - (beginTime / naturalDuration);
                    }
                }
            }

            // Assume set transition transform origin timeline has zero-length duration and do not account for its length.
            // Also, clamp the begin time not to be negative nor extend past the end of animation.
            normalizedBeginTime = std::min(std::max(0.0f, normalizedBeginTime), 1.0f);

            CValue valueTemp;
            valueTemp.SetFloat(static_cast<XFLOAT>(duration->Value().GetTimeSpanInSec() * normalizedBeginTime));

            CREATEPARAMETERS cpBeginTime(newAnimation->GetContext(), valueTemp);
            xref_ptr<CDependencyObject> beginTime;
            IFC_RETURN(CTimeSpan::Create(beginTime.ReleaseAndGetAddressOf(), &cpBeginTime));
            valueTemp.WrapObjectNoRef(beginTime.get());
            IFC_RETURN(newAnimation->SetValueByIndex(KnownPropertyIndex::Timeline_BeginTime, valueTemp));
        }

        return S_OK;
    }

    _Check_return_ HRESULT SetEasingFunction(_In_ CTimeline* timeline, _In_ CDependencyObject* easingFunction)
    {
        const CDependencyProperty * pEasingFunctionProperty = NULL;
        CValue val;

        switch (timeline->GetTypeIndex())
        {
        case KnownTypeIndex::DoubleAnimation:
            pEasingFunctionProperty = timeline->GetPropertyByIndexInline(KnownPropertyIndex::DoubleAnimation_EasingFunction);
            break;

        case KnownTypeIndex::PointAnimation:
            pEasingFunctionProperty = timeline->GetPropertyByIndexInline(KnownPropertyIndex::PointAnimation_EasingFunction);
            break;

        case KnownTypeIndex::ColorAnimation:
            pEasingFunctionProperty = timeline->GetPropertyByIndexInline(KnownPropertyIndex::ColorAnimation_EasingFunction);
            break;
        default:
            ASSERT(false);
        }
        val.WrapObjectNoRef(easingFunction);
        IFC_RETURN(timeline->SetValue(SetValueParams(pEasingFunctionProperty, val)));
        return S_OK;
    }
}

namespace DynamicTransitionStoryboardGenerator {
    _Check_return_ HRESULT
    GenerateDynamicTransitionAnimations(
        _In_ CFrameworkElement* targetElement,
        _In_ CVisualTransition* transition,
        _In_ const std::vector<CStoryboard*>& activeStoryboards,
        _In_opt_ CStoryboard* destinationStoryboard,
        _Outptr_ CStoryboard** dynamicStoryboard)
    {
            IFC_RETURN(GenerateDynamicTransitionAnimations(
                targetElement,
                transition,
                activeStoryboards,
                destinationStoryboard,
                VisualStateSetterHelper::ResolvedVisualStateSetterCollection(),
                VisualStateSetterHelper::ResolvedVisualStateSetterCollection(),
                dynamicStoryboard));
        return S_OK;
    }

    _Check_return_ HRESULT GenerateDynamicTransitionAnimations(
        _In_ CFrameworkElement* targetElement,
        _In_ CVisualTransition* transition,
        _In_ const std::vector<CStoryboard*>& activeStoryboards,
        _In_opt_ CStoryboard* destinationStoryboard,
        _In_opt_ const VisualStateSetterHelper::ResolvedVisualStateSetterCollection& activeSetters,
        _In_opt_ const VisualStateSetterHelper::ResolvedVisualStateSetterCollection& destinationSetters,
        _Outptr_ CStoryboard** dynamicStoryboard)
    {
        TimelineLookupList currentAnimations;
        IFC_RETURN(currentAnimations.Initialize(activeStoryboards));
        TimelineLookupList currentSetters(activeSetters);
        TimelineLookupList transitionAnimations;
        IFC_RETURN(transitionAnimations.Initialize(transition->m_pStoryboard));
        TimelineLookupList newStateAnimations;
        IFC_RETURN(newStateAnimations.Initialize(destinationStoryboard));
        TimelineLookupList newStateSetters(destinationSetters);
        auto duration = GetDurationFromTransition(transition);

        *dynamicStoryboard = nullptr;

        xref_ptr<CStoryboard> storyboard;
        {
            xref_ptr<CDependencyObject> storyboardAsDO;
            CREATEPARAMETERS cp(targetElement->GetContext());
            IFC_RETURN(CStoryboard::Create(storyboardAsDO.ReleaseAndGetAddressOf(), &cp));
            storyboard = static_sp_cast<CStoryboard>(std::move(storyboardAsDO));
        }

        IFC_RETURN(GenerateThemeAnimationTransitions(newStateAnimations, transitionAnimations, storyboard.get()));

        // We need to respect VisualTransition.GeneratedDuration's constraint, which is
        // that the total duration is the longer of either GeneratedDuration, or the child storybards
        CValue durationCValue;
        durationCValue.Wrap<valueVO>(duration);
        IFC_RETURN(storyboard->SetValueByIndex(KnownPropertyIndex::Timeline_Duration, durationCValue));

        for (const auto& node : currentSetters)
        {
            currentAnimations.RemoveMatchingNodes(node);
        }

        for (const auto& node : transitionAnimations)
        {
            currentAnimations.RemoveMatchingNodes(node);
            currentSetters.RemoveMatchingNodes(node);
            newStateAnimations.RemoveMatchingNodes(node);
            newStateSetters.RemoveMatchingNodes(node);
        }

        for (const auto& node : newStateAnimations)
        {
            currentAnimations.RemoveMatchingNodes(node);
            currentSetters.RemoveMatchingNodes(node);
        }

        for (const auto& node : newStateSetters)
        {
            currentAnimations.RemoveMatchingNodes(node);
            currentSetters.RemoveMatchingNodes(node);
        }

        auto timelineGenerationFunc = [&](const TimelineLookupList::Node& node, bool processingCurrentAnimations) {
            if (!node.m_dynamicTimeline)
            {
                xref_ptr<CAnimation> newTimeline;
                if (node.m_setter)
                {
                    if (processingCurrentAnimations)
                    {
                        IFC_RETURN(GenerateFromAnimation(targetElement, node.m_setter, newTimeline));
                    }
                    else
                    {
                        IFC_RETURN(GenerateToAnimation(targetElement, node.m_setter, newTimeline));
                    }
                }
                else
                {
                    if (processingCurrentAnimations)
                    {
                        IFC_RETURN(GenerateFromAnimation(targetElement, node.m_timeline, newTimeline));
                    }
                    else
                    {
                        IFC_RETURN(GenerateToAnimation(targetElement, node.m_timeline, newTimeline));
                    }
                }

                if (newTimeline)
                {
                    xref_ptr<CDependencyObject> resolvedTarget;
                    const CDependencyProperty* property = nullptr;
                    {
                        xref::weakref_ptr<CDependencyObject> weakRef;
                        IFC_RETURN(newTimeline->GetAnimationTargets(
                            &weakRef, &property));
                        resolvedTarget = weakRef.lock();
                        ASSERT(resolvedTarget);
                    }

                    bool shouldCreateWithFullDuration = false;
                    IFC_RETURN(newTimeline->IsIndependentAnimation(resolvedTarget.get(), &shouldCreateWithFullDuration));

                    if (processingCurrentAnimations)
                    {
                        IFC_RETURN(ApplyTransformOrginWorkaround(newTimeline.get(), duration, node));
                    }

                    xref_ptr<DurationVO::Wrapper> clonedDuration = shouldCreateWithFullDuration ? duration : DurationVOHelper::CreateTimeSpan(newTimeline->GetContext(), 0.0);
                    CValue durationCValue;
                    durationCValue.Wrap<valueVO>(clonedDuration);
                    IFC_RETURN(newTimeline->SetValueByIndex(KnownPropertyIndex::Timeline_Duration, durationCValue));

                    if (transition && transition->m_pEasingFunction && shouldCreateWithFullDuration)
                    {
                        IFC_RETURN(SetEasingFunction(newTimeline.get(), transition->m_pEasingFunction));
                    }

                    IFC_RETURN(storyboard->AddChild(newTimeline.get()));
                }
            }
            return S_OK;
        };

        for (const auto& node : newStateAnimations)
        {
            IFC_RETURN(timelineGenerationFunc(node, false));
        }

        for (const auto& node : newStateSetters)
        {
            IFC_RETURN(timelineGenerationFunc(node, false));
        }

        for (const auto& node : currentAnimations)
        {
            IFC_RETURN(timelineGenerationFunc(node, true));
        }

        for (const auto& node : currentSetters)
        {
            IFC_RETURN(timelineGenerationFunc(node, true));
        }

        *dynamicStoryboard = storyboard.detach();
        return S_OK;
    }
}
