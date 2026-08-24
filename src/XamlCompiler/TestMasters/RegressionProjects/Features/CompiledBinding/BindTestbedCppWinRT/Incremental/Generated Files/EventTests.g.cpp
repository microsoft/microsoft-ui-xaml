// WARNING: Please don't edit this file...

void* winrt_make_BindTestbed_EventTests()
{
    return winrt::detach_abi(winrt::make<winrt::BindTestbed::factory_implementation::EventTests>());
}
namespace winrt::BindTestbed
{
    EventTests::EventTests() :
        EventTests(make<BindTestbed::implementation::EventTests>())
    {
    }
}
