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
struct WINRT_EBO MyItem_base : implements<D, BindTestbed::IMyItem, ::Windows::UI::Xaml::Data::INotifyPropertyChanged, composing, I...>,
    impl::require<D, ::Windows::UI::Xaml::IDependencyObject, ::Windows::UI::Xaml::IDependencyObject2>,
    impl::base<D, ::Windows::UI::Xaml::DependencyObject>
{
    using base_type = MyItem_base;
    using class_type = BindTestbed::MyItem;
    using implements_type = typename MyItem_base::implements_type;
    using implements_type::implements_type;
    using composable_base = ::Windows::UI::Xaml::DependencyObject;
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
        return L"BindTestbed.MyItem";
    }
    MyItem_base()
    {
        impl::call_factory<::Windows::UI::Xaml::DependencyObject, ::Windows::UI::Xaml::IDependencyObjectFactory>([&](auto&& f) { f.CreateInstance(*this, this->m_inner); });
    }
};

}

namespace winrt::BindTestbed::factory_implementation {

template <typename D, typename T, typename... I>
struct WINRT_EBO MyItemT : implements<D, ::Windows::Foundation::IActivationFactory, BindTestbed::IMyItemStatics, I...>
{
    using instance_type = BindTestbed::MyItem;

    hstring GetRuntimeClassName() const
    {
        return L"BindTestbed.MyItem";
    }

    ::Windows::Foundation::IInspectable ActivateInstance() const
    {
        return make<T>();
    }

    ::Windows::UI::Xaml::DependencyProperty DPOnMyItemProperty()
    {
        return T::DPOnMyItemProperty();
    }

    void DPOnMyItemProperty(::Windows::UI::Xaml::DependencyProperty const& value)
    {
        T::DPOnMyItemProperty(value);
    }
};

}

#if defined(WINRT_FORCE_INCLUDE_MYITEM_XAML_G_H) || __has_include("MyItem.xaml.g.h")

#include "MyItem.xaml.g.h"

#else

namespace winrt::BindTestbed::implementation
{
    template <typename D, typename... I>
    using MyItemT = MyItem_base<D, I...>;
}

#endif
