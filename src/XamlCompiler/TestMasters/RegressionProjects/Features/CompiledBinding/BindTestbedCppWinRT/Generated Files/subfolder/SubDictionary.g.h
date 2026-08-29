// WARNING: Please don't edit this file...

#pragma once
#include "winrt/BindTestbed.subfolder.h"
#include "winrt/Windows.Foundation.Collections.h"
#include "winrt/Windows.UI.Xaml.h"
namespace winrt::BindTestbed::subfolder::implementation
{
    template <typename D, typename... I>
    struct __declspec(empty_bases) SubDictionary_base : implements<D, BindTestbed::subfolder::SubDictionary, composing, I...>,
        impl::require<D, ::Windows::UI::Xaml::IResourceDictionary, ::Windows::Foundation::Collections::IIterable<::Windows::Foundation::Collections::IKeyValuePair<::Windows::Foundation::IInspectable, ::Windows::Foundation::IInspectable>>, ::Windows::Foundation::Collections::IMap<::Windows::Foundation::IInspectable, ::Windows::Foundation::IInspectable>, ::Windows::UI::Xaml::IDependencyObject, ::Windows::UI::Xaml::IDependencyObject2>,
        impl::base<D, ::Windows::UI::Xaml::ResourceDictionary, ::Windows::UI::Xaml::DependencyObject>
    {
        using base_type = SubDictionary_base;
        using class_type = BindTestbed::subfolder::SubDictionary;
        using implements_type = typename SubDictionary_base::implements_type;
        using implements_type::implements_type;
        using composable_base = ::Windows::UI::Xaml::ResourceDictionary;
        hstring GetRuntimeClassName() const
        {
            return L"BindTestbed.subfolder.SubDictionary";
        }
        SubDictionary_base()
        {
            impl::call_factory<::Windows::UI::Xaml::ResourceDictionary, ::Windows::UI::Xaml::IResourceDictionaryFactory>([&](auto&& f) { f.CreateInstance(*this, this->m_inner); });
        }
    };
}
namespace winrt::BindTestbed::subfolder::factory_implementation
{
    template <typename D, typename T, typename... I>
    struct __declspec(empty_bases) SubDictionaryT : implements<D, ::Windows::Foundation::IActivationFactory, I...>
    {
        using instance_type = BindTestbed::subfolder::SubDictionary;

        hstring GetRuntimeClassName() const
        {
            return L"BindTestbed.subfolder.SubDictionary";
        }
        auto ActivateInstance() const
        {
            return make<T>();
        }
    };
}

#if defined(WINRT_FORCE_INCLUDE_SUBDICTIONARY_XAML_G_H) || __has_include("subfolder/SubDictionary.xaml.g.h")
#include "subfolder/SubDictionary.xaml.g.h"
#else

namespace winrt::BindTestbed::subfolder::implementation
{
    template <typename D, typename... I>
    using SubDictionaryT = SubDictionary_base<D, I...>;
}

#endif
