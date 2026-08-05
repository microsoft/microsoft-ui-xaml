// WARNING: Please don't edit this file...

#pragma once
#include "winrt/Simple.h"
#include "winrt/Windows.UI.Xaml.Markup.h"
namespace winrt::Simple::implementation
{
    template <typename D, typename... I>
    struct __declspec(empty_bases) XamlMetaDataProvider_base : implements<D, Simple::XamlMetaDataProvider, I...>
    {
        using base_type = XamlMetaDataProvider_base;
        using class_type = Simple::XamlMetaDataProvider;
        using implements_type = typename XamlMetaDataProvider_base::implements_type;
        using implements_type::implements_type;
        
        hstring GetRuntimeClassName() const
        {
            return L"Simple.XamlMetaDataProvider";
        }
    };
}
namespace winrt::Simple::factory_implementation
{
    template <typename D, typename T, typename... I>
    struct __declspec(empty_bases) XamlMetaDataProviderT : implements<D, ::Windows::Foundation::IActivationFactory, I...>
    {
        using instance_type = Simple::XamlMetaDataProvider;

        hstring GetRuntimeClassName() const
        {
            return L"Simple.XamlMetaDataProvider";
        }
        auto ActivateInstance() const
        {
            return make<T>();
        }
    };
}

#if defined(WINRT_FORCE_INCLUDE_XAMLMETADATAPROVIDER_XAML_G_H) || __has_include("XamlMetaDataProvider.xaml.g.h")
#include "XamlMetaDataProvider.xaml.g.h"
#else

namespace winrt::Simple::implementation
{
    template <typename D, typename... I>
    using XamlMetaDataProviderT = XamlMetaDataProvider_base<D, I...>;
}

#endif
