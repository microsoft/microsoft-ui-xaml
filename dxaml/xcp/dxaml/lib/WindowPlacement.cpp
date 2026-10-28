// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "WindowPlacement.h"
#include "Microsoft.Windows.Storage.h"
#include "StaticStore.h"
#include <appmodel.h>
#include <windows.storage.h>

#define WINUI_PLACEMENTEX
#include <PlacementEx\User32Utils.h>

using namespace DirectUI;
using namespace DirectUISynonyms;

// See docs\design-notes\window-placement.md for behavior and the private storage format.

namespace
{
    using PropertyMap = wfc::IMap<HSTRING, IInspectable*>;
    namespace storage = ABI::Microsoft::Windows::Storage;

    constexpr int c_recordVersion = 1;

    void GetPlacementKey(HSTRING id, wchar_t (&key)[19])
    {
        // Keep keys short without relying on settings to compare raw ids case-sensitively.
        // The private format uses 64-bit FNV-1a over UTF-16LE bytes, without a terminating null.
        // Keep this mapping stable across runs and versions so existing records remain reachable.
        UINT32 length = 0;
        const auto text = WindowsGetStringRawBuffer(id, &length);
        uint64_t hash = 14695981039346656037ull;
        for (UINT32 i = 0; i < length; ++i)
        {
            hash = (hash ^ (text[i] & 0xff)) * 1099511628211ull;
            hash = (hash ^ (text[i] >> 8)) * 1099511628211ull;
        }

        StringCchPrintfW(key, ARRAYSIZE(key), L"P_%016llx", hash);
    }

    bool ReadValue(
        PropertyMap* fields,
        PCWSTR name,
        wf::PropertyType type,
        wrl::ComPtr<wf::IPropertyValue>& value)
    {
        wrl::ComPtr<IInspectable> boxed;
        wf::PropertyType actual{};

        return SUCCEEDED(fields->Lookup(wrl_wrappers::HStringReference(name).Get(), &boxed)) &&
               boxed &&
               SUCCEEDED(boxed.As(&value)) &&
               SUCCEEDED(value->get_Type(&actual)) &&
               actual == type;
    }

    bool ReadInt32(PropertyMap* fields, PCWSTR name, INT32& result)
    {
        wrl::ComPtr<wf::IPropertyValue> value;

        return ReadValue(fields, name, wf::PropertyType_Int32, value) &&
               SUCCEEDED(value->GetInt32(&result));
    }

    bool ReadString(PropertyMap* fields, PCWSTR name, wrl_wrappers::HString& result)
    {
        wrl::ComPtr<wf::IPropertyValue> value;

        return ReadValue(fields, name, wf::PropertyType_String, value) &&
               SUCCEEDED(value->GetString(result.GetAddressOf()));
    }

    constexpr PCWSTR c_normalFields[] = { L"NormalX", L"NormalY", L"NormalWidth", L"NormalHeight" };
    constexpr PCWSTR c_workAreaFields[] = { L"WorkAreaX", L"WorkAreaY", L"WorkAreaWidth", L"WorkAreaHeight" };
    constexpr PCWSTR c_snapFields[] = { L"SnapX", L"SnapY", L"SnapWidth", L"SnapHeight" };

    bool IsValidRect(const RECT& rect)
    {
        const int64_t width = static_cast<int64_t>(rect.right) - rect.left;
        const int64_t height = static_cast<int64_t>(rect.bottom) - rect.top;

        return width > 0 && width <= 65535 && height > 0 && height <= 65535;
    }

    bool ReadRect(PropertyMap* fields, const PCWSTR (&names)[4], RECT& rect)
    {
        INT32 x, y, width, height;
        if (!ReadInt32(fields, names[0], x) ||
            !ReadInt32(fields, names[1], y) ||
            !ReadInt32(fields, names[2], width) ||
            !ReadInt32(fields, names[3], height) ||
            width <= 0 || width > 65535 ||
            height <= 0 || height > 65535 ||
            static_cast<int64_t>(x) + width > INT32_MAX ||
            static_cast<int64_t>(y) + height > INT32_MAX)
        {
            return false;
        }

        rect = { x, y, x + width, y + height };

        return true;
    }

    bool ReadPlacement(PropertyMap* fields, HSTRING id, PlacementEx& placement)
    {
        INT32 version, dpi, state;
        wrl_wrappers::HString savedId;
        INT32 comparison = 0;

        // The hash only selects a slot; the full id check prevents restoring a colliding id's placement.
        if (!ReadInt32(fields, L"Version", version) ||
            version != c_recordVersion ||
            !ReadString(fields, L"PlacementId", savedId) ||
            FAILED(WindowsCompareStringOrdinal(id, savedId.Get(), &comparison)) ||
            comparison != 0 ||
            !ReadRect(fields, c_normalFields, placement.normalRect) ||
            !ReadRect(fields, c_workAreaFields, placement.workArea) ||
            !ReadInt32(fields, L"Dpi", dpi) ||
            dpi < 96 || dpi > 9600 ||
            !ReadInt32(fields, L"State", state) ||
            state < 0 || state > 5)
        {
            return false;
        }

        placement.dpi = dpi;
        placement.showCmd = state == 1 ?
            SW_MAXIMIZE :
            (state == 2 || state == 4 || state == 5) ? SW_MINIMIZE : SW_NORMAL;

        if (state == 3)
        {
            placement.flags |= PlacementFlags::Arranged;
        }

        if (state == 4)
        {
            placement.flags |= PlacementFlags::RestoreToMaximized;
        }

        if (state == 5)
        {
            placement.flags |= PlacementFlags::RestoreToArranged;
        }

        bool hasSnap = state == 3 || state == 5;
        for (auto name : c_snapFields)
        {
            boolean present = false;
            if (FAILED(fields->HasKey(wrl_wrappers::HStringReference(name).Get(), &present)))
            {
                return false;
            }

            hasSnap = hasSnap || present;
        }

        if (hasSnap)
        {
            RECT intersection{};
            if (!ReadRect(fields, c_snapFields, placement.arrangeRect) ||
                !IntersectRect(&intersection, &placement.arrangeRect, &placement.workArea))
            {
                return false;
            }
        }

        boolean hasName = false;
        if (FAILED(fields->HasKey(wrl_wrappers::HStringReference(L"DisplayDeviceName").Get(), &hasName)))
        {
            return false;
        }

        if (hasName)
        {
            wrl_wrappers::HString name;
            if (!ReadString(fields, L"DisplayDeviceName", name))
            {
                return false;
            }

            UINT32 length = 0;
            const auto text = WindowsGetStringRawBuffer(name.Get(), &length);
            if (length >= ARRAYSIZE(placement.deviceName) || wmemchr(text, L'\0', length))
            {
                return false;
            }

            memcpy(placement.deviceName, text, length * sizeof(wchar_t));
        }

        return placement.IsValid();
    }

    HRESULT WriteInt32(PropertyMap* fields, PCWSTR name, INT32 value)
    {
        wrl::ComPtr<IInspectable> boxed;
        IFC_RETURN(StaticStore::GetPropertyValueStaticsNoRef()->CreateInt32(value, &boxed));

        boolean replaced;
        IFC_RETURN(fields->Insert(wrl_wrappers::HStringReference(name).Get(), boxed.Get(), &replaced));

        return S_OK;
    }

    HRESULT WriteString(PropertyMap* fields, PCWSTR name, HSTRING value)
    {
        wrl::ComPtr<IInspectable> boxed;
        IFC_RETURN(StaticStore::GetPropertyValueStaticsNoRef()->CreateString(value, &boxed));

        boolean replaced;
        IFC_RETURN(fields->Insert(wrl_wrappers::HStringReference(name).Get(), boxed.Get(), &replaced));

        return S_OK;
    }

    HRESULT WriteRect(PropertyMap* fields, const PCWSTR (&names)[4], const RECT& rect)
    {
        IFC_RETURN(WriteInt32(fields, names[0], rect.left));
        IFC_RETURN(WriteInt32(fields, names[1], rect.top));
        IFC_RETURN(WriteInt32(fields, names[2], rect.right - rect.left));
        IFC_RETURN(WriteInt32(fields, names[3], rect.bottom - rect.top));

        return S_OK;
    }
}

HRESULT WindowPlacement::GetId(HSTRING* value)
{
    IFC_RETURN(m_id.CopyTo(value));

    return S_OK;
}

HRESULT WindowPlacement::SetId(HSTRING value)
{
    IFC_RETURN(m_id.Set(value));

    if (!WindowsIsStringEmpty(value))
    {
        // Storage is optional; failure must not make a window property unusable.
        IGNOREHR(EnsureStorage());
    }

    return S_OK;
}

HRESULT WindowPlacement::EnsureStorage()
{
    if (m_settings)
    {
        return S_OK;
    }

    UINT32 length = 0;
    if (GetCurrentPackageFullName(&length, nullptr) != ERROR_INSUFFICIENT_BUFFER)
    {
        return S_OK;
    }

    wrl::ComPtr<storage::IApplicationDataStatics> factory;
    IFC_RETURN(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_Windows_Storage_ApplicationData).Get(),
        &factory));

    wrl::ComPtr<storage::IApplicationData> data;
    IFC_RETURN(factory->GetDefault(&data));

    wrl::ComPtr<storage::IApplicationDataContainer> settings, container;
    IFC_RETURN(data->get_LocalSettings(&settings));
    IFC_RETURN(settings->CreateContainer(
        wrl_wrappers::HStringReference(L"Microsoft.UI.Xaml.WindowPlacement").Get(),
        storage::ApplicationDataCreateDisposition_Always,
        &container));

    wrl::ComPtr<wfc::IPropertySet> values;
    IFC_RETURN(container->get_Values(&values));

    wrl::ComPtr<PropertyMap> map;
    IFC_RETURN(values.As(&map));

    // Instance activation is not reliable during synchronous shutdown messages.
    // Prepare the reusable composite before WM_DESTROY/WM_ENDSESSION.
    wrl::ComPtr<IInspectable> composite;
    IFC_RETURN(RoActivateInstance(
        wrl_wrappers::HStringReference(RuntimeClass_Windows_Storage_ApplicationDataCompositeValue).Get(),
        &composite));
    IFC_RETURN(composite.As(&m_record));

    m_settings = std::move(map);

    return S_OK;
}

bool WindowPlacement::TryRestore(HWND hwnd)
{
    if (WindowsIsStringEmpty(m_id.Get()))
    {
        return false;
    }

    IGNOREHR(EnsureStorage());
    if (!m_settings)
    {
        return false;
    }

    wrl::ComPtr<IInspectable> record;
    wrl::ComPtr<PropertyMap> fields;
    PlacementEx placement;
    wchar_t key[19]{};
    GetPlacementKey(m_id.Get(), key);
    
    // Ignore missing, unreadable, or invalid records so the caller can use fallback placement.
    if (FAILED(m_settings->Lookup(wrl_wrappers::HStringReference(key).Get(), &record)) ||
        !record ||
        FAILED(record.As(&fields)) ||
        !ReadPlacement(fields.Get(), m_id.Get(), placement))
    {
        return false;
    }

    // Reopen minimized windows at their restore target, including snap bounds when available.
    // PlacementEx needs Arranged, not just RestoreToArranged, to apply those bounds now.
    if (placement.HasFlag(PlacementFlags::RestoreToArranged))
    {
        placement.flags &= ~PlacementFlags::RestoreToArranged;
        placement.flags |= PlacementFlags::Arranged;
    }

    placement.RestoreIfMinimized();
    placement.flags |= PlacementFlags::KeepHidden | PlacementFlags::NoActivate;
    if (GetWindowLongPtr(hwnd, GWL_STYLE) & WS_THICKFRAME)
    {
        placement.flags |= PlacementFlags::AllowSizing;
    }

    // Apply the restored bounds and window state to the HWND.
    return PlacementEx::SetPlacement(hwnd, &placement);
}

HRESULT WindowPlacement::Save(HWND hwnd)
{
    // An empty id opts out. Storage is unavailable without package identity or if setup failed.
    // Saving is best effort; skip it rather than interfere with window shutdown.
    if (WindowsIsStringEmpty(m_id.Get()) || !m_settings)
    {
        return S_OK;
    }

    PlacementEx placement;
    // Keep the earlier record if capture fails or returns bounds/DPI that the reader would reject.
    // Full-screen placement is also excluded: phase 1 only saves windowed placement.
    if (!PlacementEx::GetPlacement(hwnd, &placement) ||
        !placement.IsValid() ||
        placement.HasFlag(PlacementFlags::FullScreen) ||
        !IsValidRect(placement.normalRect) ||
        !IsValidRect(placement.workArea) ||
        placement.dpi > 9600)
    {
        return S_OK;
    }

    const bool minimized = PlacementEx::IsMinimizeShowCmd(placement.showCmd);
    const bool arranged = placement.HasFlag(PlacementFlags::Arranged) ||
                          placement.HasFlag(PlacementFlags::RestoreToArranged);
    const INT32 state = minimized ?
        (placement.HasFlag(PlacementFlags::RestoreToMaximized) ? 4 : arranged ? 5 : 2) :
        placement.showCmd == SW_MAXIMIZE ? 1 : arranged ? 3 : 0;

    // Reject invalid snap bounds or bounds that do not overlap the saved work area.
    // Keep the earlier record; a minimized snapped window can report offscreen frame bounds.
    RECT intersection{};
    if (arranged &&
        (!IsValidRect(placement.arrangeRect) ||
         !IntersectRect(&intersection, &placement.arrangeRect, &placement.workArea)))
    {
        return S_OK;
    }

    // Clear the reused composite so an earlier snapped save cannot leave stale snap fields.
    IFC_RETURN(m_record->Clear());
    IFC_RETURN(WriteInt32(m_record.Get(), L"Version", c_recordVersion));
    IFC_RETURN(WriteString(m_record.Get(), L"PlacementId", m_id.Get()));
    IFC_RETURN(WriteRect(m_record.Get(), c_normalFields, placement.normalRect));
    IFC_RETURN(WriteRect(m_record.Get(), c_workAreaFields, placement.workArea));
    IFC_RETURN(WriteInt32(m_record.Get(), L"Dpi", placement.dpi));
    IFC_RETURN(WriteInt32(m_record.Get(), L"State", state));
    IFC_RETURN(WriteString(
        m_record.Get(),
        L"DisplayDeviceName",
        wrl_wrappers::HStringReference(placement.deviceName).Get()));

    if (arranged)
    {
        IFC_RETURN(WriteRect(m_record.Get(), c_snapFields, placement.arrangeRect));
    }

    boolean replaced;
    wchar_t key[19]{};
    GetPlacementKey(m_id.Get(), key);

    // Publish the complete composite once, rather than inserting a partially filled record.
    IFC_RETURN(m_settings->Insert(wrl_wrappers::HStringReference(key).Get(), m_record.Get(), &replaced));

    return S_OK;
}
