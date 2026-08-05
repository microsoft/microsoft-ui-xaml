// WARNING: Please don't edit this file...

#pragma once
#include "winrt/BindTestbed.h"
namespace winrt::BindTestbed::implementation
{
    template <typename D, typename... I>
    struct __declspec(empty_bases) CastingModel_base : implements<D, BindTestbed::CastingModel, I...>
    {
        using base_type = CastingModel_base;
        using class_type = BindTestbed::CastingModel;
        using implements_type = typename CastingModel_base::implements_type;
        using implements_type::implements_type;
        
        hstring GetRuntimeClassName() const
        {
            return L"BindTestbed.CastingModel";
        }
    };
}
namespace winrt::BindTestbed::factory_implementation
{
    template <typename D, typename T, typename... I>
    struct __declspec(empty_bases) CastingModelT : implements<D, ::Windows::Foundation::IActivationFactory, I...>
    {
        using instance_type = BindTestbed::CastingModel;

        hstring GetRuntimeClassName() const
        {
            return L"BindTestbed.CastingModel";
        }
        auto ActivateInstance() const
        {
            return make<T>();
        }
    };
}

#if defined(WINRT_FORCE_INCLUDE_CASTINGMODEL_XAML_G_H) || __has_include("CastingModel.xaml.g.h")
#include "CastingModel.xaml.g.h"
#else

namespace winrt::BindTestbed::implementation
{
    template <typename D, typename... I>
    using CastingModelT = CastingModel_base<D, I...>;
}

#endif
