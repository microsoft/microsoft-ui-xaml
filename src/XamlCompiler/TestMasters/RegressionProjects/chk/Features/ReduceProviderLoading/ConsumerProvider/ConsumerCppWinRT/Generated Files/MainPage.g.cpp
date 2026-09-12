// WARNING: Please don't edit this file...

void* winrt_make_ConsumerCppWinRT_MainPage()
{
    return winrt::detach_abi(winrt::make<winrt::ConsumerCppWinRT::factory_implementation::MainPage>());
}
WINRT_EXPORT namespace winrt::ConsumerCppWinRT
{
    MainPage::MainPage() :
        MainPage(make<ConsumerCppWinRT::implementation::MainPage>())
    {
    }
}
