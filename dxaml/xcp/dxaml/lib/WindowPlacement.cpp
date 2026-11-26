// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "WindowPlacement.h"
#include "Microsoft.Windows.Storage.h"
#include <Microsoft.UI.Interop.h>
#include "StaticStore.h"
#include <appmodel.h>
#include <sddl.h>
#include <shobjidl.h>
#include <type_traits>

#pragma comment(lib, "advapi32.lib")

#define WINUI_PLACEMENTEX
#include <PlacementEx\User32Utils.h>

using namespace DirectUI;
using namespace DirectUISynonyms;

// See docs\design-notes\window-placement.md for behavior and the private storage format.

namespace
{
    using PropertyMap = wfc::IMap<HSTRING, IInspectable*>;
    namespace storage = ABI::Microsoft::Windows::Storage;
    namespace windowing = ABI::Microsoft::UI::Windowing;
    using PlacementDetails = windowing::IAppWindowPlacementDetails;

    constexpr uint32_t c_recordVersion = 1;
    constexpr uint32_t c_maxRecordSize = 8192;
    constexpr uint32_t c_maxDpi = 9600;

    HRESULT OpenCascadeInitializationMutex(_Out_ wil::unique_handle& mutex)
    {
        wil::unique_handle token;
        IFCEXPECTRC_RETURN(OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token), HRESULT_FROM_WIN32(GetLastError()));
        DWORD size = 0;
        GetTokenInformation(token.get(), TokenUser, nullptr, 0, &size);
        IFCEXPECTRC_RETURN(GetLastError() == ERROR_INSUFFICIENT_BUFFER, HRESULT_FROM_WIN32(GetLastError()));
        std::vector<BYTE> buffer(size);
        IFCEXPECTRC_RETURN(GetTokenInformation(token.get(), TokenUser, buffer.data(), size, &size), HRESULT_FROM_WIN32(GetLastError()));
        wil::unique_hlocal_string sid;
        IFCEXPECTRC_RETURN(ConvertSidToStringSidW(reinterpret_cast<TOKEN_USER*>(buffer.data())->User.Sid, &sid),
            HRESULT_FROM_WIN32(GetLastError()));

        const std::wstring name = std::wstring(L"Local\\Microsoft.UI.Xaml.WindowPlacement.Initialize.") + sid.get();
        // Same-user full-trust processes can initialize storage regardless of elevation. Other users cannot.
        const std::wstring sddl = std::wstring(L"D:P(A;;0x00100001;;;SY)(A;;0x00100001;;;") +
            sid.get() + L")S:(ML;;NW;;;ME)";
        wil::unique_hlocal_security_descriptor descriptor;
        IFCEXPECTRC_RETURN(ConvertStringSecurityDescriptorToSecurityDescriptorW(
            sddl.c_str(), SDDL_REVISION_1, &descriptor, nullptr), HRESULT_FROM_WIN32(GetLastError()));
        SECURITY_ATTRIBUTES attributes{ sizeof(attributes), descriptor.get(), FALSE };
        mutex.reset(CreateMutexExW(&attributes, name.c_str(), 0, SYNCHRONIZE | MUTEX_MODIFY_STATE));
        IFCEXPECTRC_RETURN(mutex, HRESULT_FROM_WIN32(GetLastError()));
        return S_OK;
    }

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
            return SW_SHOWMAXIMIZED;
        }

        if (state == SavedWindowState::MinimizedFromNormal ||
            state == SavedWindowState::MinimizedFromMaximized ||
            state == SavedWindowState::MinimizedFromSnapped)
        {
            // Match the show command that GetWindowPlacement and AppWindow.GetCurrentPlacement report.
            return SW_SHOWMINIMIZED;
        }

        return SW_SHOWNORMAL;
    }

    RECT ToRect(const ABI::Windows::Graphics::RectInt32& rect)
    {
        return { rect.X, rect.Y, rect.X + rect.Width, rect.Y + rect.Height };
    }

    ABI::Windows::Graphics::RectInt32 ToRectInt32(const RECT& rect)
    {
        return { rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top };
    }

    // WinUI's copy of PlacementEx and AppWindow's PlacementInfo use different flag values.
    // Flags without an equivalent (KeepHidden, NoActivate, AllowPartiallyOffScreen, ...) are dropped.
    struct FlagMapping
    {
        PlacementFlags placementEx;
        windowing::PlacementInfo appWindow;
    };
    constexpr FlagMapping c_flagMappings[] =
    {
        { PlacementFlags::RestoreToMaximized, windowing::PlacementInfo_RestoreToMaximized },
        { PlacementFlags::RestoreToArranged, windowing::PlacementInfo_RestoreToArranged },
        { PlacementFlags::Arranged, windowing::PlacementInfo_Arranged },
        { PlacementFlags::AllowSizing, windowing::PlacementInfo_Resizable },
        { PlacementFlags::FullScreen, windowing::PlacementInfo_FullScreen },
    };

    HRESULT ToPlacementEx(_In_ PlacementDetails* details, _Out_ PlacementEx& placement)
    {
        placement.Clear();

        ABI::Windows::Graphics::RectInt32 normalRect{}, workArea{}, arrangeRect{};
        INT32 dpi = 0;
        INT32 showCmd = 0;
        windowing::PlacementInfo info{};
        wrl_wrappers::HString deviceName;
        IFC_RETURN(details->get_NormalRect(&normalRect));
        IFC_RETURN(details->get_WorkArea(&workArea));
        IFC_RETURN(details->get_ArrangeRect(&arrangeRect));
        IFC_RETURN(details->get_Dpi(&dpi));
        IFC_RETURN(details->get_ShowCmd(&showCmd));
        IFC_RETURN(details->get_Flags(&info));
        IFC_RETURN(details->get_DeviceName(deviceName.GetAddressOf()));

        // AppWindowPlacementDetails.Create doesn't validate. Reject values that can't be a placement.
        IFCEXPECTRC_RETURN(dpi >= 96 && static_cast<uint32_t>(dpi) <= c_maxDpi, E_INVALIDARG);
        IFCEXPECTRC_RETURN(showCmd >= 0 && showCmd <= SW_MAX, E_INVALIDARG);

        placement.normalRect = ToRect(normalRect);
        placement.workArea = ToRect(workArea);
        placement.arrangeRect = ToRect(arrangeRect);
        placement.dpi = static_cast<UINT>(dpi);
        placement.showCmd = static_cast<UINT>(showCmd);
        for (const auto& mapping : c_flagMappings)
        {
            if ((static_cast<uint32_t>(info) & static_cast<uint32_t>(mapping.appWindow)) != 0)
            {
                placement.flags |= mapping.placementEx;
            }
        }

        IFC_RETURN(StringCchCopyW(placement.deviceName, ARRAYSIZE(placement.deviceName),
            WindowsGetStringRawBuffer(deviceName.Get(), nullptr)));

        return S_OK;
    }

    HRESULT FromPlacementEx(const PlacementEx& placement, _COM_Outptr_ PlacementDetails** details)
    {
        *details = nullptr;

        wrl::ComPtr<windowing::IAppWindowPlacementDetailsStatics> statics;
        IFC_RETURN(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Windowing_AppWindowPlacementDetails).Get(),
            &statics));

        uint32_t info = 0;
        for (const auto& mapping : c_flagMappings)
        {
            if (placement.HasFlag(mapping.placementEx))
            {
                info |= static_cast<uint32_t>(mapping.appWindow);
            }
        }

        // Use the string length, not the buffer size, so the HSTRING has no padding nulls.
        const size_t deviceNameLength = wcsnlen(placement.deviceName, ARRAYSIZE(placement.deviceName));
        IFCEXPECTRC_RETURN(deviceNameLength < ARRAYSIZE(placement.deviceName), E_INVALIDARG);

        IFC_RETURN(statics->Create(
            ToRectInt32(placement.normalRect),
            ToRectInt32(placement.workArea),
            static_cast<INT32>(placement.dpi),
            static_cast<INT32>(placement.showCmd),
            ToRectInt32(placement.arrangeRect),
            static_cast<windowing::PlacementInfo>(info),
            wrl_wrappers::HStringReference(placement.deviceName, static_cast<unsigned int>(deviceNameLength)).Get(),
            details));

        return S_OK;
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
    INT32 comparison = 0;
    IFC_RETURN(WindowsCompareStringOrdinal(m_id.Get(), value, &comparison));
    if (comparison != 0)
    {
        IFC_RETURN(m_id.Set(value));
        ClearCascadeTag();
        m_cascadePropertyName.Release();
    }

    return S_OK;
}

HRESULT WindowPlacement::EnsureCascadePropertyName()
{
    if (!HasId() || !WindowsIsStringEmpty(m_cascadePropertyName.Get()))
    {
        return S_OK;
    }

    wil::unique_handle mutex;
    IFC_RETURN(OpenCascadeInitializationMutex(mutex));
    const DWORD wait = WaitForSingleObject(mutex.get(), 5000);
    IFCEXPECTRC_RETURN(wait == WAIT_OBJECT_0 || wait == WAIT_ABANDONED,
        wait == WAIT_FAILED ? HRESULT_FROM_WIN32(GetLastError()) : HRESULT_FROM_WIN32(ERROR_TIMEOUT));
    auto releaseMutex = wil::scope_exit([&]()
    {
        if (!ReleaseMutex(mutex.get()))
        {
            TRACE_HR_NORETURN(HRESULT_FROM_WIN32(GetLastError()));
        }
    });

    IFC_RETURN(EnsureStorage());
    if (!m_settings)
    {
        return S_OK;
    }

    wchar_t key[19]{};
    GetPlacementKey(m_id.Get(), key);
    key[0] = L'G';
    const auto keyString = wrl_wrappers::HStringReference(key);
    wrl_wrappers::HString record;
    wrl::ComPtr<IInspectable> boxed;
    // ApplicationData can return a null value rather than E_BOUNDS for a missing key.
    boolean hasGroup = false;
    IFC_RETURN(m_settings->HasKey(keyString.Get(), &hasGroup));
    const HRESULT lookup = hasGroup ? m_settings->Lookup(keyString.Get(), &boxed) : E_BOUNDS;
    if (lookup == E_BOUNDS)
    {
        RoClearError();
        GUID group{};
        IFC_RETURN(CoCreateGuid(&group));
        wchar_t text[39]{};
        IFCEXPECTRC_RETURN(StringFromGUID2(group, text, ARRAYSIZE(text)) == ARRAYSIZE(text), E_UNEXPECTED);
        const std::wstring value = std::wstring(WindowsGetStringRawBuffer(m_id.Get(), nullptr), WindowsGetStringLen(m_id.Get())) + text;
        IFC_RETURN(record.Set(value.c_str(), static_cast<UINT32>(value.size())));
        wrl::ComPtr<wf::IPropertyValueStatics> statics;
        IFC_RETURN(wf::GetActivationFactory(wrl_wrappers::HStringReference(RuntimeClass_Windows_Foundation_PropertyValue).Get(), &statics));
        IFC_RETURN(statics->CreateString(record.Get(), &boxed));
        boolean replaced = false;
        IFC_RETURN(m_settings->Insert(keyString.Get(), boxed.Get(), &replaced));
    }
    else
    {
        IFC_RETURN(lookup);
        IFCEXPECTRC_RETURN(boxed.Get(), HRESULT_FROM_WIN32(ERROR_INVALID_DATA));
        wrl::ComPtr<wf::IPropertyValue> value;
        IFC_RETURN(boxed.As(&value));
        IFC_RETURN(value->GetString(record.GetAddressOf()));
    }

    UINT32 length = 0;
    PCWSTR text = WindowsGetStringRawBuffer(record.Get(), &length);
    const UINT32 idLength = WindowsGetStringLen(m_id.Get());
    IFCEXPECTRC_RETURN(length == idLength + 38 &&
        memcmp(text, WindowsGetStringRawBuffer(m_id.Get(), nullptr), idLength * sizeof(wchar_t)) == 0,
        HRESULT_FROM_WIN32(ERROR_INVALID_DATA));
    GUID group{};
    IFC_RETURN(IIDFromString(text + idLength, &group));
    IFCEXPECTRC_RETURN(!IsEqualGUID(group, GUID_NULL), HRESULT_FROM_WIN32(ERROR_INVALID_DATA));
    const std::wstring property = std::wstring(L"Microsoft.UI.Xaml.WindowPlacement.Cascade.") + (text + idLength);
    IFC_RETURN(m_cascadePropertyName.Set(property.c_str(), static_cast<UINT32>(property.size())));
    return S_OK;
}

void WindowPlacement::ClearCascadeTag()
{
    if (m_taggedWindow)
    {
        RemovePropW(m_taggedWindow, WindowsGetStringRawBuffer(m_cascadePropertyName.Get(), nullptr));
        m_taggedWindow = nullptr;
    }
}

HRESULT WindowPlacement::UpdateCascadeTag(HWND hwnd, bool eligible)
{
    if (!eligible || !HasId())
    {
        ClearCascadeTag();
        return S_OK;
    }
    IFC_RETURN(EnsureCascadePropertyName());
    if (!WindowsIsStringEmpty(m_cascadePropertyName.Get()) && m_taggedWindow != hwnd)
    {
        ClearCascadeTag();
        IFCEXPECTRC_RETURN(SetPropW(hwnd, WindowsGetStringRawBuffer(m_cascadePropertyName.Get(), nullptr),
            reinterpret_cast<HANDLE>(1)), HRESULT_FROM_WIN32(GetLastError()));
        m_taggedWindow = hwnd;
    }
    return S_OK;
}

HRESULT WindowPlacement::TryCreateCascadedPlacement(HWND destination, _Outptr_result_maybenull_ PlacementDetails** details)
{
    *details = nullptr;
    IFC_RETURN(EnsureCascadePropertyName());
    if (WindowsIsStringEmpty(m_cascadePropertyName.Get()))
    {
        return S_OK;
    }

    wrl::ComPtr<IVirtualDesktopManager> desktops;
    IFC_RETURN(CoCreateInstance(CLSID_VirtualDesktopManager, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&desktops)));
    wrl::ComPtr<windowing::IAppWindowPlacementDetailsStatics> statics;
    IFC_RETURN(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Windowing_AppWindowPlacementDetails).Get(), &statics));

    struct Search
    {
        HWND destination;
        PCWSTR property;
        std::vector<HWND> sources;
    } search{ destination, WindowsGetStringRawBuffer(m_cascadePropertyName.Get(), nullptr), {} };
    SetLastError(ERROR_SUCCESS);
    const BOOL enumerated = EnumWindows([](HWND hwnd, LPARAM parameter) -> BOOL
    {
        auto& search = *reinterpret_cast<Search*>(parameter);
        if (hwnd != search.destination && IsWindowVisible(hwnd) && !IsIconic(hwnd) &&
            GetPropW(hwnd, search.property) == reinterpret_cast<HANDLE>(1))
        {
            search.sources.push_back(hwnd);
        }
        return TRUE;
    }, reinterpret_cast<LPARAM>(&search));
    if (!enumerated)
    {
        const DWORD error = GetLastError();
        IFC_RETURN(error ? HRESULT_FROM_WIN32(error) : E_FAIL);
    }

    for (HWND hwnd : search.sources)
    {
        DWORD cloaked = 0;
        const HRESULT cloakResult = DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked));
        TRACE_HR_NORETURN(cloakResult);
        if (FAILED(cloakResult) || cloaked != 0)
        {
            if (FAILED(cloakResult)) RoClearError();
            continue;
        }
        BOOL current = FALSE;
        const HRESULT desktopResult = desktops->IsWindowOnCurrentVirtualDesktop(hwnd, &current);
        TRACE_HR_NORETURN(desktopResult);
        if (FAILED(desktopResult) || !current)
        {
            if (FAILED(desktopResult)) RoClearError();
            continue;
        }
        ABI::Microsoft::UI::WindowId id{};
        const HRESULT idResult = ABI::Microsoft::UI::GetWindowIdFromWindow(hwnd, &id);
        TRACE_HR_NORETURN(idResult);
        if (FAILED(idResult))
        {
            RoClearError();
            continue;
        }
        if (GetPropW(hwnd, search.property) != reinterpret_cast<HANDLE>(1))
        {
            continue;
        }
        wrl::ComPtr<PlacementDetails> captured;
        const HRESULT capture = statics->CreateCascadedFrom(id, &captured);
        TRACE_HR_NORETURN(capture);
        if (SUCCEEDED(capture) && captured)
        {
            *details = captured.Detach();
            return S_OK;
        }
        RoClearError();
    }
    return S_OK;
}

HRESULT WindowPlacement::EnsureStorage()
{
    // Reopen the container on every use. The app can delete it (DeleteContainer or
    // ApplicationData.ClearAsync), and writes to a stale Values map would be lost.
    m_settings.Reset();

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

HRESULT WindowPlacement::TryLoad(_Outptr_result_maybenull_ PlacementDetails** details, _Out_ PlacementResult* result)
{
    IFCPTR_RETURN(details);
    IFCPTR_RETURN(result);
    *details = nullptr;
    *result = PlacementResult::None;

    if (!HasId())
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

    // WinUI never restores full screen, in either mode. Strip the flag so AppWindow doesn't switch
    // presenters. The app can enter full screen itself.
    placement.flags &= ~PlacementFlags::FullScreen;

    // The record doesn't store resizability. Always allow AppWindow to shrink the window to fit
    // a smaller screen, even if the app made the window fixed size.
    placement.flags |= PlacementFlags::AllowSizing;
    IFC_RETURN(FromPlacementEx(placement, details));

    *result |= PlacementResult::Found;

    return S_OK;
}

HRESULT WindowPlacement::Save(_In_ PlacementDetails* details, _Out_ PlacementResult* result)
{
    IFCPTR_RETURN(details);
    IFCPTR_RETURN(result);
    *result = PlacementResult::None;

    // An empty id opts out. Storage is unavailable without package identity or if setup failed.
    // Saving is best effort; skip it rather than interfere with window shutdown.
    if (!HasId())
    {
        return S_OK;
    }

    IGNOREHR(EnsureStorage());
    if (!m_settings)
    {
        return S_OK;
    }

    PlacementEx placement;
    // Keep the earlier record if the details have bounds/DPI that the reader would reject.
    IFC_RETURN(ToPlacementEx(details, placement));
    IFCEXPECTRC_RETURN(placement.IsValid(), HRESULT_FROM_WIN32(ERROR_INVALID_DATA));
    IFCEXPECTRC_RETURN(IsValidRect(placement.normalRect), HRESULT_FROM_WIN32(ERROR_INVALID_DATA));
    IFCEXPECTRC_RETURN(IsValidRect(placement.workArea), HRESULT_FROM_WIN32(ERROR_INVALID_DATA));

    // The record has no full screen state, so any FullScreen flag in the details is not stored.
    // In full screen, AppWindow reports the overlapped placement underneath it (normal, maximized,
    // or snapped). The app decides whether to enter full screen again after a restart.
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
