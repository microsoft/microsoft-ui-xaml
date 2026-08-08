// WARNING: Please don't edit this file...

void* winrt_make_BindTestbed_INotifyDataErrorInfoTests()
{
    return winrt::detach_abi(winrt::make<winrt::BindTestbed::factory_implementation::INotifyDataErrorInfoTests>());
}
namespace winrt::BindTestbed
{
    INotifyDataErrorInfoTests::INotifyDataErrorInfoTests() :
        INotifyDataErrorInfoTests(make<BindTestbed::implementation::INotifyDataErrorInfoTests>())
    {
    }
}
