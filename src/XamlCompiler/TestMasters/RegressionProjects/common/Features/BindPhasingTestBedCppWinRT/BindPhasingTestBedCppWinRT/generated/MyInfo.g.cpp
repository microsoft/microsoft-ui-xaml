// WARNING: Please don't edit this file...

void* winrt_make_BindPhasingTestBedCppWinRT_MyInfo()
{
    return winrt::detach_abi(winrt::make<winrt::BindPhasingTestBedCppWinRT::factory_implementation::MyInfo>());
}
WINRT_EXPORT namespace winrt::BindPhasingTestBedCppWinRT
{
    MyInfo::MyInfo(param::hstring const& imageUrl, param::hstring const& caption) :
        MyInfo(make<BindPhasingTestBedCppWinRT::implementation::MyInfo>(imageUrl, caption))
    {
    }
}
