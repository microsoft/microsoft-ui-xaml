// WARNING: Please don't edit this file...

void* winrt_make_MultipleViewsTestbedCppWinRT_XamlMetaDataProvider()
{
    return winrt::detach_abi(winrt::make<winrt::MultipleViewsTestbedCppWinRT::factory_implementation::XamlMetaDataProvider>());
}
WINRT_EXPORT namespace winrt::MultipleViewsTestbedCppWinRT
{
    XamlMetaDataProvider::XamlMetaDataProvider() :
        XamlMetaDataProvider(make<MultipleViewsTestbedCppWinRT::implementation::XamlMetaDataProvider>())
    {
    }
}
