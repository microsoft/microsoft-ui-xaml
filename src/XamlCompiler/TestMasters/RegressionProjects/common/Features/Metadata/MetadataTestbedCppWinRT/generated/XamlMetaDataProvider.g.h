// WARNING: Please don't edit this file...

#pragma once
#include "winrt/MetadataTestbedCppWinRT.h"
#include "winrt/Microsoft.UI.Xaml.Markup.h"
namespace winrt::MetadataTestbedCppWinRT::implementation
{
    template <typename D, typename... I>
    struct WINRT_IMPL_EMPTY_BASES XamlMetaDataProvider_base : implements<D, MetadataTestbedCppWinRT::XamlMetaDataProvider, I...>
    {
        using base_type = XamlMetaDataProvider_base;
        using class_type = MetadataTestbedCppWinRT::XamlMetaDataProvider;
        using implements_type = typename XamlMetaDataProvider_base::implements_type;
        using implements_type::implements_type;
        
        hstring GetRuntimeClassName() const
        {
            return L"MetadataTestbedCppWinRT.XamlMetaDataProvider";
        }
    };
}
namespace winrt::MetadataTestbedCppWinRT::factory_implementation
{
    template <typename D, typename T, typename... I>
    struct WINRT_IMPL_EMPTY_BASES XamlMetaDataProviderT : implements<D, winrt::Windows::Foundation::IActivationFactory, I...>
    {
        using instance_type = MetadataTestbedCppWinRT::XamlMetaDataProvider;

        hstring GetRuntimeClassName() const
        {
            return L"MetadataTestbedCppWinRT.XamlMetaDataProvider";
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

namespace winrt::MetadataTestbedCppWinRT::implementation
{
    template <typename D, typename... I>
    using XamlMetaDataProviderT = XamlMetaDataProvider_base<D, I...>;
}

#endif
