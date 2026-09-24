// WARNING: Please don't edit this file...

void* winrt_make_BindTestbed_subfolder_SubDictionary()
{
    return winrt::detach_abi(winrt::make<winrt::BindTestbed::subfolder::factory_implementation::SubDictionary>());
}
namespace winrt::BindTestbed::subfolder
{
    SubDictionary::SubDictionary() :
        SubDictionary(make<BindTestbed::subfolder::implementation::SubDictionary>())
    {
    }
}
