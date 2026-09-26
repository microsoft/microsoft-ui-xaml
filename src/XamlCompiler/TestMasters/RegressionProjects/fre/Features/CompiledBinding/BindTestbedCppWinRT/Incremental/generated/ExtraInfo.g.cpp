// WARNING: Please don't edit this file...

void* winrt_make_BindTestbed_ExtraInfo()
{
    return winrt::detach_abi(winrt::make<winrt::BindTestbed::factory_implementation::ExtraInfo>());
}
WINRT_EXPORT namespace winrt::BindTestbed
{
    ExtraInfo::ExtraInfo() :
        ExtraInfo(make<BindTestbed::implementation::ExtraInfo>())
    {
    }
}
