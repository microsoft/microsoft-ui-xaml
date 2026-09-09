// WARNING: Please don't edit this file...

#pragma once
#include "winrt/LinkedMDSubControlsCppWinRT.h"
#include "winrt/Microsoft.UI.Composition.h"
#include "winrt/Microsoft.UI.Xaml.h"
#include "winrt/Microsoft.UI.Xaml.Controls.h"
namespace winrt::LinkedMDSubControlsCppWinRT::implementation
{
    template <typename D, typename... I>
    struct WINRT_IMPL_EMPTY_BASES S_base : implements<D, LinkedMDSubControlsCppWinRT::S, composing, winrt::Microsoft::UI::Xaml::Controls::IPageOverrides, winrt::Microsoft::UI::Xaml::Controls::IControlOverrides, winrt::Microsoft::UI::Xaml::IFrameworkElementOverrides, winrt::Microsoft::UI::Xaml::IUIElementOverrides, I...>,
        impl::require<D, winrt::Microsoft::UI::Xaml::Controls::IPage, winrt::Microsoft::UI::Xaml::Controls::IUserControl, winrt::Microsoft::UI::Xaml::Controls::IControl, winrt::Microsoft::UI::Xaml::Controls::IControlProtected, winrt::Microsoft::UI::Xaml::IFrameworkElement, winrt::Microsoft::UI::Xaml::IFrameworkElementFeature_ExperimentalApi, winrt::Microsoft::UI::Xaml::IFrameworkElementProtected, winrt::Microsoft::UI::Xaml::IUIElement, winrt::Microsoft::UI::Xaml::IUIElementProtected, winrt::Microsoft::UI::Composition::IAnimationObject, winrt::Microsoft::UI::Composition::IVisualElement, winrt::Microsoft::UI::Composition::IVisualElement2, winrt::Microsoft::UI::Xaml::IDependencyObject>,
        impl::base<D, winrt::Microsoft::UI::Xaml::Controls::Page, winrt::Microsoft::UI::Xaml::Controls::UserControl, winrt::Microsoft::UI::Xaml::Controls::Control, winrt::Microsoft::UI::Xaml::FrameworkElement, winrt::Microsoft::UI::Xaml::UIElement, winrt::Microsoft::UI::Xaml::DependencyObject>,
        winrt::Microsoft::UI::Xaml::Controls::IPageOverridesT<D>, winrt::Microsoft::UI::Xaml::Controls::IControlOverridesT<D>, winrt::Microsoft::UI::Xaml::IFrameworkElementOverridesT<D>, winrt::Microsoft::UI::Xaml::IUIElementOverridesT<D>
    {
        using base_type = S_base;
        using class_type = LinkedMDSubControlsCppWinRT::S;
        using implements_type = typename S_base::implements_type;
        using implements_type::implements_type;
        using composable_base = winrt::Microsoft::UI::Xaml::Controls::Page;
        hstring GetRuntimeClassName() const
        {
            return L"LinkedMDSubControlsCppWinRT.S";
        }
        S_base()
        {
            impl::call_factory<winrt::Microsoft::UI::Xaml::Controls::Page, winrt::Microsoft::UI::Xaml::Controls::IPageFactory>([&](winrt::Microsoft::UI::Xaml::Controls::IPageFactory const& f) { [[maybe_unused]] auto winrt_impl_discarded = f.CreateInstance(*this, this->m_inner); });
        }
    };
}
namespace winrt::LinkedMDSubControlsCppWinRT::factory_implementation
{
    template <typename D, typename T, typename... I>
    struct WINRT_IMPL_EMPTY_BASES ST : implements<D, winrt::Windows::Foundation::IActivationFactory, I...>
    {
        using instance_type = LinkedMDSubControlsCppWinRT::S;

        hstring GetRuntimeClassName() const
        {
            return L"LinkedMDSubControlsCppWinRT.S";
        }
        auto ActivateInstance() const
        {
            return make<T>();
        }
    };
}

#if defined(WINRT_FORCE_INCLUDE_S_XAML_G_H) || __has_include("S.xaml.g.h")

#include "S.xaml.g.h"

#else

namespace winrt::LinkedMDSubControlsCppWinRT::implementation
{
    template <typename D, typename... I>
    using ST = S_base<D, I...>;
}

#endif
