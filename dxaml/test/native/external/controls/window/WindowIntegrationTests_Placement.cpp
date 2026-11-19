// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "WindowIntegrationTests.h"
#include <XamlTailored.h>
#include <TestCleanupWrapper.h>
#include <WindowAutoCloser.h>
#include <SafeEventRegistration.h>
#include <microsoft.ui.xaml.window.h>
#include <wrl.h>
#include <collection.h>
#include <cmath>
#include <commctrl.h>
#include <dwmapi.h>
#include <memory>
#include <roerrorapi.h>
#include <string>

#pragma comment(lib, "dwmapi.lib")

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

#ifdef MUX_PRERELEASE
namespace
{
    enum class SavedWindowState
    {
        Normal = 0,
        Maximized = 1,
        MinimizedFromNormal = 2,
        Snapped = 3,
        MinimizedFromMaximized = 4,
        MinimizedFromSnapped = 5,
    };

    enum class SnapRegion { LeftHalf, RightHalf, TopLeftQuarter };
    enum class WindowChrome { Standard, ExtendedTitleBar, Borderless };

    constexpr struct
    {
        SnapRegion region;
        PCWSTR name;
    } c_snapCases[] =
    {
        { SnapRegion::LeftHalf, L"left-half" },
        { SnapRegion::RightHalf, L"right-half" },
        { SnapRegion::TopLeftQuarter, L"top-left-quarter" },
    };

    constexpr struct
    {
        WindowChrome kind;
        PCWSTR name;
    } c_chromeCases[] =
    {
        { WindowChrome::Standard, L"standard-title-bar" },
        { WindowChrome::ExtendedTitleBar, L"extended-title-bar" },
        { WindowChrome::Borderless, L"borderless" },
    };

    PCWSTR GetSavedStateName(SavedWindowState state)
    {
        switch (state)
        {
        case SavedWindowState::Normal: return L"normal";
        case SavedWindowState::Maximized: return L"maximized";
        case SavedWindowState::MinimizedFromNormal: return L"minimized-from-normal";
        case SavedWindowState::Snapped: return L"snapped";
        case SavedWindowState::MinimizedFromMaximized: return L"minimized-from-maximized";
        case SavedWindowState::MinimizedFromSnapped: return L"minimized-from-snapped";
        default: return L"unknown";
        }
    }

    HWND GetHwnd(xaml::Window^ window)
    {
        Microsoft::WRL::ComPtr<IWindowNative> native;
        VERIFY_SUCCEEDED(reinterpret_cast<IUnknown*>(window)->QueryInterface(IID_PPV_ARGS(&native)));
        HWND hwnd{};
        VERIFY_SUCCEEDED(native->get_WindowHandle(&hwnd));
        return hwnd;
    }

    Platform::String^ CreateUniquePlacementId()
    {
        GUID id{};
        VERIFY_SUCCEEDED(CoCreateGuid(&id));
        wchar_t text[40]{};
        VERIFY_IS_TRUE(StringFromGUID2(id, text, ARRAYSIZE(text)) > 0);
        return ref new Platform::String(text);
    }

    Platform::String^ GetPlacementStorageKey(Platform::String^ id)
    {
        // WARNING: This key algorithm and its constants are part of the existing placement format
        // and must not change. Changing them would make existing saved records unreachable.
        // Keep this independent of the product helper so tests use the established storage keys.
        uint64_t hash = 14695981039346656037ull;
        for (unsigned int i = 0; i < id->Length(); ++i)
        {
            hash = (hash ^ (id->Data()[i] & 0xff)) * 1099511628211ull;
            hash = (hash ^ (id->Data()[i] >> 8)) * 1099511628211ull;
        }
        wchar_t key[19]{};
        swprintf_s(key, L"P_%016llx", hash);
        return ref new Platform::String(key);
    }

    // WARNING: This layout is part of the existing version 1 placement format and must not change.
    // Existing saved records depend on its field order, types, and offsets.
    // Keep this independent of the product serializer so tests verify the stored wire format.
    struct PlacementRecordHeader
    {
        uint32_t version;
        FILETIME savedAt;
        GUID virtualDesktopId;
        RECT normalRect;
        RECT workArea;
        RECT snapRect;
        uint32_t dpi;
        uint32_t state;
        wchar_t deviceName[32];
        uint32_t idLength;
    };
    static_assert(sizeof(PlacementRecordHeader) == 152);

    struct PlacementRecord
    {
        PlacementRecordHeader fields{};
        std::wstring id;
    };

    // Give each case its own storage entry and remove it when the case finishes.
    struct PlacementData
    {
        Platform::String^ id;
        Platform::String^ key;
        ::Windows::Foundation::Collections::IPropertySet^ values;

        explicit PlacementData(Platform::String^ placementId = nullptr)
        {
            id = placementId ? placementId : CreateUniquePlacementId();
            key = GetPlacementStorageKey(id);
            auto settings = ::Microsoft::Windows::Storage::ApplicationData::GetDefault()->LocalSettings;
            values = settings->CreateContainer(L"Microsoft.UI.Xaml.WindowPlacement",
                ::Microsoft::Windows::Storage::ApplicationDataCreateDisposition::Always)->Values;
            if (HasRecord())
            {
                values->Remove(key);
            }
        }

        ~PlacementData()
        {
            if (HasRecord())
            {
                values->Remove(key);
            }
        }

        bool HasRecord()
        {
            return values->HasKey(key);
        }

        Platform::Array<byte>^ ReadBytes()
        {
            VERIFY_IS_TRUE(HasRecord(), L"The window should have saved a placement.");
            auto value = safe_cast<wf::IPropertyValue^>(values->Lookup(key));
            VERIFY_ARE_EQUAL(wf::PropertyType::UInt8Array, value->Type);
            Platform::Array<byte>^ bytes;
            value->GetUInt8Array(&bytes);
            return bytes;
        }

        std::shared_ptr<PlacementRecord> ReadRecord()
        {
            auto bytes = ReadBytes();
            VERIFY_IS_TRUE(bytes->Length >= sizeof(PlacementRecordHeader));
            auto record = std::make_shared<PlacementRecord>();
            memcpy(&record->fields, bytes->Data, sizeof(record->fields));
            VERIFY_ARE_EQUAL(sizeof(record->fields) + record->fields.idLength * sizeof(wchar_t),
                static_cast<size_t>(bytes->Length));
            record->id.assign(reinterpret_cast<const wchar_t*>(bytes->Data + sizeof(record->fields)),
                record->fields.idLength);
            return record;
        }

        void WriteRecord(const std::shared_ptr<PlacementRecord>& record)
        {
            record->fields.idLength = static_cast<uint32_t>(record->id.size());
            auto bytes = ref new Platform::Array<byte>(
                static_cast<unsigned int>(sizeof(record->fields) + record->id.size() * sizeof(wchar_t)));
            memcpy(bytes->Data, &record->fields, sizeof(record->fields));
            memcpy(bytes->Data + sizeof(record->fields), record->id.data(), record->id.size() * sizeof(wchar_t));
            values->Insert(key, wf::PropertyValue::CreateUInt8Array(bytes));
        }
    };

    void VerifySavedState(PlacementData& data, SavedWindowState expected)
    {
        const auto actual = data.ReadRecord()->fields.state;
        LOG_OUTPUT(L"Saved state: expected %s (%d), actual %d", GetSavedStateName(expected),
            static_cast<int>(expected), actual);
        VERIFY_ARE_EQUAL(static_cast<int>(expected), actual);
    }

    void WriteSavedState(const std::shared_ptr<PlacementRecord>& record, SavedWindowState state)
    {
        record->fields.state = static_cast<uint32_t>(state);
    }

    RECT GetWindowBounds(xaml::Window^ window)
    {
        // Include the invisible resize border when comparing the saved outer bounds.
        RECT bounds{};
        VERIFY_IS_TRUE(!!GetWindowRect(GetHwnd(window), &bounds));
        return bounds;
    }

    RECT MoveAndResizeWindow(xaml::Window^ window, POINT workAreaOffset, SIZE size)
    {
        HWND hwnd = GetHwnd(window);
        MONITORINFO monitor{ sizeof(monitor) };
        VERIFY_IS_TRUE(!!GetMonitorInfo(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &monitor));
        VERIFY_IS_TRUE(!!SetWindowPos(hwnd, nullptr, monitor.rcWork.left + workAreaOffset.x,
            monitor.rcWork.top + workAreaOffset.y, size.cx, size.cy, SWP_NOZORDER | SWP_NOACTIVATE));
        return GetWindowBounds(window);
    }

    void VerifyBoundsEqual(const RECT& expected, const RECT& actual)
    {
        LOG_OUTPUT(L"Expected bounds: (%ld, %ld, %ld, %ld); actual: (%ld, %ld, %ld, %ld)",
            expected.left, expected.top, expected.right, expected.bottom,
            actual.left, actual.top, actual.right, actual.bottom);
        VERIFY_ARE_EQUAL(expected.left, actual.left);
        VERIFY_ARE_EQUAL(expected.top, actual.top);
        VERIFY_ARE_EQUAL(expected.right, actual.right);
        VERIFY_ARE_EQUAL(expected.bottom, actual.bottom);
    }

    void VerifyBoundsNear(HWND hwnd, const RECT& expected, const RECT& actual)
    {
        // Snap restoration can differ by an invisible resize border.
        // Allow up to 8 DIPs per edge, not arbitrary drift or a different snap target.
        const LONG tolerance = MulDiv(8, GetDpiForWindow(hwnd), USER_DEFAULT_SCREEN_DPI);
        LOG_OUTPUT(L"Nearby bounds (tolerance %ld): expected (%ld, %ld, %ld, %ld); actual (%ld, %ld, %ld, %ld)",
            tolerance, expected.left, expected.top, expected.right, expected.bottom,
            actual.left, actual.top, actual.right, actual.bottom);
        VERIFY_IS_TRUE(std::abs(expected.left - actual.left) <= tolerance);
        VERIFY_IS_TRUE(std::abs(expected.top - actual.top) <= tolerance);
        VERIFY_IS_TRUE(std::abs(expected.right - actual.right) <= tolerance);
        VERIFY_IS_TRUE(std::abs(expected.bottom - actual.bottom) <= tolerance);
    }

    RECT GetWindowPlacementNormalRect(HWND hwnd)
    {
        // Native restore bounds use workspace coordinates, not screen coordinates.
        // Top/left taskbars can shift their origin relative to screen coordinates.
        WINDOWPLACEMENT placement{ sizeof(placement) };
        VERIFY_IS_TRUE(!!GetWindowPlacement(hwnd, &placement));
        return placement.rcNormalPosition;
    }

    bool HasSavedSnapBounds(const std::shared_ptr<PlacementRecord>& record)
    {
        const RECT empty{};
        return !EqualRect(&record->fields.snapRect, &empty);
    }

    RECT ReadSavedNormalBounds(PlacementData& data)
    {
        return data.ReadRecord()->fields.normalRect;
    }

    // Capture the native destruction save before Window.Close can hide a missing save.
    struct NativeDestructionCleanup
    {
        HWND hwnd;
        xaml::Window^ window;
        PlacementData& data;
        std::shared_ptr<PlacementRecord> recordBeforeClose;

        NativeDestructionCleanup(xaml::Window^ window, PlacementData& data) :
            hwnd(GetHwnd(window)), window(window), data(data)
        {
            VERIFY_IS_TRUE(!!SetWindowSubclass(hwnd, OnMessage, reinterpret_cast<UINT_PTR>(this),
                reinterpret_cast<DWORD_PTR>(this)));
        }

        ~NativeDestructionCleanup()
        {
            RemoveWindowSubclass(hwnd, OnMessage, reinterpret_cast<UINT_PTR>(this));
        }

        static LRESULT CALLBACK OnMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam,
            UINT_PTR, DWORD_PTR context)
        {
            auto cleanup = reinterpret_cast<NativeDestructionCleanup*>(context);
            const LRESULT result = DefSubclassProc(hwnd, message, wParam, lParam);
            if (message == WM_DESTROY)
            {
                if (cleanup->data.HasRecord())
                {
                    cleanup->recordBeforeClose = cleanup->data.ReadRecord();
                }
                // Close the XAML island while its HWND is still valid, after observing the native save.
                cleanup->window->Close();
            }
            return result;
        }
    };

    MONITORINFO GetWindowMonitorInfo(HWND hwnd)
    {
        MONITORINFO monitor{ sizeof(monitor) };
        VERIFY_IS_TRUE(!!GetMonitorInfo(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &monitor));
        return monitor;
    }

    RECT GetNormalBoundsOnScreen(HWND hwnd)
    {
        RECT normal = GetWindowPlacementNormalRect(hwnd);
        const auto monitor = GetWindowMonitorInfo(hwnd);
        // Account for top/left taskbars before comparing with saved screen-space bounds.
        OffsetRect(&normal, monitor.rcWork.left - monitor.rcMonitor.left, monitor.rcWork.top - monitor.rcMonitor.top);
        return normal;
    }

    RECT GetVisibleWindowBounds(HWND hwnd)
    {
        // Snap targets describe the visible frame, without invisible resize borders.
        RECT frame{};
        VERIFY_SUCCEEDED(DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &frame, sizeof(frame)));
        return frame;
    }

    bool IsWindowSnapped(HWND hwnd)
    {
        // IsWindowArranged is exported by user32 but has no SDK declaration.
        using IsWindowArrangedFunction = BOOL(WINAPI*)(HWND);
        static const auto isWindowArranged = reinterpret_cast<IsWindowArrangedFunction>(
            GetProcAddress(GetModuleHandleW(L"user32.dll"), "IsWindowArranged"));
        VERIFY_IS_NOT_NULL(isWindowArranged);
        return !!isWindowArranged(hwnd);
    }

    void SetSystemSnappingEnabled(BOOL enabled)
    {
        VERIFY_IS_TRUE(!!SystemParametersInfoW(SPI_SETWINARRANGING, enabled, nullptr, 0));
        BOOL actual{};
        VERIFY_IS_TRUE(!!SystemParametersInfoW(SPI_GETWINARRANGING, 0, &actual, 0));
        VERIFY_ARE_EQUAL(!!enabled, !!actual);
    }

    // Snapping is session-wide; restore the setting even if setup or a test fails.
    struct PlacementSnappingScope
    {
        BOOL original{};

        PlacementSnappingScope()
        {
            VERIFY_IS_TRUE(!!SystemParametersInfoW(SPI_GETWINARRANGING, 0, &original, 0));
            auto restoreOnFailure = wil::scope_exit([this]() { Restore(); });
            SetSystemSnappingEnabled(TRUE);
            restoreOnFailure.release();
        }

        ~PlacementSnappingScope() noexcept
        {
            Restore();
        }

        void Restore() noexcept
        {
            if (!SystemParametersInfoW(SPI_SETWINARRANGING, original, nullptr, 0))
            {
                LOG_OUTPUT(L"Could not restore the snapping setting: %lu", GetLastError());
            }
        }
    };

    RECT CalculateSnapBounds(const RECT& workArea, SnapRegion region)
    {
        RECT snap = workArea;
        if (region == SnapRegion::RightHalf)
        {
            snap.left += (snap.right - snap.left) / 2;
        }
        else
        {
            snap.right = snap.left + (snap.right - snap.left) / 2;
        }
        if (region == SnapRegion::TopLeftQuarter) snap.bottom = snap.top + (snap.bottom - snap.top) / 2;
        return snap;
    }

    void SetWindowChrome(xaml::Window^ window, WindowChrome chrome)
    {
        if (chrome == WindowChrome::ExtendedTitleBar)
        {
            window->ExtendsContentIntoTitleBar = true;
        }
        else if (chrome == WindowChrome::Borderless)
        {
            safe_cast<::Microsoft::UI::Windowing::OverlappedPresenter^>(
                window->AppWindow->Presenter)->SetBorderAndTitleBar(false, false);
        }
    }

    void SnapWindowToBounds(HWND hwnd, const RECT& snap)
    {
        // Enter the native arranged state before moving it. SetWindowPos alone does not snap.
        DefWindowProcW(hwnd, WM_NCLBUTTONDBLCLK, HTTOP, 0);
        VERIFY_IS_TRUE(IsWindowSnapped(hwnd));
        VERIFY_SUCCEEDED(DwmFlush());
        RECT outer{};
        VERIFY_IS_TRUE(!!GetWindowRect(hwnd, &outer));
        const RECT frame = GetVisibleWindowBounds(hwnd);
        // Include the invisible borders so the visible frame lands on the requested snap bounds.
        VERIFY_IS_TRUE(!!SetWindowPos(hwnd, nullptr,
            snap.left - (frame.left - outer.left), snap.top - (frame.top - outer.top),
            snap.right - snap.left + (frame.left - outer.left) + (outer.right - frame.right),
            snap.bottom - snap.top + (frame.top - outer.top) + (outer.bottom - frame.bottom),
            SWP_NOZORDER | SWP_NOACTIVATE));
        VERIFY_IS_TRUE(IsWindowSnapped(hwnd));
    }

    RECT SavePlacementFromNewWindow(PlacementData& data, SavedWindowState state = SavedWindowState::Normal)
    {
        // Start with a record saved by a real window, then let tests change only the fields they need.
        WindowAutoCloser window;
        window.Attach(ref new xaml::Window());
        window->PlacementId = data.id;
        window->Activate();
        const RECT bounds = MoveAndResizeWindow(window.get(), { 60, 60 }, { 420, 300 });
        const auto hwnd = GetHwnd(window.get());
        if (state == SavedWindowState::Maximized || state == SavedWindowState::MinimizedFromMaximized)
        {
            ShowWindow(hwnd, SW_MAXIMIZE);
        }
        if (state == SavedWindowState::MinimizedFromNormal || state == SavedWindowState::MinimizedFromMaximized)
        {
            ShowWindow(hwnd, SW_MINIMIZE);
        }
        window->Close();
        auto record = data.ReadRecord();
        VERIFY_ARE_EQUAL(1u, record->fields.version);
        VERIFY_IS_TRUE(IsEqualGUID(GUID_NULL, record->fields.virtualDesktopId));
        return bounds;
    }

    void VerifyClientSizeInDips(xaml::Window^ window, double width, double height)
    {
        // Check the actual client area in DIPs; property getters can return pending size requests.
        RECT client{};
        const auto hwnd = GetHwnd(window);
        VERIFY_IS_TRUE(!!GetClientRect(hwnd, &client));
        const double scale = GetDpiForWindow(hwnd) / 96.0;
        VERIFY_IS_TRUE(std::abs(client.right / scale - width) <= 1.0);
        VERIFY_IS_TRUE(std::abs(client.bottom / scale - height) <= 1.0);
    }
}

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace Window {

    void WindowIntegrationTests::WindowPlacementRestoreNormal()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([&]()
        {
            PlacementData data;
            WindowAutoCloser originalWindow;
            originalWindow.Attach(ref new xaml::Window());
            originalWindow->PlacementId = data.id;
            const RECT originalBounds = MoveAndResizeWindow(originalWindow.get(), { 60, 60 }, { 420, 300 });
            originalWindow->Activate();
            originalWindow->Close();
            VerifySavedState(data, SavedWindowState::Normal);
            VerifyBoundsEqual(originalBounds, ReadSavedNormalBounds(data));

            WindowAutoCloser restoredWindow;
            restoredWindow.Attach(ref new xaml::Window());
            restoredWindow->PlacementId = data.id;
            restoredWindow->Width = 250;
            restoredWindow->Height = 200;
            restoredWindow->Activate();
            VERIFY_IS_FALSE(!!IsIconic(GetHwnd(restoredWindow.get())));
            VERIFY_IS_FALSE(!!IsZoomed(GetHwnd(restoredWindow.get())));
            VerifyBoundsEqual(originalBounds, GetWindowBounds(restoredWindow.get()));

            // Setting Height still opts into preserving client height after saved placement wins.
            const double restoredWidth = restoredWindow->Width;
            const double restoredHeight = restoredWindow->Height;
            restoredWindow->ExtendsContentIntoTitleBar = true;
            VerifyClientSizeInDips(restoredWindow.get(), restoredWidth, restoredHeight);
        });
    }

    void WindowIntegrationTests::WindowPlacementPersistMovedBounds()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([&]()
        {
            PlacementData data;
            WindowAutoCloser originalWindow;
            originalWindow.Attach(ref new xaml::Window());
            originalWindow->PlacementId = data.id;
            originalWindow->Activate();
            MoveAndResizeWindow(originalWindow.get(), { 60, 60 }, { 420, 300 });
            const RECT movedBounds = MoveAndResizeWindow(originalWindow.get(), { 100, 100 }, { 460, 340 });
            originalWindow->Close();
            VerifyBoundsEqual(movedBounds, ReadSavedNormalBounds(data));

            WindowAutoCloser restoredWindow;
            restoredWindow.Attach(ref new xaml::Window());
            restoredWindow->PlacementId = data.id;
            restoredWindow->Width = 250;
            restoredWindow->Height = 200;
            restoredWindow->Activate();
            VERIFY_IS_FALSE(!!IsIconic(GetHwnd(restoredWindow.get())));
            VERIFY_IS_FALSE(!!IsZoomed(GetHwnd(restoredWindow.get())));
            VerifyBoundsEqual(movedBounds, GetWindowBounds(restoredWindow.get()));
        });
    }

    void WindowIntegrationTests::WindowPlacementRestoreMaximized()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([&]()
        {
            PlacementData data;
            WindowAutoCloser originalWindow;
            originalWindow.Attach(ref new xaml::Window());
            originalWindow->PlacementId = data.id;
            originalWindow->Activate();
            const RECT normalBoundsOnScreen = MoveAndResizeWindow(originalWindow.get(), { 60, 60 }, { 420, 300 });
            const auto originalHwnd = GetHwnd(originalWindow.get());
            const RECT normalBoundsInWorkspace = GetWindowPlacementNormalRect(originalHwnd);
            ShowWindow(originalHwnd, SW_MAXIMIZE);
            originalWindow->Close();
            VerifySavedState(data, SavedWindowState::Maximized);
            VerifyBoundsEqual(normalBoundsOnScreen, ReadSavedNormalBounds(data));

            WindowAutoCloser restoredWindow;
            restoredWindow.Attach(ref new xaml::Window());
            restoredWindow->PlacementId = data.id;
            restoredWindow->Width = 250;
            restoredWindow->Height = 200;
            restoredWindow->Activate();
            const auto restoredHwnd = GetHwnd(restoredWindow.get());
            VERIFY_IS_FALSE(!!IsIconic(restoredHwnd));
            VERIFY_IS_TRUE(!!IsZoomed(restoredHwnd));
            VerifyBoundsEqual(normalBoundsInWorkspace, GetWindowPlacementNormalRect(restoredHwnd));
            ShowWindow(restoredHwnd, SW_RESTORE);
            VerifyBoundsEqual(normalBoundsOnScreen, GetWindowBounds(restoredWindow.get()));
        });
    }

    void WindowIntegrationTests::WindowPlacementMaximizedRestoreTracksNormalSize()
    {
        TestCleanupWrapper cleanup;
        for (bool extendedTitleBar : { false, true })
        {
            for (auto kind : { ::Microsoft::UI::Windowing::AppWindowPresenterKind::FullScreen,
                ::Microsoft::UI::Windowing::AppWindowPresenterKind::CompactOverlay })
            {
                PlacementData data;
                WindowAutoCloser window;
                double width{}, height{};
                RunOnUIThread([&]()
                {
                    SavePlacementFromNewWindow(data, SavedWindowState::Maximized);
                    window.Attach(ref new xaml::Window());
                    window->PlacementId = data.id;
                    window->ExtendsContentIntoTitleBar = extendedTitleBar;
                    window->Width = 310;
                    window->Height = 220;
                    window->Activate();
                    VERIFY_IS_TRUE(!!IsZoomed(GetHwnd(window.get())));
                    // Keep the Width/Height getter values when changing presenters.
                    width = window->Width;
                    height = window->Height;
                    window->AppWindow->SetPresenter(kind);
                    VERIFY_IS_TRUE(std::abs(window->Width - width) <= 1.0);
                    VERIFY_IS_TRUE(std::abs(window->Height - height) <= 1.0);
                });
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    // Deferred size updates must not change the Width/Height getter values either.
                    VERIFY_IS_TRUE(std::abs(window->Width - width) <= 1.0);
                    VERIFY_IS_TRUE(std::abs(window->Height - height) <= 1.0);
                    window->Close();
                });
            }
        }
    }

    void WindowIntegrationTests::WindowPlacementRestoreMinimizedNormal()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([&]()
        {
            PlacementData data;
            WindowAutoCloser originalWindow;
            originalWindow.Attach(ref new xaml::Window());
            originalWindow->PlacementId = data.id;
            originalWindow->Activate();
            const RECT normalBoundsOnScreen = MoveAndResizeWindow(originalWindow.get(), { 60, 60 }, { 420, 300 });
            const auto originalHwnd = GetHwnd(originalWindow.get());
            const RECT normalBoundsInWorkspace = GetWindowPlacementNormalRect(originalHwnd);
            ShowWindow(originalHwnd, SW_MINIMIZE);
            originalWindow->Close();
            VerifySavedState(data, SavedWindowState::MinimizedFromNormal);
            VerifyBoundsEqual(normalBoundsOnScreen, ReadSavedNormalBounds(data));

            WindowAutoCloser restoredWindow;
            restoredWindow.Attach(ref new xaml::Window());
            restoredWindow->PlacementId = data.id;
            restoredWindow->Width = 250;
            restoredWindow->Height = 200;
            restoredWindow->Activate();
            const auto restoredHwnd = GetHwnd(restoredWindow.get());
            VERIFY_IS_FALSE(!!IsIconic(restoredHwnd));
            VERIFY_IS_FALSE(!!IsZoomed(restoredHwnd));
            VerifyBoundsEqual(normalBoundsInWorkspace, GetWindowPlacementNormalRect(restoredHwnd));
            VerifyBoundsEqual(normalBoundsOnScreen, GetWindowBounds(restoredWindow.get()));
        });
    }

    void WindowIntegrationTests::WindowPlacementRestoreMinimizedMaximized()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([&]()
        {
            PlacementData data;
            WindowAutoCloser originalWindow;
            originalWindow.Attach(ref new xaml::Window());
            originalWindow->PlacementId = data.id;
            originalWindow->Activate();
            const RECT normalBoundsOnScreen = MoveAndResizeWindow(originalWindow.get(), { 60, 60 }, { 420, 300 });
            const auto originalHwnd = GetHwnd(originalWindow.get());
            const RECT normalBoundsInWorkspace = GetWindowPlacementNormalRect(originalHwnd);
            ShowWindow(originalHwnd, SW_MAXIMIZE);
            ShowWindow(originalHwnd, SW_MINIMIZE);
            originalWindow->Close();
            VerifySavedState(data, SavedWindowState::MinimizedFromMaximized);
            VerifyBoundsEqual(normalBoundsOnScreen, ReadSavedNormalBounds(data));

            WindowAutoCloser restoredWindow;
            restoredWindow.Attach(ref new xaml::Window());
            restoredWindow->PlacementId = data.id;
            restoredWindow->Width = 250;
            restoredWindow->Height = 200;
            restoredWindow->Activate();
            const auto restoredHwnd = GetHwnd(restoredWindow.get());
            VERIFY_IS_FALSE(!!IsIconic(restoredHwnd));
            VERIFY_IS_TRUE(!!IsZoomed(restoredHwnd));
            VerifyBoundsEqual(normalBoundsInWorkspace, GetWindowPlacementNormalRect(restoredHwnd));
            ShowWindow(restoredHwnd, SW_RESTORE);
            VerifyBoundsEqual(normalBoundsOnScreen, GetWindowBounds(restoredWindow.get()));
        });
    }

    void WindowIntegrationTests::WindowPlacementRestoresOnlyOnFirstActivation()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([&]()
        {
            for (auto state : { SavedWindowState::Normal, SavedWindowState::Maximized,
                SavedWindowState::MinimizedFromNormal, SavedWindowState::MinimizedFromMaximized })
            {
                LOG_OUTPUT(L"Repeated activation after restoring %s", GetSavedStateName(state));
                PlacementData data;
                SavePlacementFromNewWindow(data, state);
                WindowAutoCloser restoredWindow;
                restoredWindow.Attach(ref new xaml::Window());
                restoredWindow->PlacementId = data.id;
                restoredWindow->Activate();
                if (IsZoomed(GetHwnd(restoredWindow.get())))
                {
                    ShowWindow(GetHwnd(restoredWindow.get()), SW_RESTORE);
                }
                const RECT movedBounds = MoveAndResizeWindow(restoredWindow.get(), { 100, 100 }, { 460, 340 });
                restoredWindow->Activate();
                VerifyBoundsEqual(movedBounds, GetWindowBounds(restoredWindow.get()));
            }
        });
    }

    void WindowIntegrationTests::WindowPlacementDoesNotCascadeForSharedId()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([&]()
        {
            for (auto state : { SavedWindowState::Normal, SavedWindowState::Maximized,
                SavedWindowState::MinimizedFromNormal, SavedWindowState::MinimizedFromMaximized })
            {
                LOG_OUTPUT(L"Shared id after saving %s", GetSavedStateName(state));
                PlacementData data;
                const RECT savedBounds = SavePlacementFromNewWindow(data, state);
                WindowAutoCloser liveWindow;
                liveWindow.Attach(ref new xaml::Window());
                liveWindow->PlacementId = data.id;
                liveWindow->Activate();
                if (IsZoomed(GetHwnd(liveWindow.get())))
                {
                    ShowWindow(GetHwnd(liveWindow.get()), SW_RESTORE);
                }
                MoveAndResizeWindow(liveWindow.get(), { 100, 100 }, { 460, 340 });

                WindowAutoCloser sameIdWindow;
                sameIdWindow.Attach(ref new xaml::Window());
                sameIdWindow->PlacementId = data.id;
                sameIdWindow->Activate();
                const auto hwnd = GetHwnd(sameIdWindow.get());
                if (IsZoomed(hwnd)) ShowWindow(hwnd, SW_RESTORE);
                VerifyBoundsEqual(savedBounds, GetWindowBounds(sameIdWindow.get()));
            }
        });
    }

    void WindowIntegrationTests::WindowPlacementIdCanBeSetInMarkup()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([&]()
        {
            PlacementData data;
            WindowAutoCloser window;
            const std::wstring markup =
                L"<Window xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' PlacementId='{}" +
                std::wstring(data.id->Data()) + L"'/>";
            window.Attach(safe_cast<xaml::Window^>(xaml_markup::XamlReader::Load(ref new Platform::String(markup.c_str()))));
            VERIFY_IS_TRUE(window->PlacementId == data.id);
            window->Activate();
            const RECT bounds = MoveAndResizeWindow(window.get(), { 60, 60 }, { 420, 300 });
            window->Close();
            VerifyBoundsEqual(bounds, ReadSavedNormalBounds(data));
            VERIFY_IS_TRUE(data.ReadRecord()->id == std::wstring(data.id->Data(), data.id->Length()));
        });
    }

    void WindowIntegrationTests::WindowPlacementSavesUsingCurrentId()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([&]()
        {
            PlacementData originalId, currentId;
            WindowAutoCloser window;
            window.Attach(ref new xaml::Window());
            window->PlacementId = originalId.id;
            window->Activate();
            const RECT bounds = MoveAndResizeWindow(window.get(), { 60, 60 }, { 420, 300 });
            window->PlacementId = currentId.id;
            window->Close();
            VERIFY_IS_FALSE(originalId.HasRecord());
            VERIFY_IS_TRUE(currentId.ReadRecord()->id == std::wstring(currentId.id->Data(), currentId.id->Length()));
            VerifyBoundsEqual(bounds, ReadSavedNormalBounds(currentId));
        });
    }

    void WindowIntegrationTests::WindowPlacementIdRemainsUsableAfterClose()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([&]()
        {
            PlacementData originalId, changedId;
            WindowAutoCloser window;
            window.Attach(ref new xaml::Window());
            window->PlacementId = originalId.id;
            window->Activate();
            MoveAndResizeWindow(window.get(), { 60, 60 }, { 420, 300 });
            window->Close();
            VERIFY_IS_TRUE(window->PlacementId == originalId.id);
            window->PlacementId = changedId.id;
            VERIFY_IS_TRUE(window->PlacementId == changedId.id);
            window->PlacementId = nullptr;
            VERIFY_IS_TRUE(window->PlacementId == L"");
            VERIFY_IS_TRUE(originalId.HasRecord());
            VERIFY_IS_FALSE(changedId.HasRecord());
        });
    }

    void WindowIntegrationTests::WindowPlacementSettingIdAfterActivationDoesNotRestore()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([&]()
        {
            PlacementData data;
            SavePlacementFromNewWindow(data);
            WindowAutoCloser window;
            window.Attach(ref new xaml::Window());
            window->Activate();
            const RECT currentBounds = MoveAndResizeWindow(window.get(), { 90, 90 }, { 450, 330 });
            window->PlacementId = data.id;
            window->Activate();
            VerifyBoundsEqual(currentBounds, GetWindowBounds(window.get()));
        });
    }

    void WindowIntegrationTests::WindowPlacementClearingIdDisablesSaving()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([&]()
        {
            PlacementData data;
            const RECT savedBounds = SavePlacementFromNewWindow(data);
            WindowAutoCloser window;
            window.Attach(ref new xaml::Window());
            window->PlacementId = data.id;
            window->Activate();
            MoveAndResizeWindow(window.get(), { 90, 90 }, { 450, 330 });
            window->PlacementId = nullptr;
            VERIFY_IS_TRUE(window->PlacementId == L"");
            window->Close();
            VerifyBoundsEqual(savedBounds, ReadSavedNormalBounds(data));
        });
    }

    void WindowIntegrationTests::WindowPlacementIdsAreCaseSensitive()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([&]()
        {
            const auto prefix = CreateUniquePlacementId();
            PlacementData lowerCaseId(prefix + L"-case");
            PlacementData upperCaseId(prefix + L"-CASE");
            const RECT upperCaseBounds = SavePlacementFromNewWindow(upperCaseId);

            WindowAutoCloser lowerCaseWindow;
            lowerCaseWindow.Attach(ref new xaml::Window());
            lowerCaseWindow->PlacementId = lowerCaseId.id;
            lowerCaseWindow->Width = 310;
            lowerCaseWindow->Height = 220;
            lowerCaseWindow->Activate();
            VerifyClientSizeInDips(lowerCaseWindow.get(), 310, 220);
            const RECT lowerCaseBounds = GetWindowBounds(lowerCaseWindow.get());
            lowerCaseWindow->Close();

            VERIFY_IS_TRUE(lowerCaseId.ReadRecord()->id == std::wstring(lowerCaseId.id->Data(), lowerCaseId.id->Length()));
            VERIFY_IS_TRUE(upperCaseId.ReadRecord()->id == std::wstring(upperCaseId.id->Data(), upperCaseId.id->Length()));
            VerifyBoundsEqual(lowerCaseBounds, ReadSavedNormalBounds(lowerCaseId));
            VerifyBoundsEqual(upperCaseBounds, ReadSavedNormalBounds(upperCaseId));
        });
    }

    void WindowIntegrationTests::WindowPlacementFallbackWithoutSavedPlacement()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([&]()
        {
            PlacementData data;
            VERIFY_IS_FALSE(data.HasRecord());
            WindowAutoCloser originalWindow;
            originalWindow.Attach(ref new xaml::Window());
            originalWindow->PlacementId = data.id;
            originalWindow->Width = 310;
            originalWindow->Height = 220;
            originalWindow->Activate();
            VerifyClientSizeInDips(originalWindow.get(), 310, 220);
            originalWindow->ExtendsContentIntoTitleBar = true;
            VerifyClientSizeInDips(originalWindow.get(), 310, 220);
            const RECT movedBounds = MoveAndResizeWindow(originalWindow.get(), { 60, 60 }, { 420, 300 });
            originalWindow->Close();

            WindowAutoCloser restoredWindow;
            restoredWindow.Attach(ref new xaml::Window());
            restoredWindow->PlacementId = data.id;
            restoredWindow->Width = 310;
            restoredWindow->Height = 220;
            restoredWindow->Activate();
            VERIFY_IS_FALSE(!!IsIconic(GetHwnd(restoredWindow.get())));
            VERIFY_IS_FALSE(!!IsZoomed(GetHwnd(restoredWindow.get())));
            VerifyBoundsEqual(movedBounds, GetWindowBounds(restoredWindow.get()));
        });
    }

    void WindowIntegrationTests::WindowPlacementFallbackKeepsLatestSizeAfterAppWindowShow()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([&]()
        {
            for (const bool updateWidth : { true, false })
            {
                LOG_OUTPUT(L"Update %s after AppWindow.Show", updateWidth ? L"Width" : L"Height");
                WindowAutoCloser window;
                window.Attach(ref new xaml::Window());
                window->Width = 310;
                window->Height = 220;
                window->AppWindow->Show();

                const double expectedWidth = updateWidth ? 610 : 310;
                const double expectedHeight = updateWidth ? 220 : 420;
                if (updateWidth)
                {
                    window->Width = expectedWidth;
                }
                else
                {
                    window->Height = expectedHeight;
                }

                VERIFY_IS_TRUE(std::abs(window->Width - expectedWidth) <= 1.0);
                VERIFY_IS_TRUE(std::abs(window->Height - expectedHeight) <= 1.0);
                window->Activate();
                VerifyClientSizeInDips(window.get(), expectedWidth, expectedHeight);
            }
        });
    }

    void WindowIntegrationTests::WindowPlacementRequiresActivate()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([&]()
        {
            const struct
            {
                PCWSTR name;
                bool showBeforeSettingId;
                bool showAfterSettingId;
            } scenarios[] =
            {
                { L"never-shown", false, false },
                { L"id-before-AppWindow-Show", false, true },
                { L"id-after-AppWindow-Show", true, false },
            };
            for (bool existingRecord : { false, true })
            {
                for (const auto& scenario : scenarios)
                {
                    LOG_OUTPUT(L"Requires Window.Activate: %s, existing-record=%d", scenario.name, existingRecord);
                    PlacementData data;
                    if (existingRecord) SavePlacementFromNewWindow(data);
                    WindowAutoCloser window;
                    window.Attach(ref new xaml::Window());
                    if (scenario.showBeforeSettingId) window->AppWindow->Show();
                    window->PlacementId = data.id;
                    if (scenario.showAfterSettingId) window->AppWindow->Show();
                    MoveAndResizeWindow(window.get(), { 130, 130 }, { 490, 370 });
                    SendMessage(GetHwnd(window.get()), WM_ENDSESSION, TRUE, 0);
                    VERIFY_ARE_EQUAL(existingRecord, data.HasRecord());
                    if (existingRecord)
                    {
                        const auto normal = data.ReadRecord()->fields.normalRect;
                        VERIFY_ARE_EQUAL(420L, normal.right - normal.left);
                    }
                    window->Close();
                    VERIFY_ARE_EQUAL(existingRecord, data.HasRecord());
                    if (existingRecord)
                    {
                        const auto normal = data.ReadRecord()->fields.normalRect;
                        VERIFY_ARE_EQUAL(420L, normal.right - normal.left);
                    }
                }
            }
        });
    }

    void WindowIntegrationTests::WindowPlacementActivateRestoresAfterAppWindowShow()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([&]()
        {
            PlacementData data;
            const RECT savedBounds = SavePlacementFromNewWindow(data);
            WindowAutoCloser window;
            window.Attach(ref new xaml::Window());
            window->PlacementId = data.id;
            window->AppWindow->Show();
            MoveAndResizeWindow(window.get(), { 130, 130 }, { 490, 370 });
            window->Activate();
            VerifyBoundsEqual(savedBounds, GetWindowBounds(window.get()));
        });
    }

    void WindowIntegrationTests::WindowPlacementSavesOnlyConfirmedSessionEnd()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([&]()
        {
            PlacementData data;
            WindowAutoCloser window;
            window.Attach(ref new xaml::Window());
            window->PlacementId = data.id;
            window->Activate();
            const RECT bounds = MoveAndResizeWindow(window.get(), { 60, 60 }, { 420, 300 });
            const auto hwnd = GetHwnd(window.get());
            SendMessage(hwnd, WM_ENDSESSION, FALSE, 0);
            VERIFY_IS_FALSE(data.HasRecord());
            SendMessage(hwnd, WM_ENDSESSION, TRUE, ENDSESSION_LOGOFF);
            VerifyBoundsEqual(bounds, ReadSavedNormalBounds(data));
        });
    }

    void WindowIntegrationTests::WindowPlacementSavesFinalCloseHandlerChanges()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([&]()
        {
            PlacementData originalId, closeHandlerId;
            WindowAutoCloser window;
            window.Attach(ref new xaml::Window());
            window->PlacementId = originalId.id;
            window->Activate();
            const RECT originalBounds = MoveAndResizeWindow(window.get(), { 60, 60 }, { 420, 300 });
            SendMessage(GetHwnd(window.get()), WM_ENDSESSION, TRUE, ENDSESSION_LOGOFF);

            RECT closeHandlerBounds{};
            bool cancel = true;
            auto closed = CreateSafeEventRegistration(xaml::Window, Closed);
            closed.Attach(window.get(), ref new wf::TypedEventHandler<Platform::Object^, xaml::WindowEventArgs^>(
                [&](Platform::Object^, xaml::WindowEventArgs^ args)
                {
                    window->PlacementId = closeHandlerId.id;
                    closeHandlerBounds = MoveAndResizeWindow(window.get(), { 100, 100 }, { 460, 340 });
                    args->Handled = cancel;
                }));
            window->Close();
            VERIFY_IS_FALSE(closeHandlerId.HasRecord());
            cancel = false;
            window->Close();
            VerifyBoundsEqual(closeHandlerBounds, ReadSavedNormalBounds(closeHandlerId));
            VerifyBoundsEqual(originalBounds, ReadSavedNormalBounds(originalId));
        });
    }

    void WindowIntegrationTests::WindowPlacementBinaryRecord()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([&]()
        {
            const wchar_t suffix[] = { L'-', L'\0', L'X', 0x03a9, 0xd83d, 0xde80 };
            const auto prefix = CreateUniquePlacementId();
            std::wstring longestId(prefix->Data(), prefix->Length());
            longestId.resize(MAX_PATH, L'x');
            Platform::String^ ids[] =
            {
                prefix,
                prefix + ref new Platform::String(suffix, ARRAYSIZE(suffix)),
                ref new Platform::String(longestId.c_str()),
            };
            for (auto id : ids)
            {
                PlacementData data(id);
                FILETIME before{}, after{};
                GetSystemTimeAsFileTime(&before);
                const RECT saved = SavePlacementFromNewWindow(data);
                GetSystemTimeAsFileTime(&after);
                auto record = data.ReadRecord();
                VERIFY_ARE_EQUAL(1u, record->fields.version);
                VERIFY_IS_TRUE(record->id == std::wstring(id->Data(), id->Length()));
                VERIFY_IS_TRUE(CompareFileTime(&before, &record->fields.savedAt) <= 0);
                VERIFY_IS_TRUE(CompareFileTime(&record->fields.savedAt, &after) <= 0);
                VERIFY_IS_TRUE(IsEqualGUID(GUID_NULL, record->fields.virtualDesktopId));

                WindowAutoCloser window;
                window.Attach(ref new xaml::Window());
                window->PlacementId = data.id;
                window->Activate();
                VerifyBoundsEqual(saved, GetWindowBounds(window.get()));
                window->Close();
            }

            // Reject an oversized id without changing the existing id or its storage key.
            longestId += L'x';
            PlacementData tooLong(ref new Platform::String(longestId.c_str()));
            PlacementData previousId(prefix);
            WindowAutoCloser window;
            window.Attach(ref new xaml::Window());
            window->PlacementId = previousId.id;
            VERIFY_THROWS_SPECIFIC_WINRT(window->PlacementId = tooLong.id, Platform::Exception^,
                [](Platform::Exception^ ex) { return ex->HResult == E_INVALIDARG; });
            VERIFY_IS_TRUE(window->PlacementId == previousId.id);
            window->Activate();
            window->Close();
            VERIFY_IS_TRUE(previousId.HasRecord());
            VERIFY_IS_FALSE(tooLong.HasRecord());
        });
    }

    void WindowIntegrationTests::WindowPlacementRejectsInvalidRecords()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([&]()
        {
            PlacementData data;
            SavePlacementFromNewWindow(data);
            auto valid = data.ReadRecord();
            auto validBytes = data.ReadBytes();
            using Record = std::shared_ptr<PlacementRecord>;
            const struct
            {
                PCWSTR name;
                void (*invalidate)(const Record&);
            } scenarios[] =
            {
                { L"Future version", [](const Record& r) { r->fields.version = 2; } },
                { L"Mismatched id in the same hash slot", [](const Record& r) { r->id[0] = L'!'; } },
                { L"Negative width", [](const Record& r) { r->fields.normalRect.right = r->fields.normalRect.left - 1; } },
                { L"Invalid normal rectangle", [](const Record& r) { r->fields.normalRect.left = INT_MAX; } },
                { L"Unknown state", [](const Record& r) { r->fields.state = 99; } },
                { L"Partial snap rectangle", [](const Record& r) { r->fields.snapRect.left = 1; } },
                { L"Zero dpi", [](const Record& r) { r->fields.dpi = 0; } },
                { L"Zero height", [](const Record& r) { r->fields.normalRect.bottom = r->fields.normalRect.top; } },
                { L"Oversized width", [](const Record& r) { r->fields.normalRect.right = r->fields.normalRect.left + 65536; } },
                { L"Empty work area", [](const Record& r) { r->fields.workArea.bottom = r->fields.workArea.top; } },
                { L"Oversized work area", [](const Record& r) { r->fields.workArea.right = r->fields.workArea.left + 65536; } },
                { L"Invalid work area", [](const Record& r) { r->fields.workArea.top = INT_MAX; } },
                { L"Dpi below minimum", [](const Record& r) { r->fields.dpi = 95; } },
                { L"Dpi above maximum", [](const Record& r) { r->fields.dpi = 9601; } },
                { L"Snapped without snap bounds", [](const Record& r) { WriteSavedState(r, SavedWindowState::Snapped); } },
                { L"Minimized from snapped without snap bounds", [](const Record& r)
                    { WriteSavedState(r, SavedWindowState::MinimizedFromSnapped); } },
                { L"Normal bounds outside saved work area", [](const Record& r)
                    {
                        OffsetRect(&r->fields.normalRect, r->fields.workArea.right + 1 - r->fields.normalRect.left, 0);
                    } },
                { L"Unterminated display name", [](const Record& r)
                    { wmemset(r->fields.deviceName, L'a', ARRAYSIZE(r->fields.deviceName)); } },
                { L"Embedded null in display name", [](const Record& r)
                    {
                        r->fields.deviceName[0] = L'\0';
                        r->fields.deviceName[1] = L'b';
                    } },
                { L"Snap bounds outside saved work area", [](const Record& r)
                    {
                        r->fields.snapRect = r->fields.normalRect;
                        OffsetRect(&r->fields.snapRect, r->fields.workArea.right + 1 - r->fields.snapRect.left, 0);
                    } },
            };
            auto verifyFallback = [&]()
            {
                WindowAutoCloser window;
                window.Attach(ref new xaml::Window());
                window->PlacementId = data.id;
                window->Width = 310;
                window->Height = 220;
                window->Activate();
                VerifyClientSizeInDips(window.get(), 310, 220);
                window->ExtendsContentIntoTitleBar = true;
                VerifyClientSizeInDips(window.get(), 310, 220);
                window->PlacementId = L"";
                window->Close();
            };
            for (const auto& scenario : scenarios)
            {
                LOG_OUTPUT(L"Invalid placement record: %s", scenario.name);
                auto record = std::make_shared<PlacementRecord>(*valid);
                scenario.invalidate(record);
                data.WriteRecord(record);
                verifyFallback();
            }
            for (unsigned int size : { 1u, 151u, validBytes->Length - 2, validBytes->Length - 1,
                validBytes->Length + 1, validBytes->Length + 2 })
            {
                LOG_OUTPUT(L"Invalid binary record size: %u", size);
                auto bytes = ref new Platform::Array<byte>(size);
                memcpy(bytes->Data, validBytes->Data, (std::min)(size, validBytes->Length));
                data.values->Insert(data.key, wf::PropertyValue::CreateUInt8Array(bytes));
                verifyFallback();
            }
            for (uint32_t length : { 0u, UINT32_MAX })
            {
                LOG_OUTPUT(L"Invalid binary record id length: %u", length);
                auto bytes = ref new Platform::Array<byte>(validBytes->Data, validBytes->Length);
                memcpy(bytes->Data + offsetof(PlacementRecordHeader, idLength), &length, sizeof(length));
                data.values->Insert(data.key, wf::PropertyValue::CreateUInt8Array(bytes));
                verifyFallback();
            }
        });
    }

    void WindowIntegrationTests::WindowPlacementRecordCompatibility()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([&]()
        {
            enum class RecordKind { ReservedMetadata, FutureVersion, NotBinary };
            const struct
            {
                PCWSTR name;
                RecordKind kind;
            } scenarios[] =
            {
                { L"reserved-metadata", RecordKind::ReservedMetadata },
                { L"future-version", RecordKind::FutureVersion },
                { L"not-binary", RecordKind::NotBinary },
            };
            for (const auto& scenario : scenarios)
            {
                LOG_OUTPUT(L"Record compatibility: %s", scenario.name);
                PlacementData data;
                const RECT saved = SavePlacementFromNewWindow(data);
                auto record = data.ReadRecord();
                ZeroMemory(record->fields.deviceName, sizeof(record->fields.deviceName));
                VERIFY_SUCCEEDED(CoCreateGuid(&record->fields.virtualDesktopId));
                record->fields.savedAt = {};
                if (scenario.kind == RecordKind::FutureVersion) record->fields.version = 2;
                if (scenario.kind == RecordKind::NotBinary)
                {
                    data.values->Insert(data.key, L"not-binary");
                }
                else
                {
                    data.WriteRecord(record);
                }

                WindowAutoCloser window;
                window.Attach(ref new xaml::Window());
                window->PlacementId = data.id;
                window->Width = 310;
                window->Height = 220;
                window->Activate();
                RECT actual{};
                VERIFY_IS_TRUE(!!GetWindowRect(GetHwnd(window.get()), &actual));
                // Reading must leave reserved metadata and unsupported records unchanged in storage.
                if (scenario.kind == RecordKind::ReservedMetadata)
                {
                    VerifyBoundsEqual(saved, actual);
                    VERIFY_IS_TRUE(IsEqualGUID(record->fields.virtualDesktopId, data.ReadRecord()->fields.virtualDesktopId));
                    VERIFY_ARE_EQUAL(0u, data.ReadRecord()->fields.savedAt.dwLowDateTime);
                    VERIFY_ARE_EQUAL(0u, data.ReadRecord()->fields.savedAt.dwHighDateTime);
                }
                else
                {
                    VerifyClientSizeInDips(window.get(), 310, 220);
                    if (scenario.kind == RecordKind::FutureVersion)
                    {
                        VERIFY_ARE_EQUAL(2u, data.ReadRecord()->fields.version);
                    }
                    else if (scenario.kind == RecordKind::NotBinary)
                    {
                        VERIFY_IS_TRUE(safe_cast<Platform::String^>(data.values->Lookup(data.key)) == L"not-binary");
                    }
                }
                // Closing writes a fresh supported record using the window's final bounds.
                window->Close();
                record = data.ReadRecord();
                VERIFY_ARE_EQUAL(1u, record->fields.version);
                VERIFY_IS_TRUE(IsEqualGUID(GUID_NULL, record->fields.virtualDesktopId));
                VERIFY_IS_TRUE(record->fields.savedAt.dwLowDateTime != 0 || record->fields.savedAt.dwHighDateTime != 0);
                VerifyBoundsEqual(actual, ReadSavedNormalBounds(data));
            }
        });
    }

    void WindowIntegrationTests::WindowPlacementReportsHashCollision()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([&]()
        {
            enum class RecordKind { SameId, DifferentIdSameLength, DifferentIdLength, NotBinary };
            const struct
            {
                RecordKind kind;
                bool restore;
            } scenarios[] =
            {
                { RecordKind::SameId, false },
                { RecordKind::DifferentIdSameLength, false },
                { RecordKind::DifferentIdLength, false },
                { RecordKind::NotBinary, false },
                { RecordKind::SameId, true },
                { RecordKind::DifferentIdSameLength, true },
                { RecordKind::DifferentIdLength, true },
                { RecordKind::NotBinary, true },
            };
            for (const auto& scenario : scenarios)
            {
                const auto kind = scenario.kind;
                LOG_OUTPUT(L"Hash collision diagnostic: %s, record kind %d",
                    scenario.restore ? L"restore" : L"save", static_cast<int>(kind));
                PlacementData data;
                const RECT originalBounds = SavePlacementFromNewWindow(data);
                WindowAutoCloser window;
                window.Attach(ref new xaml::Window());
                window->PlacementId = data.id;
                RECT saved{};
                if (scenario.restore)
                {
                    window->Width = 310;
                    window->Height = 220;
                }
                else
                {
                    window->Activate();
                    saved = MoveAndResizeWindow(window.get(), { 100, 100 }, { 460, 340 });
                }

                // Change the record before the operation whose diagnostic we want to observe.
                auto record = data.ReadRecord();
                if (kind == RecordKind::DifferentIdSameLength) record->id[0] = L'!';
                if (kind == RecordKind::DifferentIdLength) record->id += L"-other";
                if (kind == RecordKind::NotBinary)
                {
                    data.values->Insert(data.key, L"not-binary");
                }
                else
                {
                    data.WriteRecord(record);
                }

                static thread_local unsigned int originatedErrors;
                static thread_local HRESULT lastOriginatedError;
                originatedErrors = 0;
                lastOriginatedError = S_OK;
                auto handler = AddVectoredExceptionHandler(TRUE, [](EXCEPTION_POINTERS* exception) -> LONG
                {
                    const auto record = exception->ExceptionRecord;
                    if (record->ExceptionCode == EXCEPTION_RO_ORIGINATEERROR && record->NumberParameters > 0)
                    {
                        ++originatedErrors;
                        lastOriginatedError = static_cast<HRESULT>(record->ExceptionInformation[0]);
                    }
                    return EXCEPTION_CONTINUE_SEARCH;
                });
                VERIFY_IS_NOT_NULL(handler);
                auto removeHandler = wil::scope_exit([&]() { RemoveVectoredExceptionHandler(handler); });

                // Observe the diagnostic without a debugger and restore the thread's reporting settings.
                UINT32 previousFlags = 0;
                VERIFY_SUCCEEDED(RoGetErrorReportingFlags(&previousFlags));
                auto restoreFlags = wil::scope_exit([&]() { RoSetErrorReportingFlags(previousFlags); });
                const UINT32 flags = (previousFlags &
                    ~(RO_ERROR_REPORTING_SUPPRESSEXCEPTIONS | RO_ERROR_REPORTING_SUPPRESSSETERRORINFO)) |
                    RO_ERROR_REPORTING_FORCEEXCEPTIONS | RO_ERROR_REPORTING_USESETERRORINFO;
                VERIFY_SUCCEEDED(RoSetErrorReportingFlags(flags));

                if (scenario.restore)
                {
                    window->Activate();
                }
                else
                {
                    window->Close();
                }
                const bool collision = kind == RecordKind::DifferentIdSameLength ||
                    kind == RecordKind::DifferentIdLength;
                const bool invalidRecord = scenario.restore && kind == RecordKind::NotBinary;
                VERIFY_ARE_EQUAL(collision || invalidRecord ? 1u : 0u, originatedErrors);
                if (collision)
                {
                    VERIFY_ARE_EQUAL(HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS), lastOriginatedError);
                }
                else if (invalidRecord)
                {
                    VERIFY_ARE_EQUAL(HRESULT_FROM_WIN32(ERROR_INVALID_DATA), lastOriginatedError);
                }
                Microsoft::WRL::ComPtr<IRestrictedErrorInfo> errorInfo;
                VERIFY_SUCCEEDED(GetRestrictedErrorInfo(&errorInfo));
                VERIFY_IS_NULL(errorInfo.Get());
                if (scenario.restore)
                {
                    if (kind == RecordKind::SameId)
                    {
                        VerifyBoundsEqual(originalBounds, GetWindowBounds(window.get()));
                    }
                    else
                    {
                        VerifyClientSizeInDips(window.get(), 310, 220);
                    }
                    // Do not save while closing a window used only to test restoration.
                    window->PlacementId = L"";
                    window->Close();
                }
                else
                {
                    VERIFY_IS_TRUE(data.ReadRecord()->id == std::wstring(data.id->Data(), data.id->Length()));
                    VerifyBoundsEqual(saved, ReadSavedNormalBounds(data));
                }
            }
        });
    }

    void WindowIntegrationTests::WindowPlacementSkipsUnsupportedPresenters()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([&]()
        {
            for (auto kind : { ::Microsoft::UI::Windowing::AppWindowPresenterKind::FullScreen,
                ::Microsoft::UI::Windowing::AppWindowPresenterKind::CompactOverlay })
            {
                PlacementData data;
                SavePlacementFromNewWindow(data);
                WindowAutoCloser window;
                window.Attach(ref new xaml::Window());
                window->PlacementId = data.id;
                window->AppWindow->SetPresenter(kind);
                window->Activate();
                VERIFY_IS_TRUE(window->AppWindow->Presenter->Kind == kind);
                window->Close();
                auto normal = data.ReadRecord()->fields.normalRect;
                VERIFY_ARE_EQUAL(420L, normal.right - normal.left);

                WindowAutoCloser changed;
                changed.Attach(ref new xaml::Window());
                changed->PlacementId = data.id;
                changed->Activate();
                MoveAndResizeWindow(changed.get(), { 120, 120 }, { 480, 360 });
                changed->AppWindow->SetPresenter(kind);
                changed->Close();
                normal = data.ReadRecord()->fields.normalRect;
                VERIFY_ARE_EQUAL(420L, normal.right - normal.left);
            }
        });
    }

    void WindowIntegrationTests::WindowPlacementCloseDuringActivation()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([&]()
        {
            for (bool existingRecord : { false, true })
            {
                for (bool closeFromSize : { false, true })
                {
                    PlacementData data;
                    LOG_OUTPUT(L"Close during %s, existing-record=%d",
                        closeFromSize ? L"SizeChanged" : L"Activated", existingRecord);
                    if (existingRecord) SavePlacementFromNewWindow(data);
                    WindowAutoCloser window;
                    window.Attach(ref new xaml::Window());
                    window->PlacementId = data.id;
                    window->Width = 310;
                    window->Height = 220;
                    bool closed = false;
                    auto close = [&]()
                    {
                        if (!closed)
                        {
                            closed = true;
                            window->Activate();
                            MoveAndResizeWindow(window.get(), { 110, 110 }, { 470, 350 });
                            window->Close();
                        }
                    };
                    auto size = CreateSafeEventRegistration(xaml::Window, SizeChanged);
                    auto activated = CreateSafeEventRegistration(xaml::Window, Activated);
                    if (closeFromSize)
                    {
                        size.Attach(window.get(), ref new wf::TypedEventHandler<Platform::Object^, xaml::WindowSizeChangedEventArgs^>(
                            [&](Platform::Object^, xaml::WindowSizeChangedEventArgs^) { close(); }));
                    }
                    else
                    {
                        activated.Attach(window.get(), ref new wf::TypedEventHandler<Platform::Object^, xaml::WindowActivatedEventArgs^>(
                            [&](Platform::Object^, xaml::WindowActivatedEventArgs^) { close(); }));
                    }
                    window->Activate();
                    VERIFY_IS_TRUE(closed);
                    VERIFY_ARE_EQUAL(existingRecord, data.HasRecord());
                    if (existingRecord)
                    {
                        const auto normal = data.ReadRecord()->fields.normalRect;
                        VERIFY_ARE_EQUAL(420L, normal.right - normal.left);
                    }
                }
            }
        });
    }

    void WindowIntegrationTests::WindowPlacementReentrantActivate()
    {
        TestCleanupWrapper cleanup;
        for (bool maximize : { false, true })
        {
            PlacementData data;
            WindowAutoCloser window;
            RECT savedBoundsOnScreen{}, normalBoundsInWorkspace{};
            bool reentered = false;
            auto activated = CreateSafeEventRegistration(xaml::Window, Activated);
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Reentrant activation: %s", maximize ? L"maximized" : L"normal");
                savedBoundsOnScreen = SavePlacementFromNewWindow(data,
                    maximize ? SavedWindowState::Maximized : SavedWindowState::Normal);
                window.Attach(ref new xaml::Window());
                window->PlacementId = data.id;
                window->Width = 250;
                window->Height = 200;
                activated.Attach(window.get(), ref new wf::TypedEventHandler<Platform::Object^, xaml::WindowActivatedEventArgs^>(
                    [&](Platform::Object^, xaml::WindowActivatedEventArgs^ args)
                    {
                        if (!reentered && args->WindowActivationState != xaml::WindowActivationState::Deactivated)
                        {
                            reentered = true;
                            window->Activate();
                        }
                    }));
                window->Activate();
                VERIFY_IS_TRUE(reentered);
                const auto hwnd = GetHwnd(window.get());
                VERIFY_ARE_EQUAL(maximize, !!IsZoomed(hwnd));
                VERIFY_IS_FALSE(!!IsIconic(hwnd));
                normalBoundsInWorkspace = GetWindowPlacementNormalRect(hwnd);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                const auto hwnd = GetHwnd(window.get());
                VERIFY_ARE_EQUAL(maximize, !!IsZoomed(hwnd));
                VERIFY_IS_FALSE(!!IsIconic(hwnd));
                VerifyBoundsEqual(normalBoundsInWorkspace, GetWindowPlacementNormalRect(hwnd));
                if (maximize) ShowWindow(hwnd, SW_RESTORE);
                RECT actual{};
                VERIFY_IS_TRUE(!!GetWindowRect(hwnd, &actual));
                VerifyBoundsEqual(savedBoundsOnScreen, actual);
                const RECT moved = MoveAndResizeWindow(window.get(), { 100, 100 }, { 460, 340 });
                window->Close();
                VerifyBoundsEqual(moved, ReadSavedNormalBounds(data));
            });
        }
    }

    void WindowIntegrationTests::WindowPlacementActivatedCanOverrideSize()
    {
        TestCleanupWrapper cleanup;
        const struct
        {
            bool changeWidth;
            bool changeHeight;
        } requests[] =
        {
            { true, false },
            { false, true },
            { true, true },
        };
        for (bool reenter : { true, false })
        {
            for (const auto& request : requests)
            {
                LOG_OUTPUT(L"Activated sizing: Width=%d, Height=%d, nested Activate=%d",
                    request.changeWidth, request.changeHeight, reenter);
                WindowAutoCloser window;
                bool changed = false;
                const double width = request.changeWidth ? 610 : 310;
                const double height = request.changeHeight ? 420 : 220;
                auto activated = CreateSafeEventRegistration(xaml::Window, Activated);
                RunOnUIThread([&]()
                {
                    window.Attach(ref new xaml::Window());
                    window->Width = 310;
                    window->Height = 220;
                    activated.Attach(window.get(), ref new wf::TypedEventHandler<Platform::Object^, xaml::WindowActivatedEventArgs^>(
                        [&](Platform::Object^, xaml::WindowActivatedEventArgs^ args)
                        {
                            if (!changed && args->WindowActivationState != xaml::WindowActivationState::Deactivated)
                            {
                                changed = true;
                                if (request.changeWidth) window->Width = width;
                                if (request.changeHeight) window->Height = height;
                                if (reenter) window->Activate();
                            }
                        }));
                    window->Activate();
                    VERIFY_IS_TRUE(changed);
                });
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    VerifyClientSizeInDips(window.get(), width, height);
                    window->Close();
                });
            }
        }
    }

    void WindowIntegrationTests::WindowPlacementSizeChangedCanOverrideRestoredSize()
    {
        TestCleanupWrapper cleanup;
        const struct
        {
            PCWSTR name;
            bool changeWidth;
            bool changeHeight;
            bool showBeforeActivate;
        } requests[] =
        {
            { L"width-only", true, false, false },
            { L"height-only", false, true, false },
            { L"width-and-height", true, true, false },
            { L"width-only", true, false, true },
            { L"height-only", false, true, true },
            { L"width-and-height", true, true, true },
        };
        for (const auto& request : requests)
        {
            LOG_OUTPUT(L"SizeChanged overrides restored size: %s, AppWindow.Show first=%d",
                request.name, request.showBeforeActivate);
            PlacementData data;
            WindowAutoCloser window;
            bool changed = false;
            double width = 0, height = 0;
            auto size = CreateSafeEventRegistration(xaml::Window, SizeChanged);
            RunOnUIThread([&]()
            {
                SavePlacementFromNewWindow(data);
                window.Attach(ref new xaml::Window());
                window->PlacementId = data.id;
                window->Width = 310;
                window->Height = 220;
                if (request.showBeforeActivate) window->AppWindow->Show();
                size.Attach(window.get(), ref new wf::TypedEventHandler<Platform::Object^, xaml::WindowSizeChangedEventArgs^>(
                    [&](Platform::Object^, xaml::WindowSizeChangedEventArgs^)
                    {
                        if (!changed)
                        {
                            changed = true;
                            width = request.changeWidth ? 360 : window->Width;
                            height = request.changeHeight ? 270 : window->Height;
                            if (request.changeWidth) window->Width = width;
                            if (request.changeHeight) window->Height = height;
                        }
                    }));
                window->Activate();
                VERIFY_IS_TRUE(changed);
                VerifyClientSizeInDips(window.get(), width, height);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VerifyClientSizeInDips(window.get(), width, height);
                if (request.changeHeight)
                {
                    window->ExtendsContentIntoTitleBar = true;
                    VerifyClientSizeInDips(window.get(), width, height);
                }
                window->Close();
            });
        }
    }

    void WindowIntegrationTests::WindowPlacementSavesDuringSentMessages()
    {
        TestCleanupWrapper cleanup;
        PlacementData original, final;
        WindowAutoCloser window;
        HWND hwnd{};
        RECT endSessionBounds{}, closeBounds{};
        RunOnUIThread([&]()
        {
            window.Attach(ref new xaml::Window());
            window->PlacementId = original.id;
            window->Activate();
            hwnd = GetHwnd(window.get());
            endSessionBounds = MoveAndResizeWindow(window.get(), { 60, 60 }, { 420, 300 });
        });
        TestServices::WindowHelper->WaitForIdle();

        // Send from the test thread, as Windows does during synchronous shutdown.
        DWORD_PTR result{};
        // Cancelled shutdown must not save; confirmed logoff must save without closing the window.
        VERIFY_IS_TRUE(!!SendMessageTimeoutW(hwnd, WM_ENDSESSION, FALSE, 0, SMTO_ABORTIFHUNG, 30000, &result));
        VERIFY_IS_FALSE(original.HasRecord());
        VERIFY_IS_TRUE(!!SendMessageTimeoutW(hwnd, WM_ENDSESSION, TRUE, ENDSESSION_LOGOFF, SMTO_ABORTIFHUNG, 30000, &result));
        VerifyBoundsEqual(endSessionBounds, ReadSavedNormalBounds(original));

        RunOnUIThread([&]()
        {
            window->PlacementId = final.id;
            closeBounds = MoveAndResizeWindow(window.get(), { 100, 100 }, { 460, 340 });
        });
        TestServices::WindowHelper->WaitForIdle();
        VERIFY_IS_TRUE(!!SendMessageTimeoutW(hwnd, WM_CLOSE, 0, 0, SMTO_ABORTIFHUNG, 30000, &result));
        TestServices::WindowHelper->WaitForIdle();
        VERIFY_IS_FALSE(!!IsWindow(hwnd));
        VerifyBoundsEqual(closeBounds, ReadSavedNormalBounds(final));
        VerifyBoundsEqual(endSessionBounds, ReadSavedNormalBounds(original));
    }

    void WindowIntegrationTests::WindowPlacementSavesOnDestroy()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([&]()
        {
            for (bool sendCloseMessage : { false, true })
            {
                PlacementData data;
                WindowAutoCloser window;
                window.Attach(ref new xaml::Window());
                window->PlacementId = data.id;
                window->Activate();
                const auto hwnd = GetHwnd(window.get());
                LOG_OUTPUT(L"Save during WM_DESTROY: %s", sendCloseMessage ? L"WM_CLOSE" : L"Window.Close");
                const RECT bounds = MoveAndResizeWindow(window.get(), { 60, 60 }, { 420, 300 });
                struct DestroyObservation
                {
                    PlacementData* data;
                    bool received = false;
                    bool savedBefore = false;
                    bool savedAfter = false;
                } observation{ &data };
                const SUBCLASSPROC observeDestroy = [](HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam,
                    UINT_PTR, DWORD_PTR context) -> LRESULT
                {
                    auto observation = reinterpret_cast<DestroyObservation*>(context);
                    if (message == WM_DESTROY)
                    {
                        observation->received = true;
                        observation->savedBefore = observation->data->HasRecord();
                    }
                    const LRESULT result = DefSubclassProc(hwnd, message, wParam, lParam);
                    if (message == WM_DESTROY)
                    {
                        observation->savedAfter = observation->data->HasRecord();
                    }
                    return result;
                };
                VERIFY_IS_TRUE(!!SetWindowSubclass(hwnd, observeDestroy, 0, reinterpret_cast<DWORD_PTR>(&observation)));
                auto removeSubclass = wil::scope_exit([&]() { RemoveWindowSubclass(hwnd, observeDestroy, 0); });
                if (sendCloseMessage)
                {
                    SendMessage(hwnd, WM_CLOSE, 0, 0);
                }
                else
                {
                    window->Close();
                }
                VERIFY_IS_TRUE(observation.received);
                VERIFY_IS_FALSE(observation.savedBefore);
                VERIFY_IS_TRUE(observation.savedAfter);
                VerifyBoundsEqual(bounds, ReadSavedNormalBounds(data));
            }
        });
    }

    void WindowIntegrationTests::WindowPlacementSavesDuringNativeDestruction()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([&]()
        {
            enum class DestructionPath { NativeWindow, AppWindow, OwnerWindow };
            const struct
            {
                PCWSTR name;
                DestructionPath path;
            } scenarios[] =
            {
                { L"DestroyWindow", DestructionPath::NativeWindow },
                { L"AppWindow.Destroy", DestructionPath::AppWindow },
                { L"owner-destruction", DestructionPath::OwnerWindow },
            };
            for (const auto& scenario : scenarios)
            {
                LOG_OUTPUT(L"Save during native destruction: %s", scenario.name);
                PlacementData data;
                WindowAutoCloser window;
                window.Attach(ref new xaml::Window());
                window->PlacementId = data.id;
                window->Activate();
                const auto hwnd = GetHwnd(window.get());

                HWND owner{};
                auto closeOwner = wil::scope_exit([&]() { if (IsWindow(owner)) DestroyWindow(owner); });
                if (scenario.path == DestructionPath::OwnerWindow)
                {
                    owner = CreateWindowExW(0, L"STATIC", L"Placement test owner", WS_OVERLAPPEDWINDOW,
                        0, 0, 200, 200, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
                    VERIFY_IS_NOT_NULL(owner);
                    SetWindowLongPtrW(hwnd, GWLP_HWNDPARENT, reinterpret_cast<LONG_PTR>(owner));
                    VERIFY_ARE_EQUAL(owner, GetWindow(hwnd, GW_OWNER));
                }
                const RECT finalBounds = MoveAndResizeWindow(window.get(), { 100, 100 }, { 460, 340 });
                NativeDestructionCleanup nativeCleanup(window.get(), data);

                if (scenario.path == DestructionPath::AppWindow)
                {
                    window->AppWindow->Destroy();
                }
                else
                {
                    VERIFY_IS_TRUE(!!DestroyWindow(owner ? owner : hwnd));
                }
                VERIFY_IS_FALSE(!!IsWindow(hwnd));
                VERIFY_IS_NOT_NULL(nativeCleanup.recordBeforeClose.get());
                VerifyBoundsEqual(finalBounds, nativeCleanup.recordBeforeClose->fields.normalRect);
            }
        });
    }

    void WindowIntegrationTests::WindowPlacementNativeDestructionSkipsUnsupportedPresenters()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([&]()
        {
            for (auto kind : { ::Microsoft::UI::Windowing::AppWindowPresenterKind::FullScreen,
                ::Microsoft::UI::Windowing::AppWindowPresenterKind::CompactOverlay })
            {
                for (bool useAppWindowDestroy : { false, true })
                {
                    LOG_OUTPUT(L"Native destruction skips %s: %s",
                        kind == ::Microsoft::UI::Windowing::AppWindowPresenterKind::FullScreen ? L"full-screen" : L"compact-overlay",
                        useAppWindowDestroy ? L"AppWindow.Destroy" : L"DestroyWindow");
                    PlacementData data;
                    const RECT savedBounds = SavePlacementFromNewWindow(data);
                    WindowAutoCloser window;
                    window.Attach(ref new xaml::Window());
                    window->PlacementId = data.id;
                    window->Activate();
                    const auto hwnd = GetHwnd(window.get());
                    MoveAndResizeWindow(window.get(), { 100, 100 }, { 460, 340 });
                    window->AppWindow->SetPresenter(kind);
                    NativeDestructionCleanup nativeCleanup(window.get(), data);

                    if (useAppWindowDestroy)
                    {
                        window->AppWindow->Destroy();
                    }
                    else
                    {
                        VERIFY_IS_TRUE(!!DestroyWindow(hwnd));
                    }
                    VERIFY_IS_FALSE(!!IsWindow(hwnd));
                    VERIFY_IS_NOT_NULL(nativeCleanup.recordBeforeClose.get());
                    VerifyBoundsEqual(savedBounds, nativeCleanup.recordBeforeClose->fields.normalRect);
                }
            }
        });
    }

    void WindowIntegrationTests::WindowPlacementAdaptToReplacementDisplay()
    {
        TestCleanupWrapper cleanup;
        PlacementData data;
        WindowAutoCloser window;
        RECT workArea{};
        UINT savedDpi{};
        HMONITOR targetMonitor{};
        RunOnUIThread([&]()
        {
            SavePlacementFromNewWindow(data);
            auto record = data.ReadRecord();
            savedDpi = record->fields.dpi;

            // Simulate a disconnected display without changing the VM's display setup.
            OffsetRect(&record->fields.workArea, 5000, 0);
            OffsetRect(&record->fields.normalRect, 5000, 0);
            ZeroMemory(record->fields.deviceName, sizeof(record->fields.deviceName));
            VERIFY_SUCCEEDED(StringCchCopyW(record->fields.deviceName, ARRAYSIZE(record->fields.deviceName),
                L"DisconnectedPlacementDisplay"));
            const RECT normal = record->fields.normalRect;
            targetMonitor = MonitorFromRect(&normal, MONITOR_DEFAULTTONEAREST);
            MONITORINFO monitor{ sizeof(monitor) };
            VERIFY_IS_TRUE(!!GetMonitorInfo(targetMonitor, &monitor));
            workArea = monitor.rcWork;
            data.WriteRecord(record);
            window.Attach(ref new xaml::Window());
            window->PlacementId = data.id;
            window->Activate();
        });
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            const RECT actual = GetWindowBounds(window.get());
            const auto hwnd = GetHwnd(window.get());
            VERIFY_IS_TRUE(targetMonitor == MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST));
            const UINT dpi = GetDpiForWindow(hwnd);
            const LONG tolerance = MulDiv(16, dpi, USER_DEFAULT_SCREEN_DPI);
            VERIFY_IS_TRUE(actual.left >= workArea.left - tolerance);
            VERIFY_IS_TRUE(actual.top >= workArea.top - tolerance);
            VERIFY_IS_TRUE(actual.right <= workArea.right + tolerance);
            VERIFY_IS_TRUE(actual.bottom <= workArea.bottom + tolerance);
            VERIFY_ARE_EQUAL(MulDiv(420, dpi, savedDpi), actual.right - actual.left);
            VERIFY_ARE_EQUAL(MulDiv(300, dpi, savedDpi), actual.bottom - actual.top);
            window->Close();
        });
    }

    void WindowIntegrationTests::WindowPlacementPreferSameNameDisplay()
    {
        TestCleanupWrapper cleanup;
        PlacementData data;
        WindowAutoCloser window;
        RECT workArea{};
        UINT dpi{};
        RunOnUIThread([&]()
        {
            SavePlacementFromNewWindow(data);
            auto record = data.ReadRecord();
            workArea = record->fields.workArea;
            dpi = record->fields.dpi;

            // Keep the display name but make its saved coordinates differ from today's.
            OffsetRect(&record->fields.workArea, 5000, 0);
            OffsetRect(&record->fields.normalRect, 5000, 0);
            data.WriteRecord(record);
            window.Attach(ref new xaml::Window());
            window->PlacementId = data.id;
            window->Activate();
        });
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            const RECT actual = GetWindowBounds(window.get());
            const LONG tolerance = MulDiv(16, dpi, USER_DEFAULT_SCREEN_DPI);
            VERIFY_IS_TRUE(actual.left >= workArea.left - tolerance);
            VERIFY_IS_TRUE(actual.top >= workArea.top - tolerance);
            VERIFY_IS_TRUE(actual.right <= workArea.right + tolerance);
            VERIFY_IS_TRUE(actual.bottom <= workArea.bottom + tolerance);
            VERIFY_ARE_EQUAL(420L, actual.right - actual.left);
            VERIFY_ARE_EQUAL(300L, actual.bottom - actual.top);
            window->Close();
        });
    }

    void WindowIntegrationTests::WindowPlacementFitOffscreenBounds()
    {
        TestCleanupWrapper cleanup;
        PlacementData data;
        WindowAutoCloser window;
        RECT workArea{};
        UINT dpi{};
        RunOnUIThread([&]()
        {
            SavePlacementFromNewWindow(data);
            auto record = data.ReadRecord();
            workArea = record->fields.workArea;
            dpi = record->fields.dpi;

            // Put half of the saved 420-pixel-wide window beyond the work area's right edge.
            OffsetRect(&record->fields.normalRect, workArea.right - 210 - record->fields.normalRect.left, 0);
            data.WriteRecord(record);
            window.Attach(ref new xaml::Window());
            window->PlacementId = data.id;
            window->Activate();
        });
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            const RECT actual = GetWindowBounds(window.get());
            const LONG tolerance = MulDiv(16, dpi, USER_DEFAULT_SCREEN_DPI);
            VERIFY_IS_TRUE(actual.left >= workArea.left - tolerance);
            VERIFY_IS_TRUE(actual.top >= workArea.top - tolerance);
            VERIFY_IS_TRUE(actual.right <= workArea.right + tolerance);
            VERIFY_IS_TRUE(actual.bottom <= workArea.bottom + tolerance);
            VERIFY_ARE_EQUAL(420L, actual.right - actual.left);
            VERIFY_ARE_EQUAL(300L, actual.bottom - actual.top);
            window->Close();
        });
    }

    void WindowIntegrationTests::WindowPlacementRestoreSnapped()
    {
        TestCleanupWrapper cleanup;
        PlacementSnappingScope snapping;
        for (const auto& chrome : c_chromeCases)
        {
            for (const auto& snapCase : c_snapCases)
            {
                LOG_OUTPUT(L"restore-snapped: %s, %s", snapCase.name, chrome.name);
                PlacementData data;
                WindowAutoCloser originalWindow, restoredWindow;
                RECT normal{}, snap{};
                RunOnUIThread([&]()
                {
                    originalWindow.Attach(ref new xaml::Window());
                    SetWindowChrome(originalWindow.get(), chrome.kind);
                    originalWindow->PlacementId = data.id;
                    originalWindow->Activate();
                    normal = MoveAndResizeWindow(originalWindow.get(), { 60, 60 }, { 420, 300 });
                });
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    const auto hwnd = GetHwnd(originalWindow.get());
                    snap = CalculateSnapBounds(GetWindowMonitorInfo(hwnd).rcWork, snapCase.region);
                    SnapWindowToBounds(hwnd, snap);
                });
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    const auto hwnd = GetHwnd(originalWindow.get());
                    VerifyBoundsEqual(snap, GetVisibleWindowBounds(hwnd));
                    VerifyBoundsEqual(normal, GetNormalBoundsOnScreen(hwnd));
                    originalWindow->Close();
                    auto record = data.ReadRecord();
                    VerifySavedState(data, SavedWindowState::Snapped);
                    VerifyBoundsEqual(normal, ReadSavedNormalBounds(data));
                    VerifyBoundsEqual(snap, record->fields.snapRect);

                    restoredWindow.Attach(ref new xaml::Window());
                    SetWindowChrome(restoredWindow.get(), chrome.kind);
                    restoredWindow->PlacementId = data.id;
                    restoredWindow->Width = 250;
                    restoredWindow->Height = 200;
                    restoredWindow->Activate();
                });
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    const auto hwnd = GetHwnd(restoredWindow.get());
                    VERIFY_IS_TRUE(IsWindowSnapped(hwnd));
                    VERIFY_IS_FALSE(!!IsIconic(hwnd));
                    const RECT restoredSnap = GetVisibleWindowBounds(hwnd);
                    const RECT restoredNormal = GetNormalBoundsOnScreen(hwnd);
                    VerifyBoundsNear(hwnd, snap, restoredSnap);
                    VerifyBoundsNear(hwnd, normal, restoredNormal);
                    VERIFY_ARE_EQUAL(normal.right - normal.left, restoredNormal.right - restoredNormal.left);
                    VERIFY_ARE_EQUAL(normal.bottom - normal.top, restoredNormal.bottom - restoredNormal.top);
                    restoredWindow->Close();
                    VerifyBoundsEqual(restoredSnap, data.ReadRecord()->fields.snapRect);
                    VerifyBoundsEqual(restoredNormal, ReadSavedNormalBounds(data));
                });
            }
        }
    }

    void WindowIntegrationTests::WindowPlacementDocumentsMinimizedSnappedCaptureLimitations()
    {
        TestCleanupWrapper cleanup;
        PlacementSnappingScope snapping;
        for (bool previouslySaved : { false, true })
        {
            PlacementData data;
            WindowAutoCloser window;
            RECT normal{}, snap{};
            RunOnUIThread([&]()
            {
                window.Attach(ref new xaml::Window());
                window->PlacementId = data.id;
                window->Activate();
                normal = MoveAndResizeWindow(window.get(), { 60, 60 }, { 420, 300 });
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                const auto hwnd = GetHwnd(window.get());
                snap = CalculateSnapBounds(GetWindowMonitorInfo(hwnd).rcWork, SnapRegion::LeftHalf);
                SnapWindowToBounds(hwnd, snap);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                const auto hwnd = GetHwnd(window.get());
                VerifyBoundsEqual(snap, GetVisibleWindowBounds(hwnd));
                if (previouslySaved)
                {
                    SendMessageW(hwnd, WM_ENDSESSION, TRUE, 0);
                    VerifySavedState(data, SavedWindowState::Snapped);
                }
                ShowWindow(hwnd, SW_MINIMIZE);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                const auto hwnd = GetHwnd(window.get());
                VERIFY_IS_TRUE(!!IsIconic(hwnd));

                // Known capture gap: PlacementEx does not recover the pre-minimize snap rect.
                // Windows 10 still reports arranged, but its minimized frame is off-screen:
                // WinUI rejects that capture and preserves any earlier record.
                // Windows 11 reports plain minimized, so it saves normal bounds without snap.
                // Expect MinimizedFromSnapped and exact snap bounds if capture support is added.
                const bool arranged = IsWindowSnapped(hwnd);
                if (arranged)
                {
                    const RECT frame = GetVisibleWindowBounds(hwnd);
                    const RECT workArea = GetWindowMonitorInfo(hwnd).rcWork;
                    RECT intersection{};
                    VERIFY_IS_FALSE(!!IntersectRect(&intersection, &frame, &workArea));
                }
                window->Close();
                if (arranged && !previouslySaved)
                {
                    VERIFY_IS_FALSE(data.HasRecord());
                }
                else
                {
                    auto record = data.ReadRecord();
                    VerifySavedState(data, arranged ? SavedWindowState::Snapped : SavedWindowState::MinimizedFromNormal);
                    VerifyBoundsEqual(normal, ReadSavedNormalBounds(data));
                    VERIFY_ARE_EQUAL(arranged, HasSavedSnapBounds(record));
                    if (arranged) VerifyBoundsEqual(snap, record->fields.snapRect);
                }
            });
        }
    }

    void WindowIntegrationTests::WindowPlacementRestoreMinimizedSnapped()
    {
        TestCleanupWrapper cleanup;
        PlacementSnappingScope snapping;
        for (const auto& snapCase : c_snapCases)
        {
            LOG_OUTPUT(L"restore-minimized-snapped (edited saved data): %s", snapCase.name);
            PlacementData data;
            WindowAutoCloser window;
            RECT normal{}, snap{};
            RunOnUIThread([&]()
            {
                normal = SavePlacementFromNewWindow(data);
                auto record = data.ReadRecord();
                snap = CalculateSnapBounds(record->fields.workArea, snapCase.region);
                WriteSavedState(record, SavedWindowState::MinimizedFromSnapped);
                record->fields.snapRect = snap;
                data.WriteRecord(record);
                window.Attach(ref new xaml::Window());
                window->PlacementId = data.id;
                window->Activate();
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                const auto hwnd = GetHwnd(window.get());
                VERIFY_IS_FALSE(!!IsIconic(hwnd));
                VERIFY_IS_FALSE(!!IsZoomed(hwnd));
                VERIFY_IS_TRUE(IsWindowSnapped(hwnd));
                // Restore works with test-created snap fields even though native capture is incomplete.
                VerifyBoundsNear(hwnd, normal, GetNormalBoundsOnScreen(hwnd));
                VerifyBoundsNear(hwnd, snap, GetVisibleWindowBounds(hwnd));
                VerifySavedState(data, SavedWindowState::MinimizedFromSnapped);
                window->Close();
                VerifySavedState(data, SavedWindowState::Snapped);
            });
        }
    }

    void WindowIntegrationTests::WindowPlacementMigratesChangedWorkArea()
    {
        TestCleanupWrapper cleanup;
        PlacementSnappingScope snapping;
        for (auto state : { SavedWindowState::Normal, SavedWindowState::Maximized,
            SavedWindowState::Snapped, SavedWindowState::MinimizedFromSnapped })
        {
            const bool savedSnapped = state == SavedWindowState::Snapped || state == SavedWindowState::MinimizedFromSnapped;
            for (bool shrink : { false, true })
            {
                LOG_OUTPUT(L"Work-area migration: %s, %s", GetSavedStateName(state),
                    shrink ? L"work-area-shrank" : L"work-area-grew");
                PlacementData data;
                WindowAutoCloser window;
                RECT workArea{}, oldWorkArea{}, normal{}, expectedSnap{}, restoredNormal{};
                RunOnUIThread([&]()
                {
                    SavePlacementFromNewWindow(data);
                    auto record = data.ReadRecord();
                    workArea = record->fields.workArea;
                    oldWorkArea = workArea;
                    oldWorkArea.right += shrink ? 240 : -240;
                    oldWorkArea.bottom += shrink ? 180 : -180;
                    normal = { oldWorkArea.left + 60, oldWorkArea.top + 60,
                        oldWorkArea.left + 480, oldWorkArea.top + 360 };
                    record->fields.workArea = oldWorkArea;
                    record->fields.normalRect = normal;
                    WriteSavedState(record, state);
                    if (savedSnapped)
                    {
                        record->fields.snapRect = CalculateSnapBounds(oldWorkArea, SnapRegion::LeftHalf);
                        expectedSnap = CalculateSnapBounds(workArea, SnapRegion::LeftHalf);
                    }
                    data.WriteRecord(record);
                    window.Attach(ref new xaml::Window());
                    window->PlacementId = data.id;
                    window->Activate();
                });
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    const auto hwnd = GetHwnd(window.get());
                    VERIFY_IS_FALSE(!!IsIconic(hwnd));
                    VERIFY_ARE_EQUAL(state == SavedWindowState::Maximized, !!IsZoomed(hwnd));
                    VERIFY_ARE_EQUAL(savedSnapped, IsWindowSnapped(hwnd));
                    restoredNormal = GetNormalBoundsOnScreen(hwnd);
                    VERIFY_ARE_EQUAL(420L, restoredNormal.right - restoredNormal.left);
                    VERIFY_ARE_EQUAL(300L, restoredNormal.bottom - restoredNormal.top);
                    VERIFY_IS_TRUE(restoredNormal.left >= workArea.left);
                    VERIFY_IS_TRUE(restoredNormal.top >= workArea.top);
                    VERIFY_IS_TRUE(restoredNormal.right <= workArea.right);
                    VERIFY_IS_TRUE(restoredNormal.bottom <= workArea.bottom);
                    if (savedSnapped)
                    {
                        VerifyBoundsNear(hwnd, expectedSnap, GetVisibleWindowBounds(hwnd));
                    }
                    else if (state == SavedWindowState::Normal)
                    {
                        RECT outer{};
                        VERIFY_IS_TRUE(!!GetWindowRect(hwnd, &outer));
                        VerifyBoundsEqual(restoredNormal, outer);
                    }
                    // Activation must not overwrite the old monitor geometry.
                    VerifyBoundsEqual(oldWorkArea, data.ReadRecord()->fields.workArea);
                    VerifyBoundsEqual(normal, ReadSavedNormalBounds(data));
                    window->Close();
                    VerifyBoundsEqual(workArea, data.ReadRecord()->fields.workArea);
                    VerifyBoundsEqual(restoredNormal, ReadSavedNormalBounds(data));
                    VerifySavedState(data, savedSnapped ? SavedWindowState::Snapped : state);
                });
            }
        }
    }

    void WindowIntegrationTests::WindowPlacementRespectsFixedSizing()
    {
        TestCleanupWrapper cleanup;
        for (bool resizable : { false, true })
        {
            LOG_OUTPUT(L"Fit oversized placement: %s", resizable ? L"resizable" : L"fixed-size");
            PlacementData data;
            WindowAutoCloser window;
            RECT workArea{}, oversized{}, initial{};
            SIZE maxTrack{};
            RunOnUIThread([&]()
            {
                SavePlacementFromNewWindow(data);
                auto record = data.ReadRecord();
                workArea = record->fields.workArea;
                RECT oldWorkArea = workArea;
                oldWorkArea.right += 400;
                oldWorkArea.bottom += 300;
                oversized = oldWorkArea;
                InflateRect(&oversized, -20, -20);
                record->fields.workArea = oldWorkArea;
                record->fields.normalRect = oversized;
                data.WriteRecord(record);
                window.Attach(ref new xaml::Window());
                initial = MoveAndResizeWindow(window.get(), { 100, 100 }, { 460, 340 });
                safe_cast<::Microsoft::UI::Windowing::OverlappedPresenter^>(
                    window->AppWindow->Presenter)->IsResizable = resizable;
                const UINT dpi = GetDpiForWindow(GetHwnd(window.get()));
                maxTrack = { GetSystemMetricsForDpi(SM_CXMAXTRACK, dpi), GetSystemMetricsForDpi(SM_CYMAXTRACK, dpi) };
                window->PlacementId = data.id;
                window->Activate();
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                const auto hwnd = GetHwnd(window.get());
                RECT actual{};
                VERIFY_IS_TRUE(!!GetWindowRect(hwnd, &actual));
                const LONG width = actual.right - actual.left;
                const LONG height = actual.bottom - actual.top;
                VERIFY_ARE_EQUAL(resizable, safe_cast<::Microsoft::UI::Windowing::OverlappedPresenter^>(
                    window->AppWindow->Presenter)->IsResizable);
                if (resizable)
                {
                    VERIFY_IS_TRUE(width <= workArea.right - workArea.left);
                    VERIFY_IS_TRUE(height <= workArea.bottom - workArea.top);
                }
                else
                {
                    // Windows can keep the fixed size or apply the saved size.
                    // Require one complete outcome, not a mix of their dimensions.
                    VERIFY_IS_TRUE(maxTrack.cx > 0);
                    VERIFY_IS_TRUE(maxTrack.cy > 0);
                    const LONG savedWidth = oversized.right - oversized.left;
                    const LONG savedHeight = oversized.bottom - oversized.top;
                    const bool preservedSize = width == initial.right - initial.left &&
                        height == initial.bottom - initial.top;
                    const bool restoredSavedSize = width == (savedWidth < maxTrack.cx ? savedWidth : maxTrack.cx) &&
                        height == (savedHeight < maxTrack.cy ? savedHeight : maxTrack.cy);
                    VERIFY_IS_TRUE(preservedSize || restoredSavedSize);
                }
                // Fixed-size windows may extend past the edges, but their title bar stays reachable.
                VERIFY_IS_TRUE(actual.left >= workArea.left);
                VERIFY_IS_TRUE(actual.top >= workArea.top);
                VERIFY_IS_TRUE(actual.top < workArea.bottom);
                window->Close();
                VerifyBoundsEqual(actual, ReadSavedNormalBounds(data));
            });
        }
    }

    void WindowIntegrationTests::WindowPlacementPreserveLogicalSize()
    {
        TestCleanupWrapper cleanup;
        PlacementSnappingScope snapping;
        for (auto state : { SavedWindowState::Normal, SavedWindowState::Maximized,
            SavedWindowState::Snapped, SavedWindowState::MinimizedFromSnapped })
        {
            LOG_OUTPUT(L"preserve-logical-size: %s, saved DPI is twice the target DPI", GetSavedStateName(state));
            const bool savedSnapped = state == SavedWindowState::Snapped || state == SavedWindowState::MinimizedFromSnapped;
            PlacementData data;
            WindowAutoCloser window;
            RECT savedNormal{}, snap{}, restoredNormal{};
            int targetDpi{};
            RunOnUIThread([&]()
            {
                SavePlacementFromNewWindow(data);
                auto record = data.ReadRecord();
                const RECT workArea = record->fields.workArea;
                savedNormal = { workArea.left + 120, workArea.top + 120,
                    workArea.left + 960, workArea.top + 720 };
                targetDpi = static_cast<int>(record->fields.dpi);
                // Simulate a DPI change without changing the VM's display settings.
                record->fields.dpi = targetDpi * 2;
                WriteSavedState(record, state);
                record->fields.normalRect = savedNormal;
                if (savedSnapped)
                {
                    // Snap bounds follow the physical work area, not the normal size's DPI ratio.
                    snap = CalculateSnapBounds(workArea, SnapRegion::TopLeftQuarter);
                    record->fields.snapRect = snap;
                }
                data.WriteRecord(record);
                window.Attach(ref new xaml::Window());
                window->PlacementId = data.id;
                window->Width = 250;
                window->Height = 200;
                window->Activate();
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                const auto hwnd = GetHwnd(window.get());
                VERIFY_ARE_EQUAL(static_cast<UINT>(targetDpi), GetDpiForWindow(hwnd));
                VERIFY_IS_FALSE(!!IsIconic(hwnd));
                VERIFY_ARE_EQUAL(state == SavedWindowState::Maximized, !!IsZoomed(hwnd));
                VERIFY_ARE_EQUAL(savedSnapped, IsWindowSnapped(hwnd));
                restoredNormal = GetNormalBoundsOnScreen(hwnd);
                VERIFY_ARE_EQUAL(420L, restoredNormal.right - restoredNormal.left);
                VERIFY_ARE_EQUAL(300L, restoredNormal.bottom - restoredNormal.top);
                if (savedSnapped)
                {
                    VerifyBoundsNear(hwnd, snap, GetVisibleWindowBounds(hwnd));
                }
                else if (state == SavedWindowState::Normal)
                {
                    RECT outer{};
                    VERIFY_IS_TRUE(!!GetWindowRect(hwnd, &outer));
                    VerifyBoundsEqual(restoredNormal, outer);
                }
                // Restore scales the live bounds; only closing saves the new DPI and bounds.
                VERIFY_ARE_EQUAL(static_cast<uint32_t>(targetDpi * 2), data.ReadRecord()->fields.dpi);
                VerifyBoundsEqual(savedNormal, ReadSavedNormalBounds(data));
                window->Close();
                VERIFY_ARE_EQUAL(static_cast<uint32_t>(targetDpi), data.ReadRecord()->fields.dpi);
                VerifyBoundsEqual(restoredNormal, ReadSavedNormalBounds(data));
            });
        }
    }

    void WindowIntegrationTests::WindowPlacementClearsStaleSnapFields()
    {
        TestCleanupWrapper cleanup;
        PlacementSnappingScope snapping;
        PlacementData data;
        WindowAutoCloser window;
        RECT normal{}, snap{};
        RunOnUIThread([&]()
        {
            window.Attach(ref new xaml::Window());
            window->PlacementId = data.id;
            window->Activate();
            MoveAndResizeWindow(window.get(), { 60, 60 }, { 420, 300 });
        });
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            const auto hwnd = GetHwnd(window.get());
            snap = CalculateSnapBounds(GetWindowMonitorInfo(hwnd).rcWork, SnapRegion::RightHalf);
            SnapWindowToBounds(hwnd, snap);
        });
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            const auto hwnd = GetHwnd(window.get());
            SendMessageW(hwnd, WM_ENDSESSION, TRUE, 0);
            VerifySavedState(data, SavedWindowState::Snapped);
            VerifyBoundsEqual(snap, data.ReadRecord()->fields.snapRect);
            ShowWindow(hwnd, SW_RESTORE);
            normal = MoveAndResizeWindow(window.get(), { 100, 100 }, { 460, 340 });
        });
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(IsWindowSnapped(GetHwnd(window.get())));
            SendMessageW(GetHwnd(window.get()), WM_ENDSESSION, TRUE, 0);
            auto record = data.ReadRecord();
            VerifySavedState(data, SavedWindowState::Normal);
            VerifyBoundsEqual(normal, ReadSavedNormalBounds(data));
            VERIFY_IS_FALSE(HasSavedSnapBounds(record));
            normal = MoveAndResizeWindow(window.get(), { 140, 140 }, { 500, 380 });
            window->Close();
            VerifyBoundsEqual(normal, ReadSavedNormalBounds(data));
            VERIFY_IS_FALSE(HasSavedSnapBounds(data.ReadRecord()));
        });
    }

    void WindowIntegrationTests::WindowPlacementRespectsSnappingSetting()
    {
        TestCleanupWrapper cleanup;
        PlacementSnappingScope snapping;
        // PlacementEx's downlevel SetWindowPlacement path honors SPI_GETWINARRANGING.
        // Its ApplyWindowAction path currently restores snap even when snapping is disabled.
        // This records that gap, not a cross-version policy; tighten the expectation if
        // PlacementEx aligns the paths.
        for (bool enabled : { false, true })
        {
            SetSystemSnappingEnabled(enabled);
            for (auto state : { SavedWindowState::Snapped, SavedWindowState::MinimizedFromSnapped })
            {
                LOG_OUTPUT(L"Snap setting: %s, %s", enabled ? L"enabled" : L"disabled", GetSavedStateName(state));
                PlacementData data;
                WindowAutoCloser window;
                RECT normal{}, snap{};
                RunOnUIThread([&]()
                {
                    normal = SavePlacementFromNewWindow(data);
                    auto record = data.ReadRecord();
                    snap = CalculateSnapBounds(record->fields.workArea, SnapRegion::LeftHalf);
                    WriteSavedState(record, state);
                    record->fields.snapRect = snap;
                    data.WriteRecord(record);
                    window.Attach(ref new xaml::Window());
                    window->PlacementId = data.id;
                    window->Activate();
                });
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    const auto hwnd = GetHwnd(window.get());
                    VERIFY_IS_FALSE(!!IsIconic(hwnd));
                    VERIFY_IS_FALSE(!!IsZoomed(hwnd));
                    const bool restoresSnapped = IsWindowSnapped(hwnd);
                    if (enabled)
                    {
                        VERIFY_IS_TRUE(restoresSnapped);
                    }
                    VerifyBoundsNear(hwnd, normal, GetNormalBoundsOnScreen(hwnd));
                    if (restoresSnapped)
                    {
                        VerifyBoundsNear(hwnd, snap, GetVisibleWindowBounds(hwnd));
                    }
                    else
                    {
                        RECT actual{};
                        VERIFY_IS_TRUE(!!GetWindowRect(hwnd, &actual));
                        VerifyBoundsEqual(normal, actual);
                    }
                    VerifySavedState(data, state);
                    window->Close();
                    VerifySavedState(data, restoresSnapped ? SavedWindowState::Snapped : SavedWindowState::Normal);
                    VERIFY_ARE_EQUAL(restoresSnapped, HasSavedSnapBounds(data.ReadRecord()));
                });
            }
        }
    }

}}}}}}
#endif
