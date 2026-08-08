// WARNING: Please don't edit this file...

#pragma once

#include "winrt/BindTestbedCXModel.h"
#include "winrt/BindTestbedModel.h"
#include "winrt/Windows.UI.Xaml.h"
#include "winrt/Windows.UI.Xaml.Controls.h"
#include "winrt/Windows.UI.Xaml.Data.h"
#include "winrt/Windows.UI.Xaml.Interop.h"
#include "winrt/Windows.UI.Xaml.Markup.h"
#include "winrt/Windows.Foundation.Collections.h"
#include "winrt/Windows.UI.Composition.h"
#include "winrt/BindTestbed.h"

namespace winrt::BindTestbed::implementation {

template <typename D, typename... I>
struct WINRT_EBO MyInfo_base : implements<D, BindTestbed::IMyInfo, I...>
{
    using base_type = MyInfo_base;
    using class_type = BindTestbed::MyInfo;
    using implements_type = typename MyInfo_base::implements_type;
    using implements_type::implements_type;
    
#if _MSC_VER < 1914
    operator class_type() const noexcept
    {
        static_assert(std::is_same_v<typename impl::implements_default_interface<D>::type, default_interface<class_type>>);
        class_type result{ nullptr };
        attach_abi(result, detach_abi(static_cast<default_interface<class_type>>(*this)));
        return result;
    }
#else
    operator impl::producer_ref<class_type> const() const noexcept
    {
        return { to_abi<default_interface<class_type>>(this) };
    }
#endif

    hstring GetRuntimeClassName() const
    {
        return L"BindTestbed.MyInfo";
    }
};

}

namespace winrt::BindTestbed::factory_implementation {

template <typename D, typename T, typename... I>
struct WINRT_EBO MyInfoT : implements<D, ::Windows::Foundation::IActivationFactory, I...>
{
    using instance_type = BindTestbed::MyInfo;

    hstring GetRuntimeClassName() const
    {
        return L"BindTestbed.MyInfo";
    }

    ::Windows::Foundation::IInspectable ActivateInstance() const
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
