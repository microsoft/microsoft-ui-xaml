// WARNING: Please don't edit this file...

#pragma once
#include "winrt/CppWinRTComponent.h"
namespace winrt::CppWinRTComponent::implementation
{
    template <typename D, typename... I>
    struct WINRT_IMPL_EMPTY_BASES Class_base : implements<D, CppWinRTComponent::Class, I...>
    {
        using base_type = Class_base;
        using class_type = CppWinRTComponent::Class;
        using implements_type = typename Class_base::implements_type;
        using implements_type::implements_type;
        
        hstring GetRuntimeClassName() const
        {
            return L"CppWinRTComponent.Class";
        }
    };
}
namespace winrt::CppWinRTComponent::factory_implementation
{
    template <typename D, typename T, typename... I>
    struct WINRT_IMPL_EMPTY_BASES ClassT : implements<D, winrt::Windows::Foundation::IActivationFactory, I...>
    {
        using instance_type = CppWinRTComponent::Class;

        hstring GetRuntimeClassName() const
        {
            return L"CppWinRTComponent.Class";
        }
        auto ActivateInstance() const
        {
            return make<T>();
        }
    };
}

#if defined(WINRT_FORCE_INCLUDE_CLASS_XAML_G_H) || __has_include("Class.xaml.g.h")

#include "Class.xaml.g.h"

#else

namespace winrt::CppWinRTComponent::implementation
{
    template <typename D, typename... I>
    using ClassT = Class_base<D, I...>;
}

#endif
