// WARNING: Please don't edit this file...

void* winrt_make_StaticLibInApp_XamlMetaDataProvider()
{
    return winrt::detach_abi(winrt::make<winrt::StaticLibInApp::factory_implementation::XamlMetaDataProvider>());
}
WINRT_EXPORT namespace winrt::StaticLibInApp
{
    XamlMetaDataProvider::XamlMetaDataProvider() :
        XamlMetaDataProvider(make<StaticLibInApp::implementation::XamlMetaDataProvider>())
    {
    }
}
