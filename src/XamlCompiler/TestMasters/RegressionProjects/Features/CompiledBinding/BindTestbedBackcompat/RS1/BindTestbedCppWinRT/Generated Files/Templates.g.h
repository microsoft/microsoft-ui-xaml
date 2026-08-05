// WARNING: Please don't edit this file...

#pragma once

#include "winrt/BindTestbedCXModel.h"
#include "winrt/BindTestbedModel.h"
#include "winrt/Windows.UI.Xaml.h"
#include "winrt/Windows.UI.Xaml.Controls.h"
#include "winrt/Windows.UI.Xaml.Data.h"
#include "winrt/Windows.UI.Xaml.Interop.h"
#include "winrt/Windows.UI.Xaml.Markup.h"
#include "winrt/Windows.Foundation.Collections.h"
#include "winrt/Windows.UI.Composition.h"
#include "winrt/BindTestbed.h"

namespace winrt::BindTestbed::implementation {

template <typename D, typename... I>
struct WINRT_EBO Templates_base : implements<D, BindTestbed::ITemplates, composing, I...>,
    impl::require<D, ::Windows::Foundation::Collections::IIterable<::Windows::Foundation::Collections::IKeyValuePair<::Windows::Foundation::IInspectable, ::Windows::Foundation::IInspectable>>, ::Windows::Foundation::Collections::IMap<::Windows::Foundation::IInspectable, ::Windows::Foundation::IInspectable>, ::Windows::UI::Xaml::IDependencyObject, ::Windows::UI::Xaml::IDependencyObject2, ::Windows::UI::Xaml::IResourceDictionary>,
    impl::base<D, ::Windows::UI::Xaml::ResourceDictionary, ::Windows::UI::Xaml::DependencyObject>
{
    using base_type = Templates_base;
    using class_type = BindTestbed::Templates;
    using implements_type = typename Templates_base::implements_type;
    using implements_type::implements_type;
    using composable_base = ::Windows::UI::Xaml::ResourceDictionary;
#if _MSC_VER < 1914
    operator class_type() const noexcept
    {
        static_assert(std::is_same_v<typename impl::implements_default_interface<D>::type, default_interface<class_type>>);
        class_type result{ nullptr };
        attach_abi(result, detach_abi(static_cast<default_interface<class_type>>(*this)));
        return result;
    }
#else
    operator impl::producer_ref<class_type> const() const noexcept
    {
        return { to_abi<default_interface<class_type>>(this) };
    }
#endif

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

namespace winrt::BindTestbed::factory_implementation {

template <typename D, typename T, typename... I>
struct WINRT_EBO TemplatesT : implements<D, ::Windows::Foundation::IActivationFactory, I...>
{
    using instance_type = BindTestbed::Templates;

    hstring GetRuntimeClassName() const
    {
        return L"BindTestbed.Templates";
    }

    ::Windows::Foundation::IInspectable ActivateInstance() const
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
