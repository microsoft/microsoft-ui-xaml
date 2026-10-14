// WARNING: Please don't edit this file...

#pragma once
#include "winrt/CppWinRTComponent.h"
#include "winrt/Microsoft.UI.Composition.h"
#include "winrt/Microsoft.UI.Xaml.h"
#include "winrt/Microsoft.UI.Xaml.Controls.h"
namespace winrt::CppWinRTComponent::implementation
{
    template <typename D, typename... I>
    struct WINRT_IMPL_EMPTY_BASES CppWinRTUserControlFromWinRTComponent_base : implements<D, CppWinRTComponent::CppWinRTUserControlFromWinRTComponent, composing, winrt::Microsoft::UI::Xaml::Controls::IControlOverrides, winrt::Microsoft::UI::Xaml::IFrameworkElementOverrides, winrt::Microsoft::UI::Xaml::IUIElementOverrides, I...>,
        impl::require<D, winrt::Microsoft::UI::Xaml::Controls::IUserControl, winrt::Microsoft::UI::Xaml::Controls::IControl, winrt::Microsoft::UI::Xaml::IFrameworkElement, winrt::Microsoft::UI::Xaml::IFrameworkElementFeature_ExperimentalApi, winrt::Microsoft::UI::Xaml::IUIElement, winrt::Microsoft::UI::Composition::IAnimationObject, winrt::Microsoft::UI::Composition::IVisualElement, winrt::Microsoft::UI::Composition::IVisualElement2, winrt::Microsoft::UI::Xaml::IDependencyObject>,
        protected impl::require<D, winrt::Microsoft::UI::Xaml::Controls::IControlProtected, winrt::Microsoft::UI::Xaml::IFrameworkElementProtected, winrt::Microsoft::UI::Xaml::IUIElementProtected>,
        impl::base<D, winrt::Microsoft::UI::Xaml::Controls::UserControl, winrt::Microsoft::UI::Xaml::Controls::Control, winrt::Microsoft::UI::Xaml::FrameworkElement, winrt::Microsoft::UI::Xaml::UIElement, winrt::Microsoft::UI::Xaml::DependencyObject>,
        winrt::Microsoft::UI::Xaml::Controls::IControlOverridesT<D>, winrt::Microsoft::UI::Xaml::IFrameworkElementOverridesT<D>, winrt::Microsoft::UI::Xaml::IUIElementOverridesT<D>
    {
        using base_type = CppWinRTUserControlFromWinRTComponent_base;
        using class_type = CppWinRTComponent::CppWinRTUserControlFromWinRTComponent;
        using implements_type = typename CppWinRTUserControlFromWinRTComponent_base::implements_type;
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
            return L"CppWinRTComponent.CppWinRTUserControlFromWinRTComponent";
        }
        CppWinRTUserControlFromWinRTComponent_base()
        {
            impl::call_factory<winrt::Microsoft::UI::Xaml::Controls::UserControl, winrt::Microsoft::UI::Xaml::Controls::IUserControlFactory>([&](winrt::Microsoft::UI::Xaml::Controls::IUserControlFactory const& f) { [[maybe_unused]] auto winrt_impl_discarded = f.CreateInstance(*this, this->m_inner); });
        }
    };
}
namespace winrt::CppWinRTComponent::factory_implementation
{
    template <typename D, typename T, typename... I>
    struct WINRT_IMPL_EMPTY_BASES CppWinRTUserControlFromWinRTComponentT : implements<D, winrt::Windows::Foundation::IActivationFactory, I...>
    {
        using instance_type = CppWinRTComponent::CppWinRTUserControlFromWinRTComponent;

        hstring GetRuntimeClassName() const
        {
            return L"CppWinRTComponent.CppWinRTUserControlFromWinRTComponent";
        }
        auto ActivateInstance() const
        {
            return make<T>();
        }
    };
}

#if defined(WINRT_FORCE_INCLUDE_CPPWINRTUSERCONTROLFROMWINRTCOMPONENT_XAML_G_H) || __has_include("CppWinRTUserControlFromWinRTComponent.xaml.g.h")

#include "CppWinRTUserControlFromWinRTComponent.xaml.g.h"

#else

namespace winrt::CppWinRTComponent::implementation
{
    template <typename D, typename... I>
    using CppWinRTUserControlFromWinRTComponentT = CppWinRTUserControlFromWinRTComponent_base<D, I...>;
}

#endif
