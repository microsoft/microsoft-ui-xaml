// WARNING: Please don't edit this file...

#pragma once
#include "winrt/Simple.h"
namespace winrt::Simple::implementation
{
    template <typename D, typename... I>
    struct __declspec(empty_bases) PropBag_base : implements<D, Simple::PropBag, I...>
    {
        using base_type = PropBag_base;
        using class_type = Simple::PropBag;
        using implements_type = typename PropBag_base::implements_type;
        using implements_type::implements_type;
        
        hstring GetRuntimeClassName() const
        {
            return L"Simple.PropBag";
        }
    };
}
namespace winrt::Simple::factory_implementation
{
    template <typename D, typename T, typename... I>
    struct __declspec(empty_bases) PropBagT : implements<D, ::Windows::Foundation::IActivationFactory, I...>
    {
        using instance_type = Simple::PropBag;

        hstring GetRuntimeClassName() const
        {
            return L"Simple.PropBag";
        }
        auto ActivateInstance() const
        {
            return make<T>();
        }
    };
}

#if defined(WINRT_FORCE_INCLUDE_PROPBAG_XAML_G_H) || __has_include("PropBag.xaml.g.h")
#include "PropBag.xaml.g.h"
#else

namespace winrt::Simple::implementation
{
    template <typename D, typename... I>
    using PropBagT = PropBag_base<D, I...>;
}

#endif
