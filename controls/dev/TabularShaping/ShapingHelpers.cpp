// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ShapingHelpers.h"

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstring>
#include <numeric>
#include <unordered_map>
#include "winnls.h"

namespace TabularShapingHelpers
{
    namespace
    {
        enum class NumericKind { None, Signed, Unsigned, Floating };

        enum class ValueClassRank
        {
            Numeric = 0,
            String,
            Guid,
            Boolean,
            Char16,
            DateTime,
            TimeSpan,
            Empty,
            OtherProperty,
            Stringable,
            Object,
            Unknown,
        };

        int CompareFloating(double a, double b);
        int CompareObjectLookupKeys(winrt::IInspectable const& a, winrt::IInspectable const& b);

        NumericKind ClassifyNumeric(
            winrt::Windows::Foundation::IPropertyValue const& propertyValue,
            int64_t& signedValue,
            uint64_t& unsignedValue,
            double& doubleValue)
        {
            switch (propertyValue.Type())
            {
            case winrt::Windows::Foundation::PropertyType::Int16:
                signedValue = propertyValue.GetInt16();
                doubleValue = static_cast<double>(signedValue);
                return NumericKind::Signed;
            case winrt::Windows::Foundation::PropertyType::Int32:
                signedValue = propertyValue.GetInt32();
                doubleValue = static_cast<double>(signedValue);
                return NumericKind::Signed;
            case winrt::Windows::Foundation::PropertyType::Int64:
                signedValue = propertyValue.GetInt64();
                doubleValue = static_cast<double>(signedValue);
                return NumericKind::Signed;
            case winrt::Windows::Foundation::PropertyType::UInt8:
                unsignedValue = propertyValue.GetUInt8();
                doubleValue = static_cast<double>(unsignedValue);
                return NumericKind::Unsigned;
            case winrt::Windows::Foundation::PropertyType::UInt16:
                unsignedValue = propertyValue.GetUInt16();
                doubleValue = static_cast<double>(unsignedValue);
                return NumericKind::Unsigned;
            case winrt::Windows::Foundation::PropertyType::UInt32:
                unsignedValue = propertyValue.GetUInt32();
                doubleValue = static_cast<double>(unsignedValue);
                return NumericKind::Unsigned;
            case winrt::Windows::Foundation::PropertyType::UInt64:
                unsignedValue = propertyValue.GetUInt64();
                doubleValue = static_cast<double>(unsignedValue);
                return NumericKind::Unsigned;
            case winrt::Windows::Foundation::PropertyType::Single:
                doubleValue = propertyValue.GetSingle();
                return NumericKind::Floating;
            case winrt::Windows::Foundation::PropertyType::Double:
                doubleValue = propertyValue.GetDouble();
                return NumericKind::Floating;
            default:
                return NumericKind::None;
            }
        }

        int CompareIntToDouble(int64_t signedValue, double doubleValue)
        {
            if (std::isnan(doubleValue))
            {
                return -1;
            }

            constexpr double int64LowerBoundAsDouble{ -9223372036854775808.0 };
            constexpr double int64UpperBoundAsDouble{ 9223372036854775808.0 };
            if (doubleValue >= int64UpperBoundAsDouble)
            {
                return -1;
            }
            if (doubleValue < int64LowerBoundAsDouble)
            {
                return 1;
            }

            const double floored = std::floor(doubleValue);
            const auto doubleAsInt = static_cast<int64_t>(floored);
            if (signedValue < doubleAsInt)
            {
                return -1;
            }
            if (signedValue > doubleAsInt)
            {
                return 1;
            }
            return doubleValue > floored ? -1 : 0;
        }

        int CompareUIntToDouble(uint64_t unsignedValue, double doubleValue)
        {
            if (std::isnan(doubleValue))
            {
                return -1;
            }

            constexpr double uint64UpperBoundAsDouble{ 18446744073709551616.0 };
            if (doubleValue >= uint64UpperBoundAsDouble)
            {
                return -1;
            }
            if (doubleValue < 0.0)
            {
                return 1;
            }

            const double floored = std::floor(doubleValue);
            const auto doubleAsUInt = static_cast<uint64_t>(floored);
            if (unsignedValue < doubleAsUInt)
            {
                return -1;
            }
            if (unsignedValue > doubleAsUInt)
            {
                return 1;
            }
            return doubleValue > floored ? -1 : 0;
        }

        int CompareStrings(winrt::hstring const& a, winrt::hstring const& b)
        {
            if (a.size() <= static_cast<uint32_t>(INT_MAX) &&
                b.size() <= static_cast<uint32_t>(INT_MAX))
            {
                const int result = CompareStringEx(
                    LOCALE_NAME_USER_DEFAULT,
                    0,
                    a.c_str(),
                    static_cast<int>(a.size()),
                    b.c_str(),
                    static_cast<int>(b.size()),
                    nullptr,
                    nullptr,
                    0);
                if (result == CSTR_LESS_THAN)
                {
                    return -1;
                }
                if (result == CSTR_GREATER_THAN)
                {
                    return 1;
                }
                if (result == CSTR_EQUAL)
                {
                    return 0;
                }
            }

            return a < b ? -1 : (a > b ? 1 : 0);
        }

        int CompareSamePropertyType(
            winrt::Windows::Foundation::IPropertyValue const& a,
            winrt::Windows::Foundation::IPropertyValue const& b)
        {
            switch (a.Type())
            {
            case winrt::Windows::Foundation::PropertyType::String:
            {
                const winrt::hstring va{ a.GetString() };
                const winrt::hstring vb{ b.GetString() };
                return CompareStrings(va, vb);
            }
            case winrt::Windows::Foundation::PropertyType::Int16:
                return a.GetInt16() < b.GetInt16() ? -1 : (a.GetInt16() > b.GetInt16() ? 1 : 0);
            case winrt::Windows::Foundation::PropertyType::Int32:
                return a.GetInt32() < b.GetInt32() ? -1 : (a.GetInt32() > b.GetInt32() ? 1 : 0);
            case winrt::Windows::Foundation::PropertyType::Int64:
                return a.GetInt64() < b.GetInt64() ? -1 : (a.GetInt64() > b.GetInt64() ? 1 : 0);
            case winrt::Windows::Foundation::PropertyType::UInt8:
                return a.GetUInt8() < b.GetUInt8() ? -1 : (a.GetUInt8() > b.GetUInt8() ? 1 : 0);
            case winrt::Windows::Foundation::PropertyType::UInt16:
                return a.GetUInt16() < b.GetUInt16() ? -1 : (a.GetUInt16() > b.GetUInt16() ? 1 : 0);
            case winrt::Windows::Foundation::PropertyType::UInt32:
                return a.GetUInt32() < b.GetUInt32() ? -1 : (a.GetUInt32() > b.GetUInt32() ? 1 : 0);
            case winrt::Windows::Foundation::PropertyType::UInt64:
                return a.GetUInt64() < b.GetUInt64() ? -1 : (a.GetUInt64() > b.GetUInt64() ? 1 : 0);
            case winrt::Windows::Foundation::PropertyType::Single:
            {
                const auto va = a.GetSingle();
                const auto vb = b.GetSingle();
                const bool aNaN = std::isnan(va);
                const bool bNaN = std::isnan(vb);
                if (aNaN || bNaN)
                {
                    return aNaN == bNaN ? 0 : (aNaN ? 1 : -1);
                }
                return va < vb ? -1 : (va > vb ? 1 : 0);
            }
            case winrt::Windows::Foundation::PropertyType::Double:
            {
                const auto va = a.GetDouble();
                const auto vb = b.GetDouble();
                const bool aNaN = std::isnan(va);
                const bool bNaN = std::isnan(vb);
                if (aNaN || bNaN)
                {
                    return aNaN == bNaN ? 0 : (aNaN ? 1 : -1);
                }
                return va < vb ? -1 : (va > vb ? 1 : 0);
            }
            case winrt::Windows::Foundation::PropertyType::Boolean:
                return a.GetBoolean() == b.GetBoolean() ? 0 : (a.GetBoolean() ? 1 : -1);
            case winrt::Windows::Foundation::PropertyType::Char16:
                return a.GetChar16() < b.GetChar16() ? -1 : (a.GetChar16() > b.GetChar16() ? 1 : 0);
            case winrt::Windows::Foundation::PropertyType::DateTime:
                return a.GetDateTime().time_since_epoch().count() < b.GetDateTime().time_since_epoch().count() ? -1 :
                    (a.GetDateTime().time_since_epoch().count() > b.GetDateTime().time_since_epoch().count() ? 1 : 0);
            case winrt::Windows::Foundation::PropertyType::TimeSpan:
                return a.GetTimeSpan().count() < b.GetTimeSpan().count() ? -1 :
                    (a.GetTimeSpan().count() > b.GetTimeSpan().count() ? 1 : 0);
            case winrt::Windows::Foundation::PropertyType::Guid:
            {
                const auto va = a.GetGuid();
                const auto vb = b.GetGuid();
                if (va.Data1 != vb.Data1) return va.Data1 < vb.Data1 ? -1 : 1;
                if (va.Data2 != vb.Data2) return va.Data2 < vb.Data2 ? -1 : 1;
                if (va.Data3 != vb.Data3) return va.Data3 < vb.Data3 ? -1 : 1;
                for (int i = 0; i < 8; ++i)
                {
                    if (va.Data4[i] != vb.Data4[i])
                    {
                        return va.Data4[i] < vb.Data4[i] ? -1 : 1;
                    }
                }
                return 0;
            }
            case winrt::Windows::Foundation::PropertyType::Point:
            {
                const auto va = a.GetPoint();
                const auto vb = b.GetPoint();
                const int x = CompareFloating(va.X, vb.X);
                return x != 0 ? x : CompareFloating(va.Y, vb.Y);
            }
            case winrt::Windows::Foundation::PropertyType::Size:
            {
                const auto va = a.GetSize();
                const auto vb = b.GetSize();
                const int width = CompareFloating(va.Width, vb.Width);
                return width != 0 ? width : CompareFloating(va.Height, vb.Height);
            }
            case winrt::Windows::Foundation::PropertyType::Rect:
            {
                const auto va = a.GetRect();
                const auto vb = b.GetRect();
                const int x = CompareFloating(va.X, vb.X);
                if (x != 0) return x;
                const int y = CompareFloating(va.Y, vb.Y);
                if (y != 0) return y;
                const int width = CompareFloating(va.Width, vb.Width);
                return width != 0 ? width : CompareFloating(va.Height, vb.Height);
            }
            default:
                return CompareObjectLookupKeys(a, b);
            }
        }

        int ComparePropertyValues(
            winrt::Windows::Foundation::IPropertyValue const& a,
            winrt::Windows::Foundation::IPropertyValue const& b)
        {
            if (a.Type() == b.Type())
            {
                return CompareSamePropertyType(a, b);
            }

            int64_t signedA = 0;
            int64_t signedB = 0;
            uint64_t unsignedA = 0;
            uint64_t unsignedB = 0;
            double doubleA = 0;
            double doubleB = 0;
            const auto kindA = ClassifyNumeric(a, signedA, unsignedA, doubleA);
            const auto kindB = ClassifyNumeric(b, signedB, unsignedB, doubleB);
            if (kindA == NumericKind::None || kindB == NumericKind::None)
            {
                return 0;
            }

            if (kindA == NumericKind::Signed && kindB == NumericKind::Signed)
            {
                return signedA < signedB ? -1 : (signedA > signedB ? 1 : 0);
            }
            if (kindA == NumericKind::Unsigned && kindB == NumericKind::Unsigned)
            {
                return unsignedA < unsignedB ? -1 : (unsignedA > unsignedB ? 1 : 0);
            }
            if (kindA != NumericKind::Floating && kindB != NumericKind::Floating)
            {
                const bool aNegative = kindA == NumericKind::Signed && signedA < 0;
                const bool bNegative = kindB == NumericKind::Signed && signedB < 0;
                if (aNegative != bNegative)
                {
                    return aNegative ? -1 : 1;
                }

                const uint64_t va = kindA == NumericKind::Signed ? static_cast<uint64_t>(signedA) : unsignedA;
                const uint64_t vb = kindB == NumericKind::Signed ? static_cast<uint64_t>(signedB) : unsignedB;
                return va < vb ? -1 : (va > vb ? 1 : 0);
            }

            const bool aNaN = std::isnan(doubleA);
            const bool bNaN = std::isnan(doubleB);
            if (aNaN || bNaN)
            {
                return aNaN == bNaN ? 0 : (aNaN ? 1 : -1);
            }
            if (kindA == NumericKind::Signed)
            {
                return CompareIntToDouble(signedA, doubleB);
            }
            if (kindB == NumericKind::Signed)
            {
                return -CompareIntToDouble(signedB, doubleA);
            }
            if (kindA == NumericKind::Unsigned)
            {
                return CompareUIntToDouble(unsignedA, doubleB);
            }
            if (kindB == NumericKind::Unsigned)
            {
                return -CompareUIntToDouble(unsignedB, doubleA);
            }
            return doubleA < doubleB ? -1 : (doubleA > doubleB ? 1 : 0);
        }

        ValueClassRank GetPropertyValueClassRank(winrt::Windows::Foundation::IPropertyValue const& propertyValue)
        {
            switch (propertyValue.Type())
            {
            case winrt::Windows::Foundation::PropertyType::Int16:
            case winrt::Windows::Foundation::PropertyType::Int32:
            case winrt::Windows::Foundation::PropertyType::Int64:
            case winrt::Windows::Foundation::PropertyType::UInt8:
            case winrt::Windows::Foundation::PropertyType::UInt16:
            case winrt::Windows::Foundation::PropertyType::UInt32:
            case winrt::Windows::Foundation::PropertyType::UInt64:
            case winrt::Windows::Foundation::PropertyType::Single:
            case winrt::Windows::Foundation::PropertyType::Double:
                return ValueClassRank::Numeric;
            case winrt::Windows::Foundation::PropertyType::String:
                return ValueClassRank::String;
            case winrt::Windows::Foundation::PropertyType::Guid:
                return ValueClassRank::Guid;
            case winrt::Windows::Foundation::PropertyType::Boolean:
                return ValueClassRank::Boolean;
            case winrt::Windows::Foundation::PropertyType::Char16:
                return ValueClassRank::Char16;
            case winrt::Windows::Foundation::PropertyType::DateTime:
                return ValueClassRank::DateTime;
            case winrt::Windows::Foundation::PropertyType::TimeSpan:
                return ValueClassRank::TimeSpan;
            case winrt::Windows::Foundation::PropertyType::Empty:
                return ValueClassRank::Empty;
            default:
                return ValueClassRank::OtherProperty;
            }
        }

        ValueClassRank GetValueClassRank(winrt::IInspectable const& value)
        {
            if (auto propertyValue = value.try_as<winrt::Windows::Foundation::IPropertyValue>())
            {
                return GetPropertyValueClassRank(propertyValue);
            }
            if (value.try_as<winrt::Windows::Foundation::IStringable>())
            {
                return ValueClassRank::Stringable;
            }
            if (value.try_as<::IUnknown>())
            {
                return ValueClassRank::Object;
            }
            return ValueClassRank::Unknown;
        }

        int CompareValueClassRank(ValueClassRank a, ValueClassRank b)
        {
            return a < b ? -1 : (a > b ? 1 : 0);
        }

        int CompareFloating(double a, double b)
        {
            const bool aNaN = std::isnan(a);
            const bool bNaN = std::isnan(b);
            if (aNaN || bNaN)
            {
                return aNaN == bNaN ? 0 : (aNaN ? 1 : -1);
            }
            return a < b ? -1 : (a > b ? 1 : 0);
        }

        int CompareObjectLookupKeys(winrt::IInspectable const& a, winrt::IInspectable const& b)
        {
            const auto keyA = TabularValueKey::ToObjectLookupKey(a);
            const auto keyB = TabularValueKey::ToObjectLookupKey(b);
            return keyA < keyB ? -1 : (keyA > keyB ? 1 : 0);
        }
    }

    bool TabularValueKey::TryFormatPropertyValue(
        winrt::Windows::Foundation::IPropertyValue const& propertyValue,
        winrt::hstring& key,
        bool rejectEmptyString)
    {
        key = {};
        switch (propertyValue.Type())
        {
        case winrt::Windows::Foundation::PropertyType::Empty:
            key = L"<empty>";
            return true;
        case winrt::Windows::Foundation::PropertyType::String:
        {
            const auto value = propertyValue.GetString();
            if (rejectEmptyString && value.empty())
            {
                return false;
            }
            key = L"s:" + value;
            return true;
        }
        case winrt::Windows::Foundation::PropertyType::Int16:
            key = L"i16:" + winrt::to_hstring(propertyValue.GetInt16());
            return true;
        case winrt::Windows::Foundation::PropertyType::Int32:
            key = L"i32:" + winrt::to_hstring(propertyValue.GetInt32());
            return true;
        case winrt::Windows::Foundation::PropertyType::Int64:
            key = L"i64:" + winrt::to_hstring(propertyValue.GetInt64());
            return true;
        case winrt::Windows::Foundation::PropertyType::UInt8:
            key = L"u8:" + winrt::to_hstring(propertyValue.GetUInt8());
            return true;
        case winrt::Windows::Foundation::PropertyType::UInt16:
            key = L"u16:" + winrt::to_hstring(propertyValue.GetUInt16());
            return true;
        case winrt::Windows::Foundation::PropertyType::UInt32:
            key = L"u32:" + winrt::to_hstring(propertyValue.GetUInt32());
            return true;
        case winrt::Windows::Foundation::PropertyType::UInt64:
            key = L"u64:" + winrt::to_hstring(propertyValue.GetUInt64());
            return true;
        case winrt::Windows::Foundation::PropertyType::Single:
        {
            const auto value = propertyValue.GetSingle();
            uint32_t bits = 0;
            static_assert(sizeof(bits) == sizeof(value));
            std::memcpy(&bits, &value, sizeof(bits));
            wchar_t buffer[16];
            swprintf_s(buffer, L"f32:%08X", bits);
            key = winrt::hstring{ buffer };
            return true;
        }
        case winrt::Windows::Foundation::PropertyType::Double:
        {
            const auto value = propertyValue.GetDouble();
            uint64_t bits = 0;
            static_assert(sizeof(bits) == sizeof(value));
            std::memcpy(&bits, &value, sizeof(bits));
            wchar_t buffer[24];
            swprintf_s(buffer, L"f64:%016llX", static_cast<unsigned long long>(bits));
            key = winrt::hstring{ buffer };
            return true;
        }
        case winrt::Windows::Foundation::PropertyType::Boolean:
            key = propertyValue.GetBoolean() ? L"b:1" : L"b:0";
            return true;
        case winrt::Windows::Foundation::PropertyType::Char16:
            key = L"c:" + winrt::to_hstring(static_cast<uint16_t>(propertyValue.GetChar16()));
            return true;
        case winrt::Windows::Foundation::PropertyType::DateTime:
            key = L"dt:" + winrt::to_hstring(propertyValue.GetDateTime().time_since_epoch().count());
            return true;
        case winrt::Windows::Foundation::PropertyType::TimeSpan:
            key = L"ts:" + winrt::to_hstring(propertyValue.GetTimeSpan().count());
            return true;
        case winrt::Windows::Foundation::PropertyType::Guid:
        {
            auto guid = propertyValue.GetGuid();
            wchar_t buffer[40];
            swprintf_s(buffer, 40, L"g:%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
                guid.Data1, guid.Data2, guid.Data3,
                guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
                guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
            key = winrt::hstring{ buffer };
            return true;
        }
        default:
            return false;
        }
    }

    bool TabularValueKey::TryGetStablePropertyKey(
        winrt::IInspectable const& value,
        winrt::hstring& key,
        bool rejectEmptyString)
    {
        key = {};
        if (!value)
        {
            return false;
        }

        if (auto propertyValue = value.try_as<winrt::Windows::Foundation::IPropertyValue>())
        {
            return TryFormatPropertyValue(propertyValue, key, rejectEmptyString);
        }

        return false;
    }

    winrt::hstring TabularValueKey::ToString(winrt::IInspectable const& value)
    {
        if (!value)
        {
            return L"<null>";
        }

        winrt::hstring key;
        if (TryGetStablePropertyKey(value, key, false))
        {
            return key;
        }

        if (auto stringable = value.try_as<winrt::Windows::Foundation::IStringable>())
        {
            try
            {
                return L"x:" + stringable.ToString();
            }
            catch (...)
            {
            }
        }

        if (auto unknown = value.try_as<::IUnknown>())
        {
            wchar_t buffer[40];
            swprintf_s(buffer, 40, L"o:%016llX", static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(unknown.get())));
            return winrt::hstring{ buffer };
        }

        return L"<unknown>";
    }

    winrt::hstring TabularValueKey::ToObjectLookupKey(winrt::IInspectable const& value, bool rejectEmptyString)
    {
        if (!value)
        {
            return L"null:";
        }

        winrt::hstring key;
        if (TryGetStablePropertyKey(value, key, rejectEmptyString))
        {
            return L"value:" + key;
        }

        if (auto unknown = value.try_as<::IUnknown>())
        {
            wchar_t buffer[32];
            swprintf_s(buffer, 32, L"object:%p", unknown.get());
            return winrt::hstring{ buffer };
        }

        return L"";
    }

    bool TabularValueComparer::UsesFallbackKey(winrt::IInspectable const& value)
    {
        return value && !value.try_as<winrt::Windows::Foundation::IPropertyValue>();
    }

    int TabularValueComparer::Compare(winrt::IInspectable const& a, winrt::IInspectable const& b)
    {
        return Compare(a, b, nullptr, nullptr);
    }

    int TabularValueComparer::Compare(
        winrt::IInspectable const& a,
        winrt::IInspectable const& b,
        winrt::hstring const* fallbackKeyA,
        winrt::hstring const* fallbackKeyB)
    {
        if (!a && !b) return 0;
        if (!a) return -1;
        if (!b) return 1;

        auto propertyA = a.try_as<winrt::Windows::Foundation::IPropertyValue>();
        auto propertyB = b.try_as<winrt::Windows::Foundation::IPropertyValue>();
        if (propertyA && propertyB)
        {
            if (propertyA.Type() == propertyB.Type())
            {
                return ComparePropertyValues(propertyA, propertyB);
            }

            int64_t signedA = 0;
            int64_t signedB = 0;
            uint64_t unsignedA = 0;
            uint64_t unsignedB = 0;
            double doubleA = 0;
            double doubleB = 0;
            if (ClassifyNumeric(propertyA, signedA, unsignedA, doubleA) != NumericKind::None &&
                ClassifyNumeric(propertyB, signedB, unsignedB, doubleB) != NumericKind::None)
            {
                return ComparePropertyValues(propertyA, propertyB);
            }

            const auto rankA = GetPropertyValueClassRank(propertyA);
            const auto rankB = GetPropertyValueClassRank(propertyB);
            if (rankA != rankB)
            {
                return CompareValueClassRank(rankA, rankB);
            }

            return propertyA.Type() < propertyB.Type() ? -1 : 1;
        }

        const auto rankA = GetValueClassRank(a);
        const auto rankB = GetValueClassRank(b);
        if (rankA != rankB)
        {
            return CompareValueClassRank(rankA, rankB);
        }

        const winrt::hstring keyA{ fallbackKeyA ? *fallbackKeyA : TabularValueKey::ToString(a) };
        const winrt::hstring keyB{ fallbackKeyB ? *fallbackKeyB : TabularValueKey::ToString(b) };
        return keyA < keyB ? -1 : (keyA > keyB ? 1 : 0);
    }

    std::vector<winrt::IInspectable> EnumerateInspectableItems(
        winrt::IInspectable const& source,
        bool throwIfUnsupported)
    {
        // Single entry point for callers holding a raw source. The classification and the walk both
        // live on CollectionAccessor so there is exactly one interface ladder in the stack.
        return CollectionAccessor{ source }.Enumerate(throwIfUnsupported);
    }

    CollectionAccessor::CollectionAccessor(winrt::IInspectable const& source) :
        m_source(source)
    {
        if (!source)
        {
            return;
        }

        // Read resolution, cheapest indexed call first. IVectorView sits above IBindableVector
        // because a projected view is a direct call where the bindable interface goes through the
        // XAML interop shim.
        if (auto vector = source.try_as<winrt::Windows::Foundation::Collections::IVector<winrt::IInspectable>>())
        {
            m_vector = std::move(vector);
        }
        else if (auto vectorView = source.try_as<winrt::Windows::Foundation::Collections::IVectorView<winrt::IInspectable>>())
        {
            m_vectorView = std::move(vectorView);
        }
        else if (auto bindableVector = source.try_as<winrt::Microsoft::UI::Xaml::Interop::IBindableVector>())
        {
            m_bindableVector = std::move(bindableVector);
        }

        // Observe resolution, independent of the above: a source can be indexable through one
        // interface and observable through another.
        if (auto notifyCollectionChanged = source.try_as<winrt::Microsoft::UI::Xaml::Interop::INotifyCollectionChanged>())
        {
            m_notifyCollectionChanged = std::move(notifyCollectionChanged);
        }
        else if (auto observableVector = source.try_as<winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable>>())
        {
            m_observableVector = std::move(observableVector);
        }
        else if (auto bindableObservableVector = source.try_as<winrt::Microsoft::UI::Xaml::Interop::IBindableObservableVector>())
        {
            m_bindableObservableVector = std::move(bindableObservableVector);
        }
    }

    bool CollectionAccessor::IsIndexable() const noexcept
    {
        return m_vector || m_vectorView || m_bindableVector;
    }

    uint32_t CollectionAccessor::Count() const
    {
        if (m_vector)
        {
            return m_vector.Size();
        }
        if (m_vectorView)
        {
            return m_vectorView.Size();
        }
        if (m_bindableVector)
        {
            return m_bindableVector.Size();
        }
        return 0;
    }

    winrt::IInspectable CollectionAccessor::GetAtUnchecked(uint32_t index) const
    {
        if (m_vector)
        {
            return m_vector.GetAt(index);
        }
        if (m_vectorView)
        {
            return m_vectorView.GetAt(index);
        }
        return m_bindableVector.GetAt(index);
    }

    bool CollectionAccessor::TryGetAt(uint32_t index, winrt::IInspectable& item) const
    {
        item = nullptr;
        if (!IsIndexable() || index >= Count())
        {
            return false;
        }

        item = GetAtUnchecked(index);
        return true;
    }

    std::vector<winrt::IInspectable> CollectionAccessor::Enumerate(bool throwIfUnsupported) const
    {
        std::vector<winrt::IInspectable> result;
        if (!m_source)
        {
            return result;
        }

        // Indexed sources are walked positionally instead of through an iterator. Besides being the
        // cheaper path, it avoids the managed-iterator problem handled below entirely: a source can
        // have a working indexed surface and an unusable iterator.
        if (IsIndexable())
        {
            const uint32_t size = Count();
            result.reserve(size);
            for (uint32_t i = 0; i < size; ++i)
            {
                result.push_back(GetAtUnchecked(i));
            }
            return result;
        }

        // Enumerate-only sources: no indexed surface, so the iterator is the only way in.
        if (auto iterable = m_source.try_as<winrt::Windows::Foundation::Collections::IIterable<winrt::IInspectable>>())
        {
            try
            {
                for (auto const& item : iterable)
                {
                    result.push_back(item);
                }
                return result;
            }
            catch (winrt::hresult_error const& e)
            {
                // CLR compatibility: some managed sources project IIterable<Object>, but their
                // iterator returns E_NOINTERFACE from First()/HasCurrent(). Legacy XAML
                // (CollectionViewManager) retries such sources through IBindableIterable; mirror
                // that here. Discard any partial enumeration and fall through to the bindable
                // branch below. Any other failure is a genuine error and is rethrown.
                if (e.code() != E_NOINTERFACE)
                {
                    throw;
                }
                result.clear();
            }
        }

        if (auto bindableIterable = m_source.try_as<winrt::Microsoft::UI::Xaml::Interop::IBindableIterable>())
        {
            auto iterator = bindableIterable.First();
            while (iterator && iterator.HasCurrent())
            {
                result.push_back(iterator.Current());
                if (!iterator.MoveNext())
                {
                    break;
                }
            }
            return result;
        }

        if (throwIfUnsupported)
        {
            throw winrt::hresult_invalid_argument(
                L"items must implement a supported collection interface: IVector<IInspectable>, "
                L"IBindableVector, IIterable<IInspectable>, or IBindableIterable.");
        }

        return result;
    }

    void ApplyPredicateFilter(
        std::vector<winrt::IInspectable>& items,
        std::function<bool(winrt::IInspectable const& item)> const& predicate)
    {
        if (!predicate)
        {
            return;
        }

        std::vector<winrt::IInspectable> kept;
        kept.reserve(items.size());
        for (auto const& item : items)
        {
            bool keep = false;
            try
            {
                keep = predicate(item);
            }
            catch (...)
            {
                keep = false;
            }

            if (keep)
            {
                kept.push_back(item);
            }
        }
        items = std::move(kept);
    }

    void StableSortByKeys(
        std::vector<winrt::IInspectable>& items,
        size_t axisCount,
        std::function<winrt::IInspectable(winrt::IInspectable const& item, size_t axisIndex)> const& extractKey,
        std::function<winrt::SortDirection(size_t axisIndex)> const& axisDirection)
    {
        if (axisCount == 0 || items.size() < 2)
        {
            return;
        }

        // Decorate-sort-undecorate: extract each item's keys once up front (key extraction can be
        // O(reflection)), then run a cheap stable_sort over an index permutation. stable_sort
        // preserves the relative order of equal-key items (indices start in original order).
        //
        // The decoration is held as flat row-major arrays rather than a per-row struct of three
        // vectors: at 100k rows the latter is ~300k separate heap allocations before the sort even
        // starts, which dominates the sort itself.
        //
        // Reference keys fall through to a ToString-based tiebreak that may call app code.
        // FallbackKeys materializes it once here so the comparator stays cheap and, more
        // importantly, sees a frozen key set — a non-deterministic app ToString would otherwise
        // break the strict-weak-ordering precondition of stable_sort. The entry is empty (and its
        // HasFallbackKey flag false) for IPropertyValue keys, which never reach that tiebreak.
        const size_t n = items.size();
        std::vector<winrt::IInspectable> keys(n * axisCount);
        std::vector<winrt::hstring> fallbackKeys(n * axisCount);
        std::vector<char> hasFallbackKey(n * axisCount, 0);

        for (size_t i = 0; i < n; ++i)
        {
            const size_t rowBase = i * axisCount;
            for (size_t s = 0; s < axisCount; ++s)
            {
                auto key = extractKey(items[i], s);
                const bool needsFallback = TabularValueComparer::UsesFallbackKey(key);
                if (needsFallback)
                {
                    fallbackKeys[rowBase + s] = TabularValueKey::ToString(key);
                }
                hasFallbackKey[rowBase + s] = needsFallback ? 1 : 0;
                keys[rowBase + s] = std::move(key);
            }
        }

        std::vector<size_t> order(n);
        for (size_t i = 0; i < n; ++i)
        {
            order[i] = i;
        }

        std::stable_sort(order.begin(), order.end(),
            [&](size_t a, size_t b)
            {
                const size_t aBase = a * axisCount;
                const size_t bBase = b * axisCount;
                for (size_t s = 0; s < axisCount; ++s)
                {
                    const int cmp = TabularValueComparer::Compare(
                        keys[aBase + s],
                        keys[bBase + s],
                        hasFallbackKey[aBase + s] ? &fallbackKeys[aBase + s] : nullptr,
                        hasFallbackKey[bBase + s] ? &fallbackKeys[bBase + s] : nullptr);
                    if (cmp != 0)
                    {
                        return axisDirection(s) == winrt::SortDirection::Ascending ? (cmp < 0) : (cmp > 0);
                    }
                }
                return false;
            });

        std::vector<winrt::IInspectable> sorted;
        sorted.reserve(n);
        for (auto const index : order)
        {
            sorted.push_back(items[index]);
        }
        items = std::move(sorted);
    }

    bool BucketizeToGroups(
        std::vector<winrt::IInspectable> const& items,
        std::function<winrt::IInspectable(winrt::IInspectable const& item)> const& resolveKey,
        std::function<bool(winrt::IInspectable const& key, winrt::hstring& identity, wchar_t const*& reason)> const& resolveIdentity,
        std::function<bool(winrt::IInspectable const& existingKey, winrt::IInspectable const& newKey)> const& keysConsideredEqual,
        std::vector<KeyedBucket>& outBuckets,
        wchar_t const*& degradeReason)
    {
        outBuckets.clear();
        // identity string -> index into outBuckets, so first-seen group order is preserved by
        // outBuckets itself (no separate order vector needed).
        std::unordered_map<winrt::hstring, size_t> indexByIdentity;

        for (auto const& item : items)
        {
            winrt::IInspectable key = resolveKey ? resolveKey(item) : winrt::IInspectable{ nullptr };

            winrt::hstring identity;
            wchar_t const* reason = nullptr;
            if (!resolveIdentity || !resolveIdentity(key, identity, reason))
            {
                degradeReason = reason;
                return false;
            }

            auto it = indexByIdentity.find(identity);
            if (it == indexByIdentity.end())
            {
                indexByIdentity.emplace(identity, outBuckets.size());
                outBuckets.push_back(KeyedBucket{ key, identity, std::vector<winrt::IInspectable>{ item } });
            }
            else
            {
                auto& bucket = outBuckets[it->second];
                // A genuine identity COLLISION (two logically-different keys mapping to the same
                // identity string) forces a flat degrade; keysConsideredEqual lets the adapter
                // treat intentional shared identities (e.g. an app-supplied identity selector) as
                // the same group instead.
                if (keysConsideredEqual && !keysConsideredEqual(bucket.Key, key))
                {
                    degradeReason = L"duplicate group identity";
                    return false;
                }
                bucket.Items.push_back(item);
            }
        }

        return true;
    }

    uint32_t UpperBoundInsertIndex(
        uint32_t count,
        std::function<int(uint32_t existingIndex)> const& compareNewToExisting)
    {
        uint32_t lo = 0;
        uint32_t hi = count;
        while (lo < hi)
        {
            const uint32_t mid = lo + (hi - lo) / 2;
            if (compareNewToExisting(mid) < 0)
            {
                hi = mid;
            }
            else
            {
                lo = mid + 1;
            }
        }
        return lo;
    }

    int32_t CustomSortRankAdapter::SafeCompare(
        winrt::IInspectable const& left,
        winrt::IInspectable const& right) const
    {
        if (!m_comparer)
        {
            return 0;
        }

        try
        {
            const auto result = m_comparer(left, right);
            return result < 0 ? -1 : (result > 0 ? 1 : 0);
        }
        catch (...)
        {
            return 0;
        }
    }

    bool CustomSortRankAdapter::IndexComesBefore(
        std::vector<winrt::IInspectable> const& rows,
        uint64_t generation,
        size_t leftIndex,
        size_t rightIndex,
        bool& staleState) const
    {
        if (leftIndex == rightIndex)
        {
            return false;
        }

        const int32_t comparison = SafeCompare(rows[leftIndex], rows[rightIndex]);
        if (m_generation != generation)
        {
            staleState = true;
            return false;
        }
        if (comparison != 0)
        {
            return comparison < 0;
        }

        // Equal keys keep source order, which is what makes the sort stable.
        return leftIndex < rightIndex;
    }

    bool CustomSortRankAdapter::StableMergeSortOrder(
        std::vector<size_t>& order,
        std::vector<winrt::IInspectable> const& rows,
        uint64_t generation) const
    {
        order.resize(rows.size());
        std::iota(order.begin(), order.end(), size_t{ 0 });
        if (order.size() < 2)
        {
            return true;
        }

        std::vector<size_t> scratch(order.size());
        for (size_t width = 1; width < order.size(); width *= 2)
        {
            for (size_t lo = 0; lo < order.size(); lo += 2 * width)
            {
                const size_t mid = std::min(lo + width, order.size());
                const size_t hi = std::min(lo + 2 * width, order.size());
                size_t left = lo;
                size_t right = mid;
                size_t out = lo;
                while (left < mid && right < hi)
                {
                    bool staleState = false;
                    const bool takeRight = IndexComesBefore(rows, generation, order[right], order[left], staleState);
                    if (staleState)
                    {
                        return false;
                    }
                    scratch[out++] = takeRight ? order[right++] : order[left++];
                }
                while (left < mid)
                {
                    scratch[out++] = order[left++];
                }
                while (right < hi)
                {
                    scratch[out++] = order[right++];
                }
            }
            order.swap(scratch);
        }

        return true;
    }

    void CustomSortRankAdapter::ClearRanks()
    {
        ++m_generation;
        m_ranks.clear();
        m_rankByIdentity.clear();
    }

    void CustomSortRankAdapter::Reset()
    {
        m_comparer = nullptr;
        ClearRanks();
    }

    void CustomSortRankAdapter::Rank(
        TabularPairwiseComparer const& comparer,
        std::vector<winrt::IInspectable> const& rows)
    {
        // Set the flag before ClearRanks so a reentrant path cannot observe a torn intermediate.
        if (m_comparerActive)
        {
            MUX_ASSERT_MSG(false, "TableView custom sort comparer re-entered rank population. A sort predicate must be a pure function of its inputs.");
            return;
        }
        m_comparerActive = true;
        auto comparerGuard = wil::scope_exit([this]() noexcept
        {
            m_comparerActive = false;
        });

        m_comparer = comparer;
        ClearRanks();

        if (!m_comparer)
        {
            return;
        }
        const auto generation = m_generation;

        std::vector<size_t> order;
        if (!StableMergeSortOrder(order, rows, generation))
        {
            return;
        }

        // Rank, not position: equal items must share a rank or the projection would impose an
        // arbitrary order on a tie the comparer called equal.
        int32_t rank = 0;
        for (size_t i = 0; i < order.size(); ++i)
        {
            if (i > 0)
            {
                const int32_t comparison = SafeCompare(rows[order[i - 1]], rows[order[i]]);
                if (m_generation != generation)
                {
                    return;
                }
                if (comparison != 0)
                {
                    ++rank;
                }
            }

            auto const& item = rows[order[i]];
            m_ranks.push_back({ item, rank });
            if (auto unknown = item.try_as<::IUnknown>())
            {
                m_rankByIdentity[unknown.get()] = rank;
            }
        }
    }

    winrt::IInspectable CustomSortRankAdapter::KeyFor(winrt::IInspectable const& item)
    {
        if (!m_comparer)
        {
            return nullptr;
        }

        if (m_comparerActive)
        {
            MUX_ASSERT_MSG(false, "TableView custom sort comparer re-entered the sort infrastructure. A sort predicate must be a pure function of its inputs.");
            return nullptr;
        }
        m_comparerActive = true;
        auto comparerGuard = wil::scope_exit([this]() noexcept
        {
            m_comparerActive = false;
        });

        const auto generation = m_generation;

        if (auto unknown = item.try_as<::IUnknown>())
        {
            auto const found = m_rankByIdentity.find(unknown.get());
            if (found != m_rankByIdentity.end())
            {
                return winrt::box_value(found->second);
            }
        }

        // A row the rank pass never saw - added after the sort was applied. Find where it belongs
        // among the existing ranks and insert it there, shifting the ranks above it.
        int32_t rank = 0;
        for (size_t i = 0; i < m_ranks.size(); ++i)
        {
            auto const entryItem = m_ranks[i].Item;
            auto const entryRank = m_ranks[i].Rank;
            const int32_t comparison = SafeCompare(entryItem, item);
            if (m_generation != generation)
            {
                return nullptr;
            }
            if (comparison == 0)
            {
                return winrt::box_value(entryRank);
            }
            if (comparison < 0)
            {
                rank = std::max(rank, entryRank + 1);
            }
        }

        if (m_generation != generation)
        {
            return nullptr;
        }

        for (auto& entry : m_ranks)
        {
            if (entry.Rank >= rank)
            {
                ++entry.Rank;
                if (auto unknown = entry.Item.try_as<::IUnknown>())
                {
                    m_rankByIdentity[unknown.get()] = entry.Rank;
                }
            }
        }

        m_ranks.push_back({ item, rank });
        if (auto unknown = item.try_as<::IUnknown>())
        {
            m_rankByIdentity[unknown.get()] = rank;
        }
        return winrt::box_value(rank);
    }
}
