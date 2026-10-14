// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

namespace XamlUtil
{
    inline bool IsXamlLoaded()
    {
        return (::GetModuleHandle(L"Microsoft.UI.Xaml.dll") != nullptr);
    }

    inline bool IsXamlRunningInProcess()
    {
        if (IsXamlLoaded())
        {
            auto app = winrt::Microsoft::UI::Xaml::Application::Current();
            return (app != nullptr);
        }
        return false;
    }

    inline bool IsXamlRunningOnThread()
    {
        return IsXamlLoaded() &&
            winrt::Microsoft::UI::Xaml::Hosting::WindowsXamlManager::GetForCurrentThread() != nullptr;
    }

}