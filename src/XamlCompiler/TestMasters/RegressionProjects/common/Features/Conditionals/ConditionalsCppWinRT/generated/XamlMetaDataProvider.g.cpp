// WARNING: Please don't edit this file...

void* winrt_make_ConditionalsCppWinRT_XamlMetaDataProvider()
{
    return winrt::detach_abi(winrt::make<winrt::ConditionalsCppWinRT::factory_implementation::XamlMetaDataProvider>());
}
WINRT_EXPORT namespace winrt::ConditionalsCppWinRT
{
    XamlMetaDataProvider::XamlMetaDataProvider() :
        XamlMetaDataProvider(make<ConditionalsCppWinRT::implementation::XamlMetaDataProvider>())
    {
    }
}
