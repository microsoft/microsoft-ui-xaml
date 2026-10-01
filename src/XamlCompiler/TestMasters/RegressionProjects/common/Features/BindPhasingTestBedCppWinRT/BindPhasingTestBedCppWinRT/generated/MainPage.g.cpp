// WARNING: Please don't edit this file...

void* winrt_make_BindPhasingTestBedCppWinRT_MainPage()
{
    return winrt::detach_abi(winrt::make<winrt::BindPhasingTestBedCppWinRT::factory_implementation::MainPage>());
}
WINRT_EXPORT namespace winrt::BindPhasingTestBedCppWinRT
{
    MainPage::MainPage() :
        MainPage(make<BindPhasingTestBedCppWinRT::implementation::MainPage>())
    {
    }
}
