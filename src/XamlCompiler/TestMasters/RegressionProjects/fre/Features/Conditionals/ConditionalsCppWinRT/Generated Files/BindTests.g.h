// WARNING: Please don't edit this file...

#pragma once
#include "winrt/ConditionalsCppWinRT.h"
#include "winrt/Microsoft.UI.Composition.h"
#include "winrt/Microsoft.UI.Xaml.h"
#include "winrt/Microsoft.UI.Xaml.Controls.h"
#include "winrt/Microsoft.UI.Xaml.Data.h"
namespace winrt::ConditionalsCppWinRT::implementation
{
    template <typename D, typename... I>
    struct WINRT_IMPL_EMPTY_BASES BindTests_base : implements<D, ConditionalsCppWinRT::BindTests, winrt::Microsoft::UI::Xaml::Data::INotifyPropertyChanged, composing, winrt::Microsoft::UI::Xaml::Controls::IControlOverrides, winrt::Microsoft::UI::Xaml::IFrameworkElementOverrides, winrt::Microsoft::UI::Xaml::IUIElementOverrides, I...>,
        impl::require<D, winrt::Microsoft::UI::Xaml::Controls::IUserControl, winrt::Microsoft::UI::Xaml::Controls::IControl, winrt::Microsoft::UI::Xaml::IFrameworkElement, winrt::Microsoft::UI::Xaml::IFrameworkElementFeature_ExperimentalApi, winrt::Microsoft::UI::Xaml::IUIElement, winrt::Microsoft::UI::Composition::IAnimationObject, winrt::Microsoft::UI::Composition::IVisualElement, winrt::Microsoft::UI::Composition::IVisualElement2, winrt::Microsoft::UI::Xaml::IDependencyObject>,
        protected impl::require<D, winrt::Microsoft::UI::Xaml::Controls::IControlProtected, winrt::Microsoft::UI::Xaml::IFrameworkElementProtected, winrt::Microsoft::UI::Xaml::IUIElementProtected>,
        impl::base<D, winrt::Microsoft::UI::Xaml::Controls::UserControl, winrt::Microsoft::UI::Xaml::Controls::Control, winrt::Microsoft::UI::Xaml::FrameworkElement, winrt::Microsoft::UI::Xaml::UIElement, winrt::Microsoft::UI::Xaml::DependencyObject>,
        winrt::Microsoft::UI::Xaml::Controls::IControlOverridesT<D>, winrt::Microsoft::UI::Xaml::IFrameworkElementOverridesT<D>, winrt::Microsoft::UI::Xaml::IUIElementOverridesT<D>
    {
        using base_type = BindTests_base;
        using class_type = ConditionalsCppWinRT::BindTests;
        using implements_type = typename BindTests_base::implements_type;
        using implements_type::implements_type;
        using composable_base = winrt::Microsoft::UI::Xaml::Controls::UserControl;
        friend impl::consume_t<D, winrt::Microsoft::UI::Xaml::Controls::IControlProtected>;
        friend impl::require_one<D, winrt::Microsoft::UI::Xaml::Controls::IControlProtected>;
        friend impl::consume_t<D, winrt::Microsoft::UI::Xaml::IFrameworkElementProtected>;
        friend impl::require_one<D, winrt::Microsoft::UI::Xaml::IFrameworkElementProtected>;
        friend impl::consume_t<D, winrt::Microsoft::UI::Xaml::IUIElementProtected>;
        friend impl::require_one<D, winrt::Microsoft::UI::Xaml::IUIElementProtected>;
        hstring GetRuntimeClassName() const
        {
            return L"ConditionalsCppWinRT.BindTests";
        }
        BindTests_base()
        {
            impl::call_factory<winrt::Microsoft::UI::Xaml::Controls::UserControl, winrt::Microsoft::UI::Xaml::Controls::IUserControlFactory>([&](winrt::Microsoft::UI::Xaml::Controls::IUserControlFactory const& f) { [[maybe_unused]] auto winrt_impl_discarded = f.CreateInstance(*this, this->m_inner); });
        }
    };
}
namespace winrt::ConditionalsCppWinRT::factory_implementation
{
    template <typename D, typename T, typename... I>
    struct WINRT_IMPL_EMPTY_BASES BindTestsT : implements<D, winrt::Windows::Foundation::IActivationFactory, I...>
    {
        using instance_type = ConditionalsCppWinRT::BindTests;

        hstring GetRuntimeClassName() const
        {
            return L"ConditionalsCppWinRT.BindTests";
        }
        auto ActivateInstance() const
        {
            return make<T>();
        }
    };
}

#if defined(WINRT_FORCE_INCLUDE_BINDTESTS_XAML_G_H) || __has_include("BindTests.xaml.g.h")

#include "BindTests.xaml.g.h"

#else

namespace winrt::ConditionalsCppWinRT::implementation
{
    template <typename D, typename... I>
    using BindTestsT = BindTests_base<D, I...>;
}

#endif
