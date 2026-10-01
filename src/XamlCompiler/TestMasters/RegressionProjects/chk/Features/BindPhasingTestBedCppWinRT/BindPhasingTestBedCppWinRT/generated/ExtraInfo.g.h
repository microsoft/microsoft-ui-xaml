// WARNING: Please don't edit this file...

#pragma once
#include "winrt/BindPhasingTestBedCppWinRT.h"
namespace winrt::BindPhasingTestBedCppWinRT::implementation
{
    template <typename D, typename... I>
    struct WINRT_IMPL_EMPTY_BASES ExtraInfo_base : implements<D, BindPhasingTestBedCppWinRT::ExtraInfo, I...>
    {
        using base_type = ExtraInfo_base;
        using class_type = BindPhasingTestBedCppWinRT::ExtraInfo;
        using implements_type = typename ExtraInfo_base::implements_type;
        using implements_type::implements_type;
        
        hstring GetRuntimeClassName() const
        {
            return L"BindPhasingTestBedCppWinRT.ExtraInfo";
        }
    };
}
namespace winrt::BindPhasingTestBedCppWinRT::factory_implementation
{
    template <typename D, typename T, typename... I>
    struct WINRT_IMPL_EMPTY_BASES ExtraInfoT : implements<D, winrt::Windows::Foundation::IActivationFactory, winrt::BindPhasingTestBedCppWinRT::IExtraInfoFactory, I...>
    {
        using instance_type = BindPhasingTestBedCppWinRT::ExtraInfo;

        hstring GetRuntimeClassName() const
        {
            return L"BindPhasingTestBedCppWinRT.ExtraInfo";
        }
        auto CreateInstance(hstring const& caption)
        {
            return make<T>(caption);
        }
        [[noreturn]] winrt::Windows::Foundation::IInspectable ActivateInstance() const
        {
            throw hresult_not_implemented();
        }
    };
}

#if defined(WINRT_FORCE_INCLUDE_EXTRAINFO_XAML_G_H) || __has_include("ExtraInfo.xaml.g.h")

#include "ExtraInfo.xaml.g.h"

#else

namespace winrt::BindPhasingTestBedCppWinRT::implementation
{
    template <typename D, typename... I>
    using ExtraInfoT = ExtraInfo_base<D, I...>;
}

#endif
