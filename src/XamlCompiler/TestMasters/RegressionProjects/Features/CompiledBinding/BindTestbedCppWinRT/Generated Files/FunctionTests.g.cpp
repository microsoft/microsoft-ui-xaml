// WARNING: Please don't edit this file...

void* winrt_make_BindTestbed_FunctionTests()
{
    return winrt::detach_abi(winrt::make<winrt::BindTestbed::factory_implementation::FunctionTests>());
}
namespace winrt::BindTestbed
{
    FunctionTests::FunctionTests() :
        FunctionTests(make<BindTestbed::implementation::FunctionTests>())
    {
    }
}
