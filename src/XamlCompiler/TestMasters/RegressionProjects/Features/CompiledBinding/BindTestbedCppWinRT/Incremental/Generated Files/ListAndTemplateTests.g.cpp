// WARNING: Please don't edit this file...

void* winrt_make_BindTestbed_ListAndTemplateTests()
{
    return winrt::detach_abi(winrt::make<winrt::BindTestbed::factory_implementation::ListAndTemplateTests>());
}
namespace winrt::BindTestbed
{
    ListAndTemplateTests::ListAndTemplateTests() :
        ListAndTemplateTests(make<BindTestbed::implementation::ListAndTemplateTests>())
    {
    }
}
