// WARNING: Please don't edit this file...

void* winrt_make_BindTestbed_DetectLeaksPage()
{
    return winrt::detach_abi(winrt::make<winrt::BindTestbed::factory_implementation::DetectLeaksPage>());
}
WINRT_EXPORT namespace winrt::BindTestbed
{
    DetectLeaksPage::DetectLeaksPage() :
        DetectLeaksPage(make<BindTestbed::implementation::DetectLeaksPage>())
    {
    }
}
