// WARNING: Please don't edit this file...

#pragma once
#include "winrt/Microsoft.UI.Composition.h"
#include "winrt/Microsoft.UI.Xaml.h"
#include "winrt/Microsoft.UI.Xaml.Controls.h"
#include "winrt/Simple.h"
namespace winrt::Simple::implementation
{
    template <typename D, typename B, typename... I>
    struct WINRT_IMPL_EMPTY_BASES BlankPage_base : implements<D, Simple::BlankPage, B, no_module_lock, I...>,
        winrt::Microsoft::UI::Xaml::Controls::IPageOverridesT<D>, winrt::Microsoft::UI::Xaml::Controls::IControlOverridesT<D>, winrt::Microsoft::UI::Xaml::IFrameworkElementOverridesT<D>, winrt::Microsoft::UI::Xaml::IUIElementOverridesT<D>
    {
        using base_type = BlankPage_base;
        using class_type = Simple::BlankPage;
        using implements_type = typename BlankPage_base::implements_type;
        using implements_type::implements_type;
        using composable_base = B;
        hstring GetRuntimeClassName() const
        {
            return L"Simple.BlankPage";
        }
    };
}
namespace winrt::Simple::factory_implementation
{
    template <typename D, typename T, typename... I>
    struct WINRT_IMPL_EMPTY_BASES BlankPageT : implements<D, winrt::Windows::Foundation::IActivationFactory, I...>
    {
        using instance_type = Simple::BlankPage;

        hstring GetRuntimeClassName() const
        {
            return L"Simple.BlankPage";
        }
        auto ActivateInstance() const
        {
            return make<T>();
        }
    };
}

#if defined(WINRT_FORCE_INCLUDE_BLANKPAGE_XAML_G_H) || __has_include("BlankPage.xaml.g.h")

#include "BlankPage.xaml.g.h"

#else

namespace winrt::Simple::implementation
{
    template <typename D, typename... I>
    using BlankPageT = BlankPage_base<D, I...>;
}

#endif
