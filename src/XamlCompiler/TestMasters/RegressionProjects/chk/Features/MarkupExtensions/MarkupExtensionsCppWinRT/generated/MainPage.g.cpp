// WARNING: Please don't edit this file...

void* winrt_make_MarkupExtensionsCppWinRT_MainPage()
{
    return winrt::detach_abi(winrt::make<winrt::MarkupExtensionsCppWinRT::factory_implementation::MainPage>());
}
WINRT_EXPORT namespace winrt::MarkupExtensionsCppWinRT
{
    MainPage::MainPage() :
        MainPage(make<MarkupExtensionsCppWinRT::implementation::MainPage>())
    {
    }
}
