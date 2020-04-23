// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// This is a header-only implementation of Win2D-like effects descriptions,
// which can be use to describe effects graph in the Windows::UI::Composition API.

#pragma

#include <cstring>
#include <wrl.h>
#include <d2d1_1.h>
#include <d2d1effects_2.h>

#pragma warning(push)
#pragma warning(disable : 28285 28196 6387 6319 26812 26496)

#include "AlphaMaskEffect.g.h"
#include "ArithmeticCompositeEffect.g.h"
#include "BlendEffect.g.h"
#include "BorderEffect.g.h"
#include "ColorMatrixEffect.g.h"
#include "ColorSourceEffect.g.h"
#include "CompositeStepEffect.g.h"
#include "ContrastEffect.g.h"
#include "CrossFadeEffect.g.h"
#include "DistantDiffuseEffect.g.h"
#include "DistantSpecularEffect.g.h"
#include "ExposureEffect.g.h"
#include "GammaTransferEffect.g.h"
#include "GaussianBlurEffect.g.h"
#include "GrayscaleEffect.g.h"
#include "HueRotationEffect.g.h"
#include "InvertEffect.g.h"
#include "LinearTransferEffect.g.h"
#include "LuminanceToAlphaEffect.g.h"
#include "OpacityEffect.g.h"
#include "PointDiffuseEffect.g.h"
#include "PointSpecularEffect.g.h"
#include "PosterizeEffect.g.h"
#include "PremultiplyEffect.g.h"
#include "SaturationEffect.g.h"
#include "SepiaEffect.g.h"
#include "SpotDiffuseEffect.g.h"
#include "SpotSpecularEffect.g.h"
#include "TemperatureAndTintEffect.g.h"
#include "TintEffect.g.h"
#include "Transform2DEffect.g.h"
#include "UnPremultiplyEffect.g.h"

typedef enum GRAPHICS_EFFECT_PROPERTY_MAPPING
{
    GRAPHICS_EFFECT_PROPERTY_MAPPING_UNKNOWN,
    GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT,
    GRAPHICS_EFFECT_PROPERTY_MAPPING_VECTORX,
    GRAPHICS_EFFECT_PROPERTY_MAPPING_VECTORY,
    GRAPHICS_EFFECT_PROPERTY_MAPPING_VECTORZ,
    GRAPHICS_EFFECT_PROPERTY_MAPPING_VECTORW,
    GRAPHICS_EFFECT_PROPERTY_MAPPING_RECT_TO_VECTOR4,
    GRAPHICS_EFFECT_PROPERTY_MAPPING_RADIANS_TO_DEGREES,
    GRAPHICS_EFFECT_PROPERTY_MAPPING_COLORMATRIX_ALPHA_MODE,
    GRAPHICS_EFFECT_PROPERTY_MAPPING_COLOR_TO_VECTOR3,
    GRAPHICS_EFFECT_PROPERTY_MAPPING_COLOR_TO_VECTOR4
} GRAPHICS_EFFECT_PROPERTY_MAPPING;

namespace abi
{
    struct IPropertyValue : IInspectable {};
    struct IGraphicsEffectSource : IInspectable {};
}

// NOTE: We are redefining this to work around VS15.5 issue where the MIDL headers give errors with the new
// compiler. We don't really want to be including MIDL headers anyway so this is kind of ok. This interface
// is a redefinition of the one in Windows.Graphics.Effects.Interop.h.

DECLARE_INTERFACE_IID_(IGraphicsEffectD2D1Interop, IUnknown, "2FC57384-A068-44D7-A331-30982FCF7177")
{
    STDMETHOD(GetEffectId)(
        _Out_ GUID * id
        ) PURE;

    STDMETHOD(GetNamedPropertyMapping)(
        LPCWSTR name,
        _Out_ UINT * index,
        _Out_ GRAPHICS_EFFECT_PROPERTY_MAPPING * mapping
        ) PURE;

    STDMETHOD(GetPropertyCount)(
        _Out_ UINT * count
        ) PURE;

    STDMETHOD(GetProperty)(
        UINT index,
        _Outptr_ abi::IPropertyValue ** value
        ) PURE;

    STDMETHOD(GetSource)(
        UINT index,
        _Outptr_ abi::IGraphicsEffectSource ** source
        ) PURE;

    STDMETHOD(GetSourceCount)(
        _Out_ UINT * count
        ) PURE;
};

inline winrt::IGraphicsEffectSource& to_winrt(abi::IGraphicsEffectSource*& instance)
{
    return reinterpret_cast<winrt::IGraphicsEffectSource&>(instance);
}

inline winrt::IPropertyValue& to_winrt(abi::IPropertyValue*& instance)
{
    return reinterpret_cast<winrt::IPropertyValue&>(instance);
}


namespace Microsoft { namespace UI { namespace Composition { namespace Effects
{
    // Base class for Win2D-like effect descriptions
    class EffectBase : 
        public winrt::implements<
            EffectBase,
            ::IGraphicsEffectD2D1Interop>
    {
    protected:
        // This is a header file so we can't use "using namespace", but we can do this:
        using UIColor = winrt::Color; // Renamed because we use "Color" as a field name
        using PropertyValue = winrt::PropertyValue;
        using Vector2 = winrt::float2;
        using Vector3 = winrt::float3;
        using Matrix3x2 = winrt::float3x2;
        using Matrix5x4 = winrt::Matrix5x4;
        using PropertyMapping = GRAPHICS_EFFECT_PROPERTY_MAPPING;

    public:
        // IGraphicsEffect
        winrt::hstring Name() { return m_Name; }
        void Name(winrt::hstring const& value) { m_Name = value; }

        // IGraphicsEffectD2D1Interop
        IFACEMETHODIMP GetSourceCount(_Out_ UINT * count) override { *count = 0; return S_OK; }
        IFACEMETHODIMP GetPropertyCount(_Out_ UINT* count) override { *count = 0; return S_OK; }

        IFACEMETHODIMP GetSource(UINT, _Outptr_ abi::IGraphicsEffectSource**) override
        {
            return E_INVALIDARG;
        }

        IFACEMETHODIMP GetProperty(UINT, _Outptr_ abi::IPropertyValue**) override
        {
            return E_INVALIDARG;
        }

        IFACEMETHODIMP GetNamedPropertyMapping(LPCWSTR, _Out_ UINT*,
            _Out_ GRAPHICS_EFFECT_PROPERTY_MAPPING*) override
        {
            return E_INVALIDARG;
        }

    protected:
        // Invokes a functor with the pointer to the property factory
        static HRESULT UsePropertyFactory(_Outptr_ abi::IPropertyValue **value, std::function<winrt::IInspectable()> const& func) try
        {
            auto ret = func();
            auto propertyValue = ret.as<winrt::IPropertyValue>();
            to_winrt(*value) = propertyValue;
            CATCH_RETURN;
        }

        template<UINT32 ComponentCount>
        static winrt::IInspectable CreateColor(UIColor color)
        {
            static_assert(ComponentCount == 3 || ComponentCount == 4, "Unexpected color component count.");
            float values[] = { color.R / 255.0f, color.G / 255.0f, color.B / 255.0f, color.A / 255.0f };
            return winrt::PropertyValue::CreateSingleArray(reinterpret_cast<std::array<float, ComponentCount>&>(values));
        }

        // Helpers to implement GetNamedPropertyMapping more succintly
        struct NamedProperty
        {
            const wchar_t* Name; // Compile-time constant
            UINT Index; // Property index
            GRAPHICS_EFFECT_PROPERTY_MAPPING Mapping;
        };

        HRESULT GetNamedPropertyMappingImpl(
            _In_count_(mappingCount) const NamedProperty* namedProperties,
            UINT namedPropertyCount,
            LPCWSTR name,
            _Out_ UINT * index,
            _Out_ GRAPHICS_EFFECT_PROPERTY_MAPPING * mapping)
        {
            for (UINT i = 0; i < namedPropertyCount; ++i)
            {
                const auto& prop = namedProperties[i];
                if (_wcsicmp(name, prop.Name) == 0)
                {
                    *index = prop.Index;
                    *mapping = prop.Mapping;
                    return S_OK;
                }
            }
            return E_INVALIDARG;
        }

        // M_PI requires us to be the first to include math.h, not worth it
        static constexpr float k_PI = 3.14159265358979f;
        static constexpr float k_DegreesPerRadian = 180.0f / k_PI;

    public:
        winrt::hstring m_Name;
    };

    //-----------------------------------------------------------------------------------------------------------------
    // Helper macros to make implementation more succint

#pragma push_macro("DECLARE_D2D_GUID")
#undef DECLARE_D2D_GUID
#define DECLARE_D2D_GUID(Guid) \
    IFACEMETHODIMP GetEffectId(_Out_ GUID * id) override { *id = Guid; return S_OK; }

#pragma push_macro("DECLARE_POD_PROPERTY")
#undef DECLARE_POD_PROPERTY
#define DECLARE_POD_PROPERTY(Name, Type, InitialValue, Condition) \
    private: \
    Type m_##Name = InitialValue; \
    public: \
    Type Name() { return m_##Name; } \
    void Name(Type const& value) \
    { \
        if (!(0, Condition)) { throw winrt::hresult_invalid_argument(); } \
        m_##Name = value; \
    }

#pragma push_macro("DECLARE_SOURCE")
#undef DECLARE_SOURCE
#define DECLARE_SOURCE(Name) \
    winrt::IGraphicsEffectSource m_##Name; \
    winrt::IGraphicsEffectSource Name() { return m_##Name; } \
    void Name(winrt::IGraphicsEffectSource const& value) { m_##Name = value; }

#pragma push_macro("DECLARE_SINGLE_SOURCE")
#undef DECLARE_SINGLE_SOURCE
#define DECLARE_SINGLE_SOURCE(Name) \
    DECLARE_SOURCE(Name) \
    IFACEMETHODIMP GetSourceCount(_Out_ UINT * count) override { *count = 1; return S_OK; } \
    IFACEMETHODIMP GetSource(UINT index, _Outptr_ abi::IGraphicsEffectSource ** source) override try \
    { \
        if (index == 0) to_winrt(*source) = m_##Name; \
        else throw winrt::hresult_invalid_argument(); \
        CATCH_RETURN; \
    }

#pragma push_macro("DECLARE_DUAL_SOURCES")
#undef DECLARE_DUAL_SOURCES
#define DECLARE_DUAL_SOURCES(Name1, Name2) \
    DECLARE_SOURCE(Name1) \
    DECLARE_SOURCE(Name2) \
    IFACEMETHODIMP GetSourceCount(_Out_ UINT * count) override { *count = 2; return S_OK; } \
    IFACEMETHODIMP GetSource(UINT index, _Outptr_ abi::IGraphicsEffectSource ** source) override try \
    { \
        if (index == 0) to_winrt(*source) = m_##Name1; \
        else if (index == 1) to_winrt(*source) = m_##Name2; \
        else throw winrt::hresult_invalid_argument(); \
        CATCH_RETURN; \
    }

#pragma push_macro("DECLARE_NAMED_PROPERTY_MAPPING")
#undef DECLARE_NAMED_PROPERTY_MAPPING
#define DECLARE_NAMED_PROPERTY_MAPPING(...) \
    IFACEMETHODIMP GetNamedPropertyMapping(LPCWSTR name, _Out_ UINT * index, \
        _Out_ GRAPHICS_EFFECT_PROPERTY_MAPPING * mapping) override \
    { \
        static const NamedProperty s_Properties[] = { __VA_ARGS__ }; \
        return GetNamedPropertyMappingImpl(s_Properties, _countof(s_Properties), name, index, mapping); \
    }
    

    //-----------------------------------------------------------------------------------------------------------------

    class AlphaMaskEffect : 
        public winrt::Microsoft::UI::Composition::Effects::implementation::AlphaMaskEffectT<AlphaMaskEffect, EffectBase>
    {
    public:
        DECLARE_D2D_GUID(CLSID_D2D1AlphaMask);
        DECLARE_DUAL_SOURCES(Source, Mask);
    };

    //-----------------------------------------------------------------------------------------------------------------

    class ArithmeticCompositeEffect :
        public winrt::Microsoft::UI::Composition::Effects::implementation::ArithmeticCompositeEffectT<ArithmeticCompositeEffect, EffectBase>
    {
    public:
        DECLARE_D2D_GUID(CLSID_D2D1ArithmeticComposite);
        DECLARE_DUAL_SOURCES(Source1, Source2);
        DECLARE_POD_PROPERTY(MultiplyAmount, float, 1.0f, true);
        DECLARE_POD_PROPERTY(Source1Amount, float, 0.0f, true);
        DECLARE_POD_PROPERTY(Source2Amount, float, 0.0f, true);
        DECLARE_POD_PROPERTY(Offset, float, 0.0f, true);
        DECLARE_POD_PROPERTY(ClampOutput, bool, false, true);
        DECLARE_NAMED_PROPERTY_MAPPING(
            { L"MultiplyAmount", D2D1_ARITHMETICCOMPOSITE_PROP_COEFFICIENTS, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_VECTORX },
            { L"Source1Amount", D2D1_ARITHMETICCOMPOSITE_PROP_COEFFICIENTS, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_VECTORY },
            { L"Source2Amount", D2D1_ARITHMETICCOMPOSITE_PROP_COEFFICIENTS, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_VECTORZ },
            { L"Offset", D2D1_ARITHMETICCOMPOSITE_PROP_COEFFICIENTS, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_VECTORW },
            { L"ClampOutput", D2D1_ARITHMETICCOMPOSITE_PROP_CLAMP_OUTPUT, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT });

    public:
        IFACEMETHODIMP GetPropertyCount(_Out_ UINT * count) override { *count = 2; return S_OK; }

        IFACEMETHODIMP GetProperty(UINT index, _Outptr_ abi::IPropertyValue ** value) override
        {
            return UsePropertyFactory(value, [=]()
            {
                switch (index)
                {
                    case D2D1_ARITHMETICCOMPOSITE_PROP_COEFFICIENTS:
                    {
                        float coefficients[4] = { m_MultiplyAmount, m_Source1Amount, m_Source2Amount, m_Offset };
                        return winrt::PropertyValue::CreateSingleArray({ coefficients });
                    }
                    case D2D1_ARITHMETICCOMPOSITE_PROP_CLAMP_OUTPUT:
                        return winrt::PropertyValue::CreateBoolean(m_ClampOutput);
                    default:
                        throw winrt::hresult_invalid_argument();
                }
            });
        }
    };

    //-----------------------------------------------------------------------------------------------------------------

    class BlendEffect :
        public winrt::Microsoft::UI::Composition::Effects::implementation::BlendEffectT<BlendEffect, EffectBase>
    {
    public:
        DECLARE_D2D_GUID(CLSID_D2D1Blend);
        DECLARE_DUAL_SOURCES(Background, Foreground);
        DECLARE_POD_PROPERTY(Mode, winrt::BlendEffectMode, winrt::BlendEffectMode::Multiply, true);
        DECLARE_NAMED_PROPERTY_MAPPING(
            { L"Mode", D2D1_BLEND_PROP_MODE, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT });

    public:
        IFACEMETHODIMP GetPropertyCount(_Out_ UINT * count) override { *count = 1; return S_OK; }

        IFACEMETHODIMP GetProperty(UINT index, _Outptr_ abi::IPropertyValue ** value) override
        {
            return UsePropertyFactory(value, [=]()
            {
                switch (index)
                {
                    case D2D1_BLEND_PROP_MODE: return winrt::PropertyValue::CreateUInt32((UINT32)m_Mode);
                    default: throw winrt::hresult_invalid_argument();
                }
            });
        }
    };

    //-----------------------------------------------------------------------------------------------------------------

    class BorderEffect :
        public winrt::Microsoft::UI::Composition::Effects::implementation::BorderEffectT<BorderEffect, EffectBase>
    {
    public:
        DECLARE_D2D_GUID(CLSID_D2D1Border);
        DECLARE_SINGLE_SOURCE(Source);
        DECLARE_POD_PROPERTY(ExtendX, winrt::CanvasEdgeBehavior, winrt::CanvasEdgeBehavior::Clamp, true);
        DECLARE_POD_PROPERTY(ExtendY, winrt::CanvasEdgeBehavior, winrt::CanvasEdgeBehavior::Clamp, true);
        DECLARE_NAMED_PROPERTY_MAPPING(
            { L"ExtendX", D2D1_BORDER_PROP_EDGE_MODE_X, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"ExtendY", D2D1_BORDER_PROP_EDGE_MODE_Y, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT });

    public:
        IFACEMETHODIMP GetPropertyCount(_Out_ UINT * count) override { *count = 2; return S_OK; }

        IFACEMETHODIMP GetProperty(UINT index, _Outptr_ abi::IPropertyValue ** value) override
        {
            return UsePropertyFactory(value, [=]()
            {
                switch (index)
                {
                case D2D1_BORDER_PROP_EDGE_MODE_X: return winrt::PropertyValue::CreateUInt32((UINT32)m_ExtendX);
                case D2D1_BORDER_PROP_EDGE_MODE_Y: return winrt::PropertyValue::CreateUInt32((UINT32)m_ExtendY);
                default: throw winrt::hresult_invalid_argument();
                }
            });
        }
    };

    //-----------------------------------------------------------------------------------------------------------------

    class ColorMatrixEffect :
        public winrt::Microsoft::UI::Composition::Effects::implementation::ColorMatrixEffectT<ColorMatrixEffect, EffectBase>
    {
    public:
        DECLARE_D2D_GUID(CLSID_D2D1ColorMatrix);
        DECLARE_SINGLE_SOURCE(Source);
        DECLARE_POD_PROPERTY(ColorMatrix, winrt::Matrix5x4, (winrt::Matrix5x4{ 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0 }), true);
        DECLARE_POD_PROPERTY(AlphaMode, winrt::CanvasAlphaMode, winrt::CanvasAlphaMode::Premultiplied, value != winrt::CanvasAlphaMode::Ignore);
        DECLARE_POD_PROPERTY(ClampOutput, bool, false, true);
        DECLARE_NAMED_PROPERTY_MAPPING(
            { L"ColorMatrix", D2D1_COLORMATRIX_PROP_COLOR_MATRIX, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"AlphaMode", D2D1_COLORMATRIX_PROP_ALPHA_MODE, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_COLORMATRIX_ALPHA_MODE },
            { L"ClampOutput", D2D1_COLORMATRIX_PROP_CLAMP_OUTPUT, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT });

    public:
        IFACEMETHODIMP GetPropertyCount(_Out_ UINT * count) override { *count = 3; return S_OK; }

        IFACEMETHODIMP GetProperty(UINT index, _Outptr_ abi::IPropertyValue ** value) override
        {
            return UsePropertyFactory(value, [=]()
            {
                switch (index)
                {
                    case D2D1_COLORMATRIX_PROP_COLOR_MATRIX: return winrt::PropertyValue::CreateSingleArray(reinterpret_cast<std::array<float, 20>&>(m_ColorMatrix));
                    case D2D1_COLORMATRIX_PROP_ALPHA_MODE:
                    {
                        switch (m_AlphaMode)
                        {
                            case winrt::CanvasAlphaMode::Premultiplied: 
                                return winrt::PropertyValue::CreateUInt32(D2D1_COLORMANAGEMENT_ALPHA_MODE_PREMULTIPLIED);
                            case winrt::CanvasAlphaMode::Straight:
                                return winrt::PropertyValue::CreateUInt32(D2D1_COLORMANAGEMENT_ALPHA_MODE_STRAIGHT);
                        }
                        break;
                    }
                    case D2D1_COLORMATRIX_PROP_CLAMP_OUTPUT: return winrt::PropertyValue::CreateBoolean(m_ClampOutput);
                }
                throw winrt::hresult_invalid_argument();
            });
        }
    };

    //-----------------------------------------------------------------------------------------------------------------

    class ColorSourceEffect :
        public winrt::Microsoft::UI::Composition::Effects::implementation::ColorSourceEffectT<ColorSourceEffect, EffectBase>
    {
    public:
        DECLARE_D2D_GUID(CLSID_D2D1Flood);
        DECLARE_POD_PROPERTY(Color, UIColor, (UIColor{ 255, 0, 0, 0 }), true);
        DECLARE_NAMED_PROPERTY_MAPPING(
            { L"Color", D2D1_FLOOD_PROP_COLOR, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_COLOR_TO_VECTOR4 });

    public:
        IFACEMETHODIMP GetPropertyCount(_Out_ UINT * count) override { *count = 1; return S_OK; }

        IFACEMETHODIMP GetProperty(UINT index, _Outptr_ abi::IPropertyValue ** value) override
        {
            return UsePropertyFactory(value, [=]()
            {
                switch (index)
                {
                    case D2D1_FLOOD_PROP_COLOR: return CreateColor<4>(m_Color);
                    default: throw winrt::hresult_invalid_argument();
                }
            });
        }
    };

    //-----------------------------------------------------------------------------------------------------------------
    // Win2D has CompositeEffect with an arbitrary number of sources,
    // but this involves having an IVector of sources and is more trouble than it's worth.
    // We declare a simplified single-step composite effect between two sources.

    class CompositeStepEffect :
        public winrt::Microsoft::UI::Composition::Effects::implementation::CompositeStepEffectT<CompositeStepEffect, EffectBase>
    {
    public:
        DECLARE_D2D_GUID(CLSID_D2D1Composite);
        DECLARE_DUAL_SOURCES(Destination, Source);
        DECLARE_POD_PROPERTY(Mode, winrt::CanvasComposite, winrt::CanvasComposite::SourceOver, true);
        DECLARE_NAMED_PROPERTY_MAPPING(
            { L"Mode", D2D1_COMPOSITE_PROP_MODE, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT });

    public:
        IFACEMETHODIMP GetPropertyCount(_Out_ UINT * count) override { *count = 1; return S_OK; }

        IFACEMETHODIMP GetProperty(UINT index, _Outptr_ abi::IPropertyValue ** value) override
        {
            return UsePropertyFactory(value, [=]()
            {
                switch (index)
                {
                    case D2D1_COMPOSITE_PROP_MODE: return winrt::PropertyValue::CreateUInt32((UINT32)m_Mode);
                    default: throw winrt::hresult_invalid_argument();
                }
            });
        }
    };

    //-----------------------------------------------------------------------------------------------------------------

    class ContrastEffect :
        public winrt::Microsoft::UI::Composition::Effects::implementation::ContrastEffectT<ContrastEffect, EffectBase>
    {
    public:
        DECLARE_D2D_GUID(CLSID_D2D1Contrast);
        DECLARE_SINGLE_SOURCE(Source);
        DECLARE_POD_PROPERTY(Contrast, float, 0.0f, value >= -1.0f && value <= 1.0f);
        DECLARE_POD_PROPERTY(ClampSource, bool, false, true);
        DECLARE_NAMED_PROPERTY_MAPPING(
            { L"Contrast", D2D1_CONTRAST_PROP_CONTRAST, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"ClampSource", D2D1_CONTRAST_PROP_CLAMP_INPUT, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT });

    public:
        IFACEMETHODIMP GetPropertyCount(_Out_ UINT * count) override { *count = 2; return S_OK; }

        IFACEMETHODIMP GetProperty(UINT index, _Outptr_ abi::IPropertyValue ** value) override
        {
            return UsePropertyFactory(value, [=]()
            {
                switch (index)
                {
                    case D2D1_CONTRAST_PROP_CONTRAST: return winrt::PropertyValue::CreateSingle(m_Contrast);
                    case D2D1_CONTRAST_PROP_CLAMP_INPUT: return winrt::PropertyValue::CreateBoolean(m_ClampSource);
                    default: throw winrt::hresult_invalid_argument();
                }
            });
        }
    };

    //-----------------------------------------------------------------------------------------------------------------

    class CrossFadeEffect : 
        public winrt::Microsoft::UI::Composition::Effects::implementation::CrossFadeEffectT<CrossFadeEffect, EffectBase>
    {
    public:
        DECLARE_D2D_GUID(CLSID_D2D1CrossFade);
        DECLARE_DUAL_SOURCES(Source1, Source2);
        DECLARE_POD_PROPERTY(Weight, float, 0.5f, value >= 0.0f && value <= 1.0f);
        DECLARE_NAMED_PROPERTY_MAPPING(
            { L"Weight", D2D1_CROSSFADE_PROP_WEIGHT, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT });

    public:
        IFACEMETHODIMP GetPropertyCount(_Out_ UINT * count) override { *count = 1; return S_OK; }

        IFACEMETHODIMP GetProperty(UINT index, _Outptr_ abi::IPropertyValue ** value) override
        {
            return UsePropertyFactory(value, [=]()
            {
                switch (index)
                {
                    case D2D1_CROSSFADE_PROP_WEIGHT: return winrt::PropertyValue::CreateSingle(m_Weight);
                    default: throw winrt::hresult_invalid_argument();
                }
            });
        }
    };

    //-----------------------------------------------------------------------------------------------------------------

    class DistantDiffuseEffect :
        public winrt::Microsoft::UI::Composition::Effects::implementation::DistantDiffuseEffectT<DistantDiffuseEffect, EffectBase>
    {
    public:
        DECLARE_D2D_GUID(CLSID_D2D1DistantDiffuse);
        DECLARE_SINGLE_SOURCE(Source);
        DECLARE_POD_PROPERTY(Azimuth, float, 0.0f, true); // D2D clamps within [0, 360] degrees
        DECLARE_POD_PROPERTY(Elevation, float, 0.0f, true); // D2D clamps within [0, 360] degrees
        DECLARE_POD_PROPERTY(DiffuseAmount, float, 1.0f, value >= 0.0f && value <= 10000.0f);
        DECLARE_POD_PROPERTY(HeightMapScale, float, 1.0f, value >= -10000.0f && value <= 10000.0f);
        DECLARE_POD_PROPERTY(LightColor, UIColor, (UIColor{ 255, 255, 255, 255 }), true);
        DECLARE_POD_PROPERTY(HeightMapKernelSize, winrt::float2, (winrt::float2{ 1.0f, 1.0f }),
            value.x >= 0.01f && value.y >= 0.01f && value.x <= 100.0f && value.y <= 100.0f);
        DECLARE_POD_PROPERTY(HeightMapInterpolationMode, winrt::CanvasImageInterpolation, winrt::CanvasImageInterpolation::Linear, true);
        DECLARE_NAMED_PROPERTY_MAPPING(
            { L"Azimuth", D2D1_DISTANTDIFFUSE_PROP_AZIMUTH, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_RADIANS_TO_DEGREES },
            { L"Elevation", D2D1_DISTANTDIFFUSE_PROP_ELEVATION, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_RADIANS_TO_DEGREES },
            { L"DiffuseAmount", D2D1_DISTANTDIFFUSE_PROP_DIFFUSE_CONSTANT, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"HeightMapScale", D2D1_DISTANTDIFFUSE_PROP_SURFACE_SCALE, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"LightColor", D2D1_DISTANTDIFFUSE_PROP_COLOR, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_COLOR_TO_VECTOR3 },
            { L"HeightMapKernelSize", D2D1_DISTANTDIFFUSE_PROP_KERNEL_UNIT_LENGTH, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"HeightMapInterpolationMode", D2D1_DISTANTDIFFUSE_PROP_SCALE_MODE, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT }, );

    public:
        IFACEMETHODIMP GetPropertyCount(_Out_ UINT * count) override { *count = 7; return S_OK; }

        IFACEMETHODIMP GetProperty(UINT index, _Outptr_ abi::IPropertyValue ** value) override
        {
            return UsePropertyFactory(value, [=]()
            {
                switch (index)
                {
                    case D2D1_DISTANTDIFFUSE_PROP_AZIMUTH: return winrt::PropertyValue::CreateSingle(m_Azimuth * k_DegreesPerRadian);
                    case D2D1_DISTANTDIFFUSE_PROP_ELEVATION: return winrt::PropertyValue::CreateSingle(m_Elevation * k_DegreesPerRadian);
                    case D2D1_DISTANTDIFFUSE_PROP_DIFFUSE_CONSTANT: return winrt::PropertyValue::CreateSingle(m_DiffuseAmount);
                    case D2D1_DISTANTDIFFUSE_PROP_SURFACE_SCALE: return winrt::PropertyValue::CreateSingle(m_HeightMapScale);
                    case D2D1_DISTANTDIFFUSE_PROP_COLOR: return CreateColor<3>(m_LightColor);
                    case D2D1_DISTANTDIFFUSE_PROP_KERNEL_UNIT_LENGTH:
                        return winrt::PropertyValue::CreateSingleArray(reinterpret_cast<std::array<float, 2>&>(m_HeightMapKernelSize));
                    case D2D1_DISTANTDIFFUSE_PROP_SCALE_MODE: return winrt::PropertyValue::CreateUInt32((UINT32)m_HeightMapInterpolationMode);
                    default: throw winrt::hresult_invalid_argument();
                }
            });
        }
    };

    //-----------------------------------------------------------------------------------------------------------------

    class DistantSpecularEffect :
        public winrt::Microsoft::UI::Composition::Effects::implementation::DistantSpecularEffectT<DistantSpecularEffect, EffectBase>
    {
    public:
        DECLARE_D2D_GUID(CLSID_D2D1DistantSpecular);
        DECLARE_SINGLE_SOURCE(Source);
        DECLARE_POD_PROPERTY(Azimuth, float, 0.0f, true); // D2D clamps within [0, 360] degrees
        DECLARE_POD_PROPERTY(Elevation, float, 0.0f, true); // D2D clamps within [0, 360] degrees
        DECLARE_POD_PROPERTY(SpecularExponent, float, 1.0f, value >= -10000.0f && value <= 10000.0f);
        DECLARE_POD_PROPERTY(SpecularAmount, float, 1.0f, value >= 0.0f && value <= 10000.0f);
        DECLARE_POD_PROPERTY(HeightMapScale, float, 1.0f, value >= -10000.0f && value <= 10000.0f);
        DECLARE_POD_PROPERTY(LightColor, UIColor, (UIColor{ 255, 255, 255, 255 }), true);
        DECLARE_POD_PROPERTY(HeightMapKernelSize, Vector2, (Vector2{ 1.0f, 1.0f }),
            value.x >= 0.01f && value.y >= 0.01f && value.x <= 100.0f && value.y <= 100.0f);
        DECLARE_POD_PROPERTY(HeightMapInterpolationMode, winrt::CanvasImageInterpolation, winrt::CanvasImageInterpolation::Linear, true);
        DECLARE_NAMED_PROPERTY_MAPPING(
            { L"Azimuth", D2D1_DISTANTSPECULAR_PROP_AZIMUTH, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_RADIANS_TO_DEGREES },
            { L"Elevation", D2D1_DISTANTSPECULAR_PROP_ELEVATION, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_RADIANS_TO_DEGREES },
            { L"SpecularExponent", D2D1_DISTANTSPECULAR_PROP_SPECULAR_EXPONENT, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"SpecularAmount", D2D1_DISTANTSPECULAR_PROP_SPECULAR_CONSTANT, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"HeightMapScale", D2D1_DISTANTSPECULAR_PROP_SURFACE_SCALE, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"LightColor", D2D1_DISTANTSPECULAR_PROP_COLOR, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_COLOR_TO_VECTOR3 },
            { L"HeightMapKernelSize", D2D1_DISTANTSPECULAR_PROP_KERNEL_UNIT_LENGTH, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"HeightMapInterpolationMode", D2D1_DISTANTSPECULAR_PROP_SCALE_MODE, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },);

    public:
        IFACEMETHODIMP GetPropertyCount(_Out_ UINT * count) override { *count = 8; return S_OK; }

        IFACEMETHODIMP GetProperty(UINT index, _Outptr_ abi::IPropertyValue ** value) override
        {
            return UsePropertyFactory(value, [=]()
            {
                switch (index)
                {
                    case D2D1_DISTANTSPECULAR_PROP_AZIMUTH: return winrt::PropertyValue::CreateSingle(m_Azimuth * k_DegreesPerRadian);
                    case D2D1_DISTANTSPECULAR_PROP_ELEVATION: return winrt::PropertyValue::CreateSingle(m_Elevation * k_DegreesPerRadian);
                    case D2D1_DISTANTSPECULAR_PROP_SPECULAR_EXPONENT: return winrt::PropertyValue::CreateSingle(m_SpecularExponent);
                    case D2D1_DISTANTSPECULAR_PROP_SPECULAR_CONSTANT: return winrt::PropertyValue::CreateSingle(m_SpecularAmount);
                    case D2D1_DISTANTSPECULAR_PROP_SURFACE_SCALE: return winrt::PropertyValue::CreateSingle(m_HeightMapScale);
                    case D2D1_DISTANTSPECULAR_PROP_COLOR: return CreateColor<3>(m_LightColor);
                    case D2D1_DISTANTSPECULAR_PROP_KERNEL_UNIT_LENGTH:
                        return winrt::PropertyValue::CreateSingleArray(reinterpret_cast<std::array<float, 2>&>(m_HeightMapKernelSize));
                    case D2D1_DISTANTSPECULAR_PROP_SCALE_MODE: return winrt::PropertyValue::CreateUInt32((UINT32)m_HeightMapInterpolationMode);
                    default: throw winrt::hresult_invalid_argument();
                }
            });
        }
    };

    //-----------------------------------------------------------------------------------------------------------------

    class ExposureEffect :
        public winrt::Microsoft::UI::Composition::Effects::implementation::ExposureEffectT<ExposureEffect, EffectBase>
    {
    public:
        DECLARE_D2D_GUID(CLSID_D2D1Exposure);
        DECLARE_SINGLE_SOURCE(Source);
        DECLARE_POD_PROPERTY(Exposure, float, 0.0f, value >= -2.0f && value <= 2.0f);
        DECLARE_NAMED_PROPERTY_MAPPING(
            { L"Exposure", D2D1_EXPOSURE_PROP_EXPOSURE_VALUE, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT });

    public:
        IFACEMETHODIMP GetPropertyCount(_Out_ UINT * count) override { *count = 1; return S_OK; }

        IFACEMETHODIMP GetProperty(UINT index, _Outptr_ abi::IPropertyValue ** value) override
        {
            return UsePropertyFactory(value, [=]()
            {
                switch (index)
                {
                    case D2D1_EXPOSURE_PROP_EXPOSURE_VALUE: return winrt::PropertyValue::CreateSingle(m_Exposure);
                    default: throw winrt::hresult_invalid_argument();
                }
            });
        }
    };

    //-----------------------------------------------------------------------------------------------------------------

    class GammaTransferEffect :
        public winrt::Microsoft::UI::Composition::Effects::implementation::GammaTransferEffectT<GammaTransferEffect, EffectBase>
    {
    public:
        DECLARE_D2D_GUID(CLSID_D2D1GammaTransfer);
        DECLARE_SINGLE_SOURCE(Source);
        DECLARE_POD_PROPERTY(RedAmplitude, float, 1.0f, true);
        DECLARE_POD_PROPERTY(RedExponent, float, 1.0f, true);
        DECLARE_POD_PROPERTY(RedOffset, float, 0.0f, true);
        DECLARE_POD_PROPERTY(RedDisable, bool, false, true);
        DECLARE_POD_PROPERTY(GreenAmplitude, float, 1.0f, true);
        DECLARE_POD_PROPERTY(GreenExponent, float, 1.0f, true);
        DECLARE_POD_PROPERTY(GreenOffset, float, 0.0f, true);
        DECLARE_POD_PROPERTY(GreenDisable, bool, false, true);
        DECLARE_POD_PROPERTY(BlueAmplitude, float, 1.0f, true);
        DECLARE_POD_PROPERTY(BlueExponent, float, 1.0f, true);
        DECLARE_POD_PROPERTY(BlueOffset, float, 0.0f, true);
        DECLARE_POD_PROPERTY(BlueDisable, bool, false, true);
        DECLARE_POD_PROPERTY(AlphaAmplitude, float, 1.0f, true);
        DECLARE_POD_PROPERTY(AlphaExponent, float, 1.0f, true);
        DECLARE_POD_PROPERTY(AlphaOffset, float, 0.0f, true);
        DECLARE_POD_PROPERTY(AlphaDisable, bool, false, true);
        DECLARE_POD_PROPERTY(ClampOutput, bool, false, true);
        DECLARE_NAMED_PROPERTY_MAPPING(
            { L"RedAmplitude", D2D1_GAMMATRANSFER_PROP_RED_AMPLITUDE, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"RedExponent", D2D1_GAMMATRANSFER_PROP_RED_EXPONENT, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"RedOffset", D2D1_GAMMATRANSFER_PROP_RED_OFFSET, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"RedDisable", D2D1_GAMMATRANSFER_PROP_RED_DISABLE, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"GreenAmplitude", D2D1_GAMMATRANSFER_PROP_GREEN_AMPLITUDE, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"GreenExponent", D2D1_GAMMATRANSFER_PROP_GREEN_EXPONENT, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"GreenOffset", D2D1_GAMMATRANSFER_PROP_GREEN_OFFSET, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"GreenDisable", D2D1_GAMMATRANSFER_PROP_GREEN_DISABLE, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"BlueAmplitude", D2D1_GAMMATRANSFER_PROP_BLUE_AMPLITUDE, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"BlueExponent", D2D1_GAMMATRANSFER_PROP_BLUE_EXPONENT, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"BlueOffset", D2D1_GAMMATRANSFER_PROP_BLUE_OFFSET, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"BlueDisable", D2D1_GAMMATRANSFER_PROP_BLUE_DISABLE, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"AlphaAmplitude", D2D1_GAMMATRANSFER_PROP_ALPHA_AMPLITUDE, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"AlphaExponent", D2D1_GAMMATRANSFER_PROP_ALPHA_EXPONENT, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"AlphaOffset", D2D1_GAMMATRANSFER_PROP_ALPHA_OFFSET, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"AlphaDisable", D2D1_GAMMATRANSFER_PROP_ALPHA_DISABLE, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"ClampOutput", D2D1_GAMMATRANSFER_PROP_CLAMP_OUTPUT, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT });

    public:
        IFACEMETHODIMP GetPropertyCount(_Out_ UINT * count) override { *count = 17; return S_OK; }

        IFACEMETHODIMP GetProperty(UINT index, _Outptr_ abi::IPropertyValue ** value) override
        {
            return UsePropertyFactory(value, [=]()
            {
                switch (index)
                {
                    case D2D1_GAMMATRANSFER_PROP_RED_AMPLITUDE: return winrt::PropertyValue::CreateSingle(m_RedAmplitude);
                    case D2D1_GAMMATRANSFER_PROP_RED_EXPONENT: return winrt::PropertyValue::CreateSingle(m_RedExponent);
                    case D2D1_GAMMATRANSFER_PROP_RED_OFFSET: return winrt::PropertyValue::CreateSingle(m_RedOffset);
                    case D2D1_GAMMATRANSFER_PROP_RED_DISABLE: return winrt::PropertyValue::CreateBoolean(m_RedDisable);
                    case D2D1_GAMMATRANSFER_PROP_GREEN_AMPLITUDE: return winrt::PropertyValue::CreateSingle(m_GreenAmplitude);
                    case D2D1_GAMMATRANSFER_PROP_GREEN_EXPONENT: return winrt::PropertyValue::CreateSingle(m_GreenExponent);
                    case D2D1_GAMMATRANSFER_PROP_GREEN_OFFSET: return winrt::PropertyValue::CreateSingle(m_GreenOffset);
                    case D2D1_GAMMATRANSFER_PROP_GREEN_DISABLE: return winrt::PropertyValue::CreateBoolean(m_GreenDisable);
                    case D2D1_GAMMATRANSFER_PROP_BLUE_AMPLITUDE: return winrt::PropertyValue::CreateSingle(m_BlueAmplitude);
                    case D2D1_GAMMATRANSFER_PROP_BLUE_EXPONENT: return winrt::PropertyValue::CreateSingle(m_BlueExponent);
                    case D2D1_GAMMATRANSFER_PROP_BLUE_OFFSET: return winrt::PropertyValue::CreateSingle(m_BlueOffset);
                    case D2D1_GAMMATRANSFER_PROP_BLUE_DISABLE: return winrt::PropertyValue::CreateBoolean(m_BlueDisable);
                    case D2D1_GAMMATRANSFER_PROP_ALPHA_AMPLITUDE: return winrt::PropertyValue::CreateSingle(m_AlphaAmplitude);
                    case D2D1_GAMMATRANSFER_PROP_ALPHA_EXPONENT: return winrt::PropertyValue::CreateSingle(m_AlphaExponent);
                    case D2D1_GAMMATRANSFER_PROP_ALPHA_OFFSET: return winrt::PropertyValue::CreateSingle(m_AlphaOffset);
                    case D2D1_GAMMATRANSFER_PROP_ALPHA_DISABLE: return winrt::PropertyValue::CreateBoolean(m_AlphaDisable);
                    case D2D1_GAMMATRANSFER_PROP_CLAMP_OUTPUT: return winrt::PropertyValue::CreateBoolean(m_ClampOutput);
                    default: throw winrt::hresult_invalid_argument();
                }
            });
        }
    };

    //-----------------------------------------------------------------------------------------------------------------
    class GaussianBlurEffect : 
        public winrt::Microsoft::UI::Composition::Effects::implementation::GaussianBlurEffectT<GaussianBlurEffect, EffectBase>
    {
    public:
        DECLARE_D2D_GUID(CLSID_D2D1GaussianBlur);
        DECLARE_SINGLE_SOURCE(Source);
        DECLARE_POD_PROPERTY(BlurAmount, float, 3.0f, value >= 0.0f && value <= 250.0f);
        DECLARE_POD_PROPERTY(Optimization, winrt::EffectOptimization, winrt::EffectOptimization::Balanced, true);
        DECLARE_POD_PROPERTY(BorderMode, winrt::EffectBorderMode, winrt::EffectBorderMode::Soft, true);
        DECLARE_NAMED_PROPERTY_MAPPING(
            { L"BlurAmount", D2D1_GAUSSIANBLUR_PROP_STANDARD_DEVIATION, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"Optimization", D2D1_GAUSSIANBLUR_PROP_OPTIMIZATION, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"BorderMode", D2D1_GAUSSIANBLUR_PROP_BORDER_MODE, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT });

    public:
        IFACEMETHODIMP GetPropertyCount(_Out_ UINT * count) override { *count = 3; return S_OK; }

        IFACEMETHODIMP GetProperty(UINT index, _Outptr_ abi::IPropertyValue ** value) override
        {
            return UsePropertyFactory(value, [=]()
            {
                switch (index)
                {
                    case D2D1_GAUSSIANBLUR_PROP_STANDARD_DEVIATION: return winrt::PropertyValue::CreateSingle(m_BlurAmount);
                    case D2D1_GAUSSIANBLUR_PROP_OPTIMIZATION: return winrt::PropertyValue::CreateUInt32((UINT32)m_Optimization);
                    case D2D1_GAUSSIANBLUR_PROP_BORDER_MODE: return winrt::PropertyValue::CreateUInt32((UINT32)m_BorderMode);
                    default: throw winrt::hresult_invalid_argument();
                }
            });
        }
    };

    //-----------------------------------------------------------------------------------------------------------------

    class GrayscaleEffect :
        public winrt::Microsoft::UI::Composition::Effects::implementation::GrayscaleEffectT<GrayscaleEffect, EffectBase>
    {
    public:
        DECLARE_D2D_GUID(CLSID_D2D1Grayscale);
        DECLARE_SINGLE_SOURCE(Source);
    };

    //-----------------------------------------------------------------------------------------------------------------

    class HueRotationEffect :
        public winrt::Microsoft::UI::Composition::Effects::implementation::HueRotationEffectT<HueRotationEffect, EffectBase>
    {
    public:
        DECLARE_D2D_GUID(CLSID_D2D1HueRotation);
        DECLARE_SINGLE_SOURCE(Source);
        DECLARE_POD_PROPERTY(Angle, float, 0.0f, true);  // D2D clamps within [0, 360] degrees
        DECLARE_NAMED_PROPERTY_MAPPING(
            { L"Angle", D2D1_HUEROTATION_PROP_ANGLE, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_RADIANS_TO_DEGREES });

    public:
        IFACEMETHODIMP GetPropertyCount(_Out_ UINT * count) override { *count = 1; return S_OK; }

        IFACEMETHODIMP GetProperty(UINT index, _Outptr_ abi::IPropertyValue ** value) override
        {
            return UsePropertyFactory(value, [=]()
            {
                switch (index)
                {
                    case D2D1_HUEROTATION_PROP_ANGLE:
                        return winrt::PropertyValue::CreateSingle(m_Angle * k_DegreesPerRadian);
                    default: throw winrt::hresult_invalid_argument();
                }
            });
        }
    };

    //-----------------------------------------------------------------------------------------------------------------

    class InvertEffect :
        public winrt::Microsoft::UI::Composition::Effects::implementation::InvertEffectT<InvertEffect, EffectBase>
    {
    public:
        DECLARE_D2D_GUID(CLSID_D2D1Invert);
        DECLARE_SINGLE_SOURCE(Source);
    };

    //-----------------------------------------------------------------------------------------------------------------

    class LinearTransferEffect :
        public winrt::Microsoft::UI::Composition::Effects::implementation::LinearTransferEffectT<LinearTransferEffect, EffectBase>
    {
    public:
        DECLARE_D2D_GUID(CLSID_D2D1LinearTransfer);
        DECLARE_SINGLE_SOURCE(Source);
        DECLARE_POD_PROPERTY(RedOffset, float, 0.0f, true);
        DECLARE_POD_PROPERTY(RedSlope, float, 1.0f, true);
        DECLARE_POD_PROPERTY(RedDisable, bool, false, true);
        DECLARE_POD_PROPERTY(GreenOffset, float, 0.0f, true);
        DECLARE_POD_PROPERTY(GreenSlope, float, 1.0f, true);
        DECLARE_POD_PROPERTY(GreenDisable, bool, false, true);
        DECLARE_POD_PROPERTY(BlueOffset, float, 0.0f, true);
        DECLARE_POD_PROPERTY(BlueSlope, float, 1.0f, true);
        DECLARE_POD_PROPERTY(BlueDisable, bool, false, true);
        DECLARE_POD_PROPERTY(AlphaOffset, float, 0.0f, true);
        DECLARE_POD_PROPERTY(AlphaSlope, float, 1.0f, true);
        DECLARE_POD_PROPERTY(AlphaDisable, bool, false, true);
        DECLARE_POD_PROPERTY(ClampOutput, bool, false, true);
        DECLARE_NAMED_PROPERTY_MAPPING(
            { L"RedOffset", D2D1_LINEARTRANSFER_PROP_RED_Y_INTERCEPT, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"RedSlope", D2D1_LINEARTRANSFER_PROP_RED_SLOPE, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"RedDisable", D2D1_LINEARTRANSFER_PROP_RED_DISABLE, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"GreenOffset", D2D1_LINEARTRANSFER_PROP_GREEN_Y_INTERCEPT, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"GreenSlope", D2D1_LINEARTRANSFER_PROP_GREEN_SLOPE, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"GreenDisable", D2D1_LINEARTRANSFER_PROP_GREEN_DISABLE, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"BlueOffset", D2D1_LINEARTRANSFER_PROP_BLUE_Y_INTERCEPT, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"BlueSlope", D2D1_LINEARTRANSFER_PROP_BLUE_SLOPE, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"BlueDisable", D2D1_LINEARTRANSFER_PROP_BLUE_DISABLE, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"AlphaOffset", D2D1_LINEARTRANSFER_PROP_ALPHA_Y_INTERCEPT, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"AlphaSlope", D2D1_LINEARTRANSFER_PROP_ALPHA_SLOPE, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"AlphaDisable", D2D1_LINEARTRANSFER_PROP_ALPHA_DISABLE, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"ClampOutput", D2D1_LINEARTRANSFER_PROP_CLAMP_OUTPUT, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT } );

    public:
        IFACEMETHODIMP GetPropertyCount(_Out_ UINT * count) override { *count = 13; return S_OK; }

        IFACEMETHODIMP GetProperty(UINT index, _Outptr_ abi::IPropertyValue ** value) override
        {
            return UsePropertyFactory(value, [=]()
            {
                switch (index)
                {
                    case D2D1_LINEARTRANSFER_PROP_RED_Y_INTERCEPT: return winrt::PropertyValue::CreateSingle(m_RedOffset);
                    case D2D1_LINEARTRANSFER_PROP_RED_SLOPE: return winrt::PropertyValue::CreateSingle(m_RedSlope);
                    case D2D1_LINEARTRANSFER_PROP_RED_DISABLE: return winrt::PropertyValue::CreateBoolean(m_RedDisable);
                    case D2D1_LINEARTRANSFER_PROP_GREEN_Y_INTERCEPT: return winrt::PropertyValue::CreateSingle(m_GreenOffset);
                    case D2D1_LINEARTRANSFER_PROP_GREEN_SLOPE: return winrt::PropertyValue::CreateSingle(m_GreenSlope);
                    case D2D1_LINEARTRANSFER_PROP_GREEN_DISABLE: return winrt::PropertyValue::CreateBoolean(m_GreenDisable);
                    case D2D1_LINEARTRANSFER_PROP_BLUE_Y_INTERCEPT: return winrt::PropertyValue::CreateSingle(m_BlueOffset);
                    case D2D1_LINEARTRANSFER_PROP_BLUE_SLOPE: return winrt::PropertyValue::CreateSingle(m_BlueSlope);
                    case D2D1_LINEARTRANSFER_PROP_BLUE_DISABLE: return winrt::PropertyValue::CreateBoolean(m_BlueDisable);
                    case D2D1_LINEARTRANSFER_PROP_ALPHA_Y_INTERCEPT: return winrt::PropertyValue::CreateSingle(m_AlphaOffset);
                    case D2D1_LINEARTRANSFER_PROP_ALPHA_SLOPE: return winrt::PropertyValue::CreateSingle(m_AlphaSlope);
                    case D2D1_LINEARTRANSFER_PROP_ALPHA_DISABLE: return winrt::PropertyValue::CreateBoolean(m_AlphaDisable);
                    case D2D1_LINEARTRANSFER_PROP_CLAMP_OUTPUT: return winrt::PropertyValue::CreateBoolean(m_ClampOutput);
                    default: throw winrt::hresult_invalid_argument();
                }
            });
        }
    };

    //-----------------------------------------------------------------------------------------------------------------

    class LuminanceToAlphaEffect :
        public winrt::Microsoft::UI::Composition::Effects::implementation::LuminanceToAlphaEffectT<LuminanceToAlphaEffect, EffectBase>
    {
    public:
        DECLARE_D2D_GUID(CLSID_D2D1LuminanceToAlpha);
        DECLARE_SINGLE_SOURCE(Source);
    };

    //-----------------------------------------------------------------------------------------------------------------

    class OpacityEffect :
        public winrt::Microsoft::UI::Composition::Effects::implementation::OpacityEffectT<OpacityEffect, EffectBase>
    {
    public:
        DECLARE_D2D_GUID(CLSID_D2D1Opacity);
        DECLARE_SINGLE_SOURCE(Source);
        DECLARE_POD_PROPERTY(Opacity, float, 1.0f, value >= 0.0f && value <= 1.0f);
        DECLARE_NAMED_PROPERTY_MAPPING(
            { L"Opacity", D2D1_OPACITY_PROP_OPACITY, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT });

    public:
        IFACEMETHODIMP GetPropertyCount(_Out_ UINT * count) override { *count = 1; return S_OK; }

        IFACEMETHODIMP GetProperty(UINT index, _Outptr_ abi::IPropertyValue ** value) override
        {
            return UsePropertyFactory(value, [=]()
            {
                switch (index)
                {
                    case D2D1_OPACITY_PROP_OPACITY: return winrt::PropertyValue::CreateSingle(m_Opacity);
                    default: throw winrt::hresult_invalid_argument();
                }
            });
        }
    };

    //-----------------------------------------------------------------------------------------------------------------

    class PointDiffuseEffect :
        public winrt::Microsoft::UI::Composition::Effects::implementation::PointDiffuseEffectT<PointDiffuseEffect, EffectBase>
    {
    public:
        DECLARE_D2D_GUID(CLSID_D2D1PointDiffuse);
        DECLARE_SINGLE_SOURCE(Source);
        DECLARE_POD_PROPERTY(LightPosition, Vector3, (Vector3{ 0.0f, 0.0f, 0.0f }), true);
        DECLARE_POD_PROPERTY(DiffuseAmount, float, 1.0f, value >= 0.0f && value <= 10000.0f);
        DECLARE_POD_PROPERTY(HeightMapScale, float, 1.0f, value >= -10000.0f && value <= 10000.0f);
        DECLARE_POD_PROPERTY(LightColor, UIColor, (UIColor{ 255, 255, 255, 255 }), true);
        DECLARE_POD_PROPERTY(HeightMapKernelSize, Vector2, (Vector2{ 1.0f, 1.0f }),
            value.x >= 0.01f && value.y >= 0.01f && value.x <= 100.0f && value.y <= 100.0f);
        DECLARE_POD_PROPERTY(HeightMapInterpolationMode, winrt::CanvasImageInterpolation, winrt::CanvasImageInterpolation::Linear, true);
        DECLARE_NAMED_PROPERTY_MAPPING(
            { L"LightPosition", D2D1_POINTDIFFUSE_PROP_LIGHT_POSITION, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"DiffuseAmount", D2D1_POINTDIFFUSE_PROP_DIFFUSE_CONSTANT, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"HeightMapScale", D2D1_POINTDIFFUSE_PROP_SURFACE_SCALE, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"LightColor", D2D1_POINTDIFFUSE_PROP_COLOR, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_COLOR_TO_VECTOR3 },
            { L"HeightMapKernelSize", D2D1_POINTDIFFUSE_PROP_KERNEL_UNIT_LENGTH, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"HeightMapInterpolationMode", D2D1_POINTDIFFUSE_PROP_SCALE_MODE, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },);

    public:
        IFACEMETHODIMP GetPropertyCount(_Out_ UINT * count) override { *count = 6; return S_OK; }

        IFACEMETHODIMP GetProperty(UINT index, _Outptr_ abi::IPropertyValue ** value) override
        {
            return UsePropertyFactory(value, [=]()
            {
                switch (index)
                {
                    case D2D1_POINTDIFFUSE_PROP_LIGHT_POSITION:
                        return winrt::PropertyValue::CreateSingleArray(reinterpret_cast<std::array<float, 3>&>(m_LightPosition));
                    case D2D1_POINTDIFFUSE_PROP_DIFFUSE_CONSTANT: return winrt::PropertyValue::CreateSingle(m_DiffuseAmount);
                    case D2D1_POINTDIFFUSE_PROP_SURFACE_SCALE: return winrt::PropertyValue::CreateSingle(m_HeightMapScale);
                    case D2D1_POINTDIFFUSE_PROP_COLOR: return CreateColor<3>(m_LightColor);
                    case D2D1_POINTDIFFUSE_PROP_KERNEL_UNIT_LENGTH:
                        return winrt::PropertyValue::CreateSingleArray(reinterpret_cast<std::array<float, 2>&>(m_HeightMapKernelSize));
                    case D2D1_POINTDIFFUSE_PROP_SCALE_MODE: return winrt::PropertyValue::CreateUInt32((UINT32)m_HeightMapInterpolationMode);
                    default: throw winrt::hresult_invalid_argument();
                }
            });
        }
    };

    //-----------------------------------------------------------------------------------------------------------------

    class PointSpecularEffect :
        public winrt::Microsoft::UI::Composition::Effects::implementation::PointSpecularEffectT<PointSpecularEffect, EffectBase>
    {
    public:
        DECLARE_D2D_GUID(CLSID_D2D1PointSpecular);
        DECLARE_SINGLE_SOURCE(Source);
        DECLARE_POD_PROPERTY(LightPosition, Vector3, (Vector3{ 0.0f, 0.0f, 0.0f }), true);
        DECLARE_POD_PROPERTY(SpecularExponent, float, 1.0f, value >= -10000.0f && value <= 10000.0f);
        DECLARE_POD_PROPERTY(SpecularAmount, float, 1.0f, value >= 0.0f && value <= 10000.0f);
        DECLARE_POD_PROPERTY(HeightMapScale, float, 1.0f, value >= -10000.0f && value <= 10000.0f);
        DECLARE_POD_PROPERTY(LightColor, UIColor, (UIColor{ 255, 255, 255, 255 }), true);
        DECLARE_POD_PROPERTY(HeightMapKernelSize, Vector2, (Vector2{ 1.0f, 1.0f }),
            value.x >= 0.01f && value.y >= 0.01f && value.x <= 100.0f && value.y <= 100.0f);
        DECLARE_POD_PROPERTY(HeightMapInterpolationMode, winrt::CanvasImageInterpolation, winrt::CanvasImageInterpolation::Linear, true);
        DECLARE_NAMED_PROPERTY_MAPPING(
            { L"LightPosition", D2D1_POINTDIFFUSE_PROP_LIGHT_POSITION, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"SpecularExponent", D2D1_POINTSPECULAR_PROP_SPECULAR_EXPONENT, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"SpecularAmount", D2D1_POINTSPECULAR_PROP_SPECULAR_CONSTANT, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"HeightMapScale", D2D1_POINTSPECULAR_PROP_SURFACE_SCALE, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"LightColor", D2D1_POINTSPECULAR_PROP_COLOR, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_COLOR_TO_VECTOR3 },
            { L"HeightMapKernelSize", D2D1_POINTSPECULAR_PROP_KERNEL_UNIT_LENGTH, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"HeightMapInterpolationMode", D2D1_POINTSPECULAR_PROP_SCALE_MODE, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },);

    public:
        IFACEMETHODIMP GetPropertyCount(_Out_ UINT * count) override { *count = 7; return S_OK; }

        IFACEMETHODIMP GetProperty(UINT index, _Outptr_ abi::IPropertyValue ** value) override
        {
            return UsePropertyFactory(value, [=]()
            {
                switch (index)
                {
                    case D2D1_POINTDIFFUSE_PROP_LIGHT_POSITION:
                        return winrt::PropertyValue::CreateSingleArray(reinterpret_cast<std::array<float, 3>&>(m_LightPosition));
                    case D2D1_POINTSPECULAR_PROP_SPECULAR_EXPONENT: return winrt::PropertyValue::CreateSingle(m_SpecularExponent);
                    case D2D1_POINTSPECULAR_PROP_SPECULAR_CONSTANT: return winrt::PropertyValue::CreateSingle(m_SpecularAmount);
                    case D2D1_POINTSPECULAR_PROP_SURFACE_SCALE: return winrt::PropertyValue::CreateSingle(m_HeightMapScale);
                    case D2D1_POINTSPECULAR_PROP_COLOR: return CreateColor<3>(m_LightColor);
                    case D2D1_POINTSPECULAR_PROP_KERNEL_UNIT_LENGTH:
                        return winrt::PropertyValue::CreateSingleArray(reinterpret_cast<std::array<float, 2>&>(m_HeightMapKernelSize));
                    case D2D1_POINTSPECULAR_PROP_SCALE_MODE: return winrt::PropertyValue::CreateUInt32((UINT32)m_HeightMapInterpolationMode);
                    default: throw winrt::hresult_invalid_argument();
                }
            });
        }
    };

    //-----------------------------------------------------------------------------------------------------------------

    class PosterizeEffect :
        public winrt::Microsoft::UI::Composition::Effects::implementation::PosterizeEffectT<PosterizeEffect, EffectBase>
    {
    public:
        DECLARE_D2D_GUID(CLSID_D2D1Posterize);
        DECLARE_SINGLE_SOURCE(Source);
        DECLARE_POD_PROPERTY(RedValueCount, int, 4, value >= 2 && value <= 16);
        DECLARE_POD_PROPERTY(GreenValueCount, int, 4, value >= 2 && value <= 16);
        DECLARE_POD_PROPERTY(BlueValueCount, int, 4, value >= 2 && value <= 16);
        DECLARE_NAMED_PROPERTY_MAPPING(
            { L"RedValueCount", D2D1_POSTERIZE_PROP_RED_VALUE_COUNT, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"GreenValueCount", D2D1_POSTERIZE_PROP_GREEN_VALUE_COUNT, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"BlueValueCount", D2D1_POSTERIZE_PROP_BLUE_VALUE_COUNT, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT });

    public:
        IFACEMETHODIMP GetPropertyCount(_Out_ UINT * count) override { *count = 1; return S_OK; }

        IFACEMETHODIMP GetProperty(UINT index, _Outptr_ abi::IPropertyValue ** value) override
        {
            return UsePropertyFactory(value, [=]()
            {
                switch (index)
                {
                    case D2D1_POSTERIZE_PROP_RED_VALUE_COUNT: return winrt::PropertyValue::CreateInt32(m_RedValueCount);
                    case D2D1_POSTERIZE_PROP_GREEN_VALUE_COUNT: return winrt::PropertyValue::CreateInt32(m_GreenValueCount);
                    case D2D1_POSTERIZE_PROP_BLUE_VALUE_COUNT: return winrt::PropertyValue::CreateInt32(m_BlueValueCount);
                    default: throw winrt::hresult_invalid_argument();
                }
            });
        }
    };

    //-----------------------------------------------------------------------------------------------------------------

    class PremultiplyEffect :
        public winrt::Microsoft::UI::Composition::Effects::implementation::PremultiplyEffectT<PremultiplyEffect, EffectBase>
    {
    public:
        DECLARE_D2D_GUID(CLSID_D2D1Premultiply);
        DECLARE_SINGLE_SOURCE(Source);
    };

    //-----------------------------------------------------------------------------------------------------------------

    class SaturationEffect :
        public winrt::Microsoft::UI::Composition::Effects::implementation::SaturationEffectT<SaturationEffect, EffectBase>
    {
    public:
        DECLARE_D2D_GUID(CLSID_D2D1Saturation);
        DECLARE_SINGLE_SOURCE(Source);
        DECLARE_POD_PROPERTY(Saturation, float, 0.5f, value >= 0.0f && value <= 2.0f);
        DECLARE_NAMED_PROPERTY_MAPPING(
            { L"Saturation", D2D1_SATURATION_PROP_SATURATION, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT });

    public:
        IFACEMETHODIMP GetPropertyCount(_Out_ UINT * count) override { *count = 1; return S_OK; }

        IFACEMETHODIMP GetProperty(UINT index, _Outptr_ abi::IPropertyValue ** value) override
        {
            return UsePropertyFactory(value, [=]()
            {
                switch (index)
                {
                    case D2D1_SATURATION_PROP_SATURATION: return winrt::PropertyValue::CreateSingle(m_Saturation);
                    default: throw winrt::hresult_invalid_argument();
                }
            });
        }
    };

    //-----------------------------------------------------------------------------------------------------------------

    class SepiaEffect :
        public winrt::Microsoft::UI::Composition::Effects::implementation::SepiaEffectT<SepiaEffect, EffectBase>
    {
    public:
        DECLARE_D2D_GUID(CLSID_D2D1Sepia);
        DECLARE_SINGLE_SOURCE(Source);
        DECLARE_POD_PROPERTY(Intensity, float, 0.5f, value >= 0.0f && value <= 1.0f);
        DECLARE_POD_PROPERTY(AlphaMode, winrt::CanvasAlphaMode, winrt::CanvasAlphaMode::Premultiplied, value != winrt::CanvasAlphaMode::Ignore);
        DECLARE_NAMED_PROPERTY_MAPPING(
            { L"Intensity", D2D1_SEPIA_PROP_INTENSITY, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"AlphaMode", D2D1_SEPIA_PROP_ALPHA_MODE, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_COLORMATRIX_ALPHA_MODE });

    public:
        IFACEMETHODIMP GetPropertyCount(_Out_ UINT * count) override { *count = 2; return S_OK; }

        IFACEMETHODIMP GetProperty(UINT index, _Outptr_ abi::IPropertyValue ** value) override
        {
            return UsePropertyFactory(value, [=]()
            {
                switch (index)
                {
                    case D2D1_SEPIA_PROP_INTENSITY: return winrt::PropertyValue::CreateSingle(m_Intensity);
                    case D2D1_SEPIA_PROP_ALPHA_MODE:
                    {
                        switch (m_AlphaMode)
                        {
                            case winrt::CanvasAlphaMode::Premultiplied: 
                                return winrt::PropertyValue::CreateUInt32(D2D1_COLORMANAGEMENT_ALPHA_MODE_PREMULTIPLIED);
                            case winrt::CanvasAlphaMode::Straight:
                                return winrt::PropertyValue::CreateUInt32(D2D1_COLORMANAGEMENT_ALPHA_MODE_STRAIGHT);
                        }
                        break;
                    }
                }
                throw winrt::hresult_invalid_argument();
            });
        }
    };

    //-----------------------------------------------------------------------------------------------------------------

    class SpotDiffuseEffect :
        public winrt::Microsoft::UI::Composition::Effects::implementation::SpotDiffuseEffectT<SpotDiffuseEffect, EffectBase>
    {
    public:
        DECLARE_D2D_GUID(CLSID_D2D1SpotDiffuse);
        DECLARE_SINGLE_SOURCE(Source);
        DECLARE_POD_PROPERTY(LightPosition, Vector3, (Vector3{ 0.0f, 0.0f, 0.0f }), true);
        DECLARE_POD_PROPERTY(LightTarget, Vector3, (Vector3{ 0.0f, 0.0f, 0.0f }), true);
        DECLARE_POD_PROPERTY(Focus, float, 1.0f, value >= -10000.0f && value <= 10000.0f);
        DECLARE_POD_PROPERTY(LimitingConeAngle, float, k_PI / 2.0f, true); // D2D clamps within [-90, 90] degrees
        DECLARE_POD_PROPERTY(DiffuseAmount, float, 1.0f, value >= 0.0f && value <= 10000.0f);
        DECLARE_POD_PROPERTY(HeightMapScale, float, 1.0f, value >= -10000.0f && value <= 10000.0f);
        DECLARE_POD_PROPERTY(LightColor, UIColor, (UIColor{ 255, 255, 255, 255 }), true);
        DECLARE_POD_PROPERTY(HeightMapKernelSize, Vector2, (Vector2{ 1.0f, 1.0f }),
            value.x >= 0.01f && value.y >= 0.01f && value.x <= 100.0f && value.y <= 100.0f);
        DECLARE_POD_PROPERTY(HeightMapInterpolationMode, winrt::CanvasImageInterpolation, winrt::CanvasImageInterpolation::Linear, true);
        DECLARE_NAMED_PROPERTY_MAPPING(
            { L"LightPosition", D2D1_SPOTDIFFUSE_PROP_LIGHT_POSITION, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"LightTarget", D2D1_SPOTDIFFUSE_PROP_POINTS_AT, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"Focus", D2D1_SPOTDIFFUSE_PROP_FOCUS, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"LimitingConeAngle", D2D1_SPOTDIFFUSE_PROP_LIMITING_CONE_ANGLE, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_RADIANS_TO_DEGREES },
            { L"DiffuseAmount", D2D1_SPOTDIFFUSE_PROP_DIFFUSE_CONSTANT, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"HeightMapScale", D2D1_SPOTDIFFUSE_PROP_SURFACE_SCALE, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"LightColor", D2D1_SPOTDIFFUSE_PROP_COLOR, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_COLOR_TO_VECTOR3 },
            { L"HeightMapKernelSize", D2D1_SPOTDIFFUSE_PROP_KERNEL_UNIT_LENGTH, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"HeightMapInterpolationMode", D2D1_SPOTDIFFUSE_PROP_SCALE_MODE, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },);

    public:
        IFACEMETHODIMP GetPropertyCount(_Out_ UINT * count) override { *count = 9; return S_OK; }

        IFACEMETHODIMP GetProperty(UINT index, _Outptr_ abi::IPropertyValue ** value) override
        {
            return UsePropertyFactory(value, [=]()
            {
                switch (index)
                {
                    case D2D1_SPOTDIFFUSE_PROP_LIGHT_POSITION:
                        return winrt::PropertyValue::CreateSingleArray(reinterpret_cast<std::array<float, 3>&>(m_LightPosition));
                    case D2D1_SPOTDIFFUSE_PROP_POINTS_AT:
                        return winrt::PropertyValue::CreateSingleArray(reinterpret_cast<std::array<float, 3>&>(m_LightTarget));
                    case D2D1_SPOTDIFFUSE_PROP_FOCUS: return winrt::PropertyValue::CreateSingle(m_Focus);
                    case D2D1_SPOTDIFFUSE_PROP_LIMITING_CONE_ANGLE:
                        return winrt::PropertyValue::CreateSingle(m_LimitingConeAngle * k_DegreesPerRadian);
                    case D2D1_SPOTDIFFUSE_PROP_DIFFUSE_CONSTANT: return winrt::PropertyValue::CreateSingle(m_DiffuseAmount);
                    case D2D1_SPOTDIFFUSE_PROP_SURFACE_SCALE: return winrt::PropertyValue::CreateSingle(m_HeightMapScale);
                    case D2D1_SPOTDIFFUSE_PROP_COLOR: return CreateColor<3>(m_LightColor);
                    case D2D1_SPOTDIFFUSE_PROP_KERNEL_UNIT_LENGTH:
                        return winrt::PropertyValue::CreateSingleArray(reinterpret_cast<std::array<float, 2>&>(m_HeightMapKernelSize));
                    case D2D1_SPOTDIFFUSE_PROP_SCALE_MODE: return winrt::PropertyValue::CreateUInt32((UINT32)m_HeightMapInterpolationMode);
                    default: throw winrt::hresult_invalid_argument();
                }
            });
        }
    };

    //-----------------------------------------------------------------------------------------------------------------

    class SpotSpecularEffect :
        public winrt::Microsoft::UI::Composition::Effects::implementation::SpotSpecularEffectT<SpotSpecularEffect, EffectBase>
    {
    public:
        DECLARE_D2D_GUID(CLSID_D2D1SpotSpecular);
        DECLARE_SINGLE_SOURCE(Source);
        DECLARE_POD_PROPERTY(LightPosition, Vector3, (Vector3{ 0.0f, 0.0f, 0.0f }), true);
        DECLARE_POD_PROPERTY(LightTarget, Vector3, (Vector3{ 0.0f, 0.0f, 0.0f }), true);
        DECLARE_POD_PROPERTY(Focus, float, 1.0f, value >= -10000.0f && value <= 10000.0f);
        DECLARE_POD_PROPERTY(LimitingConeAngle, float, k_PI / 2.0f, true); // D2D clamps within [-90, 90] degrees
        DECLARE_POD_PROPERTY(SpecularExponent, float, 1.0f, value >= -10000.0f && value <= 10000.0f);
        DECLARE_POD_PROPERTY(SpecularAmount, float, 1.0f, value >= 0.0f && value <= 10000.0f);
        DECLARE_POD_PROPERTY(HeightMapScale, float, 1.0f, value >= -10000.0f && value <= 10000.0f);
        DECLARE_POD_PROPERTY(LightColor, UIColor, (UIColor{ 255, 255, 255, 255 }), true);
        DECLARE_POD_PROPERTY(HeightMapKernelSize, Vector2, (Vector2{ 1.0f, 1.0f }),
            value.x >= 0.01f && value.y >= 0.01f && value.x <= 100.0f && value.y <= 100.0f);
        DECLARE_POD_PROPERTY(HeightMapInterpolationMode, winrt::CanvasImageInterpolation, winrt::CanvasImageInterpolation::Linear, true);
        DECLARE_NAMED_PROPERTY_MAPPING(
            { L"LightPosition", D2D1_SPOTDIFFUSE_PROP_LIGHT_POSITION, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"LightTarget", D2D1_SPOTDIFFUSE_PROP_POINTS_AT, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"Focus", D2D1_SPOTDIFFUSE_PROP_FOCUS, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"LimitingConeAngle", D2D1_SPOTDIFFUSE_PROP_LIMITING_CONE_ANGLE, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_RADIANS_TO_DEGREES },
            { L"SpecularExponent", D2D1_SPOTSPECULAR_PROP_SPECULAR_EXPONENT, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"SpecularAmount", D2D1_SPOTSPECULAR_PROP_SPECULAR_CONSTANT, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"HeightMapScale", D2D1_SPOTSPECULAR_PROP_SURFACE_SCALE, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"LightColor", D2D1_SPOTSPECULAR_PROP_COLOR, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_COLOR_TO_VECTOR3 },
            { L"HeightMapKernelSize", D2D1_SPOTSPECULAR_PROP_KERNEL_UNIT_LENGTH, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"HeightMapInterpolationMode", D2D1_SPOTSPECULAR_PROP_SCALE_MODE, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },);

    public:
        IFACEMETHODIMP GetPropertyCount(_Out_ UINT * count) override { *count = 10; return S_OK; }

        IFACEMETHODIMP GetProperty(UINT index, _Outptr_ abi::IPropertyValue ** value) override
        {
            return UsePropertyFactory(value, [=]()
            {
                switch (index)
                {
                    case D2D1_SPOTDIFFUSE_PROP_LIGHT_POSITION:
                        return winrt::PropertyValue::CreateSingleArray(reinterpret_cast<std::array<float, 3>&>(m_LightPosition));
                    case D2D1_SPOTDIFFUSE_PROP_POINTS_AT:
                        return winrt::PropertyValue::CreateSingleArray(reinterpret_cast<std::array<float, 3>&>(m_LightTarget));
                    case D2D1_SPOTDIFFUSE_PROP_FOCUS: return winrt::PropertyValue::CreateSingle(m_Focus);
                    case D2D1_SPOTDIFFUSE_PROP_LIMITING_CONE_ANGLE:
                        return winrt::PropertyValue::CreateSingle(m_LimitingConeAngle * k_DegreesPerRadian);
                    case D2D1_SPOTSPECULAR_PROP_SPECULAR_EXPONENT: return winrt::PropertyValue::CreateSingle(m_SpecularExponent);
                    case D2D1_SPOTSPECULAR_PROP_SPECULAR_CONSTANT: return winrt::PropertyValue::CreateSingle(m_SpecularAmount);
                    case D2D1_SPOTSPECULAR_PROP_SURFACE_SCALE: return winrt::PropertyValue::CreateSingle(m_HeightMapScale);
                    case D2D1_SPOTSPECULAR_PROP_COLOR: return CreateColor<3>(m_LightColor);
                    case D2D1_SPOTSPECULAR_PROP_KERNEL_UNIT_LENGTH:
                        return winrt::PropertyValue::CreateSingleArray(reinterpret_cast<std::array<float, 2>&>(m_HeightMapKernelSize));
                    case D2D1_SPOTSPECULAR_PROP_SCALE_MODE: return winrt::PropertyValue::CreateUInt32((UINT32)m_HeightMapInterpolationMode);
                    default: throw winrt::hresult_invalid_argument();
                }
            });
        }
    };

    //-----------------------------------------------------------------------------------------------------------------

    class TemperatureAndTintEffect :
        public winrt::Microsoft::UI::Composition::Effects::implementation::TemperatureAndTintEffectT<TemperatureAndTintEffect, EffectBase>
    {
    public:
        DECLARE_D2D_GUID(CLSID_D2D1TemperatureTint);
        DECLARE_SINGLE_SOURCE(Source);
        DECLARE_POD_PROPERTY(Temperature, float, 0.0f, value >= -1.0f && value <= 1.0f);
        DECLARE_POD_PROPERTY(Tint, float, 0.0f, value >= -1.0f && value <= 1.0f);
        DECLARE_NAMED_PROPERTY_MAPPING(
            { L"Temperature", D2D1_TEMPERATUREANDTINT_PROP_TEMPERATURE, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"Tint", D2D1_TEMPERATUREANDTINT_PROP_TINT, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT });

    public:
        IFACEMETHODIMP GetPropertyCount(_Out_ UINT * count) override { *count = 2; return S_OK; }

        IFACEMETHODIMP GetProperty(UINT index, _Outptr_ abi::IPropertyValue ** value) override
        {
            return UsePropertyFactory(value, [=]()
            {
                switch (index)
                {
                    case D2D1_TEMPERATUREANDTINT_PROP_TEMPERATURE: return winrt::PropertyValue::CreateSingle(m_Temperature);
                    case D2D1_TEMPERATUREANDTINT_PROP_TINT: return winrt::PropertyValue::CreateSingle(m_Tint);
                    default: throw winrt::hresult_invalid_argument();
                }
            });
        }
    };

    //-----------------------------------------------------------------------------------------------------------------

    class TintEffect :
        public winrt::Microsoft::UI::Composition::Effects::implementation::TintEffectT<TintEffect, EffectBase>
    {
    public:
        DECLARE_D2D_GUID(CLSID_D2D1Tint);
        DECLARE_SINGLE_SOURCE(Source);
        DECLARE_POD_PROPERTY(Color, UIColor, (UIColor{ 255, 255, 255, 255 }), true);
        DECLARE_POD_PROPERTY(ClampOutput, bool, false, true);
        DECLARE_NAMED_PROPERTY_MAPPING(
            { L"Color", D2D1_TINT_PROP_COLOR, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_COLOR_TO_VECTOR4 },
            { L"ClampOutput", D2D1_TINT_PROP_CLAMP_OUTPUT, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT });

    public:
        IFACEMETHODIMP GetPropertyCount(_Out_ UINT * count) override { *count = 2; return S_OK; }

        IFACEMETHODIMP GetProperty(UINT index, _Outptr_ abi::IPropertyValue ** value) override
        {
            return UsePropertyFactory(value, [=]()
            {
                switch (index)
                {
                    case D2D1_TINT_PROP_COLOR: return CreateColor<4>(m_Color);
                    case D2D1_TINT_PROP_CLAMP_OUTPUT: return winrt::PropertyValue::CreateBoolean(m_ClampOutput);
                    default: throw winrt::hresult_invalid_argument();
                }
            });
        }
    };

    //-----------------------------------------------------------------------------------------------------------------

    class Transform2DEffect :
        public winrt::Microsoft::UI::Composition::Effects::implementation::Transform2DEffectT<Transform2DEffect, EffectBase>
    {
    public:
        DECLARE_D2D_GUID(CLSID_D2D12DAffineTransform);
        DECLARE_SINGLE_SOURCE(Source);
        DECLARE_POD_PROPERTY(InterpolationMode, winrt::CanvasImageInterpolation, winrt::CanvasImageInterpolation::Linear, true);
        DECLARE_POD_PROPERTY(BorderMode, winrt::EffectBorderMode, winrt::EffectBorderMode::Soft, true);
        DECLARE_POD_PROPERTY(TransformMatrix, winrt::float3x2, (winrt::float3x2{ 1, 0, 0, 1, 0, 0}), true);
        DECLARE_POD_PROPERTY(Sharpness, float, 0.0f, value >= 0.0f && value <= 1.0f);
        DECLARE_NAMED_PROPERTY_MAPPING(
            { L"InterpolationMode", D2D1_2DAFFINETRANSFORM_PROP_INTERPOLATION_MODE, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"BorderMode", D2D1_2DAFFINETRANSFORM_PROP_BORDER_MODE, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"TransformMatrix", D2D1_2DAFFINETRANSFORM_PROP_TRANSFORM_MATRIX, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT },
            { L"Sharpness", D2D1_2DAFFINETRANSFORM_PROP_SHARPNESS, PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT });

    public:
        IFACEMETHODIMP GetPropertyCount(_Out_ UINT * count) override { *count = 4; return S_OK; }

        IFACEMETHODIMP GetProperty(UINT index, _Outptr_ abi::IPropertyValue ** value) override
        {
            return UsePropertyFactory(value, [=]()
            {
                switch (index)
                {
                    case D2D1_2DAFFINETRANSFORM_PROP_INTERPOLATION_MODE: return winrt::PropertyValue::CreateUInt32((UINT32)m_InterpolationMode);
                    case D2D1_2DAFFINETRANSFORM_PROP_BORDER_MODE: return winrt::PropertyValue::CreateUInt32((UINT32)m_BorderMode);
                    case D2D1_2DAFFINETRANSFORM_PROP_TRANSFORM_MATRIX:
                        return winrt::PropertyValue::CreateSingleArray(reinterpret_cast<std::array<float, 6>&>(m_TransformMatrix));
                    case D2D1_2DAFFINETRANSFORM_PROP_SHARPNESS: return winrt::PropertyValue::CreateSingle(m_Sharpness);
                    default: throw winrt::hresult_invalid_argument();
                }
            });
        }
    };

    //-----------------------------------------------------------------------------------------------------------------

    class UnPremultiplyEffect :
        public winrt::Microsoft::UI::Composition::Effects::implementation::UnPremultiplyEffectT<UnPremultiplyEffect, EffectBase>
    {
    public:
        DECLARE_D2D_GUID(CLSID_D2D1UnPremultiply);
        DECLARE_SINGLE_SOURCE(Source);
    };

    //-----------------------------------------------------------------------------------------------------------------
    // Clean up preprocessor state

#ifndef MICROSOFT_UI_COMPOSITION_EFFECT_IMPL_KEEP_MACROS
#    pragma pop_macro("DECLARE_D2D_GUID")
#    pragma pop_macro("DECLARE_POD_PROPERTY")
#    pragma pop_macro("DECLARE_SOURCE")
#    pragma pop_macro("DECLARE_SINGLE_SOURCE")
#    pragma pop_macro("DECLARE_DUAL_SOURCES")
#    pragma pop_macro("DECLARE_NAMED_PROPERTY_MAPPING")
#endif

}}}}
#pragma warning(pop)
