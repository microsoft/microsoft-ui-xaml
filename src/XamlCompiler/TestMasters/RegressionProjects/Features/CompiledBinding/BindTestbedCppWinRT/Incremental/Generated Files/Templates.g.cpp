// WARNING: Please don't edit this file...

void* winrt_make_BindTestbed_Templates()
{
    return winrt::detach_abi(winrt::make<winrt::BindTestbed::factory_implementation::Templates>());
}
namespace winrt::BindTestbed
{
    Templates::Templates() :
        Templates(make<BindTestbed::implementation::Templates>())
    {
    }
}
