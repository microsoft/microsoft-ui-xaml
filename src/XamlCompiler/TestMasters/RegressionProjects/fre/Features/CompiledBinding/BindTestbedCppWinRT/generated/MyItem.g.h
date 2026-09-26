// WARNING: Please don't edit this file...

#pragma once
#include "winrt/BindTestbed.h"
#include "winrt/Microsoft.UI.Xaml.h"
#include "winrt/Microsoft.UI.Xaml.Data.h"
namespace winrt::BindTestbed::implementation
{
    template <typename D, typename... I>
    struct WINRT_IMPL_EMPTY_BASES MyItem_base : implements<D, BindTestbed::MyItem, winrt::Microsoft::UI::Xaml::Data::INotifyPropertyChanged, composing, I...>,
        impl::require<D, winrt::Microsoft::UI::Xaml::IDependencyObject>,
        impl::base<D, winrt::Microsoft::UI::Xaml::DependencyObject>
    {
        using base_type = MyItem_base;
        using class_type = BindTestbed::MyItem;
        using implements_type = typename MyItem_base::implements_type;
        using implements_type::implements_type;
        using composable_base = winrt::Microsoft::UI::Xaml::DependencyObject;
        hstring GetRuntimeClassName() const
        {
            return L"BindTestbed.MyItem";
        }
        MyItem_base()
        {
            impl::call_factory<winrt::Microsoft::UI::Xaml::DependencyObject, winrt::Microsoft::UI::Xaml::IDependencyObjectFactory>([&](winrt::Microsoft::UI::Xaml::IDependencyObjectFactory const& f) { [[maybe_unused]] auto winrt_impl_discarded = f.CreateInstance(*this, this->m_inner); });
        }
    };
}
namespace winrt::BindTestbed::factory_implementation
{
    template <typename D, typename T, typename... I>
    struct WINRT_IMPL_EMPTY_BASES MyItemT : implements<D, winrt::Windows::Foundation::IActivationFactory, winrt::BindTestbed::IMyItemStatics, I...>
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
        auto DPOnMyItemProperty(winrt::Microsoft::UI::Xaml::DependencyProperty const& value)
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
