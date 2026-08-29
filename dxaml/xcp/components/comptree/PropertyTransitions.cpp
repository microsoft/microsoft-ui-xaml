// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "PropertyTransitions.h"
#include "SimpleProperties.h"
#include "DCompTreeHost.h"
#include "SharedTransitionAnimations.h"
#include "XStringBuilder.h"
#include <string>

using namespace xaml;

CScalarTransition::~CScalarTransition()
{
    SimpleProperty::Property::NotifyDestroyed<CScalarTransition>(this);
}

WUComp::ICompositionAnimationBase* CScalarTransition::GetWUCAnimationNoRef(_In_ DCompTreeHost* dcompTreeHost, const wrl::Wrappers::HStringReference& propertyName, float finalValue)
{
    wf::TimeSpan duration = SimpleProperty::Property::id<KnownPropertyIndex::ScalarTransition_Duration>::Get(this);

    return dcompTreeHost->GetSharedTransitionAnimationsNoRef()->GetScalarAnimationNoRef(dcompTreeHost->GetEasingFunctionStatics(), dcompTreeHost->GetCompositor(), propertyName, finalValue, duration);
}

CVector3Transition::~CVector3Transition()
{
    SimpleProperty::Property::NotifyDestroyed<CVector3Transition>(this);
}

WUComp::ICompositionAnimationBase* CVector3Transition::GetWUCAnimationNoRef(
    _In_ DCompTreeHost* dcompTreeHost,
    const wrl::Wrappers::HStringReference& propertyName,
    const wfn::Vector3& finalValue)
{
    const Vector3TransitionComponents components = SimpleProperty::Property::id<KnownPropertyIndex::Vector3Transition_Components>::Get(this);

    if (static_cast<uint8_t>(components) > 0)
    {
        const wf::TimeSpan duration = SimpleProperty::Property::id<KnownPropertyIndex::Vector3Transition_Duration>::Get(this);

        if (components == (Vector3TransitionComponents::Vector3TransitionComponents_X | Vector3TransitionComponents::Vector3TransitionComponents_Y | Vector3TransitionComponents::Vector3TransitionComponents_Z))
        {
            // All subchannels are animated. Use the starting value.

            return dcompTreeHost->GetSharedTransitionAnimationsNoRef()->GetVector3AnimationNoRef(
                dcompTreeHost->GetEasingFunctionStatics(),
                dcompTreeHost->GetCompositor(),
                propertyName,
                finalValue,
                wrl::Wrappers::HStringReference(L"this.StartingValue"),
                duration);
        }
        else
        {
            // Some subchannels aren't animated. Snap them to their new value at the beginning of the animation,
            // so that the animation runs from a constant to the same constant, which has the effect of not being
            // animated.

            XStringBuilder initialFrameBuilder;
            IFCFAILFAST(initialFrameBuilder.Append(XSTRING_PTR_EPHEMERAL(L"Vector3(")));
            if (WI_IsFlagSet(components, Vector3TransitionComponents::Vector3TransitionComponents_X))
            {
                IFCFAILFAST(initialFrameBuilder.Append(XSTRING_PTR_EPHEMERAL(L"this.StartingValue.X")));
            }
            else
            {
                std::wstring valueString = std::to_wstring(finalValue.X);
                IFCFAILFAST(initialFrameBuilder.Append(valueString.data(), static_cast<uint32_t>(valueString.length())));
            }

            if (WI_IsFlagSet(components, Vector3TransitionComponents::Vector3TransitionComponents_Y))
            {
                IFCFAILFAST(initialFrameBuilder.Append(XSTRING_PTR_EPHEMERAL(L",this.StartingValue.Y")));
            }
            else
            {
                IFCFAILFAST(initialFrameBuilder.AppendChar(','));
                std::wstring valueString = std::to_wstring(finalValue.Y);
                IFCFAILFAST(initialFrameBuilder.Append(valueString.data(), static_cast<uint32_t>(valueString.length())));
            }

            if (WI_IsFlagSet(components, Vector3TransitionComponents::Vector3TransitionComponents_Z))
            {
                IFCFAILFAST(initialFrameBuilder.Append(XSTRING_PTR_EPHEMERAL(L",this.StartingValue.Z")));
            }
            else
            {
                IFCFAILFAST(initialFrameBuilder.AppendChar(','));
                std::wstring valueString = std::to_wstring(finalValue.Z);
                IFCFAILFAST(initialFrameBuilder.Append(valueString.data(), static_cast<uint32_t>(valueString.length())));
            }
            IFCFAILFAST(initialFrameBuilder.Append(XSTRING_PTR_EPHEMERAL(L")")));

            return dcompTreeHost->GetSharedTransitionAnimationsNoRef()->GetVector3AnimationNoRef(
                dcompTreeHost->GetEasingFunctionStatics(),
                dcompTreeHost->GetCompositor(),
                propertyName,
                finalValue,
                wrl::Wrappers::HStringReference(initialFrameBuilder.GetBuffer()),
                duration);
        }
    }
    else
    {
        // All channels are disabled. There is no animation.
        return nullptr;
    }
}

CBrushTransition::~CBrushTransition()
{
    SimpleProperty::Property::NotifyDestroyed<CBrushTransition>(this);
}

wf::TimeSpan CBrushTransition::GetDuration() const
{
    return SimpleProperty::Property::id<KnownPropertyIndex::BrushTransition_Duration>::Get(this);
}
