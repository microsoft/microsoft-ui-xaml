// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "includes.h"

class xstring_ptr;

#pragma region XamlDiagnosticsHelpers

class XamlDiagnosticsHelpers
{
private:
    static wrl_wrappers::HString _fallbackString;

public:

    #pragma region Environment Helpers

    static std::tuple<bool, std::wstring> TryGetEnv(
        _In_ const std::wstring& varName,
        _In_ const dt::EnvironmentMap& extraEnv);

    static std::wstring GetEnv(
        _In_ const std::wstring& varName,
        _In_ const dt::EnvironmentMap& extraEnv,
        _In_ const std::wstring& defaultValue);

    #pragma endregion

    #pragma region Conversion Helpers

    static const wrl_wrappers::HString& GetFallbackString();

    static PCWSTR FindKnownColorName(
        _In_ const wu::Color &color);

    static PCWSTR FindKnownFontWeightName(
        _In_ const wut::FontWeight &fontWeight);

    static HRESULT ValueToString(
        _In_ const ctl::ComPtr<IInspectable>& value,
        _Out_ xstring_ptr* valueString);

    static HRESULT ConvertValueToStringOverride(
        _In_ wf::IPropertyValue* pPropertyValue,
        _In_ wf::PropertyType propertyType,
        _Out_ HSTRING *phstr);

    #pragma endregion

    #pragma region QI Helpers

    template <class T>
    static HRESULT do_query_interface(
        _In_opt_ IUnknown *pIn,
        _Outptr_ T** pOut);

    template <>
    static HRESULT do_query_interface(
        _In_opt_ IUnknown *pIn,
        _Outptr_ HSTRING* pOut);

    template <class T>
    static wrl::ComPtr<T> as_or_null(
        _In_opt_ IUnknown *pIn);

    template <typename T, typename U>
    static bool is(
        _In_opt_ U *pInterface);

    template <class T>
    static HRESULT WinRTCreateInstance(
        _In_ PCWSTR szType,
        _Outptr_ T** pOut);

    #pragma endregion

    #pragma region BitmapData Helpers

    static HRESULT WicPixelFormatFromDxgiFormat(
        _In_ DXGI_FORMAT format,
        _In_ DXGI_ALPHA_MODE alphaMode,
        _Out_ GUID* guid);

    static HRESULT ShrinkToFit(
        _In_ unsigned int maxPixelWidth,
        _In_ unsigned int maxPixelHeight,
        _In_ const wrl::ComPtr<IWICImagingFactory2>& wicImagingFactory,
        _Inout_ wrl::ComPtr<IWICBitmapSource>& wicBitmapSource);

    static HRESULT ConvertWICBitmapSource(
        _In_ GUID desiredWicPIxelFomat,
        _In_ const wrl::ComPtr<IWICImagingFactory2>& wicImagingFactory,
        _Inout_ wrl::ComPtr<IWICBitmapSource>& wicBitmapSource);

    #pragma endregion
};

#pragma endregion

#pragma region Collection

// Implements ICollection to proxy property additions from
// an IInternalDebugInterop into a std::list<T>.  This
// class is intended to be instantiated on the stack.
template <class T>
class Collection : public dt::ICollection<T>
{
public:
    Collection() {}

    HRESULT Initialize(const std::vector<T>& other) override;

    HRESULT Append(const T& item) override;
    HRESULT Append(T&& item) override;

    const std::vector<T>& GetVectorView() const override;
    size_t GetSize() const override;
    void Clear() override;

    std::vector<T>& GetVector();

protected:
    std::vector<T> m_vector;
};

#pragma endregion

#pragma region SafeArrayWrapper

template <typename T>
struct SafeArrayAutomationType
{
    static_assert(sizeof(SafeArrayAutomationType) == 0, "Missing automation type. Please add a DEFINE_AUTOMATION_TYPE entry below");
};

#define DEFINE_AUTOMATION_TYPE(ctype, oleautomationtype) \
    template <> \
    struct SafeArrayAutomationType<ctype> \
    { \
        enum { type = oleautomationtype }; \
    };

DEFINE_AUTOMATION_TYPE(int, VT_INT);
DEFINE_AUTOMATION_TYPE(BSTR, VT_BSTR);

template <class T>
class SafeArrayPtr
{
public:
    SafeArrayPtr(_In_ size_t size);
    ~SafeArrayPtr();

    T& operator[](_In_ size_t index);

    SAFEARRAY* Detach();

private:
    void InternalLock();
    void InternalUnlock();
    void InternalDestroy();
    T& InternalGetAt(size_t index);

private:
    SAFEARRAY* m_ptr;
};

#pragma endregion

#pragma region ComValueCollection

template <class T>
class ComValueCollection final
{
public:
    void Append(T&& item)
    {
        m_vector.push_back(std::forward<T>(item));
    }

    std::vector<T>& GetVectorView()
    {
        return m_vector;
    }

    size_t GetSize() const
    {
        return m_vector.size();
    }

    T* RealizeAndDetach()
    {
        auto size = GetSize();
        T* pResult = reinterpret_cast<T*>(CoTaskMemAlloc(sizeof(T) * size));
        ZeroMemory(pResult, sizeof(T) * size);
        auto guard = wil::scope_exit([&]()
        {
            CoTaskMemFree(pResult);
        });

        std::move(m_vector.begin(), m_vector.end(), pResult);
        guard.release();
        m_vector.clear();

        return pResult;
    }

private:
    std::vector<T> m_vector;
};

template <class T, class U>
class ComValueCollectionTranslator : public Collection<T>
{
public:
    U* RealizeAndDetach(_In_ std::function<U(const T&)> translator)
    {
        auto size = this->GetSize();
        U* pResult = reinterpret_cast<U*>(CoTaskMemAlloc(sizeof(U) * size));
        ZeroMemory(pResult, sizeof(U) * size);
        auto guard = wil::scope_exit([&]()
        {
            CoTaskMemFree(pResult);
        });

        std::transform(
            this->m_vector.begin(),
            this->m_vector.end(),
            pResult,
            translator);

        guard.release();
        this->Clear();

        return pResult;
    }
};

#pragma endregion

#include "helpersImpl.h"