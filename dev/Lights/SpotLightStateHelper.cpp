// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "SpotLightStateHelper.h"
#include "AnimationUtility.h"

bool IsStateAnimated(const RevealHoverSpotlightStateDesc& state)
{
    bool isColorOrIntensityAnimated = false;
    if (SharedHelpers::IsRS3OrHigher())
    {
        isColorOrIntensityAnimated = (!IsIgnored(state.InnerConeIntensity) && IsAnimated(state.InnerConeIntensity)) ||
            (!IsIgnored(state.OuterConeIntensity) && IsAnimated(state.OuterConeIntensity));
    }
    else
    {
        isColorOrIntensityAnimated = (!IsIgnored(state.InnerConeColor) && IsAnimated(state.InnerConeColor)) ||
            (!IsIgnored(state.OuterConeColor) && IsAnimated(state.OuterConeColor));
    }

    const bool isOuterAngleScaleAnimated = !IsIgnored(state.OuterAngleScale) && IsAnimated(state.OuterAngleScale);

    return isColorOrIntensityAnimated || isOuterAngleScaleAnimated;
}

bool IsStateIgnored(const RevealHoverSpotlightStateDesc& state)
{
    bool isColorOrIntensityIgnored = false;
    if (SharedHelpers::IsRS3OrHigher())
    {
        isColorOrIntensityIgnored = IsIgnored(state.InnerConeIntensity) && IsIgnored(state.OuterConeIntensity);
    }
    else
    {
        isColorOrIntensityIgnored = IsIgnored(state.InnerConeColor) && IsIgnored(state.OuterConeColor);
    }

    const bool isOuterAngleScaleIgnored = IsIgnored(state.OuterAngleScale);

    return isColorOrIntensityIgnored && isOuterAngleScaleIgnored;
}


// Returns a function that cancels onComplete callback
void PlaySpotLightStateAnimation(
    const winrt::SpotLight& compositionSpotLight,
    const winrt::CompositionPropertySet& colorsProxy,
    const winrt::CompositionPropertySet& offsetProps,
    const RevealHoverSpotlightStateDesc& targetState,
    std::function<void()>* cancelationFunction,
    std::function<void()> onComplete)
{
    if (cancelationFunction) *cancelationFunction = nullptr;

    if (compositionSpotLight && colorsProxy && offsetProps)
    {
        if (!IsStateAnimated(targetState))
        {
            if (SharedHelpers::IsRS3OrHigher())
            {
                AnimateTo(compositionSpotLight, targetState.InnerConeIntensity);
                AnimateTo(compositionSpotLight, targetState.OuterConeIntensity);
            }
            else
            {
                AnimateTo(colorsProxy, targetState.InnerConeColor);
                AnimateTo(colorsProxy, targetState.OuterConeColor);
            }
            AnimateTo(offsetProps, targetState.OuterAngleScale);

            if (onComplete)
            {
                onComplete();
            }
        }
        else
        {
            winrt::CompositionScopedBatch scopedBatch = compositionSpotLight.Compositor().CreateScopedBatch(winrt::CompositionBatchTypes::Animation);

            if (SharedHelpers::IsRS3OrHigher())
            {
                AnimateTo(compositionSpotLight, targetState.InnerConeIntensity);
                AnimateTo(compositionSpotLight, targetState.OuterConeIntensity);
            }
            else
            {
                AnimateTo(colorsProxy, targetState.InnerConeColor);
                AnimateTo(colorsProxy, targetState.OuterConeColor);
            }
            AnimateTo(offsetProps, targetState.OuterAngleScale);

            scopedBatch.End();

            if (onComplete)
            {
                const auto completedEventToken = scopedBatch.Completed([onComplete = std::move(onComplete)](auto&, auto&)
                {
                    onComplete();
                });

                if (cancelationFunction)
                {
                    *cancelationFunction = [scopedBatch, completedEventToken]
                    {
                        scopedBatch.Completed(completedEventToken);
                    };
                }
            }
        }
    }
    else
    {
        if (onComplete)
        {
            onComplete();
        }
    }
}

void SetSpotLightStateImmediate(const winrt::SpotLight& compositionSpotLight,
    const winrt::CompositionPropertySet& colorsProxy,
    const winrt::CompositionPropertySet& offsetProps,
    const RevealHoverSpotlightStateDesc& targetState)
{
    if (!compositionSpotLight || IsStateIgnored(targetState))
    {
        return;
    }

    if (SharedHelpers::IsRS3OrHigher())
    {
        if (!IsIgnored(targetState.InnerConeIntensity))
        {
            SetValueDirect(compositionSpotLight, targetState.InnerConeIntensity.SpotlightProperty, targetState.InnerConeIntensity.Value);
        }

        if (!IsIgnored(targetState.OuterConeIntensity))
        {
            SetValueDirect(compositionSpotLight, targetState.OuterConeIntensity.SpotlightProperty, targetState.OuterConeIntensity.Value);
        }
    }
    else
    {
        if (!IsIgnored(targetState.InnerConeColor))
        {
            SetValueDirect(colorsProxy, targetState.InnerConeColor.SpotlightProperty, targetState.InnerConeColor.Value);
        }

        if (!IsIgnored(targetState.OuterConeColor))
        {
            SetValueDirect(colorsProxy, targetState.OuterConeColor.SpotlightProperty, targetState.OuterConeColor.Value);
        }
    }

    if (!IsIgnored(targetState.OuterAngleScale))
    {
        SetValueDirect(offsetProps, targetState.OuterAngleScale.PropertyName, targetState.OuterAngleScale.Value);
    }
}

void SetSpotLightStateImmediate(const winrt::SpotLight& compositionSpotLight,
    const winrt::CompositionPropertySet& colorsProxy,
    const winrt::CompositionPropertySet& offsetProps,
    const RevealBorderSpotlightStateDesc& targetState)
{
    if (!compositionSpotLight)
    {
        return;
    }

    SetValueDirect(compositionSpotLight, targetState.ConstantAttenuation.SpotlightProperty, targetState.ConstantAttenuation.Value);
    SetValueDirect(compositionSpotLight, targetState.LinearAttenuation.SpotlightProperty, targetState.LinearAttenuation.Value);
    SetValueDirect(compositionSpotLight, targetState.InnerConeAngleInDegrees.SpotlightProperty, targetState.InnerConeAngleInDegrees.Value);
    SetValueDirect(colorsProxy, targetState.InnerConeColor.SpotlightProperty, targetState.InnerConeColor.Value);
    SetValueDirect(compositionSpotLight, targetState.OuterConeAngleInDegrees.SpotlightProperty, targetState.OuterConeAngleInDegrees.Value);
    SetValueDirect(colorsProxy, targetState.OuterConeColor.SpotlightProperty, targetState.OuterConeColor.Value);
    SetValueDirect(offsetProps, targetState.Height.PropertyName, targetState.Height.Value);
    SetValueDirect(offsetProps, targetState.OuterAngleScale.PropertyName, targetState.OuterAngleScale.Value);
}

// Returns property set that contains InnerConeColor, OuterConeColor and LightIntensity properties.
// Since the light does not have a dedicated linear intensity knob we introduce our own by combining
// the scalar LightIntensity and the color in an expression animation.
// SoftLightSwitch operates on the LightIntensity property of the returned property set.
// Other parties may independently set InnerConeColor and OuterConeColor.
// Note that the light ignores the A component of the color so the intensity must affect all RGB channels.
winrt::CompositionPropertySet CreateSpotLightColorsProxy(const winrt::SpotLight& compositionSpotLight)
{
    auto lightProperties = compositionSpotLight.Properties();
    auto compositor = compositionSpotLight.Compositor();

    lightProperties.InsertColor(L"InnerConeColor", { 255, 255, 255, 255 });
    lightProperties.InsertColor(L"OuterConeColor", { 255, 255, 255, 255 });
    lightProperties.InsertScalar(L"LightIntensity", 0);

    auto innerConeColorExpression = compositor.CreateExpressionAnimation(L"ColorLerp(ColorRgb(0,0,0,0), target.InnerConeColor, target.LightIntensity)");
    innerConeColorExpression.SetReferenceParameter(L"target", lightProperties);

    auto outerConeColorExpression = compositor.CreateExpressionAnimation(L"ColorLerp(ColorRgb(0,0,0,0), target.OuterConeColor, target.LightIntensity)");
    outerConeColorExpression.SetReferenceParameter(L"target", lightProperties);

    compositionSpotLight.StartAnimation(L"InnerConeColor", innerConeColorExpression);
    compositionSpotLight.StartAnimation(L"OuterConeColor", outerConeColorExpression);

    return lightProperties;
}
