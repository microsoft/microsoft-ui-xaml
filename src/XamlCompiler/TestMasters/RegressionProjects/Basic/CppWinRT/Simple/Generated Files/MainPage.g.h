// WARNING: Please don't edit this file...

#pragma once
#include "winrt/Simple.h"
#include "winrt/Windows.UI.Composition.h"
#include "winrt/Windows.UI.Xaml.h"
#include "winrt/Windows.UI.Xaml.Controls.h"
namespace winrt::Simple::implementation
{
    template <typename D, typename B, typename... I>
    struct __declspec(empty_bases) MainPage_base : implements<D, Simple::MainPage, B, no_module_lock, I...>,
        ::Windows::UI::Xaml::Controls::IPageOverridesT<D>, ::Windows::UI::Xaml::Controls::IControlOverridesT<D>, ::Windows::UI::Xaml::Controls::IControlOverrides6T<D>, ::Windows::UI::Xaml::IFrameworkElementOverridesT<D>, ::Windows::UI::Xaml::IFrameworkElementOverrides2T<D>, ::Windows::UI::Xaml::IUIElementOverridesT<D>, ::Windows::UI::Xaml::IUIElementOverrides7T<D>, ::Windows::UI::Xaml::IUIElementOverrides8T<D>, ::Windows::UI::Xaml::IUIElementOverrides9T<D>
    {
        using base_type = MainPage_base;
        using class_type = Simple::MainPage;
        using implements_type = typename MainPage_base::implements_type;
        using implements_type::implements_type;
        
        hstring GetRuntimeClassName() const
        {
            return L"Simple.MainPage";
        }
    };
}
namespace winrt::Simple::factory_implementation
{
    template <typename D, typename T, typename... I>
    struct __declspec(empty_bases) MainPageT : implements<D, ::Windows::Foundation::IActivationFactory, I...>
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
