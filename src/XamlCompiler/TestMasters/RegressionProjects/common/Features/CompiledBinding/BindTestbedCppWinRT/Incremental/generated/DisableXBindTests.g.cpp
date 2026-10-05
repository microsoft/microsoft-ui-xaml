// WARNING: Please don't edit this file...

void* winrt_make_BindTestbed_DisableXBindTests()
{
    return winrt::detach_abi(winrt::make<winrt::BindTestbed::factory_implementation::DisableXBindTests>());
}
WINRT_EXPORT namespace winrt::BindTestbed
{
    DisableXBindTests::DisableXBindTests() :
        DisableXBindTests(make<BindTestbed::implementation::DisableXBindTests>())
    {
    }
}
