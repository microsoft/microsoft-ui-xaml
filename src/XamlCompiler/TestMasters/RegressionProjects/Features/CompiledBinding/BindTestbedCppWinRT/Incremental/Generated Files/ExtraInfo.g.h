// WARNING: Please don't edit this file...

#pragma once
#include "winrt/BindTestbed.h"
namespace winrt::BindTestbed::implementation
{
    template <typename D, typename... I>
    struct __declspec(empty_bases) ExtraInfo_base : implements<D, BindTestbed::ExtraInfo, I...>
    {
        using base_type = ExtraInfo_base;
        using class_type = BindTestbed::ExtraInfo;
        using implements_type = typename ExtraInfo_base::implements_type;
        using implements_type::implements_type;
        
        hstring GetRuntimeClassName() const
        {
            return L"BindTestbed.ExtraInfo";
        }
    };
}
namespace winrt::BindTestbed::factory_implementation
{
    template <typename D, typename T, typename... I>
    struct __declspec(empty_bases) ExtraInfoT : implements<D, ::Windows::Foundation::IActivationFactory, I...>
    {
        using instance_type = BindTestbed::ExtraInfo;

        hstring GetRuntimeClassName() const
        {
            return L"BindTestbed.ExtraInfo";
        }
        auto ActivateInstance() const
        {
            return make<T>();
        }
    };
}

#if defined(WINRT_FORCE_INCLUDE_EXTRAINFO_XAML_G_H) || __has_include("ExtraInfo.xaml.g.h")
#include "ExtraInfo.xaml.g.h"
#else

namespace winrt::BindTestbed::implementation
{
    template <typename D, typename... I>
    using ExtraInfoT = ExtraInfo_base<D, I...>;
}

#endif
