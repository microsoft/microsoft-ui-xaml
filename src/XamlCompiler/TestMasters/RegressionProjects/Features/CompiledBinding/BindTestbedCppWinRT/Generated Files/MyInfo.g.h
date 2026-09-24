// WARNING: Please don't edit this file...

#pragma once
#include "winrt/BindTestbed.h"
namespace winrt::BindTestbed::implementation
{
    template <typename D, typename... I>
    struct __declspec(empty_bases) MyInfo_base : implements<D, BindTestbed::MyInfo, I...>
    {
        using base_type = MyInfo_base;
        using class_type = BindTestbed::MyInfo;
        using implements_type = typename MyInfo_base::implements_type;
        using implements_type::implements_type;
        
        hstring GetRuntimeClassName() const
        {
            return L"BindTestbed.MyInfo";
        }
    };
}
namespace winrt::BindTestbed::factory_implementation
{
    template <typename D, typename T, typename... I>
    struct __declspec(empty_bases) MyInfoT : implements<D, ::Windows::Foundation::IActivationFactory, I...>
    {
        using instance_type = BindTestbed::MyInfo;

        hstring GetRuntimeClassName() const
        {
            return L"BindTestbed.MyInfo";
        }
        auto ActivateInstance() const
        {
            return make<T>();
        }
    };
}

#if defined(WINRT_FORCE_INCLUDE_MYINFO_XAML_G_H) || __has_include("MyInfo.xaml.g.h")
#include "MyInfo.xaml.g.h"
#else

namespace winrt::BindTestbed::implementation
{
    template <typename D, typename... I>
    using MyInfoT = MyInfo_base<D, I...>;
}

#endif
