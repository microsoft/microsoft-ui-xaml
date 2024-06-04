// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <functional>
#include <list>
#include <vector>
#include <map>
#include <utility>
#include "XcpAllocation.h"
#include <NamespaceAliases.h>
#include <wrl/wrappers/corewrappers.h>

namespace DirectUI
{
    class Binding;
    class PropertyPathStep;
    class DXamlCore;
    class DependencyObject;
    interface IValueConverterInternal;
}

namespace DebugTool
{
#if XCP_MONITOR
    typedef std::map<std::wstring, std::wstring,
        std::less<std::wstring>,
        XcpAllocation::LeakIgnoringAllocator<std::pair<std::wstring, std::wstring>>> EnvironmentMap;
#else
    typedef std::map<std::wstring, std::wstring> EnvironmentMap;
#endif

    // This interface provides access to detailed information about bindings.
    interface __declspec(uuid("10660cf5-d18d-4a4f-ab87-8412dde84ade")) IDebugBindingExpression : IUnknown
    {
        virtual _Check_return_ HRESULT GetBindingString(_Out_ HSTRING* phDebugString) = 0;
        virtual bool IsBindingValid() = 0;
        virtual DirectUI::PropertyPathStep* GetFirstPropertyPathStep() = 0;
        virtual xaml::IDependencyObject* GetBindingAsDO() = 0;

        virtual HRESULT GetPath(_Out_ HSTRING* phPath) = 0;
        virtual HRESULT GetBindingMode(_Out_ xaml_data::BindingMode* bindingMode) = 0;
        virtual HRESULT GetSource(_Outptr_opt_ IInspectable** ppSource) = 0;
        virtual HRESULT GetElementName(_Out_ HSTRING* phElementName) = 0;
        virtual HRESULT GetConverter(_Outptr_opt_ void** ppConverter) = 0;
        virtual HRESULT GetConverterParameter(_Outptr_opt_ IInspectable** ppConverterParameter) = 0;
        virtual HRESULT GetConverterLanguage(_Out_ HSTRING* phConverterLanguage) = 0;
        virtual HRESULT GetFallbackValue(_Outptr_opt_ IInspectable** ppFallbackValue) = 0;
        virtual HRESULT GetTargetNullValue(_Outptr_opt_ IInspectable** ppTargetNullValue) = 0;
    };

    template <class T>
    interface ICollection
    {
        // This initialize is only used for old snoop.
        virtual HRESULT Initialize(const std::vector<T>& other) = 0;
        virtual HRESULT Append(const T& item) = 0;
        virtual HRESULT Append(T&& item) = 0;
        virtual const std::vector<T>& GetVectorView() const = 0;
        virtual size_t GetSize() const = 0;
        virtual void Clear() = 0;
    };

    class DebugEnumInfo2
    {
    public:
        DebugEnumInfo2() = default;
        DebugEnumInfo2(const DebugEnumInfo2& other)
        {
            *this = other;
        }
        DebugEnumInfo2(DebugEnumInfo2&& other)
        {
            *this = std::move(other);
        }

        DebugEnumInfo2& operator=(const DebugEnumInfo2& other)
        {
            if(this != &other)
            {
                m_index = other.m_index;
                m_name.Set(other.m_name.Get());
                m_values = other.m_values;
            }

            return *this;
        }
        DebugEnumInfo2& operator=(DebugEnumInfo2&& other) noexcept
        {
            if (this != &other)
            {
                m_index = std::move(other.m_index);
                m_name = std::move(other.m_name);
                m_values = std::move(other.m_values);
            }

            return *this;
        }

        HRESULT Initialize(
            _In_ int index,
            _In_ PCWSTR szName,
            _In_ const std::vector<std::pair<int, PCWSTR>>& values)
        {
            if (m_name.Set(szName) != S_OK)
            {
                return E_OUTOFMEMORY;
            }

            m_index = index;
            m_values = values;

            return S_OK;
        }

        inline int GetTypeIndex() const { return m_index; }
        inline PCWSTR GetName() const { return m_name.GetRawBuffer(nullptr); }
        inline const std::vector<std::pair<int, PCWSTR>>& GetValues() const { return m_values; }

    private:
        int m_index = 0; // KnownTypeIndex::UnknownType
        wrl_wrappers::HString m_name;
        std::vector<std::pair<int, PCWSTR>> m_values;
    };

} // namespace DebugTool
