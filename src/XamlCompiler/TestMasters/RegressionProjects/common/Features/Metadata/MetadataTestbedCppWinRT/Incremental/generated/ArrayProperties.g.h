// WARNING: Please don't edit this file...

#pragma once
#include "winrt/MetadataTestbedCppWinRT.h"
namespace winrt::MetadataTestbedCppWinRT::implementation
{
    template <typename D, typename... I>
    struct WINRT_IMPL_EMPTY_BASES ArrayProperties_base : implements<D, MetadataTestbedCppWinRT::ArrayProperties, I...>
    {
        using base_type = ArrayProperties_base;
        using class_type = MetadataTestbedCppWinRT::ArrayProperties;
        using implements_type = typename ArrayProperties_base::implements_type;
        using implements_type::implements_type;
        
        hstring GetRuntimeClassName() const
        {
            return L"MetadataTestbedCppWinRT.ArrayProperties";
        }
    };
}
namespace winrt::MetadataTestbedCppWinRT::factory_implementation
{
    template <typename D, typename T, typename... I>
    struct WINRT_IMPL_EMPTY_BASES ArrayPropertiesT : implements<D, winrt::Windows::Foundation::IActivationFactory, winrt::MetadataTestbedCppWinRT::IArrayPropertiesStatics, I...>
    {
        using instance_type = MetadataTestbedCppWinRT::ArrayProperties;

        hstring GetRuntimeClassName() const
        {
            return L"MetadataTestbedCppWinRT.ArrayProperties";
        }
        auto ActivateInstance() const
        {
            return make<T>();
        }
        auto GetAttachedValues(winrt::Windows::Foundation::IInspectable const& target)
        {
            return T::GetAttachedValues(target);
        }
        auto SetAttachedValues(winrt::Windows::Foundation::IInspectable const& target, array_view<int32_t const> value)
        {
            return T::SetAttachedValues(target, value);
        }
    };
}

#if defined(WINRT_FORCE_INCLUDE_ARRAYPROPERTIES_XAML_G_H) || __has_include("ArrayProperties.xaml.g.h")

#include "ArrayProperties.xaml.g.h"

#else

namespace winrt::MetadataTestbedCppWinRT::implementation
{
    template <typename D, typename... I>
    using ArrayPropertiesT = ArrayProperties_base<D, I...>;
}

#endif
