// WARNING: Please don't edit this file...

void* winrt_make_BindTestbed_PhasingTests()
{
    return winrt::detach_abi(winrt::make<winrt::BindTestbed::factory_implementation::PhasingTests>());
}
WINRT_EXPORT namespace winrt::BindTestbed
{
    PhasingTests::PhasingTests() :
        PhasingTests(make<BindTestbed::implementation::PhasingTests>())
    {
    }
}
