// WARNING: Please don't edit this file...

void* winrt_make_BindTestbed_PhasingTests()
{
    return winrt::detach_abi(winrt::make<winrt::BindTestbed::factory_implementation::PhasingTests>());
}
namespace winrt::BindTestbed
{
    PhasingTests::PhasingTests() :
        PhasingTests(make<BindTestbed::implementation::PhasingTests>())
    {
    }
}
