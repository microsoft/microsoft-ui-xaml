// WARNING: Please don't edit this file...

void* winrt_make_NoAppContainerProperty_XamlMetaDataProvider()
{
    return winrt::detach_abi(winrt::make<winrt::NoAppContainerProperty::factory_implementation::XamlMetaDataProvider>());
}
WINRT_EXPORT namespace winrt::NoAppContainerProperty
{
    XamlMetaDataProvider::XamlMetaDataProvider() :
        XamlMetaDataProvider(make<NoAppContainerProperty::implementation::XamlMetaDataProvider>())
    {
    }
}
