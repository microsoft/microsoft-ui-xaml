// WARNING: Please don't edit this file...

void* winrt_make_MetadataTestbedCppWinRT_XamlMetaDataProvider()
{
    return winrt::detach_abi(winrt::make<winrt::MetadataTestbedCppWinRT::factory_implementation::XamlMetaDataProvider>());
}
WINRT_EXPORT namespace winrt::MetadataTestbedCppWinRT
{
    XamlMetaDataProvider::XamlMetaDataProvider() :
        XamlMetaDataProvider(make<MetadataTestbedCppWinRT::implementation::XamlMetaDataProvider>())
    {
    }
}
