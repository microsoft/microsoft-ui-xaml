// WARNING: Please don't edit this file...

#pragma once
#include "winrt/Microsoft.UI.Composition.h"
#include "winrt/Microsoft.UI.Xaml.h"
#include "winrt/Microsoft.UI.Xaml.Controls.h"
#include "winrt/Simple.h"
namespace winrt::Simple::implementation
{
    template <typename D, typename... I>
    struct WINRT_IMPL_EMPTY_BASES MainPageBase_base : implements<D, Simple::MainPageBase, composable, composing, winrt::Microsoft::UI::Xaml::Controls::IPageOverrides, winrt::Microsoft::UI::Xaml::Controls::IControlOverrides, winrt::Microsoft::UI::Xaml::IFrameworkElementOverrides, winrt::Microsoft::UI::Xaml::IUIElementOverrides, I...>,
        impl::require<D, winrt::Microsoft::UI::Xaml::Controls::IPage, winrt::Microsoft::UI::Xaml::Controls::IUserControl, winrt::Microsoft::UI::Xaml::Controls::IControl, winrt::Microsoft::UI::Xaml::IFrameworkElement, winrt::Microsoft::UI::Xaml::IFrameworkElementFeature_ExperimentalApi, winrt::Microsoft::UI::Xaml::IUIElement, winrt::Microsoft::UI::Composition::IAnimationObject, winrt::Microsoft::UI::Composition::IVisualElement, winrt::Microsoft::UI::Composition::IVisualElement2, winrt::Microsoft::UI::Xaml::IDependencyObject>,
        protected impl::require<D, winrt::Microsoft::UI::Xaml::Controls::IControlProtected, winrt::Microsoft::UI::Xaml::IFrameworkElementProtected, winrt::Microsoft::UI::Xaml::IUIElementProtected>,
        impl::base<D, winrt::Microsoft::UI::Xaml::Controls::Page, winrt::Microsoft::UI::Xaml::Controls::UserControl, winrt::Microsoft::UI::Xaml::Controls::Control, winrt::Microsoft::UI::Xaml::FrameworkElement, winrt::Microsoft::UI::Xaml::UIElement, winrt::Microsoft::UI::Xaml::DependencyObject>,
        winrt::Microsoft::UI::Xaml::Controls::IPageOverridesT<D>, winrt::Microsoft::UI::Xaml::Controls::IControlOverridesT<D>, winrt::Microsoft::UI::Xaml::IFrameworkElementOverridesT<D>, winrt::Microsoft::UI::Xaml::IUIElementOverridesT<D>
    {
        using base_type = MainPageBase_base;
        using class_type = Simple::MainPageBase;
        using implements_type = typename MainPageBase_base::implements_type;
        using implements_type::implements_type;
        using composable_base = winrt::Microsoft::UI::Xaml::Controls::Page;
        friend impl::consume_t<D, winrt::Microsoft::UI::Xaml::Controls::IControlProtected>;
        friend impl::require_one<D, winrt::Microsoft::UI::Xaml::Controls::IControlProtected>;
        friend impl::consume_t<D, winrt::Microsoft::UI::Xaml::IFrameworkElementProtected>;
        friend impl::require_one<D, winrt::Microsoft::UI::Xaml::IFrameworkElementProtected>;
        friend impl::consume_t<D, winrt::Microsoft::UI::Xaml::IUIElementProtected>;
        friend impl::require_one<D, winrt::Microsoft::UI::Xaml::IUIElementProtected>;
        hstring GetRuntimeClassName() const
        {
            return L"Simple.MainPageBase";
        }
        MainPageBase_base()
        {
            impl::call_factory<winrt::Microsoft::UI::Xaml::Controls::Page, winrt::Microsoft::UI::Xaml::Controls::IPageFactory>([&](winrt::Microsoft::UI::Xaml::Controls::IPageFactory const& f) { [[maybe_unused]] auto winrt_impl_discarded = f.CreateInstance(*this, this->m_inner); });
        }

    protected:
        using dispatch = impl::dispatch_to_overridable<D, winrt::Microsoft::UI::Xaml::Controls::IPageOverrides, winrt::Microsoft::UI::Xaml::Controls::IControlOverrides, winrt::Microsoft::UI::Xaml::IFrameworkElementOverrides, winrt::Microsoft::UI::Xaml::IUIElementOverrides>;
        auto overridable() noexcept { return dispatch::overridable(static_cast<D&>(*this)); }
    };
}
namespace winrt::Simple::factory_implementation
{
    template <typename D, typename T, typename... I>
    struct WINRT_IMPL_EMPTY_BASES MainPageBaseT : implements<D, winrt::Windows::Foundation::IActivationFactory, winrt::Simple::IMainPageBaseFactory, I...>
    {
        using instance_type = Simple::MainPageBase;

        hstring GetRuntimeClassName() const
        {
            return L"Simple.MainPageBase";
        }
        auto CreateInstance(winrt::Windows::Foundation::IInspectable const& baseInterface, winrt::Windows::Foundation::IInspectable& innerInterface)
        {
            return impl::composable_factory<T>::template CreateInstance<winrt::Simple::MainPageBase>(baseInterface, innerInterface);
        }
        [[noreturn]] winrt::Windows::Foundation::IInspectable ActivateInstance() const
        {
            throw hresult_not_implemented();
        }
    };
}

#if defined(WINRT_FORCE_INCLUDE_MAINPAGEBASE_XAML_G_H) || __has_include("MainPageBase.xaml.g.h")

#include "MainPageBase.xaml.g.h"

#else

namespace winrt::Simple::implementation
{
    template <typename D, typename... I>
    using MainPageBaseT = MainPageBase_base<D, I...>;
}

#endif
