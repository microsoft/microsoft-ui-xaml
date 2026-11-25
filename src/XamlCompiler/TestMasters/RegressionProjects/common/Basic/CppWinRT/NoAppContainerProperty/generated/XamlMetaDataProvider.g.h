// WARNING: Please don't edit this file...

#pragma once
#include "winrt/Microsoft.UI.Xaml.Markup.h"
#include "winrt/NoAppContainerProperty.h"
namespace winrt::NoAppContainerProperty::implementation
{
    template <typename D, typename... I>
    struct WINRT_IMPL_EMPTY_BASES XamlMetaDataProvider_base : implements<D, NoAppContainerProperty::XamlMetaDataProvider, I...>
    {
        using base_type = XamlMetaDataProvider_base;
        using class_type = NoAppContainerProperty::XamlMetaDataProvider;
        using implements_type = typename XamlMetaDataProvider_base::implements_type;
        using implements_type::implements_type;
        
        hstring GetRuntimeClassName() const
        {
            return L"NoAppContainerProperty.XamlMetaDataProvider";
        }
    };
}
namespace winrt::NoAppContainerProperty::factory_implementation
{
    template <typename D, typename T, typename... I>
    struct WINRT_IMPL_EMPTY_BASES XamlMetaDataProviderT : implements<D, winrt::Windows::Foundation::IActivationFactory, I...>
    {
        using instance_type = NoAppContainerProperty::XamlMetaDataProvider;

        hstring GetRuntimeClassName() const
        {
            return L"NoAppContainerProperty.XamlMetaDataProvider";
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

namespace winrt::NoAppContainerProperty::implementation
{
    template <typename D, typename... I>
    using XamlMetaDataProviderT = XamlMetaDataProvider_base<D, I...>;
}

#endif
