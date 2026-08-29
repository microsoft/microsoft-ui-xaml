// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Effects.h"

EffectBase::EffectBase()
{
    IFCFAILFAST(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Windows_Foundation_PropertyValue).Get(),
        &m_propertyValueFactory));
}

HRESULT EffectBase::put_Name(_In_ HSTRING name)
{
    return Name.Set(name);
}

HRESULT EffectBase::get_Name(_Out_ HSTRING* name)
{
    return Name.CopyTo(name);
}

ColorSourceEffect::ColorSourceEffect()
{
    Color = { 0, 0, 0, 0 };
}

HRESULT ColorSourceEffect::GetEffectId(_Out_ GUID * id)
{
    *id = CLSID_D2D1Flood;
    return S_OK;
}

HRESULT ColorSourceEffect::GetSourceCount(_Out_ UINT * count)
{
    *count = 0;
    return S_OK;
}

HRESULT ColorSourceEffect::GetPropertyCount(_Out_ UINT * count)
{
    *count = 1;
    return S_OK;
}

HRESULT ColorSourceEffect::GetSource(UINT /*index*/, _Outptr_ wgr::Effects::IGraphicsEffectSource ** source)
{
    *source = nullptr;
    return E_INVALIDARG;
}

HRESULT ColorSourceEffect::GetProperty(UINT /*index*/, _Outptr_ wf::IPropertyValue ** value)
{
    return m_propertyValueFactory->CreateSingleArray(4, reinterpret_cast<float*>(&Color), (IInspectable**)value);
}

HRESULT ColorSourceEffect::GetNamedPropertyMapping(LPCWSTR name, _Out_ UINT * index,
    _Out_ wgr::Effects::GRAPHICS_EFFECT_PROPERTY_MAPPING * mapping)
{
    *index = D2D1_FLOOD_PROP_COLOR;
    *mapping = wgr::Effects::GRAPHICS_EFFECT_PROPERTY_MAPPING_COLOR_TO_VECTOR4;
    return _wcsicmp(name, L"Color") == 0 ? S_OK : E_INVALIDARG;
}

HRESULT CompositeEffect::GetEffectId(_Out_ GUID * id)
{
    *id = CLSID_D2D1Composite;
    return S_OK;
}

HRESULT CompositeEffect::GetSourceCount(_Out_ UINT * count)
{
    *count = 2;
    return S_OK;
}

HRESULT CompositeEffect::GetPropertyCount(_Out_ UINT * count)
{
    *count = 1;
    return S_OK;
}

HRESULT CompositeEffect::GetSource(UINT index, _Outptr_ wgr::Effects::IGraphicsEffectSource ** source)
{
    if (index >= 2) { return E_INVALIDARG; }
    return (index == 0 ? Destination : Source).CopyTo(source);
}

HRESULT CompositeEffect::GetProperty(UINT /*index*/, _Outptr_ wf::IPropertyValue ** value)
{
    return m_propertyValueFactory->CreateUInt32(static_cast<UINT32>(Mode), (IInspectable**)value);
}

HRESULT CompositeEffect::GetNamedPropertyMapping(LPCWSTR name, _Out_ UINT * index,
    _Out_ wgr::Effects::GRAPHICS_EFFECT_PROPERTY_MAPPING * mapping)
{
    *index = D2D1_COMPOSITE_PROP_MODE;
    *mapping = wgr::Effects::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT;
    return _wcsicmp(name, L"Mode") == 0 ? S_OK : E_INVALIDARG;
}

Transform2DEffect::Transform2DEffect()
{
    TransformMatrix = { 1, 0, 0, 1, 0, 0 };
}

HRESULT Transform2DEffect::GetEffectId(_Out_ GUID * id)
{
    *id = CLSID_D2D12DAffineTransform;
    return S_OK;
}

HRESULT Transform2DEffect::GetSourceCount(_Out_ UINT * count)
{
    *count = 1;
    return S_OK;
}

HRESULT Transform2DEffect::GetPropertyCount(_Out_ UINT * count)
{
    *count = 4;
    return S_OK;
}

HRESULT Transform2DEffect::GetSource(UINT index, _Outptr_ wgr::Effects::IGraphicsEffectSource ** source)
{
    Source.CopyTo(source);
    return index == 0 ? S_OK : E_INVALIDARG;
}

HRESULT Transform2DEffect::GetProperty(UINT index, _Outptr_ wf::IPropertyValue ** value)
{
    switch (index)
    {
        case D2D1_2DAFFINETRANSFORM_PROP_INTERPOLATION_MODE:
            return m_propertyValueFactory->CreateUInt32(static_cast<UINT32>(InterpolationMode), (IInspectable**)value);
        case D2D1_2DAFFINETRANSFORM_PROP_BORDER_MODE:
            return m_propertyValueFactory->CreateUInt32(static_cast<UINT32>(BorderMode), (IInspectable**)value);
        case D2D1_2DAFFINETRANSFORM_PROP_TRANSFORM_MATRIX:
            return m_propertyValueFactory->CreateSingleArray(6, reinterpret_cast<float*>(&TransformMatrix), (IInspectable**)value);
        case D2D1_2DAFFINETRANSFORM_PROP_SHARPNESS:
            return m_propertyValueFactory->CreateSingle(Sharpness, (IInspectable**)value);
        default:
            return E_INVALIDARG;
    }
}

HRESULT Transform2DEffect::GetNamedPropertyMapping(LPCWSTR name, _Out_ UINT * index,
    _Out_ wgr::Effects::GRAPHICS_EFFECT_PROPERTY_MAPPING * mapping)
{
    if (_wcsicmp(name, L"InterpolationMode") == 0)
    {
        *index = D2D1_2DAFFINETRANSFORM_PROP_INTERPOLATION_MODE;
        *mapping = wgr::Effects::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT;
    }
    else if (_wcsicmp(name, L"BorderMode") == 0)
    {
        *index = D2D1_2DAFFINETRANSFORM_PROP_BORDER_MODE;
        *mapping = wgr::Effects::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT;
    }
    else if (_wcsicmp(name, L"TransformMatrix") == 0)
    {
        *index = D2D1_2DAFFINETRANSFORM_PROP_TRANSFORM_MATRIX;
        *mapping = wgr::Effects::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT;
    }
    else if (_wcsicmp(name, L"Sharpness") == 0)
    {
        *index = D2D1_2DAFFINETRANSFORM_PROP_SHARPNESS;
        *mapping = wgr::Effects::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT;
    }
    else
    {
        return E_INVALIDARG;
    }

    return S_OK;
}
