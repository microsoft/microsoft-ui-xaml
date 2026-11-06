// WARNING: Please don't edit this file...

void* winrt_make_CppWinRTExe_XamlMetaDataProvider()
{
    return winrt::detach_abi(winrt::make<winrt::CppWinRTExe::factory_implementation::XamlMetaDataProvider>());
}
WINRT_EXPORT namespace winrt::CppWinRTExe
{
    XamlMetaDataProvider::XamlMetaDataProvider() :
        XamlMetaDataProvider(make<CppWinRTExe::implementation::XamlMetaDataProvider>())
    {
    }
}
