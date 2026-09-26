// WARNING: Please don't edit this file...

void* winrt_make_LinkedMDAppCppWinRT_XamlMetaDataProvider()
{
    return winrt::detach_abi(winrt::make<winrt::LinkedMDAppCppWinRT::factory_implementation::XamlMetaDataProvider>());
}
WINRT_EXPORT namespace winrt::LinkedMDAppCppWinRT
{
    XamlMetaDataProvider::XamlMetaDataProvider() :
        XamlMetaDataProvider(make<LinkedMDAppCppWinRT::implementation::XamlMetaDataProvider>())
    {
    }
}
