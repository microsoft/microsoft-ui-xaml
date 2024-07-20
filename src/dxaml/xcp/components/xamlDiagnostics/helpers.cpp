// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "helpers.h"
#include "wil\resource.h"
#include "runtimeEnabledFeatures\inc\RuntimeEnabledFeatures.h"
#include "dependencyLocator\inc\DependencyLocator.h"
#include "colors\inc\ColorUtil.h"
#include "xstring_ptr.h"
#include "DiagnosticsInterop.h"
#include "HandleStore.h"
#include <windows.ui.core.corewindow-defs.h>
#include <microsoft.UI.Dispatching.h>
#include <WRLHelper.h>

using namespace RuntimeFeatureBehavior;

#pragma region XamlDiagnosticsHelpers
wrl_wrappers::HString XamlDiagnosticsHelpers::_fallbackString;

#pragma region Environment Helpers

std::tuple<bool, std::wstring>
XamlDiagnosticsHelpers::TryGetEnv(
    _In_ const std::wstring& varName,
    _In_ const dt::EnvironmentMap& extraEnv)
{
    auto iter = extraEnv.find(varName);
    if (iter != extraEnv.end())
    {
        return std::make_tuple(true, iter->second);
    }

    auto varSize = GetEnvironmentVariable(varName.c_str(), nullptr, 0);
    wchar_t var [MAX_PATH];
    if ((varSize != 0) && (GetEnvironmentVariable(varName.c_str(), &var[0], ARRAYSIZE(var)) == varSize - 1) && varSize < MAX_PATH)
    {
        return std::make_tuple(true, std::wstring(var, varSize));
    }

    return std::make_tuple(false, std::wstring());

}

std::wstring
XamlDiagnosticsHelpers::GetEnv(
    _In_ const std::wstring& varName,
    _In_ const dt::EnvironmentMap& extraEnv,
    _In_ const std::wstring& defaultValue)
{
    bool success;
    std::wstring value;
    std::tie(success, value) = TryGetEnv(varName, extraEnv);
    return success ? value : defaultValue;
}

#pragma endregion

#pragma region Conversion Helpers

const wrl_wrappers::HString&
XamlDiagnosticsHelpers::GetFallbackString()
{
    if (XamlDiagnosticsHelpers::_fallbackString.IsEmpty())
    {
        XamlDiagnosticsHelpers::_fallbackString.Set(L"");
    }

    return XamlDiagnosticsHelpers::_fallbackString;
}

// Try to convert the given color to a known color name.  Returns nullptr
// if no match is found.
PCWSTR
XamlDiagnosticsHelpers::FindKnownColorName(
    _In_ const wu::Color &color)
{
    return ColorUtils::GetColorName(color);
}

// Try to convert the given FontWeight to a known FontWeight name.
// Returns nullptr if no match is found.
PCWSTR
XamlDiagnosticsHelpers::FindKnownFontWeightName(
    _In_ const wut::FontWeight &fontWeight)
{
    wut::FontWeight knownWeight = {};
    wrl::ComPtr<ABI::Microsoft::UI::Text::IFontWeightsStatics> spFontWeightsStatics;

    if (FAILED(XamlDiagnosticsHelpers::WinRTCreateInstance<ABI::Microsoft::UI::Text::IFontWeightsStatics>(L"Microsoft.UI.Text.FontWeights", &spFontWeightsStatics)))
    {
        return nullptr;
    }

    CHECKFONTWEIGHT(Black)
    CHECKFONTWEIGHT(Bold)
    CHECKFONTWEIGHT(ExtraBlack)
    CHECKFONTWEIGHT(ExtraBold)
    CHECKFONTWEIGHT(ExtraLight)
    CHECKFONTWEIGHT(Light)
    CHECKFONTWEIGHT(Medium)
    CHECKFONTWEIGHT(Normal)
    CHECKFONTWEIGHT(SemiBold)
    CHECKFONTWEIGHT(SemiLight)
    CHECKFONTWEIGHT(Thin)

    return nullptr;
}

HRESULT
XamlDiagnosticsHelpers::ValueToString(
    _In_ const ctl::ComPtr<IInspectable>& value,
    _Out_ xstring_ptr* valueString)
{
    auto spFontFamily = value.AsOrNull<xaml_media::IFontFamily>();
    if (spFontFamily)
    {
        wrl_wrappers::HString valueHstring;
        IFC_RETURN(spFontFamily->get_Source(valueHstring.GetAddressOf()));
        IFC_RETURN(xstring_ptr::CloneRuntimeStringHandle(valueHstring.Get(), valueString));
        return S_OK;
    }

    auto spUri = value.AsOrNull<wf::IUriRuntimeClass>();
    if (spUri)
    {
        wrl_wrappers::HString valueHstring;
        IFC_RETURN(spUri->get_AbsoluteUri(valueHstring.GetAddressOf()));
        IFC_RETURN(xstring_ptr::CloneRuntimeStringHandle(valueHstring.Get(), valueString));
        return S_OK;
    }

    auto propValue = value.AsOrNull<wf::IPropertyValue>();
    if (propValue)
    {
        wf::PropertyType valueType = wf::PropertyType_Empty;
        IFC_RETURN(propValue->get_Type(&valueType));
        wrl_wrappers::HString valueHstring;
        if (FAILED(XamlDiagnosticsHelpers::ConvertValueToStringOverride(propValue.Get(), valueType, valueHstring.ReleaseAndGetAddressOf())))
        {
            auto interop = Diagnostics::GetDiagnosticsInterop(true);
            if (FAILED(interop->ConvertValueToString(propValue.Get(), valueType, valueHstring.ReleaseAndGetAddressOf())))
            {
                IFC_RETURN(XamlDiagnosticsHelpers::GetFallbackString().CopyTo(valueHstring.ReleaseAndGetAddressOf()));
            }
        }
        IFC_RETURN(xstring_ptr::CloneRuntimeStringHandle(valueHstring.Get(), valueString));
        return S_OK;
    }

    InstanceHandle propertyHandle = 0;
    WCHAR szValue[256];
    szValue[0] = '\0';
    IFC_RETURN(XamlDiagnosticsShared::HandleStore::CreateHandle(value.Get(), &propertyHandle));
    IFCCHECK_RETURN(swprintf_s(szValue, ARRAYSIZE(szValue), L"%llu", propertyHandle));
    IFC_RETURN(xstring_ptr::CloneBuffer(szValue, valueString));
    return S_OK;
}


// Converts the given property value of the given type to a string.
HRESULT
XamlDiagnosticsHelpers::ConvertValueToStringOverride(
    _In_ wf::IPropertyValue* pPropertyValue,
    _In_ wf::PropertyType propertyType,
    _Out_ HSTRING *phstr)
{
    WCHAR szValue[256];
    szValue[0] = L'\0';

    *phstr = nullptr;

    switch (propertyType)
    {
        case wf::PropertyType_String:
        {
            IFC_RETURN(pPropertyValue->GetString(phstr));

            if (!*phstr)
            {
                return E_FAIL;
            }

            break;
        }

        case wf::PropertyType_Point:
        {
            wrl::ComPtr<wf::IReference<wf::Point>> spRefPoint;
            IFC_RETURN(do_query_interface<wf::IReference<wf::Point>>(pPropertyValue, &spRefPoint));

            wf::Point point = {};
            IFC_RETURN(spRefPoint->get_Value(&point));

            IFCCHECK_RETURN(swprintf_s(szValue, ARRAYSIZE(szValue), L"%g,%g", point.X, point.Y) > 0);
            break;
        }

        case wf::PropertyType_Size:
        {
            wrl::ComPtr<wf::IReference<wf::Size>> spRefSize;
            IFC_RETURN(do_query_interface<wf::IReference<wf::Size>>(pPropertyValue, &spRefSize));

            wf::Size size = {};
            IFC_RETURN(spRefSize->get_Value(&size));

            IFCCHECK_RETURN(swprintf_s(szValue, ARRAYSIZE(szValue), L"%gx%g", size.Width, size.Height) > 0);
            break;
        }

        case wf::PropertyType_Rect:
        {
            wrl::ComPtr<wf::IReference<wf::Rect>> spRefRect;
            IFC_RETURN(do_query_interface<wf::IReference<wf::Rect>>(pPropertyValue, &spRefRect));

            wf::Rect rect = {};
            IFC_RETURN(spRefRect->get_Value(&rect));

            IFCCHECK_RETURN(swprintf_s(szValue, ARRAYSIZE(szValue), L"%g,%g %gx%g", rect.X, rect.Y, rect.Width, rect.Height) > 0);
            break;
        }

        case wf::PropertyType_DateTime:
        {
            wrl::ComPtr<wf::IReference<wf::DateTime>> spRefDateTime;
            IFC_RETURN(do_query_interface<wf::IReference<wf::DateTime>>(pPropertyValue, &spRefDateTime));

            wf::DateTime dateTime = {};
            IFC_RETURN(spRefDateTime->get_Value(&dateTime));

            IFCCHECK_RETURN(swprintf_s(szValue, ARRAYSIZE(szValue), L"%I64d", dateTime.UniversalTime));
            break;
        }

        case wf::PropertyType_OtherType:
        {
            if (XamlDiagnosticsHelpers::is<wf::IReference<xaml::Thickness>>(pPropertyValue))
            {
                wrl::ComPtr<wf::IReference<xaml::Thickness>> spRefThickness;
                IFC_RETURN(do_query_interface<wf::IReference<xaml::Thickness>>(pPropertyValue, &spRefThickness));

                xaml::Thickness thickness = {};
                IFC_RETURN(spRefThickness->get_Value(&thickness));

                IFCCHECK_RETURN(swprintf_s(szValue, ARRAYSIZE(szValue), L"%g,%g,%g,%g", thickness.Left, thickness.Top, thickness.Right, thickness.Bottom) > 0);
            }
            else if (XamlDiagnosticsHelpers::is<wf::IReference<wu::Color>>(pPropertyValue))
            {
                wrl::ComPtr<wf::IReference<wu::Color>> spRefColor;
                IFC_RETURN(do_query_interface<wf::IReference<wu::Color>>(pPropertyValue, &spRefColor));

                wu::Color color = {};
                IFC_RETURN(spRefColor->get_Value(&color));

                PCWSTR szColorName = XamlDiagnosticsHelpers::FindKnownColorName(color);

                if (szColorName)
                {
                    IFCCHECK_RETURN(swprintf_s(szValue, ARRAYSIZE(szValue), szColorName) > 0);
                }
                else
                {
                    IFCCHECK_RETURN(swprintf_s(szValue, ARRAYSIZE(szValue), L"#%02X%02X%02X%02X", color.A, color.R, color.G, color.B) > 0);
                }
            }
            else if (XamlDiagnosticsHelpers::is<wf::IReference<wut::FontWeight>>(pPropertyValue))
            {
                wrl::ComPtr<wf::IReference<wut::FontWeight>> spRefFontWeight;
                IFC_RETURN(do_query_interface<wf::IReference<wut::FontWeight>>(pPropertyValue, &spRefFontWeight));

                wut::FontWeight fontWeight = {};
                IFC_RETURN(spRefFontWeight->get_Value(&fontWeight));

                PCWSTR szFontWeightName = FindKnownFontWeightName(fontWeight);
                if (szFontWeightName)
                {
                    IFCCHECK_RETURN(swprintf_s(szValue, ARRAYSIZE(szValue), L"%s", szFontWeightName) > 0);
                }
                else
                {
                    IFCCHECK_RETURN(swprintf_s(szValue, ARRAYSIZE(szValue), L"%d", fontWeight.Weight) > 0);
                }
            }
            else if (XamlDiagnosticsHelpers::is<wf::IReference<wxaml_interop::TypeName>>(pPropertyValue))
            {
                wrl::ComPtr<wf::IReference<wxaml_interop::TypeName>> spRefTypename;
                IFC_RETURN(do_query_interface<wf::IReference<wxaml_interop::TypeName>>(pPropertyValue, &spRefTypename));

                wxaml_interop::TypeName typeName{};

                IFC_RETURN(spRefTypename->get_Value(&typeName));

                IFC_RETURN(::WindowsDuplicateString(typeName.Name, phstr));
            }
            else if (XamlDiagnosticsHelpers::is<wf::IReference<xaml::GridLength>>(pPropertyValue))
            {
                wrl::ComPtr<wf::IReference<xaml::GridLength>> spRef;
                xaml::GridLength value;

                IFC_RETURN(do_query_interface<wf::IReference<xaml::GridLength>>(pPropertyValue, &spRef));
                IFC_RETURN(spRef->get_Value(&value));

                switch (value.GridUnitType)
                {
                case xaml::GridUnitType_Auto:
                    IFCCHECK_RETURN(swprintf_s(szValue, ARRAYSIZE(szValue), L"Auto") > 0);
                    break;

                case xaml::GridUnitType_Pixel:
                    IFCCHECK_RETURN(swprintf_s(szValue, ARRAYSIZE(szValue), L"%g", value.Value) > 0);
                    break;

                case xaml::GridUnitType_Star:
                    IFCCHECK_RETURN(swprintf_s(szValue, ARRAYSIZE(szValue), L"%g*", value.Value) > 0);
                    break;
                }
            }
            else if (XamlDiagnosticsHelpers::is<wf::IReference<xaml_media::Matrix>>(pPropertyValue))
            {
                wrl::ComPtr<wf::IReference<xaml_media::Matrix>> spRef;
                xaml_media::Matrix value;

                IFC_RETURN(do_query_interface<wf::IReference<xaml_media::Matrix>>(pPropertyValue, &spRef));
                IFC_RETURN(spRef->get_Value(&value));

                IFCCHECK_RETURN(swprintf_s(szValue, ARRAYSIZE(szValue), L"%f,%f,%f,%f,%f,%f", value.M11, value.M12, value.M21, value.M22, value.OffsetX, value.OffsetY) > 0);
            }
            else if (auto vector3 = XamlDiagnosticsHelpers::as_or_null<wf::IReference<wfn::Vector3>>(pPropertyValue))
            {
                wfn::Vector3 value{};
                IFC_RETURN(vector3->get_Value(&value));
                IFCCHECK_RETURN(swprintf_s(szValue, ARRAYSIZE(szValue), L"%f,%f,%f", value.X, value.Y, value.Z) > 0);
            }
            else if (auto vector2 = XamlDiagnosticsHelpers::as_or_null<wf::IReference<wfn::Vector2>>(pPropertyValue))
            {
                wfn::Vector2 value{};
                IFC_RETURN(vector2->get_Value(&value));
                IFCCHECK_RETURN(swprintf_s(szValue, ARRAYSIZE(szValue), L"%f,%f", value.X, value.Y) > 0);
            }
            else if (auto quaternion = XamlDiagnosticsHelpers::as_or_null<wf::IReference<wfn::Quaternion>>(pPropertyValue))
            {
                wfn::Quaternion value{};
                IFC_RETURN(quaternion->get_Value(&value));
                IFCCHECK_RETURN(swprintf_s(szValue, ARRAYSIZE(szValue), L"%f,%f,%f,%f", value.X, value.Y, value.Z, value.W) > 0);
            }
            else if (auto matrix3x2 = XamlDiagnosticsHelpers::as_or_null<wf::IReference<wfn::Matrix3x2>>(pPropertyValue))
            {
                wfn::Matrix3x2 value{};
                IFC_RETURN(matrix3x2->get_Value(&value));
                IFCCHECK_RETURN(swprintf_s(szValue, ARRAYSIZE(szValue), L"{%f,%f}, {%f,%f}, {%f,%f}",
                    value.M11, value.M12, value.M21, value.M22, value.M31, value.M32) > 0);
            }
            else if (auto matrix4x4 = XamlDiagnosticsHelpers::as_or_null<wf::IReference<wfn::Matrix4x4>>(pPropertyValue))
            {
                wfn::Matrix4x4 value{};
                IFC_RETURN(matrix4x4->get_Value(&value));
                IFCCHECK_RETURN(swprintf_s(szValue, ARRAYSIZE(szValue), L"{%f,%f,%f,%f}, {%f,%f,%f,%f}, {%f,%f,%f,%f}, {%f,%f,%f,%f}",
                    value.M11, value.M12, value.M13, value.M14,
                    value.M21, value.M22, value.M23, value.M24,
                    value.M31, value.M32, value.M33, value.M34,
                    value.M41, value.M42, value.M43, value.M44) > 0);
            }
            else
            {
                return E_FAIL;
            }
            break;
        }

        default:
            return E_FAIL;
    }

    // If the phstr hasn't been set, then the value is in the buffer.
    if (!*phstr)
    {
        IFC_RETURN(wrl_wrappers::HStringReference(szValue, static_cast<unsigned int>(wcslen(szValue))).CopyTo(phstr));
    }

    return S_OK;
}

#pragma endregion

#pragma region BitmapData Helpers

// Returns the equivalent GUID_WicPixelFormat for the given DXGI_FORMAT and DXGI_ALPHA_MODE.
// If the format is not supported the returned GUID value is GUID_NULL.
// The current supported formats are the formats that a CompositionDrawingSurface supports. Because
// of this is necessary to update this function if CompositionDrawingSurface begins supporting new ones.
HRESULT
XamlDiagnosticsHelpers::WicPixelFormatFromDxgiFormat(
    _In_ DXGI_FORMAT format,
    _In_ DXGI_ALPHA_MODE alphaMode,
    _Out_ GUID* guid)
{
    IFCPTR_RETURN(guid);
    *guid = GUID_NULL;

    switch (alphaMode)
    {
    case DXGI_ALPHA_MODE_PREMULTIPLIED:
        switch (format)
        {
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
            *guid = GUID_WICPixelFormat64bppPRGBAHalf;
            break;
        case DXGI_FORMAT_B8G8R8A8_UNORM:
            *guid = GUID_WICPixelFormat32bppPBGRA;
            break;
        case DXGI_FORMAT_R8G8B8A8_UNORM:
            *guid = GUID_WICPixelFormat32bppPRGBA;
            break;
        case DXGI_FORMAT_A8_UNORM:
            *guid = GUID_WICPixelFormat8bppAlpha;
            break;
        }
        break;
    case DXGI_ALPHA_MODE_STRAIGHT:
        switch (format)
        {
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
            *guid = GUID_WICPixelFormat64bppRGBAHalf;
            break;
        case DXGI_FORMAT_B8G8R8A8_UNORM:
            *guid = GUID_WICPixelFormat32bppBGRA;
            break;
        case DXGI_FORMAT_R8G8B8A8_UNORM:
            *guid = GUID_WICPixelFormat32bppRGBA;
            break;
        case DXGI_FORMAT_A8_UNORM:
            *guid = GUID_WICPixelFormat8bppAlpha;
            break;
        }
        break;
    case DXGI_ALPHA_MODE_IGNORE:
        switch (format)
        {
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
            *guid = GUID_WICPixelFormat64bppRGBHalf;
            break;
        case DXGI_FORMAT_B8G8R8A8_UNORM:
            *guid = GUID_WICPixelFormat32bppBGR;
            break;
        case DXGI_FORMAT_R8G8B8A8_UNORM:
            *guid = GUID_WICPixelFormat32bppRGB;
            break;
        case DXGI_FORMAT_A8_UNORM:
            // Alpha channel cannot be ignored in this format.
            break;
        }
        break;
    case DXGI_ALPHA_MODE_UNSPECIFIED:
    case DXGI_ALPHA_MODE_FORCE_DWORD:
        // Invalid Alpha Modes
        break;
    }

    if (*guid == GUID_NULL)
    {
        return E_INVALIDARG;
    }
    return S_OK;
}

// Scale down the bitmap so its width and height are not larger than the given maxPixelWidth and maxPixelHeight.
// If the bitmap fits in the given max dimensions, no scaling happens.
HRESULT
XamlDiagnosticsHelpers::ShrinkToFit(
    _In_ unsigned int maxPixelWidth,
    _In_ unsigned int maxPixelHeight,
    _In_ const wrl::ComPtr<IWICImagingFactory2>& wicImagingFactory,
    _Inout_ wrl::ComPtr<IWICBitmapSource>& wicBitmapSource)
{
    unsigned int width;
    unsigned int height;
    IFC_RETURN(wicBitmapSource->GetSize(&width, &height));

    // Scale down the image if necessary.
    if (width > maxPixelWidth || height > maxPixelHeight)
    {
        double maxPixelWidthInDouble = static_cast<double>(maxPixelWidth);
        double maxPixelHeightInDouble = static_cast<double>(maxPixelHeight);

        // Get the new dimensions for the image.
        unsigned int desiredWidth;
        unsigned int desiredHeight;
        if ((width / maxPixelWidthInDouble) >= (height / maxPixelHeightInDouble))
        {
            desiredWidth = maxPixelWidth;
            desiredHeight = static_cast<unsigned int>(height * (maxPixelWidthInDouble / width));
        }
        else
        {
            desiredHeight = maxPixelHeight;
            desiredWidth = static_cast<unsigned int>(width * (maxPixelHeightInDouble / height));
        }

        // Create and initialize the bitmap scaler
        wrl::ComPtr<IWICBitmapScaler> wicBitmapScaler;
        IFC_RETURN(wicImagingFactory->CreateBitmapScaler(&wicBitmapScaler));
        IFC_RETURN(wicBitmapScaler->Initialize(
            wicBitmapSource.Get(),
            desiredWidth,
            desiredHeight,
            WICBitmapInterpolationModeLinear));

        wicBitmapSource = wicBitmapScaler;
    }
    return S_OK;
}

// Converts the bitmap source to the given GUID_WICPixelFormat.
// When the bitmap pixel format is equal to the desired one, no conversion happens.
HRESULT
XamlDiagnosticsHelpers::ConvertWICBitmapSource(
    _In_ GUID desiredWicPIxelFomat,
    _In_ const wrl::ComPtr<IWICImagingFactory2>& wicImagingFactory,
    _Inout_ wrl::ComPtr<IWICBitmapSource>& wicBitmapSource)
{
    GUID guidBitmapSource;
    IFC_RETURN(wicBitmapSource->GetPixelFormat(&guidBitmapSource));
    if (guidBitmapSource != desiredWicPIxelFomat)
    {
        // Create and initialize the format converter
        wrl::ComPtr<IWICFormatConverter> wicFormatConverter;
        IFC_RETURN(wicImagingFactory->CreateFormatConverter(&wicFormatConverter));

        IFC_RETURN(wicFormatConverter->Initialize(
            wicBitmapSource.Get(),
            desiredWicPIxelFomat, // Target WIC Pixel Format
            WICBitmapDitherTypeNone, // Dither Algorithm to apply when converting the image
            nullptr, // IWICPalette
            0.0, // The alpha threshold to use for conversion
            WICBitmapPaletteTypeCustom)); // The palette used for conversion, this indicates that is not needed

        wicBitmapSource = wicFormatConverter;
    }
    return S_OK;
}

#pragma endregion

#pragma endregion

#pragma endregion
