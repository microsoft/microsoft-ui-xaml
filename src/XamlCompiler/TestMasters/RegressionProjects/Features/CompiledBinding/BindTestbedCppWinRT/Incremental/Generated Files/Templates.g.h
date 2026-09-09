// WARNING: Please don't edit this file...

#pragma once
#include "winrt/BindTestbed.h"
#include "winrt/Microsoft.UI.Xaml.h"
#include "winrt/Windows.Foundation.Collections.h"
namespace winrt::BindTestbed::implementation
{
    template <typename D, typename... I>
    struct WINRT_IMPL_EMPTY_BASES Templates_base : implements<D, BindTestbed::Templates, composing, I...>,
        impl::require<D, winrt::Microsoft::UI::Xaml::IResourceDictionary, winrt::Windows::Foundation::Collections::IIterable<winrt::Windows::Foundation::Collections::IKeyValuePair<winrt::Windows::Foundation::IInspectable, winrt::Windows::Foundation::IInspectable>>, winrt::Windows::Foundation::Collections::IMap<winrt::Windows::Foundation::IInspectable, winrt::Windows::Foundation::IInspectable>, winrt::Microsoft::UI::Xaml::IDependencyObject>,
        impl::base<D, winrt::Microsoft::UI::Xaml::ResourceDictionary, winrt::Microsoft::UI::Xaml::DependencyObject>
    {
        using base_type = Templates_base;
        using class_type = BindTestbed::Templates;
        using implements_type = typename Templates_base::implements_type;
        using implements_type::implements_type;
        using composable_base = winrt::Microsoft::UI::Xaml::ResourceDictionary;
        hstring GetRuntimeClassName() const
        {
            return L"BindTestbed.Templates";
        }
        Templates_base()
        {
            impl::call_factory<winrt::Microsoft::UI::Xaml::ResourceDictionary, winrt::Microsoft::UI::Xaml::IResourceDictionaryFactory>([&](winrt::Microsoft::UI::Xaml::IResourceDictionaryFactory const& f) { [[maybe_unused]] auto winrt_impl_discarded = f.CreateInstance(*this, this->m_inner); });
        }
    };
}
namespace winrt::BindTestbed::factory_implementation
{
    template <typename D, typename T, typename... I>
    struct WINRT_IMPL_EMPTY_BASES TemplatesT : implements<D, winrt::Windows::Foundation::IActivationFactory, I...>
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
