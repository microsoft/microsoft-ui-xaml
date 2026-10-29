// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "Source/SourceRecord.h"

#include <cwchar>
#include <string_view>

#include <winrt/Microsoft.UI.Xaml.Data.h>

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
    // Teardown never calls Item.get(): the item may already be collected. A record subscribed
    // for live shaping (SourceTable::SubscribeRecordPropertyChanged) is revoked here. Revoke
    // failures are swallowed.
    if (PropertyChangedToken.value)
    {
        try
        {
            if (auto const notifier = Item.safe_get().try_as<winrt::Microsoft::UI::Xaml::Data::INotifyPropertyChanged>())
            {
                notifier.PropertyChanged(PropertyChangedToken);
            }
        }
        catch (...)
        {
        }
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
        wchar_t buffer[2 + 2 * sizeof(ObjectAddress) + 1]{};
        swprintf_s(buffer, L"0x%llx", static_cast<unsigned long long>(Address));
        m_identity = buffer;
    }

    return m_identity;
}

std::optional<ObjectAddress> SourceRecord::TryParseIdentity(winrt::hstring const& identity) noexcept
{
    // The exact inverse of Identity(): "0x" and 1 to 2 * sizeof(ObjectAddress) lowercase hex
    // digits. Anything else (a group identity, an app string) is not a record identity.
    std::wstring_view const text{ identity };
    if (text.size() < 3 || text.size() > 2 + 2 * sizeof(ObjectAddress) || text[0] != L'0' || text[1] != L'x')
    {
        return std::nullopt;
    }

    ObjectAddress value = 0;
    for (wchar_t const c : text.substr(2))
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
