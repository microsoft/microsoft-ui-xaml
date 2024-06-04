// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Defines classes for effects used by VisualContentRenderer
//  Basically these are copies of effect descriptions from Win2D

#pragma once

#include <string>
#include <windows.h>
#include <d2d1_1.h>
#include <d2d1effects.h>
#include <Windows.Graphics.Effects.h>
#include <Windows.Graphics.Effects.Interop.h>

using namespace Microsoft::WRL;

// Base class for Win2D-like effect descriptions
class EffectBase : public Microsoft::WRL::Implements<
        Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::WinRtClassicComMix>,
        wgr::Effects::IGraphicsEffect,
        wgr::Effects::IGraphicsEffectSource,
        wgr::Effects::IGraphicsEffectD2D1Interop>
{
public:

    IFACEMETHODIMP put_Name(_In_ HSTRING name) override;
    IFACEMETHODIMP get_Name(_Out_ HSTRING * name) override;
    
protected:
    EffectBase();
    virtual ~EffectBase() {}

protected:
    Microsoft::WRL::ComPtr<wf::IPropertyValueStatics> m_propertyValueFactory;

public:
    wrl_wrappers::HString Name;
};

// The Win2D ColorSource or D2D Flood effect, which generates a solid color
class ColorSourceEffect : public Microsoft::WRL::RuntimeClass<
        Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::WinRtClassicComMix>,
        Microsoft::WRL::MixIn<ColorSourceEffect, EffectBase>>,
        public EffectBase
{
    InspectableClass(L"Microsoft.UI.Xaml.ColorSourceEffect", BaseTrust);

public:
    ColorSourceEffect();

public:
    IFACEMETHODIMP GetEffectId(_Out_ GUID * id) override;
    IFACEMETHODIMP GetSourceCount(_Out_ UINT * count) override;
    IFACEMETHODIMP GetPropertyCount(_Out_ UINT * count) override;
    IFACEMETHODIMP GetSource(UINT /*index*/, _Outptr_ wgr::Effects::IGraphicsEffectSource ** source) override;
    IFACEMETHODIMP GetProperty(UINT /*index*/, _Outptr_ wf::IPropertyValue ** value) override;
    IFACEMETHODIMP GetNamedPropertyMapping(LPCWSTR name, _Out_ UINT * index,
        _Out_ wgr::Effects::GRAPHICS_EFFECT_PROPERTY_MAPPING * mapping) override;

public:
    D2D1_COLOR_F Color;
};

// The Win2D/D2D Composite effect, which merge two inputs based on alpha channels
class CompositeEffect : public Microsoft::WRL::RuntimeClass<
        Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::WinRtClassicComMix>,
        Microsoft::WRL::MixIn<CompositeEffect, EffectBase>>,
        public EffectBase
{
    InspectableClass(L"Microsoft.UI.Xaml.CompositeEffect", BaseTrust);

public:
    IFACEMETHODIMP GetEffectId(_Out_ GUID * id) override;
    IFACEMETHODIMP GetSourceCount(_Out_ UINT * count) override;
    IFACEMETHODIMP GetPropertyCount(_Out_ UINT * count) override;
    IFACEMETHODIMP GetSource(UINT index, _Outptr_ wgr::Effects::IGraphicsEffectSource ** source) override;
    IFACEMETHODIMP GetProperty(UINT /*index*/, _Outptr_ wf::IPropertyValue ** value) override;
    IFACEMETHODIMP GetNamedPropertyMapping(LPCWSTR name, _Out_ UINT * index,
        _Out_ wgr::Effects::GRAPHICS_EFFECT_PROPERTY_MAPPING * mapping) override;

public:
    Microsoft::WRL::ComPtr<wgr::Effects::IGraphicsEffectSource> Source;
    Microsoft::WRL::ComPtr<wgr::Effects::IGraphicsEffectSource> Destination;
    D2D1_COMPOSITE_MODE Mode = D2D1_COMPOSITE_MODE_SOURCE_OVER;
};


// The Win2D Transform2D / D2D 2DAffineTransform effect, which transforms an element of the effect graph
class Transform2DEffect : public Microsoft::WRL::RuntimeClass<
        Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::WinRtClassicComMix>,
        Microsoft::WRL::MixIn<Transform2DEffect, EffectBase>>,
        public EffectBase
{
    InspectableClass(L"Microsoft.UI.Xaml.Tranform2DEffect", BaseTrust);

public:
    Transform2DEffect();

    IFACEMETHODIMP GetEffectId(_Out_ GUID * id) override;
    IFACEMETHODIMP GetSourceCount(_Out_ UINT * count) override;
    IFACEMETHODIMP GetPropertyCount(_Out_ UINT * count) override;
    IFACEMETHODIMP GetSource(UINT index, _Outptr_ wgr::Effects::IGraphicsEffectSource ** source) override;
    IFACEMETHODIMP GetProperty(UINT index, _Outptr_ wf::IPropertyValue ** value) override;
    IFACEMETHODIMP GetNamedPropertyMapping(LPCWSTR name, _Out_ UINT * index,
        _Out_ wgr::Effects::GRAPHICS_EFFECT_PROPERTY_MAPPING * mapping) override;

public:
    Microsoft::WRL::ComPtr<wgr::Effects::IGraphicsEffectSource> Source;
    D2D1_2DAFFINETRANSFORM_INTERPOLATION_MODE InterpolationMode = D2D1_2DAFFINETRANSFORM_INTERPOLATION_MODE_LINEAR;
    D2D1_BORDER_MODE BorderMode = D2D1_BORDER_MODE_SOFT;
    D2D1_MATRIX_3X2_F TransformMatrix;
    float Sharpness = 0;
};
