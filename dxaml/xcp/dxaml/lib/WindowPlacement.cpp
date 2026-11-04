// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "WindowPlacement.h"
#include "Microsoft.Windows.Storage.h"
#include "StaticStore.h"
#include <appmodel.h>
#include <type_traits>

#define WINUI_PLACEMENTEX
#include <PlacementEx\User32Utils.h>

using namespace DirectUI;
using namespace DirectUISynonyms;

// See docs\design-notes\window-placement.md for behavior and the private storage format.

namespace
{
    using PropertyMap = wfc::IMap<HSTRING, IInspectable*>;
    namespace storage = ABI::Microsoft::Windows::Storage;

    constexpr uint32_t c_recordVersion = 1;
    constexpr uint32_t c_maxRecordSize = 8192;
    constexpr uint32_t c_maxDpi = 9600;

    // These values are stored in version 1 records. Keep their numeric values stable.
    enum class SavedWindowState : uint32_t
    {
        Normal = 0,
        Maximized = 1,
        MinimizedFromNormal = 2,
        Snapped = 3,
        MinimizedFromMaximized = 4,
        MinimizedFromSnapped = 5,
    };

    // WARNING: This layout is part of the existing version 1 placement format and must not change.
    // Existing saved records depend on its field order, types, and offsets.
    // This is a private little-endian binary format, to save data for PlacementEx.
    // The full UTF-16 placement id string follows this fixed header, without a terminating null.
    struct PlacementRecord
    {
        uint32_t version;
        FILETIME savedAt;       // 8 bytes
        GUID virtualDesktopId;  // 16 bytes
        RECT normalRect;
        RECT workArea;
        RECT snapRect;
        uint32_t dpi;
        SavedWindowState state;
        wchar_t deviceName[32];
        uint32_t idLength;
        // ID string is stored immediately after this header, without a terminating null.
    };
    static_assert(std::is_trivially_copyable_v<PlacementRecord>);
    static_assert(sizeof(SavedWindowState) == 4);
    static_assert(sizeof(wchar_t) == 2);
    static_assert(sizeof(PlacementRecord) == 152);
    static_assert(offsetof(PlacementRecord, savedAt) == 4);
    static_assert(offsetof(PlacementRecord, normalRect) == 28);
    static_assert(offsetof(PlacementRecord, state) == 80);
    static_assert(offsetof(PlacementRecord, deviceName) == 84);
    static_assert(offsetof(PlacementRecord, idLength) == 148);

    // Computes a fixed-length key for the given placement id.
    // We use this key instead of the placement id string because the APIs we call are case-insensitive
    // and we want placement id to be case-sensitive.
    void GetPlacementKey(HSTRING id, _Out_writes_z_(19) wchar_t (&key)[19])
    {
        // WARNING: This key algorithm and its constants are part of the existing placement format
        // and must not change. Changing them would make existing saved records unreachable.
        // It uses 64-bit FNV-1a over UTF-16LE bytes, without a terminating null.
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

    bool IsValidRect(const RECT& rect)
    {
        const int64_t width = static_cast<int64_t>(rect.right) - rect.left;
        const int64_t height = static_cast<int64_t>(rect.bottom) - rect.top;

        return width > 0 && width <= 65535 && height > 0 && height <= 65535;
    }

    UINT GetShowCommand(SavedWindowState state)
    {
        if (state == SavedWindowState::Maximized)
        {
            return SW_MAXIMIZE;
        }

        if (state == SavedWindowState::MinimizedFromNormal ||
            state == SavedWindowState::MinimizedFromMaximized ||
            state == SavedWindowState::MinimizedFromSnapped)
        {
            return SW_MINIMIZE;
        }

        return SW_NORMAL;
    }

    HRESULT ReadPlacement(
        IInspectable* boxed,
        HSTRING placementId,
        _Inout_ PlacementEx& placement,
        _Inout_ WindowPlacement::PlacementResult& result)
    {
        wrl::ComPtr<wf::IPropertyValue> value;
        IFC_RETURN(boxed->QueryInterface(IID_PPV_ARGS(&value)));

        wf::PropertyType type{};
        IFC_RETURN(value->get_Type(&type));
        IFCEXPECTRC_RETURN(type == wf::PropertyType_UInt8Array, HRESULT_FROM_WIN32(ERROR_INVALID_DATA));

        UINT32 size = 0;
        wil::unique_cotaskmem_ptr<BYTE[]> bytes;
        IFC_RETURN(value->GetUInt8Array(&size, wil::out_param(bytes)));
        IFCEXPECTRC_RETURN(size >= sizeof(PlacementRecord) && size <= c_maxRecordSize, HRESULT_FROM_WIN32(ERROR_INVALID_DATA));

        PlacementRecord record{};
        memcpy(&record, bytes.get(), sizeof(record));

        UINT32 placementIdLength = 0;
        const WCHAR* placementIdStr = WindowsGetStringRawBuffer(placementId, &placementIdLength);

        // Check the byte count before comparing the full id. The hash only selects a slot;
        // the exact UTF-16 comparison prevents restoring a colliding id's placement.
        IFCEXPECTRC_RETURN(record.version == c_recordVersion, HRESULT_FROM_WIN32(ERROR_INVALID_DATA));
        IFCEXPECTRC_RETURN(record.idLength > 0 && record.idLength <= MAX_PATH, HRESULT_FROM_WIN32(ERROR_INVALID_DATA));
        IFCEXPECTRC_RETURN(record.idLength == (size - sizeof(record)) / sizeof(wchar_t), HRESULT_FROM_WIN32(ERROR_INVALID_DATA));
        IFCEXPECTRC_RETURN((size - sizeof(record)) % sizeof(wchar_t) == 0, HRESULT_FROM_WIN32(ERROR_INVALID_DATA));
        if (record.idLength != placementIdLength ||
            memcmp(bytes.get() + sizeof(record), placementIdStr, placementIdLength * sizeof(wchar_t)) != 0)
        {
            result |= WindowPlacement::PlacementResult::HashCollision;
            IFC_RETURN(HRESULT_FROM_WIN32(ERROR_INVALID_DATA));
        }
        IFCEXPECTRC_RETURN(IsValidRect(record.normalRect), HRESULT_FROM_WIN32(ERROR_INVALID_DATA));
        IFCEXPECTRC_RETURN(IsValidRect(record.workArea), HRESULT_FROM_WIN32(ERROR_INVALID_DATA));
        IFCEXPECTRC_RETURN(record.dpi >= 96 && record.dpi <= c_maxDpi, HRESULT_FROM_WIN32(ERROR_INVALID_DATA));
        IFCEXPECTRC_RETURN(record.state <= SavedWindowState::MinimizedFromSnapped, HRESULT_FROM_WIN32(ERROR_INVALID_DATA));

        const WCHAR* nameEnd = wmemchr(record.deviceName, L'\0', ARRAYSIZE(record.deviceName));
        IFCEXPECTRC_RETURN(nameEnd, HRESULT_FROM_WIN32(ERROR_INVALID_DATA));

        // Require unused characters after the display name's terminating null to be zero,
        // matching the format produced by Save.
        for (auto ch = nameEnd; ch != std::end(record.deviceName); ++ch)
        {
            IFCEXPECTRC_RETURN(*ch == L'\0', HRESULT_FROM_WIN32(ERROR_INVALID_DATA));
        }

        placement.normalRect = record.normalRect;
        placement.workArea = record.workArea;
        placement.dpi = record.dpi;
        const auto state = record.state;
        placement.showCmd = GetShowCommand(state);

        if (state == SavedWindowState::Snapped)
        {
            placement.flags |= PlacementFlags::Arranged;
        }

        if (state == SavedWindowState::MinimizedFromMaximized)
        {
            placement.flags |= PlacementFlags::RestoreToMaximized;
        }

        if (state == SavedWindowState::MinimizedFromSnapped)
        {
            placement.flags |= PlacementFlags::RestoreToArranged;
        }

        const RECT emptyRect{};
        const bool hasSnap = state == SavedWindowState::Snapped ||
                             state == SavedWindowState::MinimizedFromSnapped ||
                             !EqualRect(&record.snapRect, &emptyRect);
        if (hasSnap)
        {
            RECT intersection{};
            IFCEXPECTRC_RETURN(IsValidRect(record.snapRect), HRESULT_FROM_WIN32(ERROR_INVALID_DATA));
            IFCEXPECTRC_RETURN(IntersectRect(&intersection, &record.snapRect, &placement.workArea), HRESULT_FROM_WIN32(ERROR_INVALID_DATA));
            placement.arrangeRect = record.snapRect;
        }

        static_assert(sizeof(placement.deviceName) == sizeof(record.deviceName), "Device name size mismatch");
        memcpy(placement.deviceName, record.deviceName, sizeof(record.deviceName));

        // savedAt is metadata only. virtualDesktopId is reserved and is not restored yet.

        IFCEXPECTRC_RETURN(placement.IsValid(), HRESULT_FROM_WIN32(ERROR_INVALID_DATA));

        return S_OK;
    }

    bool HasHashCollision(PropertyMap* settings, HSTRING key, HSTRING placementId)
    {
        // Collision reporting is best effort. A missing or unreadable record must not block saving.
        boolean hasRecord = false;
        if (FAILED(settings->HasKey(key, &hasRecord)) || !hasRecord)
        {
            return false;
        }

        wrl::ComPtr<IInspectable> existing;
        if (FAILED(settings->Lookup(key, &existing)) || !existing)
        {
            return false;
        }

        using PlacementResult = WindowPlacement::PlacementResult;
        PlacementResult result = PlacementResult::None;
        PlacementEx placement;
        IGNOREHR(ReadPlacement(existing.Get(), placementId, placement, result));
        return (result & PlacementResult::HashCollision) != PlacementResult::None;
    }
}

HRESULT WindowPlacement::GetId(_Out_ HSTRING* value)
{
    IFC_RETURN(m_id.CopyTo(value));

    return S_OK;
}

HRESULT WindowPlacement::SetId(_In_opt_ HSTRING value)
{
    IFCEXPECTRC_RETURN(WindowsGetStringLen(value) <= MAX_PATH, E_INVALIDARG);
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

    m_settings = std::move(map);

    return S_OK;
}

HRESULT WindowPlacement::TryLoadAndApply(HWND hwnd, _Out_ PlacementResult* result)
{
    IFCPTR_RETURN(result);
    *result = PlacementResult::None;

    if (WindowsIsStringEmpty(m_id.Get()))
    {
        // An empty string means placement is disabled, but this is not an error.
        return S_OK;
    }

    IFC_RETURN(EnsureStorage());
    if (!m_settings)
    {
        PCWSTR message = L"Window.PlacementId is set, but this app does not have package identity. WinUI automatic window placement requires package identity.";
        ::RoOriginateError(
            HRESULT_FROM_WIN32(APPMODEL_ERROR_NO_PACKAGE),
            wrl_wrappers::HStringReference(message).Get());
        ::OutputDebugStringW(message);
        ::OutputDebugStringW(L"\n");
        // Placement is best effort. Clear the WinRT error details after notifying the debugger.
        ::RoClearError();
        return S_OK;
    }

    wrl::ComPtr<IInspectable> record;
    PlacementEx placement;
    wchar_t key[19]{};
    GetPlacementKey(m_id.Get(), key);

    // A missing record is normal, so do not use a failing Lookup to detect it.
    const auto keyString = wrl_wrappers::HStringReference(key);
    boolean hasRecord = false;
    IFC_RETURN(m_settings->HasKey(keyString.Get(), &hasRecord));
    if (!hasRecord)
    {
        return S_OK;
    }

    const HRESULT lookupResult = m_settings->Lookup(keyString.Get(), &record);
    if (lookupResult == E_BOUNDS)
    {
        // Another window can remove the record between HasKey and Lookup.
        return S_OK;
    }

    IFC_RETURN(lookupResult);
    IFCEXPECTRC_RETURN(record.Get(), HRESULT_FROM_WIN32(ERROR_INVALID_DATA));
    IFC_RETURN(ReadPlacement(record.Get(), m_id.Get(), placement, *result));

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
    IFCEXPECTRC_RETURN(PlacementEx::SetPlacement(hwnd, &placement), E_FAIL);

    *result |= PlacementResult::LoadedAndApplied;

    return S_OK;
}

HRESULT WindowPlacement::Save(HWND hwnd, _Out_ PlacementResult* result)
{
    IFCPTR_RETURN(result);
    *result = PlacementResult::None;

    // An empty id opts out. Storage is unavailable without package identity or if setup failed.
    // Saving is best effort; skip it rather than interfere with window shutdown.
    if (WindowsIsStringEmpty(m_id.Get()) || !m_settings)
    {
        return S_OK;
    }

    PlacementEx placement;
    // Keep the earlier record if capture fails or returns bounds/DPI that the reader would reject.
    IFCEXPECTRC_RETURN(PlacementEx::GetPlacement(hwnd, &placement), E_FAIL);

    // Full-screen placement is also excluded: phase 1 only saves overlapped-window placement.
    if (placement.HasFlag(PlacementFlags::FullScreen))
    {
        return S_OK;
    }

    IFCEXPECTRC_RETURN(placement.IsValid(), HRESULT_FROM_WIN32(ERROR_INVALID_DATA));
    IFCEXPECTRC_RETURN(IsValidRect(placement.normalRect), HRESULT_FROM_WIN32(ERROR_INVALID_DATA));
    IFCEXPECTRC_RETURN(IsValidRect(placement.workArea), HRESULT_FROM_WIN32(ERROR_INVALID_DATA));
    IFCEXPECTRC_RETURN(placement.dpi <= c_maxDpi, HRESULT_FROM_WIN32(ERROR_INVALID_DATA));

    const bool minimized = PlacementEx::IsMinimizeShowCmd(placement.showCmd);
    const bool arranged = placement.HasFlag(PlacementFlags::Arranged) ||
                          placement.HasFlag(PlacementFlags::RestoreToArranged);
    SavedWindowState state;
    if (minimized && placement.HasFlag(PlacementFlags::RestoreToMaximized))
    {
        state = SavedWindowState::MinimizedFromMaximized;
    }
    else if (minimized)
    {
        state = arranged ? SavedWindowState::MinimizedFromSnapped : SavedWindowState::MinimizedFromNormal;
    }
    else if (placement.showCmd == SW_MAXIMIZE)
    {
        state = SavedWindowState::Maximized;
    }
    else
    {
        state = arranged ? SavedWindowState::Snapped : SavedWindowState::Normal;
    }

    // Reject invalid snap bounds or bounds that do not overlap the saved work area.
    // Keep the earlier record; a minimized snapped window can report offscreen frame bounds.
    RECT intersection{};
    if (arranged)
    {
        IFCEXPECTRC_RETURN(IsValidRect(placement.arrangeRect), HRESULT_FROM_WIN32(ERROR_INVALID_DATA));
        IFCEXPECTRC_RETURN(IntersectRect(&intersection, &placement.arrangeRect, &placement.workArea), HRESULT_FROM_WIN32(ERROR_INVALID_DATA));
    }

    UINT32 idLength = 0;
    const wchar_t* idText = WindowsGetStringRawBuffer(m_id.Get(), &idLength);
    // A byte-array setting is limited to 8 KB. Do not truncate an id or replace a valid earlier
    // record if the full id cannot fit alongside the header.
    IFCEXPECTRC_RETURN(idLength <= (c_maxRecordSize - sizeof(PlacementRecord)) / sizeof(wchar_t),
        HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW));

    PlacementRecord record{};
    record.version = c_recordVersion;
    record.idLength = idLength;
    GetSystemTimeAsFileTime(&record.savedAt);
    record.normalRect = placement.normalRect;
    record.workArea = placement.workArea;
    record.snapRect = arranged ? placement.arrangeRect : RECT{};
    record.dpi = placement.dpi;
    record.state = state;
    IFC_RETURN(StringCchCopyW(record.deviceName, ARRAYSIZE(record.deviceName), placement.deviceName));

    const UINT32 size = static_cast<UINT32>(sizeof(record) + idLength * sizeof(wchar_t));
    std::unique_ptr<BYTE[]> bytes(new (std::nothrow) BYTE[size]);
    IFCPTR_RETURN(bytes.get());
    memcpy(bytes.get(), &record, sizeof(record));
    memcpy(bytes.get() + sizeof(record), idText, idLength * sizeof(wchar_t));

    wrl::ComPtr<IInspectable> boxed;
    IFC_RETURN(StaticStore::GetPropertyValueStaticsNoRef()->CreateUInt8Array(size, bytes.get(), &boxed));

    boolean replaced;
    wchar_t key[19]{};
    GetPlacementKey(m_id.Get(), key);
    const auto keyString = wrl_wrappers::HStringReference(key);
    const bool collision = HasHashCollision(m_settings.Get(), keyString.Get(), m_id.Get());

    // Publish the complete binary record with one insertion.
    IFC_RETURN(m_settings->Insert(keyString.Get(), boxed.Get(), &replaced));
    if (collision)
    {
        *result |= PlacementResult::HashCollision;
    }

    return S_OK;
}
