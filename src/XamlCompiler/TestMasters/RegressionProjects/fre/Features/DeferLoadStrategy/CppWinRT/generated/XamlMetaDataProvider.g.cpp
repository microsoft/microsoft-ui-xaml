// WARNING: Please don't edit this file...

void* winrt_make_CppWinRT_XamlMetaDataProvider()
{
    return winrt::detach_abi(winrt::make<winrt::CppWinRT::factory_implementation::XamlMetaDataProvider>());
}
WINRT_EXPORT namespace winrt::CppWinRT
{
    XamlMetaDataProvider::XamlMetaDataProvider() :
        XamlMetaDataProvider(make<CppWinRT::implementation::XamlMetaDataProvider>())
    {
    }
}
