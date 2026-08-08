// WARNING: Please don't edit this file...

#include "pch.h"
#include "winrt/base.h"
void* winrt_make_BindTestbed_BasicTests();
void* winrt_make_BindTestbed_CastingModel();
void* winrt_make_BindTestbed_CastingTests();
void* winrt_make_BindTestbed_DetectLeaksPage();
void* winrt_make_BindTestbed_EventTests();
void* winrt_make_BindTestbed_ExtraInfo();
void* winrt_make_BindTestbed_FunctionTests();
void* winrt_make_BindTestbed_INotifyDataErrorInfoTests();
void* winrt_make_BindTestbed_ListAndTemplateTests();
void* winrt_make_BindTestbed_LoadAndCreateFromStringTests();
void* winrt_make_BindTestbed_LonelyStaticBinding();
void* winrt_make_BindTestbed_MainModel();
void* winrt_make_BindTestbed_MainPage();
void* winrt_make_BindTestbed_MyInfo();
void* winrt_make_BindTestbed_MyItem();
void* winrt_make_BindTestbed_MyUserControl1();
void* winrt_make_BindTestbed_NullableTests();
void* winrt_make_BindTestbed_PhasingTests();
void* winrt_make_BindTestbed_Templates();
void* winrt_make_BindTestbed_TestsPage2();
void* winrt_make_BindTestbed_TwoWayTests();
void* winrt_make_BindTestbed_XamlMetaDataProvider();
void* winrt_make_BindTestbed_subfolder_SubDictionary();

bool __stdcall winrt_can_unload_now() noexcept
{
    if (winrt::get_module_lock())
    {
        return false;
    }

    winrt::clear_factory_cache();
    return true;
}

void* __stdcall winrt_get_activation_factory([[maybe_unused]] std::wstring_view const& name)
{
    auto requal = [](std::wstring_view const& left, std::wstring_view const& right) noexcept
    {
        return std::equal(left.rbegin(), left.rend(), right.rbegin(), right.rend());
    };

    if (requal(name, L"BindTestbed.BasicTests"))
    {
        return winrt_make_BindTestbed_BasicTests();
    }

    if (requal(name, L"BindTestbed.CastingModel"))
    {
        return winrt_make_BindTestbed_CastingModel();
    }

    if (requal(name, L"BindTestbed.CastingTests"))
    {
        return winrt_make_BindTestbed_CastingTests();
    }

    if (requal(name, L"BindTestbed.DetectLeaksPage"))
    {
        return winrt_make_BindTestbed_DetectLeaksPage();
    }

    if (requal(name, L"BindTestbed.EventTests"))
    {
        return winrt_make_BindTestbed_EventTests();
    }

    if (requal(name, L"BindTestbed.ExtraInfo"))
    {
        return winrt_make_BindTestbed_ExtraInfo();
    }

    if (requal(name, L"BindTestbed.FunctionTests"))
    {
        return winrt_make_BindTestbed_FunctionTests();
    }

    if (requal(name, L"BindTestbed.INotifyDataErrorInfoTests"))
    {
        return winrt_make_BindTestbed_INotifyDataErrorInfoTests();
    }

    if (requal(name, L"BindTestbed.ListAndTemplateTests"))
    {
        return winrt_make_BindTestbed_ListAndTemplateTests();
    }

    if (requal(name, L"BindTestbed.LoadAndCreateFromStringTests"))
    {
        return winrt_make_BindTestbed_LoadAndCreateFromStringTests();
    }

    if (requal(name, L"BindTestbed.LonelyStaticBinding"))
    {
        return winrt_make_BindTestbed_LonelyStaticBinding();
    }

    if (requal(name, L"BindTestbed.MainModel"))
    {
        return winrt_make_BindTestbed_MainModel();
    }

    if (requal(name, L"BindTestbed.MainPage"))
    {
        return winrt_make_BindTestbed_MainPage();
    }

    if (requal(name, L"BindTestbed.MyInfo"))
    {
        return winrt_make_BindTestbed_MyInfo();
    }

    if (requal(name, L"BindTestbed.MyItem"))
    {
        return winrt_make_BindTestbed_MyItem();
    }

    if (requal(name, L"BindTestbed.MyUserControl1"))
    {
        return winrt_make_BindTestbed_MyUserControl1();
    }

    if (requal(name, L"BindTestbed.NullableTests"))
    {
        return winrt_make_BindTestbed_NullableTests();
    }

    if (requal(name, L"BindTestbed.PhasingTests"))
    {
        return winrt_make_BindTestbed_PhasingTests();
    }

    if (requal(name, L"BindTestbed.Templates"))
    {
        return winrt_make_BindTestbed_Templates();
    }

    if (requal(name, L"BindTestbed.TestsPage2"))
    {
        return winrt_make_BindTestbed_TestsPage2();
    }

    if (requal(name, L"BindTestbed.TwoWayTests"))
    {
        return winrt_make_BindTestbed_TwoWayTests();
    }

    if (requal(name, L"BindTestbed.XamlMetaDataProvider"))
    {
        return winrt_make_BindTestbed_XamlMetaDataProvider();
    }

    if (requal(name, L"BindTestbed.subfolder.SubDictionary"))
    {
        return winrt_make_BindTestbed_subfolder_SubDictionary();
    }

    return nullptr;
}

int32_t __stdcall WINRT_CanUnloadNow() noexcept
{
#ifdef _WRL_MODULE_H_
    if (!::Microsoft::WRL::Module<::Microsoft::WRL::InProc>::GetModule().Terminate())
    {
        return 1;
    }
#endif

    return winrt_can_unload_now() ? 0 : 1;
}

int32_t __stdcall WINRT_GetActivationFactory(void* classId, void** factory) noexcept try
{
    uint32_t length{};
    wchar_t const* const buffer = WINRT_WindowsGetStringRawBuffer(classId, &length);
    std::wstring_view const name{ buffer, length };
    *factory = winrt_get_activation_factory(name);

    if (*factory)
    {
        return 0;
    }

#ifdef _WRL_MODULE_H_
    return ::Microsoft::WRL::Module<::Microsoft::WRL::InProc>::GetModule().GetActivationFactory(static_cast<HSTRING>(classId), reinterpret_cast<::IActivationFactory**>(factory));
#else
    return winrt::hresult_class_not_available(name).to_abi();
#endif
}
catch (...) { return winrt::to_hresult(); }
