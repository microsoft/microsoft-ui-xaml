// WARNING: Please don't edit this file...

void* winrt_make_ProviderCppWinRT_XamlMetaDataProvider()
{
    return winrt::detach_abi(winrt::make<winrt::ProviderCppWinRT::factory_implementation::XamlMetaDataProvider>());
}
WINRT_EXPORT namespace winrt::ProviderCppWinRT
{
    XamlMetaDataProvider::XamlMetaDataProvider() :
        XamlMetaDataProvider(make<ProviderCppWinRT::implementation::XamlMetaDataProvider>())
    {
    }
}
