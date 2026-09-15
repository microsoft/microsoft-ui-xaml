// WARNING: Please don't edit this file...

void* winrt_make_BindPhasingTestBedCppWinRT_XamlMetaDataProvider()
{
    return winrt::detach_abi(winrt::make<winrt::BindPhasingTestBedCppWinRT::factory_implementation::XamlMetaDataProvider>());
}
WINRT_EXPORT namespace winrt::BindPhasingTestBedCppWinRT
{
    XamlMetaDataProvider::XamlMetaDataProvider() :
        XamlMetaDataProvider(make<BindPhasingTestBedCppWinRT::implementation::XamlMetaDataProvider>())
    {
    }
}
