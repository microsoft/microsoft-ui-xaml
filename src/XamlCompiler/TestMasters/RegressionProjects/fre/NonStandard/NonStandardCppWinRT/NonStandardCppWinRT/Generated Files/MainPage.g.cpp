// WARNING: Please don't edit this file...

void* winrt_make_NonStandardCppWinRT_MainPage()
{
    return winrt::detach_abi(winrt::make<winrt::NonStandardCppWinRT::factory_implementation::MainPage>());
}
WINRT_EXPORT namespace winrt::NonStandardCppWinRT
{
    MainPage::MainPage() :
        MainPage(make<NonStandardCppWinRT::implementation::MainPage>())
    {
    }
}
