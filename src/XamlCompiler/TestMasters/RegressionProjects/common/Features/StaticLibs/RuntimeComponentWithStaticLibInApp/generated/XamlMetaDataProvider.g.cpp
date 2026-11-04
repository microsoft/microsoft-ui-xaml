// WARNING: Please don't edit this file...

void* winrt_make_RuntimeComponentWithStaticLibInApp_XamlMetaDataProvider()
{
    return winrt::detach_abi(winrt::make<winrt::RuntimeComponentWithStaticLibInApp::factory_implementation::XamlMetaDataProvider>());
}
WINRT_EXPORT namespace winrt::RuntimeComponentWithStaticLibInApp
{
    XamlMetaDataProvider::XamlMetaDataProvider() :
        XamlMetaDataProvider(make<RuntimeComponentWithStaticLibInApp::implementation::XamlMetaDataProvider>())
    {
    }
}
