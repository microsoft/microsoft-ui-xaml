// WARNING: Please don't edit this file...

void* winrt_make_ProviderCppWinRT_MainPage()
{
    return winrt::detach_abi(winrt::make<winrt::ProviderCppWinRT::factory_implementation::MainPage>());
}
WINRT_EXPORT namespace winrt::ProviderCppWinRT
{
    MainPage::MainPage() :
        MainPage(make<ProviderCppWinRT::implementation::MainPage>())
    {
    }
    void MainPage::DoSomething()
    {
        ProviderCppWinRT::implementation::MainPage::DoSomething();
    }
    hstring MainPage::GetTextToShow()
    {
        return ProviderCppWinRT::implementation::MainPage::GetTextToShow();
    }
}
