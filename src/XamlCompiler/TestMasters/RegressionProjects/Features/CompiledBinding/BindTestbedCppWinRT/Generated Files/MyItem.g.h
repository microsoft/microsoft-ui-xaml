// WARNING: Please don't edit this file...

#pragma once
#include "winrt/BindTestbed.h"
#include "winrt/Windows.UI.Xaml.h"
#include "winrt/Windows.UI.Xaml.Data.h"
namespace winrt::BindTestbed::implementation
{
    template <typename D, typename... I>
    struct __declspec(empty_bases) MyItem_base : implements<D, BindTestbed::MyItem, ::Windows::UI::Xaml::Data::INotifyPropertyChanged, composing, I...>,
        impl::require<D, ::Windows::UI::Xaml::IDependencyObject, ::Windows::UI::Xaml::IDependencyObject2>,
        impl::base<D, ::Windows::UI::Xaml::DependencyObject>
    {
        using base_type = MyItem_base;
        using class_type = BindTestbed::MyItem;
        using implements_type = typename MyItem_base::implements_type;
        using implements_type::implements_type;
        using composable_base = ::Windows::UI::Xaml::DependencyObject;
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
namespace winrt::BindTestbed::factory_implementation
{
    template <typename D, typename T, typename... I>
    struct __declspec(empty_bases) MyItemT : implements<D, ::Windows::Foundation::IActivationFactory, BindTestbed::IMyItemStatics, I...>
    {
        using instance_type = BindTestbed::MyItem;

        hstring GetRuntimeClassName() const
        {
            return L"BindTestbed.MyItem";
        }
        auto ActivateInstance() const
        {
            return make<T>();
        }
        auto DPOnMyItemProperty()
        {
            return T::DPOnMyItemProperty();
        }
        auto DPOnMyItemProperty(::Windows::UI::Xaml::DependencyProperty const& value)
        {
            return T::DPOnMyItemProperty(value);
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
