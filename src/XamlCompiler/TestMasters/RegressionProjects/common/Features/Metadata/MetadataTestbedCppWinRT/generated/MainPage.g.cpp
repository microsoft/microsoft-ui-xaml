// WARNING: Please don't edit this file...

void* winrt_make_MetadataTestbedCppWinRT_MainPage()
{
    return winrt::detach_abi(winrt::make<winrt::MetadataTestbedCppWinRT::factory_implementation::MainPage>());
}
WINRT_EXPORT namespace winrt::MetadataTestbedCppWinRT
{
    MainPage::MainPage() :
        MainPage(make<MetadataTestbedCppWinRT::implementation::MainPage>())
    {
    }
}
