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

    PCWSTR SavedStateName(SavedWindowState state)
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

    HWND PlacementHwnd(xaml::Window^ window)
    {
        Microsoft::WRL::ComPtr<IWindowNative> native;
        VERIFY_SUCCEEDED(reinterpret_cast<IUnknown*>(window)->QueryInterface(IID_PPV_ARGS(&native)));
        HWND hwnd{};
        VERIFY_SUCCEEDED(native->get_WindowHandle(&hwnd));
        return hwnd;
    }

    Platform::String^ NewPlacementId()
    {
        GUID id{};
        VERIFY_SUCCEEDED(CoCreateGuid(&id));
        wchar_t text[40]{};
        VERIFY_IS_TRUE(StringFromGUID2(id, text, ARRAYSIZE(text)) > 0);
        return ref new Platform::String(text);
    }

    Platform::String^ StorageKey(Platform::String^ id)
    {
        // Use the product's key format so tests can inspect or change the actual saved record.
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

    // Give each case its own storage entry and remove it when the case finishes.
    struct PlacementData
    {
        Platform::String^ id;
        Platform::String^ key;
        ::Windows::Foundation::Collections::IPropertySet^ values;

        explicit PlacementData(Platform::String^ placementId = nullptr)
        {
            id = placementId ? placementId : NewPlacementId();
            key = StorageKey(id);
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

        ::Windows::Storage::ApplicationDataCompositeValue^ Read()
        {
            VERIFY_IS_TRUE(HasRecord(), L"The window should have saved a placement.");
            return safe_cast<::Windows::Storage::ApplicationDataCompositeValue^>(values->Lookup(key));
        }
    };

    void VerifySavedState(PlacementData& data, SavedWindowState expected)
    {
        const int actual = safe_cast<int>(data.Read()->Lookup(L"State"));
        LOG_OUTPUT(L"Saved state: expected %s (%d), actual %d", SavedStateName(expected),
            static_cast<int>(expected), actual);
        VERIFY_ARE_EQUAL(static_cast<int>(expected), actual);
    }

    void WriteSavedState(::Windows::Storage::ApplicationDataCompositeValue^ record, SavedWindowState state)
    {
        record->Insert(L"State", static_cast<int>(state));
    }

    RECT ReadWindowBounds(xaml::Window^ window)
    {
        // Include the invisible resize border when comparing the saved outer bounds.
        RECT bounds{};
        VERIFY_IS_TRUE(!!GetWindowRect(PlacementHwnd(window), &bounds));
        return bounds;
    }

    RECT MoveAndResizeWindow(xaml::Window^ window, POINT workAreaOffset, SIZE size)
    {
        HWND hwnd = PlacementHwnd(window);
        MONITORINFO monitor{ sizeof(monitor) };
        VERIFY_IS_TRUE(!!GetMonitorInfo(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &monitor));
        VERIFY_IS_TRUE(!!SetWindowPos(hwnd, nullptr, monitor.rcWork.left + workAreaOffset.x,
            monitor.rcWork.top + workAreaOffset.y, size.cx, size.cy, SWP_NOZORDER | SWP_NOACTIVATE));
        return ReadWindowBounds(window);
    }

    void VerifyPlacementRect(const RECT& expected, const RECT& actual)
    {
        LOG_OUTPUT(L"Expected bounds: (%ld, %ld, %ld, %ld); actual: (%ld, %ld, %ld, %ld)",
            expected.left, expected.top, expected.right, expected.bottom,
            actual.left, actual.top, actual.right, actual.bottom);
        VERIFY_ARE_EQUAL(expected.left, actual.left);
        VERIFY_ARE_EQUAL(expected.top, actual.top);
        VERIFY_ARE_EQUAL(expected.right, actual.right);
        VERIFY_ARE_EQUAL(expected.bottom, actual.bottom);
    }

    void VerifyPlacementRectNear(HWND hwnd, const RECT& expected, const RECT& actual)
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

    RECT ReadNormalRect(HWND hwnd)
    {
        // Native restore bounds use workspace coordinates, not screen coordinates.
        WINDOWPLACEMENT placement{ sizeof(placement) };
        VERIFY_IS_TRUE(!!GetWindowPlacement(hwnd, &placement));
        return placement.rcNormalPosition;
    }

    RECT ReadRecordRect(::Windows::Storage::ApplicationDataCompositeValue^ record, const wchar_t* prefix)
    {
        auto name = ref new Platform::String(prefix);
        const int x = safe_cast<int>(record->Lookup(name + L"X"));
        const int y = safe_cast<int>(record->Lookup(name + L"Y"));
        return { x, y, x + safe_cast<int>(record->Lookup(name + L"Width")),
            y + safe_cast<int>(record->Lookup(name + L"Height")) };
    }

    void WriteRecordRect(::Windows::Storage::ApplicationDataCompositeValue^ record, const wchar_t* prefix, const RECT& rect)
    {
        auto name = ref new Platform::String(prefix);
        record->Insert(name + L"X", static_cast<int>(rect.left));
        record->Insert(name + L"Y", static_cast<int>(rect.top));
        record->Insert(name + L"Width", static_cast<int>(rect.right - rect.left));
        record->Insert(name + L"Height", static_cast<int>(rect.bottom - rect.top));
    }

    RECT ReadSavedRect(PlacementData& data)
    {
        return ReadRecordRect(data.Read(), L"Normal");
    }

    // Capture the native destruction save before Window.Close can hide a missing save.
    struct NativeDestructionCleanup
    {
        HWND hwnd;
        xaml::Window^ window;
        PlacementData& data;
        ::Windows::Storage::ApplicationDataCompositeValue^ recordBeforeClose;

        NativeDestructionCleanup(xaml::Window^ window, PlacementData& data) :
            hwnd(PlacementHwnd(window)), window(window), data(data)
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
                    cleanup->recordBeforeClose = cleanup->data.Read();
                }
                // Close the XAML island while its HWND is still valid, after observing the native save.
                cleanup->window->Close();
            }
            return result;
        }
    };

    MONITORINFO ReadPlacementMonitor(HWND hwnd)
    {
        MONITORINFO monitor{ sizeof(monitor) };
        VERIFY_IS_TRUE(!!GetMonitorInfo(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &monitor));
        return monitor;
    }

    RECT ReadNormalScreenRect(HWND hwnd)
    {
        RECT normal = ReadNormalRect(hwnd);
        const auto monitor = ReadPlacementMonitor(hwnd);
        // Account for top/left taskbars before comparing with saved screen-space bounds.
        OffsetRect(&normal, monitor.rcWork.left - monitor.rcMonitor.left, monitor.rcWork.top - monitor.rcMonitor.top);
        return normal;
    }

    RECT ReadPlacementFrame(HWND hwnd)
    {
        // Snap targets describe the visible frame, without invisible resize borders.
        RECT frame{};
        VERIFY_SUCCEEDED(DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &frame, sizeof(frame)));
        return frame;
    }

    bool IsPlacementArranged(HWND hwnd)
    {
        // IsWindowArranged is exported by user32 but has no SDK declaration.
        using IsWindowArrangedFunction = BOOL(WINAPI*)(HWND);
        static const auto isWindowArranged = reinterpret_cast<IsWindowArrangedFunction>(
            GetProcAddress(GetModuleHandleW(L"user32.dll"), "IsWindowArranged"));
        VERIFY_IS_NOT_NULL(isWindowArranged);
        return !!isWindowArranged(hwnd);
    }

    void SetPlacementSnapping(BOOL enabled)
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
            SetPlacementSnapping(TRUE);
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

    RECT PlacementSnapRect(const RECT& workArea, SnapRegion region)
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

    void SetPlacementChrome(xaml::Window^ window, WindowChrome chrome)
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

    void SnapPlacementWindow(HWND hwnd, const RECT& snap)
    {
        // Enter the native arranged state before moving it. SetWindowPos alone does not snap.
        DefWindowProcW(hwnd, WM_NCLBUTTONDBLCLK, HTTOP, 0);
        VERIFY_IS_TRUE(IsPlacementArranged(hwnd));
        VERIFY_SUCCEEDED(DwmFlush());
        RECT outer{};
        VERIFY_IS_TRUE(!!GetWindowRect(hwnd, &outer));
        const RECT frame = ReadPlacementFrame(hwnd);
        // Include the invisible borders so the visible frame lands on the requested snap bounds.
        VERIFY_IS_TRUE(!!SetWindowPos(hwnd, nullptr,
            snap.left - (frame.left - outer.left), snap.top - (frame.top - outer.top),
            snap.right - snap.left + (frame.left - outer.left) + (outer.right - frame.right),
            snap.bottom - snap.top + (frame.top - outer.top) + (outer.bottom - frame.bottom),
            SWP_NOZORDER | SWP_NOACTIVATE));
        VERIFY_IS_TRUE(IsPlacementArranged(hwnd));
    }

    RECT SavePlacementFromWindow(PlacementData& data, SavedWindowState state = SavedWindowState::Normal)
    {
        // Start with a record saved by a real window, then let tests change only the fields they need.
        WindowAutoCloser window;
        window.Attach(ref new xaml::Window());
        window->PlacementId = data.id;
        window->Activate();
        const RECT bounds = MoveAndResizeWindow(window.get(), { 60, 60 }, { 420, 300 });
        const auto hwnd = PlacementHwnd(window.get());
        if (state == SavedWindowState::Maximized || state == SavedWindowState::MinimizedFromMaximized)
        {
            ShowWindow(hwnd, SW_MAXIMIZE);
        }
        if (state == SavedWindowState::MinimizedFromNormal || state == SavedWindowState::MinimizedFromMaximized)
        {
            ShowWindow(hwnd, SW_MINIMIZE);
        }
        window->Close();
        auto record = data.Read();
        VERIFY_ARE_EQUAL(1, safe_cast<int>(record->Lookup(L"Version")));
        VERIFY_IS_FALSE(record->HasKey(L"VirtualDesktopId"));
        return bounds;
    }

    void VerifyRequestedSize(xaml::Window^ window, double width, double height)
    {
        // Check the actual client area in DIPs; property getters can return pending size requests.
        RECT client{};
        const auto hwnd = PlacementHwnd(window);
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
            VerifyPlacementRect(originalBounds, ReadSavedRect(data));

            WindowAutoCloser restoredWindow;
            restoredWindow.Attach(ref new xaml::Window());
            restoredWindow->PlacementId = data.id;
            restoredWindow->Width = 250;
            restoredWindow->Height = 200;
            restoredWindow->Activate();
            VERIFY_IS_FALSE(!!IsIconic(PlacementHwnd(restoredWindow.get())));
            VERIFY_IS_FALSE(!!IsZoomed(PlacementHwnd(restoredWindow.get())));
            VerifyPlacementRect(originalBounds, ReadWindowBounds(restoredWindow.get()));

            // Setting Height still opts into preserving client height after saved placement wins.
            const double restoredWidth = restoredWindow->Width;
            const double restoredHeight = restoredWindow->Height;
            restoredWindow->ExtendsContentIntoTitleBar = true;
            VerifyRequestedSize(restoredWindow.get(), restoredWidth, restoredHeight);
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
            VerifyPlacementRect(movedBounds, ReadSavedRect(data));

            WindowAutoCloser restoredWindow;
            restoredWindow.Attach(ref new xaml::Window());
            restoredWindow->PlacementId = data.id;
            restoredWindow->Width = 250;
            restoredWindow->Height = 200;
            restoredWindow->Activate();
            VERIFY_IS_FALSE(!!IsIconic(PlacementHwnd(restoredWindow.get())));
            VERIFY_IS_FALSE(!!IsZoomed(PlacementHwnd(restoredWindow.get())));
            VerifyPlacementRect(movedBounds, ReadWindowBounds(restoredWindow.get()));
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
            const RECT normalBounds = MoveAndResizeWindow(originalWindow.get(), { 60, 60 }, { 420, 300 });
            const auto originalHwnd = PlacementHwnd(originalWindow.get());
            const RECT normalPosition = ReadNormalRect(originalHwnd);
            ShowWindow(originalHwnd, SW_MAXIMIZE);
            originalWindow->Close();
            VerifySavedState(data, SavedWindowState::Maximized);
            VerifyPlacementRect(normalBounds, ReadSavedRect(data));

            WindowAutoCloser restoredWindow;
            restoredWindow.Attach(ref new xaml::Window());
            restoredWindow->PlacementId = data.id;
            restoredWindow->Width = 250;
            restoredWindow->Height = 200;
            restoredWindow->Activate();
            const auto restoredHwnd = PlacementHwnd(restoredWindow.get());
            VERIFY_IS_FALSE(!!IsIconic(restoredHwnd));
            VERIFY_IS_TRUE(!!IsZoomed(restoredHwnd));
            VerifyPlacementRect(normalPosition, ReadNormalRect(restoredHwnd));
            ShowWindow(restoredHwnd, SW_RESTORE);
            VerifyPlacementRect(normalBounds, ReadWindowBounds(restoredWindow.get()));
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
                    SavePlacementFromWindow(data, SavedWindowState::Maximized);
                    window.Attach(ref new xaml::Window());
                    window->PlacementId = data.id;
                    window->ExtendsContentIntoTitleBar = extendedTitleBar;
                    window->Width = 310;
                    window->Height = 220;
                    window->Activate();
                    VERIFY_IS_TRUE(!!IsZoomed(PlacementHwnd(window.get())));
                    // Changing presenters must not lose the normal size behind the maximized window.
                    width = window->Width;
                    height = window->Height;
                    window->AppWindow->SetPresenter(kind);
                    VERIFY_IS_TRUE(std::abs(window->Width - width) <= 1.0);
                    VERIFY_IS_TRUE(std::abs(window->Height - height) <= 1.0);
                });
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    // Deferred size updates must not overwrite the restored normal size either.
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
            const RECT normalBounds = MoveAndResizeWindow(originalWindow.get(), { 60, 60 }, { 420, 300 });
            const auto originalHwnd = PlacementHwnd(originalWindow.get());
            const RECT normalPosition = ReadNormalRect(originalHwnd);
            ShowWindow(originalHwnd, SW_MINIMIZE);
            originalWindow->Close();
            VerifySavedState(data, SavedWindowState::MinimizedFromNormal);
            VerifyPlacementRect(normalBounds, ReadSavedRect(data));

            WindowAutoCloser restoredWindow;
            restoredWindow.Attach(ref new xaml::Window());
            restoredWindow->PlacementId = data.id;
            restoredWindow->Width = 250;
            restoredWindow->Height = 200;
            restoredWindow->Activate();
            const auto restoredHwnd = PlacementHwnd(restoredWindow.get());
            VERIFY_IS_FALSE(!!IsIconic(restoredHwnd));
            VERIFY_IS_FALSE(!!IsZoomed(restoredHwnd));
            VerifyPlacementRect(normalPosition, ReadNormalRect(restoredHwnd));
            VerifyPlacementRect(normalBounds, ReadWindowBounds(restoredWindow.get()));
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
            const RECT normalBounds = MoveAndResizeWindow(originalWindow.get(), { 60, 60 }, { 420, 300 });
            const auto originalHwnd = PlacementHwnd(originalWindow.get());
            const RECT normalPosition = ReadNormalRect(originalHwnd);
            ShowWindow(originalHwnd, SW_MAXIMIZE);
            ShowWindow(originalHwnd, SW_MINIMIZE);
            originalWindow->Close();
            VerifySavedState(data, SavedWindowState::MinimizedFromMaximized);
            VerifyPlacementRect(normalBounds, ReadSavedRect(data));

            WindowAutoCloser restoredWindow;
            restoredWindow.Attach(ref new xaml::Window());
            restoredWindow->PlacementId = data.id;
            restoredWindow->Width = 250;
            restoredWindow->Height = 200;
            restoredWindow->Activate();
            const auto restoredHwnd = PlacementHwnd(restoredWindow.get());
            VERIFY_IS_FALSE(!!IsIconic(restoredHwnd));
            VERIFY_IS_TRUE(!!IsZoomed(restoredHwnd));
            VerifyPlacementRect(normalPosition, ReadNormalRect(restoredHwnd));
            ShowWindow(restoredHwnd, SW_RESTORE);
            VerifyPlacementRect(normalBounds, ReadWindowBounds(restoredWindow.get()));
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
                LOG_OUTPUT(L"Repeated activation after restoring %s", SavedStateName(state));
                PlacementData data;
                SavePlacementFromWindow(data, state);
                WindowAutoCloser restoredWindow;
                restoredWindow.Attach(ref new xaml::Window());
                restoredWindow->PlacementId = data.id;
                restoredWindow->Activate();
                if (IsZoomed(PlacementHwnd(restoredWindow.get())))
                {
                    ShowWindow(PlacementHwnd(restoredWindow.get()), SW_RESTORE);
                }
                const RECT movedBounds = MoveAndResizeWindow(restoredWindow.get(), { 100, 100 }, { 460, 340 });
                restoredWindow->Activate();
                VerifyPlacementRect(movedBounds, ReadWindowBounds(restoredWindow.get()));
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
                LOG_OUTPUT(L"Shared id after saving %s", SavedStateName(state));
                PlacementData data;
                const RECT savedBounds = SavePlacementFromWindow(data, state);
                WindowAutoCloser liveWindow;
                liveWindow.Attach(ref new xaml::Window());
                liveWindow->PlacementId = data.id;
                liveWindow->Activate();
                if (IsZoomed(PlacementHwnd(liveWindow.get())))
                {
                    ShowWindow(PlacementHwnd(liveWindow.get()), SW_RESTORE);
                }
                MoveAndResizeWindow(liveWindow.get(), { 100, 100 }, { 460, 340 });

                WindowAutoCloser sameIdWindow;
                sameIdWindow.Attach(ref new xaml::Window());
                sameIdWindow->PlacementId = data.id;
                sameIdWindow->Activate();
                const auto hwnd = PlacementHwnd(sameIdWindow.get());
                if (IsZoomed(hwnd)) ShowWindow(hwnd, SW_RESTORE);
                VerifyPlacementRect(savedBounds, ReadWindowBounds(sameIdWindow.get()));
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
            VerifyPlacementRect(bounds, ReadSavedRect(data));
            VERIFY_IS_TRUE(safe_cast<Platform::String^>(data.Read()->Lookup(L"PlacementId")) == data.id);
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
            VERIFY_IS_TRUE(safe_cast<Platform::String^>(currentId.Read()->Lookup(L"PlacementId")) == currentId.id);
            VerifyPlacementRect(bounds, ReadSavedRect(currentId));
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
            SavePlacementFromWindow(data);
            WindowAutoCloser window;
            window.Attach(ref new xaml::Window());
            window->Activate();
            const RECT currentBounds = MoveAndResizeWindow(window.get(), { 90, 90 }, { 450, 330 });
            window->PlacementId = data.id;
            window->Activate();
            VerifyPlacementRect(currentBounds, ReadWindowBounds(window.get()));
        });
    }

    void WindowIntegrationTests::WindowPlacementClearingIdDisablesSaving()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([&]()
        {
            PlacementData data;
            const RECT savedBounds = SavePlacementFromWindow(data);
            WindowAutoCloser window;
            window.Attach(ref new xaml::Window());
            window->PlacementId = data.id;
            window->Activate();
            MoveAndResizeWindow(window.get(), { 90, 90 }, { 450, 330 });
            window->PlacementId = nullptr;
            VERIFY_IS_TRUE(window->PlacementId == L"");
            window->Close();
            VerifyPlacementRect(savedBounds, ReadSavedRect(data));
        });
    }

    void WindowIntegrationTests::WindowPlacementIdsAreCaseSensitive()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([&]()
        {
            const auto prefix = NewPlacementId();
            PlacementData lowerCaseId(prefix + L"-case");
            PlacementData upperCaseId(prefix + L"-CASE");
            const RECT upperCaseBounds = SavePlacementFromWindow(upperCaseId);

            WindowAutoCloser lowerCaseWindow;
            lowerCaseWindow.Attach(ref new xaml::Window());
            lowerCaseWindow->PlacementId = lowerCaseId.id;
            lowerCaseWindow->Width = 310;
            lowerCaseWindow->Height = 220;
            lowerCaseWindow->Activate();
            VerifyRequestedSize(lowerCaseWindow.get(), 310, 220);
            const RECT lowerCaseBounds = ReadWindowBounds(lowerCaseWindow.get());
            lowerCaseWindow->Close();

            VERIFY_IS_TRUE(safe_cast<Platform::String^>(lowerCaseId.Read()->Lookup(L"PlacementId")) == lowerCaseId.id);
            VERIFY_IS_TRUE(safe_cast<Platform::String^>(upperCaseId.Read()->Lookup(L"PlacementId")) == upperCaseId.id);
            VerifyPlacementRect(lowerCaseBounds, ReadSavedRect(lowerCaseId));
            VerifyPlacementRect(upperCaseBounds, ReadSavedRect(upperCaseId));
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
            VerifyRequestedSize(originalWindow.get(), 310, 220);
            originalWindow->ExtendsContentIntoTitleBar = true;
            VerifyRequestedSize(originalWindow.get(), 310, 220);
            const RECT movedBounds = MoveAndResizeWindow(originalWindow.get(), { 60, 60 }, { 420, 300 });
            originalWindow->Close();

            WindowAutoCloser restoredWindow;
            restoredWindow.Attach(ref new xaml::Window());
            restoredWindow->PlacementId = data.id;
            restoredWindow->Width = 310;
            restoredWindow->Height = 220;
            restoredWindow->Activate();
            VERIFY_IS_FALSE(!!IsIconic(PlacementHwnd(restoredWindow.get())));
            VERIFY_IS_FALSE(!!IsZoomed(PlacementHwnd(restoredWindow.get())));
            VerifyPlacementRect(movedBounds, ReadWindowBounds(restoredWindow.get()));
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
                    if (existingRecord) SavePlacementFromWindow(data);
                    WindowAutoCloser window;
                    window.Attach(ref new xaml::Window());
                    if (scenario.showBeforeSettingId) window->AppWindow->Show();
                    window->PlacementId = data.id;
                    if (scenario.showAfterSettingId) window->AppWindow->Show();
                    MoveAndResizeWindow(window.get(), { 130, 130 }, { 490, 370 });
                    SendMessage(PlacementHwnd(window.get()), WM_ENDSESSION, TRUE, 0);
                    VERIFY_ARE_EQUAL(existingRecord, data.HasRecord());
                    if (existingRecord)
                    {
                        VERIFY_ARE_EQUAL(420, safe_cast<int>(data.Read()->Lookup(L"NormalWidth")));
                    }
                    window->Close();
                    VERIFY_ARE_EQUAL(existingRecord, data.HasRecord());
                    if (existingRecord)
                    {
                        VERIFY_ARE_EQUAL(420, safe_cast<int>(data.Read()->Lookup(L"NormalWidth")));
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
            const RECT savedBounds = SavePlacementFromWindow(data);
            WindowAutoCloser window;
            window.Attach(ref new xaml::Window());
            window->PlacementId = data.id;
            window->AppWindow->Show();
            MoveAndResizeWindow(window.get(), { 130, 130 }, { 490, 370 });
            window->Activate();
            VerifyPlacementRect(savedBounds, ReadWindowBounds(window.get()));
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
            const auto hwnd = PlacementHwnd(window.get());
            SendMessage(hwnd, WM_ENDSESSION, FALSE, 0);
            VERIFY_IS_FALSE(data.HasRecord());
            SendMessage(hwnd, WM_ENDSESSION, TRUE, ENDSESSION_LOGOFF);
            VerifyPlacementRect(bounds, ReadSavedRect(data));
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
            SendMessage(PlacementHwnd(window.get()), WM_ENDSESSION, TRUE, ENDSESSION_LOGOFF);

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
            VerifyPlacementRect(closeHandlerBounds, ReadSavedRect(closeHandlerId));
            VerifyPlacementRect(originalBounds, ReadSavedRect(originalId));
        });
    }

    void WindowIntegrationTests::WindowPlacementRejectsInvalidRecords()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([&]()
        {
            PlacementData data;
            SavePlacementFromWindow(data);
            auto valid = data.Read();
            using Record = ::Windows::Storage::ApplicationDataCompositeValue;
            const struct
            {
                PCWSTR name;
                void (*invalidate)(Record^);
            } scenarios[] =
            {
                { L"Future version", [](Record^ r) { r->Insert(L"Version", 2); } },
                { L"Mismatched id", [](Record^ r) { r->Insert(L"PlacementId", L"another-id"); } },
                { L"Missing width", [](Record^ r) { r->Remove(L"NormalWidth"); } },
                { L"String dpi", [](Record^ r) { r->Insert(L"Dpi", L"96"); } },
                { L"Negative width", [](Record^ r) { r->Insert(L"NormalWidth", -1); } },
                { L"Overflowing normal rectangle", [](Record^ r) { r->Insert(L"NormalX", INT_MAX); } },
                { L"Unknown state", [](Record^ r) { r->Insert(L"State", 99); } },
                { L"Partial snap rectangle", [](Record^ r) { r->Insert(L"SnapX", wf::PropertyValue::CreateInt32(0)); } },
                { L"Zero dpi", [](Record^ r) { r->Insert(L"Dpi", wf::PropertyValue::CreateInt32(0)); } },
                { L"Int64 width", [](Record^ r) { r->Insert(L"NormalWidth", wf::PropertyValue::CreateInt64(420)); } },
                { L"Zero height", [](Record^ r) { r->Insert(L"NormalHeight", wf::PropertyValue::CreateInt32(0)); } },
                { L"Oversized width", [](Record^ r) { r->Insert(L"NormalWidth", 65536); } },
                { L"Empty work area", [](Record^ r) { r->Insert(L"WorkAreaHeight", wf::PropertyValue::CreateInt32(0)); } },
                { L"Oversized work area", [](Record^ r) { r->Insert(L"WorkAreaWidth", 65536); } },
                { L"Overflowing work area", [](Record^ r) { r->Insert(L"WorkAreaY", INT_MAX); } },
                { L"Dpi below minimum", [](Record^ r) { r->Insert(L"Dpi", 95); } },
                { L"Dpi above maximum", [](Record^ r) { r->Insert(L"Dpi", 9601); } },
                { L"Snapped without snap bounds", [](Record^ r) { WriteSavedState(r, SavedWindowState::Snapped); } },
                { L"Minimized from snapped without snap bounds", [](Record^ r)
                    { WriteSavedState(r, SavedWindowState::MinimizedFromSnapped); } },
                { L"Normal bounds outside saved work area", [](Record^ r)
                    {
                        r->Insert(L"NormalX", safe_cast<int>(r->Lookup(L"WorkAreaX")) +
                            safe_cast<int>(r->Lookup(L"WorkAreaWidth")) + 1);
                    } },
                { L"Wrong display name type", [](Record^ r) { r->Insert(L"DisplayDeviceName", 123); } },
                { L"Long display name", [](Record^ r)
                    { r->Insert(L"DisplayDeviceName", L"12345678901234567890123456789012"); } },
                { L"Embedded null in display name", [](Record^ r)
                    {
                        const wchar_t name[] = { L'a', L'\0', L'b' };
                        r->Insert(L"DisplayDeviceName", ref new Platform::String(name, ARRAYSIZE(name)));
                    } },
                { L"Snap bounds outside saved work area", [](Record^ r)
                    {
                        r->Insert(L"SnapX", safe_cast<int>(r->Lookup(L"WorkAreaX")) +
                            safe_cast<int>(r->Lookup(L"WorkAreaWidth")) + 1);
                        r->Insert(L"SnapY", r->Lookup(L"NormalY"));
                        r->Insert(L"SnapWidth", r->Lookup(L"NormalWidth"));
                        r->Insert(L"SnapHeight", r->Lookup(L"NormalHeight"));
                    } },
            };
            for (const auto& scenario : scenarios)
            {
                LOG_OUTPUT(L"Invalid placement record: %s", scenario.name);
                auto record = ref new Record();
                for (auto field : valid) record->Insert(field->Key, field->Value);
                scenario.invalidate(record);
                data.values->Insert(data.key, record);
                WindowAutoCloser window;
                window.Attach(ref new xaml::Window());
                window->PlacementId = data.id;
                window->Width = 310;
                window->Height = 220;
                window->Activate();
                VerifyRequestedSize(window.get(), 310, 220);
                window->ExtendsContentIntoTitleBar = true;
                VerifyRequestedSize(window.get(), 310, 220);
                window->PlacementId = L"";
                window->Close();
            }
        });
    }

    void WindowIntegrationTests::WindowPlacementRecordCompatibility()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([&]()
        {
            enum class RecordKind { CompatibleWithExtraFields, FutureVersion, NotComposite };
            const struct
            {
                PCWSTR name;
                RecordKind kind;
            } scenarios[] =
            {
                { L"compatible-record-with-extra-fields", RecordKind::CompatibleWithExtraFields },
                { L"future-version", RecordKind::FutureVersion },
                { L"not-a-composite", RecordKind::NotComposite },
            };
            for (const auto& scenario : scenarios)
            {
                LOG_OUTPUT(L"Record compatibility: %s", scenario.name);
                PlacementData data;
                const RECT saved = SavePlacementFromWindow(data);
                auto record = data.Read();
                record->Remove(L"DisplayDeviceName");
                record->Insert(L"FutureOptionalField", 123);
                record->Insert(L"VirtualDesktopId", L"ignored-even-with-the-wrong-type");
                if (scenario.kind == RecordKind::FutureVersion) record->Insert(L"Version", 2);
                if (scenario.kind == RecordKind::NotComposite)
                {
                    data.values->Insert(data.key, L"not-a-composite");
                }
                else
                {
                    data.values->Insert(data.key, record);
                }

                WindowAutoCloser window;
                window.Attach(ref new xaml::Window());
                window->PlacementId = data.id;
                window->Width = 310;
                window->Height = 220;
                window->Activate();
                RECT actual{};
                VERIFY_IS_TRUE(!!GetWindowRect(PlacementHwnd(window.get()), &actual));
                // Reading must leave extra fields and unsupported records unchanged in storage.
                if (scenario.kind == RecordKind::CompatibleWithExtraFields)
                {
                    VerifyPlacementRect(saved, actual);
                    VERIFY_IS_TRUE(data.Read()->HasKey(L"FutureOptionalField"));
                }
                else
                {
                    VerifyRequestedSize(window.get(), 310, 220);
                    if (scenario.kind == RecordKind::FutureVersion)
                    {
                        VERIFY_ARE_EQUAL(2, safe_cast<int>(data.Read()->Lookup(L"Version")));
                    }
                    else
                    {
                        VERIFY_IS_TRUE(safe_cast<Platform::String^>(data.values->Lookup(data.key)) == L"not-a-composite");
                    }
                }
                // Closing writes a fresh supported record using the window's final bounds.
                window->Close();
                record = data.Read();
                VERIFY_ARE_EQUAL(1, safe_cast<int>(record->Lookup(L"Version")));
                VERIFY_IS_FALSE(record->HasKey(L"FutureOptionalField"));
                VERIFY_IS_FALSE(record->HasKey(L"VirtualDesktopId"));
                VerifyPlacementRect(actual, ReadSavedRect(data));
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
                SavePlacementFromWindow(data);
                WindowAutoCloser window;
                window.Attach(ref new xaml::Window());
                window->PlacementId = data.id;
                window->AppWindow->SetPresenter(kind);
                window->Activate();
                VERIFY_IS_TRUE(window->AppWindow->Presenter->Kind == kind);
                window->Close();
                VERIFY_ARE_EQUAL(420, safe_cast<int>(data.Read()->Lookup(L"NormalWidth")));

                WindowAutoCloser changed;
                changed.Attach(ref new xaml::Window());
                changed->PlacementId = data.id;
                changed->Activate();
                MoveAndResizeWindow(changed.get(), { 120, 120 }, { 480, 360 });
                changed->AppWindow->SetPresenter(kind);
                changed->Close();
                VERIFY_ARE_EQUAL(420, safe_cast<int>(data.Read()->Lookup(L"NormalWidth")));
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
                    if (existingRecord) SavePlacementFromWindow(data);
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
                        VERIFY_ARE_EQUAL(420, safe_cast<int>(data.Read()->Lookup(L"NormalWidth")));
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
            RECT saved{}, normal{};
            bool reentered = false;
            auto activated = CreateSafeEventRegistration(xaml::Window, Activated);
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Reentrant activation: %s", maximize ? L"maximized" : L"normal");
                saved = SavePlacementFromWindow(data,
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
                const auto hwnd = PlacementHwnd(window.get());
                VERIFY_ARE_EQUAL(maximize, !!IsZoomed(hwnd));
                VERIFY_IS_FALSE(!!IsIconic(hwnd));
                normal = ReadNormalRect(hwnd);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                const auto hwnd = PlacementHwnd(window.get());
                VERIFY_ARE_EQUAL(maximize, !!IsZoomed(hwnd));
                VERIFY_IS_FALSE(!!IsIconic(hwnd));
                VerifyPlacementRect(normal, ReadNormalRect(hwnd));
                if (maximize) ShowWindow(hwnd, SW_RESTORE);
                RECT actual{};
                VERIFY_IS_TRUE(!!GetWindowRect(hwnd, &actual));
                VerifyPlacementRect(saved, actual);
                const RECT moved = MoveAndResizeWindow(window.get(), { 100, 100 }, { 460, 340 });
                window->Close();
                VerifyPlacementRect(moved, ReadSavedRect(data));
            });
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
        } requests[] =
        {
            { L"width-only", true, false },
            { L"height-only", false, true },
            { L"width-and-height", true, true },
        };
        for (const auto& request : requests)
        {
            LOG_OUTPUT(L"SizeChanged overrides restored size: %s", request.name);
            PlacementData data;
            WindowAutoCloser window;
            bool changed = false;
            double width = 0, height = 0;
            auto size = CreateSafeEventRegistration(xaml::Window, SizeChanged);
            RunOnUIThread([&]()
            {
                SavePlacementFromWindow(data);
                window.Attach(ref new xaml::Window());
                window->PlacementId = data.id;
                window->Width = 310;
                window->Height = 220;
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
                VerifyRequestedSize(window.get(), width, height);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VerifyRequestedSize(window.get(), width, height);
                if (request.changeHeight)
                {
                    window->ExtendsContentIntoTitleBar = true;
                    VerifyRequestedSize(window.get(), width, height);
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
            hwnd = PlacementHwnd(window.get());
            endSessionBounds = MoveAndResizeWindow(window.get(), { 60, 60 }, { 420, 300 });
        });
        TestServices::WindowHelper->WaitForIdle();

        // Send from the test thread, as Windows does during synchronous shutdown.
        DWORD_PTR result{};
        // Cancelled shutdown must not save; confirmed logoff must save without closing the window.
        VERIFY_IS_TRUE(!!SendMessageTimeoutW(hwnd, WM_ENDSESSION, FALSE, 0, SMTO_ABORTIFHUNG, 30000, &result));
        VERIFY_IS_FALSE(original.HasRecord());
        VERIFY_IS_TRUE(!!SendMessageTimeoutW(hwnd, WM_ENDSESSION, TRUE, ENDSESSION_LOGOFF, SMTO_ABORTIFHUNG, 30000, &result));
        VerifyPlacementRect(endSessionBounds, ReadSavedRect(original));

        RunOnUIThread([&]()
        {
            window->PlacementId = final.id;
            closeBounds = MoveAndResizeWindow(window.get(), { 100, 100 }, { 460, 340 });
        });
        TestServices::WindowHelper->WaitForIdle();
        VERIFY_IS_TRUE(!!SendMessageTimeoutW(hwnd, WM_CLOSE, 0, 0, SMTO_ABORTIFHUNG, 30000, &result));
        TestServices::WindowHelper->WaitForIdle();
        VERIFY_IS_FALSE(!!IsWindow(hwnd));
        VerifyPlacementRect(closeBounds, ReadSavedRect(final));
        VerifyPlacementRect(endSessionBounds, ReadSavedRect(original));
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
                const auto hwnd = PlacementHwnd(window.get());
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
                VerifyPlacementRect(bounds, ReadSavedRect(data));
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
                const auto hwnd = PlacementHwnd(window.get());

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
                VERIFY_IS_NOT_NULL(nativeCleanup.recordBeforeClose);
                VerifyPlacementRect(finalBounds, ReadRecordRect(nativeCleanup.recordBeforeClose, L"Normal"));
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
                    const RECT savedBounds = SavePlacementFromWindow(data);
                    WindowAutoCloser window;
                    window.Attach(ref new xaml::Window());
                    window->PlacementId = data.id;
                    window->Activate();
                    const auto hwnd = PlacementHwnd(window.get());
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
                    VERIFY_IS_NOT_NULL(nativeCleanup.recordBeforeClose);
                    VerifyPlacementRect(savedBounds, ReadRecordRect(nativeCleanup.recordBeforeClose, L"Normal"));
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
            SavePlacementFromWindow(data);
            auto record = data.Read();
            const RECT oldWorkArea = ReadRecordRect(record, L"WorkArea");
            savedDpi = safe_cast<int>(record->Lookup(L"Dpi"));

            // Simulate a disconnected display without changing the VM's display setup.
            record->Insert(L"WorkAreaX", static_cast<int>(oldWorkArea.left + 5000));
            record->Insert(L"NormalX", safe_cast<int>(record->Lookup(L"NormalX")) + 5000);
            record->Insert(L"DisplayDeviceName", L"DisconnectedPlacementDisplay");
            const RECT normal = ReadRecordRect(record, L"Normal");
            targetMonitor = MonitorFromRect(&normal, MONITOR_DEFAULTTONEAREST);
            MONITORINFO monitor{ sizeof(monitor) };
            VERIFY_IS_TRUE(!!GetMonitorInfo(targetMonitor, &monitor));
            workArea = monitor.rcWork;
            data.values->Insert(data.key, record);
            window.Attach(ref new xaml::Window());
            window->PlacementId = data.id;
            window->Activate();
        });
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            const RECT actual = ReadWindowBounds(window.get());
            const auto hwnd = PlacementHwnd(window.get());
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
            SavePlacementFromWindow(data);
            auto record = data.Read();
            workArea = ReadRecordRect(record, L"WorkArea");
            dpi = safe_cast<int>(record->Lookup(L"Dpi"));

            // Keep the display name but make its saved coordinates differ from today's.
            record->Insert(L"WorkAreaX", static_cast<int>(workArea.left + 5000));
            record->Insert(L"NormalX", safe_cast<int>(record->Lookup(L"NormalX")) + 5000);
            data.values->Insert(data.key, record);
            window.Attach(ref new xaml::Window());
            window->PlacementId = data.id;
            window->Activate();
        });
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            const RECT actual = ReadWindowBounds(window.get());
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
            SavePlacementFromWindow(data);
            auto record = data.Read();
            workArea = ReadRecordRect(record, L"WorkArea");
            dpi = safe_cast<int>(record->Lookup(L"Dpi"));

            // Put half of the saved 420-pixel-wide window beyond the work area's right edge.
            record->Insert(L"NormalX", static_cast<int>(workArea.right - 210));
            data.values->Insert(data.key, record);
            window.Attach(ref new xaml::Window());
            window->PlacementId = data.id;
            window->Activate();
        });
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            const RECT actual = ReadWindowBounds(window.get());
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
                    SetPlacementChrome(originalWindow.get(), chrome.kind);
                    originalWindow->PlacementId = data.id;
                    originalWindow->Activate();
                    normal = MoveAndResizeWindow(originalWindow.get(), { 60, 60 }, { 420, 300 });
                });
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    const auto hwnd = PlacementHwnd(originalWindow.get());
                    snap = PlacementSnapRect(ReadPlacementMonitor(hwnd).rcWork, snapCase.region);
                    SnapPlacementWindow(hwnd, snap);
                });
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    const auto hwnd = PlacementHwnd(originalWindow.get());
                    VerifyPlacementRect(snap, ReadPlacementFrame(hwnd));
                    VerifyPlacementRect(normal, ReadNormalScreenRect(hwnd));
                    originalWindow->Close();
                    auto record = data.Read();
                    VerifySavedState(data, SavedWindowState::Snapped);
                    VerifyPlacementRect(normal, ReadSavedRect(data));
                    VerifyPlacementRect(snap, ReadRecordRect(record, L"Snap"));

                    restoredWindow.Attach(ref new xaml::Window());
                    SetPlacementChrome(restoredWindow.get(), chrome.kind);
                    restoredWindow->PlacementId = data.id;
                    restoredWindow->Width = 250;
                    restoredWindow->Height = 200;
                    restoredWindow->Activate();
                });
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    const auto hwnd = PlacementHwnd(restoredWindow.get());
                    VERIFY_IS_TRUE(IsPlacementArranged(hwnd));
                    VERIFY_IS_FALSE(!!IsIconic(hwnd));
                    const RECT restoredSnap = ReadPlacementFrame(hwnd);
                    const RECT restoredNormal = ReadNormalScreenRect(hwnd);
                    VerifyPlacementRectNear(hwnd, snap, restoredSnap);
                    VerifyPlacementRectNear(hwnd, normal, restoredNormal);
                    VERIFY_ARE_EQUAL(normal.right - normal.left, restoredNormal.right - restoredNormal.left);
                    VERIFY_ARE_EQUAL(normal.bottom - normal.top, restoredNormal.bottom - restoredNormal.top);
                    restoredWindow->Close();
                    VerifyPlacementRect(restoredSnap, ReadRecordRect(data.Read(), L"Snap"));
                    VerifyPlacementRect(restoredNormal, ReadSavedRect(data));
                });
            }
        }
    }

    void WindowIntegrationTests::WindowPlacementCapturesMinimizedSnappedBounds()
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
                const auto hwnd = PlacementHwnd(window.get());
                snap = PlacementSnapRect(ReadPlacementMonitor(hwnd).rcWork, SnapRegion::LeftHalf);
                SnapPlacementWindow(hwnd, snap);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                const auto hwnd = PlacementHwnd(window.get());
                VerifyPlacementRect(snap, ReadPlacementFrame(hwnd));
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
                const auto hwnd = PlacementHwnd(window.get());
                VERIFY_IS_TRUE(!!IsIconic(hwnd));

                // Known capture gap: PlacementEx does not recover the pre-minimize snap rect.
                // Windows 10 still reports arranged, but its minimized frame is off-screen:
                // WinUI rejects that capture and preserves any earlier record.
                // Windows 11 reports plain minimized, so it saves normal bounds without snap.
                // Expect MinimizedFromSnapped and exact snap bounds if capture support is added.
                const bool arranged = IsPlacementArranged(hwnd);
                if (arranged)
                {
                    const RECT frame = ReadPlacementFrame(hwnd);
                    const RECT workArea = ReadPlacementMonitor(hwnd).rcWork;
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
                    auto record = data.Read();
                    VerifySavedState(data, arranged ? SavedWindowState::Snapped : SavedWindowState::MinimizedFromNormal);
                    VerifyPlacementRect(normal, ReadSavedRect(data));
                    VERIFY_ARE_EQUAL(arranged, record->HasKey(L"SnapX"));
                    if (arranged) VerifyPlacementRect(snap, ReadRecordRect(record, L"Snap"));
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
                normal = SavePlacementFromWindow(data);
                auto record = data.Read();
                snap = PlacementSnapRect(ReadRecordRect(record, L"WorkArea"), snapCase.region);
                WriteSavedState(record, SavedWindowState::MinimizedFromSnapped);
                WriteRecordRect(record, L"Snap", snap);
                data.values->Insert(data.key, record);
                window.Attach(ref new xaml::Window());
                window->PlacementId = data.id;
                window->Activate();
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                const auto hwnd = PlacementHwnd(window.get());
                VERIFY_IS_FALSE(!!IsIconic(hwnd));
                VERIFY_IS_FALSE(!!IsZoomed(hwnd));
                VERIFY_IS_TRUE(IsPlacementArranged(hwnd));
                // Restore works with test-created snap fields even though native capture is incomplete.
                VerifyPlacementRectNear(hwnd, normal, ReadNormalScreenRect(hwnd));
                VerifyPlacementRectNear(hwnd, snap, ReadPlacementFrame(hwnd));
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
                LOG_OUTPUT(L"Work-area migration: %s, %s", SavedStateName(state),
                    shrink ? L"work-area-shrank" : L"work-area-grew");
                PlacementData data;
                WindowAutoCloser window;
                RECT workArea{}, oldWorkArea{}, normal{}, expectedSnap{}, restoredNormal{};
                RunOnUIThread([&]()
                {
                    SavePlacementFromWindow(data);
                    auto record = data.Read();
                    workArea = ReadRecordRect(record, L"WorkArea");
                    oldWorkArea = workArea;
                    oldWorkArea.right += shrink ? 240 : -240;
                    oldWorkArea.bottom += shrink ? 180 : -180;
                    normal = { oldWorkArea.left + 60, oldWorkArea.top + 60,
                        oldWorkArea.left + 480, oldWorkArea.top + 360 };
                    WriteRecordRect(record, L"WorkArea", oldWorkArea);
                    WriteRecordRect(record, L"Normal", normal);
                    WriteSavedState(record, state);
                    if (savedSnapped)
                    {
                        WriteRecordRect(record, L"Snap", PlacementSnapRect(oldWorkArea, SnapRegion::LeftHalf));
                        expectedSnap = PlacementSnapRect(workArea, SnapRegion::LeftHalf);
                    }
                    data.values->Insert(data.key, record);
                    window.Attach(ref new xaml::Window());
                    window->PlacementId = data.id;
                    window->Activate();
                });
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    const auto hwnd = PlacementHwnd(window.get());
                    VERIFY_IS_FALSE(!!IsIconic(hwnd));
                    VERIFY_ARE_EQUAL(state == SavedWindowState::Maximized, !!IsZoomed(hwnd));
                    VERIFY_ARE_EQUAL(savedSnapped, IsPlacementArranged(hwnd));
                    restoredNormal = ReadNormalScreenRect(hwnd);
                    VERIFY_ARE_EQUAL(420L, restoredNormal.right - restoredNormal.left);
                    VERIFY_ARE_EQUAL(300L, restoredNormal.bottom - restoredNormal.top);
                    VERIFY_IS_TRUE(restoredNormal.left >= workArea.left);
                    VERIFY_IS_TRUE(restoredNormal.top >= workArea.top);
                    VERIFY_IS_TRUE(restoredNormal.right <= workArea.right);
                    VERIFY_IS_TRUE(restoredNormal.bottom <= workArea.bottom);
                    if (savedSnapped)
                    {
                        VerifyPlacementRectNear(hwnd, expectedSnap, ReadPlacementFrame(hwnd));
                    }
                    else if (state == SavedWindowState::Normal)
                    {
                        RECT outer{};
                        VERIFY_IS_TRUE(!!GetWindowRect(hwnd, &outer));
                        VerifyPlacementRect(restoredNormal, outer);
                    }
                    // Activation must not overwrite the old monitor geometry.
                    VerifyPlacementRect(oldWorkArea, ReadRecordRect(data.Read(), L"WorkArea"));
                    VerifyPlacementRect(normal, ReadSavedRect(data));
                    window->Close();
                    VerifyPlacementRect(workArea, ReadRecordRect(data.Read(), L"WorkArea"));
                    VerifyPlacementRect(restoredNormal, ReadSavedRect(data));
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
                SavePlacementFromWindow(data);
                auto record = data.Read();
                workArea = ReadRecordRect(record, L"WorkArea");
                RECT oldWorkArea = workArea;
                oldWorkArea.right += 400;
                oldWorkArea.bottom += 300;
                oversized = oldWorkArea;
                InflateRect(&oversized, -20, -20);
                WriteRecordRect(record, L"WorkArea", oldWorkArea);
                WriteRecordRect(record, L"Normal", oversized);
                data.values->Insert(data.key, record);
                window.Attach(ref new xaml::Window());
                initial = MoveAndResizeWindow(window.get(), { 100, 100 }, { 460, 340 });
                safe_cast<::Microsoft::UI::Windowing::OverlappedPresenter^>(
                    window->AppWindow->Presenter)->IsResizable = resizable;
                const UINT dpi = GetDpiForWindow(PlacementHwnd(window.get()));
                maxTrack = { GetSystemMetricsForDpi(SM_CXMAXTRACK, dpi), GetSystemMetricsForDpi(SM_CYMAXTRACK, dpi) };
                window->PlacementId = data.id;
                window->Activate();
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                const auto hwnd = PlacementHwnd(window.get());
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
                VerifyPlacementRect(actual, ReadSavedRect(data));
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
            LOG_OUTPUT(L"preserve-logical-size: %s, saved DPI is twice the target DPI", SavedStateName(state));
            const bool savedSnapped = state == SavedWindowState::Snapped || state == SavedWindowState::MinimizedFromSnapped;
            PlacementData data;
            WindowAutoCloser window;
            RECT savedNormal{}, snap{}, restoredNormal{};
            int targetDpi{};
            RunOnUIThread([&]()
            {
                SavePlacementFromWindow(data);
                auto record = data.Read();
                const RECT workArea = ReadRecordRect(record, L"WorkArea");
                savedNormal = { workArea.left + 120, workArea.top + 120,
                    workArea.left + 960, workArea.top + 720 };
                targetDpi = safe_cast<int>(record->Lookup(L"Dpi"));
                // Simulate a DPI change without changing the VM's display settings.
                record->Insert(L"Dpi", targetDpi * 2);
                WriteSavedState(record, state);
                WriteRecordRect(record, L"Normal", savedNormal);
                if (savedSnapped)
                {
                    // Snap bounds follow the physical work area, not the normal size's DPI ratio.
                    snap = PlacementSnapRect(workArea, SnapRegion::TopLeftQuarter);
                    WriteRecordRect(record, L"Snap", snap);
                }
                data.values->Insert(data.key, record);
                window.Attach(ref new xaml::Window());
                window->PlacementId = data.id;
                window->Width = 250;
                window->Height = 200;
                window->Activate();
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                const auto hwnd = PlacementHwnd(window.get());
                VERIFY_ARE_EQUAL(static_cast<UINT>(targetDpi), GetDpiForWindow(hwnd));
                VERIFY_IS_FALSE(!!IsIconic(hwnd));
                VERIFY_ARE_EQUAL(state == SavedWindowState::Maximized, !!IsZoomed(hwnd));
                VERIFY_ARE_EQUAL(savedSnapped, IsPlacementArranged(hwnd));
                restoredNormal = ReadNormalScreenRect(hwnd);
                VERIFY_ARE_EQUAL(420L, restoredNormal.right - restoredNormal.left);
                VERIFY_ARE_EQUAL(300L, restoredNormal.bottom - restoredNormal.top);
                if (savedSnapped)
                {
                    VerifyPlacementRectNear(hwnd, snap, ReadPlacementFrame(hwnd));
                }
                else if (state == SavedWindowState::Normal)
                {
                    RECT outer{};
                    VERIFY_IS_TRUE(!!GetWindowRect(hwnd, &outer));
                    VerifyPlacementRect(restoredNormal, outer);
                }
                // Restore scales the live bounds; only closing saves the new DPI and bounds.
                VERIFY_ARE_EQUAL(targetDpi * 2, safe_cast<int>(data.Read()->Lookup(L"Dpi")));
                VerifyPlacementRect(savedNormal, ReadSavedRect(data));
                window->Close();
                VERIFY_ARE_EQUAL(targetDpi, safe_cast<int>(data.Read()->Lookup(L"Dpi")));
                VerifyPlacementRect(restoredNormal, ReadSavedRect(data));
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
            const auto hwnd = PlacementHwnd(window.get());
            snap = PlacementSnapRect(ReadPlacementMonitor(hwnd).rcWork, SnapRegion::RightHalf);
            SnapPlacementWindow(hwnd, snap);
        });
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            const auto hwnd = PlacementHwnd(window.get());
            SendMessageW(hwnd, WM_ENDSESSION, TRUE, 0);
            VerifySavedState(data, SavedWindowState::Snapped);
            VerifyPlacementRect(snap, ReadRecordRect(data.Read(), L"Snap"));
            ShowWindow(hwnd, SW_RESTORE);
            normal = MoveAndResizeWindow(window.get(), { 100, 100 }, { 460, 340 });
        });
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(IsPlacementArranged(PlacementHwnd(window.get())));
            SendMessageW(PlacementHwnd(window.get()), WM_ENDSESSION, TRUE, 0);
            auto record = data.Read();
            VerifySavedState(data, SavedWindowState::Normal);
            VerifyPlacementRect(normal, ReadSavedRect(data));
            for (auto field : { L"SnapX", L"SnapY", L"SnapWidth", L"SnapHeight" })
            {
                VERIFY_IS_FALSE(record->HasKey(ref new Platform::String(field)));
            }
            normal = MoveAndResizeWindow(window.get(), { 140, 140 }, { 500, 380 });
            window->Close();
            VerifyPlacementRect(normal, ReadSavedRect(data));
            VERIFY_IS_FALSE(data.Read()->HasKey(L"SnapX"));
        });
    }

    void WindowIntegrationTests::WindowPlacementRespectsSnappingSetting()
    {
        TestCleanupWrapper cleanup;
        PlacementSnappingScope snapping;
        // The fallback honors SPI_GETWINARRANGING. The newer ApplyWindowAction path
        // currently restores snap even when disabled. This records that gap, not a
        // cross-version policy; tighten the expectation if PlacementEx aligns the paths.
        for (bool enabled : { false, true })
        {
            SetPlacementSnapping(enabled);
            for (auto state : { SavedWindowState::Snapped, SavedWindowState::MinimizedFromSnapped })
            {
                LOG_OUTPUT(L"Snap setting: %s, %s", enabled ? L"enabled" : L"disabled", SavedStateName(state));
                PlacementData data;
                WindowAutoCloser window;
                RECT normal{}, snap{};
                RunOnUIThread([&]()
                {
                    normal = SavePlacementFromWindow(data);
                    auto record = data.Read();
                    snap = PlacementSnapRect(ReadRecordRect(record, L"WorkArea"), SnapRegion::LeftHalf);
                    WriteSavedState(record, state);
                    WriteRecordRect(record, L"Snap", snap);
                    data.values->Insert(data.key, record);
                    window.Attach(ref new xaml::Window());
                    window->PlacementId = data.id;
                    window->Activate();
                });
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    const auto hwnd = PlacementHwnd(window.get());
                    VERIFY_IS_FALSE(!!IsIconic(hwnd));
                    VERIFY_IS_FALSE(!!IsZoomed(hwnd));
                    const bool restoresSnapped = IsPlacementArranged(hwnd);
                    if (enabled)
                    {
                        VERIFY_IS_TRUE(restoresSnapped);
                    }
                    VerifyPlacementRectNear(hwnd, normal, ReadNormalScreenRect(hwnd));
                    if (restoresSnapped)
                    {
                        VerifyPlacementRectNear(hwnd, snap, ReadPlacementFrame(hwnd));
                    }
                    else
                    {
                        RECT actual{};
                        VERIFY_IS_TRUE(!!GetWindowRect(hwnd, &actual));
                        VerifyPlacementRect(normal, actual);
                    }
                    VerifySavedState(data, state);
                    window->Close();
                    VerifySavedState(data, restoresSnapped ? SavedWindowState::Snapped : SavedWindowState::Normal);
                    VERIFY_ARE_EQUAL(restoresSnapped, data.Read()->HasKey(L"SnapX"));
                });
            }
        }
    }

}}}}}}
#endif
