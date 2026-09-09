// WARNING: Please don't edit this file...

void* winrt_make_BindTestbed_CastingModel()
{
    return winrt::detach_abi(winrt::make<winrt::BindTestbed::factory_implementation::CastingModel>());
}
WINRT_EXPORT namespace winrt::BindTestbed
{
    CastingModel::CastingModel() :
        CastingModel(make<BindTestbed::implementation::CastingModel>())
    {
    }
}
