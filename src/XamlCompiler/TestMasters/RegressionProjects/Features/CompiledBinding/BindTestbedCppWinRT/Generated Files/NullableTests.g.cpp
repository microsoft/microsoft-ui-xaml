// WARNING: Please don't edit this file...

void* winrt_make_BindTestbed_NullableTests()
{
    return winrt::detach_abi(winrt::make<winrt::BindTestbed::factory_implementation::NullableTests>());
}
namespace winrt::BindTestbed
{
    NullableTests::NullableTests() :
        NullableTests(make<BindTestbed::implementation::NullableTests>())
    {
    }
}
