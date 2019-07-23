// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "AnimationUtility.h"
#include "common.h"

void SetValueDirect(const winrt::CompositionPropertySet& target, std::wstring_view const& propName, const float value)
{
    target.InsertScalar(propName, value);
}

void SetValueDirect(const winrt::CompositionPropertySet& target, std::wstring_view const& propName, const winrt::Color& value)
{
    target.InsertColor(propName, value);
}

void SetValueDirect(const winrt::SpotLight& target, const SpotlightProperty prop, const winrt::float3& value)
{
    switch (prop)
    {
    case SpotlightProperty::Offset:
        target.Offset(value);
        break;
    case SpotlightProperty::Direction:
        target.Direction(value);
        break;
    }
}

void SetValueDirect(const winrt::SpotLight& target, const SpotlightProperty prop, const float value)
{
    switch (prop)
    {
    case SpotlightProperty::ConstantAttenuation:
        target.ConstantAttenuation(value);
        break;
    case SpotlightProperty::LinearAttenuation:
        target.LinearAttenuation(value);
        break;
    case SpotlightProperty::QuadraticAttenuation:
        target.QuadraticAttenuation(value);
        break;
    case SpotlightProperty::InnerConeAngleInDegrees:
        target.InnerConeAngleInDegrees(value);
        break;
    case SpotlightProperty::InnerConeAngle:
        target.InnerConeAngle(value);
        break;
    case SpotlightProperty::InnerConeIntensity:
        target.InnerConeIntensity(value);
        break;
    case SpotlightProperty::OuterConeAngleInDegrees:
        target.OuterConeAngleInDegrees(value);
        break;
    case SpotlightProperty::OuterConeAngle:
        target.OuterConeAngle(value);
        break;
    case SpotlightProperty::OuterConeIntensity:
        target.OuterConeIntensity(value);
        break;
    }
}

void SetValueDirect(const winrt::CompositionPropertySet& target, const SpotlightProperty prop, const winrt::Color& value)
{
    switch (prop)
    {
    case SpotlightProperty::InnerConeColor:
        target.InsertColor(L"InnerConeColor", value);
        break;
    case SpotlightProperty::OuterConeColor:
        target.InsertColor(L"OuterConeColor", value);
        break;
    }
}