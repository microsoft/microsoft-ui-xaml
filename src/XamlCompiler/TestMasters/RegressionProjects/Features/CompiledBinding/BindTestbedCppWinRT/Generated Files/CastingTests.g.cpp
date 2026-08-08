// WARNING: Please don't edit this file...

void* winrt_make_BindTestbed_CastingTests()
{
    return winrt::detach_abi(winrt::make<winrt::BindTestbed::factory_implementation::CastingTests>());
}
namespace winrt::BindTestbed
{
    CastingTests::CastingTests() :
        CastingTests(make<BindTestbed::implementation::CastingTests>())
    {
    }
}
