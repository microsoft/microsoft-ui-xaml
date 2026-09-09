// WARNING: Please don't edit this file...

#pragma once
#include "winrt/BindTestbed.h"
#include "winrt/Microsoft.UI.Composition.h"
#include "winrt/Microsoft.UI.Xaml.h"
#include "winrt/Microsoft.UI.Xaml.Controls.h"
namespace winrt::BindTestbed::implementation
{
    template <typename D, typename... I>
    struct WINRT_IMPL_EMPTY_BASES MyUserControl1_base : implements<D, BindTestbed::MyUserControl1, composing, winrt::Microsoft::UI::Xaml::Controls::IControlOverrides, winrt::Microsoft::UI::Xaml::IFrameworkElementOverrides, winrt::Microsoft::UI::Xaml::IUIElementOverrides, I...>,
        impl::require<D, winrt::Microsoft::UI::Xaml::Controls::IUserControl, winrt::Microsoft::UI::Xaml::Controls::IControl, winrt::Microsoft::UI::Xaml::IFrameworkElement, winrt::Microsoft::UI::Xaml::IFrameworkElementFeature_ExperimentalApi, winrt::Microsoft::UI::Xaml::IUIElement, winrt::Microsoft::UI::Composition::IAnimationObject, winrt::Microsoft::UI::Composition::IVisualElement, winrt::Microsoft::UI::Composition::IVisualElement2, winrt::Microsoft::UI::Xaml::IDependencyObject>,
        protected impl::require<D, winrt::Microsoft::UI::Xaml::Controls::IControlProtected, winrt::Microsoft::UI::Xaml::IFrameworkElementProtected, winrt::Microsoft::UI::Xaml::IUIElementProtected>,
        impl::base<D, winrt::Microsoft::UI::Xaml::Controls::UserControl, winrt::Microsoft::UI::Xaml::Controls::Control, winrt::Microsoft::UI::Xaml::FrameworkElement, winrt::Microsoft::UI::Xaml::UIElement, winrt::Microsoft::UI::Xaml::DependencyObject>,
        winrt::Microsoft::UI::Xaml::Controls::IControlOverridesT<D>, winrt::Microsoft::UI::Xaml::IFrameworkElementOverridesT<D>, winrt::Microsoft::UI::Xaml::IUIElementOverridesT<D>
    {
        using base_type = MyUserControl1_base;
        using class_type = BindTestbed::MyUserControl1;
        using implements_type = typename MyUserControl1_base::implements_type;
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
            return L"BindTestbed.MyUserControl1";
        }
        MyUserControl1_base()
        {
            impl::call_factory<winrt::Microsoft::UI::Xaml::Controls::UserControl, winrt::Microsoft::UI::Xaml::Controls::IUserControlFactory>([&](winrt::Microsoft::UI::Xaml::Controls::IUserControlFactory const& f) { [[maybe_unused]] auto winrt_impl_discarded = f.CreateInstance(*this, this->m_inner); });
        }
    };
}
namespace winrt::BindTestbed::factory_implementation
{
    template <typename D, typename T, typename... I>
    struct WINRT_IMPL_EMPTY_BASES MyUserControl1T : implements<D, winrt::Windows::Foundation::IActivationFactory, I...>
    {
        using instance_type = BindTestbed::MyUserControl1;

        hstring GetRuntimeClassName() const
        {
            return L"BindTestbed.MyUserControl1";
        }
        auto ActivateInstance() const
        {
            return make<T>();
        }
    };
}

#if defined(WINRT_FORCE_INCLUDE_MYUSERCONTROL1_XAML_G_H) || __has_include("MyUserControl1.xaml.g.h")

#include "MyUserControl1.xaml.g.h"

#else

namespace winrt::BindTestbed::implementation
{
    template <typename D, typename... I>
    using MyUserControl1T = MyUserControl1_base<D, I...>;
}

#endif
