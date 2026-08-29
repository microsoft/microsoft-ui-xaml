// WARNING: Please don't edit this file...

void* winrt_make_BindTestbed_MyItem()
{
    return winrt::detach_abi(winrt::make<winrt::BindTestbed::factory_implementation::MyItem>());
}
namespace winrt::BindTestbed
{
    MyItem::MyItem() :
        MyItem(make<BindTestbed::implementation::MyItem>())
    {
    }
    ::Windows::UI::Xaml::DependencyProperty MyItem::DPOnMyItemProperty()
    {
        return BindTestbed::implementation::MyItem::DPOnMyItemProperty();
    }
    void MyItem::DPOnMyItemProperty(::Windows::UI::Xaml::DependencyProperty const& value)
    {
        return BindTestbed::implementation::MyItem::DPOnMyItemProperty(value);
    }
}
