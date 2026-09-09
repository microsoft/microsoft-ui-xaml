// WARNING: Please don't edit this file...

void* winrt_make_StaticControlsLib_XamlMetaDataProvider()
{
    return winrt::detach_abi(winrt::make<winrt::StaticControlsLib::factory_implementation::XamlMetaDataProvider>());
}
WINRT_EXPORT namespace winrt::StaticControlsLib
{
    XamlMetaDataProvider::XamlMetaDataProvider() :
        XamlMetaDataProvider(make<StaticControlsLib::implementation::XamlMetaDataProvider>())
    {
    }
}
