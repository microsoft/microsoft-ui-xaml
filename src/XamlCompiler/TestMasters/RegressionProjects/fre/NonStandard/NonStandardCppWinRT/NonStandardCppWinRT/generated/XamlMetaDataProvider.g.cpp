// WARNING: Please don't edit this file...

void* winrt_make_NonStandardCppWinRT_XamlMetaDataProvider()
{
    return winrt::detach_abi(winrt::make<winrt::NonStandardCppWinRT::factory_implementation::XamlMetaDataProvider>());
}
WINRT_EXPORT namespace winrt::NonStandardCppWinRT
{
    XamlMetaDataProvider::XamlMetaDataProvider() :
        XamlMetaDataProvider(make<NonStandardCppWinRT::implementation::XamlMetaDataProvider>())
    {
    }
}
