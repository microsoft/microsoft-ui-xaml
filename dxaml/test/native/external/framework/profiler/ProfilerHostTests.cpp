// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
#include <windows.h>
#include <cstdio>
#include <string>
#undef GetCurrentTime
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Microsoft.UI.Dispatching.h>
#include <winrt/Microsoft.UI.Xaml.Hosting.h>
#include <winrt/Microsoft.UI.Xaml.Controls.h>

int main(int argc, char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    try
    {
        winrt::init_apartment(winrt::apartment_type::single_threaded);
        const auto dispatcher = winrt::Microsoft::UI::Dispatching::DispatcherQueueController::CreateOnCurrentThread();
        auto manager = winrt::Microsoft::UI::Xaml::Hosting::WindowsXamlManager::InitializeForCurrentThread();
        std::puts("PASS: supported selfcontained WindowsXamlManager initialization");
        {
            winrt::Microsoft::UI::Xaml::Controls::Grid object;
            if (!object) return 1;
            object.Width(24);
        }
        std::puts("PASS: actual XAML object creation and teardown");
        wchar_t path[32768]{};
        if (!GetModuleFileNameW(GetModuleHandleW(L"Microsoft.UI.Xaml.dll"), path, ARRAYSIZE(path))) return 1;
        std::wprintf(L"LOADED_LOCAL_PRODUCT: %ls\n", path);
        std::puts("HOST_READY");
        std::fflush(stdout);
        if (argc == 2 && std::string(argv[1]) == "--verify-deployment") Sleep(10000);
        else if (argc != 1) return 1;
        manager.Close();
        std::puts("PASS: WindowsXamlManager teardown");
        return 0;
    }
    catch (const winrt::hresult_error& error)
    {
        std::printf("FAIL: HRESULT 0x%08lx\n", static_cast<unsigned long>(error.code().value));
        std::wprintf(L"%ls\n", error.message().c_str());
        return 1;
    }
}
