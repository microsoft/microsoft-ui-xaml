// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "resource.h"
#include "DesktopWindow.h"

struct ButtonCommand
{
    const wchar_t* name{ nullptr };
    std::function<void(HWND)> func;
    HWND hwnd{ NULL };
    int id{ -1 };
};

class MainWindow : public DesktopWindowT<MainWindow>
{
public:
    MainWindow(HINSTANCE hInstance, int nCmdShow, HWND parentHWnd = nullptr) noexcept;
    ~MainWindow();
    [[nodiscard]] LRESULT MessageHandler(UINT const message, WPARAM const wParam, LPARAM const lParam) noexcept;

private:
    void InitInstance(HINSTANCE hInstance, int nCmdShow, HWND parentHWnd);

    bool OnCreate(HWND, LPCREATESTRUCT);
    void OnCommand(HWND, int id, HWND hwndCtl, UINT codeNotify);
    void OnDestroy(HWND hwnd);
    void OnResize(HWND, UINT state, int cx, int cy);
    void OnPaint(HWND);
    void OnTimer(HWND, UINT);

    static DWORD WINAPI CreateNewWindowOnNewThreadProc(_In_ LPVOID parameter);

    void CreateButton(const wchar_t* name, DWORD id, std::function<void(HWND)> func, int x, int y, int w, int h);
    std::vector<ButtonCommand> m_buttons;

    winrt::Microsoft::UI::Xaml::Hosting::WindowsXamlManager m_windowsXamlManager{ nullptr };

    winrt::event_token m_dispatcherQueueShutdownStartingToken;

    HWND m_label{ 0 };

    static INT_PTR CALLBACK DialogBoxProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

public:

    static [[nodiscard]] int APIENTRY Run(_In_ HINSTANCE hInstance, _In_ int nCmdShow);
};
