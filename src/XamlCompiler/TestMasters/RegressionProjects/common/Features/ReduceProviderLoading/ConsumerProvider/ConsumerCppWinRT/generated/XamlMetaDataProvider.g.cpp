// WARNING: Please don't edit this file...

void* winrt_make_ConsumerCppWinRT_XamlMetaDataProvider()
{
    return winrt::detach_abi(winrt::make<winrt::ConsumerCppWinRT::factory_implementation::XamlMetaDataProvider>());
}
WINRT_EXPORT namespace winrt::ConsumerCppWinRT
{
    XamlMetaDataProvider::XamlMetaDataProvider() :
        XamlMetaDataProvider(make<ConsumerCppWinRT::implementation::XamlMetaDataProvider>())
    {
    }
}
