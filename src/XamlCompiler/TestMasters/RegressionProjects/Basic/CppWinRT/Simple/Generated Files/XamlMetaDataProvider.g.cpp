// WARNING: Please don't edit this file...

void* winrt_make_Simple_XamlMetaDataProvider()
{
    return winrt::detach_abi(winrt::make<winrt::Simple::factory_implementation::XamlMetaDataProvider>());
}
namespace winrt::Simple
{
    XamlMetaDataProvider::XamlMetaDataProvider() :
        XamlMetaDataProvider(make<Simple::implementation::XamlMetaDataProvider>())
    {
    }
}
