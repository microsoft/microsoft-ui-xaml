// WARNING: Please don't edit this file...

#pragma once
#include "winrt/Simple.h"
#include "winrt/Windows.UI.Composition.h"
#include "winrt/Windows.UI.Xaml.h"
#include "winrt/Windows.UI.Xaml.Controls.h"
namespace winrt::Simple::implementation
{
    template <typename D, typename... I>
    struct __declspec(empty_bases) MainPageBase_base : implements<D, Simple::MainPageBase, composable, composing, ::Windows::UI::Xaml::Controls::IPageOverrides, ::Windows::UI::Xaml::Controls::IControlOverrides, ::Windows::UI::Xaml::Controls::IControlOverrides6, ::Windows::UI::Xaml::IFrameworkElementOverrides, ::Windows::UI::Xaml::IFrameworkElementOverrides2, ::Windows::UI::Xaml::IUIElementOverrides, ::Windows::UI::Xaml::IUIElementOverrides7, ::Windows::UI::Xaml::IUIElementOverrides8, ::Windows::UI::Xaml::IUIElementOverrides9, I...>,
        impl::require<D, ::Windows::UI::Xaml::Controls::IPage, ::Windows::UI::Xaml::Controls::IUserControl, ::Windows::UI::Xaml::Controls::IControl, ::Windows::UI::Xaml::Controls::IControl2, ::Windows::UI::Xaml::Controls::IControl3, ::Windows::UI::Xaml::Controls::IControl4, ::Windows::UI::Xaml::Controls::IControl5, ::Windows::UI::Xaml::Controls::IControl7, ::Windows::UI::Xaml::Controls::IControlProtected, ::Windows::UI::Xaml::IFrameworkElement, ::Windows::UI::Xaml::IFrameworkElement2, ::Windows::UI::Xaml::IFrameworkElement3, ::Windows::UI::Xaml::IFrameworkElement4, ::Windows::UI::Xaml::IFrameworkElement6, ::Windows::UI::Xaml::IFrameworkElement7, ::Windows::UI::Xaml::IFrameworkElementProtected7, ::Windows::UI::Xaml::IUIElement, ::Windows::UI::Xaml::IUIElement2, ::Windows::UI::Xaml::IUIElement3, ::Windows::UI::Xaml::IUIElement4, ::Windows::UI::Xaml::IUIElement5, ::Windows::UI::Xaml::IUIElement7, ::Windows::UI::Xaml::IUIElement8, ::Windows::UI::Xaml::IUIElement9, ::Windows::UI::Xaml::IUIElement10, ::Windows::UI::Composition::IAnimationObject, ::Windows::UI::Composition::IVisualElement, ::Windows::UI::Xaml::IDependencyObject, ::Windows::UI::Xaml::IDependencyObject2>,
        impl::base<D, ::Windows::UI::Xaml::Controls::Page, ::Windows::UI::Xaml::Controls::UserControl, ::Windows::UI::Xaml::Controls::Control, ::Windows::UI::Xaml::FrameworkElement, ::Windows::UI::Xaml::UIElement, ::Windows::UI::Xaml::DependencyObject>,
        ::Windows::UI::Xaml::Controls::IPageOverridesT<D>, ::Windows::UI::Xaml::Controls::IControlOverridesT<D>, ::Windows::UI::Xaml::Controls::IControlOverrides6T<D>, ::Windows::UI::Xaml::IFrameworkElementOverridesT<D>, ::Windows::UI::Xaml::IFrameworkElementOverrides2T<D>, ::Windows::UI::Xaml::IUIElementOverridesT<D>, ::Windows::UI::Xaml::IUIElementOverrides7T<D>, ::Windows::UI::Xaml::IUIElementOverrides8T<D>, ::Windows::UI::Xaml::IUIElementOverrides9T<D>
    {
        using base_type = MainPageBase_base;
        using class_type = Simple::MainPageBase;
        using implements_type = typename MainPageBase_base::implements_type;
        using implements_type::implements_type;
        using composable_base = ::Windows::UI::Xaml::Controls::Page;
        hstring GetRuntimeClassName() const
        {
            return L"Simple.MainPageBase";
        }
        MainPageBase_base()
        {
            impl::call_factory<::Windows::UI::Xaml::Controls::Page, ::Windows::UI::Xaml::Controls::IPageFactory>([&](auto&& f) { f.CreateInstance(*this, this->m_inner); });
        }

    protected:
        using dispatch = impl::dispatch_to_overridable<D, ::Windows::UI::Xaml::Controls::IPageOverrides, ::Windows::UI::Xaml::Controls::IControlOverrides, ::Windows::UI::Xaml::Controls::IControlOverrides6, ::Windows::UI::Xaml::IFrameworkElementOverrides, ::Windows::UI::Xaml::IFrameworkElementOverrides2, ::Windows::UI::Xaml::IUIElementOverrides, ::Windows::UI::Xaml::IUIElementOverrides7, ::Windows::UI::Xaml::IUIElementOverrides8, ::Windows::UI::Xaml::IUIElementOverrides9>;
        auto overridable() noexcept { return dispatch::overridable(static_cast<D&>(*this)); }
    };
}
namespace winrt::Simple::factory_implementation
{
    template <typename D, typename T, typename... I>
    struct __declspec(empty_bases) MainPageBaseT : implements<D, ::Windows::Foundation::IActivationFactory, Simple::IMainPageBaseFactory, I...>
    {
        using instance_type = Simple::MainPageBase;

        hstring GetRuntimeClassName() const
        {
            return L"Simple.MainPageBase";
        }
        auto CreateInstance(::Windows::Foundation::IInspectable const& baseInterface, ::Windows::Foundation::IInspectable& innerInterface)
        {
            return impl::composable_factory<T>::template CreateInstance<Simple::MainPageBase>(baseInterface, innerInterface);
        }
        [[noreturn]] ::Windows::Foundation::IInspectable ActivateInstance() const
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
