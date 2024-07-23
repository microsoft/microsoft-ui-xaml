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
        if (IsXamlLoaded())
        {
            try
            {
                // Creating a button will throw a RPC_E_WRONG_THREAD HRESULT if XAML's not running on the thread.
                auto b = winrt::Microsoft::UI::Xaml::Controls::Button();
                return true;
            }
            catch (winrt::hresult_wrong_thread) {}
        }
        return false;
    }

}