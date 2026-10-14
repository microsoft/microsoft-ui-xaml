// WARNING: Please don't edit this file...

void* winrt_make_BindTestbed_LonelyStaticBinding()
{
    return winrt::detach_abi(winrt::make<winrt::BindTestbed::factory_implementation::LonelyStaticBinding>());
}
WINRT_EXPORT namespace winrt::BindTestbed
{
    LonelyStaticBinding::LonelyStaticBinding() :
        LonelyStaticBinding(make<BindTestbed::implementation::LonelyStaticBinding>())
    {
    }
}
