// WARNING: Please don't edit this file...

void* winrt_make_BindTestbed_MyInfo()
{
    return winrt::detach_abi(winrt::make<winrt::BindTestbed::factory_implementation::MyInfo>());
}
namespace winrt::BindTestbed
{
    MyInfo::MyInfo() :
        MyInfo(make<BindTestbed::implementation::MyInfo>())
    {
    }
}
