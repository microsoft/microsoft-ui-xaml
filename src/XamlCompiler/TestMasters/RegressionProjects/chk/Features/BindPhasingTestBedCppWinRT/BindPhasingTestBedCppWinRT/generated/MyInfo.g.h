// WARNING: Please don't edit this file...

#pragma once
#include "winrt/BindPhasingTestBedCppWinRT.h"
namespace winrt::BindPhasingTestBedCppWinRT::implementation
{
    template <typename D, typename... I>
    struct WINRT_IMPL_EMPTY_BASES MyInfo_base : implements<D, BindPhasingTestBedCppWinRT::MyInfo, I...>
    {
        using base_type = MyInfo_base;
        using class_type = BindPhasingTestBedCppWinRT::MyInfo;
        using implements_type = typename MyInfo_base::implements_type;
        using implements_type::implements_type;
        
        hstring GetRuntimeClassName() const
        {
            return L"BindPhasingTestBedCppWinRT.MyInfo";
        }
    };
}
namespace winrt::BindPhasingTestBedCppWinRT::factory_implementation
{
    template <typename D, typename T, typename... I>
    struct WINRT_IMPL_EMPTY_BASES MyInfoT : implements<D, winrt::Windows::Foundation::IActivationFactory, winrt::BindPhasingTestBedCppWinRT::IMyInfoFactory, I...>
    {
        using instance_type = BindPhasingTestBedCppWinRT::MyInfo;

        hstring GetRuntimeClassName() const
        {
            return L"BindPhasingTestBedCppWinRT.MyInfo";
        }
        auto CreateInstance(hstring const& imageUrl, hstring const& caption)
        {
            return make<T>(imageUrl, caption);
        }
        [[noreturn]] winrt::Windows::Foundation::IInspectable ActivateInstance() const
        {
            throw hresult_not_implemented();
        }
    };
}

#if defined(WINRT_FORCE_INCLUDE_MYINFO_XAML_G_H) || __has_include("MyInfo.xaml.g.h")

#include "MyInfo.xaml.g.h"

#else

namespace winrt::BindPhasingTestBedCppWinRT::implementation
{
    template <typename D, typename... I>
    using MyInfoT = MyInfo_base<D, I...>;
}

#endif
