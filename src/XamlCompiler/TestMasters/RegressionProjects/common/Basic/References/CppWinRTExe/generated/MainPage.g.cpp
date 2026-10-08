// WARNING: Please don't edit this file...

void* winrt_make_CppWinRTExe_MainPage()
{
    return winrt::detach_abi(winrt::make<winrt::CppWinRTExe::factory_implementation::MainPage>());
}
WINRT_EXPORT namespace winrt::CppWinRTExe
{
    MainPage::MainPage() :
        MainPage(make<CppWinRTExe::implementation::MainPage>())
    {
    }
}
