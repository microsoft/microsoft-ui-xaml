// WARNING: Please don't edit this file...

void* winrt_make_MarkupExtensionsCppWinRT_XamlMetaDataProvider()
{
    return winrt::detach_abi(winrt::make<winrt::MarkupExtensionsCppWinRT::factory_implementation::XamlMetaDataProvider>());
}
WINRT_EXPORT namespace winrt::MarkupExtensionsCppWinRT
{
    XamlMetaDataProvider::XamlMetaDataProvider() :
        XamlMetaDataProvider(make<MarkupExtensionsCppWinRT::implementation::XamlMetaDataProvider>())
    {
    }
}
