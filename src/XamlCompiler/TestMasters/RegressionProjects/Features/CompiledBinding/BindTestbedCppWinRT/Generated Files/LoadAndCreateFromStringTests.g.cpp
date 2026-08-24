// WARNING: Please don't edit this file...

void* winrt_make_BindTestbed_LoadAndCreateFromStringTests()
{
    return winrt::detach_abi(winrt::make<winrt::BindTestbed::factory_implementation::LoadAndCreateFromStringTests>());
}
namespace winrt::BindTestbed
{
    LoadAndCreateFromStringTests::LoadAndCreateFromStringTests() :
        LoadAndCreateFromStringTests(make<BindTestbed::implementation::LoadAndCreateFromStringTests>())
    {
    }
}
