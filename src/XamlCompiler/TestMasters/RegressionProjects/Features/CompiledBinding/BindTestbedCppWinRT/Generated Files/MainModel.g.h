// WARNING: Please don't edit this file...

#pragma once
#include "winrt/BindTestbed.h"
namespace winrt::BindTestbed::implementation
{
    template <typename D, typename... I>
    struct __declspec(empty_bases) MainModel_base : implements<D, BindTestbed::MainModel, I...>
    {
        using base_type = MainModel_base;
        using class_type = BindTestbed::MainModel;
        using implements_type = typename MainModel_base::implements_type;
        using implements_type::implements_type;
        
        hstring GetRuntimeClassName() const
        {
            return L"BindTestbed.MainModel";
        }
    };
}
namespace winrt::BindTestbed::factory_implementation
{
    template <typename D, typename T, typename... I>
    struct __declspec(empty_bases) MainModelT : implements<D, ::Windows::Foundation::IActivationFactory, I...>
    {
        using instance_type = BindTestbed::MainModel;

        hstring GetRuntimeClassName() const
        {
            return L"BindTestbed.MainModel";
        }
        auto ActivateInstance() const
        {
            return make<T>();
        }
    };
}

#if defined(WINRT_FORCE_INCLUDE_MAINMODEL_XAML_G_H) || __has_include("MainModel.xaml.g.h")
#include "MainModel.xaml.g.h"
#else

namespace winrt::BindTestbed::implementation
{
    template <typename D, typename... I>
    using MainModelT = MainModel_base<D, I...>;
}

#endif
