// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once

#include <wil/resource.h>
#include "WindowHelpers.h"

extern HINSTANCE g_hInstance;

template<typename T>
class BaseWindow
{
protected:
    void _CreateWindow(
        _In_ DWORD dwExStyle,
        _In_opt_ LPCWSTR lpClassName,
        _In_opt_ LPCWSTR lpWindowName,
        _In_ DWORD dwStyle,
        _In_ int X,
        _In_ int Y,
        _In_ int nWidth,
        _In_ int nHeight,
        _In_opt_ HWND hWndParent,
        _In_opt_ HMENU hMenu) noexcept
    {
        const auto result = ::CreateWindowEx(
            dwExStyle,
            lpClassName,
            lpWindowName,
            dwStyle,
            X, Y, nWidth, nHeight,
            hWndParent,
            hMenu,
            g_hInstance,
            (LPVOID)static_cast<T*>(this));
        if (!m_hwnd.get() || !result)
        {
            // A win32 window creation failure is a non-recoverable error.
            // m_hwnd is initialized during WM_NCCREATE
            IFCFAILFAST(HRESULT_FROM_WIN32(GetLastError()));
        }
    }

    [[nodiscard]] ATOM _RegisterClass(_In_ WNDCLASSEX* wndClassEx) noexcept
    {
        wndClassEx->hInstance = g_hInstance;
        wndClassEx->lpfnWndProc = WndProc;
        // A win32 class registration failure is a non-recoverable error. 
        const auto result = RegisterClassExW(wndClassEx);
        FAIL_FAST_ASSERT(result);
        return result;
    }

    virtual ~BaseWindow()
    {
        if (m_hwnd)
        {
            ::DestroyWindow(m_hwnd.get());
            m_hwnd = NULL;
        }
    }

private:
    static T* GetThisFromHandle(HWND const window) noexcept
    {
        return reinterpret_cast<T*>(GetWindowLongPtr(window, GWLP_USERDATA));
    }

    [[nodiscard]] static LRESULT __stdcall WndProc(HWND const window, UINT const message, WPARAM const wparam, LPARAM const lparam) noexcept
    {
        ASSERT(window);

        if (WM_NCCREATE == message)
        {
            auto cs = reinterpret_cast<CREATESTRUCT*>(lparam);
            T* that = static_cast<T*>(cs->lpCreateParams);
            ASSERT(that);
            ASSERT(!that->m_hwnd);
            that->m_hwnd = wil::unique_hwnd(window);
            ::SetWindowLongPtr(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(that));

            return that->OnNcCreate(wparam, lparam);
        }
        else if (T* that = GetThisFromHandle(window))
        {
            return that->MessageHandler(message, wparam, lparam);
        }

        return DefWindowProc(window, message, wparam, lparam);
    }

    [[nodiscard]] LRESULT MessageHandler(UINT const message, WPARAM const wparam, LPARAM const lparam) noexcept
    {
        return OnMessage(message, wparam, lparam);
    }

    // Method Description:
    // - This method is called when the window receives the WM_NCCREATE message.
    // Return Value:
    // - The value returned from the window proc.
    [[nodiscard]] LRESULT OnNcCreate(WPARAM wParam, LPARAM lParam)
    {
        OnCreate();
        return MessageHandler(WM_NCCREATE, wParam, lParam);
    }

protected:

    RECT GetWindowRect() const noexcept
    {
        RECT rc = { 0 };
        ::GetWindowRect(m_hwnd.get(), &rc);
        return rc;
    }

    HWND GetHandle() const noexcept
    {
        return m_hwnd.get();
    }

    float GetCurrentDpiScale() const noexcept
    {
        return WindowHelpers::GetCurrentDpiScale(m_hwnd.get());
    }

    wf::Size GetLogicalSize() const noexcept
    {
        return WindowHelpers::GetLogicalSize(GetPhysicalSize(), GetCurrentDpiScale());
    }

    // Gets the physical size of the client area of the HWND in _window
    SIZE GetPhysicalSize() const noexcept
    {
        return GetPhysicalSize(m_hwnd.get());
    }

protected:
    virtual LRESULT OnMessage(UINT const message, WPARAM const wparam, LPARAM const lparam) noexcept
    {
        return ::DefWindowProc(m_hwnd.get(), message, wparam, lparam);
    }

    virtual void OnCreate() noexcept = 0;

    wil::unique_hwnd m_hwnd;
};
