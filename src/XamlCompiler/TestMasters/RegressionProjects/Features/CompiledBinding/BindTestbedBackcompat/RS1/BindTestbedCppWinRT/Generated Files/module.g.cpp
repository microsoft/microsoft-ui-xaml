// WARNING: Please don't edit this file...

#include "pch.h"
#include "BasicTests.h"
#include "CastingModel.h"
#include "CastingTests.h"
#include "DetectLeaksPage.h"
#include "EventTests.h"
#include "ExtraInfo.h"
#include "FunctionTests.h"
#include "ListAndTemplateTests.h"
#include "LoadAndCreateFromStringTests.h"
#include "LonelyStaticBinding.h"
#include "MainModel.h"
#include "MainPage.h"
#include "MyInfo.h"
#include "MyItem.h"
#include "MyUserControl1.h"
#include "PhasingTests.h"
#include "Templates.h"
#include "TestsPage2.h"
#include "TwoWayTests.h"
#include "XamlMetaDataProvider.h"
#include "subfolder.SubDictionary.h"

int32_t WINRT_CALL WINRT_CanUnloadNow() noexcept
{
#ifdef _WRL_MODULE_H_
    if (!::Microsoft::WRL::Module<::Microsoft::WRL::InProc>::GetModule().Terminate())
    {
        return 1; // S_FALSE
    }
#endif

    if (winrt::get_module_lock())
    {
        return 1; // S_FALSE
    }

    winrt::clear_factory_cache();
    return 0; // S_OK
}

int32_t WINRT_CALL WINRT_GetActivationFactory(void* classId, void** factory) noexcept
{
    try
    {
        *factory = nullptr;
        uint32_t length{};
        wchar_t const* const buffer = WINRT_WindowsGetStringRawBuffer(classId, &length);
        std::wstring_view const name{ buffer, length };

        auto requal = [](std::wstring_view const& left, std::wstring_view const& right) noexcept
        {
            return std::equal(left.rbegin(), left.rend(), right.rbegin(), right.rend());
        };

        if (requal(name, L"BindTestbed.BasicTests"))
        {
            *factory = winrt::detach_abi(winrt::make<winrt::BindTestbed::factory_implementation::BasicTests>());
            return 0;
        }

        if (requal(name, L"BindTestbed.CastingModel"))
        {
            *factory = winrt::detach_abi(winrt::make<winrt::BindTestbed::factory_implementation::CastingModel>());
            return 0;
        }

        if (requal(name, L"BindTestbed.CastingTests"))
        {
            *factory = winrt::detach_abi(winrt::make<winrt::BindTestbed::factory_implementation::CastingTests>());
            return 0;
        }

        if (requal(name, L"BindTestbed.DetectLeaksPage"))
        {
            *factory = winrt::detach_abi(winrt::make<winrt::BindTestbed::factory_implementation::DetectLeaksPage>());
            return 0;
        }

        if (requal(name, L"BindTestbed.EventTests"))
        {
            *factory = winrt::detach_abi(winrt::make<winrt::BindTestbed::factory_implementation::EventTests>());
            return 0;
        }

        if (requal(name, L"BindTestbed.ExtraInfo"))
        {
            *factory = winrt::detach_abi(winrt::make<winrt::BindTestbed::factory_implementation::ExtraInfo>());
            return 0;
        }

        if (requal(name, L"BindTestbed.FunctionTests"))
        {
            *factory = winrt::detach_abi(winrt::make<winrt::BindTestbed::factory_implementation::FunctionTests>());
            return 0;
        }

        if (requal(name, L"BindTestbed.ListAndTemplateTests"))
        {
            *factory = winrt::detach_abi(winrt::make<winrt::BindTestbed::factory_implementation::ListAndTemplateTests>());
            return 0;
        }

        if (requal(name, L"BindTestbed.LoadAndCreateFromStringTests"))
        {
            *factory = winrt::detach_abi(winrt::make<winrt::BindTestbed::factory_implementation::LoadAndCreateFromStringTests>());
            return 0;
        }

        if (requal(name, L"BindTestbed.LonelyStaticBinding"))
        {
            *factory = winrt::detach_abi(winrt::make<winrt::BindTestbed::factory_implementation::LonelyStaticBinding>());
            return 0;
        }

        if (requal(name, L"BindTestbed.MainModel"))
        {
            *factory = winrt::detach_abi(winrt::make<winrt::BindTestbed::factory_implementation::MainModel>());
            return 0;
        }

        if (requal(name, L"BindTestbed.MainPage"))
        {
            *factory = winrt::detach_abi(winrt::make<winrt::BindTestbed::factory_implementation::MainPage>());
            return 0;
        }

        if (requal(name, L"BindTestbed.MyInfo"))
        {
            *factory = winrt::detach_abi(winrt::make<winrt::BindTestbed::factory_implementation::MyInfo>());
            return 0;
        }

        if (requal(name, L"BindTestbed.MyItem"))
        {
            *factory = winrt::detach_abi(winrt::make<winrt::BindTestbed::factory_implementation::MyItem>());
            return 0;
        }

        if (requal(name, L"BindTestbed.MyUserControl1"))
        {
            *factory = winrt::detach_abi(winrt::make<winrt::BindTestbed::factory_implementation::MyUserControl1>());
            return 0;
        }

        if (requal(name, L"BindTestbed.PhasingTests"))
        {
            *factory = winrt::detach_abi(winrt::make<winrt::BindTestbed::factory_implementation::PhasingTests>());
            return 0;
        }

        if (requal(name, L"BindTestbed.Templates"))
        {
            *factory = winrt::detach_abi(winrt::make<winrt::BindTestbed::factory_implementation::Templates>());
            return 0;
        }

        if (requal(name, L"BindTestbed.TestsPage2"))
        {
            *factory = winrt::detach_abi(winrt::make<winrt::BindTestbed::factory_implementation::TestsPage2>());
            return 0;
        }

        if (requal(name, L"BindTestbed.TwoWayTests"))
        {
            *factory = winrt::detach_abi(winrt::make<winrt::BindTestbed::factory_implementation::TwoWayTests>());
            return 0;
        }

        if (requal(name, L"BindTestbed.XamlMetaDataProvider"))
        {
            *factory = winrt::detach_abi(winrt::make<winrt::BindTestbed::factory_implementation::XamlMetaDataProvider>());
            return 0;
        }

        if (requal(name, L"BindTestbed.subfolder.SubDictionary"))
        {
            *factory = winrt::detach_abi(winrt::make<winrt::BindTestbed::subfolder::factory_implementation::SubDictionary>());
            return 0;
        }

#ifdef _WRL_MODULE_H_
        return ::Microsoft::WRL::Module<::Microsoft::WRL::InProc>::GetModule().GetActivationFactory(static_cast<HSTRING>(classId), reinterpret_cast<::IActivationFactory**>(factory));
#else
        return winrt::hresult_class_not_available(name).to_abi();
#endif
    }
    catch (...) { return winrt::to_hresult(); }
}
