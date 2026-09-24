// WARNING: Please don't edit this file...

void* winrt_make_BindTestbed_TwoWayTests()
{
    return winrt::detach_abi(winrt::make<winrt::BindTestbed::factory_implementation::TwoWayTests>());
}
namespace winrt::BindTestbed
{
    TwoWayTests::TwoWayTests() :
        TwoWayTests(make<BindTestbed::implementation::TwoWayTests>())
    {
    }
}
