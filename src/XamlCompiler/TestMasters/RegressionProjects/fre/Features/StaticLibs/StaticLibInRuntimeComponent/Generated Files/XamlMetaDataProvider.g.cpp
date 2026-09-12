// WARNING: Please don't edit this file...

void* winrt_make_StaticLibInRuntimeComponent_XamlMetaDataProvider()
{
    return winrt::detach_abi(winrt::make<winrt::StaticLibInRuntimeComponent::factory_implementation::XamlMetaDataProvider>());
}
WINRT_EXPORT namespace winrt::StaticLibInRuntimeComponent
{
    XamlMetaDataProvider::XamlMetaDataProvider() :
        XamlMetaDataProvider(make<StaticLibInRuntimeComponent::implementation::XamlMetaDataProvider>())
    {
    }
}
