// WARNING: Please don't edit this file...

void* winrt_make_MultipleViewsTestbedCppWinRT_MainPage()
{
    return winrt::detach_abi(winrt::make<winrt::MultipleViewsTestbedCppWinRT::factory_implementation::MainPage>());
}
WINRT_EXPORT namespace winrt::MultipleViewsTestbedCppWinRT
{
    MainPage::MainPage() :
        MainPage(make<MultipleViewsTestbedCppWinRT::implementation::MainPage>())
    {
    }
}
