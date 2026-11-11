// WARNING: Please don't edit this file...

void* winrt_make_BindTestbed_TwoWayStructTests()
{
    return winrt::detach_abi(winrt::make<winrt::BindTestbed::factory_implementation::TwoWayStructTests>());
}
WINRT_EXPORT namespace winrt::BindTestbed
{
    TwoWayStructTests::TwoWayStructTests() :
        TwoWayStructTests(make<BindTestbed::implementation::TwoWayStructTests>())
    {
    }
}
