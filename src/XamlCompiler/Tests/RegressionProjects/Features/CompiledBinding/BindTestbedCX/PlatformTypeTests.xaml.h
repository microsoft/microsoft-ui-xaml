// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// PlatformTypeTests.xaml.h
// Declaration of the PlatformTypeTests class
//

#pragma once

#include "PlatformTypeTests.g.h"

namespace BindTestbedCX
{
    /// <summary>
    /// x:Load Tests
    /// </summary>
    [::Windows::Foundation::Metadata::WebHostHidden]
    public ref class PlatformTypeTests sealed
    {
    public:
        PlatformTypeTests();

        // Verify we can use these platform types as properties, and that we code-gen the correct type names
        property Platform::Type^ test;
        property Platform::String^ testString;
        property ::Windows::Foundation::Numerics::float3 vecThree;
        property ::Windows::Foundation::Numerics::float4x4 matFourFour;
        property ::Windows::Foundation::Numerics::quaternion quat;

        property ::Windows::Foundation::Rect rect;
        property ::Windows::Foundation::Point point;
        property ::Windows::Foundation::Size size;

        property Microsoft::UI::Xaml::CornerRadius cradi;
        property Microsoft::UI::Xaml::Duration dura;
        property Microsoft::UI::Xaml::DurationType duraT;
        property Microsoft::UI::Xaml::GridLength gridLen;
        property Microsoft::UI::Xaml::GridUnitType gridUnitType;
        property Microsoft::UI::Xaml::Thickness thickness;

        property Platform::Enum^ enumProp;
        property Platform::UIntPtr uintptr;
        property Platform::ValueType^ valType;
        property Platform::SizeT sizeT;
    };
}
