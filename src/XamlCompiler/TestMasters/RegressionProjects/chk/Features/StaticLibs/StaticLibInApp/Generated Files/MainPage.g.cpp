// WARNING: Please don't edit this file...

void* winrt_make_StaticLibInApp_MainPage()
{
    return winrt::detach_abi(winrt::make<winrt::StaticLibInApp::factory_implementation::MainPage>());
}
WINRT_EXPORT namespace winrt::StaticLibInApp
{
    MainPage::MainPage() :
        MainPage(make<StaticLibInApp::implementation::MainPage>())
    {
    }
}
