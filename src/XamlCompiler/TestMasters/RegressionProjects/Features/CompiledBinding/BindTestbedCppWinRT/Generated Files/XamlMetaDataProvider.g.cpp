// WARNING: Please don't edit this file...

void* winrt_make_BindTestbed_XamlMetaDataProvider()
{
    return winrt::detach_abi(winrt::make<winrt::BindTestbed::factory_implementation::XamlMetaDataProvider>());
}
namespace winrt::BindTestbed
{
    XamlMetaDataProvider::XamlMetaDataProvider() :
        XamlMetaDataProvider(make<BindTestbed::implementation::XamlMetaDataProvider>())
    {
    }
}
