// WARNING: Please don't edit this file...

void* winrt_make_BindTestbed_MyUserControl1()
{
    return winrt::detach_abi(winrt::make<winrt::BindTestbed::factory_implementation::MyUserControl1>());
}
namespace winrt::BindTestbed
{
    MyUserControl1::MyUserControl1() :
        MyUserControl1(make<BindTestbed::implementation::MyUserControl1>())
    {
    }
}
