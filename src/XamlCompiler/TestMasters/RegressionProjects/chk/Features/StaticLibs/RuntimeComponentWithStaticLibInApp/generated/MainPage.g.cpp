// WARNING: Please don't edit this file...

void* winrt_make_RuntimeComponentWithStaticLibInApp_MainPage()
{
    return winrt::detach_abi(winrt::make<winrt::RuntimeComponentWithStaticLibInApp::factory_implementation::MainPage>());
}
WINRT_EXPORT namespace winrt::RuntimeComponentWithStaticLibInApp
{
    MainPage::MainPage() :
        MainPage(make<RuntimeComponentWithStaticLibInApp::implementation::MainPage>())
    {
    }
}
