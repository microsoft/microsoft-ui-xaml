// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// This file contains a simple IGraphicsEffect implementation of a CrossFadeEffect. The
// code here is very similar to microsoft.ui.private.composition.effects_impl.h in the
// controls folder, except this is a raw ABI version and only supports the one effect.

#include <d2d1effects_2.h>
#include <Windows.Graphics.Effects.h>
#include <Windows.Graphics.Effects.Interop.h>

namespace ConnectedAnimationHelpers
{
    // Base class for Win2D-like effect descriptions
    template<typename TEffectInterface>
    class EffectBase abstract : public ::Microsoft::WRL::RuntimeClass<
        ::Microsoft::WRL::RuntimeClassFlags<::Microsoft::WRL::WinRtClassicComMix>,
        XAML_ABI_PARAMETER(Windows::Graphics::Effects::IGraphicsEffect),
        XAML_ABI_PARAMETER(Windows::Graphics::Effects::IGraphicsEffectSource),
        XAML_ABI_PARAMETER(Windows::Graphics::Effects::IGraphicsEffectD2D1Interop),
        TEffectInterface>
    {
    protected:
        using IPropertyValue = XAML_ABI_PARAMETER(Windows::Foundation::IPropertyValue);
        using IPropertyValueStatics = XAML_ABI_PARAMETER(Windows::Foundation::IPropertyValueStatics);
        using GRAPHICS_EFFECT_PROPERTY_MAPPING = XAML_ABI_PARAMETER(Windows::Graphics::Effects::GRAPHICS_EFFECT_PROPERTY_MAPPING);
        using PropertyMapping = XAML_ABI_PARAMETER(Windows::Graphics::Effects::GRAPHICS_EFFECT_PROPERTY_MAPPING);
        using IGraphicsEffectSource = XAML_ABI_PARAMETER(Windows::Graphics::Effects::IGraphicsEffectSource);

    public:
        // IGraphicsEffect
        IFACEMETHODIMP get_Name(_Out_ HSTRING* name) override { return Name.CopyTo(name); }
        IFACEMETHODIMP put_Name(_In_ HSTRING name) override { return Name.Set(name); }

        // IGraphicsEffectD2D1Interop
        IFACEMETHODIMP GetSourceCount(_Out_ UINT* count) override { *count = 0; return S_OK; }
        IFACEMETHODIMP GetPropertyCount(_Out_ UINT* count) override { *count = 0; return S_OK; }

        IFACEMETHODIMP GetSource(UINT, _Outptr_result_maybenull_ IGraphicsEffectSource**) override
        {
            return E_INVALIDARG;
        }

        IFACEMETHODIMP GetProperty(UINT, _Outptr_ IPropertyValue**) override
        {
            return E_INVALIDARG;
        }

        IFACEMETHODIMP GetNamedPropertyMapping(_In_z_ LPCWSTR, _Out_ UINT*,
            _Out_ GRAPHICS_EFFECT_PROPERTY_MAPPING*) override
        {
            return E_INVALIDARG;
        }

    protected:
        // Invokes a functor with the pointer to the property factory
        template<typename TFunc>
        static HRESULT UsePropertyFactory(const TFunc& func)
        {
            ::Microsoft::WRL::ComPtr<IPropertyValueStatics> propertyValueFactory;
            ::wrl_wrappers::HStringReference activatableClassId{ RuntimeClass_Windows_Foundation_PropertyValue };
            HRESULT hr = GetActivationFactory(activatableClassId.Get(), &propertyValueFactory);
            return FAILED(hr) ? hr : func(propertyValueFactory.Get());
        }

    public:
        ::wrl_wrappers::HString Name;
    };

    // This interface is used in tests by MockDComp to dump the values of the CrossFadeEffect
    interface __declspec(uuid("2A451393-CCC3-4C7E-B9BC-5AD9F9538C09"))
    ICrossFadeEffect : public IInspectable
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE get_Weight(float * value) = 0;
        virtual HRESULT STDMETHODCALLTYPE put_Weight(float value) = 0;
        virtual HRESULT STDMETHODCALLTYPE get_Source1(XAML_ABI_PARAMETER(Windows::Graphics::Effects::IGraphicsEffectSource) **source) = 0;
        virtual HRESULT STDMETHODCALLTYPE put_Source1(XAML_ABI_PARAMETER(Windows::Graphics::Effects::IGraphicsEffectSource) *source) = 0;
        virtual HRESULT STDMETHODCALLTYPE get_Source2(XAML_ABI_PARAMETER(Windows::Graphics::Effects::IGraphicsEffectSource) **source) = 0;
        virtual HRESULT STDMETHODCALLTYPE put_Source2(XAML_ABI_PARAMETER(Windows::Graphics::Effects::IGraphicsEffectSource) *source) = 0;
    };
    
    class CrossFadeEffect WrlFinal : public EffectBase<ICrossFadeEffect>
    {
        InspectableClass(L"n/a", BaseTrust);

    public:
        IFACEMETHODIMP GetEffectId(_Out_ GUID* id) override { *id = CLSID_D2D1CrossFade; return S_OK; }

        IFACEMETHODIMP get_Weight(_Out_ float* value) override { *value = m_Weight; return S_OK; }
        IFACEMETHODIMP put_Weight(float value) override
        {
            if (!(value >= 0.0f && value <= 1.0f)) { return E_INVALIDARG; }
            m_Weight = value;
            return S_OK;
        }

        IFACEMETHODIMP get_Source1(_Outptr_result_maybenull_ IGraphicsEffectSource** value) override { return m_Source1.CopyTo(value); }
        IFACEMETHODIMP put_Source1(_In_ IGraphicsEffectSource* value) override { m_Source1 = value; return S_OK; }

        IFACEMETHODIMP get_Source2(_Outptr_result_maybenull_ IGraphicsEffectSource** value) override { return m_Source2.CopyTo(value); }
        IFACEMETHODIMP put_Source2(_In_ IGraphicsEffectSource* value) override { m_Source2 = value; return S_OK; }

        IFACEMETHODIMP GetSourceCount(_Out_ UINT* count) override { *count = 2; return S_OK; }
        IFACEMETHODIMP GetSource(UINT index, _Outptr_result_maybenull_ IGraphicsEffectSource** source) override
        {
            return index == 0 ? m_Source1.CopyTo(source) : index == 1 ? m_Source2.CopyTo(source) : E_INVALIDARG;
        }

        IFACEMETHODIMP GetNamedPropertyMapping(_In_z_ LPCWSTR name, _Out_ UINT* index,
            _Out_ GRAPHICS_EFFECT_PROPERTY_MAPPING* mapping) override
        {
            if (_wcsicmp(name, L"Weight") == 0)
            {
                *index = D2D1_CROSSFADE_PROP_WEIGHT;
                *mapping = PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT;
                return S_OK;
            }
            return E_INVALIDARG;
        }

        IFACEMETHODIMP GetPropertyCount(_Out_ UINT * count) override { *count = 1; return S_OK; }
        IFACEMETHODIMP GetProperty(UINT index, _Outptr_ IPropertyValue ** value) override
        {
            return UsePropertyFactory([=](IPropertyValueStatics* statics)
            {
                switch (index)
                {
                    case D2D1_CROSSFADE_PROP_WEIGHT: return statics->CreateSingle(m_Weight, (IInspectable**)value);
                    default: return E_INVALIDARG;
                }
            });
        }

    private:
        float m_Weight = 0.5f;
        ::Microsoft::WRL::ComPtr<IGraphicsEffectSource> m_Source1;
        ::Microsoft::WRL::ComPtr<IGraphicsEffectSource> m_Source2;
    };
}
