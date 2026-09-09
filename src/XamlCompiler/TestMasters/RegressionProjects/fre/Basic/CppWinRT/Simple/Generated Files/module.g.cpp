// WARNING: Please don't edit this file...

#include "pch.h"
#include "winrt/base.h"
void* winrt_make_Simple_BlankPage();
void* winrt_make_Simple_BlankPageBase();
void* winrt_make_Simple_CPPEventArgumentsTest();
void* winrt_make_Simple_FieldModifierTests();
void* winrt_make_Simple_MainPage();
void* winrt_make_Simple_MainPageBase();
void* winrt_make_Simple_PropBag();
void* winrt_make_Simple_XamlMetaDataProvider();

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

    if (requal(name, L"Simple.BlankPage"))
    {
        return winrt_make_Simple_BlankPage();
    }

    if (requal(name, L"Simple.BlankPageBase"))
    {
        return winrt_make_Simple_BlankPageBase();
    }

    if (requal(name, L"Simple.CPPEventArgumentsTest"))
    {
        return winrt_make_Simple_CPPEventArgumentsTest();
    }

    if (requal(name, L"Simple.FieldModifierTests"))
    {
        return winrt_make_Simple_FieldModifierTests();
    }

    if (requal(name, L"Simple.MainPage"))
    {
        return winrt_make_Simple_MainPage();
    }

    if (requal(name, L"Simple.MainPageBase"))
    {
        return winrt_make_Simple_MainPageBase();
    }

    if (requal(name, L"Simple.PropBag"))
    {
        return winrt_make_Simple_PropBag();
    }

    if (requal(name, L"Simple.XamlMetaDataProvider"))
    {
        return winrt_make_Simple_XamlMetaDataProvider();
    }

    return nullptr;
}

int32_t __stdcall WINRT_CanUnloadNow() noexcept
{
#ifdef _WRL_MODULE_H_
#ifdef _MSC_VER
#pragma warning(suppress: 4324) // structure was padded due to alignment specifier
#endif
    if (!::Microsoft::WRL::Module<::Microsoft::WRL::InProc>::GetModule().Terminate())
    {
        return 1;
    }
#endif

    return winrt_can_unload_now() ? 0 : 1;
}

int32_t __stdcall WINRT_GetActivationFactory(void* classId, void** factory) noexcept try
{
    std::wstring_view const name{ *reinterpret_cast<winrt::hstring*>(&classId) };
    *factory = winrt_get_activation_factory(name);

    if (*factory)
    {
        return 0;
    }

#ifdef _WRL_MODULE_H_
#pragma warning(suppress: 4324) // structure was padded due to alignment specifier
    return ::Microsoft::WRL::Module<::Microsoft::WRL::InProc>::GetModule().GetActivationFactory(static_cast<HSTRING>(classId), reinterpret_cast<::IActivationFactory**>(factory));
#else
    return winrt::hresult_class_not_available(name).to_abi();
#endif
}
catch (...) { return winrt::to_hresult(); }
