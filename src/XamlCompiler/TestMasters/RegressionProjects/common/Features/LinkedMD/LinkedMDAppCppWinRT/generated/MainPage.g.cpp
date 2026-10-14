// WARNING: Please don't edit this file...

void* winrt_make_LinkedMDAppCppWinRT_MainPage()
{
    return winrt::detach_abi(winrt::make<winrt::LinkedMDAppCppWinRT::factory_implementation::MainPage>());
}
WINRT_EXPORT namespace winrt::LinkedMDAppCppWinRT
{
    MainPage::MainPage() :
        MainPage(make<LinkedMDAppCppWinRT::implementation::MainPage>())
    {
    }
}
