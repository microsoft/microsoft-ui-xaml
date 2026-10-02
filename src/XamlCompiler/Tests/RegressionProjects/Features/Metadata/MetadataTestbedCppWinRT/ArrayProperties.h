// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include <vector>
#include "ArrayProperties.g.h"

namespace winrt::MetadataTestbedCppWinRT::implementation
{
    struct ArrayProperties : ArrayPropertiesT<ArrayProperties>
    {
        ArrayProperties() = default;

        com_array<int32_t> Int32Values() const
        {
            return { m_int32Values.begin(), m_int32Values.end() };
        }

        void Int32Values(array_view<int32_t const> const& value)
        {
            m_int32Values.assign(value.begin(), value.end());
        }

        com_array<uint16_t> UInt16Values() const
        {
            return { m_uint16Values.begin(), m_uint16Values.end() };
        }

        void UInt16Values(array_view<uint16_t const> const& value)
        {
            m_uint16Values.assign(value.begin(), value.end());
        }

        com_array<hstring> StringValues() const
        {
            return { m_stringValues.begin(), m_stringValues.end() };
        }

        void StringValues(array_view<hstring const> const& value)
        {
            m_stringValues.assign(value.begin(), value.end());
        }

        com_array<guid> GuidValues() const
        {
            return { m_guidValues.begin(), m_guidValues.end() };
        }

        void GuidValues(array_view<guid const> const& value)
        {
            m_guidValues.assign(value.begin(), value.end());
        }

        com_array<char16_t> CharValues() const
        {
            return { m_charValues.begin(), m_charValues.end() };
        }

        void CharValues(array_view<char16_t const> const& value)
        {
            m_charValues.assign(value.begin(), value.end());
        }

        com_array<Windows::Foundation::IInspectable> ObjectValues() const
        {
            return { m_objectValues.begin(), m_objectValues.end() };
        }

        void ObjectValues(array_view<Windows::Foundation::IInspectable const> const& value)
        {
            m_objectValues.assign(value.begin(), value.end());
        }

        com_array<Windows::Foundation::Point> PointValues() const
        {
            return { m_pointValues.begin(), m_pointValues.end() };
        }

        void PointValues(array_view<Windows::Foundation::Point const> const& value)
        {
            m_pointValues.assign(value.begin(), value.end());
        }

        com_array<int32_t> ReadOnlyValues() const
        {
            return Int32Values();
        }

        static com_array<int32_t> GetAttachedValues(Windows::Foundation::IInspectable const& target)
        {
            return target.as<MetadataTestbedCppWinRT::ArrayProperties>().Int32Values();
        }

        static void SetAttachedValues(Windows::Foundation::IInspectable const& target, array_view<int32_t const> const& value)
        {
            target.as<MetadataTestbedCppWinRT::ArrayProperties>().Int32Values(value);
        }

    private:
        std::vector<int32_t> m_int32Values;
        std::vector<uint16_t> m_uint16Values;
        std::vector<hstring> m_stringValues;
        std::vector<guid> m_guidValues;
        std::vector<char16_t> m_charValues;
        std::vector<Windows::Foundation::IInspectable> m_objectValues;
        std::vector<Windows::Foundation::Point> m_pointValues;
    };
}

namespace winrt::MetadataTestbedCppWinRT::factory_implementation
{
    struct ArrayProperties : ArrayPropertiesT<ArrayProperties, implementation::ArrayProperties>
    {
    };
}
