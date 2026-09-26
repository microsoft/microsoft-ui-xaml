// WARNING: Please don't edit this file...

void* winrt_make_LinkedMDControlsCppWinRT_XamlMetaDataProvider()
{
    return winrt::detach_abi(winrt::make<winrt::LinkedMDControlsCppWinRT::factory_implementation::XamlMetaDataProvider>());
}
WINRT_EXPORT namespace winrt::LinkedMDControlsCppWinRT
{
    XamlMetaDataProvider::XamlMetaDataProvider() :
        XamlMetaDataProvider(make<LinkedMDControlsCppWinRT::implementation::XamlMetaDataProvider>())
    {
    }
}
