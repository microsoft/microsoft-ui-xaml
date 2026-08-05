// WARNING: Please don't edit this file...

#pragma once
#include "winrt/BindTestbed.h"
#include "winrt/Windows.Foundation.Collections.h"
#include "winrt/Windows.UI.Xaml.h"
namespace winrt::BindTestbed::implementation
{
    template <typename D, typename... I>
    struct __declspec(empty_bases) Templates_base : implements<D, BindTestbed::Templates, composing, I...>,
        impl::require<D, ::Windows::UI::Xaml::IResourceDictionary, ::Windows::Foundation::Collections::IIterable<::Windows::Foundation::Collections::IKeyValuePair<::Windows::Foundation::IInspectable, ::Windows::Foundation::IInspectable>>, ::Windows::Foundation::Collections::IMap<::Windows::Foundation::IInspectable, ::Windows::Foundation::IInspectable>, ::Windows::UI::Xaml::IDependencyObject, ::Windows::UI::Xaml::IDependencyObject2>,
        impl::base<D, ::Windows::UI::Xaml::ResourceDictionary, ::Windows::UI::Xaml::DependencyObject>
    {
        using base_type = Templates_base;
        using class_type = BindTestbed::Templates;
        using implements_type = typename Templates_base::implements_type;
        using implements_type::implements_type;
        using composable_base = ::Windows::UI::Xaml::ResourceDictionary;
        hstring GetRuntimeClassName() const
        {
            return L"BindTestbed.Templates";
        }
        Templates_base()
        {
            impl::call_factory<::Windows::UI::Xaml::ResourceDictionary, ::Windows::UI::Xaml::IResourceDictionaryFactory>([&](auto&& f) { f.CreateInstance(*this, this->m_inner); });
        }
    };
}
namespace winrt::BindTestbed::factory_implementation
{
    template <typename D, typename T, typename... I>
    struct __declspec(empty_bases) TemplatesT : implements<D, ::Windows::Foundation::IActivationFactory, I...>
    {
        using instance_type = BindTestbed::Templates;

        hstring GetRuntimeClassName() const
        {
            return L"BindTestbed.Templates";
        }
        auto ActivateInstance() const
        {
            return make<T>();
        }
    };
}

#if defined(WINRT_FORCE_INCLUDE_TEMPLATES_XAML_G_H) || __has_include("Templates.xaml.g.h")
#include "Templates.xaml.g.h"
#else

namespace winrt::BindTestbed::implementation
{
    template <typename D, typename... I>
    using TemplatesT = Templates_base<D, I...>;
}

#endif
