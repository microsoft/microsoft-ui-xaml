// WARNING: Please don't edit this file...

void* winrt_make_BindTestbed_TestsPage2()
{
    return winrt::detach_abi(winrt::make<winrt::BindTestbed::factory_implementation::TestsPage2>());
}
namespace winrt::BindTestbed
{
    TestsPage2::TestsPage2() :
        TestsPage2(make<BindTestbed::implementation::TestsPage2>())
    {
    }
}
