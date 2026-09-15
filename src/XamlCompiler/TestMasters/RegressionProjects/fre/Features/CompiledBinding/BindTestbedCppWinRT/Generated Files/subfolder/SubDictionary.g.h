// WARNING: Please don't edit this file...

#pragma once
#include "winrt/BindTestbed.subfolder.h"
#include "winrt/Microsoft.UI.Xaml.h"
#include "winrt/Windows.Foundation.Collections.h"
namespace winrt::BindTestbed::subfolder::implementation
{
    template <typename D, typename... I>
    struct WINRT_IMPL_EMPTY_BASES SubDictionary_base : implements<D, BindTestbed::subfolder::SubDictionary, composing, I...>,
        impl::require<D, winrt::Microsoft::UI::Xaml::IResourceDictionary, winrt::Windows::Foundation::Collections::IIterable<winrt::Windows::Foundation::Collections::IKeyValuePair<winrt::Windows::Foundation::IInspectable, winrt::Windows::Foundation::IInspectable>>, winrt::Windows::Foundation::Collections::IMap<winrt::Windows::Foundation::IInspectable, winrt::Windows::Foundation::IInspectable>, winrt::Microsoft::UI::Xaml::IDependencyObject>,
        impl::base<D, winrt::Microsoft::UI::Xaml::ResourceDictionary, winrt::Microsoft::UI::Xaml::DependencyObject>
    {
        using base_type = SubDictionary_base;
        using class_type = BindTestbed::subfolder::SubDictionary;
        using implements_type = typename SubDictionary_base::implements_type;
        using implements_type::implements_type;
        using composable_base = winrt::Microsoft::UI::Xaml::ResourceDictionary;
        hstring GetRuntimeClassName() const
        {
            return L"BindTestbed.subfolder.SubDictionary";
        }
        SubDictionary_base()
        {
            impl::call_factory<winrt::Microsoft::UI::Xaml::ResourceDictionary, winrt::Microsoft::UI::Xaml::IResourceDictionaryFactory>([&](winrt::Microsoft::UI::Xaml::IResourceDictionaryFactory const& f) { [[maybe_unused]] auto winrt_impl_discarded = f.CreateInstance(*this, this->m_inner); });
        }
    };
}
namespace winrt::BindTestbed::subfolder::factory_implementation
{
    template <typename D, typename T, typename... I>
    struct WINRT_IMPL_EMPTY_BASES SubDictionaryT : implements<D, winrt::Windows::Foundation::IActivationFactory, I...>
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
