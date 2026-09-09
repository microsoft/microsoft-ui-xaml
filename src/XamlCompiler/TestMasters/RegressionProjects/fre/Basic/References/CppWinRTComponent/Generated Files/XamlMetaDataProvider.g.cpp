// WARNING: Please don't edit this file...

void* winrt_make_CppWinRTComponent_XamlMetaDataProvider()
{
    return winrt::detach_abi(winrt::make<winrt::CppWinRTComponent::factory_implementation::XamlMetaDataProvider>());
}
WINRT_EXPORT namespace winrt::CppWinRTComponent
{
    XamlMetaDataProvider::XamlMetaDataProvider() :
        XamlMetaDataProvider(make<CppWinRTComponent::implementation::XamlMetaDataProvider>())
    {
    }
}
