// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "Source/SourceRecord.h"

#include <string_view>

#include <winrt/Microsoft.UI.Xaml.Data.h>

namespace
{
    // Identity() format: "0x" followed by Address in lowercase hex, no leading zeros.
    constexpr std::wstring_view c_identityPrefix{ L"0x" };
    constexpr size_t c_maxHexDigits = 2 * sizeof(ObjectAddress);
    constexpr wchar_t c_hexDigits[] = L"0123456789abcdef";
}

SourceRecord::SourceRecord(
    ITrackerHandleManager const* owner,
    winrt::Windows::Foundation::IInspectable const& item)
    : ShapedNode(ShapedNodeKind::Record)
    , Item(owner)
    , Address(AddressOf(item))
{
    Item.set(item);
}

SourceRecord::~SourceRecord()
{
    if (!PropertyChangedToken.value)
    {
        return;
    }

    // Teardown path: the item may already have been collected by the GC, so read it only through
    // safe_get(), never get(). A collected item has no subscription left to revoke.
    try
    {
        if (auto const notifier = Item.safe_get().try_as<winrt::Microsoft::UI::Xaml::Data::INotifyPropertyChanged>())
        {
            notifier.PropertyChanged(PropertyChangedToken);
        }
    }
    catch (...)
    {
        // A destructor never throws; a failed revoke leaves nothing for the record to clean up.
    }
}

ObjectAddress SourceRecord::AddressOf(winrt::Windows::Foundation::IInspectable const& item)
{
    return reinterpret_cast<ObjectAddress>(winrt::get_abi(item.as<winrt::Windows::Foundation::IUnknown>()));
}

winrt::hstring const& SourceRecord::Identity() const
{
    if (m_identity.empty())
    {
        wchar_t buffer[c_identityPrefix.size() + c_maxHexDigits];
        wchar_t* const end = buffer + std::size(buffer);
        wchar_t* first = end;

        ObjectAddress value = Address;
        do
        {
            *--first = c_hexDigits[value & 0xF];
            value >>= 4;
        } while (value != 0);

        first -= c_identityPrefix.size();
        c_identityPrefix.copy(first, c_identityPrefix.size());

        m_identity = winrt::hstring{ std::wstring_view{ first, static_cast<size_t>(end - first) } };
    }

    return m_identity;
}

std::optional<ObjectAddress> SourceRecord::TryParseIdentity(winrt::hstring const& identity) noexcept
{
    // Exact inverse of Identity(): the prefix, then 1 to c_maxHexDigits lowercase hex digits.
    // Anything else (a "g|" group identity, an app string) is not a record identity.
    std::wstring_view const text{ identity };
    if (!text.starts_with(c_identityPrefix))
    {
        return std::nullopt;
    }

    std::wstring_view const digits = text.substr(c_identityPrefix.size());
    if (digits.empty() || digits.size() > c_maxHexDigits)
    {
        return std::nullopt;
    }

    ObjectAddress value = 0;
    for (wchar_t const c : digits)
    {
        ObjectAddress digit;
        if (c >= L'0' && c <= L'9')
        {
            digit = static_cast<ObjectAddress>(c - L'0');
        }
        else if (c >= L'a' && c <= L'f')
        {
            digit = static_cast<ObjectAddress>(c - L'a' + 10);
        }
        else
        {
            return std::nullopt;
        }

        value = (value << 4) | digit;
    }

    return value;
}
