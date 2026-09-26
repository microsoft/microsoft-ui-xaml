// WARNING: Please don't edit this file...

void* winrt_make_LinkedMDSubControlsCppWinRT_XamlMetaDataProvider()
{
    return winrt::detach_abi(winrt::make<winrt::LinkedMDSubControlsCppWinRT::factory_implementation::XamlMetaDataProvider>());
}
WINRT_EXPORT namespace winrt::LinkedMDSubControlsCppWinRT
{
    XamlMetaDataProvider::XamlMetaDataProvider() :
        XamlMetaDataProvider(make<LinkedMDSubControlsCppWinRT::implementation::XamlMetaDataProvider>())
    {
    }
}
