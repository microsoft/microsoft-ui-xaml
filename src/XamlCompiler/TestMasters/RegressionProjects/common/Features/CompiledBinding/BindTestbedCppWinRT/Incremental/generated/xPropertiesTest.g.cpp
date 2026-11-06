// WARNING: Please don't edit this file...

void* winrt_make_BindTestbed_xPropertiesTest()
{
    return winrt::detach_abi(winrt::make<winrt::BindTestbed::factory_implementation::xPropertiesTest>());
}
WINRT_EXPORT namespace winrt::BindTestbed
{
    xPropertiesTest::xPropertiesTest() :
        xPropertiesTest(make<BindTestbed::implementation::xPropertiesTest>())
    {
    }
}
