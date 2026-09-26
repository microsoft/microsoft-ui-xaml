// WARNING: Please don't edit this file...

void* winrt_make_CppWinRT_MainPage()
{
    return winrt::detach_abi(winrt::make<winrt::CppWinRT::factory_implementation::MainPage>());
}
WINRT_EXPORT namespace winrt::CppWinRT
{
    MainPage::MainPage() :
        MainPage(make<CppWinRT::implementation::MainPage>())
    {
    }
}
