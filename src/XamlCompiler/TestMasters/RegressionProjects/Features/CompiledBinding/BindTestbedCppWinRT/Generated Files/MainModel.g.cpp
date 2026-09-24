// WARNING: Please don't edit this file...

void* winrt_make_BindTestbed_MainModel()
{
    return winrt::detach_abi(winrt::make<winrt::BindTestbed::factory_implementation::MainModel>());
}
namespace winrt::BindTestbed
{
    MainModel::MainModel() :
        MainModel(make<BindTestbed::implementation::MainModel>())
    {
    }
}
