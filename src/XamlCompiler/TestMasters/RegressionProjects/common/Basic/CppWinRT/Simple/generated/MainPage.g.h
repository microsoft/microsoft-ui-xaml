// WARNING: Please don't edit this file...

#pragma once
#include "winrt/Microsoft.UI.Composition.h"
#include "winrt/Microsoft.UI.Xaml.h"
#include "winrt/Microsoft.UI.Xaml.Controls.h"
#include "winrt/Simple.h"
namespace winrt::Simple::implementation
{
    template <typename D, typename B, typename... I>
    struct WINRT_IMPL_EMPTY_BASES MainPage_base : implements<D, Simple::MainPage, B, no_module_lock, I...>,
        winrt::Microsoft::UI::Xaml::Controls::IPageOverridesT<D>, winrt::Microsoft::UI::Xaml::Controls::IControlOverridesT<D>, winrt::Microsoft::UI::Xaml::IFrameworkElementOverridesT<D>, winrt::Microsoft::UI::Xaml::IUIElementOverridesT<D>
    {
        using base_type = MainPage_base;
        using class_type = Simple::MainPage;
        using implements_type = typename MainPage_base::implements_type;
        using implements_type::implements_type;
        using composable_base = B;
        hstring GetRuntimeClassName() const
        {
            return L"Simple.MainPage";
        }
    };
}
namespace winrt::Simple::factory_implementation
{
    template <typename D, typename T, typename... I>
    struct WINRT_IMPL_EMPTY_BASES MainPageT : implements<D, winrt::Windows::Foundation::IActivationFactory, I...>
    {
        using instance_type = Simple::MainPage;

        hstring GetRuntimeClassName() const
        {
            return L"Simple.MainPage";
        }
        auto ActivateInstance() const
        {
            return make<T>();
        }
    };
}

#if defined(WINRT_FORCE_INCLUDE_MAINPAGE_XAML_G_H) || __has_include("MainPage.xaml.g.h")

#include "MainPage.xaml.g.h"

#else

namespace winrt::Simple::implementation
{
    template <typename D, typename... I>
    using MainPageT = MainPage_base<D, I...>;
}

#endif
