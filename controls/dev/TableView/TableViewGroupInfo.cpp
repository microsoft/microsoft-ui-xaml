// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableViewGroupInfo.h"
#include "TableViewGroupingHelpers.h"
#include "ResourceAccessor.h"

#include <cmath>
#include <string>
#include <winrt/Windows.UI.Xaml.Interop.h>

namespace
{
    // Every step here can fail on a locale-starved or self-contained host, and this runs inside
    // a binding getter during measure -- so nothing is allowed to escape.
    winrt::hstring FormatGroupHeaderCountText(int32_t groupItemCount) noexcept
    {
        std::wstring countFormat;
        try
        {
            countFormat = ResourceAccessor::GetLocalizedStringResource(SR_TableViewGroupHeaderCountFormat);
        }
        catch (...)
        {
        }

        if (countFormat.empty())
        {
            countFormat = L"({0})";
        }

        const auto formattedCount = TableViewDetails::FormatIntegerForCurrentCulture(groupItemCount);
        if (const auto placeholderIndex = countFormat.find(L"{0}"); placeholderIndex != std::wstring::npos)
        {
            countFormat.replace(placeholderIndex, 3, formattedCount.c_str());
        }
        else
        {
            countFormat.append(formattedCount.c_str());
        }

        return winrt::hstring{ countFormat };
    }

    class GroupInfoProperty :
        public winrt::implements<GroupInfoProperty, winrt::Microsoft::UI::Xaml::Data::ICustomProperty>
    {
    public:
        using Getter = winrt::IInspectable (*)(TableViewGroupInfo&);

        GroupInfoProperty(wchar_t const* name, winrt::TypeName const& type, Getter getter)
            : m_name(name), m_type(type), m_getter(getter)
        {
        }

        winrt::TypeName Type() { return m_type; }
        winrt::hstring Name() { return m_name; }
        bool CanRead() { return true; }
        bool CanWrite() { return false; }

        winrt::IInspectable GetValue(winrt::IInspectable const& target)
        {
            return m_getter(*winrt::get_self<TableViewGroupInfo>(target.as<winrt::TableViewGroupInfo>()));
        }

        void SetValue(winrt::IInspectable const&, winrt::IInspectable const&)
        {
            throw winrt::hresult_not_implemented();
        }

        winrt::IInspectable GetIndexedValue(winrt::IInspectable const&, winrt::IInspectable const&)
        {
            throw winrt::hresult_not_implemented();
        }

        void SetIndexedValue(winrt::IInspectable const&, winrt::IInspectable const&, winrt::IInspectable const&)
        {
            throw winrt::hresult_not_implemented();
        }

    private:
        winrt::hstring m_name;
        winrt::TypeName m_type;
        Getter m_getter;
    };
}

TableViewGroupInfo::TableViewGroupInfo(
    winrt::IInspectable const& key,
    int32_t itemCount,
    int32_t level,
    bool isExpandable,
    bool isExpanded,
    winrt::hstring const& keyText)
    : m_itemCount(itemCount)
    , m_level(level)
    , m_isExpandable(isExpandable)
    , m_isExpanded(isExpanded)
    , m_keyText(keyText)
{
    m_key = key;
}

winrt::IInspectable TableViewGroupInfo::Key()
{
    return m_key;
}

int32_t TableViewGroupInfo::ItemCount()
{
    return m_itemCount;
}

int32_t TableViewGroupInfo::Level()
{
    return m_level;
}

bool TableViewGroupInfo::IsExpandable()
{
    return m_isExpandable;
}

bool TableViewGroupInfo::IsExpanded()
{
    return m_isExpanded;
}

winrt::hstring TableViewGroupInfo::KeyText()
{
    return m_keyText;
}

winrt::hstring TableViewGroupInfo::ItemCountText()
{
    if (!m_hasItemCountText)
    {
        m_itemCountText = FormatGroupHeaderCountText(m_itemCount);
        m_hasItemCountText = true;
    }
    return m_itemCountText;
}

winrt::event_token TableViewGroupInfo::PropertyChanged(winrt::PropertyChangedEventHandler const& value)
{
    return m_propertyChanged.add(value);
}

void TableViewGroupInfo::PropertyChanged(winrt::event_token const& token)
{
    m_propertyChanged.remove(token);
}

void TableViewGroupInfo::RaisePropertyChanged(wchar_t const* propertyName)
{
    m_propertyChanged(*this, winrt::PropertyChangedEventArgs{ winrt::hstring{ propertyName } });
}

winrt::TypeName TableViewGroupInfo::Type()
{
    return { winrt::get_class_name(*this), winrt::TypeKind::Metadata };
}

winrt::Microsoft::UI::Xaml::Data::ICustomProperty TableViewGroupInfo::GetCustomProperty(winrt::hstring const& name)
{
    static const struct
    {
        wchar_t const* name;
        winrt::TypeName (*type)();
        GroupInfoProperty::Getter getter;
    } properties[] =
    {
        { L"Key", &winrt::xaml_typename<winrt::IInspectable>, [](TableViewGroupInfo& info) { return info.Key(); } },
        { L"KeyText", &winrt::xaml_typename<winrt::hstring>, [](TableViewGroupInfo& info) { return winrt::box_value(info.KeyText()); } },
        { L"ItemCount", &winrt::xaml_typename<int32_t>, [](TableViewGroupInfo& info) { return winrt::box_value(info.ItemCount()); } },
        { L"ItemCountText", &winrt::xaml_typename<winrt::hstring>, [](TableViewGroupInfo& info) { return winrt::box_value(info.ItemCountText()); } },
        { L"Level", &winrt::xaml_typename<int32_t>, [](TableViewGroupInfo& info) { return winrt::box_value(info.Level()); } },
        { L"IsExpandable", &winrt::xaml_typename<bool>, [](TableViewGroupInfo& info) { return winrt::box_value(info.IsExpandable()); } },
        { L"IsExpanded", &winrt::xaml_typename<bool>, [](TableViewGroupInfo& info) { return winrt::box_value(info.IsExpanded()); } },
    };

    for (auto const& property : properties)
    {
        if (name == property.name)
        {
            return winrt::make<GroupInfoProperty>(property.name, property.type(), property.getter);
        }
    }
    return nullptr;
}

winrt::Microsoft::UI::Xaml::Data::ICustomProperty TableViewGroupInfo::GetIndexedProperty(winrt::hstring const&, winrt::TypeName const&)
{
    return nullptr;
}

winrt::hstring TableViewGroupInfo::GetStringRepresentation()
{
    return m_keyText;
}

winrt::hstring TableViewGroupInfo::ToString()
{
    return m_keyText;
}

// C++/WinRT's operator== compares the ABI pointer of the interface currently held, with no
// QI, so the same object reached through different interfaces compares unequal. Boxed keys
// are also re-created per projection pass, so two *equal* keys are distinct objects --
// identity alone would still raise a spurious change every render. Compare boxed values by
// value first, then fall back to COM identity for reference keys.
bool SameGroupKey(winrt::IInspectable const& left, winrt::IInspectable const& right) noexcept
{
    if (!left || !right)
    {
        return !left && !right;
    }

    try
    {
        auto const leftValue = left.try_as<winrt::IPropertyValue>();
        auto const rightValue = right.try_as<winrt::IPropertyValue>();
        if (leftValue && rightValue)
        {
            if (leftValue.Type() != rightValue.Type())
            {
                return false;
            }

            switch (leftValue.Type())
            {
            case winrt::PropertyType::String:
                return leftValue.GetString() == rightValue.GetString();
            case winrt::PropertyType::Int32:
                return leftValue.GetInt32() == rightValue.GetInt32();
            case winrt::PropertyType::Int64:
                return leftValue.GetInt64() == rightValue.GetInt64();
            case winrt::PropertyType::UInt32:
                return leftValue.GetUInt32() == rightValue.GetUInt32();
            case winrt::PropertyType::UInt64:
                return leftValue.GetUInt64() == rightValue.GetUInt64();
            case winrt::PropertyType::Boolean:
                return leftValue.GetBoolean() == rightValue.GetBoolean();
            // NaN != NaN, but a NaN key is still one group; treat it as equal so the cached
            // projection is updated in place rather than replaced.
            case winrt::PropertyType::Double:
            {
                auto const l = leftValue.GetDouble();
                auto const r = rightValue.GetDouble();
                return l == r || (std::isnan(l) && std::isnan(r));
            }
            case winrt::PropertyType::Single:
            {
                auto const l = leftValue.GetSingle();
                auto const r = rightValue.GetSingle();
                return l == r || (std::isnan(l) && std::isnan(r));
            }
            case winrt::PropertyType::UInt8:
                return leftValue.GetUInt8() == rightValue.GetUInt8();
            case winrt::PropertyType::Int16:
                return leftValue.GetInt16() == rightValue.GetInt16();
            case winrt::PropertyType::UInt16:
                return leftValue.GetUInt16() == rightValue.GetUInt16();
            case winrt::PropertyType::Char16:
                return leftValue.GetChar16() == rightValue.GetChar16();
            // Grouping by a date or duration is common (orders by day, tasks by elapsed
            // time); without these the key falls to identity and re-raises every render.
            case winrt::PropertyType::DateTime:
                return leftValue.GetDateTime() == rightValue.GetDateTime();
            case winrt::PropertyType::TimeSpan:
                return leftValue.GetTimeSpan() == rightValue.GetTimeSpan();
            case winrt::PropertyType::Guid:
                return leftValue.GetGuid() == rightValue.GetGuid();
            default:
                break;
            }
        }

        auto const leftUnknown = left.try_as<::IUnknown>();
        auto const rightUnknown = right.try_as<::IUnknown>();
        return leftUnknown && rightUnknown && leftUnknown.get() == rightUnknown.get();
    }
    catch (...)
    {
        return false;
    }
}

void TableViewGroupInfo::UpdateInternal(
    winrt::IInspectable const& key,
    int32_t itemCount,
    int32_t level,
    winrt::hstring const& keyText)
{
    if (!SameGroupKey(m_key, key))
    {
        m_key = key;
        RaisePropertyChanged(L"Key");
    }

    if (m_keyText != keyText)
    {
        m_keyText = keyText;
        RaisePropertyChanged(L"KeyText");
    }

    if (m_itemCount != itemCount)
    {
        m_itemCount = itemCount;
        // Invalidate the derived string too; recomputed only if something binds it.
        m_hasItemCountText = false;
        RaisePropertyChanged(L"ItemCount");
        RaisePropertyChanged(L"ItemCountText");
    }

    if (m_level != level)
    {
        m_level = level;
        RaisePropertyChanged(L"Level");
    }
}

void TableViewGroupInfo::SetExpansionInternal(bool isExpandable, bool isExpanded)
{
    if (m_isExpandable != isExpandable)
    {
        m_isExpandable = isExpandable;
        RaisePropertyChanged(L"IsExpandable");
    }

    if (m_isExpanded != isExpanded)
    {
        m_isExpanded = isExpanded;
        RaisePropertyChanged(L"IsExpanded");
    }
}
