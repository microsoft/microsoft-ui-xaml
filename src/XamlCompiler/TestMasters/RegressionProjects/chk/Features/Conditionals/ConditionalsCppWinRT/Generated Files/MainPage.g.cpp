// WARNING: Please don't edit this file...

void* winrt_make_ConditionalsCppWinRT_MainPage()
{
    return winrt::detach_abi(winrt::make<winrt::ConditionalsCppWinRT::factory_implementation::MainPage>());
}
WINRT_EXPORT namespace winrt::ConditionalsCppWinRT
{
    MainPage::MainPage() :
        MainPage(make<ConditionalsCppWinRT::implementation::MainPage>())
    {
    }
}
